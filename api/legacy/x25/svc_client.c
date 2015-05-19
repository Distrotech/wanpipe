/*****************************************************************************
* svc_client.c	X25 API: SVC Client Application 
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
* 	The call_place utility will establish, user defined number, of 
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
#include <linux/sdla_x25.h>
#include <linux/if_ether.h>


/*===========================================================
 * Defines 
 *==========================================================*/

#define FALSE	0
#define TRUE	1

#define TX_PACKET_LEN	1428	
#define NO_FRMS_TO_TX   100	
#define MAX_RX_PKT_LEN  4096

#define MAX_X25_ADDR_SIZE	16
#define MAX_X25_DATA_SIZE 	129
#define MAX_X25_FACL_SIZE	110

#define MAX_SOCK_NUM 255

#define TIMEOUT 10

#define NOWAIT_ON_CONNECT 1

#define CLEAR_NO_WAIT 0x00
#define CLEAR_WAIT_FOR_DATA 0x80

#define APP_NAME "[svc_client]"


/*===========================================================
 * Global Variables 
 *==========================================================*/


/* A single packet will be used to simulate tx data */
unsigned char Tx_data[TX_PACKET_LEN + sizeof(x25api_hdr_t)];

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
	char clear;
}sock_api_t;

sock_api_t sock_fd[MAX_SOCK_NUM];

int max_sock_fd=0;

int con_max=0;
int tx_pkt=0;


/* X25 tx/rx header structure.  This structure will
 * be used to send commands down to the driver, as
 * well as read/insert header information from/into
 * rx and tx data respectively.
 * 
 * Defined globaly so it would only get allocated once.  
 */  

x25api_t api_cmd;



/*============================================================
 * Function Prototypes 
 *===========================================================*/
int MakeConnection( char * );
void tx_rx_data( void);
void setup_signal_handlers (void);
void quit (int);
int decode_oob_event(x25api_t* api_hdr, int sk_index);
int issue_clear_call(int i);
void sig_chld (int sigio);
void sig_hnd (int sigio);
int rx_data_act(int i);
int tx_data_act(int i,int Tx_lgth);
int handle_oob_event(int sk_index);




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
	
	if (argc != 4){
		printf("Usage: ./wait_client <card name> <num of calls> < num of packets to send> \n");
		exit(0);
	}

	signal(SIGCHLD,&sig_chld);
	signal(SIGINT,&sig_hnd);
	signal(SIGTERM,&sig_hnd);
	con_max = atoi(argv[2]);
	tx_pkt = atoi(argv[3]);

	memset(sock_fd,0,sizeof(sock_fd));

	printf("\n%s: Attempting to establish %i calls, and tx %i packets \n",
			APP_NAME, con_max, tx_pkt);

	proceed = MakeConnection(argv[1]);
	if (proceed == TRUE){

		tx_rx_data();

		/* Wait for the clear call children */
		while ((pid=waitpid(-1,&stat,WUNTRACED)) > 0){
			//printf("Child %d terminated\n",pid);
		}
	}

	for (i=0;i<con_max;i++){
		if (!sock_fd[i].sock || sock_fd[i].clear)
			continue;
		
		ioctl(sock_fd[i].sock,SIOC_WANPIPE_CLEAR_CALL,NULL);
		close(sock_fd[i].sock);
		sock_fd[i].sock=0;
		sock_fd[i].clear=1;
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

#ifdef	NOWAIT_ON_CONNECT
	printf("%s: All sockets will use nonblocking call request!\n\n",APP_NAME);
#endif
	
	for (i=0; i<con_max; i++){
	
		memset(&sa,0,sizeof(struct wan_sockaddr_ll));

		/* Create a new socket */
   		sock_fd[i].sock = socket( AF_WANPIPE, SOCK_RAW, 0);
   		if( sock_fd[i].sock < 0 ) {
      			perror("Socket");
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
		sa.sll_protocol = htons(X25_PROT);
		strcpy(sa.sll_device, "svc_connect");
		strcpy(sa.sll_card, i_name);
	

		/* Bind a sock using the above address structure */
		if(bind(sock_fd[i].sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) == -1){
			perror("bind");
			printf("Failed to bind socket to %s interface\n",i_name);
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			return FALSE;
		}


		/* Reset the api command structure */
		memset(&api_cmd, 0, sizeof(x25api_t));

		/* Write the call data into the command structure.
		 * Note: -u data must be have even number of characters.
		 *       -f field cannot be set to zero (i.e. -f0) 
		 *       Refer to x25.pdf hardware manual.
		 */
		sprintf(api_cmd.data,"-d1111 -s2222 -u1111");
		api_cmd.hdr.length = strlen(api_cmd.data);

		/* Write the call data into a socket mail box using
		 * the following ioctl call.  This must be one before
		 * the connect system call 
		 */
		if ((err=ioctl(sock_fd[i].sock,SIOC_WANPIPE_SET_CALL_DATA,&api_cmd)) != 0){
			printf ("%s: Setting call data Failed!\n",APP_NAME);
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			return FALSE;
		}

		/* In some cases when placing multiple calls, this option
		 * should be enabled to prevent the connect() call from
		 * blocking and waiting for a response. 
		 *
		 * By tunning SET NON_BLOCK ioctl call, the 
		 * connect() system call will not wait for the remote reply. 
		 */
#ifdef NOWAIT_ON_CONNECT
		if ((err=ioctl(sock_fd[i].sock,SIOC_WANPIPE_SET_NONBLOCK,0)) < 0){
			printf ("%s: Failed to setup socket for NON BLOCKING call request!",APP_NAME);
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			return FALSE;
		}
#endif
		
		/* Now we are ready to place a call. The address
		 * structure is optional. The sa structure will contain
		 * a network interface name on which the connection was
		 * established 
		 */
		if (connect(sock_fd[i].sock, (struct sockaddr *)&sa, len) != 0){
			perror("Connect: ");
			printf("%s: Failed to place call !\n",APP_NAME);
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			i--;
			continue;
		}

		/* In order to see what lcn we have established connection on,
		 * run GET CALL DATA ioctl call, which will return the
		 * appropriate information into api_cmd structure 
		 */
		if ((err=ioctl(sock_fd[i].sock,SIOC_WANPIPE_GET_CALL_DATA,&api_cmd)) != 0){
			printf("%s: Failed to obtain call data!\n",APP_NAME);
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			return FALSE;
		}else{
			printf("%s: Call Bounded to LCN %i\n",
					APP_NAME,api_cmd.hdr.lcn);
		}

		/* Flush all pending print statements to screen */
		fflush(stdout);
	}
	
	/* At this point all the calls has been placed.  If the connect() function
	 * has been used in blocking mode, then we will wait in select() for
	 * the accept calls to come in.  Otherwise, the calls have been accepted,
	 * and can proceed to tx and rx data. */

	max_sock_fd=sock_fd[con_max-1].sock;
	printf("%s: Issued %i Calls\n",APP_NAME,con_max);
	
	return TRUE;
}

/*=================================================================
 * TX_RX_DATA 
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

void tx_rx_data(void)
{
	unsigned short Tx_lgth,timeout=0;
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

	/* Initialize the tx packet length */
	Tx_lgth = TX_PACKET_LEN;


	/* Cast the x25 16 byte header to the begining
	 * of the tx packet.  Using the pointer fill 
	 * in appropriate header information such as
	 * QDM bits */
	api_tx_el = (x25api_t *)Tx_data;

	/* Initialize the 16 byte header */
	memset(api_tx_el, 0, sizeof(x25api_hdr_t));

	/* Set the Mbit (more bit) 
	 * currently this option is disabled */
#ifdef MORE_BIT_SET
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

		/* Initialize select() flags */
		FD_ZERO(&readset);
		FD_ZERO(&oobset);
		FD_ZERO(&writeset);
		
		/* For all connected sockets, tell the select
		 * what to wait for.  Tx, Rx and OOB */
		for (i=0;i<con_max;i++){
			if (!sock_fd[i].sock || sock_fd[i].clear)
				continue;
			
			FD_SET(sock_fd[i].sock,&readset);
			FD_SET(sock_fd[i].sock,&oobset);
			FD_SET(sock_fd[i].sock,&writeset);
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
		
	        if((err=select(max_sock_fd + 1,&readset, &writeset, &oobset, NULL))){

			/* One of the sockets returned OK for tx rx or oob 
			 * Thus, for all waiting connections, check each flag */
			
			for (i=0;i<con_max;i++){
		
				if (!sock_fd[i].sock || sock_fd[i].clear){
					continue;
				}

				/* First check for OOB messages */
				if (FD_ISSET(sock_fd[i].sock,&oobset)){
					handle_oob_event(i);
					continue;
				}

				if (FD_ISSET(sock_fd[i].sock,&readset)){
					if (rx_data_act(i) != 0){
						continue;
					}
				}

				if (FD_ISSET(sock_fd[i].sock,&writeset)){
				
					int err=tx_data_act(i,Tx_lgth);

					if (err==0){
						if (Tx_lgth == 1428){
							Tx_lgth=176;
						}else{
							Tx_lgth=1428;
						}
					}
					
					/* In this example I am clearing the connection
					 * after I sent user specified number of packets */
					if (sock_fd[i].tx >= tx_pkt){
						issue_clear_call(i);
						/* The parent process must close the sock
						 * file handlers */
						close(sock_fd[i].sock);
						sock_fd[i].sock=0;
						sock_fd[i].clear=1;
					}	
				}
			}

			for (i=0;i<con_max;i++){
				if (sock_fd[i].sock)
					break;
			}

			/* All channels are down ! */
			if (i>=con_max)
				break;

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

	for (i=0;i<con_max;i++){
		if (!sock_fd[i].sock || sock_fd[i].clear)
			continue;
		
		ioctl(sock_fd[i].sock,SIOC_WANPIPE_CLEAR_CALL,NULL);
		close(sock_fd[i].sock);
		sock_fd[i].sock=0;
		sock_fd[i].clear=1;
	}
} 


int tx_data_act(int i, int Tx_lgth)
{
	int err=0;
	
	/* This socket is ready to tx */
	
	/* The tx packet length contains the 16 byte
	 * header. The tx packet was created above. */
	err = send(sock_fd[i].sock, Tx_data, 
		   Tx_lgth + sizeof(x25api_hdr_t), 0);

	/* err contains number of bytes transmited */		
	if (err>0){
		printf("%s: SockId=%i : TX : Cnt=%i\n",
				APP_NAME,
				sock_fd[i].sock,
				++sock_fd[i].tx);
		err=0;
	}else if (errno != EBUSY){
		perror("Send");
		err=1;
	}else{
		err=-1;
	}

	
	
	/* If err<=0 it means that the send failed and that
	 * driver is busy.  Thus, the packet should be
	 * requeued for re-transmission on the next
	 * try !!!! 
	 */

	return err;
}


int rx_data_act(int i)
{

	int err;
	
	/* This socket has received a packet. */
					
	err = recv(sock_fd[i].sock, Rx_data, sizeof(Rx_data), 0);
	
	/* err contains number of bytes transmited */		
	if(err < 0 ) {
		
		/* This should never happen. However, check the sock state:
		 * if we are still connected, proceed with normal operation.
		 */
		if ((err=ioctl(sock_fd[i].sock,SIOC_WANPIPE_SOCK_STATE,0)) == 0){
			return 1;
		}
		
		printf("%s: Sockid=%i : Failed to rcv packet %i\n",
				APP_NAME,
				sock_fd[i].sock,
				err);
		perror("Recv:");
		close(sock_fd[i].sock);
		sock_fd[i].sock=0;
		return 1;
		
	}else{

		/* Packet received OK !
		 *
		 * Every received packet comes with the 16
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

		printf("%s: SockId=%i : RX : size=%i, qdm=0x%02X, cnt=%i\n", 
			  APP_NAME,
			  sock_fd[i].sock, 
			  err,
			  api_data->qdm, 
			  ++sock_fd[i].rx);
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

		if ((err=ioctl(sock_fd[i].sock,SIOC_WANPIPE_SOCK_STATE,0)) == 0){
			return 1;
		}

		printf("%s: SockId=%i : OOB Event : State = %s\n",
			APP_NAME,sock_fd[i].sock, 
			err == 1 ? "Disconnected" : "(Dis/Con)necting");

		memset(&api_cmd,0,sizeof(api_cmd));
		if ((err=ioctl(sock_fd[i].sock,SIOC_WANPIPE_GET_CALL_DATA,&api_cmd)) == 0){
			if (decode_oob_event(&api_cmd, i) == 0){
				close(sock_fd[i].sock);
				sock_fd[i].sock=0;
			}
		}else{
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
		}
		
		/* Do what ever you have to do to handle
		 * this condiditon */
	}else{

		/* OOB packet received OK ! */
		
		api_tx_el = (x25api_t *)Rx_data;

		printf("%s: SockId=%i : OOB : Packet type 0x%02X, Cause 0x%02X,"
		    " Diagn 0x%02X, Result 0x%02X, Len %i, LCN %i\n",
			APP_NAME,
			sock_fd[i].sock,
			api_tx_el->hdr.pktType, 
			api_tx_el->hdr.cause,
			api_tx_el->hdr.diagn, 
			api_tx_el->hdr.result,
			api_tx_el->hdr.length,
			api_tx_el->hdr.lcn);


		if (decode_oob_event(api_tx_el, i) == 0){
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
		}
	}
	return 0;
}


int decode_oob_event(x25api_t* api_hdr, int sk_index)
{

	switch (api_hdr->hdr.pktType){

		case ASE_RESET_RQST:
			printf("%s: SockId=%i : OOB : Rx Reset Call : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					APP_NAME,
					sock_fd[sk_index].sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);

			/* NOTE: we don't have to close the socket,
			 * since the reset doesn't clear the call
			 * however, it means that there is something really
			 * wrong and that data has been lost */
			return 1;
		
		
		case ASE_CLEAR_RQST:
			printf("%s: SockId=%i : OOB : Rx Clear Call : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					APP_NAME,
					sock_fd[sk_index].sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			break;		
			
		case ASE_RESTART_RQST:
			printf("%s: SockId=%i : OOB : Rx Restart Req : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					APP_NAME,
					sock_fd[sk_index].sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			
			break;
		case ASE_INTERRUPT:
			printf("%s: SockId=%i : OOB : Rx Interrupt Req : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					APP_NAME,
					sock_fd[sk_index].sock,
					api_hdr->hdr.lcn,
					api_hdr->hdr.diagn,
					api_hdr->hdr.cause);
			
			break;


		default:
			printf("%s: SockId=%i : OOB : Rx Type=0x%02X : Lcn=%i : diag=0x%02X : cause=0x%02X\n",
					APP_NAME,
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

int issue_clear_call(int i)
{
	volatile int err=0;

	//FIXME: Fork is expensive use Pthread
	if (fork() == 0){
		
		printf("\n%s: Issuing Clear Call: SockId=%i, Tx=%i, Pid=%i\n\n",
			APP_NAME, sock_fd[i].sock, sock_fd[i].tx, getpid());

		while(ioctl(sock_fd[i].sock,SIOC_WANPIPE_CHECK_TX,NULL)){
			//printf("%s: Wait Count %i\n",prognamed,cnt);
		}

try_clear_again:
		printf("Clearing...\n");
		memset(&api_cmd,0,sizeof(api_cmd));
		api_cmd.hdr.cause=0x01;
		api_cmd.hdr.diagn=0x01;
		
		/* Fast select implementation */
		sprintf(api_cmd.data,"-u1111");
		api_cmd.hdr.length = strlen(api_cmd.data);
		
		err=ioctl(sock_fd[i].sock,SIOC_WANPIPE_CLEAR_CALL,&api_cmd);
		if (err && err == EBUSY){
			perror("Ioctl Clear Call ");
			goto try_clear_again;
		}

		printf("%s: SockId=%i: Call Cleared !\n",
				APP_NAME,
				sock_fd[i].sock);

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

	for (i=0;i<con_max;i++){
		if (!sock_fd[i].sock || sock_fd[i].clear)
			continue;
		
		ioctl(sock_fd[i].sock,SIOC_WANPIPE_CLEAR_CALL,NULL);
		close(sock_fd[i].sock);
		sock_fd[i].sock=0;
		sock_fd[i].clear=1;
	}
	
	exit (0);
}


