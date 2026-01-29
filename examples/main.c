#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>

#define S_RATE 122880000

typedef struct _cbdata_t {
	int obj;
	struct libusb_transfer *transfer;
} cbdata_t, *cbdata_p;

void cb_in( struct libusb_transfer * transfer ) {
	cbdata_p cbd;
	int res;
	int j,k;
	struct libusb_transfer * transfer_in;
	cbd = (cbdata_p)transfer->user_data;
	switch(transfer->status) {
		case LIBUSB_TRANSFER_COMPLETED:
			fprintf( stderr, "CB result: Readed %d %d bytes\n", cbd->obj, transfer->actual_length );
			fprintf( stderr, "CB transfer type %d\n", transfer->type );
			for( j=0; j<transfer->actual_length && j<64; ) {
				for( k=0; k<16; j++,k++ ) {
					fprintf( stderr, "%02X ", transfer->buffer[j] );
				}
				fprintf( stderr, "\n" );
			}
			break;
		case LIBUSB_TRANSFER_CANCELLED:
			fprintf( stderr, "CB result: Cancelled %d\n", cbd->obj );
			break;
		case LIBUSB_TRANSFER_NO_DEVICE:
			fprintf( stderr, "CB result: Nodevice %d\n", cbd->obj );
			break;
		case LIBUSB_TRANSFER_TIMED_OUT:
			fprintf( stderr, "CB result: Timedout %d\n", cbd->obj );
			break;
		case LIBUSB_TRANSFER_ERROR:
			fprintf( stderr, "CB result: Error %d\n", cbd->obj );
			break;
		case LIBUSB_TRANSFER_STALL:
			fprintf( stderr, "CB result: Stalled %d\n", cbd->obj );
			break;
		case LIBUSB_TRANSFER_OVERFLOW:
			fprintf( stderr, "CB result: Overflowed %d\n", cbd->obj );
			break;
	}
	fflush( stderr );
	transfer_in = cbd->transfer;
	res = libusb_submit_transfer( transfer_in );
	if( res ) {
		fprintf( stderr, "libusb_submit_transfer failed (%d)\n", res ); 
	}
}

int main( int ac, char *av[], char **ev ) {

	libusb_context *ctx;
	libusb_device_handle *dev_handle; 

	int res;
	unsigned char buffer[1024];
	int j;
	long freq;
	long tune;
        double tuningFreq;
	int sampleRateCorr;
        unsigned int tuningWordHex, twLS, twMS;

	unsigned char pBuffer1[512*24];
	unsigned char pBuffer2[512*24];
	struct libusb_transfer *transfer_in1;
	struct libusb_transfer *transfer_in2;

	cbdata_t cbdata1;
	cbdata_t cbdata2;

	fprintf( stderr, "Operations init\n" );

	// Read Central Frequency From Argument, if not 14200000
	if( ac<2 ) {
		tune = 14200000;
	} else {
		sscanf( av[1], "%ld", &tune );
	}
	fprintf( stderr, "Central Frequency: %10ld\n", tune );

	// Initialize libusb-1.0
	res = libusb_init(&ctx);
	if( res < 0 ){
		fprintf( stderr, "Init Error %d\n", res ); 
		return 1;
	}

	// set verbosity level to 3, as suggested in the documentation
	libusb_set_debug( ctx, 3 );

	dev_handle = libusb_open_device_with_vid_pid( ctx, 0x1721, 0x061a );
	if( dev_handle == NULL ){
		fprintf( stderr, "Cannot open FDM DUO Device\n" ); 
		return 3;
	} else {
		fprintf( stderr, "FDM DUO Device Opened\n" ); 
	}

	// detach kernel drivers
	if( libusb_kernel_driver_active( dev_handle, 0 ) == 1 ) {
		fprintf( stderr, "Kernel Driver Active\n" ); 
		if( libusb_detach_kernel_driver( dev_handle, 0 ) == 0 ) {
			fprintf( stderr, "Kernel Driver Detached\n" ); 
		}
	}

	// claim interface
	if( libusb_claim_interface( dev_handle, 0 ) < 0 ) {
		fprintf( stderr, "Cannot claim Interface\n" ); 
		return 4;
	}
	fprintf( stderr, "Interface claimed\n" ); 

	// read USB driver version
	memset( buffer, 0, sizeof( buffer ) );
	res = libusb_control_transfer( dev_handle, 0xc0, 0xFF, 0x0000, 0x0000, buffer, 2, 0 );
	if( res  ){
		fprintf( stderr, "USB Driver Version: %d.%d\n", buffer[0], buffer[1] ); 
	} else {
		fprintf( stderr, "USB Driver Version read failed\n" ); 
		return 5;
	}

	// read HW version
	memset( buffer, 0, sizeof( buffer ) );
	res = libusb_control_transfer( dev_handle, 0xc0, 0xA2, 0x404C, 0x0151, buffer, 2, 0 );
	if( res==2  ){
		fprintf( stderr, "HW Version: %d.%d\n", buffer[0], buffer[1] ); 
	} else {
		fprintf( stderr, "HW Version read failed\n" ); 
		return 6;
	}

	// read serial number
	memset( buffer, 0, sizeof( buffer ) );
	res = libusb_control_transfer( dev_handle, 0xc0, 0xA2, 0x4000, 0x0151, buffer, 32, 0 );
	if( res==32  ){
		fprintf( stderr, "Serial: %-s\n", buffer ); 
	} else {
		fprintf( stderr, "Serial read failed (%d)\n", res ); 
		return 7;
	}

        // stop FIFO
        memset( buffer, 0, sizeof( buffer ) );
	res = libusb_control_transfer( dev_handle, 0xc0, 0xE1, 0x0000, 0xE9<<8, buffer, 1, 0 );
	// res = control_transfer( localDeviceInfo.winUsbHandle, 0xc0, 0xE1, 0x0000, 0x00E9<<8, buffer, 1, 0 );
	if( res==1  ){
		fprintf( stderr, "Stop FIFO\n" ); 
	} else {
		fprintf( stderr, "Stop FIFO failed (%d)\n", res ); 
		return 8;
	}

        // imposta la FIFO di EP6 del CY-USB in modalità slave...
        memset( buffer, 0, sizeof( buffer ) );
	res = libusb_control_transfer( dev_handle, 0xc0, 0xE1, 0x0000, 0xE8<<8, buffer, 1, 0 );
        // res = control_transfer( localDeviceInfo.winUsbHandle, 0xc0, 0xE1, 0x0000, 0x00E8<<8, buffer, 1, 0 );
	if( res==1  ){
		fprintf( stderr, "Init FIFO\n" ); 
	} else {
		fprintf( stderr, "Init FIFO failed (%d)\n", res ); 
		return 9;
	}

	// read sample rate correction
	memset( buffer, 0, sizeof( buffer ) );
	res = libusb_control_transfer( dev_handle, 0xc0, 0xA2, 0x4024, 0x0151, buffer, 4, 0 );
	if( res==4  ){
		memcpy( &sampleRateCorr, buffer, 4 );
		fprintf( stderr, "Sample Rate Correction: %d\n", sampleRateCorr ); 
	} else {
		sampleRateCorr = 0;
		fprintf( stderr, "Serial read failed (%d)\n", res ); 
		return 8;
	}

	// read tuning frequency from FPGA
	memset( buffer, 0, sizeof( buffer ) );
	res = libusb_control_transfer( dev_handle, 0xc0, 0xE1, 0x00, 0x0F5<<8, buffer, 11, 0 );
	if( res==11  ){
		freq = (int)buffer[1];
		freq <<= 8;
		freq |= (int)buffer[2];
		freq <<= 8;
		freq |= (int)buffer[3];
		freq <<= 8;
		freq |= (int)buffer[4];
		fprintf( stderr, "LO Frequency: %ld\n", freq ); 
	} else {
		fprintf( stderr, "LO Frequency read failed (%d)\n", res ); 
		return 9;
	}

	// set tuning frequency into FPGA
	memset( buffer, 0, sizeof( buffer ) );
        tuningFreq = tune - (floor(tune/(S_RATE+sampleRateCorr)) * (S_RATE+sampleRateCorr));
        tuningWordHex = (unsigned int)((4294967296.0*tuningFreq)/(S_RATE+sampleRateCorr));
	// fprintf( stderr, "Tuning TuningHEX=%08X ", tuningWordHex );
        twLS = tuningWordHex&0x0000FFFF;
        tuningWordHex>>=16;
        twMS = 0xF2;
        twMS <<= 8;
        twMS |= (tuningWordHex&0x000000FF);
        tuningWordHex>>=8;
        buffer[0] = tuningWordHex&0x000000FF;
        buffer[1] = 0;
	// fprintf( stderr, "twMS=%04X twLS=%04X Buffer[0]=%02X Buffer[1]=%02X\n", twMS, twLS, buffer[0], buffer[1] );
	res = libusb_control_transfer( dev_handle, 0x40, 0xE1, twLS, twMS, buffer, 2, 0 );
        // bRes=SendDatatoControlEndpoint(deviceHandle,VX_GENERIC,twLS,twMS,d,2,&byteSent);
	if( res == 2 ) {
		printf( "Xilinx frequency set\n" );
	} else {
		fprintf( stderr, "Xilinx Frequency set failed\n" ); 
		return 10;
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
		fprintf( stderr, "CAT buffer waiting error\n" ); 
		return 11;
	}
        if ((j==200)&&(buffer[2]&0x04)==0x04) {
		fprintf( stderr, "CAT buffer waiting timeout\n" ); 
		return 12;
        }
	fprintf( stderr, "CAT buffer waiting ended (%d)\n", j ); 

	// set tuning frequency to ibe delivered to CAT
	memset( buffer, 0, sizeof( buffer ) );
        sprintf( (char *)buffer, "CF%11ld;", tune );
	res = libusb_control_transfer( dev_handle, 0x40, 0xE1, 16, 0xF1<<8, buffer, 16, 0 );
        //SendDatatoControlEndpoint(deviceHandle,VX_GENERIC,16,index,buf,16,&byteSent);
        printf( "End setting cat frequency\n" );

	// start FIFO
	res = libusb_control_transfer( dev_handle, 0xc0, 0xE1, 0x0001, 0x0E9<<8, buffer, 1, 0 );
	if( res != 1 || buffer[0] != 0xE9 ) {
		fprintf( stderr, "Enable SYN failed (%d %02X)\n", res, buffer[0] ); 
		return 13;
	}
	fprintf( stderr, "SYN Enabled\n" );

	// parte copiata da libreria per S1 che funziona
	transfer_in1 = libusb_alloc_transfer(0);
	if( !transfer_in1 ) {
		fprintf( stderr, "libusb_alloc_transfer failed\n" ); 
		return 14;
	}
	transfer_in2 = libusb_alloc_transfer(0);
	if( !transfer_in2 ) {
		fprintf( stderr, "libusb_alloc_transfer failed\n" ); 
		return 15;
	}
	fprintf( stderr, "libusb_alloc_transfer succedeed\n" ); 

	cbdata1.obj=0;
	cbdata1.transfer=transfer_in1;
	cbdata2.obj=1;
	cbdata2.transfer=transfer_in2;

	libusb_fill_bulk_transfer( transfer_in1, dev_handle, 0x86, pBuffer1, 512*24, cb_in, &cbdata1, 2000);
	libusb_fill_bulk_transfer( transfer_in2, dev_handle, 0x86, pBuffer2, 512*24, cb_in, &cbdata2, 2000);
 
	res = libusb_submit_transfer( transfer_in1 );
	if( res ) {
		fprintf( stderr, "libusb_submit_transfer failed (%d)\n", res ); 
		return 16;
	}
	res = libusb_submit_transfer( transfer_in2 );
	if( res ) {
		fprintf( stderr, "libusb_submit_transfer failed (%d)\n", res ); 
		return 17;
	}
	fprintf( stderr, "libusb_submit_transfer succedeed\n" ); 

	fprintf( stderr, "SYN transfer starting\n" );

	// makes only 4 rounds of reading - enough to see something
	for( j=0, res=0; res==0 && j<4; j++) {
		res = libusb_handle_events_completed( NULL, NULL );
		fprintf( stderr, "libusb_handle events completed (%d)\n", res );
	}

	// dirty exit
	fprintf( stderr, "Operations end\n" );

	return 0;
}
