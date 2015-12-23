/*****************************************************************************
* server_v1.c	X25 API: SVC Server Application
*
* Author(s):	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2001 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*
* Description:
*
* 	The server_v1.c utility will accept, user defined number, of 
* 	outgoing calls and tx/rx, user defined number, of packets on each
* 	active svc.  
*
*	The server_v1 architecture is the classic mulit-process server,
*	that spawns a child process in order to handle incoming connections.
*	
*	IMPORTANT: For large number of LCN's, please refer to the P-Thread
*	           server example pthread/ directory.  The thread based 
*	           architecture model is a much more scalable solution.
* 
* 	This utility should be used as an architectual model.  It is up to
* 	the user to handle all conditions of x25.  Please refer to the
* 	X25API programming manual for futher details.
*/


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
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
#define MAX_FRM_LGTH  	1400
#define NO_FRMS_TO_TX   1000	

#define MAX_X25_ADDR_SIZE	16
#define MAX_X25_DATA_SIZE 	129
#define MAX_X25_FACL_SIZE	110

unsigned char Tx_data[MIN_FRM_LGTH + sizeof(x25api_hdr_t)];
unsigned char Rx_data[1000];

/* Prototypes */
int MakeConnection( char * );
void process_con(int,int);
void sig_chld (int sigio);
void handle_oob_event(x25api_t* api_hdr, int sock);


/***************************************************************
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call process_con() to transmit data
 *
 **************************************************************/   

int main(int argc, char* argv[])
{
	if (argc != 2){
		printf("Usage: ./server <card name>\n");
		exit(0);
	}

	signal(SIGCHLD,&sig_chld);
	
	MakeConnection(argv[argc-1]);
	return 0;
};


/***************************************************
* MakeConnection
*
*   o Create a Socket
*   o Bind a socket to a wanpipe network interface
*
* IMPORTANT:
*     No signals are allowed in this part of the code.
*     Accept ioctl call, blocks and waits on a 
*     signal which indicates that command is accepted.
*     If external signal tries to wake up the application
*     the accept ioctl call will wake up and fail the connection.
*     
*     This will be fixed in future releases.
*/         

int MakeConnection (char *i_name ) 
{
	int err = 0;
	int sock, sock1;
	struct wan_sockaddr_ll sa;
	x25api_t api_cmd;
	fd_set	oob,readset;
	
	errno = 0;

	memset(&sa,0,sizeof(struct wan_sockaddr_ll));

	/* Create a new socket */
   	sock = socket( AF_WANPIPE, SOCK_RAW, 0);
   	if( sock < 0 ) {
      		perror("Socket");
      		return FALSE;
   	} 

	/* Fill in binding information. 
	 * Before we use listen() system call
         * a socket must be binded into a virtual
         * network interface svc_listen.
         * 
         * Card name must be supplied as well. In 
         * this case the user supplies the name
         * eg: wanpipe1.
         */ 
	sa.sll_family = AF_WANPIPE;
	sa.sll_protocol = htons(0x16);
	strcpy(sa.sll_device, "svc_listen");
	strcpy(sa.sll_card, i_name);
		
	/* Bind a sock using the above address structure */
        if(bind(sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) == -1){
                perror("bind");
		printf("Failed to bind socket to %s interface\n",i_name);
                return FALSE;
        }


	/* Put the sock into listening mode. Number 10 does not 
         * mean anything, since number of connections is depended
	 * on number of LCNs configured */

	listen(sock,10);


	for (;;){

		sock1=-1;
		FD_ZERO(&readset);
		FD_ZERO(&oob);
		FD_SET(sock,&readset);
		FD_SET(sock,&oob);


		/* IMPORATAN: We use select() to wait for an incoming call
		 * instead of just calling accept(), because the child
		 * exit would trigger a CHILD signal which we will catch 
		 * in order to clear the process status, however this signal
		 * would also wake up accept() system call. */
			

		if (select(sock+1,&readset,NULL,&oob,NULL) > 0){
	
			if (FD_ISSET(sock,&oob)){
				printf("OOB Msg on listen Sock State %i !\n",
						ioctl(sock,SIOC_WANPIPE_SOCK_STATE,0));

				/* The X25 link went down or the driver is
				 * in process of shutting down.  Use listen()
				 * to try to rebind to the x25 layer, and wait
				 * until it comes up.  If listen() fails the
				 * driver is in shutdown process. */

				/* Run listen() and try to rebind to the
				 * driver */
				if (listen(sock,10) == 0){
					/* Listen successful: Listen socket will stay
				 	 * connected to the x25 layer until it
				 	 * comes back up. */
					continue;
				}
				
				printf("Listen closing on OOB: Sock State %i !\n",
					ioctl(sock,SIOC_WANPIPE_SOCK_STATE,0));

				/* Failed to listen(), the wanpipe
				 * driver is shutting down. Thus
				 * get out. */
				close(sock);
				break;
			}

			if (FD_ISSET(sock,&readset)){
				/* Accept call will block and wait for incoming calls.
				 * When a call comes in, a new socket will be created
				 * sock1. NOTE: that connection is not established yet.*/ 

				sock1 = accept(sock,NULL,NULL);
			}
		}else{
			continue;
		}
		
		if (sock1 < 0){
			printf ("Sock Accept Call Failed! %i\n",sock1);
			continue;
		}	

		/* Reset the api command structure */
		memset(&api_cmd, 0, sizeof(x25api_t));

		/* The GET CALL DATA ioctl call will write in the 
                 * incoming call data into api_cmd structure. Once
                 * we have call data information, it is up to the 
                 * user to accept or clear call. */
		
		if ((err=ioctl(sock1,SIOC_WANPIPE_GET_CALL_DATA,&api_cmd)) < 0){
			perror("GET CALL");
			printf ("Get Call Data Failed %i\n",err);
			close(sock1);
			continue;
		}else{
			printf("Call data: Lcn %i, Data %s\n",
				api_cmd.hdr.lcn,api_cmd.data);
		}

		/* In this case, I accept all calls. 
                 * The ACCEPT CALL ioctl will acknowledge connecton */ 

                /* The user can send data along with the call accept
                 * indication, by submiting a filled in api_cmd 
                 * structure in the third field of the ioctl call. */

		if ((err=ioctl(sock1,SIOC_WANPIPE_ACCEPT_CALL,0)) != 0){
			printf("Accept failed %i\n",err);
			close(sock1);
			continue;
		}else{
			printf("Accept OK %i\n",api_cmd.hdr.lcn);
		}


		/* If the call is accepted, fork a child to service
                 * the call */

		if (!fork()){
			close(sock);
			printf("CHILD Sock State %i CMD %i!\n",
						ioctl(sock1,SIOC_WANPIPE_SOCK_STATE,0),
						SIOC_WANPIPE_SOCK_STATE);
			process_con(sock1,api_cmd.hdr.lcn);	
			exit(0);
		}
		
		close(sock1);
	}
	
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
* 	Only relevant byte in the 16 byte tx header, is the
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
* 	Each rx data will contain the above 16 byte header!
*
* 	Relevant bytes in the 16 byte rx header, are the
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



void process_con(int sock1, int lcn)
{
	unsigned int Tx_count=0, Rx_count=0;
	unsigned short Tx_lgth;
	x25api_t* api_tx_el;
	int err=0, i=0;
	fd_set	writeset, readset, oobset;
	x25api_t api_cmd;

	memset(&api_cmd, 0, sizeof(x25api_t));

	/* Initialize the tx packet length */
	Tx_lgth = MIN_FRM_LGTH;
	
	/* Cast the x25 16 byte header to the begining
	 * of the tx packet.  Using the pointer fill 
	 * in appropriate header information such as
	 * QDM bits */
	api_tx_el = (x25api_t *)Tx_data;
	
	/* Initialize the 16 byte tx header */
	memset(api_tx_el, 0, sizeof(x25api_hdr_t));
 
	/* Set the Mbit (more bit) 
	 * currently this option is disabled */
#ifdef _MORE_BIT_SET_
	api_tx_el->hdr.qdm = 0x01
#endif	

	/* Fill in the tx packet data with arbitrary
	 * information */ 
	for (i=0; i<Tx_lgth ; i++){
               	api_tx_el->data[i] = (unsigned char) i;
	}

	/* Start an infinite loop which will tx/rx data 
	 * over connected x25 svc's */
	for(;;) {
		FD_ZERO(&writeset);
		FD_ZERO(&oobset);	
		FD_ZERO(&readset);

		//FD_SET(sock1,&writeset);
		FD_SET(sock1,&oobset);
		FD_SET(sock1,&readset);


			
		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if the socket cannot send
		 * or there is nothing to receive.
		 *
		 * By using the last socket file descriptor +1 select will wait
		 * for all active x25 sockets.
		 *
		 * If the NONBLOCKING option has been used during connect()
		 * the select will wait untill the channel is connected 
		 * (i.e. accept has been received).  Once the channel is connected
		 * the tx and rx will start.  However, if the call is cleared for
		 * any reason, the OOB message will indicate that event.
		 */ 

	        if(select(sock1 + 1, &readset, &writeset, &oobset, NULL)){


			if (FD_ISSET(sock1, &oobset)){

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

				err = recv(sock1, Rx_data, 
						sizeof(Rx_data), MSG_OOB);
		
				/* err contains number of bytes transmited */		
				if(err < 0 ) {

					/* SVC is disconnected */
					printf("Failed to read OOB %i , %i\n", 
							Tx_count, err);
					break;
				}else{
					api_tx_el = (x25api_t *)Rx_data;
					handle_oob_event(api_tx_el,sock1);
					break;
				}
			}

			if (FD_ISSET(sock1, &writeset)){

				/* This socket is ready to tx */
					
				/* The tx packet length contains the 16 byte
				 * header. The tx packet was created above. */
				err = send(sock1, Tx_data, 
						Tx_lgth+sizeof(x25api_hdr_t), 0);

				/* err contains number of bytes transmited */		
				if(err > 0 ) {
					printf("Child LCN %i Packet Sent OK %i\n",
						lcn,++Tx_count);
				}
				/* If err<=0 it means that the send failed and that
				 * driver is busy.  Thus, the packet should be
				 * requeued for re-transmission on the next
				 * try !!!! 
				 */
			}
			
			
			if (FD_ISSET(sock1, &readset)){

				/* If there are pending rx packets
				 * for a LISTEN socket, it means that
				 * calls are pending.  Thus, accept 
				 * a pending call */

				err = recv(sock1, Rx_data, sizeof(Rx_data), 0);

				/* err contains number of bytes transmited */		
				if(err < 0 ) {
					printf("Failed to rcv %i , %i\n", 
							Rx_count, err);
				}else{
					x25api_hdr_t *api_data = 
							(x25api_hdr_t *)Rx_data;

					if (api_data->qdm & 0x01){
						/* More bit is set, thus
						 * handle it accordingly */
					}

					printf("\tReceive OK, size: %i, qdm %x,"
						" cause %x, diagn %x, cnt: %i\n", 
						  err, api_data->qdm, 
						  api_data->cause, 
						  api_data->diagn, ++Rx_count);
				}

			}
		
#if 0
			/* Stop after number of frames */
			if(Tx_count == NO_FRMS_TO_TX){

				/* Clear a call using CLEAR_CALL ioctl function.
				 * User can send data along with the clear 
                                 * indication by submitting x25api_t structure
                                 * in the third field of the ioctl call */
				memset(&api_cmd,0,sizeof(x25api_t));
				while(1){
					api_cmd.hdr.qdm = 0x80;
					if ((err=ioctl(sock1,SIOC_WANPIPE_CLEAR_CALL,&api_cmd)) == 0){
						printf("Clear Call OK sock %i\n",lcn);
						break;
					}

					if (ioctl(sock1,SIOC_WANPIPE_SOCK_STATE,0) == 1){
						/* Sock is disconnected */
						break;
					}
					usleep(100);
				}
				break;
			}
#endif
		}//if select
	}//For

	close(sock1);
	return;
} 


void sig_chld (int sigio)
{
	pid_t pid;
	int stat;

	while ((pid=waitpid(-1,&stat,WNOHANG)) > 0){
		printf("Child %d terminated\n",pid);
	}

	return;
}


void handle_oob_event(x25api_t* api_hdr, int sock)
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



