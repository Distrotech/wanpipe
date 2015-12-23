/*****************************************************************************
* aft_tdm_hdlc_test.c: Multiple HDLC Test Receive Module
*
* Author(s):    Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2006 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*
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
#include <pthread.h>
#include <linux/wanpipe.h>
#include <linux/wanpipe_tdm_api.h>
#include <libsangoma.h>
#include <wanpipe_hdlc.h>

#define FALSE	0
#define TRUE	1

/* Enable/Disable tx of random frames */
#define RAND_FRAME 0

#define MAX_NUM_OF_TIMESLOTS  32*32

#define LGTH_CRC_BYTES	2
#define MAX_TX_DATA     15000 //MAX_NUM_OF_TIMESLOTS*10	/* Size of tx data */
#define MAX_TX_FRAMES 	1000000     /* Number of frames to transmit */  

#define WRITE 1
#define MAX_IF_NAME 20
typedef struct {

	int sock;
	int rx_cnt;
	int tx_cnt;
	int data;
	int last_error;
	int frames;
	char if_name[MAX_IF_NAME+1];
	wanpipe_hdlc_engine_t *hdlc_eng;
	
} timeslot_t;


timeslot_t tslot_array[MAX_NUM_OF_TIMESLOTS];

int tx_change_data=0, tx_change_data_cnt=0;
int end=0;


void print_packet(unsigned char *buf, int len)
{
	int x;
	printf("{  | ");
	for (x=0;x<len;x++){
		if (x && x%24 == 0){
			printf("\n  ");
		}
		if (x && x%8 == 0)
			printf(" | ");
		printf("%02x ",buf[x]);
	}
	printf("}\n");
}

void sig_end(int sigid)
{
	//printf("%d: Got Signal %i\n",getpid(),sigid);
	end=1;
}        


static unsigned long next = 1;

/* RAND_MAX assumed to be 32767 */
int myrand(int max_rand) {
next = next * 1103515245 + 12345;
return((unsigned)(next/65536) % max_rand);
}

void mysrand(unsigned seed) {
next = seed;
}


/*===================================================
 * MakeConnection
 *
 *   o Create a Socket
 *   o Bind a socket to a wanpipe network interface
 *       (Interface name is supplied by the user)
 *==================================================*/         

int MakeConnection(timeslot_t *slot, char *router_name ) 
{

	int span,chan;
	int err;
	int period=40;
	wanpipe_api_t tdm_api;
	sangoma_span_chan_fromif(slot->if_name,&span,&chan);
	
	memset(&tdm_api,0,sizeof(tdm_api));

	if (span > 0 && chan > 0) {
		wanpipe_tdm_api_t tdm_api;
             	slot->sock = sangoma_open_tdmapi_span_chan(span,chan);
	     	if( slot->sock < 0 ) {
      			perror("Open Span Chan: ");
      			return( FALSE );
		}

        sangoma_tdm_set_codec(slot->sock,&tdm_api,WP_NONE);
	

#if 0
		err=sangoma_tdm_set_usr_period(slot->sock,&tdm_api,period);
		if (err) {
     		printf("Error: Failed to set period\n");
	    }
#endif

		printf("Socket bound to Span=%i Chan=%i\n\n",span,chan);
		return (TRUE);
	}


	
	return(FALSE);
}

int  api_tdm_fe_alarms_callback(int fd, unsigned char alarm)
{
	int fd_found=0;
	int i;
	for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++){
		if (tslot_array[i].sock == fd) {
			fd_found=1;
			break;		
		}	
	}          

	if (fd_found) {
        	printf ("FE ALARMS Call back found device %i Alarm=%i \n", 
				fd,alarm);  	

	} else {
		printf ("FE ALARMS Call back no device Alarm=%i\n",alarm);
	}

	return 0;
}



#define ALL_OK  	0
#define	BOARD_UNDERRUN 	1
#define TX_ISR_UNDERRUN 2
#define UNKNOWN_ERROR 	4

void process_con_rx(void) 
{
	unsigned int Rx_count;
	fd_set 	oob,ready;
	int err,serr,i,slots=0;
	unsigned char Rx_data[15000];
	int error_bit=0, error_crc=0, error_abort=0, error_frm=0;
	struct timeval tv;
	int frame=0,print_status=0;
	int max_fd=0, packets=0;
	timeslot_t *slot=NULL;
	wanpipe_tdm_api_t tdm_api;

	memset(&tdm_api,0,sizeof(tdm_api));

	tdm_api.wp_tdm_event.wp_fe_alarm_event = &api_tdm_fe_alarms_callback;
	
	
	i=0;
	for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++){
	
		if (tslot_array[i].sock < 0) {
			continue;		
		}
		
		tslot_array[i].hdlc_eng = wanpipe_reg_hdlc_engine();
		if (!tslot_array[i].hdlc_eng){
			printf("ERROR: Failed to register HDLC Engine\n");
			return;
		}
		
		if (tslot_array[i].sock > max_fd){
			max_fd=tslot_array[i].sock;
		}
	}
	i=0;
	
	
	tv.tv_usec = 0;
	tv.tv_sec = 10;
	
        Rx_count = 0;

	for(;;) {
	
		FD_ZERO(&ready);
		FD_ZERO(&oob);
		max_fd=0;
		for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++){
	
			if (tslot_array[i].sock < 0) {
				continue;		
			}	
			
			FD_SET(tslot_array[i].sock,&ready);
			FD_SET(tslot_array[i].sock,&oob);
			if (tslot_array[i].sock > max_fd){
				max_fd=tslot_array[i].sock;
			}
		}
		
		tv.tv_usec = 0;
		tv.tv_sec = 10;

		if (end){
			break;
		}
		
		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if the socket cannot send
		 * or there is nothing to receive.
		 *
		 * By using the last socket file descriptor +1 select will wait
		 * for all active sockets.
		 */ 
		slots=0;
  		if((serr=select(max_fd + 1, &ready, NULL, &oob, &tv))){

		    for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++){
	
	   	    	if (tslot_array[i].sock < 0) {
			    continue;		
		        }	
		
		    	slots++;
		    	slot=&tslot_array[i];

			if (FD_ISSET(slot->sock,&oob)){ 
				sangoma_tdm_read_event(slot->sock,&tdm_api);
			}
		
			
			/* Check for rx packets */
                   	if (FD_ISSET(slot->sock,&ready)){

				err = sangoma_readmsg_tdm(slot->sock, 
							 Rx_data,sizeof(wp_tdm_api_rx_hdr_t), 
							 &Rx_data[sizeof(wp_tdm_api_rx_hdr_t)],
							 sizeof(Rx_data), 0);

		   // printf("RX DATA HDLC: Len=%i err=%i\n",err-sizeof(wp_api_hdr_t),err);
        		//print_packet(&Rx_data[sizeof(wp_api_hdr_t)],err-sizeof(wp_api_hdr_t));
				/* err indicates bytes received */
				if(err > 0) {
					unsigned char *rx_frame = 
						(unsigned char *)&Rx_data[sizeof(wp_tdm_api_rx_hdr_t)];
					int len = err;
	
					
					/* Rx packet recevied OK
					 * Each rx packet will contain sizeof(wp_api_hdr_t) bytes of
					 * rx header, that must be removed.  The
					 * first byte of the sizeof(wp_api_hdr_t) byte header will
					 * indicate an error condition.
					 */
#if 0
				printf("RX DATA: Len=%i\n",len);
	      			print_packet(rx_frame,len);
				frame++;
				if ((frame % 100) == 0) {
					sangoma_get_full_cfg(slot->sock, &tdm_api);
				}
				
					continue;
#endif					
					frame++;
					print_status++;
					slot->frames++;	
					
					wanpipe_hdlc_decode(slot->hdlc_eng,rx_frame,len);
				
					packets+=wanpipe_get_rx_hdlc_packets(slot->hdlc_eng);	
					error_bit+=wanpipe_get_rx_hdlc_errors(slot->hdlc_eng);	
					error_crc+=slot->hdlc_eng->decoder.stats.crc;
					error_abort+=slot->hdlc_eng->decoder.stats.abort;
					error_frm+=slot->hdlc_eng->decoder.stats.frame_overflow;
				
#if 1	
					if (wanpipe_get_rx_hdlc_errors(slot->hdlc_eng) != slot->last_error){
						slot->last_error = wanpipe_get_rx_hdlc_errors(slot->hdlc_eng);
						if (slot->last_error > 1){
					printf("\n%s: Errors = %i (crc=%i, abort=%i, frame=%i)\n",
						slot->if_name,
						wanpipe_get_rx_hdlc_errors(slot->hdlc_eng),
						slot->hdlc_eng->decoder.stats.crc,
						slot->hdlc_eng->decoder.stats.abort,
						slot->hdlc_eng->decoder.stats.frame_overflow);
						//sangoma_get_full_cfg(slot->sock, &tdm_api);
						//wanpipe_hdlc_dump_ring(slot->hdlc_eng);
						}
					}

					if ((slot->frames % 100) == 0 && packets == 0){
                     	printf("\n%s: Frames=%i Rx Packets = %i (crc=%i, abort=%i, frame=%i)\n",
						slot->if_name,
						frame,
						wanpipe_get_rx_hdlc_packets(slot->hdlc_eng),
						slot->hdlc_eng->decoder.stats.crc,
						slot->hdlc_eng->decoder.stats.abort,
						slot->hdlc_eng->decoder.stats.frame_overflow);
					}
#endif
#if 0
					printf("%s: Received packet %i, length = %i : %s\n", 
							slot->if_name,++Rx_count, err,
							((error_bit == 0)? "USR DATA OK" :
							 (error_bit&BOARD_UNDERRUN)? "BRD_UNDR" : 
							 (error_bit&TX_ISR_UNDERRUN) ? "TX_UNDR" :
							 "UNKNOWN ERROR"));
#endif

				} else {
					printf("\n%s: Error receiving data\n",slot->if_name);
				}

			 } /* If rx */
			 
		    } /* for all slots */
		
		    if (print_status>100*slots){	 
				print_status=0;
		    putchar('\r');
		    printf("Slots=%04i frame=%04i packets=%04i errors=%04i (crc=%04i abort=%04i frm=%04i)",
		    	slots,
			frame, 
			packets,
			error_bit>slots?error_bit:0,
			error_crc,
			error_abort,
			error_frm);
		    
		    	fflush(stdout);
		    }
		    packets=0;	
		    error_bit=0;
		    error_crc=0;
		    error_abort=0;
		    error_frm=0;
		  
		} else {
			printf("\n: Error selecting rx socket rc=0x%x errno=0x%x\n",
				serr,errno);
			perror("Select: ");
			//break;
		}
	}
	
	for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++){
		if (tslot_array[i].hdlc_eng) {
			wanpipe_unreg_hdlc_engine(tslot_array[i].hdlc_eng);
		}
	}
	
	//printf("\nRx Unloading HDLC\n");
	
} 

 
void process_con_tx(timeslot_t *slot) 
{
	unsigned int Tx_count, max_tx_len, Tx_offset=0, Tx_length,Tx_hdlc_len, Tx_encoded_hdlc_len;
	fd_set 	write;
	int err=0,i,tx_ok=1;
	unsigned char Tx_data[MAX_TX_DATA + sizeof(wp_api_hdr_t)];
	unsigned char Tx_hdlc_data[MAX_TX_DATA + sizeof(wp_api_hdr_t)];
	wanpipe_hdlc_engine_t *hdlc_eng;
	unsigned char next_idle;
	wanpipe_tdm_api_t tdm_api;
	int txdata=0;

	memset(&tdm_api,0,sizeof(tdm_api));
	
	hdlc_eng = wanpipe_reg_hdlc_engine();
	if (!hdlc_eng){
		printf("ERROR: Failed to register HDLC Engine\n");
		return;
	}
	
	Tx_count = 0;
       	sangoma_get_full_cfg(slot->sock, &tdm_api);

	if (tdm_api.wp_tdm_cmd.hdlc) {
                Tx_length = max_tx_len =  100;          
	}else{
       	 	Tx_length = max_tx_len = sangoma_tdm_get_usr_mtu_mru(slot->sock, &tdm_api);
	}

	/* Send double of max so that a single big frame gets separated into two actual packets */
	Tx_hdlc_len=(max_tx_len*2);
	Tx_hdlc_len*= 0.75;

	printf("User MTU/MRU=%i  Tx Len = %i \n",max_tx_len,Tx_hdlc_len);
	
	/* If running HDLC_STREAMING then the received CRC bytes
         * will be passed to the application as part of the 
         * received data.  The CRC bytes will be appended as the 
	 * last two bytes in the rx packet.
	 */

	memset(&Tx_data[0],0,MAX_TX_DATA + sizeof(wp_api_hdr_t));
	slot->data=0x0;
	for (i=0;i<Tx_hdlc_len;i++){
		if (slot->data){
			Tx_data[i+sizeof(wp_api_hdr_t)] = slot->data;
		}else{
			Tx_data[i+sizeof(wp_api_hdr_t)] = i;
		}
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

	printf("%s: Tx Starting to write on sock %i data (0x%X)  f=%x l=%x hdr_sz=%i\n",
	 	slot->if_name,slot->sock,slot->data,Tx_data[sizeof(wp_api_hdr_t)],Tx_data[Tx_hdlc_len+sizeof(wp_api_hdr_t)-1],
		sizeof(wp_api_hdr_t));	

		
	//pause();
	
	for(;;) {	
		FD_ZERO(&write);
		FD_SET(slot->sock,&write);

		
		if (end){
			break;
		}
		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if the socket cannot send
		 * or there is nothing to receive.
		 *
		 * By using the last socket file descriptor +1 select will wait
		 * for all active sockets.
		 */ 
#if 1
		/* If we got busy on last frame repeat the frame */
		if (tx_ok == 1){
#if 0
			printf("TX DATA ORIG: Len=%i\n",Tx_hdlc_len);
	      		print_packet(&Tx_data[sizeof(wp_api_hdr_t)],Tx_hdlc_len);
#endif		
			wanpipe_hdlc_encode(hdlc_eng,&Tx_data[sizeof(wp_api_hdr_t)],Tx_hdlc_len,&Tx_hdlc_data[sizeof(wp_api_hdr_t)],&Tx_encoded_hdlc_len,&next_idle);
			if (Tx_encoded_hdlc_len < (max_tx_len*2)){
				int j;
				for (j=0;j<((max_tx_len*2) - Tx_encoded_hdlc_len);j++){
					Tx_hdlc_data[sizeof(wp_api_hdr_t)+Tx_encoded_hdlc_len+j]=next_idle;
				}
				Tx_encoded_hdlc_len+=j;
			}	
#if 0		
			printf("TX DATA HDLC: Olen=%i Len=%i\n",Tx_hdlc_len,Tx_encoded_hdlc_len);
        		print_packet(&Tx_hdlc_data[sizeof(wp_api_hdr_t)],Tx_encoded_hdlc_len);
#endif		
			if (Tx_encoded_hdlc_len > (max_tx_len*2)){
				printf("Tx hdlc len > max %i\n",Tx_encoded_hdlc_len);
				continue;
			}

			Tx_length=max_tx_len;
			Tx_offset=0;
			//printf("INITIAL Fragment Chunk tx! %i Tx_encoded =%i \n", Tx_offset,Tx_encoded_hdlc_len);
	
#if 0		
			if ((Tx_count % 60) == 0){
				Tx_hdlc_len++; /* Introduce Error */
			}
#endif
#if 0 
			printf("Data %i\n",Tx_hdlc_len);
			for (i=0;i<Tx_hdlc_len;i++){
				printf(" 0x%X",Tx_hdlc_data[sizeof(wp_api_hdr_t)+i]);
			}
			printf("\n");
#endif		
			tx_ok=0;
		} 
#endif		
  		if(select(slot->sock + 1,NULL, &write, NULL, NULL)){

			/* Check if the socket is ready to tx data */
		   	if (FD_ISSET(slot->sock,&write)){

#if 1
			        err=sangoma_writemsg_tdm(slot->sock, 
						     Tx_hdlc_data, sizeof(wp_api_hdr_t), 
						     &Tx_hdlc_data[sizeof(wp_api_hdr_t)+Tx_offset],
						     Tx_length,0);
#endif
				if (err > 0){
					/* Packet sent ok */
					//printf("\t\t%s:Packet sent: Len=%i Data=0x%x : %i\n",
					//		slot->if_name,err,slot->data,++Tx_count);
					//putchar('T');
						
					Tx_offset+=Tx_length;

					if (Tx_offset >= Tx_encoded_hdlc_len){ 
						//printf("LAST Chunk tx! %i \n", Tx_offset);
						Tx_offset=0;
						Tx_length=max_tx_len;
						/* Last fragment transmitted */
						/* pass throught */
					} else {
						Tx_length = Tx_encoded_hdlc_len - Tx_length;
						if (Tx_length > max_tx_len) {
							Tx_length=max_tx_len;
						}
	//					printf("MIDDLE Fragment Chunk tx! %i \n", Tx_offset);
						continue;
					}		

#if RAND_FRAME 	
					if (Tx_count%10 == 0){
						Tx_hdlc_len=myrand(max_tx_len*2*0.75);
						for (i=0;i<Tx_hdlc_len;i++){
							Tx_data[i+sizeof(wp_api_hdr_t)] = myrand(255);
						}
						mysrand(myrand(255));
					}
#endif					


#if 0
					if (Tx_count%100 == 0) {
						txdata = (txdata + 1) % 2;
						for (i=0;i<Tx_hdlc_len;i++){
							Tx_data[i+sizeof(wp_api_hdr_t)] = txdata;;
						}
	    			}
#endif
					Tx_count++;
					tx_ok=1;
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

#if 0
			if (Tx_count > MAX_TX_FRAMES){
				break;
			}
#endif
#if 0
			if (Tx_count % 500){
				slot->data=slot->data+1;
				for (i=0;i<Tx_length;i++){
					Tx_data[i+sizeof(wp_api_hdr_t)] = slot->data;
				}
				tx_change_data=1;
				tx_change_data_cnt=0;
			}
#endif
		} else {
                	printf("\nError selecting socket\n");
			break;
		}
	}
	//printf("\nTx Unloading HDLC\n");
	wanpipe_unreg_hdlc_engine(hdlc_eng);
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
	int x,i;
	
	if (argc < 3){
		printf("Usage: rec_wan_sock <router name> <interface name> <data> ...\n");
		exit(0);
	}	
	
	nice(-11);
	
	signal(SIGINT,&sig_end);
	signal(SIGTERM,&sig_end);
	
	memset(&tslot_array,0,sizeof(tslot_array));
	for (i=0;i<MAX_NUM_OF_TIMESLOTS;i++){
		tslot_array[i].sock=-1;
	}
	
	strncpy(router_name,argv[1],(sizeof(router_name)-1));	
	
	
	for (x=1;x<argc-1;x++) {	

		strncpy(tslot_array[x].if_name, argv[x+1], MAX_IF_NAME);		
		printf("Connecting to IF=%s\n",tslot_array[x].if_name);
	
#if 1		
		proceed = MakeConnection(&tslot_array[x],router_name);
		if( proceed == TRUE ){

			printf("Creating %s with tx data 0x%x : Sock=%i : x=%i\n",
				tslot_array[x].if_name,
				tslot_array[x].data,
				tslot_array[x].sock,
				x);
			
			if (!fork()){
				signal(SIGINT,&sig_end);
				signal(SIGTERM,&sig_end);
				process_con_tx(&tslot_array[x]);
				exit(0);
			}
		}else{
			if (tslot_array[x].sock){
				close(tslot_array[x].sock);
			}
			tslot_array[x].sock=-1;
		}
#endif
	}
	
 	process_con_rx();
	
	for (x=0;x<MAX_NUM_OF_TIMESLOTS;x++){
		if (tslot_array[x].sock){
			close(tslot_array[x].sock);
			tslot_array[x].sock=0;
		}
	}
	
	/* Wait for the clear call children */
	return 0;
};
