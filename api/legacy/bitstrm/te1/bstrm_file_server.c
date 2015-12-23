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

#define RX_FILE_GEN	
#define TX_FILE_SEND	
#define TX_FILE_NAME   "tx_ch_file.b"
#define FILE_DIR	"test_file"

#define CH_1_DATA		0x20
#define DIFF_BETWEEN_CH 	3

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

	unsigned char rx_prev_data[MAX_NUM_OF_TIMESLOTS];
} timeslot_t;


timeslot_t tslot_array[MAX_NUM_OF_TIMESLOTS];
unsigned char rx_test_data[MAX_NUM_OF_TIMESLOTS];


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
	unsigned char Rx_data_buf[1600+HEADER_SZ];
	unsigned char *Rx_data;
	int error_bit=0;
	int ok=0,brd=0,txisr=0,unknown=0;
	struct timeval tv;
	int ferr;
	int frame_count=-1;
	int channel=0;
	
	tv.tv_usec = 0;
	tv.tv_sec = 10;
	
        Rx_count = 0;

	slot->rx_file=NULL;

#ifdef RX_FILE_GEN		
	sprintf(slot->rx_file_name, "%s/rx_%s.b",FILE_DIR,slot->if_name);
	unlink(slot->rx_file_name);
	slot->rx_file = fopen(slot->rx_file_name,"wb");
	if (!slot->rx_file){
		printf("%s: Failed to open RX File: %s\n",
				slot->if_name,
				slot->rx_file_name);
		return;
	}
#endif

	printf("%s: Rx Slot %i looking for 0x%x RxSize=%i\n",
			slot->if_name,slot->sock,slot->data,sizeof(Rx_data));

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

				err = recv(slot->sock, Rx_data_buf, sizeof(Rx_data_buf), 0);
				
				/* err indicates bytes received */
				if(err > HEADER_SZ) {

					err-=HEADER_SZ;
					Rx_data=&Rx_data_buf[HEADER_SZ];
					
					/* Rx packet recevied OK
					 * Each rx packet will contain 16 bytes of
					 * rx header, that must be removed.  The
					 * first byte of the 16 byte header will
					 * indicate an error condition.
					 */

	
					for (i=0;i<err;i++){
						//if (Rx_data[i] >= 0x20 &&   Rx_data[i] <= 0x67){
						if (Rx_data[i] >= 0x20){
							if (!stream_sync){
								printf("%s: Rx: Sync Started Data=0x%x\n",
										slot->if_name,Rx_data[i]);
								stream_sync=1;
								error_bit=0;
								frame_count=1;
								ok=0;
								slot->rx_bytes=0;
								channel=0;

#ifdef RX_FILE_GEN
								ferr=fwrite((void *)&Rx_data[i], 
									sizeof(char), (err-i), slot->rx_file);
								if (ferr != (err-i)){
									printf("%s: Failed to write 1st in Rx file %s len=%i\n",
											slot->if_name,slot->rx_file_name,
											(err-i));
									return;
								}
#endif
							}

							if (stream_sync){
								if (Rx_data[i] != rx_test_data[channel]){

									if ((Rx_data[i] > rx_test_data[channel]) && 
									    (Rx_data[i] <= rx_test_data[channel]+2)){
										//OK 
									}else{
										printf("Error: Ch %i, RxData=%x\n",
										channel+1, Rx_data[i]);
										return;
									}
								}
								channel=(++channel%MAX_NUM_OF_TIMESLOTS);
							}

							ok++;
							slot->rx_bytes++;
							
						}else if (Rx_data[i] == 0x02){
							error_bit |= BOARD_UNDERRUN;
							brd++;
						}else if (Rx_data[i] == 0x04){
							txisr++;
							error_bit |= TX_ISR_UNDERRUN;
						}else{
							unknown++;
							error_bit |= UNKNOWN_ERROR;
						}
#ifdef RX_FILE_GEN
						if (stream_sync && error_bit){

							if (frame_count==1){
								printf("\n%s: Rx: Out of Sync Received on first frame 0x%x!\n",
									slot->if_name,Rx_data[i]);
								return;
							}
						
							printf("\n%s: Rx: Out of Sync Received :%s  Offset=%i 0x%x!\n\n",
								slot->if_name,
								((error_bit&TX_ISR_UNDERRUN) ? "TX UNDR" :
								 (error_bit&BOARD_UNDERRUN) ? "BRD UNDR" :
								 "UNK UNDR"),
								i,Rx_data[i]);

							
							if (i > 0){
								ferr=fwrite((void *)&Rx_data[0], 
										sizeof(char), i, slot->rx_file);
								if (ferr != i){
									printf("%s: 
									  Failed to write in Rx file %s len=%i\n",
									  slot->if_name,
									  slot->rx_file_name,
									  i);
									return;
								}	
							}
							
							return;
						}
#endif
					}

					if (stream_sync){
						if (++frame_count == 2){
							continue;
						}
					}
						
					if (stream_sync && error_bit){
						printf("\n%s: Out of Sync Received :%s !\n\n",
								slot->if_name,
								((error_bit&TX_ISR_UNDERRUN) ? "TX UNDR" :
								 (error_bit&BOARD_UNDERRUN) ? "BRD UNDR" :
								 "UNK UNDR"));
						return;
					}
#ifdef RX_FILE_GEN
					if (stream_sync && !error_bit){
						ferr=fwrite((void *)&Rx_data[0], 
							sizeof(char), (err), slot->rx_file);
						if (ferr != err){
							printf("%s: Failed to write 1st in Rx file %s len=%i\n",
									slot->if_name,slot->rx_file_name,
									err);
							return;
						}
					}
#endif
					//putchar('\r');
					//printf("ok=%i brd=%i tx=%i unk=%i len=%i data=%i",
					//		ok,brd,txisr,unknown,err,slot->data);
	
					//fflush(stdout);

				} else {
					printf("\n%s: Error receiving data\n",slot->if_name);
				}

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
	fd_set 	write;
	int err;
	unsigned char Tx_data[MAX_TX_DATA + sizeof(api_tx_hdr_t)];
	int rlen;
	int i=0;

	i=0;
	
	Tx_count = 0;
	Tx_length = MAX_TX_DATA;
	
	/* If running HDLC_STREAMING then the received CRC bytes
         * will be passed to the application as part of the 
         * received data.  The CRC bytes will be appended as the 
	 * last two bytes in the rx packet.
	 */

	memset(&Tx_data[0],0,MAX_TX_DATA + sizeof(api_tx_hdr_t));

#ifdef TX_FILE_SEND		
//	sprintf(slot->tx_file_name, "tx_%s.b",slot->if_name);
	sprintf(slot->tx_file_name, "%s/%s",FILE_DIR,TX_FILE_NAME);

	slot->tx_file = fopen(slot->tx_file_name,"rb");
	if (!slot->tx_file){
		printf("%s: Failed to open TX File: %s\n",
				slot->if_name,
				slot->tx_file_name);
		return;
	}

	rlen=fread((void*)&Tx_data[0],sizeof(char),Tx_length,slot->tx_file);
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

		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if the socket cannot send
		 * or there is nothing to receive.
		 *
		 * By using the last socket file descriptor +1 select will wait
		 * for all active sockets.
		 */ 

  		if(select(slot->sock + 1,NULL, &write, NULL, NULL)){

			/* Check if the socket is ready to tx data */
		   	if (FD_ISSET(slot->sock,&write)){

				err = send(slot->sock,Tx_data, Tx_length, 0);

				if (err > 0){
#ifdef TX_FILE_SEND	
					slot->tx_bytes+=err;

					rlen=fread((void*)&Tx_data[0],
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

					Tx_data[0]=slot->data;
					
					if (slot->tx_bytes >= ((MAX_FRAMES*MAX_TX_DATA)-240) && 
					    slot->tx_bytes<(MAX_FRAMES*MAX_TX_DATA)){
						
						printf("Sending out last frame!\n");
						Tx_data[Tx_length-1]=0x71;
						
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
				
		   	}else{
				printf("Error Tx nothing IFFSET\n");
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
	char global_if_name[20];
	int err,x,i;
	
	if (argc < 4  || argc%2){
		printf("Usage: rec_wan_sock <router name> <interface name> <data> ...\n");
		exit(0);
	}	

	signal(SIGINT,&sig_handle);
	memset(&tslot_array,0,sizeof(tslot_array));

	strncpy(router_name,argv[1],(sizeof(router_name)-1));
	
	for (x=0;x<MAX_NUM_OF_TIMESLOTS;x++){
		rx_test_data[x]=CH_1_DATA+(x*DIFF_BETWEEN_CH);
	}
	
	for (x=0,i=2;i<argc;x++,i+=2){	

		strncpy(tslot_array[x].if_name, argv[i], MAX_IF_NAME);		
		strncpy(global_if_name,argv[i], MAX_IF_NAME);
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
				process_con_rx(&tslot_array[x]);
				
				if (tslot_array[x].rx_file){
					fclose(tslot_array[x].rx_file);
				}

				close(tslot_array[x].sock);
				printf("%s: Rx child exit: Bytes %i\n",
						tslot_array[x].if_name,
						tslot_array[x].rx_bytes);
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

	{
	/* Note: Change the rx file name based on the interface name you are using*/
		
	unsigned char cmd_exec[100];
	sprintf(cmd_exec,"diff --brief test_file/rx_%s.b test_file/%s",
			global_if_name,TX_FILE_NAME);
	printf("Executing: %s\n",cmd_exec);
	err=system(cmd_exec);
	if (err == 0){
		printf("Rx file identical to Tx file: Success!\n");
	}else{
		printf("Rx file corrupted, different than Tx: ERROR!\n");
	}
	}
	return 0;
}
