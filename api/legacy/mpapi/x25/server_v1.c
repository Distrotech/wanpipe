/*****************************************************************************
* server_v1.c	X25 API: SVC Server Application 
*
* Author(s):	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*
* Description:
*
* 	The aserver utility will accept incoming x25 calls and start 
* 	receiving data on each channel.  
*
* 	The aserver utility is process based, where the listen parent
* 	will spawn child processes to handle each estabished connection.
*
* 	This utility should be used as an architectual model.  It is up to
* 	the user to handle all conditions of x25.  Please refer to the
* 	X25API programming manual for futher details.
*/

/*===========================================================
 * Header Files: Includes
 *==========================================================*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <netinet/in.h>
#include <linux/if_wanpipe.h>
#include <string.h>
#include <errno.h>
#include <linux/wanpipe.h>
#include <linux/if_ether.h>
#include <linux/wanpipe_x25_kernel.h>
#include <linux/wanpipe_lapb_kernel.h>
#include "lib_api.h"

/*===========================================================
 * Defines 
 *==========================================================*/

#define FALSE	0
#define TRUE	1

#define DATA_SZ	5000	

#define MAX_X25_ADDR_SIZE	16
#define MAX_X25_DATA_SIZE 	129
#define MAX_X25_FACL_SIZE	110
#define TIMEOUT 10

#define APP_NAME "server_v1"
/*===========================================================
 * Global Variables 
 *==========================================================*/

unsigned char Tx_data[DATA_SZ];
unsigned char Rx_data[DATA_SZ];
unsigned char prognamed[100];


/* Defined global so it would only get allocated once.  
 * If it was defined in MakeConnection() function it 
 * would be allocated and dealocated every time we 
 * run that function.
 *
 * typedef struct {
 *   	unsigned char  qdm;	
 * 	unsigned char  cause;	
 * 	unsigned char  diagn;	
 * 	unsigned char  pktType;
 * 	unsigned short length;
 * 	unsigned char  result;
 * 	unsigned short lcn;
 * 	unsigned short mtu;
 * 	unsigned short mru;
 * 	char reserved[3];
 *  }x25api_hdr_t;
 * 
 *  typedef struct {
 * 	x25api_hdr_t hdr;
 * 	char data[X25_MAX_DATA];
 * }x25api_t;
 * 
 */  

x25api_t api_cmd;


/*============================================================
  Function Prototypes 
 *===========================================================*/

int MakeConnection( char * );
void tx_rx_data(int sock, int mtu, int mru);
void sig_chld (int sigio);
int issue_clear_call(int sock, int opt, int cause, int diagn);
int handle_oob_event(int sock);
int decode_oob_event(x25api_t* api_hdr, int sock);

void sig_hdlr (int sigio)
{

	printf("GOT USER SIG\n");

	return;
}


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

	card_cnt=1;

	if (!init_args(argc,argv)){
		usage(argv[0]);
		exit(1);
	}
	
	signal(SIGUSR1,sig_hdlr);

	proceed = MakeConnection(if_name);
	return 0;
};


/***************************************************
* MakeConnection
*
*   o Create a Socket
*   o Bind a socket to a wanpipe network interface
*   o Start listening on a socket for incoming
*     calls.
*   o Accept an incoming call and spawn a child
*     to handle the connection.
*/         

int MakeConnection (char *i_name ) 
{
	int err = 0;
	int sock=0, sock1=0;
	struct wan_sockaddr_ll 	sa;
	fd_set readset,oobset;
	int mtu=128,mru=128;

	memset(&sa,0,sizeof(struct wan_sockaddr_ll));

	sprintf(prognamed,"%s",APP_NAME);

	/* Create a new socket */
   	sock = socket( AF_WANPIPE, SOCK_RAW, AF_ANNEXG_WANPIPE);
   	if( sock < 0 ) {
      		perror("Socket");
      		return 1;
   	} 

	/* Fill in binding information 
	 * before we use connect() system call
         * a socket must be binded into a virtual
         * network interface svc_connect.
         * 
         * Card name must be supplied as well. In 
         * this case the user supplies the name
         * eg: wanpipe1.
         */ 
	sa.sll_family = AF_ANNEXG_WANPIPE;
	sa.sll_protocol = htons(ETH_P_X25);
	strcpy(sa.sll_device, "svc_listen");
	strcpy(sa.sll_card, i_name);

	/* Bind a sock using the above address structure */
        if(bind(sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) == -1){
                perror("Bind");
		printf("%s: Failed to bind socket to %s interface\n",
				prognamed,i_name);
		close(sock);
                exit(0);
        }

	if (listen(sock,10) != 0){
		perror("Listen");
		close(sock);
		exit(1);
	}

	printf("\n");
	printf("%s: Server waiting form incoming calls ...\n",
			prognamed);

	err=ioctl(sock,SIOCG_LAPB_STATUS,0);
	if (err<0){
		perror("HDLC LINK STATUS");
	}
    printf("LAPB STATE = %s (err=%i)\n", err==1?"On":"Off",err);

	for (;;) {

		/* Accept call will block and wait for incoming calls.
                 * When a call comes in, a new socket will be created
                 * sock1. NOTE: that connection is not established yet.*/ 

		FD_ZERO(&readset);
		FD_ZERO(&oobset);
		FD_SET(sock,&readset);
		FD_SET(sock,&oobset);


		fflush(stdout);

		if (select(sock+1,&readset,NULL,&oobset,NULL)){

			if (FD_ISSET(sock,&oobset)){
				printf("%s: Listen sock rx OOB event!\n",
						prognamed);

				/* Set the state back to listen and wait
                                 * for connection to come up */
				if (listen(sock,10) != 0){
			                perror("Listen");
                			close(sock);
					break;
        		}

				err=ioctl(sock,SIOCG_LAPB_STATUS,0);
				if (err<0){
					perror("HDLC LINK STATUS");
				}
				printf("LAPB STATE = %s (err=%i)\n", err==1?"On":"Off",err);

			}
			
			if (FD_ISSET(sock,&readset)){
			
				printf("\n");

				sock1 = accept(sock,NULL,NULL);

				if (sock1 < 0){
					perror("Socket");
					printf ("%s: Sock Accept Call FAILED BAD,no sock %i\n",
							prognamed,sock1);
					continue;
				}	

				if ((err=ioctl(sock1,SIOC_X25_GET_CALL_DATA,&api_cmd)) != 0){
					perror("Get Call Data");
					printf("%s: Failed to Get Call data %i\n",
							prognamed,err);
					close(sock1);
					break;
				}
				
				api_cmd.data[api_cmd.hdr.length]=0;
				printf("%s: Rx Call Data Lcn %i String %s, MTU %i MRU %i\n",
						prognamed,
						api_cmd.hdr.lcn,api_cmd.data,
						api_cmd.hdr.mtu, api_cmd.hdr.mru);

				mtu=api_cmd.hdr.mtu;
				mru=api_cmd.hdr.mru;
				
				if ((err=ioctl(sock1,SIOC_X25_ACCEPT_CALL,0)) != 0){
					perror("Accept Call");
					printf("%s: Accept failed %i\n",
							prognamed,err);
					close(sock1);
					break;
				}
			
				printf("%s: Rx Call Accept Sock=%i Lcn=%i\n\n",
						prognamed,
						sock1,
						api_cmd.hdr.lcn);
				
				
				if (!fork()){
					close(sock);
					tx_rx_data(sock1,mtu,mru);	
					exit(0);
				}
				close(sock1);
				sock1=0;
			}
		}

		/* We must check if our children have died or not
                 * otherwise our process table will be full of 
                 * zombie children */

		/* Instead of having children send a SIGCHILD signal
                 * to the server indicating their death, we poll
                 * every time a new child is created.  This is not
                 * perfect but it works :) */  

		sig_chld(0);
	}

	sig_chld(0);
	
	fflush(stdout);
	return 0;
}

/*=================================================================
 * TX_RX_DATA 
 *
 *   o Send and Receive data via socket 
 *   o Create a tx packet using x25api_t data type. 
 *   o Both the tx and rx packets contains 64 bytes headers
 *     in front of the real data.  It is the responsibility
 *     of the applicatino to insert this 64 bytes on tx, and
 *     remove the 64 bytes on rx.
 *
 *	------------------------------------------
 *      |  64 bytes      |        X bytes        ...
 *	------------------------------------------
 * 	   Header              Data
 *
 *   o 64 byte Header: data structure:
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
 * 	Each tx data packet must contain the above 64 byte header!
 * 	
 * 	Only relevant byte in the 64 byte tx header, is the
 * 	QDM byte.  The driver will look at this byte to determine
 * 	if the Mbit (more data bit) should be set.
 * 		
 *       QDM: byte is a bit map of three bits:
 *
 * 	Q bit: Bit 2 : Qualifier bit, special kind of packet.
 *                      Can be used as control packet.  
 *       D bit: Bit 1 : Data acknolwedge bit. The remote will
 *                      acknowledge every packet sent.
 *       M bit: Bit 0 : If your packet is greater than x25 MTU,
 *                      i.e 1024 bytes, than cut the packet to MTU size
 *                          and set the M bit to indicate more data.                
 *
 * RX DATA:
 * --------
 * 	Each rx data will contain the above 64 byte header!
 *
 * 	Relevant bytes in the 64 byte rx header, are the
 * 	LCN and QDM bytes.
 *
 *	QDM: byte is a bit map of three bits:
 *
 * 	Q bit: Bit 2 : Qualifier bit, special kind of packet.
 *                      Can be used as control packet.  
 *       D bit: Bit 1 : Data acknolwedge bit. The remote will
 *                      acknowledge every packet sent.
 *       M bit: Bit 0 : If your packet is greater than x25 MTU,
 *                      i.e 1024 bytes, than cut the packet to MTU size
 *                          and set the M bit to indicate more data.  
 *                          
 * 	LCN = contains the lcn number for the rx frame.
 * 	
 * OOB DATA:
 * ---------
 * 	The OOB (out of band) data is used by the driver to 
 * 	indicate x25 events for the active channel: 
 * 	clear call, or restarts.
 *
 * 	Each OOB packet contains the above 64 byte header!
 *
 *	Relevant bytes in the 64 byte oob header, are the
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

void tx_rx_data(int sock, int mtu, int mru)
{
	unsigned int Tx_count=0;
	unsigned short timeout=0;
	int err=0, i, Rx_count=0;
	struct timeval tv;
	x25api_t* api_tx_el;
	fd_set	writeset,readset,oobset;
	int volatile send_ok=0;
	pid_t pid = getpid();
	
	sprintf(prognamed,"\t%s[%i]",APP_NAME,pid);

	/* If user leaves the tx_cnt as default 1
	 * we assume that user wants the server
	 * to send infinitely. Thus, only stop if
	 * the user explicity sets the tx_cnt to
	 * a non default value */
	if (tx_cnt==1){
		tx_cnt=0;
	}

	/* Specify timeout value to 5 seconds on select */
	tv.tv_usec = 0; 
	tv.tv_sec = 5;

	api_tx_el = (x25api_t *)Tx_data;

	if (tx_size > mtu){
		printf("%s: Tx Size %i greater than MTU %i, using MTU!\n",
				prognamed,
				tx_size,
				mtu);
		tx_size=mtu;
	}
	
	/* Fill in the packet data */ 
	for (i=0; i<tx_size ; i++){
               	api_tx_el->data[i] = (unsigned char) i;
	}

	/* Set the pid information used by the wanpipe
	 * proc file system: /proc/net/wanrouter/map 
	 * (Optional) */

	if ((err=ioctl(sock,SIOC_X25_SET_LCN_PID,&pid))){
		perror("Set LCN PID: ");
		printf("%s: Failed to set pid info\n",prognamed);	
		close(sock);
		return;
	}

	/* Set the label information used by the wanpipe
	 * proc file system: /proc/net/wanrouter/map 
	 * (Optional) */

	sprintf(prognamed,"%s",APP_NAME);
	if ((err=ioctl(sock,SIOC_X25_SET_LCN_LABEL,prognamed))){
		perror("Label: ");
		printf("%s: Failed to configure svc label\n",prognamed);
		close(sock);
		return;
	}

	sprintf(prognamed,"\t%s[%i]",APP_NAME,pid);


	for(;;) {
		FD_ZERO(&readset);
		FD_ZERO(&oobset);
		FD_ZERO(&writeset);
		
		FD_SET(sock,&readset);
		FD_SET(sock,&oobset);

		if (write_enable && send_ok){
			FD_SET(sock,&writeset);
		}

		/* This must be within the loop */	
		tv.tv_usec = 0; 
		tv.tv_sec = TIMEOUT;


		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if already queued packet
                 * cannot be sent 
		 */ 
	
	        if((err=select(sock + 1, &readset, &writeset, &oobset, &tv))){


			
			if (FD_ISSET(sock,&readset)){
				
				err = recv(sock, Rx_data, sizeof(Rx_data), 0);

				/* err contains number of bytes transmited */		
				if(err < 0 ) {
					perror("Recv");
					
					printf("%s: Failed to rcv %i , %i\n", 
							prognamed,Rx_count, err);
					
					if (ioctl(sock,SIOC_WANPIPE_SOCK_STATE,0)){
						printf("%s: Sock Disconnected !\n",
								prognamed);
						break;
					}
				}else{

					/* Every received packet comes with the 64
					 * bytes of header which driver adds on. 
                                         * Header is structured as x25api_hdr_t. 
					 * (same as above)
					 */
					x25api_hdr_t *api_data = 
							(x25api_hdr_t *)Rx_data;

					++Rx_count;

					err-=sizeof(x25api_hdr_t);
					if (err > mru){
						/* This should never happen, sanity check */
						printf("%s: Rx warning: rx len %i > mru %i\n",
								prognamed,
								err,mru);
					}
					
					if (api_data->qdm & M_BIT){
						/* This packet is part of a larger
						 * packet. Concatinate it... */
					}
				
					if (verbose){
						printf("%s:\tReceive OK, size: %i, qdm %x, lcn %i cnt: %i\n", 
							  prognamed,
							  err, api_data->qdm, 
							  api_data->lcn, 
							  Rx_count);
					}
					send_ok=1;

					//pause();
					//if ((Rx_count%50)==0){
					//	printf("Sleeping on Rx_count = %i\n",Rx_count);
					//	sleep(5);
					//}
				}
			}

			if (FD_ISSET(sock,&oobset)){
				if (handle_oob_event(sock) == 0){
					break;
				}
				continue;
			}


			if (FD_ISSET(sock,&writeset)){
				
				err = send(sock, Tx_data, 
					   tx_size + sizeof(x25api_hdr_t), 0);

				/* err contains number of bytes transmited */		
				if(err < 0) {
					if (errno != EBUSY){
						perror("Send");
						printf("%s: Failed to send %i , %i\n", 
								prognamed,
								Tx_count, err);

						if (ioctl(sock,SIOC_WANPIPE_SOCK_STATE,0)){
							printf("%s: Sock Disconnected !\n",
									prognamed);
							break;
						}
					}
				}else{
					++Tx_count;

					if (verbose){
						printf("%s: Packet Sent %i\n",prognamed,Tx_count);
					}
					send_ok=0;
				}				

			}
			

			/* Stop after number of frames if the
			 * tx_cnt is defined */
			
			if(tx_cnt && Tx_count >= tx_cnt){
				issue_clear_call(sock,CLEAR_WAIT_FOR_DATA,cause,diagn);
				break;
			}

			timeout=0;
		}else{
			if (err == 0){
				if (++timeout == 5){
					printf("%s: Sock timeout exceeded MAXIMUM\n",prognamed);
					break;			
				}		
				printf("%s: Sock timeout try again !!!\n",prognamed);
			}else{
				printf("%s: Error in Select !!!\n",prognamed);
				break;
			}
		}//if select
	}//for
	close(sock);
	printf("%s: Connection %i closed: Tx=%i Rx=%i\n",
			prognamed,sock,Tx_count,Rx_count);
} 


void sig_chld (int sigio)
{
	pid_t pid;
	int stat;

	while ((pid=waitpid(-1,&stat,WNOHANG)) > 0){
		//printf("%s: Child %d terminated\n",prognamed,pid);
	}

	return;
}

int handle_oob_event(int sock)
{
	int err;
	x25api_t* api_tx_el;
	
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
	if (err < 0){

		/* The state of the socket is disconnected.
		 * We must close the socket and continue with
		 * operatio */

		if ((err=ioctl(sock,SIOC_WANPIPE_SOCK_STATE,0)) == 0){
			return 1;
		}

		printf("%s: SockId=%i : OOB Event : State = %s\n",
			prognamed,sock, 
			err == 1 ? "Disconnected" : "(Dis/Con)necting");

		memset(&api_cmd,0,sizeof(api_cmd));
		if ((err=ioctl(sock,SIOC_X25_GET_CALL_DATA,&api_cmd)) == 0){
			if (decode_oob_event(&api_cmd,sock) == 0){
				close(sock);
				sock=0;
			}
		}else{
			close(sock);
			sock=0;
		}
		
		/* Do what ever you have to do to handle
		 * this condiditon */
	}else{

		/* OOB packet received OK ! */
		
		api_tx_el = (x25api_t *)Rx_data;

#if 0
		printf("%s: SockId=%i : OOB : Packet type 0x%02X, Cause 0x%02X,"
		    " Diagn 0x%02X, Result 0x%02X, Len %i, LCN %i\n",
			prognamed,
			sock,
			api_tx_el->hdr.pktType, 
			api_tx_el->hdr.cause,
			api_tx_el->hdr.diagn, 
			api_tx_el->hdr.result,
			api_tx_el->hdr.length,
			api_tx_el->hdr.lcn);
#endif

		if (decode_oob_event(api_tx_el,sock) == 0){
			close(sock);
			sock=0;
		}else{
			return 1;
		}
	}
	return 0;
}


int decode_oob_event(x25api_t* api_hdr, int sock)
{

	switch (api_hdr->hdr.pktType){

		case RESET_REQUEST_PKT:
			printf("%s: SockId=%i : OOB : Rx Reset Call : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					prognamed,
					sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);

			/* NOTE: we don't have to close the socket,
			 * since the reset doesn't clear the call
			 * however, it means that there is something really
			 * wrong and that data has been lost */
			return 1;
		
		
		case CLEAR_REQUEST_PKT:
			printf("%s: SockId=%i : OOB : Rx Clear Call : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					prognamed,
					sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			break;		
			
		case RESTART_REQUEST_PKT:
			printf("%s: SockId=%i : OOB : Rx Restart Req : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					prognamed,
					sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			
			break;
		case INTERRUPT_PKT:
			printf("%s: SockId=%i : OOB : Rx Interrupt Req : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					prognamed,
					sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			
			break;


		default:
			printf("%s: SockId=%i : OOB : Rx Type=0x%02X : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					prognamed,
					sock,
					api_hdr->hdr.pktType,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			break;
	}
	return 0;
}

/* ============================================================
 * issue_clear_call
 *
 * Input options (opt):
 * 	CLEAR_WAIT_FOR_DATA : Fail the clear call if data is
 * 	                      still pending the transmisstion.
 * 	                      This way tx data is not lost 
 * 	                      due to the clear call.
 * 	CLEAR_NO_WAIT: 	      Clear call regardless of current
 * 	                      pending data.
 *
 * NOTE: This function can block, which can cause buffer
 *       overrun on other lcn's.  Thus, it should be executed
 *       in a separate process.
 */ 

int issue_clear_call(int sock, int opt, int cause, int diagn)
{

	memset(&api_cmd,0,sizeof(x25api_t));

	api_cmd.hdr.cause=cause;
	api_cmd.hdr.diagn=diagn;
	
	if (verbose){
		printf("%s: Issuing Clear Call: SockId=%i Cause=%d  Diagn=%d\n\n",
			prognamed, sock,cause,diagn);
	}

	if (opt){
		while(ioctl(sock,SIOC_WANPIPE_CHECK_TX,NULL));
	}

	if (ioctl(sock,SIOC_X25_CLEAR_CALL,&api_cmd)){
		perror("Clear Call: ");
	}

	printf("%s: SockId=%i: Call Cleared !\n",
			prognamed,
			sock);

	close(sock);
	sock=0;
	return 0;
}


