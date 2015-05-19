/*****************************************************************************
* hdlc_client.c	X25API: HDLC ONLY : PVC Client Application
*
* Author(s):	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2000 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
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
#include <linux/if_wanpipe.h>
#include <string.h>
#include <errno.h>
#include <linux/wanpipe.h>
#include <linux/sdla_x25.h>
#include <linux/if_ether.h>

#define FALSE	0
#define TRUE	1

#define MIN_FRM_LGTH	100	
#define MAX_FRM_LGTH  	MIN_FRM_LGTH+100

#define NO_FRMS_TO_TX   100000	

#define MAX_X25_ADDR_SIZE	16
#define MAX_X25_DATA_SIZE 	129
#define MAX_X25_FACL_SIZE	110

#define TIMEOUT 10

unsigned char Tx_data[MAX_FRM_LGTH];
unsigned char Rx_data[MAX_FRM_LGTH];

/* Prototypes */
int MakeConnection( char *, char * );
void process_con( void);
void setup_signal_handlers (void);
void quit (int);
void handle_oob_event(x25api_t* api_hdr);


/* Defined global so it would only get allocated once.  
 * If it was defined in MakeConnection() function it 
 * would be allocated and dealocated every time we 
 * run that function.
 */  

int sock;

int send_ok=0;

/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call send_socket() to transmit data
 *
 **************************************************************/   

int main(int argc, char* argv[])
{
	int proceed;
	
	if (argc < 3){
		printf("Usage: ./pvc_client <card name> <interface name>\n");
		exit(0);
	}

	if (argc == 4){
		send_ok=1;
	}
	
	proceed = MakeConnection(argv[2],argv[1]);
	if (proceed == TRUE){
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

int MakeConnection (char *int_name, char *card_name) 
{
	struct wan_sockaddr_ll 	sa;
	int err=0;

	memset(&sa,0,sizeof(struct wan_sockaddr_ll));

	/* Create a new socket */
   	sock = socket( AF_WANPIPE, SOCK_RAW, 0);
   	if( sock < 0 ) {
      		perror("Socket");
      		return FALSE;
   	}
 
	/* Fill in binding information 
	 * before we use connect() system call
         * a socket must be binded into a 
         * network interface defined in wanpipe#.conf.
         * 
         * Card name must be supplied as well. In 
         * this case the user supplies the name
         * eg: wanpipe1.
         */ 
	sa.sll_family = AF_WANPIPE;
	sa.sll_protocol = htons(X25_PROT);
	strcpy(sa.sll_device, int_name);
	strcpy(sa.sll_card, card_name);
	

	/* Bind a sock to */
        if(bind(sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) < 0){
                perror("bind");
		printf("Failed to bind socket to %s interface\n",int_name);
		close(sock);
                return FALSE;
        }
	
	/* We must use connect to bring the LAPB HDLC link
         * up. The connect will block until the link comes up */
	err=connect(sock,(struct sockaddr *)&sa,sizeof(struct wan_sockaddr_ll));
	if (err == 0){
		printf("Connect OK\n");
	}else{
		perror("Connect: ");
		printf("Connect BAD, %i\n",err);
		close(sock);
		return FALSE;
	}

	fflush(stdout);
	return TRUE;
}




/***************************************************
*  process_con
*
*   o Send and Receive data via socket 
*   o Create a tx packet using x25api_t data type. 
*   o Both the tx and rx packets contains 16 bytes headers
*     in front of the real data.  It is the responsibility
*     of the applicatino to insert this 16 bytes on tx, and
*     remove the 16 bytes on rx.
*
*	------------------------------------------
*      |  16 bytes      |        X bytes        ...
*	------------------------------------------
* 	   Header              Data
*
*   o 16 byte Header: data structure:
*
*     	typedef struct {
*		unsigned char  qdm	PACKED;	Q/D/M bits 
*		unsigned char  cause	PACKED;	cause field 
*		unsigned char  diagn	PACKED; diagnostics 
*		unsigned char  pktType  PACKED; 
*		unsigned short length   PACKED;
*		unsigned char  result	PACKED;
*		unsigned short lcn	PACKED;
*		char reserved[7]	PACKED;
*	}x25api_hdr_t;
*
*	typedef struct {
*		x25api_hdr_t hdr	PACKED;
*		char data[X25_MAX_DATA]	PACKED;
*	}x25api_t;
*
* TX DATA:
* --------
* 	Each tx data packet must contain the above 16 byte header!
* 	
* 	QDM: Represents PF variable:
*
*       Thus, if you set the QDM to 1, the driver will set the
*       PF bit which will request acknowledgement of each 
*       packet sent.
*
* RX DATA:
* --------
* 	Each rx data will contain the above 16 byte header!
*
* 	Relevant bytes in the 16 byte rx header, are the
* 	LCN and QDM bytes.
*
*       QDM: It represents PF variable:
*
* 	
* OOB DATA:
* ---------
* 	The OOB (out of band) data is used by the driver to 
* 	indicate x25 events for the active channel: 
* 	clear call, or restarts.
*
* 	Each OOB packet contains the above 16 byte header!
*
*	Relevant bytes in the 16 byte oob header, are the
*	pktType, cause and diagn bytes.
*
*	pktType = event type (ex Clear Call, Restart ...)
*	cause   = x25 cause of the event
*	diagn   = x25 diagnostic information used to determine
*	          the cause of the event.
*
*	Upon receiving an event, the sock should be considered 
*	DEAD!!!  Meaning it must be closed using the close()
*	function.
*/

void process_con(void)
{
	unsigned int Tx_count=0;
	unsigned short Tx_lgth,timeout=0;
	int err=0, i, Rx_count=0;
	struct timeval tv;
	x25api_t* api_tx_el;
	fd_set	writeset,readset,oobset;

	Tx_lgth = MIN_FRM_LGTH;

	api_tx_el = (x25api_t *)Tx_data;

	/* Reset the PF Bit */
	api_tx_el->hdr.qdm = 0;	
 
	/* Fill in the packet data */ 
	for (i=0; i<Tx_lgth ; i++){
               	api_tx_el->data[i] = (unsigned char) i;
	}

	//printf("Starting Select\n");

	for(;;) {
		fflush(stdout);
		FD_ZERO(&readset);
		FD_ZERO(&oobset);
		FD_ZERO(&writeset);

		FD_SET(sock,&oobset);
#if 1
		FD_SET(sock,&readset);
		if (send_ok){
			FD_SET(sock,&writeset);
		}
#endif
		/* This must be within the loop */	
		tv.tv_usec = 0; 
		tv.tv_sec = TIMEOUT;

		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if already queued packet
                 * cannot be sent 
		 */ 
	        if((err=select(sock+1, &readset, &writeset, &oobset, NULL))){

			//printf("Select\n");

			if (FD_ISSET(sock,&oobset)){

				printf("OOB\n");
				/* The OOB Message will indicate that an 
				 * asynchronous event occured.  The applicaton 
				 * must stop everything and check the state of 
				 * the link.  Since link might  
				 * have gone down */

				/* In this example I just exit. A real 
				 * application would check the state of the 
				 * sock first by reading the header information. 
				 */

				/* IMPORTANT:
				 * If we fail to read the OOB message, we can 
				 * assume that the link is down.  Thus, close 
				 * the socket and continue !!! */

				err = recv(sock, Rx_data, sizeof(Rx_data), MSG_OOB);
		
				/* err contains number of bytes transmited */		
				if(err < 0 ) {

					/* The state of the socket is disconnected.
					 * We must close the socket and continue with
					 * operation */
					printf("Failed to receive OOB %i , %i\n", Rx_count, err);
					break;
				}else{
					api_tx_el = (x25api_t *)Rx_data;
					handle_oob_event(api_tx_el);
				}
			}

			
			if (FD_ISSET(sock,&readset)){
				
				err = recv(sock, Rx_data, sizeof(Rx_data), 0);

				/* err contains number of bytes transmited */		
				if(err < 0 ) {
					printf("Failed to rcv %i , %i\n", Rx_count, err);
					exit(1);
				}else{

					/* Every received packet comes with the 16
					 * bytes of header which driver adds on.
                                         * Header is structured as x25api_hdr_t. 
					 * (same as above)
					 */

					x25api_hdr_t *api_data = 
							(x25api_hdr_t *)Rx_data;

					if (api_data->qdm & 0x01){
						/* More bit is set, thus
						 * handle it accordingly */
					}
#ifdef VEROBSE	
					printf("\tReceive OK, size: %i, qdm %x,"
						  "cause %x, diagn %x, cnt: %i\n", 
						   err, api_data->qdm, 
						   api_data->cause, 
						   api_data->diagn, ++Rx_count);
#else
					putchar('R');
#endif
				}
		
			}
			
			if (FD_ISSET(sock,&writeset)){

				err = send(sock, Tx_data, Tx_lgth + sizeof(x25api_hdr_t), 0);

				/* err contains number of bytes transmited */		
				if(err > 0 ) {
#ifdef VERBOSE
					printf("Packet Sent %i\n",
							++Tx_count);
#else
					putchar('T');
#endif
				}
			}
			
			/* Stop after number of frames */
			if(Tx_count == NO_FRMS_TO_TX)
				break;

			timeout=0;
		}else{
			printf("Select error\n");
			if (err == 0){
				if (++timeout == 5){
					printf("Sock timeout exceeded MAXIMUM\n");
					sleep(5);
					break;			
				}		
				printf("Sock timeout try again !!!\n");
			}else{
				printf("Error in Select !!!\n");
				break;
			}
		}//if select
	}//for
	close(sock);
} 

void handle_oob_event(x25api_t* api_hdr)
{

	switch (api_hdr->hdr.pktType){

		case ASE_RESET_RQST:
			printf("SockId=%i : OOB : Rx Reset Call : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);

			/* NOTE: we don't have to close the socket,
			 * since the reset doesn't clear the call
			 * however, it means that there is something really
			 * wrong and that data has been lost */
			return;
		
		
		case ASE_CLEAR_RQST:
			printf("SockId=%i : OOB : Rx Clear Call : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			break;		
			
		case ASE_RESTART_RQST:
			printf("SockId=%i : OOB : Rx Restart Req : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			
			break;
		case ASE_INTERRUPT:
			printf("SockId=%i : OOB : Rx Interrupt Req : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			
			break;


		default:
			printf("SockId=%i : OOB : Rx Type=0x%02X : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					sock,
					api_hdr->hdr.pktType,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			break;
	}
}



