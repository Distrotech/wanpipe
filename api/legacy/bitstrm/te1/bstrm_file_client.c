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
#include <signal.h>
#include <wait.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/wanpipe.h>
#include <linux/sdla_chdlc.h>

#define FALSE	0
#define TRUE	1

#define MAX_NUM_OF_TIMESLOTS  31

#define LGTH_CRC_BYTES	2
#define MAX_TX_DATA     MAX_NUM_OF_TIMESLOTS*10	/* Size of tx data */  
#define MAX_FRAMES 	10000     /* Number of frames to transmit */  

#define WRITE 1
#define MAX_IF_NAME 20

#define ALL_OK  	0
#define	BOARD_UNDERRUN 	1
#define TX_ISR_UNDERRUN 2
#define UNKNOWN_ERROR 	4
#define RX_FILE_NAME   "rx_file.txt"

#undef RX_FILE_GEN		
#define TX_FILE_SEND	
#define TX_FILE_NAME   "tx_ch_file.b"
#define FILE_DIR	"test_file"

#define HEADER_SZ	sizeof(api_tx_hdr_t)
typedef struct {
	int sock;
	int rx_cnt;
	int tx_cnt;
	int data;
	char if_name[MAX_IF_NAME+1];
	
	FILE *rx_file;
	char rx_file_name[MAX_IF_NAME+20+1];
	int rx_bytes;
	
	FILE *tx_file;
	char tx_file_name[MAX_IF_NAME+20+1];
	int tx_bytes;
} timeslot_t;


timeslot_t tslot_array[MAX_NUM_OF_TIMESLOTS];



/*===================================================
 * MakeConnection
 *
 *   o Create a Socket
 *   o Bind a socket to a wanpipe network interface
 *       (Interface name is supplied by the user)
 *==================================================*/         

int MakeConnection(timeslot_t *slot, char *router_name ) 
{
	struct wan_sockaddr_ll 	sa;

	memset(&sa,0,sizeof(struct wan_sockaddr_ll));
	errno = 0;
   	slot->sock = socket(AF_WANPIPE, SOCK_RAW, 0);
   	if( slot->sock < 0 ) {
      		perror("Socket");
      		return( FALSE );
   	} /* if */
  
	printf("\nConnecting to router %s, interface %s\n", router_name, slot->if_name);
	
	strcpy( sa.sll_device, slot->if_name);
	strcpy( sa.sll_card, router_name);
	sa.sll_protocol = htons(PVC_PROT);
	sa.sll_family=AF_WANPIPE;

        if(bind(slot->sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) < 0){
                perror("bind");
		printf("Failed to bind a socket to %s interface\n",slot->if_name);
                exit(0);
        }
	printf("Socket bound to %s\n\n",slot->if_name);

	return(TRUE);

}


void process_con_rx(timeslot_t *slot) 
{
	unsigned int Rx_count;
	fd_set 	ready;
	int err,serr,i;
	int stream_sync=0;
	unsigned char Rx_data[1600];
	int error_bit=0;
	int ok=0,brd=0,txisr=0,unknown=0;
	struct timeval tv;
	int ferr;
	int frame_count=-1;
	
	tv.tv_usec = 0;
	tv.tv_sec = 10;
	
        Rx_count = 0;

	slot->rx_file=NULL;

	printf("%s: Rx Slot %i looking for 0x%x\n",slot->if_name,slot->sock,slot->data);

	for(;;) {	
		FD_ZERO(&ready);
		FD_SET(slot->sock,&ready);

		tv.tv_usec = 0;
		tv.tv_sec = 10;

		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if the socket cannot send
		 * or there is nothing to receive.
		 *
		 * By using the last socket file descriptor +1 select will wait
		 * for all active sockets.
		 */ 

		error_bit=0;
  		if((serr=select(slot->sock + 1,&ready, NULL, NULL, &tv))){

			/* Check for rx packets */
                   	if (FD_ISSET(slot->sock,&ready)){
				err = recv(slot->sock, Rx_data, sizeof(Rx_data), 0);
			 } 
			
		} else {
			printf("\n%s: Error selecting rx socket rc=0x%x errno=0x%x\n",
					slot->if_name,serr,errno);
			perror("Select: ");
			break;
		}
	}
} 

 
void process_con_tx(timeslot_t *slot) 
{
	unsigned int Tx_count,Tx_length;
	fd_set 	read,write;
	int err;
	unsigned char Tx_data[MAX_TX_DATA + sizeof(api_tx_hdr_t)];
	int rlen;
	int i=0;
	unsigned char Rx_data[1600];

	i=0;
	
	Tx_count = 0;
	Tx_length = MAX_TX_DATA;
	
	/* If running HDLC_STREAMING then the received CRC bytes
         * will be passed to the application as part of the 
         * received data.  The CRC bytes will be appended as the 
	 * last two bytes in the rx packet.
	 */

	memset(&Tx_data[0],0,MAX_TX_DATA + HEADER_SZ);

#ifdef TX_FILE_SEND		
	sprintf(slot->tx_file_name, "%s/%s",FILE_DIR,TX_FILE_NAME);

	slot->tx_file = fopen(slot->tx_file_name,"rb");
	if (!slot->tx_file){
		printf("%s: Failed to open TX File: %s\n",
				slot->if_name,
				slot->tx_file_name);
		return;
	}

	rlen=fread((void*)&Tx_data[HEADER_SZ],sizeof(char),Tx_length,slot->tx_file);
	if (!rlen){
		printf("%s: File failed to read %s\n",
				slot->if_name,
				slot->tx_file_name);
		return;	
	}

#else	
	Tx_data[0]=0x91;
	for (i=1;i<Tx_length;i++){
		Tx_data[i] = slot->data;
	}
#endif
	
		
	printf("%s: Tx Starting to write on sock %i\n",slot->if_name,slot->sock);
	
	//pause();
	slot->tx_bytes=0;	
	for(;;) {	
		FD_ZERO(&write);
		FD_SET(slot->sock,&write);
		FD_SET(slot->sock,&read);

		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if the socket cannot send
		 * or there is nothing to receive.
		 *
		 * By using the last socket file descriptor +1 select will wait
		 * for all active sockets.
		 */ 

  		if(select(slot->sock + 1,&read, &write, NULL, NULL)){

			if (FD_ISSET(slot->sock,&read)){
				err = recv(slot->sock, Rx_data, sizeof(Rx_data), 0);	
			}
			
			/* Check if the socket is ready to tx data */
		   	if (FD_ISSET(slot->sock,&write)){

				err = send(slot->sock,Tx_data, Tx_length+HEADER_SZ, 0);

				if (err > 0){
#ifdef TX_FILE_SEND	
					err-=HEADER_SZ;
					slot->tx_bytes+=err;

					rlen=fread((void*)&Tx_data[HEADER_SZ],
						    sizeof(char),
						    Tx_length,slot->tx_file);
					if (!rlen){
						printf("%s: Tx of file %s done!\n",
								slot->if_name,
								slot->tx_file_name);
						return;	
						
					}else if (rlen < Tx_length){
						Tx_length=rlen;
						printf("%s: Sending last part of the file\n",slot->if_name);
					}
#else
					slot->tx_bytes+=err;

					Tx_data[HEADER_SZ]=slot->data;
					
					if (slot->tx_bytes >= ((MAX_FRAMES*MAX_TX_DATA)-240) && 
					    slot->tx_bytes<(MAX_FRAMES*MAX_TX_DATA)){
						
						printf("Sending out last frame!\n");
						Tx_data[HEADER_SZ+Tx_length-1]=0x71;
						
					}else if (slot->tx_bytes >= (MAX_FRAMES*MAX_TX_DATA)){
						printf("Sending out last frame sent ok!\n");
						return;
					}
#endif
					
					/* Packet sent ok */
					//printf("\t\t%s:Packet sent: Len=%i Data=0x%x : %i\n",
					//		slot->if_name,err,slot->data,++Tx_count);
					//putchar('T');
				}else{
					if (errno != EBUSY){
						printf("Errno = %i EBUSY=%i\n",errno,-EBUSY);
						perror("Send Failed: ");
						break;
					}

					/* The driver is busy,  we must requeue this
					 * tx packet, and try resending it again 
					 * later. */
				}
				
		   	}
		} else {
                	printf("\nError selecting socket\n");
			break;
		}
	}
} 

void sig_handle(int sigio)
{
	printf("End\n");

	kill(0,SIGTERM);
	
	return;
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
	char router_name[20];	
	int err,x,i;
	
	if (argc < 4  || argc%2){
		printf("Usage: rec_wan_sock <router name> <interface name> <data> ...\n");
		exit(0);
	}	

	signal(SIGINT,&sig_handle);
	memset(&tslot_array,0,sizeof(tslot_array));

	strncpy(router_name,argv[1],(sizeof(router_name)-1));
		
	for (x=0,i=2;i<argc;x++,i+=2){	

		strncpy(tslot_array[x].if_name, argv[i], MAX_IF_NAME);		
		tslot_array[x].data = atoi(argv[i+1]);

#if 1		
		proceed = MakeConnection(&tslot_array[x],router_name);
		if( proceed == TRUE ){

			printf("Creating %s with tx data 0x%x : Sock=%i : x=%i\n",
				tslot_array[x].if_name,
				tslot_array[x].data,
				tslot_array[x].sock,
				x);

			if (!fork()){
				process_con_tx(&tslot_array[x]);
				
				if (tslot_array[x].tx_file){
					fclose(tslot_array[x].tx_file);
				}
				
				close(tslot_array[x].sock);
				printf("%s: Tx child exit: Bytes %i\n",
						tslot_array[x].if_name,
						tslot_array[x].tx_bytes);
				exit(0);
			}
		}
#endif
	}

	for (x=0;x<MAX_NUM_OF_TIMESLOTS;x++){
		if (tslot_array[x].sock){
			close(tslot_array[x].sock);
			tslot_array[x].sock=0;
		}
	}
	
	/* Wait for the clear call children */
	//pause();

	{
	pid_t pid;
	int stat;
	while ((pid=waitpid(-1,&stat,0)) > 0){
		//printf("Child %d terminated\n",pid);
	}
	}
#if 0	
	err=system("diff --brief test_file/rx_wp1_bstrm.b test_file/tx_file.b");
	if (err == 0){
		printf("Rx file identical to Tx file: Success!\n");
	}else{
		printf("Rx file corrupted, different than Tx: ERROR!\n");
	}
#endif	
	return 0;
};
