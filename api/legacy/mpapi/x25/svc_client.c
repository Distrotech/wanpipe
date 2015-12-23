/*****************************************************************************
* svc_client.c	X25 API: SVC Client Application 
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
* 	The svc_utility utility will establish, user defined number, of 
* 	outgoing calls and tx/rx, user defined number, of packets on each
* 	active svc.  
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
#include <signal.h>
#include <time.h>
#include <netinet/in.h>
#include <linux/if_wanpipe.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <linux/wanpipe.h>
#include <linux/wanpipe_x25_kernel.h>
#include <linux/if_ether.h>
#include "lib_api.h"


/*===========================================================
 * Defines 
 *==========================================================*/

#define FALSE	0
#define TRUE	1


#define TX_PACKET_LEN	5000	
#define MAX_RX_PKT_LEN  5000

#define MAX_X25_ADDR_SIZE	16
#define MAX_X25_DATA_SIZE 	129
#define MAX_X25_FACL_SIZE	110

#define MAX_SOCK_NUM 255

#define TIMEOUT 10

#define NOWAIT_ON_CONNECT 1

#define APP_NAME "svc_client"


/*===========================================================
 * Global Variables 
 *==========================================================*/


/* A single packet will be used to simulate tx data */
unsigned char Tx_data[TX_PACKET_LEN];

/* A singel Rx buffer will be used to accept incoming
 * data.  The buffer will be overwritten with each new
 * packet. */
unsigned char Rx_data[MAX_RX_PKT_LEN];


/* For each socket created, we will keep the
 * socket file descriptor and tx/rx pkt counts */
typedef struct sock_api {
	int sock;
	int tx;
	int rx;
	int mtu;
	int mru;
}sock_api_t;

sock_api_t sock_fd[MAX_SOCK_NUM];

int max_sock_fd=0;

/* X25 tx/rx header structure.  This structure will
 * be used to send commands down to the driver, as
 * well as read/insert header information from/into
 * rx and tx data respectively.
 * 
 * Defined globaly so it would only get allocated once.  
 */  

x25api_t api_cmd;

unsigned char prognamed[100];


/*============================================================
  Function Prototypes 
 *===========================================================*/
int MakeConnection( char * );
void tx_rx_data( void);
void setup_signal_handlers (void);
void quit (int);
int decode_oob_event(x25api_t* api_hdr, int sk_index);
int issue_clear_call(int i, int opt, int cause, int diagn);
void sig_chld (int sigio);
void sig_hnd (int sigio);
int rx_data_act(int i);
int tx_data_act(int i,int Tx_lgth);
int handle_oob_event(int sk_index);
void close_connection(int i);



/*=============================================================
 * Main:
 *
 *    o Make a socket connection to the driver.
 *    o Call tx_rx_data() to transmit/receive data
 *
 *============================================================*/   

int main(int argc, char* argv[])
{
	int proceed;
	pid_t pid;
	int stat;
	int i;

	sprintf(prognamed,"%s[%i]",APP_NAME,getpid());
	
	card_cnt=1;
	if (init_args(argc,argv) == 0){
		usage(argv[0]);
		return -EINVAL;
	}

	signal(SIGCHLD,&sig_chld);
	signal(SIGINT,&sig_hnd);
	signal(SIGTERM,&sig_hnd);

	memset(sock_fd,0,sizeof(sock_fd));

	printf("\n%s: Attempting to establish %i calls, and tx %i (%i bytes) packets \n",
			prognamed, tx_channels, tx_cnt, tx_size);

	proceed = MakeConnection(if_name);
	if (proceed == TRUE){

		tx_rx_data();

		/* Wait for the clear call children */
		while ((pid=waitpid(-1,&stat,WUNTRACED)) > 0){
			//printf("Child %d terminated\n",pid);
		}
	}

	for (i=0;i<tx_channels;i++){
		if (!sock_fd[i].sock)
			continue;
		
		close(sock_fd[i].sock);
		sock_fd[i].sock=0;
	}


	return 0;
};


/*===========================================================
 * MakeConnection
 *
 * For user defined number of connections
 *   o Create a Socket
 *   o Bind a socket to a wanpipe network interface
 *   o Setup x25 place call information
 *   o Place call using connect()
 *   o Read out the lcn used for that connection
 *==========================================================*/         

int MakeConnection (char *i_name ) 
{
	int len = sizeof(struct wan_sockaddr_ll);
	int err = 0;
	struct wan_sockaddr_ll 	sa;
	int i;
	int calls_issued=0;
	pid_t pid;


	for (i=0; i<tx_channels; i++){
	
		memset(&sa,0,sizeof(struct wan_sockaddr_ll));

		/* Create a new socket */
   		sock_fd[i].sock = socket( AF_WANPIPE, SOCK_RAW, AF_WANPIPE);
   		if( sock_fd[i].sock < 0 ) {
      			perror("Socket: ");
			sock_fd[i].sock=0;
			return FALSE;
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
		sa.sll_family = AF_WANPIPE;
		sa.sll_protocol = htons(ETH_P_X25);
		strcpy(sa.sll_device, "svc_connect");
		strcpy(sa.sll_card, i_name);
	

		/* Bind a sock using the above address structure */
		if(bind(sock_fd[i].sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) == -1){
			perror("Bind: ");
			printf("Failed to bind socket to %s interface\n",i_name);
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			continue;
		}


		/* Reset the api command structure */
		memset(&api_cmd, 0, sizeof(x25api_t));

		/* Write the call data into the command structure.
		 * Note: -u data must be have even number of characters.
		 *       -f field cannot be set to zero (i.e. -f0) 
		 *       Refer to x25.pdf hardware manual.
		 */

		sprintf(daddr,"1");
		sprintf(saddr,"3");
		sprintf(udata,"01");

		sprintf(api_cmd.data,"-d%s -s%s -u%s -f0180",
				daddr,
				saddr,
				udata);
		api_cmd.hdr.length = strlen(api_cmd.data);

		/* Write the call data into a socket mail box using
		 * the following ioctl call.  This must be one before
		 * the connect system call 
		 */
		if ((err=ioctl(sock_fd[i].sock,SIOC_X25_SET_CALL_DATA,&api_cmd)) != 0){
			perror("Set Call Data: ");
			printf ("%s: Setting call data Failed 0x%X!\n",
					prognamed,SIOC_X25_SET_CALL_DATA);
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			continue;
		} 

		/* Set the pid information used by the wanpipe
		 * proc file system: /proc/net/wanrouter/map 
		 * (Optional) */
		
		pid=getpid();
		if ((err=ioctl(sock_fd[i].sock,SIOC_X25_SET_LCN_PID,&pid))){
			perror("Set LCN PID: ");
			printf("Failed to sending down my pid %i\n",pid);	
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			continue;
		}

		/* Set the label information used by the wanpipe
		 * proc file system: /proc/net/wanrouter/map 
		 * (Optional) */
		sprintf(prognamed,"%s",APP_NAME);
		if ((err=ioctl(sock_fd[i].sock,SIOC_X25_SET_LCN_LABEL,prognamed))){
			perror("Label: ");
			printf("%s: Failed to configure label\n",prognamed);	
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			continue;
		}

		sprintf(prognamed,"%s[%i]",APP_NAME,getpid());

		
		/* Now we are ready to place a call. The address
		 * structure is optional. The sa structure will contain
		 * a network interface name on which the connection was
		 * established 
		 */
		if (connect(sock_fd[i].sock, (struct sockaddr *)&sa, len) != 0){
			perror("Connect: ");
			printf("%s: Failed to place call !\n",prognamed);
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			continue;
		}

		/* In order to see what lcn we have established connection on,
		 * run GET CALL DATA ioctl call, which will return the
		 * appropriate information into api_cmd structure 
		 */
		if ((err=ioctl(sock_fd[i].sock,SIOC_X25_GET_CALL_DATA,&api_cmd)) != 0){
			perror("Get Call Data: ");
			printf("%s: Failed to obtain call data!\n",prognamed);
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			continue;
		}

		/* Determine the negotiated values such as lcn number
		 * mtu and mru */
		sock_fd[i].mtu=api_cmd.hdr.mtu;
		sock_fd[i].mru=api_cmd.hdr.mru;

		printf("%s: Call Bounded to LCN=%i MTU=%i MRU=%i \n",
					prognamed,
					api_cmd.hdr.lcn,
					api_cmd.hdr.mtu,
					api_cmd.hdr.mru);
	
		calls_issued++;

		if (sock_fd[i].sock > max_sock_fd){
			max_sock_fd=sock_fd[i].sock;
		}

		/* Flush all pending print statements to screen */
		fflush(stdout);
	}
	
	/* At this point all the calls has been placed.  If the connect() function
	 * has been used in blocking mode, then we will wait in select() for
	 * the accept calls to come in.  Otherwise, the calls have been accepted,
	 * and can proceed to tx and rx data. */

	printf("%s: Issued %i Calls, Requested %i\n\n\n",
			prognamed,calls_issued,tx_channels);
	
	return TRUE;
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

void tx_rx_data(void)
{
	unsigned short timeout=0;
	int err=0, i; 
	struct timeval tv;
	x25api_t* api_tx_el;
	fd_set	writeset,readset,oobset;

	/* Setup a timeout value for the select statement, which is
	 * going to block until the socket(s) are/is ready to tx/rx
	 * or oob.  In this case we will timeout afte 10 seconds of
	 * inactivity */
	tv.tv_usec = 0; 
	tv.tv_sec = 10;


	/* Cast the x25 64 byte header to the begining
	 * of the tx packet.  Using the pointer fill 
	 * in appropriate header information such as
	 * QDM bits */
	api_tx_el = (x25api_t *)Tx_data;

	/* Initialize the 64 byte header */
	memset(api_tx_el, 0, sizeof(x25api_hdr_t));

	/* Set the Mbit (more bit) 
	 * currently this option is disabled */
#ifdef MORE_BIT_SET
	api_tx_el->hdr.qdm = 0x01
#endif	
	
	/* Fill in the tx packet data with arbitrary
	 * information */ 
	for (i=0; i<tx_size ; i++){
               	api_tx_el->data[i] = (unsigned char) i;
	}


	/* Start an infinite loop which will tx/rx data 
	 * over connected x25 svc's */

	for(;;) {
		
		tv.tv_usec = 0; 
		tv.tv_sec = 10;

		/* Initialize select() flags */
		FD_ZERO(&readset);
		FD_ZERO(&oobset);
		FD_ZERO(&writeset);

		/* For all connected sockets, tell the select
		 * what to wait for.  Tx, Rx and OOB */
		for (i=0;i<tx_channels;i++){

			if (!sock_fd[i].sock)
				continue;
		
			FD_SET(sock_fd[i].sock,&readset);
			FD_SET(sock_fd[i].sock,&oobset);

			if (write_enable){
				FD_SET(sock_fd[i].sock,&writeset);
			}	
		}
		
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
		
	        if((err=select(max_sock_fd + 1,&readset, &writeset, &oobset, &tv))){

			/* One of the sockets returned OK for tx rx or oob 
			 * Thus, for all waiting connections, check each flag */
			
			for (i=0;i<tx_channels;i++){
		
				if (!sock_fd[i].sock)
					continue;

				/* First check for OOB messages */
				if (FD_ISSET(sock_fd[i].sock,&oobset)){
					if (handle_oob_event(i)){
						close_connection(i);
					}
					goto svc_action;
				}

				if (FD_ISSET(sock_fd[i].sock,&readset)){
					if (rx_data_act(i)){
						close_connection(i);
						goto svc_action;
					}
				}

				if (FD_ISSET(sock_fd[i].sock,&writeset)){
					
					tx_data_act(i,tx_size);

					/* In this example I am clearing the connection
					 * after I sent user specified number of packets */
					if (tx_delay)
						sleep(tx_delay);


					//if (sock_fd[i].tx == 10){
					//	ioctl(sock_fd[i].sock,SIOC_X25_RESET_CALL,0);
					//}
					
					if (sock_fd[i].tx >= tx_cnt){
						
						issue_clear_call(i, CLEAR_WAIT_FOR_DATA, cause, diagn);
						close_connection(i);
					}	
				}
			}
svc_action:
			/* All channels are down ! */
			if (!max_sock_fd){
				break;
			}

		}else{

			/* select timeout occured.  The svc could be 
			 * waiting for the accept, or there is something wrong
			 * with the x25 configuration. 
			 *
			 * It is up to the user to handle this condition
			 * as they see fit */
			
			if (err == 0){
				if (++timeout == 5){
					printf("Select() timeout exceeded MAXIMUM\n");
					break;			
				}		
				printf("Select() timeout try again !!!\n");
			}else{
				printf("Error in Select() !!!\n");
				break;
			}
		}//if select
	}//for

	for (i=0;i<tx_channels;i++){

		if (!sock_fd[i].sock)
			continue;
	
		close(sock_fd[i].sock);
		sock_fd[i].sock=0;
	}
} 


int tx_data_act(int i, int Tx_lgth)
{
	int err;
	x25api_t* api_tx_el = (x25api_t *)Tx_data;

	
	/* This socket is ready to tx */
#if 0
	if (Tx_lgth > sock_fd[i].mtu){
		printf("%s: Tx warning: Tx length %i > MTU %i, using MTU!\n",
				prognamed,Tx_lgth,sock_fd[i].mtu);
		Tx_lgth=sock_fd[i].mtu;
	}
#endif	

	/* The tx packet length contains the 64 byte
	 * header. The tx packet was created above. */
	err = send(sock_fd[i].sock, Tx_data, 
		   Tx_lgth + sizeof(x25api_hdr_t), 0);

	/* err contains number of bytes transmited */		
	if (err>0){
		++sock_fd[i].tx;
		if (verbose){
			printf("%s: SockId=%i : TX : size=%i, qdm=0x%02X, cnt=%i\n",
					prognamed,
					sock_fd[i].sock,
					Tx_lgth,
					api_tx_el->hdr.qdm,
					sock_fd[i].tx);
		}
	}else{
		if (errno != EBUSY){
			perror("Send");
		}
	}				
	
	/* If err<=0 it means that the send failed and that
	 * driver is busy.  Thus, the packet should be
	 * requeued for re-transmission on the next
	 * try !!!! 
	 */

	return 0;
}


int rx_data_act(int i)
{

	int err;
	
	/* This socket has received a packet. */
					
	err = recv(sock_fd[i].sock, Rx_data, sizeof(Rx_data), 0);
	
	/* err contains number of bytes transmited */		
	if(err < 0 ) {
		
		perror("Recv");

		/* This should never happen. However, check the sock state:
		 * if we are still connected, proceed with normal operation.
		 */
		if ((err=ioctl(sock_fd[i].sock,SIOC_WANPIPE_SOCK_STATE,0)) == X25_OK){
			return 0;
		}
		
		printf("%s: Sockid=%i : Failed to rcv packet %i\n",
				prognamed,
				sock_fd[i].sock,
				err);
		return 1;
		
	}else{

		/* Packet received OK !
		 *
		 * Every received packet comes with the 64
		 * bytes of header which driver adds on. 
		 * Header is structured as x25api_hdr_t. 
		 * (same as above)
		 */
		
		x25api_hdr_t *api_data = 
				(x25api_hdr_t *)Rx_data;

		if (!read_enable){
			return 0;
		}
		

		err-=sizeof(x25api_hdr_t);

		if (err > sock_fd[i].mru){
			/* This should never happen, sanity check */
			printf("%s: Rx warning: Rx len %i > MRU %i\n",
					prognamed,
					err,sock_fd[i].mru);
		}	
		
		if (api_data->qdm & 0x01){
			
			/* More bit is set, thus
			 * handle it accordingly
			 * FIXME */
		}

		++sock_fd[i].rx;
		
		if (verbose){
			printf("%s: SockId=%i : RX : size=%i, qdm=0x%02X, cnt=%i\n", 
				  prognamed,
				  sock_fd[i].sock, 
				  err,
				  api_data->qdm, 
				  sock_fd[i].rx);
		}
	}
	return 0;
}

int handle_oob_event(int i)
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

	err = recv(sock_fd[i].sock, Rx_data, sizeof(Rx_data), MSG_OOB);

	/* err contains number of bytes transmited */		
	if (err < 0){

		/* The state of the socket is disconnected.
		 * We must close the socket and continue with
		 * operatio */

		if ((err=ioctl(sock_fd[i].sock,SIOC_WANPIPE_SOCK_STATE,0)) == X25_OK){
			return 0;
		}

		printf("%s: SockId=%i : OOB Event : State = %s\n",
			prognamed,sock_fd[i].sock, 
			err == 1 ? "Disconnected" : "(Dis/Con)necting");

		memset(&api_cmd,0,sizeof(api_cmd));
		if ((err=ioctl(sock_fd[i].sock,SIOC_X25_GET_CALL_DATA,&api_cmd)) == 0){
			if (decode_oob_event(&api_cmd, i) == 0){
				return 1;
			}
		}else{
			return 1;
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
			sock_fd[i].sock,
			api_tx_el->hdr.pktType, 
			api_tx_el->hdr.cause,
			api_tx_el->hdr.diagn, 
			api_tx_el->hdr.result,
			api_tx_el->hdr.length,
			api_tx_el->hdr.lcn);
#endif
		if (decode_oob_event(api_tx_el, i) == 0){
			return 0;
		}else{
			return 1;
		}
	}
	return 0;
}


int decode_oob_event(x25api_t* api_hdr, int sk_index)
{

	switch (api_hdr->hdr.pktType){

		case RESET_REQUEST_PKT:
			printf("%s: SockId=%i : OOB : Rx Reset Call : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					prognamed,
					sock_fd[sk_index].sock,
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
					sock_fd[sk_index].sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			break;		
			
		case RESTART_REQUEST_PKT:
			printf("%s: SockId=%i : OOB : Rx Restart Req : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					prognamed,
					sock_fd[sk_index].sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			
			break;
		case INTERRUPT_PKT:
			printf("%s: SockId=%i : OOB : Rx Interrupt Req : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					prognamed,
					sock_fd[sk_index].sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			
			break;


		default:
			printf("%s: SockId=%i : OOB : Rx Type=0x%02X : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					prognamed,
					sock_fd[sk_index].sock,
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

int issue_clear_call(int i, int opt, int cause, int diag)
{
	//FIXME: Fork is expensive use Pthread
	if (fork() == 0){

		sprintf(prognamed,"%s[%i]",APP_NAME,getpid());	
		
		if (opt){
			int cnt=0;
			while(ioctl(sock_fd[i].sock,SIOC_WANPIPE_CHECK_TX,NULL)){
				++cnt;
			}
			//printf("%s: Wait Count %i\n",prognamed,cnt);
		}

		memset(&api_cmd,0,sizeof(x25api_t));
		api_cmd.hdr.cause=cause;
		api_cmd.hdr.diagn=diag;

		printf("\n%s: Issuing Clear Call: SockId=%i, Cause=%d Diagn=%d\n\n",
			prognamed, sock_fd[i].sock, cause, diag);

		if (ioctl(sock_fd[i].sock,SIOC_X25_CLEAR_CALL,&api_cmd)){
			perror("Clear Call: ");
		}

		close(sock_fd[i].sock);
		sock_fd[i].sock=0;
		exit(0);
	}
	return 0;
}

void sig_chld (int sigio)
{
	pid_t pid;
	int stat;

	while ((pid=waitpid(-1,&stat,WNOHANG)) > 0){
		//printf("Child %d terminated\n",pid);
	}

	return;
}

void sig_hnd (int sigio)
{
	int i;

	for (i=0;i<tx_channels;i++){
		if (!sock_fd[i].sock)
			continue;
		
		close(sock_fd[i].sock);
		sock_fd[i].sock=0;
	}
	
	exit (0);
}

void new_max_sock_fd(void)
{
	int i;
	max_sock_fd=0;
	for (i=0;i<tx_channels;i++){
		if (sock_fd[i].sock > max_sock_fd){
			max_sock_fd=sock_fd[i].sock;
		}
	}
}


void close_connection(int i)
{
	close(sock_fd[i].sock);

	printf("%s: Connection SockId=%i closed: Tx=%i Rx=%i\n",
			prognamed,
			sock_fd[i].sock,
			sock_fd[i].tx,
			sock_fd[i].rx);
	
	if (max_sock_fd == sock_fd[i].sock){
		sock_fd[i].sock=0;
		new_max_sock_fd();
		//printf("%s: Finding new max sock %i\n",prognamed,max_sock_fd);
	}
	sock_fd[i].sock=0;
}

