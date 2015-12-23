/*****************************************************************************
* fr_api.c	Frame Relay API: Transmit Module
*
* Author(s):	Gideon Hack & Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2001 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
* Description:
*
* 	The fr_api.c utility will bind to a socket to a frame relay network
* 	interface, and continously tx and rx packets to an from the sockets.
*
*	This example has been written for a single interface in mind, 
*	where the same process handles tx and rx data.
*
*	A real world example, should use different processes to handle
*	tx and rx spearately.  
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <time.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <linux/wanpipe.h>
#include <linux/if_wanpipe.h>
#include <linux/sdla_fr.h>
#include <linux/if_ether.h>

#define FALSE	0
#define TRUE	1

#define MIN_FRM_LGTH	100	
#define MAX_FRM_LGTH  	1000
#define NO_FRMS_TO_TX   10000	

#define WRITE	1
 

/*===================================================
 * Golobal data
 *==================================================*/

unsigned char Tx_data[MAX_FRM_LGTH + sizeof(api_tx_hdr_t)];
unsigned char Rx_data[MAX_FRM_LGTH + sizeof(api_tx_hdr_t)];

int sock;
fd_set writeset,readset,oobset;
struct wan_sockaddr_ll sa;



/*=================================================== 
 * Function Prototypes 
 *==================================================*/
int MakeConnection(char *,  char *);
void process_con( void);
void setup_signal_handlers (void);
void quit (int);
void sig_urg (int);


/*====================================================
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call send_socket() to transmit data
 *
 *===================================================*/   

int main(int argc, char* argv[])
{
	int proceed;
	
	if (argc != 3){
                printf("Usage: fr_send <router name> <interface name>\n");
                exit(0);
        }

        proceed = MakeConnection(argv[argc - 2], argv[argc - 1]);

	if( proceed == TRUE ){
		process_con( );
		return 0;
	}
	return 1;
};



/***************************************************
* MakeConnection
*
*   o Create a Socket
*   o Bind a socket to a wanpipe network interface
*       (Interface name is supplied by the user)
*/         

int MakeConnection (char *r_name, char *i_name) 
{

	errno = 0;
   	sock = socket( AF_WANPIPE, SOCK_RAW, 0);
   	if( sock < 0 ) {
      		perror("Socket");
      		return( FALSE );
   	} /* if */
 
	sa.sll_family = AF_WANPIPE;
	sa.sll_protocol = htons(ETH_P_IP);
	strcpy(sa.sll_device, i_name);
        strcpy( sa.sll_card, r_name);
	sa.sll_ifindex = 0;

        if(bind(sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) < 0){
                perror("bind");
		printf("Failed to bind socket to %s interface\n",i_name);
                exit(0);
        }
	printf("Socket bound to %s\n",i_name);
	fflush(stdout);
	return( TRUE );
}

/***************************************************
* process_con 
*
*   o Tx/Rx data via sockt to the frame relay network.
*   o Create a tx packet using api_tx_element_t data type. 
*   o The transmit packet contains 16 bytes header 
*
*	------------------------------------------
*      |  16 bytes      |        X bytes        ...
*	------------------------------------------
* 	   Header              Data
*
* RX_DATA:
* --------
*	Each rx data packet contains the 16 byte header!
*  
* 		typedef struct {
*        		unsigned char   attr      	PACKED;
*      		 	unsigned short  time_stamp      PACKED;
*       		unsigned char   reserved[13]    PACKED;
*		} api_rx_hdr_t;
*
*		typedef struct {
*        		api_rx_hdr_t    api_rx_hdr      PACKED;
*        		void *          data            PACKED;
*		} api_rx_element_t;
*
*	Currently the frame relay device driver doesn't use any of
*	the above fields.  Thus, the user can ignore the 16 bytes
*	rx header.
*
* TX_DATA:
* --------
* 
* 	Each tx data packet MUST contain the 16 byte header!
*
*	 	typedef struct {
*      		  	unsigned char   attr            PACKED;
*      	 		unsigned char   reserved[15]    PACKED;
*		} api_tx_hdr_t;
*
*		typedef struct {
*       	 	api_tx_hdr_t    api_tx_hdr      PACKED;
*        		void *          data            PACKED;
*		} api_tx_element_t;
*
* 	Currently the frame relay device driver doesn't use any of
*	the above fields.  Thus, the user can set the 16 bytes
*	tx header to ZERO.
*/

void process_con(void)
{
	unsigned int Tx_count=0,Rx_count=0;
	unsigned short Tx_length=0,Rx_length=0;
	api_tx_element_t* api_tx_el;
	api_rx_element_t* api_rx_el;
	int err=0, i=0;

	Tx_length = MIN_FRM_LGTH;
	Rx_length = MAX_FRM_LGTH;

	/* Initialize the tx packet. The 16 byte header must
	 * be inserted before tx data.  In Frame Relay protocol,
	 * the tx x25 header is not used, thus it can be
	 * set to zero */
	
	api_tx_el = (api_tx_element_t *)Tx_data;
  
	/* Fill in the packet data */ 
	for (i=0; i<Tx_length ; i++){
               	*((unsigned char *)&api_tx_el->data + i) = (unsigned char) i;
	}

	
	for(;;) {
		FD_ZERO(&writeset);
		FD_ZERO(&readset);
		FD_ZERO(&oobset);

#ifdef WRITE
		FD_SET(sock,&writeset);
#endif
		FD_SET(sock,&readset);
		FD_SET(sock,&oobset);

	
		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if the socket cannot send
		 * or there is nothing to receive.
		 *
		 * By using the last socket file descriptor +1 select will wait
		 * for all active sockets.
		 */ 

		if(select(sock + 1, &readset, &writeset, &oobset, NULL)){


			/* Check if the socket is ready to tx data */
			
			if (FD_ISSET(sock,&writeset)){

				err = send(sock, Tx_data, Tx_length + sizeof(api_tx_hdr_t), 0);

				/* err contains number of bytes transmited */		
				if(err > 0) {
					/* Packet sent ok */
	                                printf("Packet Sent %i, length = %i\n",
						++Tx_count, Tx_length);
				}else{
					/* The driver is busy,  we must requeue this
					 * tx packet, and try resending it again 
					 * later. */
				}
				
			}
			
			if (FD_ISSET(sock,&readset)){

				err = recv(sock,Rx_data, Rx_length + sizeof(api_rx_hdr_t),0);
				
				if (err < 0){
					/* This should never happen. If it does the
					 * socket is corrupted. Thus exit.*/
					printf("Failed to rec %i , %i\n", Rx_count, err);
					break;
				}else{
					/* Rx packet recevied OK
					 * Each rx packet will contain 16 bytes of
					 * rx header, that must be removed.  The
					 * first byte of the 16 byte header will
					 * indicate an error condition.
					 */

					api_rx_el = (api_rx_element_t*)Rx_data;
				
					/* The data portion of the api_rx_el
					 * contains the real data.
					 
					 api_rx_el->data
					 
					 */
						
					printf("Packet receive %i, length = %i\n",
						++Rx_count, err);
				}	
			}
	
			/* Stop after number of frames */
			if(Tx_count == NO_FRMS_TO_TX)
				break;

		}
	}

	close(sock);
} 
