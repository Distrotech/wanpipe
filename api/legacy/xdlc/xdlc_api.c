/*****************************************************************************
* xdlc_api.c	XDLC API Sample Code
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
#include <linux/if.h>
#include <linux/wanpipe.h>
#include <linux/wanpipe_xdlc_iface.h>
#include "lib_api.h"

#define MAX_TX_DATA     5000	/* Size of tx data */  
#define MAX_FRAMES 	5000     /* Number of frames to transmit */  

#define MAX_RX_DATA	5000

unsigned short Rx_lgth;

unsigned char Rx_data[MAX_RX_DATA];
unsigned char Tx_data[MAX_TX_DATA + sizeof(wan_api_tx_hdr_t)];

/* Prototypes */
int MakeConnection(void);
void handle_socket( void);
void sig_end(int sigid);
void sig_tx(int sigid);

int sock;
FILE *tx_fd=NULL,*rx_fd=NULL;	

/***************************************************
* MakeConnection
*
*   o Create a Socket
*   o Bind a socket to a wanpipe network interface
*       (Interface name is supplied by the user)
*/         

int MakeConnection(void) 
{
	struct wan_sockaddr_ll 	sa;
	int err;
	wan_xdlc_conf_t xdlc_conf;

	memset(&sa,0,sizeof(struct wan_sockaddr_ll));
	errno = 0;
   	sock = socket(AF_WANPIPE, SOCK_RAW, 0);
   	if( sock < 0 ) {
      		perror("Socket");
      		return -1;
   	} /* if */
  
	printf("\nConnecting to card %s, interface %s prot %x\n", 
			card_name, if_name,htons(PVC_PROT));
	
	strcpy( sa.sll_device, if_name);
	strcpy( sa.sll_card, card_name);
	sa.sll_protocol = htons(PVC_PROT);
	sa.sll_family=AF_WANPIPE;
	
        if(bind(sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) < 0){
                perror("bind");
		printf("Failed to bind a socket to %s interface\n",if_name);
		close(sock);
		return -1;
        }

	err = ioctl(sock,WAN_DEVPRIV_SIOC(SIOCS_XDLC_GET_CONF), &xdlc_conf);
	if (err){
		perror("Get Address:");
		return -1;
	}

#if 1
	printf("Inital Address 0x%X!\n",
				xdlc_conf.address);
		
	xdlc_conf.address=33;
	
	err = ioctl(sock,WAN_DEVPRIV_SIOC(SIOCS_XDLC_SET_CONF), &xdlc_conf);
	if (err){
		perror("Get Address:");
		return -1;
	}

	err = ioctl(sock,WAN_DEVPRIV_SIOC(SIOCS_XDLC_GET_CONF), &xdlc_conf);
	if (err){
		perror("Get Address:");
		return -1;
	}

	printf("New Address 0x%X!\n",
				xdlc_conf.address);

#endif

#if 0
	if (xdlc_conf.station == WANOPT_DTE){
		printf("Disabling I Frames\n");
		ioctl(sock,WAN_DEVPRIV_SIOC(SIOCS_XDLC_DISABLE_IFRAMES),NULL);
	}
#endif	
	
	err = ioctl(sock,WAN_DEVPRIV_SIOC(SIOCS_XDLC_START),NULL);
	if (err){
		perror("XDLC Start: ");
	}else{
		printf("Address 0x%X started successfully!\n",
				xdlc_conf.address);
	}

	printf("Socket bound to %s\n\n",if_name);
	
	return 0;

}

/***************************************************
* HANDLE SOCKET 
*
*   o Read a socket 
*   o Cast data received to wan_api_rx_element_t data type 
*   o The received packet contains 64 bytes header 
*
*	------------------------------------------
*      |  64 bytes      |        X bytes        ...
*	------------------------------------------
* 	   Header              Data
*
*   o Data structures:
*
*   typedef struct {
*	unsigned char	error_flag;
*	unsigned short	time_stamp;
*	unsigned char	reserved[13];
*   } wan_api_rx_hdr_t;
*
*   typedef struct {
*       wan_api_rx_hdr_t	api_rx_hdr;
*       unsigned char  	data[1];
*   } api_rx_element_t;
*
*/

void handle_socket(void) 
{
	unsigned int Rx_count,Tx_count,Tx_length;
	wan_api_rx_element_t* api_rx_el;
	wan_api_tx_element_t * api_tx_el;
	fd_set 	ready,write,oob;
	int err,i;
	int rlen;

        Rx_count = 0;
	Tx_count = 0;
	Tx_length = tx_size;

	printf("\n\nSocket Handler: Rx=%d Tx=%i TxCnt=%i TxLen=%i TxDelay=%i\n",
			read_enable,write_enable,tx_cnt,tx_size,tx_delay);	

	/* Initialize the Tx Data buffer */
	memset(&Tx_data[0],0,MAX_TX_DATA + sizeof(wan_api_tx_hdr_t));

	/* If rx file is specified, open
	 * it and prepare it for writing */
	if (files_used & RX_FILE_USED){
		rx_fd=fopen(rx_file,"wb");
		if (!rx_fd){
			printf("Failed to open file %s\n",rx_file);
			perror("File: ");
			return;
		}
		printf("Opening %s rx file\n",rx_file);
	}

	/* Cast the Tx data packet with the tx element
	 * structure.  We must insert a 64 byte
	 * driver header, which driver will remove 
	 * before passing packet out the physical port */
	api_tx_el = (wan_api_tx_element_t*)&Tx_data[0];
	
	if (files_used & TX_FILE_USED){

		/* TX file is used to transmit data */
		
		tx_fd=fopen(tx_file,"rb");
		if (!tx_fd){
			printf("Failed to open file %s\n",tx_file);
			perror("File: ");
			return;
		}
		
		printf("Opening %s tx file\n",rx_file);
		
		rlen=fread((void*)&Tx_data[sizeof(wan_api_tx_hdr_t)],
				   sizeof(char),
				   Tx_length,tx_fd);
		if (!rlen){
			printf("%s: File empty!\n",
				tx_file);
			return;
		}
	}else{

		/* Create a Tx packet based on user info, or
		 * by deafult incrementing number starting from 0 */
		for (i=0;i<Tx_length;i++){
			if (tx_data == -1){
				api_tx_el->data[i] = (unsigned char)i;
			}else{
				api_tx_el->data[i] = (unsigned char)tx_data;
			}
		}
	}


	/* Main Rx Tx OOB routine */
	for(;;) {	

		/* Initialize all select() descriptors */
		FD_ZERO(&ready);
		FD_ZERO(&write);
		FD_ZERO(&oob);
		FD_SET(sock,&oob);
		FD_SET(sock,&ready);

		if (write_enable){
		     FD_SET(sock,&write);
		}

		/* Select will block, until:
		 * 	1: OOB event, link level change
		 * 	2: Rx data available
		 * 	3: Interface able to Tx */
	
		err=select(sock + 1,&ready, &write, &oob, NULL);
		
		if (err < 0){
			/* Signal occured */
			continue;

		}else if (err == 0){
			/* Timeout occured */
			continue;
		
		}else if (err > 0){

			fflush(stdout);	
		   	if (FD_ISSET(sock,&oob)){
		
				/* An OOB event is pending, usually indicating
				 * a link level change */
				
				err = recv(sock, Rx_data, MAX_RX_DATA, MSG_OOB);

				if(err < 0 ) {
					printf("Failed to receive OOB %i , %i\n", Rx_count, err);
					err = ioctl(sock,SIOC_WANPIPE_SOCK_STATE,0);
					printf("Sock state is %s\n",
							(err == 0) ? "CONNECTED" : 
							(err == 1) ? "DISCONNECTED" :
							 "CONNECTING");
					break;
				}else{
					api_rx_el=(wan_api_rx_element_t*)&Rx_data[0];
					printf("Rx: OOB Excep: Addr=0x%X State=%s Exception=%s\n",
							api_rx_el->wan_rxapi_xdlc_address,
							XDLC_STATE_DECODE(api_rx_el->wan_rxapi_xdlc_state),
							XDLC_EXCEP_DECODE(api_rx_el->wan_rxapi_xdlc_exception));
#if 0
					if (api_rx_el->wan_rxapi_xdlc_exception == EXCEP_SEC_SNRM_FRAME_RECEIVED){
						printf("Enabling I Frames\n");
						ioctl(sock,WAN_DEVPRIV_SIOC(SIOCS_XDLC_ENABLE_IFRAMES),NULL);
					}
#endif
					
					Tx_count=0;
					Rx_count=0;
					continue;
				}
			}
		  
			
                   	if (FD_ISSET(sock,&ready)){

				/* An Rx packet is pending
				 * 	1: Read the rx packet into the Rx_data
				 * 	   buffer. Confirm len > 0
				 *
				 * 	2: Cast Rx_data to the api_rx_element.
				 * 	   Thus, removing a 64 byte header
				 * 	   attached by the driver.
				 *
				 * 	3. Check error_flag:
				 * 		CRC,Abort..etc 
				 */
				err = recv(sock, Rx_data, MAX_RX_DATA, 0);

				if (!read_enable){
					goto bitstrm_skip_read;
				}
				
				/* err indicates bytes received */
				if(err > 0) {

					api_rx_el = (wan_api_rx_element_t*)&Rx_data[0];

		                	/* Check the packet length */
                                	Rx_lgth = err - sizeof(wan_api_rx_hdr_t);
                                	if(Rx_lgth<=0) {
                                        	printf("\nShort frame received (%d)\n",
                                                	Rx_lgth);
                                        	return;
                                	}

					if ((files_used & RX_FILE_USED) && rx_fd){
						fwrite((void*)&Rx_data[sizeof(wan_api_rx_hdr_t)],
						       sizeof(char),
						       Rx_lgth,
						       rx_fd);	
					}
				
					++Rx_count;

					if (verbose){
						printf("Received %i Olen=%i Length = %i\n", 
								Rx_count, err,Rx_lgth);
					}else{
						//putchar('R');
					}

					if (rx_cnt && rx_cnt < Rx_count){
						printf("User Re-Stopping\n");
						ioctl(sock,WAN_DEVPRIV_SIOC(SIOCS_XDLC_STOP),NULL);
						Rx_count=0;
					}
	
				} else {
					printf("\nError receiving data\n");
					break;
				}

		   	}
bitstrm_skip_read:
		
		   	if (FD_ISSET(sock,&write)){

				err = send(sock,Tx_data, Tx_length + sizeof(wan_api_tx_hdr_t), 0);

				if (err <= 0){
					if (errno == EBUSY){
						if (verbose){
							printf("Sock busy try again!\n");
						}
						/* Socket busy try sending again !*/
					}else{
				 		/* Check socket state */
						err = ioctl(sock,SIOC_WANPIPE_SOCK_STATE,0);
						printf("Sock state is %s\n",
							(err == 0) ? "CONNECTED" : 
							(err == 1) ? "DISCONNECTED" :
							 "CONNECTING");

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
					
					if ((files_used & TX_FILE_USED) && tx_fd){
						rlen=fread((void*)&Tx_data[sizeof(wan_api_tx_hdr_t)],
							   sizeof(char),
							   Tx_length,tx_fd);
						if (!rlen){
							printf("\nTx of file %s is done!\n",
								tx_file);	
							break;	
						}
						if (Tx_length != rlen){
							Tx_length = rlen;
						}
					}
				}
		   	}

			if (tx_delay){
				usleep(tx_delay);
			}

			if (tx_cnt && tx_size && Tx_count >= tx_cnt && !(files_used & TX_FILE_USED)){
				printf("Tx completed %i frames\n",Tx_count);
				write_enable=0;
			}
		}
	}

	if (tx_fd){
		fclose(tx_fd);
	}
	if (rx_fd){
		fclose(rx_fd);
	}
	close (sock);
} 



/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call handle_socket() to read the socket 
 *
 **************************************************************/


int main(int argc, char* argv[])
{
	int proceed;

	proceed=init_args(argc,argv);
	if (proceed != WAN_TRUE){
		usage(argv[0]);
		return -1;
	}

	signal(SIGINT,&sig_end);
	signal(SIGTERM,&sig_end);
	signal(SIGUSR1,&sig_tx);
	
	proceed = MakeConnection();
	if(proceed == 0){
		handle_socket();
		return 0;
	}

	return 0;
};


void sig_tx(int sigid)
{
	printf("Sig write enable\n");
	write_enable=1;
	tx_cnt=1;
}

void sig_end(int sigid)
{

	printf("Got Signal %i\n",sigid);

	if (tx_fd){
		fclose(tx_fd);
	}
	if (rx_fd){
		fclose(rx_fd);
	}

	ioctl(sock,WAN_DEVPRIV_SIOC(SIOCS_XDLC_STOP),NULL);

	if (sock){
		close (sock);
	}

	exit(1);
}

