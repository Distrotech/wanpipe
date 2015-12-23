/*****************************************************************************
* xdlc_mps.c	XDLC API: Multi Pri/Sec Client Application 
*
* Author(s):	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2004 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*
* Description:
*
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
#include <linux/wanpipe_xdlc_iface.h>
#include <linux/if_ether.h>
#include "lib_api.h"


/*===========================================================
 * Defines 
 *==========================================================*/

#define FALSE	0
#define TRUE	1


#define TX_PACKET_LEN	1000	
#define MAX_RX_PKT_LEN  1000

#define MAX_XDLC_DATA_SIZE 	100

#define MAX_SOCK_NUM 255

#define TIMEOUT 10

#define APP_NAME "xdlc_client"


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
	int address;
	wan_xdlc_conf_t cfg;
}sock_api_t;

sock_api_t sock_fd[MAX_SOCK_NUM];

int max_sock_fd=0;

/* XDLC tx/rx header structure.  This structure will
 * be used to send commands down to the driver, as
 * well as read/insert header information from/into
 * rx and tx data respectively.
 * 
 * Defined globaly so it would only get allocated once.  
 */  

unsigned char prognamed[100];

unsigned char *xdlc_dev_sec_map[]=
{"wp2sa1", "wp2sa2", NULL};

unsigned char *xdlc_dev_pri_map[]=
{"wp1pa1", "wp1pa2", NULL};


int xdlc_devices=0;

/*============================================================
  Global Parameters  
 *===========================================================*/



/*============================================================
  Function Prototypes 
 *===========================================================*/
int MakeConnection();
void tx_rx_data( void);
void setup_signal_handlers (void);
void quit (int);
void sig_chld (int sigio);
void sig_hnd (int sigio);
int rx_data_act(int i);
int tx_data_act(int i,int Tx_lgth);
int handle_oob_event(int sk_index);
void close_connection(int i);
int xdlc_get_numof_devices(void);


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

	xdlc_devices=xdlc_get_numof_devices();

	sprintf(prognamed,"%s[%i]",APP_NAME,getpid());

	i_cnt=1;
	card_cnt=1;
	if (init_args(argc,argv) == 0){
		usage(argv[0]);
		return -EINVAL;
	}

	signal(SIGCHLD,&sig_chld);
	signal(SIGINT,&sig_hnd);
	signal(SIGTERM,&sig_hnd);

	memset(sock_fd,0,sizeof(sock_fd));

	printf("\n%s: Attempting to establish %i connections, and tx %i (%i bytes) packets \n",
			prognamed, xdlc_devices, tx_cnt, tx_size);

	proceed = MakeConnection();
	if (proceed == TRUE){

		tx_rx_data();

		/* Wait for the clear call children */
		while ((pid=waitpid(-1,&stat,WUNTRACED)) > 0){
			//printf("Child %d terminated\n",pid);
		}
	}

	for (i=0;i<xdlc_devices;i++){
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

int MakeConnection (void) 
{
	int err = 0;
	struct wan_sockaddr_ll 	sa;
	int i;
	int connected_devices=0;


	for (i=0; i<xdlc_devices; i++){
	
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
		sa.sll_protocol = htons(PVC_PROT);
		if (primary_enable){
			strcpy(sa.sll_device, xdlc_dev_pri_map[i]);
		}else{
			strcpy(sa.sll_device, xdlc_dev_sec_map[i]);
		}
		strcpy(sa.sll_card, "wanpipe1");
	

		/* Bind a sock using the above address structure */
		if(bind(sock_fd[i].sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) == -1){
			perror("Bind: ");
			printf("Failed to bind socket to %s interface\n",sa.sll_device);
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			continue;
		}

		err = ioctl(sock_fd[i].sock,
			    WAN_DEVPRIV_SIOC(SIOCS_XDLC_GET_CONF), 
		            &sock_fd[i].cfg);
		if (err){
			perror("Get Address:");
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			continue;
		}
		
		err = ioctl(sock_fd[i].sock,
			    WAN_DEVPRIV_SIOC(SIOCS_XDLC_START),
			    NULL);
		if (err){
			perror("XDLC Start: ");
			close(sock_fd[i].sock);
			sock_fd[i].sock=0;
			continue;

		}else{
			printf("Address 0x%X started successfully!\n",
					sock_fd[i].cfg.address);
		}
	
		sock_fd[i].mtu=128;
		sock_fd[i].mru=128;
		
		connected_devices++;

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

	printf("%s: Connected to %i %s devices! (Requested %i)\n\n\n",
			prognamed,connected_devices,
			primary_enable?"PRI":"SEC",
			xdlc_devices);
	
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
 *	 typedef struct {
 *		union {
 *			struct {
 *				unsigned char	state;
 *				unsigned char	address;
 *				unsigned short  exception;
 *			}xdlc;
 *			unsigned char	reserved[16];
 *		}u;
 *
 *	#define wan_hdr_xdlc_state	u.xdlc.state
 *	#define wan_hdr_xdlc_address	u.xdlc.address
 *	#define wan_hdr_xdlc_exception	u.xdlc.exception
 *	} wan_api_rx_hdr_t;
 *
 *
 *	typedef struct {
 * 		wan_api_rx_hdr_t	api_rx_hdr;
 *		unsigned char  		data[0];
 *	#define wan_rxapi_xdlc_state		api_rx_hdr.wan_hdr_xdlc_state
 *	#define wan_rxapi_xdlc_address		api_rx_hdr.wan_hdr_xdlc_address
 *	#define wan_rxapi_xdlc_exception	api_rx_hdr.wan_hdr_xdlc_exception
 *	} wan_api_rx_element_t;
 *
 *
 * TX DATA:
 * --------
 * 	Each tx data packet must contain the above 16 byte header!
 *
 * 	There are no relevant bytes at this time.
 * 	Set the tx header to ZERO.
 *
 * RX DATA:
 * --------
 * 	Each rx data will contain the above 16 byte header!
 * 	
 *	There are no relevant bytes at this time.
 * 	Set the tx header to ZERO.
 * 	
 * OOB DATA:
 * ---------
 * 	The OOB (out of band) data is used by the driver to 
 * 	indicate Xdlc events for the active channel: 
 *
 * 	Each OOB packet contains the above 16 byte header!
 *
 *	Relevant bytes in the 16 byte oob header, are:
 *
 *      
 *	state    : State of the xdlc device
 *
 * 	address  : Address of the xdlc device
 *
 * 	exception: Exception on the xdlc device
 *
 * 	The socket will be available after ALL OOB messages.
 * 	It is up the the user to identify when the 
 * 	Xdlc connection has been terminated.
 */

void tx_rx_data(void)
{
	int err=0, i; 
	struct timeval tv;
	wan_api_tx_element_t* api_tx_el;
	fd_set	writeset,readset,oobset;

	/* Setup a timeout value for the select statement, which is
	 * going to block until the socket(s) are/is ready to tx/rx
	 * or oob.  In this case we will timeout afte 10 seconds of
	 * inactivity */
	tv.tv_usec = 0; 
	tv.tv_sec = 10;


	/* Cast the x25 16 byte header to the begining
	 * of the tx packet.  Using the pointer fill 
	 * in appropriate header information such as
	 * QDM bits */
	api_tx_el = (wan_api_tx_element_t *)Tx_data;

	/* Initialize the 16 byte header */
	memset(api_tx_el, 0, sizeof(wan_api_rx_hdr_t));

	/* Fill in the tx packet data with arbitrary
	 * information */ 
	for (i=0; i<tx_size; i++){
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
		for (i=0;i<xdlc_devices;i++){

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
		 * for all active xdlc sockets.
		 */ 
	
		err=select(max_sock_fd + 1,&readset, &writeset, &oobset, &tv);
	
		if (err < 0){

			/* Signal Interruption */
			continue;

		}else if (err == 0){

			/* timeout */
			continue;
		
		}else if (err  > 0) {

			/* One of the sockets returned OK for tx rx or oob 
			 * Thus, for all waiting connections, check each flag */
			
			for (i=0;i<xdlc_devices;i++){
		
				if (!sock_fd[i].sock)
					continue;

				/* First check for OOB messages */
				if (FD_ISSET(sock_fd[i].sock,&oobset)){
					if (handle_oob_event(i)){
						close_connection(i);
						break;
					}
					continue;
				}

				if (FD_ISSET(sock_fd[i].sock,&readset)){
					if (rx_data_act(i)){
						close_connection(i);
						break;
					}
				}

				if (FD_ISSET(sock_fd[i].sock,&writeset)){
					
					tx_data_act(i,tx_size);

					/* In this example I am clearing the connection
					 * after I sent user specified number of packets */
					if (tx_delay)
						sleep(tx_delay);

					if (sock_fd[i].tx >= tx_cnt){
						write_enable=0;
					}	
				}
			}

		}//if select
	}//for

	for (i=0;i<xdlc_devices;i++){

		if (!sock_fd[i].sock)
			continue;

		close_connection(i);
	}
} 


int tx_data_act(int i, int Tx_lgth)
{
	int err;
	/* This socket is ready to tx */
#if 0
	if (Tx_lgth > sock_fd[i].mtu){
		printf("%s: Tx warning: Tx length %i > MTU %i, using MTU!\n",
				prognamed,Tx_lgth,sock_fd[i].mtu);
		Tx_lgth=sock_fd[i].mtu;
	}
#endif	

	/* The tx packet length contains the 16 byte
	 * header. The tx packet was created above. */
	err = send(sock_fd[i].sock, Tx_data, 
		   Tx_lgth + sizeof(wan_api_tx_hdr_t), 0);

	/* err contains number of bytes transmited */		
	if (err>0){
		++sock_fd[i].tx;
		if (verbose){
			printf("%s: SockId=%i : TX : size=%i, cnt=%i\n",
					prognamed,
					sock_fd[i].sock,
					Tx_lgth,
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
		if ((err=ioctl(sock_fd[i].sock,SIOC_WANPIPE_SOCK_STATE,0)) == 0){
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
		 * Every received packet comes with the 16
		 * bytes of header which driver adds on. 
		 * Header is structured as x25api_hdr_t. 
		 * (same as above)
		 */
		
		if (!read_enable){
			return 0;
		}
		

		err-=sizeof(wan_api_rx_hdr_t);

		if (err > sock_fd[i].mru){
			/* This should never happen, sanity check */
			printf("%s: Rx warning: Rx len %i > MRU %i\n",
					prognamed,
					err,sock_fd[i].mru);
		}	
		
		++sock_fd[i].rx;
		
		if (verbose){
			printf("%s: SockId=%i : RX : size=%i, cnt=%i\n", 
				  prognamed,
				  sock_fd[i].sock, 
				  err,
				  sock_fd[i].rx);
		}
	}
	return 0;
}

int handle_oob_event(int i)
{
	int err;
	wan_api_rx_element_t* api_rx_el;
	
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
			return 0;
		}

		printf("%s: SockId=%i : OOB Event : State = %s\n",
			prognamed,sock_fd[i].sock, 
			err == 1 ? "Disconnected" : "(Dis/Con)necting");

		return 1;
		
		/* Do what ever you have to do to handle
		 * this condiditon */
	}else{

		/* OOB packet received OK ! */
		
		api_rx_el = (wan_api_rx_element_t *)Rx_data;

		printf("Rx: OOB Excep: Addr=0x%X State=%s Exception=%s\n",
			api_rx_el->wan_rxapi_xdlc_address,
			XDLC_STATE_DECODE(api_rx_el->wan_rxapi_xdlc_state),
			XDLC_EXCEP_DECODE(api_rx_el->wan_rxapi_xdlc_exception));
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

	for (i=0;i<xdlc_devices;i++){
		if (!sock_fd[i].sock)
			continue;

		ioctl(sock_fd[i].sock,WAN_DEVPRIV_SIOC(SIOCS_XDLC_STOP),NULL);

		close(sock_fd[i].sock);
		sock_fd[i].sock=0;
	}
	
	exit (0);
}

void new_max_sock_fd(void)
{
	int i;
	max_sock_fd=0;
	for (i=0;i<xdlc_devices;i++){
		if (sock_fd[i].sock > max_sock_fd){
			max_sock_fd=sock_fd[i].sock;
		}
	}
}


void close_connection(int i)
{

	printf("%s: Connection SockId=%i closed: Tx=%i Rx=%i\n",
			prognamed,
			sock_fd[i].sock,
			sock_fd[i].tx,
			sock_fd[i].rx);

	ioctl(sock_fd[i].sock,WAN_DEVPRIV_SIOC(SIOCS_XDLC_STOP),NULL);
	close(sock_fd[i].sock);

	if (max_sock_fd == sock_fd[i].sock){
		sock_fd[i].sock=0;
		new_max_sock_fd();
		//printf("%s: Finding new max sock %i\n",prognamed,max_sock_fd);
	}
	sock_fd[i].sock=0;
}

int xdlc_get_numof_devices(void)
{
	int i;
	for (i=0;i<MAX_SOCK_NUM;i++){

		if (primary_enable){
			if (xdlc_dev_pri_map[i] == NULL)
				break;
		}else{
			if (xdlc_dev_sec_map[i] == NULL)
				break;
		}
	}

	return i;
}
