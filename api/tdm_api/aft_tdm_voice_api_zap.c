/*****************************************************************************
* aft_api.c	AFT T1/E1: HDLC API Sample Code
*
* Author(s):	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2003-2004 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*/

#if 0
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_wanpipe.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <signal.h>
#endif

#define WAN_IFNAME_SZ 100

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "lib_api.h"

#define DAHDI_ISSUES 1
#include "zapcompat_user.h"

#define MAX_TX_DATA     15000	/* Size of tx data */  
#define MAX_FRAMES 	5000     /* Number of frames to transmit */  

#define MAX_RX_DATA	15000

#define BLOCK_SIZE 160 

unsigned short Rx_lgth;

unsigned char Rx_data[MAX_RX_DATA];
unsigned char Tx_data[MAX_TX_DATA ];

ZT_PARAMS tp;

/* Prototypes */
int MakeConnection(int);
void handle_span_chan( void);
void sig_end(int sigid);

int dev_fd;
FILE *tx_fd=NULL,*rx_fd=NULL;	

int zap_ec_ctrl(int sock, int tap)
{
	struct dahdi_echocanparams ecp = { .tap_length = tap };
    int res;

	res=ioctl(sock, DAHDI_ECHOCANCEL_PARAMS, &ecp);  
	if (res) {
		fprintf(stderr, "Failed to configure ec tap=%i: %s\n", tap, strerror(errno));
	}

	return res;
}


/***************************************************
* HANDLE SOCKET 
*
*   o Read a socket 
*   o Cast data received to api_rx_element_t data type 
*   o The received packet contains 16 bytes header 
*
*	------------------------------------------
*      |  sizeof(wp_api_hdr_t) bytes      |        X bytes        ...
*	------------------------------------------
* 	   Header              Data
*
* o Data structures:
* ------------------
* typedef struct {
* 	union {
* 		struct {
* 			unsigned char	_event_type;
* 			unsigned char	_rbs_rx_bits;
* 			unsigned int	_time_stamp;
* 		}wp_event;
* 		struct {
* 			unsigned char	_rbs_rx_bits;
* 			unsigned int	_time_stamp;
* 		}wp_rx;
* 		unsigned char	reserved[sizeof(wp_api_hdr_t)];
* 	}wp_rx_hdr_u;
* #define wp_api_event_type		wp_rx_hdr_u.wp_event._event_type
* #define wp_api_event_rbs_rx_bits 	wp_rx_hdr_u.wp_event._rbs_rx_bits
* #define wp_api_event_time_stamp 	wp_rx_hdr_u.wp_event._time_stamp
* } wp_tdm_api_rx_hdr_t;
* 
* typedef struct {
*         wp_tdm_api_rx_hdr_t	hdr;
*         unsigned char  		data[1];
* } wp_tdm_api_rx_element_t;
* 
* typedef struct {
* 	union {
* 		struct {
* 			unsigned char	_rbs_rx_bits;
* 			unsigned int	_time_stamp;
* 		}wp_tx;
* 		unsigned char	reserved[sizeof(wp_api_hdr_t)];
* 	}wp_tx_hdr_u;
* #define wp_api_time_stamp 	wp_tx_hdr_u.wp_tx._time_stamp
* } wp_tdm_api_tx_hdr_t;
* 
* typedef struct {
*         wp_tdm_api_tx_hdr_t	hdr;
*         unsigned char  		data[1];
* } wp_tdm_api_tx_element_t;
*                     
* #define WPTDM_A_BIT 0x08
* #define WPTDM_B_BIT 0x04
* #define WPTDM_C_BIT 0x02
* #define WPTDM_D_BIT 0x01
*
*/

void print_packet(unsigned char *buf, int len)
{
	int x;
	printf("{  | ");
	for (x=0;x<len;x++){
		if (x && x%24 == 0){
			printf("\n  ");
		}
		if (x && x%8 == 0)
			printf(" | ");
		printf("%02x ",buf[x]);
	}
	printf("}\n");
}

void handle_span_chan(void) 
{
	unsigned int Rx_count,Tx_count,Tx_length;
	fd_set 	ready,fd_write,oob;
	int err,i;
	
#if 0
	int rlen;
	int stream_sync=0;
#endif
	
	Rx_count = 0;
	Tx_count = 0;

    Tx_length = tx_size;
	
	printf("\n\nSocket Handler: Rx=%d Tx=%i TxCnt=%i TxLen=%i TxDelay=%i\n",
			read_enable,write_enable,tx_cnt,tx_size,tx_delay);	

	/* Initialize the Tx Data buffer */
	memset(&Tx_data[0],0,MAX_TX_DATA );

	/* Cast the Tx data packet with the tx element
	 * structure.  We must insert a sizeof(wp_api_hdr_t) byte
	 * driver header, which driver will remove 
	 * before passing packet out the physical port */
	

	/* Create a Tx packet based on user info, or
	 * by deafult incrementing number starting from 0 */
	for (i=0;i<Tx_length;i++){
		if (tx_data == -1){
			Tx_data[i] = (unsigned char)i;
		}else{
#if 0
			Tx_data[i] = (unsigned char)tx_data+(i%4);
#else
			Tx_data[i] = (unsigned char)tx_data;
#endif
		}
	}

	/* Main Rx Tx OOB routine */
	for(;;) {	

		/* Initialize all select() descriptors */
		FD_ZERO(&ready);
		FD_ZERO(&fd_write);
		FD_ZERO(&oob);
		FD_SET(dev_fd,&oob);
		FD_SET(dev_fd,&ready);

		if (write_enable){
		     FD_SET(dev_fd,&fd_write);
		}

		/* Select will block, until:
		 * 	1: OOB event, link level change
		 * 	2: Rx data available
		 * 	3: Interface able to Tx */
		
  		if(select(dev_fd + 1,&ready, &fd_write, &oob, NULL)){

			fflush(stdout);	
          	if (FD_ISSET(dev_fd,&ready)){

				/* An Rx packet is pending
				 * 	1: Read the rx packet into the Rx_data
				 * 	   buffer. Confirm len > 0
				 *
				 * 	2: Cast Rx_data to the api_rx_element.
				 * 	   Thus, removing a sizeof(wp_api_hdr_t) byte header
				 * 	   attached by the driver.
				 *
				 * 	3. Check error_flag:
				 * 		CRC,Abort..etc 
				 */

				memset(Rx_data, 0, sizeof(Rx_data));

				err = read(dev_fd, Rx_data, sizeof(Rx_data));

				if (!read_enable){
					goto bitstrm_skip_read;
				}

				/* err indicates bytes received */
				if(err <= 0) {
					printf("\nError receiving data\n");
					break;
				}


				/* Check the packet length */
				Rx_lgth = err;
				if(Rx_lgth<=0) {
					printf("\nShort frame received (%d)\n",
						Rx_lgth);
					return;
				}

#if 0
				if (api_rx_el->data[0] == tx_data && api_rx_el->data[1] == (tx_data+1)){
					if (!stream_sync){
						printf("GOT SYNC %x\n",api_rx_el->data[0]);
					}
					stream_sync=1;
				}else{
					if (stream_sync){
						printf("OUT OF SYNC: %x\n",api_rx_el->data[0]);
					}
				}
#endif					

				++Rx_count;

				if (verbose){
					printf("Received %i Length = %i\n", 
							Rx_count,Rx_lgth);
					printf("Data: ");
					print_packet(Rx_data,Rx_lgth);
				}else{
					//putchar('R');
				}

				if (rx_cnt > 0  && Rx_count >= rx_cnt){
					break;
				}
bitstrm_skip_read:
;
		   	}
		
		   	if (FD_ISSET(dev_fd,&fd_write)){


				err = write(dev_fd,Tx_data,Tx_length);

				if (err <= 0){
					if (errno == EBUSY){
						if (verbose){
							printf("Sock busy try again!\n");
						}
						/* Socket busy try sending again !*/
					}else{
						printf("Faild to send %i \n",errno);
						perror("Send: ");
						break;
					}
				}else{

					++Tx_count;
					
					if (verbose){
						printf("Packet sent: Sent %i : %i\n",
							err,Tx_count);
					}else{
						//putchar('T');
					}
				}
		   	}

			if (tx_delay){
				usleep(tx_delay);
			}

			if (tx_cnt && tx_size && Tx_count >= tx_cnt && !(files_used & TX_FILE_USED)){
			
				write_enable=0;
				if (rx_cnt > 0){
					/* Dont break let rx finish */
				}else{
					break;
				}
			}
		}
	}

	if (tx_fd){
		fclose(tx_fd);
	}
	if (rx_fd){
		fclose(rx_fd);
	}
	close (dev_fd);
} 

int rbs_event (int fd, unsigned char rbs_bits)
{
	printf("%d: GOT RBS EVENT: RBS BITS=0x%X\n",
			fd,rbs_bits);
	return 0;
}


int MakeConnection(int chan) 
{
	int i;
	int sock;
    ZT_BUFFERINFO bi; 
	char chan_name[100];


#ifdef DAHDI_ISSUES
	sprintf(chan_name,"/dev/dahdi/%d",chan);
#else
	sprintf(chan_name,"/dev/zap/%d",chan);
#endif

	printf("Opening chan %i  %s\n",chan,chan_name);

#ifdef DAHDI_ISSUES
	sock = open("/dev/dahdi/channel", O_RDWR | O_NONBLOCK, 0600);
#else
	sock = open("/dev/zap/channel", O_RDWR | O_NONBLOCK, 0600);
#endif
	if( sock < 0 ) {
		perror("Open Span Chan: ");
		return -1 ;
	}

	if (ioctl(sock, ZT_SPECIFY, &chan)) {
		perror("Open Span Chan: ");
		close(sock);
		sock=-1;
		return -1;
	}


	if (tp.sigtype != ZT_SIG_HARDHDLC) { 
		if (ioctl(sock, ZT_SET_BLOCKSIZE, &tx_size)) {
			fprintf(stderr, "Unable to set block size to %d: %s\n", tx_size, strerror(errno));
			return -1;
		}      
	}

    if (ioctl(sock, ZT_GET_PARAMS, &tp)) {
		fprintf(stderr, "Unable to get channel parameters\n");
		return -1;
	}

	if (tp.sigtype == ZT_SIG_HARDHDLC) {
		printf("HARDWARE HDLC %s\n",chan_name);
        bi.txbufpolicy = ZT_POLICY_IMMEDIATE;
		bi.rxbufpolicy = ZT_POLICY_IMMEDIATE;
		bi.numbufs = 32;
		bi.bufsize = 1024;
		if (ioctl(sock, ZT_SET_BUFINFO, &bi)) {
			fprintf(stderr, "Unable to set appropriate buffering on channel %s\n", chan_name);
			return -1;
		}                                      
	}
	ioctl(sock, ZT_GETEVENT);

	i = ZT_FLUSH_ALL;
	if (ioctl(sock,ZT_FLUSH,&i) == -1){
		perror("tor_flush");
		return -1;
	}                          

#if 0
	if (tp.sigtype != ZT_SIG_HARDHDLC) {
		printf("Setting AUDIO MODE\n");
		if (ioctl(sock, DAHDI_AUDIOMODE, &i)) {
			printf("Error Failed to set Auido mode %s\n\n",chan_name);
			return -1;
		}

		zap_ec_ctrl(sock,0);
		if (zap_ec_ctrl(sock,32) != 0) {
			return -1;
		}
		printf("dev=%s ec enabled\n\n",chan_name);
	}
#endif

	printf("Socket bound to dev=%s\n\n",chan_name);

	return sock;
}

/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call handle_span_chan() to read the socket 
 *
 **************************************************************/


int main(int argc, char* argv[])
{
	int proceed;
	int span,chan;

	proceed=init_args(argc,argv);
	if (proceed != 1){
		usage(argv[0]);
		return -1;
	}

	if (tx_size < 160) {
     	tx_size = 160;
	}

	signal(SIGINT,&sig_end);
	
	dev_fd =-1;
	  
	span=atoi(card_name);
	chan=atoi(if_name);

	chan=(span-1)*31+chan;
	
	dev_fd = MakeConnection(chan);
	if( dev_fd < 0){
		printf("Failed to open span chan\n");
		exit (1);
	}
	printf("HANDLING SPAN %i CHAN %i FD=%i\n",
		atoi(card_name),atoi(if_name),dev_fd);
		
	handle_span_chan();
	zap_ec_ctrl(dev_fd,0);    
	close(dev_fd);
	return 0;

	return 0;
};


void sig_end(int sigid)
{

	printf("Got Signal %i\n",sigid);

	if (tx_fd){
		fclose(tx_fd);
	}
	if (rx_fd){
		fclose(rx_fd);
	}

	if (dev_fd){
		close (dev_fd);
	}

	exit(1);
}



