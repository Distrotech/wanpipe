/*****************************************************************************
* chdlc_api.c	CHDLC API: Receive Module
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
* 	The chdlc_api.c utility will bind to a socket to a chdlc network
* 	interface, and continously tx and rx packets to an from the sockets.
*
*	This example has been written for a single interface in mind, 
*	where the same process handles tx and rx data.
*
*	A real world example, should use different processes to handle
*	tx and rx spearately.  
*/



#include <stdlib.h>
#include <stdio.h>
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
#include <linux/wanpipe.h>
#include <linux/sdla_chdlc.h>

#define FALSE	0
#define TRUE	1

#define LGTH_CRC_BYTES	2
#define MAX_TX_DATA     100	/* Size of tx data */  
#define MAX_FRAMES 	1     /* Number of frames to transmit */  
#define TIMEOUT		1

#define WRITE 1

/*===================================================
 * Golobal data
 *==================================================*/

unsigned short no_CRC_bytes_Rx;
unsigned char HDLC_streaming = FALSE;
unsigned short Rx_lgth;

unsigned char Rx_data[16000];
unsigned char Tx_data[MAX_TX_DATA + sizeof(api_tx_hdr_t)];
int 	sock;


/*=================================================== 
 * Function Prototypes 
 *==================================================*/
int MakeConnection(char *, char *);
void process_con( void);


/*===================================================
 * MakeConnection
 *
 *   o Create a Socket
 *   o Bind a socket to a wanpipe network interface
 *       (Interface name is supplied by the user)
 *==================================================*/         

int MakeConnection(char *r_name, char *i_name ) 
{
	struct wan_sockaddr_ll 	sa;

	memset(&sa,0,sizeof(struct wan_sockaddr_ll));
	errno = 0;
   	sock = socket(AF_WANPIPE, SOCK_RAW, 0);
   	if( sock < 0 ) {
      		perror("Socket");
      		return( FALSE );
   	} /* if */
  
	printf("\nConnecting to router %s, interface %s\n", r_name, i_name);
	
	strcpy( sa.sll_device, i_name);
	strcpy( sa.sll_card, r_name);
	sa.sll_protocol = htons(PVC_PROT);
	sa.sll_family=AF_WANPIPE;

        if(bind(sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) < 0){
                perror("bind");
		printf("Failed to bind a socket to %s interface\n",i_name);
                exit(0);
        }
	printf("Socket bound to %s\n\n",i_name);

	return( TRUE );

}

/*===========================================================
 * process_con 
 *
 *   o Tx/Rx data to and from the socket 
 *   o Cast received data to an api_rx_element_t data type 
 *   o The received packet contains 16 bytes header 
 *
 *	------------------------------------------
 *      |  16 bytes      |        X bytes        ...
 *	------------------------------------------
 * 	   Header              Data
 *
 * RX DATA:
 * --------
 * 	Each rx data packet contains the 16 byte header!
 * 	
 *   	o Rx 16 byte data structure:
 *
 *		typedef struct {
 *       	 	unsigned char   error_flag      PACKED;
 *        		unsigned short  time_stamp      PACKED;
 *        		unsigned char   reserved[13]    PACKED;
 *		} api_rx_hdr_t; 
 *
 *		typedef struct {
 *        		api_rx_hdr_t    api_rx_hdr      PACKED;
 *        		void *          data            PACKED;
 *		} api_rx_element_t;
 *
 * 	error_flag:
 * 		bit 0: 	incoming frame was aborted
 * 		bit 1: 	incoming frame has a CRC error
 * 		bit 2:  incoming frame has an overrun eror 
 *
 * 	time_stamp:
 * 		absolute time value in ms.
 *
 * TX_DATA:
 * --------
 *	Each tx data packet MUST contain a 16 byte header!
 *
 * 	o Tx 16 byte data structure
 * 	
 *		typedef struct {
 *			unsigned char 	attr		PACKED;
 *			unsigned char   misc_Tx_bits	PACKED;
 *			unsigned char  	reserved[14]	PACKED;
 *		} api_tx_hdr_t;
 *
 *		typedef struct {
 *			api_tx_hdr_t 	api_tx_hdr	PACKED;
 *			void *		data		PACKED;
 *		} api_tx_element_t;
 *
 * 	When device driver is configured for Switched CTS/RTS
 * 	or Idle Mark, the misc_Tx_bits must be used to control 
 * 	the state of the line after the transmisstion of data is 
 * 	complete:
 * 	
 * 	misc_Tx_bits:
 * 	------------
 * 		0			: No change
 * 		
 * 		DROP_RTS_AFTER_TX	: Drop RTS after transmission of THIS
 * 		 			  frame. 
 *		
 *		IDLE_FLAGS_AFTER_TX	: Idle flags after transmission of 
 *					  THIS frame. 
 */

void process_con(void) 
{
	unsigned int Rx_count,Tx_count,Tx_length;
	api_rx_element_t* api_rx_el;
	api_tx_element_t * api_tx_el;
	fd_set 	ready,write;
	int err,i;
	unsigned char *data;

        Rx_count = 0;
	Tx_count = 0;
	Tx_length = MAX_TX_DATA;
	
	/* If running HDLC_STREAMING then the received CRC bytes
         * will be passed to the application as part of the 
         * received data.  The CRC bytes will be appended as the 
	 * last two bytes in the rx packet.
	 */

	memset(&Tx_data[0],0,MAX_TX_DATA + sizeof(api_tx_hdr_t));


	/* Initialize the tx packet. The 16 byte header must
	 * be inserted before tx data.  
	 */
	api_tx_el = (api_tx_element_t*)&Tx_data[0];

	/* In general misc_Tx_bits are set to zero.
	 * However, if device driver is configured for 
	 * Switched CTS/RTS or Idle Mark, the misc_Tx_bits 
	 * must be used as described above */
	
	api_tx_el->wan_api_tx_hdr.wp_api_tx_hdr_chdlc_misc_tx_bits = 0; 

	/* Fill in the packet data */ 	
	
	data = (unsigned char *)&api_tx_el->data;
	for (i=0;i<Tx_length;i++){
		data[i] = (unsigned char)i;
	}
	

	/* If drivers has been configured in HDLC_STREAMING
	 * mode, the CRC bytes will be included into the 
	 * rx packet.  Thus, the application should remove
	 * the last two bytes of the frame. 
	 * 
	 * The no_CRC_bytes_Rx will indicate to the application
	 * how many bytes to cut from the end of the rx frame.
	 *
	 */
	no_CRC_bytes_Rx = 0; //LGTH_CRC_BYTES;

	for(;;) {	
		FD_ZERO(&ready);
		FD_ZERO(&write);
		FD_SET(sock,&ready);
#ifdef WRITE
		FD_SET(sock,&write);
#endif

		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if the socket cannot send
		 * or there is nothing to receive.
		 *
		 * By using the last socket file descriptor +1 select will wait
		 * for all active sockets.
		 */ 

  		if(select(sock + 1,&ready, &write, NULL, NULL)){


			/* Check for rx packets */
                   	if (FD_ISSET(sock,&ready)){

				err = recv(sock, Rx_data, sizeof(Rx_data), 0);

				/* err indicates bytes received */
				if(err > 0) {
	
					/* Rx packet recevied OK
					 * Each rx packet will contain 16 bytes of
					 * rx header, that must be removed.  The
					 * first byte of the 16 byte header will
					 * indicate an error condition.
					 */

					api_rx_el = (api_rx_element_t*)Rx_data;

					switch (api_rx_el->api_rx_hdr.wan_hdr_hdlc_error_flag){
				
					case 0:
						/* Rx packet is good */
						break;
						
					case RX_FRM_ABORT:
						/* Frame was aborted */
					case RX_FRM_CRC_ERROR:
						/* Frame has crc error */
					case RX_FRM_OVERRUN_ERROR:
						/* Frame has an overrun error */
					default:
						/* Error with the rx packet 
						 * handle it ... */
						break;
					}
					
						
					/* Check the packet length, after we remove
					 * the 16 bytes rx header */
					Rx_lgth = err - sizeof(api_rx_hdr_t);
					if(Rx_lgth <= no_CRC_bytes_Rx) {
						printf("\nShort frame received (%d)\n",
							Rx_lgth);
						return;
					}

					/* Remove any CRC bytes from the rx packet
					 * by adjusting the rx packet length */
					Rx_lgth -= no_CRC_bytes_Rx;

					printf("Received packet %i, length = %i\n", 
							++Rx_count, Rx_lgth);

				} else {
					printf("\nError receiving data\n");
					break;
				}

			 } 
		  
			/* Check if the socket is ready to tx data */
		   	if (FD_ISSET(sock,&write)){

				err = send(sock,Tx_data, Tx_length + sizeof(api_tx_hdr_t), 0);

				if (err > 0){
					/* Packet sent ok */
					printf("Packet sent: Sent %i : %i\n",
							err,++Tx_count);
				}else{
					/* The driver is busy,  we must requeue this
					 * tx packet, and try resending it again 
					 * later. */
				}
		   	}


		   	/* Stop the program afer transmitting MAX_FRAMES */
			if (Tx_count == MAX_FRAMES)
				break;

		} else {
                	printf("\nError selecting socket\n");
			break;
		}
	}
	close (sock);
} 



/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call process_con() to read/write the socket 
 *
 **************************************************************/


int main(int argc, char* argv[])
{
	int proceed;

	if (argc != 3){
		printf("Usage: rec_wan_sock <router name> <interface name>\n");
		exit(0);
	}	

	proceed = MakeConnection(argv[argc - 2], argv[argc - 1]);
	if( proceed == TRUE ){
		process_con( );
		return 0;
	}

	return 1;
};
