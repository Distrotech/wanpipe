/*****************************************************************************
* bitstrm_api.c	Bit Streaming API: Sample Module
*
* Author(s):	Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 2000-2003 Sangoma Technologies Inc.
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
#include <wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <signal.h>
#include <linux/if.h>
#include <linux/wanpipe.h>
#include <linux/sdla_bitstrm.h>
#include "lib_api.h"

#define MAX_TX_DATA     5000	/* Size of tx data */  
#define MAX_FRAMES 	5000     /* Number of frames to transmit */  

#define MAX_RX_DATA	5000

unsigned short Rx_lgth;

unsigned char Tx_data[MAX_TX_DATA];

/* Prototypes */
int MakeConnection(int *,int*);
void handle_socket(int sock, int sw_sock,int action);
void sig_end(int sigid);

int handle_oob_msg(int sk, char *Rx_data);
int handle_tx(int sk, char *Tx_data, int len);
int handle_rx(int sk, char *Rx_data, int* Rx_lgth, int action);

/***************************************************
* MakeConnection
*
*   o Create a Socket
*   o Bind a socket to a wanpipe network interface
*       (Interface name is supplied by the user)
*/         

int MakeConnection(int *sock, int *sw_sock) 
{
	struct wan_sockaddr_ll 	sa;

	memset(&sa,0,sizeof(struct wan_sockaddr_ll));
	errno = 0;
   	*sock = socket(AF_WANPIPE, SOCK_RAW, 0);
   	if( *sock < 0 ) {
      		perror("Socket");
      		return( WAN_FALSE );
   	} /* if */
 
	*sw_sock = socket(AF_WANPIPE, SOCK_RAW, 0);
   	if( *sw_sock < 0 ) {
      		perror("Socket");
      		return( WAN_FALSE );
   	} /* if */

	
	printf("\nSwitching Interfaces %s <---> %s\n",if_name,sw_if_name);
	
	strcpy( sa.sll_device, if_name);
	strcpy( sa.sll_card, card_name);
	sa.sll_protocol = htons(PVC_PROT);
	sa.sll_family=AF_WANPIPE;
	
        if(bind(*sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) < 0){
                perror("bind");
		printf("Failed to bind a socket to %s interface\n",if_name);
                exit(0);
        }

	strcpy(sa.sll_device, sw_if_name);
	strcpy(sa.sll_card, sw_card_name);
	sa.sll_protocol = htons(PVC_PROT);
	sa.sll_family=AF_WANPIPE;
	
        if(bind(*sw_sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) < 0){
                perror("bind");
		printf("Failed to bind a socket to %s interface\n",sw_if_name);
                exit(0);
        }

	printf("Socket bound to %s and %s\n\n",if_name,sw_if_name);
	
	return (WAN_TRUE);

}

/***************************************************
* HANDLE SOCKET 
*
*   o Read a socket 
*   o Cast data received to api_rx_element_t data type 
*   o The received packet contains 16 bytes header 
*
*	------------------------------------------
*      |  16 bytes      |        X bytes        ...
*	------------------------------------------
* 	   Header              Data
*
*   o Data structures:
*
*	typedef struct {
*        	unsigned char   SIO      	PACKED;
*        	unsigned short  time_stamp      PACKED;
*        	unsigned char   reserved[13]    PACKED;
*	} api_rx_hdr_t; 
*
*	typedef struct {
*        	api_rx_hdr_t    api_rx_hdr      PACKED;
*        	unsigned char   data[1]         PACKED;
*	} api_rx_element_t;
*
*
*/

void handle_socket(int sock, int sw_sock, int action) 
{
	fd_set ready,oob;
	int len;
	unsigned char Rx_data[MAX_RX_DATA];

	for(;;) {	
		FD_ZERO(&ready);
		FD_ZERO(&oob);
		
		FD_SET(sock,&oob);
		FD_SET(sock,&ready);
		
		fflush(stdout);
  		if (select(sock + 1,&ready, NULL, &oob, NULL)){

			if (FD_ISSET(sock,&oob)){
				if (handle_oob_msg(sock,Rx_data)){
					break;
				}
			}

			if (FD_ISSET(sock,&ready)){
				if (handle_rx(sock,Rx_data,&len,action)){
					break;
				}
			
				if (handle_tx(sw_sock,Rx_data,len)){
					printf("Warning: Failed to send data \n");
				}
			}
		}
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
	pid_t pid;
	int stat;
	int sock,sw_sock;

	read_enable=1;
	write_enable=1;

	proceed=init_args(argc,argv);
	if (proceed != WAN_TRUE){
		usage(argv[0]);
		return -1;
	}

	signal(SIGINT,&sig_end);
	
	proceed = MakeConnection(&sock,&sw_sock);
	if( proceed == WAN_TRUE){

		if (!fork()){
			handle_socket(sock,sw_sock,0);
			exit(0);
		}
		if (!fork()){
			handle_socket(sw_sock,sock,1);
			exit(0);
		}

		close(sock);
		close(sw_sock);

		while ((pid=waitpid(-1,&stat,WUNTRACED)) > 0){
			printf("Child %d terminated\n",pid);
		}
	}

	return 0;
};


void sig_end(int sigid)
{

	printf("Got Signal %i\n",sigid);

	exit(1);
}

int handle_oob_msg(int sk, char *Rx_data)
{
	int err = recv(sk, Rx_data, MAX_RX_DATA, MSG_OOB);

	if (err < 0){
		printf("Failed to receive OOB %i\n",err);
		err = ioctl(sk,SIOC_WANPIPE_SOCK_STATE,0);
		printf("Sock state is %s\n",
				(err == 0) ? "CONNECTED" : 
				(err == 1) ? "DISCONNECTED" :
				 "CONNECTING");
		return 1;
	}
		
	printf("GOT OOB EXCEPTION CMD Exiting\n");

	return 1;
}

int handle_rx(int sk, char *Rx_data, int* Rx_lgth, int action)
{
	api_rx_element_t* api_rx_el;
	int i,len;
	int err = recv(sk, Rx_data, MAX_RX_DATA, 0);

	if (!read_enable){
		return 0;
	}
	
	/* err indicates bytes received */
	if (err <= 0){
		printf("\nError receiving data\n");
		return 1;	
	}

	*Rx_lgth = err;
	api_rx_el = (api_rx_element_t*)&Rx_data[0];

	/* Check the packet length */
	len = err - sizeof(api_rx_hdr_t);
	if(len<=0) {
		printf("\nShort frame received (%d)\n",
			*Rx_lgth);
		return 1;
	}

	for(i=0; i<len; i++) {
		if (action){
			if (((i%24)+1) == 1){
				api_rx_el->data[i]+=1;	
			}else if (((i%24)+1) == 2){
				api_rx_el->data[i]+=2;	
			}
		}else{
			if (((i%24)+1) == 3){
				api_rx_el->data[i]+=3;	
			}else if (((i%24)+1) == 4){
				api_rx_el->data[i]+=4;	
			}
		}
	}

	return 0;
}

int handle_tx(int sock, char *Tx_data, int len)
{
	int err = send(sock,Tx_data, len, 0);

	if (err <= 0){
		if (errno == EBUSY){
			printf("Sock busy try again!\n");
			return 1;
			/* Socket busy try sending again !*/
		}else{
			/* Check socket state */
			err = ioctl(sock,SIOC_WANPIPE_SOCK_STATE,0);
			printf("Sock state is %s\n",
				(err == 0) ? "CONNECTED" : 
				(err == 1) ? "DISCONNECTED" :
				 "CONNECTING");

			printf("Failed to send %i \n",errno);
			perror("Send: ");
			return 1;
		}
	}

	return 0;
}
