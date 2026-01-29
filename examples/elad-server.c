#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/shm.h>
#include <math.h>

typedef struct _payload {
	int fifor;
	int fifow;
	int port;
	char *portc;
	char *hostname;
	pthread_cond_t *con_a;
	pthread_cond_t *con_b;
	pthread_cond_t *con;
	pthread_mutex_t *mut;
	char *buf_a;
	int *lung_a;
	char *buf_b;
	int *lung_b;
	char *buf;
	int *lung;
	int buflen;
	int *fwd;
} payload, *payloadp;

static int sockfd;
static socklen_t salen;
static struct sockaddr *sa;

void *tcpManage( void *pay ) {
	int sfd;
	struct sockaddr_in saddr;
	struct hostent *server;
	char buf[1024];
	char buffer[1024];
	int j, n;
	int isFirst = 0;
	int closure = 0;
	int connected = 0;
	int fw;
	FILE *fr;
	payloadp payl=(payloadp)pay;
	int port = payl->port;
	char *hostname = payl->hostname;
	fprintf( stderr, "Parameters for TCP received\n" );

	for( ;; ) {
		// socket: create the socket 
		sfd = socket( AF_INET, SOCK_STREAM, 0 );
		if( sfd == -1 ) {
			fprintf( stderr, "Socket error\n" );
			exit( 1 );
		}
		fprintf( stderr, "Socket for TCP created\n" );

		server = gethostbyname( hostname );
		if (server == NULL) {
			fprintf( stderr, "ERROR, no such host as %s\n", hostname );
			exit( 1 );
		}

		memset( &saddr, 0, sizeof(saddr) );
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = htonl( INADDR_ANY );
		memmove( (char *)&saddr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
		saddr.sin_port = htons( (unsigned short)port );

		/* connect: create a connection with the server */
		if( connect( sfd, (struct sockaddr *)&saddr, sizeof( saddr ) ) == -1 ) {
			fprintf( stderr, "Connect Error\n" );
			sleep( 5 );
			continue;
		}
		connected = 1;
		fprintf( stderr, "Connected\n" );

		for( isFirst=0;; ) {
			if( isFirst==0 ) {
				sprintf( buf, "/tmp/fifo%d", payl->fifor );
				fr = fopen( buf, "r" );
				if( !fr ) {
					fprintf( stderr, "Error opening %s\n", buf );
					exit( 4 );
				}
				fprintf( stderr, "FIFO read opened\n" );
			}
			memset( buf, 0, sizeof( buf ) );
			fgets( buf, sizeof( buf ), fr );
			if( !strlen( buf ) ) {
				continue;
			}
			fprintf( stderr, "FIFO message received: <%s>\n", buf );
			n = write( sfd, buf, strlen( buf ) );
			if( n == -1 ) {
				fprintf( stderr, "TCP write Error\n" );
				exit( 8 );
			}
			fprintf( stderr, "TCP write %d bytes\n", n );
			for( closure = 0;; ) {
				memset( buf, 0, sizeof( buf ) );
				for( j=0, n=0; ; j++ ) {
					n = read( sfd, buf+j,  1 );
					if( n == 0 )  {
						fprintf( stderr, "TCP disconnected\n" );
						connected = 0;
						break;
					}
					if( n == -1 )  {
						fprintf( stderr, "TCP read Error\n" );
						break;
					}
					if( buf[j]=='\n' ) {
						fprintf( stderr, "TCP closure received\n" );
						write( fw, "\n", 1 );
						closure = 1;
						break;
					}
					if( buf[j]==';' ) {
						if( isFirst==0 ) {
							fprintf( stderr, "FIFO write to be opened\n" );
							sprintf( buffer, "/tmp/fifo%d", payl->fifow );
							fw = open( buffer, O_WRONLY );
							if( fw == -1 ) {
								fprintf( stderr, "Error opening %s\n", buffer );
								exit( 4 );
							}
							isFirst++;
							fprintf( stderr, "FIFO write opened\n" );
						}
						fprintf( stderr, "TCP message received: %s\n", buf );
						n = write( fw, buf, j+1 );
						if( n == -1 ) {
							fprintf( stderr, "FIFO write Error\n" );
						exit( 8 );
						}
						break;
					}
				}
				if( connected == 0 ) {
					fprintf( stderr, "TCP disconnected 1\n" );
					break;
				}
				if( closure == 1 ) {
					fprintf( stderr, "TCP closure exited 1\n" );
					break;
				}
			}
			if( connected == 0 ) {
				fprintf( stderr, "TCP disconnected 1\n" );
				break;
			}
		}
	}
	close( sockfd );

	return NULL;
}

void *udpReceive( void *pay ) {

	int sfd; 
	int res;
	int isFirst;
	int fw;
	unsigned int clilen;
	struct sockaddr_in saddr;
	struct sockaddr_in cliaddr;
	char buf[2048];
	int a = 0;
	int j;
	pthread_cond_t *con;
	char *buff;
	int *lung;
	int bufflen;
	static char recvBuffer[1024+32];
	static long counter=0;
	static long tempCounter=0;
	static long oldTempCounter=0;
	struct timeval start, stop;
	payloadp payl=(payloadp)pay;
	int port = payl->port;
	fprintf( stderr, "Parameters for UDP received\n" );

	// socket: create the socket
	sfd = socket( AF_INET, SOCK_DGRAM, 0 );
	if( sfd == -1 ) {
		fprintf( stderr, "socket error\n" );
		exit( 10 );
	}
	fprintf( stderr, "Socket for udp created\n" );

	// reuse socket
	int opt = 1;
	setsockopt( sfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof( int ) );

	// set server address
	memset( (char *)&saddr, 0, sizeof( saddr ) );
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons( (unsigned short)port );

	// associate socket to a port
	if( bind( sfd, (struct sockaddr *)&saddr, sizeof( saddr ) ) == -1 ) {
		fprintf( stderr, "binding Error\n" );
		exit( 11 );
	}
	fprintf( stderr, "Socket for udp binded\n" );

	gettimeofday( &start, NULL );
	for( isFirst=0;; ) {
		if( isFirst==0 ) {
			sprintf( buf, "/tmp/fifo%d", payl->fifow );
			fprintf( stderr, "Opening FIFO %s\n", buf );
			fw = open( buf, O_WRONLY );
			if( fw == -1 ) {
				fprintf( stderr, "Error opening %s\n", buf );
				exit( 4 );
			}
			*(payl->fwd)=fw;
			fprintf( stderr, "FIFO %s opened\n", buf );
			isFirst++;
		}
		// receive UDP datagrams
		a=1-a;
		if( a==1 ) {
			con=payl->con_a;
			buff=payl->buf_a;
			lung=payl->lung_a;
			bufflen=payl->buflen;
		} else {
			con=payl->con_b;
			buff=payl->buf_b;
			lung=payl->lung_b;
			bufflen=payl->buflen;
		}
		memset( buff, 0, bufflen );
		for( j=0, *lung=0; j<bufflen; j+=res-4 ) {
			res = recvfrom( sfd, recvBuffer, 1028, 0, (struct sockaddr *) &cliaddr, &clilen);
			memcpy( buff+j, recvBuffer+4, res-4 );
			tempCounter = ntohl ( *(long *)recvBuffer );
			counter++;
			if( counter!=tempCounter ) {
				gettimeofday( &stop, NULL );
				fprintf( stderr, "Counter (%ld) differs from received counter (%ld) diff %ld %%=%f time=%f\n", counter, tempCounter, tempCounter-counter, (tempCounter-counter)*100.0/(tempCounter-oldTempCounter), (stop.tv_sec-start.tv_sec)+0.000001*(stop.tv_usec-start.tv_usec) );
				counter=tempCounter;
				oldTempCounter=tempCounter;
				gettimeofday( &start, NULL );
			}
			*lung += res-4;
			if ( res == -1 ) {
				fprintf( stderr,"Recvfrom Error\n" );
				exit( 12 );
			}
		}
		pthread_cond_signal( con );
	}
	return NULL;
}

void *fifoSend( void *pay ) {

	payloadp payl=(payloadp)pay;
	int res;
	int j;
	for( ;; ) {
		pthread_cond_wait(payl->con, payl->mut);
		if( payl->con_a ) {
			pthread_cond_signal( payl->con_a );
		}
		for( j=0; j<*(payl->lung); j+=res ) {
			res = write( *(payl->fwd), payl->buf+j, (*(payl->lung))-j<1024?(*(payl->lung))-j:1024 );
			if( res == -1 ) {
				fprintf( stderr, "FIFO (a/b) write Error\n" );
				exit( 8 );
			}
		}

	}
	return NULL;
}

void *FFTSend( void *pay ) {

	double *co = NULL;
	double *si = NULL;
	double *fftr = NULL;
	double *ffti = NULL;
	double *coeff = NULL;
	unsigned char *fftout = NULL;
	int N;
	double co1, si1;
	double pi=3.141592653589793238462643383279502884197169399375105820974944592;
	double fftdr, fftdi;
	int j, k; 
	int m, p, md, ng, ig, i, ind, i1, i2, jd, kp;

	struct timeval start, stop;
	payloadp payl=(payloadp)pay;
	
	key_t shm_key = 6166529;
	const int shm_size = 1028;
	int shm_id;
	char* shmaddr;

	// Allocate and attach a shared memory segment
	shm_id = shmget (shm_key, shm_size, IPC_CREAT | S_IRUSR | S_IWUSR | 0666 );
	shmaddr = (char*) shmat (shm_id, 0, 0);
	fprintf( stderr, "Shared memory attached at address %p\n", shmaddr);

	p=10;
	N=1<<p;

	fftr = calloc( N, sizeof( double ) );
	if( fftr == NULL ) {
		fprintf( stderr, "error allocating fftr buffer" );
		exit( 5 );
	}
	ffti = calloc( N, sizeof( double ) );
	if( ffti == NULL ) {
		fprintf( stderr, "error allocating ffti buffer" );
		exit( 6 );
	}
	fftout = calloc( N, sizeof( char ) );
	if( fftout == NULL ) {
		fprintf( stderr, "error allocating fftout buffer" );
		exit( 7 );
	}
	co = calloc( N/2, sizeof( double ) );
	if( co == NULL ) {
		fprintf( stderr, "error allocating co buffer" );
		exit( 8 );
	}
	si = calloc( N/2, sizeof( double ) );
	if( si == NULL ) {
		fprintf( stderr, "error allocating si buffer" );
		exit( 9 );
	}
	coeff = calloc( N, sizeof( double ) );
	if( coeff == NULL ) {
		fprintf( stderr, "error allocating coeff buffer" );
		exit( 9 );
	}

	// coeff table initialization
	for( j=1; j<N; j++ ) {
		coeff[j]=0.35875-0.48829*cos(2*pi*j/(N-1))+0.14128*cos(4*pi*j/(N-1))-0.01168*cos(6*pi*j/(N-1));
	}

	// sin table initialization
	m = N/2;
	p = 10;
	co1 = cos( pi/m );
	si1 = -sin( pi/m );
	co[0] = 1;
	si[0] = 0;
	for( j=1; j<m; j++ ) {
		co[j] = co1 * co[j-1] - si1 * si[j-1];
		si[j] = si1 * co[j-1] + co1 * si[j-1];
	}

	for( ;; ) {
		pthread_cond_wait(payl->con, payl->mut);

		// Time measurement
		gettimeofday( &start, NULL );
		
		// Read input data 
		for( j=0; j<N; j++ ) {
			fftr[j]=((float *)payl->buf)[2*j]*coeff[j];
			ffti[j]=((float *)payl->buf)[2*j+1]*coeff[j];
		}

		m = N/2;
		p = 10;
			
		// FFT Compute
		for( j=0,ng=1,md=m; j<p; j++ ) {
			for( ind=0, ig=0; ig<ng; ig++ ) {
				for( i=0, k=0; i<md; i++ ) {
					i1=ind+i;
					i2=i1+md;
					fftdr = fftr[i1] - fftr[i2];
					fftdi = ffti[i1] - ffti[i2];
					fftr[i1] = fftr[i1] + fftr[i2];
					ffti[i1] = ffti[i1] + ffti[i2];
					fftr[i2] = fftdr * co[k] - fftdi * si[k];
					ffti[i2] = fftdr * si[k] + fftdi * co[k];
					k += ng;
				}
				ind += md*2;
			}
			md >>= 1;
			ng <<= 1;
		}

		// Coefficients reordering
		for( j=0; j<N; j++ ) {
			jd=j;
			k=0;
			kp=m;
			for( i=1; i<=p; i++ ) {
				k = k + (jd-(jd/2)*2)*kp;
				jd >>= 1;
				kp >>= 1;
			}
			if( k>j ) {
				fftdr = fftr[j];
				fftdi = ffti[j];
				fftr[j] = fftr[k];
				ffti[j] = ffti[k];
				fftr[k] = fftdr;
				ffti[k] = fftdi;
			}
		}

		// Absolute value
		for( j=0; j<N; j++ ) {
			fftr[j] = fftr[j]/m;
			ffti[j] = ffti[j]/m;
			fftr[j] = sqrt( fftr[j] * fftr[j] + ffti[j] * ffti[j] );
			fftout[j]=150+(char)( 20*log10(fftr[j]) );
		}

		// time measurement
		gettimeofday( &stop, NULL );
		// fprintf( stderr, "FFT time= %f mSec\n", (stop.tv_sec-start.tv_sec)*1000+0.001*(stop.tv_usec-start.tv_usec) );

		// FFT write out
		memcpy( shmaddr+4, fftout+512, 512 );
		memcpy( shmaddr+516, fftout, 512 );
		memset( shmaddr, 0x0001, 4 );
		usleep( 5000 );
		memset( shmaddr, 0, 4 );
		usleep( 5000 );
	}
	return NULL;
}

void *udpSend( void *pay ) {

	struct addrinfo hints, *rlist, *rlistsave;
	payloadp payl=(payloadp)pay;
	char *host = payl->hostname;
	int res;

	// initialize addrinfo structure
	memset( &hints, 0, sizeof(struct addrinfo) );
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_DGRAM;
	hints.ai_protocol=IPPROTO_UDP;

	res = getaddrinfo( host, payl->portc, &hints, &rlist );
	if( res == -1 ){
		fprintf( stderr, "udp error for %s, %s: %s", host, payl->portc, gai_strerror( res ) );
		exit( 1 );
	}
	rlistsave = rlist;
	for( ;; rlist=rlist->ai_next ) {
		sockfd=socket( rlist->ai_family, rlist->ai_socktype, rlist->ai_protocol );
		if( sockfd>= 0 ){
			break; 
		}
	}

	sa=malloc( rlist->ai_addrlen );
	memcpy(sa, rlist->ai_addr, rlist->ai_addrlen );
	salen=rlist->ai_addrlen;
	freeaddrinfo( rlistsave );
	fprintf( stderr, "UDP out prepared\n" );
/*
	for( k=0; k<y; k+=1472 ) {
		res = sendto( sockfd, ((char *)(cbd->outbuf))+k, y-k<1472?y-k:1472, 0, sa, salen );
		if( res < 0 ) {
			fprintf( stderr, "CB UDP error (%d)\n", res  );
		}
	}
*/
	return NULL;
}

int main( int ac, char *av[], char **ev ) {

        int j;
	int res;

	char *host, *port;
	int portn, fifo;
	char fifoname[100];

        pthread_t pthread0;
        pthread_attr_t attr0;
	struct sched_param prio0;
	payload payl0;
        pthread_t pthread1;
        pthread_attr_t attr1;
	payload payl1;
        pthread_t pthread2;
        pthread_attr_t attr2;
	payload payl2;
        pthread_t pthread3;
        pthread_attr_t attr3;
	struct sched_param prio3;
	payload payl3;
        pthread_t pthread4;
        pthread_attr_t attr4;
	struct sched_param prio4;
	payload payl4;
        pthread_t pthread5;
        pthread_attr_t attr5;
	struct sched_param prio5;
	payload payl5;
	int fwd;

	pthread_cond_t con_a = PTHREAD_COND_INITIALIZER;
	pthread_cond_t con_b = PTHREAD_COND_INITIALIZER;
	pthread_cond_t con_c = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t mut_a = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t mut_b = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t mut_c = PTHREAD_MUTEX_INITIALIZER;

	static char buf_a[512*24];
	static char buf_b[512*24];
	int lung_a;
	int lung_b;

        fprintf( stderr, "Operations init elad-server version 1.0\n" );

	// Read host from Argument, if not localhost
	if( ac<2 ) {
		host = "localhost";
	} else {
		host = av[1];
	}
	fprintf( stderr, "host: %s\n", host );

	// Read portnumber from Argument, if not 6666
	if( ac<3 ) {
		port = "6666";
	} else {
		port = av[2];
	}
	portn=atoi( port );
	fprintf( stderr, "portnumber: %s (%d)\n", port, portn );

	// Read fifo base number from Argument, if not 1
	if( ac<4 ) {
		fifo = 1;
	} else {
		fifo = atoi( av[3] );
	}
	portn=atoi( port );
	fprintf( stderr, " fifo base number: %d\n", fifo );

	// open fifo to send/receive commands from radio and for receive/send udp data from radio
	for( j=0; j<4; j++ ) {
		sprintf( fifoname, "/tmp/fifo%d", fifo+j );
		res = mkfifo( fifoname, 0666 );
		if( res == -1 && errno != 17 ) {
			fprintf( stderr, "Fifo %d creating error %d %s\n", j, errno, strerror( errno ) );	
			return 1;
		}
	}

	// create thread that reads data using udp
	memset( &payl0, 0, sizeof( payl0 ) );
	payl0.fifow=fifo;
	payl0.fifor=fifo;
	payl0.port=portn;
	payl0.portc=port;
	payl0.hostname=host;
	payl0.con_a=&con_a;
	payl0.con_b=&con_b;
	payl0.buf_a=buf_a;
	payl0.lung_a=&lung_a;
	payl0.buf_b=buf_b;
	payl0.lung_b=&lung_b;
	payl0.buflen=sizeof( buf_a );
	payl0.fwd=&fwd;

	prio0.sched_priority=99;
	res=pthread_attr_init( &attr0 );
	if( res ) {
		fprintf( stderr, "pthread_attr_init attr0 failed\n" );
	}
	res=pthread_create( &pthread0, &attr0, udpReceive, &payl0 );
	if( res ) {
		fprintf( stderr, "pthread_create pthread 0 failed\n" );
	}
	res=pthread_setschedparam( pthread0, SCHED_RR, &prio0 );
	if( res ) {
		fprintf( stderr, "pthread_setschedparam pthread 0 failed\n" );
	}
	fprintf( stderr, "udp Receive thread created\n" ); 

	// create thread that sends commands using tcp
	memset( &payl1, 0, sizeof( payl1 ) );
	payl1.fifow=fifo+1;
	payl1.fifor=fifo+2;
	payl1.port=portn;
	payl1.portc=port;
	payl1.hostname=host;
	res=pthread_attr_init( &attr1 );
	if( res ) {
		fprintf( stderr, "pthread_attr_init attr1 failed\n" );
	}
	res=pthread_create( &pthread1, &attr1, tcpManage, (void *)&payl1 );
	if( res ) {
		fprintf( stderr, "pthread_create pthread 1 failed\n" );
	}
	fprintf( stderr, "Tcp manage thread created\n" ); 

	// create thread that sends data via udp
	memset( &payl2, 0, sizeof( payl2 ) );
	payl2.fifow=fifo+3;
	payl2.fifor=fifo+3;
	payl2.port=portn;
	payl2.portc=port;
	payl2.hostname=host;
	res=pthread_attr_init( &attr2 );
	if( res ) {
		fprintf( stderr, "pthread_attr_init attr2 failed\n" );
	}
	res=pthread_create( &pthread2, &attr2, udpSend, (void *)&payl2 );
	if( res ) {
		fprintf( stderr, "pthread_create pthread 2 failed\n" );
	}
	fprintf( stderr, "udp Send thread created\n" ); 

	// create thread that sends data to the fifo
	memset( &payl3, 0, sizeof( payl3 ) );
	payl3.fifow=fifo;
	payl3.fifor=fifo;
	payl3.port=portn;
	payl3.portc=port;
	payl3.hostname=host;
	payl3.con_a=&con_c;
	payl3.con=&con_a;
	payl3.mut=&mut_a;
	payl3.buf=buf_a;
	payl3.lung=&lung_a;
	payl3.buflen=sizeof( buf_a );
	payl3.fwd=&fwd;
	prio3.sched_priority=80;
	res=pthread_attr_init( &attr3 );
	if( res ) {
		fprintf( stderr, "pthread_attr_init attr3 failed\n" );
	}
	res=pthread_create( &pthread3, &attr3, fifoSend, (void *)&payl3 );
	if( res ) {
		fprintf( stderr, "pthread_create pthread 3 failed\n" );
	}
	res=pthread_setschedparam( pthread3, SCHED_RR, &prio3 );
	if( res ) {
		fprintf( stderr, "pthread_setschedparam pthread 3 failed\n" );
	}
	fprintf( stderr, "fifo Send a thread created\n" ); 

	// create thread that sends data to the fifo
	memset( &payl4, 0, sizeof( payl4 ) );
	payl4.fifow=fifo;
	payl4.fifor=fifo;
	payl4.port=portn;
	payl4.portc=port;
	payl4.hostname=host;
	payl4.con=&con_b;
	payl4.mut=&mut_b;
	payl4.buf=buf_b;
	payl4.lung=&lung_b;
	payl4.buflen=sizeof( buf_a );
	payl4.fwd=&fwd;
	prio4.sched_priority=80;
	res=pthread_attr_init( &attr4 );
	if( res ) {
		fprintf( stderr, "pthread_attr_init attr4 failed\n" );
	}
	res=pthread_create( &pthread4, &attr4, fifoSend, (void *)&payl4 );
	if( res ) {
		fprintf( stderr, "pthread_create pthread 4 failed\n" );
	}
	res=pthread_setschedparam( pthread4, SCHED_RR, &prio4 );
	if( res ) {
		fprintf( stderr, "pthread_setschedparam pthread 4 failed\n" );
	}
	fprintf( stderr, "fifo Send b thread created\n" ); 

	// create thread that sends FFT to the fifo
	memset( &payl5, 0, sizeof( payl5 ) );
	payl5.fifow=fifo+3;
	payl5.fifor=fifo+3;
	payl5.port=portn;
	payl5.portc=port;
	payl5.hostname=host;
	payl5.con=&con_c;
	payl5.mut=&mut_c;
	payl5.buf=buf_a;
	payl5.lung=&lung_a;
	payl5.buflen=sizeof( buf_a );
	payl5.fwd=&fwd;
	prio5.sched_priority=80;
	res=pthread_attr_init( &attr5 );
	if( res ) {
		fprintf( stderr, "pthread_attr_init attr5 failed\n" );
	}
	res=pthread_create( &pthread5, &attr5, FFTSend, (void *)&payl5 );
	if( res ) {
		fprintf( stderr, "pthread_create pthread 5 failed\n" );
	}
	res=pthread_setschedparam( pthread5, SCHED_RR, &prio5 );
	if( res ) {
		fprintf( stderr, "pthread_setschedparam pthread 5 failed\n" );
	}
	fprintf( stderr, "fifo Send FFT thread created\n" ); 

	// main loop 
	for( ;; ) {
		sleep( 10 );
	}

	// dirty exit
	fprintf( stderr, "Operations end\n" );

	return 0;
}
