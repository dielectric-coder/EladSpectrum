#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define S_RATE 122880000
#define S_RATE_S1 61440000

static int debug=0;

//--------------------------------------------------------
static char str0[]="[General]\n\
configversion=2\n\
crashed=false\n\
\n\
[audio]\n\
gain=-200\n\
\n\
[fft]\n\
pandapter_min_db=-160\n\
\n\
[gui]\n\
geometry=@ByteArray(\\x1\\xd9\\xd0\\xcb\\0\\x2\\0\\0\\0\\0\\0\\0\\0\\0\\x1\\x85\\0\\0\\x3\\xbf\\0\\0\\x4(\\0\\0\\0\\x5\\0\\0\\x1\\xa3\\0\\0\\x3\\xba\\0\\0\\x4#\\0\\0\\0\\0\\0\\0\\0\\0\\a\\x80)\n\
//state=@ByteArray(\\0\\0\\0\\xff\\0\\0\\0\\0\\xfd\\0\\0\\0\\x2\\0\\0\\0\\x1\\0\\0\\x1m\\0\\0\\x2&\\xfc\\x2\\0\\0\\0\\x2\\xfc\\0\\0\\0\\x44\\0\\0\\x1]\\0\\0\\x1]\\0\\b\\0\\x1b\\xfa\\0\\0\\0\\x2\\x2\\0\\0\\0\\x3\\xfb\\0\\0\\0\\x18\\0\\x44\\0o\\0\\x63\\0k\\0I\\0n\\0p\\0u\\0t\\0\\x43\\0t\\0l\\x1\\0\\0\\0\\0\\xff\\xff\\xff\\xff\\0\\0\\0\\xf0\\x1\\0\\0\\x3\\xfb\\0\\0\\0\\x12\\0\\x44\\0o\\0\\x63\\0k\\0R\\0x\\0O\\0p\\0t\\x1\\0\\0\\0\\0\\xff\\xff\\xff\\xff\\0\\0\\x1\\x41\\0\\a\\xff\\xff\\xfb\\0\\0\\0\\xe\\0\\x44\\0o\\0\\x63\\0k\\0\\x46\\0\\x66\\0t\\x1\\0\\0\\0\\0\\xff\\xff\\xff\\xff\\0\\0\\0\\xc8\\0\\a\\xff\\xff\\xfc\\0\\0\\x1\\xa2\\0\\0\\0\\xc8\\0\\0\\0\\xc8\\0\\xff\\xff\\xff\\xfa\\0\\0\\0\\0\\x2\\0\\0\\0\\x2\\xfb\\0\\0\\0\\x12\\0\\x44\\0o\\0\\x63\\0k\\0\\x41\\0u\\0\\x64\\0i\\0o\\x1\\0\\0\\0\\0\\xff\\xff\\xff\\xff\\0\\0\0\\xc8\\x1\\0\\0\\x3\\xfb\\0\\0\\0\\xe\\0\\x44\\0o\\0\\x63\\0k\\0R\\0\\x44\\0S\\0\\0\\0\\0\\0\\xff\\xff\\xff\\xff\\0\\0\\0r\\x1\\0\\0\\x3\\0\\0\\0\\x3\\0\\0\\0\\0\\0\\0\\0\\0\\xfc\\x1\\0\\0\\0\\x1\\xfb\\0\\0\\0\\x1a\\0\\x44\\0o\\0\\x63\\0k\\0\\x42\\0o\\0o\\0k\\0m\\0\\x61\\0r\\0k\\0s\\0\\0\\0\\0\\0\\xff\\xff\\xff\\xff\\0\\0\\x1\\x42\\x1\\0\\0\\x3\\0\\0\\x2H\\0\\0\\x2&\\0\\0\\0\\x1\\0\\0\\0\\x2\\0\\0\\0\\b\\0\\0\\0\\x2\\xfc\\0\\0\\0\\x1\\0\\0\\0\\x2\\0\\0\\0\\x1\\0\\0\\0\\x16\\0m\\0\\x61\\0i\\0n\\0T\\0o\\0o\\0l\\0\\x42\\0\\x61\\0r\\x1\\0\\0\\0\\0\\xff\\xff\\xff\\xff\\0\\0\\0\\0\\0\\0\\0\\0)\n\
\n";

//--------------------------------------------------------
static char str1[]="[input]\n\
bandwidth=399999\n\
device=\"file=/tmp/elad,rate=1\"\n\
frequency=";

//--------------------------------------------------------
static char str2[]="\n\
sample_rate=";

//--------------------------------------------------------
static char str3[]="[receiver]\n\
demod=2\n\
\n\
[remote_control]\n\
enabled=true\n";

typedef struct _cbdata_t {
	int obj;
	struct libusb_transfer *transfer;
	float *outbuf;
	long *freq;
	int *atten;
	int *filter;
	int *sampling;
	int *bytes_per_sample;
	int rescale;
	int fifo;
} cbdata_t, *cbdata_p;

libusb_device_handle *dev_handle=NULL; 
static int isDuo = 0;
static int isS1 = 0;
static int isS2 = 0;
static float globalOffset, lpOffset, attOffset;
static float recalc, rescale;

void cb_in( struct libusb_transfer * transfer ) {
	static int isFirst=1;
	cbdata_p cbd;
	int res;
	int j;
	float c;
	short rs;
	int ri;
	static struct timeval start;
	static struct timeval stop;
	struct libusb_transfer * transfer_in;
	cbd = (cbdata_p)transfer->user_data;
	if( !cbd->obj ) {
		if( isFirst ) {
			gettimeofday( &(start), NULL );
			isFirst=0;
		}
		gettimeofday( &(stop), NULL );
		gettimeofday( &(start), NULL );
	}
	switch(transfer->status) {
		case LIBUSB_TRANSFER_COMPLETED:
                        if(*(cbd->bytes_per_sample)==2) {
                                c=recalc/32.0/1024.0;
                                for( j=0; j<transfer->actual_length/sizeof(short); j++ ) {
                                      rs=(short)((uint16_t *)(transfer->buffer))[j];
                                        cbd->outbuf[j]=rs*c;
                                }
                        } else {
                                c=recalc/2.0/1024.0/1024.0/1024.0;
                                for( j=0; j<transfer->actual_length/sizeof(int); j++ ) {
                                        ri=(int)((uint32_t *)(transfer->buffer))[j];
                                        cbd->outbuf[j]=ri*c;
                                }
                        }
			//write to fifo
			write( cbd->fifo, cbd->outbuf, j*sizeof(float) );
			break;
		case LIBUSB_TRANSFER_CANCELLED:
			if( debug ) fprintf( stderr, "CB result: Cancelled %d\n", cbd->obj );
			break;
		case LIBUSB_TRANSFER_NO_DEVICE:
			if( debug ) fprintf( stderr, "CB result: Nodevice %d\n", cbd->obj );
			break;
		case LIBUSB_TRANSFER_TIMED_OUT:
			if( debug ) fprintf( stderr, "CB result: Timedout %d\n", cbd->obj );
			break;
		case LIBUSB_TRANSFER_ERROR:
			if( debug ) fprintf( stderr, "CB result: Error %d\n", cbd->obj );
			break;
		case LIBUSB_TRANSFER_STALL:
			if( debug ) fprintf( stderr, "CB result: Stalled %d\n", cbd->obj );
			break;
		case LIBUSB_TRANSFER_OVERFLOW:
			if( debug ) fprintf( stderr, "CB result: Overflowed %d\n", cbd->obj );
			break;
	}
	fflush( stderr );
	transfer_in = cbd->transfer;
	res = libusb_submit_transfer( transfer_in );
	if( res ) {
		if( debug ) fprintf( stderr, "libusb_submit_transfer failed (%d)\n", res ); 
	}
}

void set_FREQ( long LOfreq ) {
	int res;
	unsigned char buffer[1024];
	int j;
	double tuningFreq;
	int sampleRateCorr;
	unsigned int tuningWordHex, twLS, twMS;

	// read global offset
	memset( buffer, 0, sizeof( buffer ) );
	res = libusb_control_transfer( dev_handle, 0xc0, 0xA2, 0x4028, 0x0151, buffer, 4, 0 );
	if( res==4  ){
		memcpy( &globalOffset, buffer, 4 );
		if( debug ) fprintf( stderr, "Global Offset: %f\n", globalOffset ); 
	} else {
		sampleRateCorr = 0;
		if( debug ) fprintf( stderr, "Global Offset failed (%d)\n", res ); 
		return;
	}

	// read lp offset
	memset( buffer, 0, sizeof( buffer ) );
	res = libusb_control_transfer( dev_handle, 0xc0, 0xA2, 0x402c, 0x0151, buffer, 4, 0 );
	if( res==4  ){
		memcpy( &lpOffset, buffer, 4 );
		if( debug ) fprintf( stderr, "LP Offset: %f\n", lpOffset ); 
	} else {
		sampleRateCorr = 0;
		if( debug ) fprintf( stderr, "LP Offset failed (%d)\n", res ); 
		return;
	}

	// read att offset
	memset( buffer, 0, sizeof( buffer ) );
	res = libusb_control_transfer( dev_handle, 0xc0, 0xA2, 0x4030, 0x0151, buffer, 4, 0 );
	if( res==4  ){
		memcpy( &attOffset, buffer, 4 );
		if( debug ) fprintf( stderr, "ATT Offset: %f\n", attOffset ); 
	} else {
		sampleRateCorr = 0;
		if( debug ) fprintf( stderr, "ATT Offset failed (%d)\n", res ); 
		return;
	}

	// read sample rate correction
	memset( buffer, 0, sizeof( buffer ) );
	res = libusb_control_transfer( dev_handle, 0xc0, 0xA2, 0x4024, 0x0151, buffer, 4, 0 );
	if( res==4  ){
		memcpy( &sampleRateCorr, buffer, 4 );
		if( debug ) fprintf( stderr, "Sample Rate Correction: %d\n", sampleRateCorr ); 
	} else {
		sampleRateCorr = 0;
		if( debug ) fprintf( stderr, "Sample rare correction failed (%d)\n", res ); 
		return;
	}

	if( isDuo ) {
		// set tuning frequency into FPGA
		memset( buffer, 0, sizeof( buffer ) );
		tuningFreq = LOfreq - (floor(LOfreq/(S_RATE+sampleRateCorr)) * (S_RATE+sampleRateCorr));
		tuningWordHex = (unsigned int)((4294967296.0*tuningFreq)/(S_RATE+sampleRateCorr));
		if( debug ) fprintf( stderr, "Tuning TuningHEX=%08X ", tuningWordHex );
		twLS = tuningWordHex&0x0000FFFF;
		tuningWordHex>>=16;
		twMS = 0xF2;
		twMS <<= 8;
		twMS |= (tuningWordHex&0x000000FF);
		tuningWordHex>>=8;
		buffer[0] = tuningWordHex&0x000000FF;
		buffer[1] = 0;
		res = libusb_control_transfer( dev_handle, 0x40, 0xE1, twLS, twMS, buffer, 2, 0 );
		if( res != 2 ) {
			if( debug ) fprintf( stderr, "Xilinx Frequency set failed\n" ); 
			return;
		}

		// verify that CAT buffer is free from previous commands
		memset( buffer, 0, sizeof( buffer ) );
		for( j=0; ; j++ ) {
			res = libusb_control_transfer( dev_handle, 0xc0, 0xE1, 0x00, 0x0FC<<8, buffer, 3, 0 );
			if( res != 3 || ((buffer[2]&0x04)!=0x04) || j==200 ) {
				break;
			}
			usleep( 10000 );
		} 
		if( res != 3 ) {
			if( debug ) fprintf( stderr, "CAT buffer waiting error\n" ); 
			return;
		}
		if ((j==200)&&(buffer[2]&0x04)==0x04) {
			if( debug ) fprintf( stderr, "CAT buffer waiting timeout\n" ); 
			return;
		}

		// set tuning frequency to ibe delivered to CAT
		memset( buffer, 0, sizeof( buffer ) );
		sprintf( (char *)buffer, "CF%11ld;", LOfreq );
		res = libusb_control_transfer( dev_handle, 0x40, 0xE1, 16, 0xF1<<8, buffer, 16, 0 );
		if( debug ) fprintf( stderr, "Duo set freq %ld\n", LOfreq );
	} else {
		// set tuning frequency into FPGA
		memset( buffer, 0, sizeof( buffer ) );
		tuningFreq = LOfreq - (floor(LOfreq/((isS1?S_RATE_S1:S_RATE)+sampleRateCorr)) * ((isS1?S_RATE_S1:S_RATE)+sampleRateCorr));
		tuningWordHex = (unsigned int)((4294967296.0*tuningFreq)/((isS1?S_RATE_S1:S_RATE)+sampleRateCorr));
		twLS = tuningWordHex&0x0000FFFF;
		tuningWordHex>>=16;
		twMS = tuningWordHex&0x0000FFFF;
		buffer[0] = tuningWordHex&0x000000FF;
		buffer[1] = 0;
		res = libusb_control_transfer( dev_handle, 0x40, 0xF2, twLS, twMS, buffer, 2, 0 );
		if( res != 2 ) {
			if( debug ) fprintf( stderr, "Xilinx Frequency set failed\n" ); 
		}
		if( debug ) fprintf( stderr, "S1/S2 set freq %ld\n", LOfreq );
	}
}

void *readfreq( void *a ) {

	FILE *fp;
	char buf[100];
	char *pp;
	long freq, oldfreq;
	freq=oldfreq=-1;
	(void)a;
	short port=7356;
        struct sockaddr_in saddr;
        struct hostent *server;
	int rc;
	int sfd;

	sfd = socket( AF_INET, SOCK_STREAM, 0 );
	if( sfd == -1 ) {
		if( debug ) fprintf( stderr, "Socket error %d\n", errno );
		exit( 0 );
	}
  
	for( ;; ) {
                server = gethostbyname( "localhost" );
                if (server == NULL) {
                        if( debug ) fprintf( stderr, "ERROR, no such host as \"localhost\"\n" );
                        exit( 0 );
                }

                memset( &saddr, 0, sizeof(saddr) );
                saddr.sin_family = AF_INET;
                saddr.sin_addr.s_addr = htonl( INADDR_ANY );
                memmove( (char *)&saddr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
                saddr.sin_port = htons( (unsigned short)port );

                /* connect: create a connection with the server */
                rc=connect( sfd, (struct sockaddr *)&saddr, sizeof( saddr ) );
                if( rc == -1 ) {
                        if( debug ) fprintf( stderr, "Connect Error" );
			sleep( 1 );
                        continue;
                }
		fp=fdopen( sfd, "r+" );
		for( ;; ) {
			rc=write( sfd, "f\n", 2 );
			if( rc==-1 ) {
				exit(0);
			}
			pp=fgets( buf, sizeof( buf ), fp );
			if( pp ) {
				freq=atol( pp );
			}
			if( freq != oldfreq ) {
				set_FREQ( freq );
				oldfreq=freq;
			}	
			usleep( 50000 );
		}
	}
}

void set_filter( long freq, int filter, int atten ) {
	int j;
	int res;
	unsigned char buffer[1024];
	unsigned int fl;
	if( isS1 ) {
		res = libusb_control_transfer( dev_handle, 0xc0, 0xF7, (unsigned int)filter, 0x0002, buffer, 1, 0 );
		if( res != 1 || buffer[0] != 0xF7 ) {
			if( debug ) fprintf( stderr, "Set filters failed (%d %02X)\n", res, buffer[0] ); 
		}
		if( debug ) fprintf( stderr, "S1 Filters set (%d)\n", filter );
		res = libusb_control_transfer( dev_handle, 0xc0, 0xF7, (unsigned int)atten, 0x0003, buffer, 1, 0 );
		if( res != 1 || buffer[0] != 0xF7 ) {
			if( debug ) fprintf( stderr, "Set attenuator failed (%d %02X)\n", res, buffer[0] ); 
		}
		if( debug ) fprintf( stderr, "S1 Attenuation set (%d)\n", atten );
	}
	if( isS2 ) {
		if( freq < 61440000 ) {
			fl=0x03;
		} else if( freq < 122880000 ) {
			fl=0x02;
		} else {
			fl=0x04;
		}
		if( !filter ) {
			fl = 0x01;
		}
		if( atten ) {
			fl |=0x08;
		}
		res = libusb_control_transfer( dev_handle, 0xc0, 0xF7, fl, 0x0002, buffer, 1, 0 );
		if( res != 1 || buffer[0] != 0xF7 ) {
			if( debug ) fprintf( stderr, "Set filters failed (%d %02X)\n", res, buffer[0] ); 
		}
		if( debug ) fprintf( stderr, "S2 Filters set (%d)\n", filter );
	}
	if( isDuo ) {
		// verify that CAT buffer is free from previous commands
		memset( buffer, 0, sizeof( buffer ) );
		for( j=0; ; j++ ) {
			res = libusb_control_transfer( dev_handle, 0xc0, 0xE1, 0x00, 0x0FC<<8, buffer, 3, 0 );
			if( res != 3 || ((buffer[2]&0x04)!=0x04) || j==200 ) {
				break;
			}
			usleep( 10000 );
		} 
		if( res != 3 ) {
			if( debug ) fprintf( stderr, "CAT buffer waiting error\n" ); 
			return;
		}
		if ((j==200)&&(buffer[2]&0x04)==0x04) {
			if( debug ) fprintf( stderr, "CAT buffer waiting timeout\n" ); 
			return;
		}
		// if( debug ) fprintf( stderr, "CAT buffer waiting ended (%d)\n", j ); 

		// set filter to be delivered to CAT
		memset( buffer, 0, sizeof( buffer ) );
		sprintf( (char *)buffer, "LP%1d;", filter );
		res = libusb_control_transfer( dev_handle, 0x40, 0xE1, 16, 0xF1<<8, buffer, 16, 0 );
		if( debug ) fprintf( stderr, "DUO LPF set (%d - %s -> %d)\n", filter, buffer, res );

		// set atten to be delivered to CAT
		memset( buffer, 0, sizeof( buffer ) );
		sprintf( (char *)buffer, "AT%1d;", atten );
		res = libusb_control_transfer( dev_handle, 0x40, 0xE1, 16, 0xF1<<8, buffer, 16, 0 );
		if( debug ) fprintf( stderr, "DUO Attenuation set (%d - %s -> %d)\n", atten, buffer, res );
	}
	recalc = pow( 10.0,((isDuo?21.4:0)+rescale+globalOffset+(filter?lpOffset:0)+(atten?attOffset:0))/20.0);
}

int main( int ac, char *av[], char **ev ) {

        libusb_context *ctx;
        libusb_device **devs;
        struct libusb_device_descriptor desc;
        int cnt, vid, pid;
        int j,k;

	int res;
	unsigned char buffer[1024];

	unsigned char pBuffer1[512*24];
	unsigned char pBuffer2[512*24];
	unsigned char pBuffer3[512*24];
	unsigned char pBuffer4[512*24];
	struct libusb_transfer *transfer_in1;
	struct libusb_transfer *transfer_in2;
	struct libusb_transfer *transfer_in3;
	struct libusb_transfer *transfer_in4;
	float pBuffer1a[512*12];
	float pBuffer2a[512*12];
	float pBuffer3a[512*12];
	float pBuffer4a[512*12];

	cbdata_t cbdata1;
	cbdata_t cbdata2;
	cbdata_t cbdata3;
	cbdata_t cbdata4;

	long LOfreq;
	static int atten;
	static int filter;
	static int sampling;
	long resampling;
	static int bytes_per_sample=4;

	static char cmd[2000];
	static char file[200];
	static int fifo;
        pthread_t pthread;
        pthread_attr_t attr;
	static int conf;
	static struct stat statbuf;	
	static char *str0a, *str1a, *str3a;
	static int str0l, str1l, str2l, str3l;
	static char buf[30000];
	char *pp;

	pp=getenv( "GQRX_DEBUG" );
	if( pp ) {
		debug=atoi( pp );
	}

        if( debug ) fprintf( stderr, "Operations init elad-gqrx version 1.1\n" );

	strcpy( cmd, "./elad-firmware " );
	if( ac>1 ) {
		strcat( cmd, "\"" );
		strcat( cmd, av[1] );
		strcat( cmd, "\" " );
	}
	if( ac>4 ) {
		strcat( cmd, "\"" );
		strcat( cmd, av[4] );
		strcat( cmd, "\" " );
	}
	if( debug ) fprintf( stderr, "launching %s\n", cmd );
	res=system( cmd );
	if( debug ) fprintf( stderr, "%s has returned %d\n", cmd, res );
	unlink( "/tmp/elad" );
	mkfifo( "/tmp/elad", 666 );
	system( "chmod 666 /tmp/elad" );

	// Read Central Frequency From Argument, if not 14200000
	if( ac<3 ) {
		LOfreq = 14200000;
	} else {
		sscanf( av[2], "%ld", &LOfreq );
	}
	if( debug ) fprintf( stderr, "Central Frequency: %10ld\n", LOfreq );

	// Read Filter + atten From Argument, if not 1/0
	if( ac<4 ) {
		filter = 1;
		atten = 0;
	} else {
		filter=av[3][0]-'0';
		atten=av[3][1]-'0';
	}
	if( debug ) fprintf( stderr, "filter: %d atten: %d\n", filter, atten );

	// Read samplerate From Argument, if not 192000
	if( ac<5 ) {
		sampling = 1;
	} else {
		sampling = atoi( av[4] );
	}
	if( debug ) fprintf( stderr, "sampling: %d\n", sampling );

	switch(sampling) {
		case 6:
			bytes_per_sample=2;
			rescale=-0.7;
			resampling=6144000;
			break;
		case 5:
			bytes_per_sample=4;
			rescale=5.4;
			resampling=3072000;
			break;
		case 4:
			bytes_per_sample=4;
			rescale=5.4;
			resampling=1536000;
			break;
		case 3:
			bytes_per_sample=4;
			rescale=6;
			resampling=768000;
			break;
		case 2:
			bytes_per_sample=4;
			rescale=6;
			resampling=384000;
			break;
		case 1:
			bytes_per_sample=4;
			rescale=0;
			resampling=192000;
			break;
		default:
			bytes_per_sample=4;
			rescale=0;
			resampling=192000;
			break;
	}
	if( debug ) fprintf( stderr, "resampling: %ld\n", resampling );

	system( "killall gqrx >/dev/null 2>/dev/null" );
	sleep( 1 );

	str0a=str0;
	str0l=sizeof( str0 )-1;
	str1a=str1;
	str1l=sizeof( str1 )-1;
	str2l=sizeof( str2 )-1;
	str3a=str3;
	str3l=sizeof( str3 )-1;
	sprintf( file, "%s/.config/gqrx/default.conf", getenv( "HOME" ) );
	conf=open( file,  O_RDONLY );
	if( conf > 0 ) {
		res = fstat( conf, &statbuf );
		if( res == 0 ) {
			res = read( conf, buf, statbuf.st_size );
			if( res == statbuf.st_size ) {
				for( j=0, k=0; j<res; j++ ) {
					if( k==0 && !strncmp( buf+j, "[input]", 7 ) ) {
						str0a=buf;
						str0l=j;
						k++;
						continue;
					}
					if( k==1 && !strncmp( buf+j, "[", 1 ) ) {
						str3a=buf+j;
						str3l=res-j;
						k++;
						break;
					}
				}
			} else {
				if( debug ) fprintf( stderr, "Conf file %s not read wanted=%ld get=%d\n", file, (long)statbuf.st_size, res );
			}
			close( conf );
		} else {
			if( debug ) fprintf( stderr, "Conf file %s not stat rc=%d error=%d\n", file, res, errno );
		}

	} else {
		if( debug ) fprintf( stderr, "Conf file %s not opened rc=%d error=%d\n", file, conf, errno );
	}
	
	sprintf( file, "%s/.config/gqrx/default.conf", getenv( "HOME" ) );
	conf=open( file,  O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH );
	if( conf > 0 ) {
		res=write( conf, str0a, str0l );
		if( debug ) fprintf( stderr, "Conf file %s (%-*.*s) %d bytes written on file %d error %d\n-----------------------\n", file, str0l, str0l, str0a, res, conf, errno );
		res=write( conf, str1a, str1l );
		if( debug ) fprintf( stderr, "Conf file %s (%-*.*s) %d bytes written on file %d error %d\n-----------------------\n", file, str1l, str1l, str1a, res, conf, errno );
		sprintf( cmd, "%ld", LOfreq );
		res=write( conf, cmd, strlen( cmd ) );
		if( debug ) fprintf( stderr, "Conf file %s (%s) %d bytes written on file %d error %d\n-----------------------\n", file, cmd, res, conf, errno );
		res=write( conf, str2, str2l );
		if( debug ) fprintf( stderr, "Conf file %s (%-*.*s) %d bytes written on file %d error %d\n-----------------------\n", file, str2l, str2l, str2, res, conf, errno );
		sprintf( cmd, "%ld\n\n", resampling );
		res=write( conf, cmd, strlen( cmd ) );
		if( debug ) fprintf( stderr, "Conf file %s (%s) %d bytes written on file %d error %d\n-----------------------\n", file, cmd, res, conf, errno );
		res=write( conf, str3a, str3l );
		if( debug ) fprintf( stderr, "Conf file %s (%-*.*s) %d bytes written on file %d error %d\n-----------------------\n", file, str3l, str3l, str3a, res, conf, errno );
		close( conf );
		if( debug ) fprintf( stderr, "Conf file %s %d bytes written on file %d\n", file,  res, conf );
	} else {
		if( debug ) fprintf( stderr, "Conf file %s not created rc=%d error=%d\n", file,  conf, errno );
	}
	
	sleep( 1 );
	//sprintf( cmd, "%s /Applications/Gqrx.app/Contents/MacOS/gqrx >/dev/null 2>/dev/null&", getenv( GQRX_PATH ) );
	sprintf( cmd, "%sgqrx >/dev/null 2>/dev/null&", getenv( "GQRX_PATH" )?getenv( "GQRX_PATH" ):"" );
	if( debug ) fprintf( stderr, "launching %s\n", cmd );
	res=system( cmd );
	if( debug ) fprintf( stderr, "%s has returned %d\n", cmd, res );
	fifo=open( "/tmp/elad", O_WRONLY );
	if( debug ) fprintf( stderr, "open fifo returned %d\n", fifo );
	// Initialize libusb-1.0
	res = libusb_init(&ctx);
	if( res < 0 ){
		if( debug ) fprintf( stderr, "Init Error %d\n", res );
		return 1;
	}

	// set verbosity level to 3, as suggested in the documentation
	libusb_set_debug( ctx, 3 );

	cnt = libusb_get_device_list(ctx, &devs);
	if(cnt < 0) {
		if( debug ) fprintf( stderr, "Get Device Error\n" ); 
		return 3;
	}
	for( j=0,k=0; j<cnt;j++ ) {
        	res = libusb_get_device_descriptor( devs[j], &desc );
		if( res != 0 ){
			if( debug ) fprintf( stderr, "Get Device Descriptor %d %d\n", j, res ); 
			return 4;
		}
		vid=desc.idVendor;
		pid=desc.idProduct;
		if( debug ) fprintf( stderr, "%d vid %04x pid %04x %d\n", j, vid, pid, k );
		if( vid==0x1721 && ( pid==0x0610 || pid==0x061c || pid==0x061a ) ) {
			res = libusb_open( devs[j], &dev_handle );
			if( res ) {
				if( debug ) fprintf( stderr, "Cannot open FDM Device\n" ); 
				return 6;
			} else {
				if( debug ) fprintf( stderr, "FDM Device Opened\n" ); 
			}

			// detach kernel drivers
			if( libusb_kernel_driver_active( dev_handle, 0 ) == 1 ) {
				if( debug ) fprintf( stderr, "Kernel Driver Active\n" ); 
				if( libusb_detach_kernel_driver( dev_handle, 0 ) == 0 ) {
					if( debug ) fprintf( stderr, "Kernel Driver Detached\n" ); 
				}
			}

			// claim interface
			if( libusb_claim_interface( dev_handle, 0 ) < 0 ) {
				if( debug ) fprintf( stderr, "Cannot claim Interface\n" ); 
				return 7;
			}
			if( debug ) fprintf( stderr, "Interface claimed\n" ); 

			// read USB driver version
			memset( buffer, 0, sizeof( buffer ) );
			res = libusb_control_transfer( dev_handle, 0xc0, 0xFF, 0x0000, 0x0000, buffer, 2, 0 );
			if( res  ){
				if( debug ) fprintf( stderr, "USB Driver Version: %d.%d\n", buffer[0], buffer[1] );
			} else {
				if( debug ) fprintf( stderr, "USB Driver Version read failed\n" );
			}

			// read HW version
			memset( buffer, 0, sizeof( buffer ) );
			res = libusb_control_transfer( dev_handle, 0xc0, 0xA2, 0x404C, 0x0151, buffer, 2, 0 );
			if( res==2  ){
				if( debug ) fprintf( stderr, "HW Version: %d.%d\n", buffer[0], buffer[1] );
			} else {
				if( debug ) fprintf( stderr, "HW Version read failed\n" );
			}

			// read serial number
			memset( buffer, 0, sizeof( buffer ) );
			res = libusb_control_transfer( dev_handle, 0xc0, 0xA2, 0x4000, 0x0151, buffer, 32, 0 );
			if( res==32  ){
				if( pid==0x061a ) {
					if( debug ) fprintf( stderr, "FDM DUO " ); 
					isDuo=1;
					isS1=0;
					isS2=0;
				}
				if( pid==0x061c ) {
					if( debug ) fprintf( stderr, "FDM S2 " ); 
					isDuo=0;
					isS1=0;
					isS2=1;
				}
				if( pid==0x0610 ) {
					if( debug ) fprintf( stderr, "FDM S1 " ); 
					isDuo=0;
					isS1=1;
					isS2=0;
				}
				if( debug ) fprintf( stderr, "Serial: %-s\n", buffer ); 
			} else {
				if( debug ) fprintf( stderr, "Serial read failed (%d)\n", res ); 
				return 10;
			}
			if( ac<2 || !strcmp( (const char *)buffer, (const char *)av[1] ) || av[1][0]=='+' ) {
				if( debug ) fprintf( stderr, "Device caught\n" ); 
				libusb_free_device_list( devs, 1 );
				if( debug ) fprintf( stderr, "libusb devices list freed\n" ); 
				break;
			} else {
				if( debug ) fprintf( stderr, "Get >%s<  Need >%s<\n", buffer, av[1] );
				libusb_close( dev_handle );
				dev_handle = NULL;
			}			
		}
		k++;
	}
	if( j==cnt ) {
		if( debug ) fprintf( stderr, "Cannot find FDM Device\n" ); 
		return 5;
	}
	if( !dev_handle ) {
		res = libusb_open( devs[j], &dev_handle );
		if( res ) {
			if( debug ) fprintf( stderr, "Cannot open FDM Device\n" ); 
			return 6;
		} else {
			if( debug ) fprintf( stderr, "FDM Device Opened\n" ); 
		}

		libusb_free_device_list( devs, 1 );
		if( debug ) fprintf( stderr, "libusb devices list freed\n" ); 

		// detach kernel drivers
		if( libusb_kernel_driver_active( dev_handle, 0 ) == 1 ) {
			if( debug ) fprintf( stderr, "Kernel Driver Active\n" ); 
			if( libusb_detach_kernel_driver( dev_handle, 0 ) == 0 ) {
				if( debug ) fprintf( stderr, "Kernel Driver Detached\n" ); 
			}
		}

		// claim interface
		if( libusb_claim_interface( dev_handle, 0 ) < 0 ) {
			if( debug ) fprintf( stderr, "Cannot claim Interface\n" ); 
			return 7;
		}
		if( debug ) fprintf( stderr, "Interface claimed\n" ); 
	}

	if( isDuo ) {

		// stop FIFO
		memset( buffer, 0, sizeof( buffer ) );
		res = libusb_control_transfer( dev_handle, 0xc0, 0xE1, 0x0000, 0xE9<<8, buffer, 1, 0 );
		// res = control_transfer( localDeviceInfo.winUsbHandle, 0xc0, 0xE1, 0x0000, 0x00E9<<8, buffer, 1, 0 );
		if( res==1  ){
			if( debug ) fprintf( stderr, "Stop FIFO\n" );
		} else {
			if( debug ) fprintf( stderr, "Stop FIFO failed (%d)\n", res );
		}

		// imposta la FIFO di EP6 del CY-USB in modalità slave...
		memset( buffer, 0, sizeof( buffer ) );
		res = libusb_control_transfer( dev_handle, 0xc0, 0xE1, 0x0000, 0xE8<<8, buffer, 1, 0 );
		if( res==1  ){
			if( debug ) fprintf( stderr, "Init FIFO\n" );
		} else {
			if( debug ) fprintf( stderr, "Init FIFO failed (%d)\n", res );
		}

		// start FIFO
		memset( buffer, 0, sizeof( buffer ) );
		res = libusb_control_transfer( dev_handle, 0xc0, 0xE1, 0x0001, 0x0E9<<8, buffer, 1, 0 );
		if( res != 1 || buffer[0] != 0xE9 ) {
			if( debug ) fprintf( stderr, "Enable SYN failed (%d %02X)\n", res, buffer[0] );
		}
		if( debug ) fprintf( stderr, "SYN Enabled\n" );
	} else {

		// start FIFO
		memset( buffer, 0, sizeof( buffer ) );
		res = libusb_control_transfer( dev_handle, 0xc0, 0xE9, 0x0001, 0x0000, buffer, 1, 0 );
		if( res != 1 || buffer[0] != 0xE9 ) {
			if( debug ) fprintf( stderr, "Enable SYN failed (%d %02X)\n", res, buffer[0] ); 
			return 13;
		}
		if( debug ) fprintf( stderr, "SYN Enabled\n" );
	}
	set_FREQ( LOfreq );
	set_filter( LOfreq, filter, atten );

	// prepare async transfer
	transfer_in1 = libusb_alloc_transfer(0);
	if( !transfer_in1 ) {
		if( debug ) fprintf( stderr, "libusb_alloc_transfer failed\n" ); 
		return 14;
	}
	transfer_in2 = libusb_alloc_transfer(0);
	if( !transfer_in2 ) {
		if( debug ) fprintf( stderr, "libusb_alloc_transfer failed\n" ); 
		return 15;
	}
	transfer_in3 = libusb_alloc_transfer(0);
	if( !transfer_in3 ) {
		if( debug ) fprintf( stderr, "libusb_alloc_transfer failed\n" ); 
		return 16;
	}
	transfer_in4 = libusb_alloc_transfer(0);
	if( !transfer_in4 ) {
		if( debug ) fprintf( stderr, "libusb_alloc_transfer failed\n" ); 
		return 17;
	}
	if( debug ) fprintf( stderr, "libusb_alloc_transfer succedeed\n" ); 

	cbdata1.obj=0;
	cbdata1.transfer=transfer_in1;
	cbdata1.outbuf=pBuffer1a;
	cbdata1.freq = &LOfreq;
	cbdata1.sampling = &sampling;
	cbdata1.atten = &atten;
	cbdata1.filter = &filter;
	cbdata1.bytes_per_sample = &bytes_per_sample;
	cbdata1.rescale = rescale;
	cbdata1.fifo = fifo;
	cbdata2.obj=1;
	cbdata2.transfer=transfer_in2;
	cbdata2.outbuf=pBuffer2a;
	cbdata2.freq = &LOfreq;
	cbdata2.sampling = &sampling;
	cbdata2.atten = &atten;
	cbdata2.filter = &filter;
	cbdata2.bytes_per_sample = &bytes_per_sample;
	cbdata2.rescale = rescale;
	cbdata2.fifo = fifo;
	cbdata3.obj=2;
	cbdata3.transfer=transfer_in3;
	cbdata3.outbuf=pBuffer3a;
	cbdata3.freq = &LOfreq;
	cbdata3.sampling = &sampling;
	cbdata3.atten = &atten;
	cbdata3.filter = &filter;
	cbdata3.bytes_per_sample = &bytes_per_sample;
	cbdata3.rescale = rescale;
	cbdata3.fifo = fifo;
	cbdata4.obj=3;
	cbdata4.transfer=transfer_in4;
	cbdata4.outbuf=pBuffer4a;
	cbdata4.freq = &LOfreq;
	cbdata4.sampling = &sampling;
	cbdata4.atten = &atten;
	cbdata4.filter = &filter;
	cbdata4.bytes_per_sample = &bytes_per_sample;
	cbdata4.rescale = rescale;
	cbdata4.fifo = fifo;

	libusb_fill_bulk_transfer( transfer_in1, dev_handle, 0x86, pBuffer1, 512*24, cb_in, &cbdata1, 2000);
	libusb_fill_bulk_transfer( transfer_in2, dev_handle, 0x86, pBuffer2, 512*24, cb_in, &cbdata2, 2000);
	libusb_fill_bulk_transfer( transfer_in3, dev_handle, 0x86, pBuffer3, 512*24, cb_in, &cbdata3, 2000);
	libusb_fill_bulk_transfer( transfer_in4, dev_handle, 0x86, pBuffer4, 512*24, cb_in, &cbdata4, 2000);
 
	pthread_attr_init( &attr );
	(void)pthread_create( &pthread, &attr, readfreq, NULL );
 
	res = libusb_submit_transfer( transfer_in1 );
	if( res ) {
		if( debug ) fprintf( stderr, "libusb_submit_transfer failed (%d)\n", res ); 
		return 18;
	}
	res = libusb_submit_transfer( transfer_in2 );
	if( res ) {
		if( debug ) fprintf( stderr, "libusb_submit_transfer failed (%d)\n", res ); 
		return 19;
	}
	res = libusb_submit_transfer( transfer_in3 );
	if( res ) {
		if( debug ) fprintf( stderr, "libusb_submit_transfer failed (%d)\n", res ); 
		return 20;
	}
	res = libusb_submit_transfer( transfer_in4 );
	if( res ) {
		if( debug ) fprintf( stderr, "libusb_submit_transfer failed (%d)\n", res ); 
		return 21;
	}
	if( debug ) fprintf( stderr, "libusb_submit_transfer succedeed\n" ); 

	if( debug ) fprintf( stderr, "SYN transfer starting\n" );

	// main loop 
	for( j=0, res=0; res==0 ; j++) {
		res = libusb_handle_events_completed( ctx, NULL );
	}
	if( debug ) fprintf( stderr, "libusb_handle events completed (%d)\n", res );

	// dirty exit
	if( debug ) fprintf( stderr, "Operations end\n" );

	return 0;
}
