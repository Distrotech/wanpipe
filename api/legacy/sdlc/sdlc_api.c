/*****************************************************************************
* bsc_api.c	BiSync API: Receive Module
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
* Description:
*
* 	The bsc_api.c utility will bind to a socket to a bsc network
* 	interface, and continously tx and rx packets to an from the sockets.
*
*	This example has been written for a single interface in mind, 
*	where the same process handles tx and rx data.
*
*	A real world example, should use different processes to handle
*	tx and rx spearately.  
*
* Command Documentation:
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
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <signal.h>
#include <linux/if.h>
#include <linux/wanpipe.h>
#include <linux/sdla_sdlc.h>
#include "sdlc_api.h"

#define FALSE	0
#define TRUE	1

#define LGTH_CRC_BYTES	2
#define MAX_TX_DATA     100	/* Size of tx data */  
#define MAX_FRAMES 	5000     /* Number of frames to transmit */  

#define STATION_ADDR	0x01
#define MAX_SDLC_LIST	100

#define MAX_INIT_SDLC_ADDR_CFG	1

#define MAX_TX_DELAY 	10000
#define MAX_INIT_TX_CNT 0

/*===================================================
 * Golobal data
 *==================================================*/

unsigned short rx_len;
unsigned char  g_rx_data[1000];
int 	sock;
int g_tx_enabled=0;
int g_tx_waiting=0;
int sdlc_exit=0;
int max_sdlc_stations=MAX_INIT_SDLC_ADDR_CFG;
/* Structure used to execute BiSync commands */
wan_mbox_t mbox;


sdlc_ch_t sdlc_ch_list[MAX_SDLC_LIST];

/*=================================================== 
 * Function Prototypes 
 *==================================================*/
int MakeConnection(char *, char *);
void process_con( void);
int sdlc_open_addr(int);
int sdlc_close_all_ch (void);
int sdlc_tx_all_ch (void);
int sdlc_tx_all_enable (void);
int sdlc_check_tx_all_ch(void);


/*===================================================
 * DoCommand
 *
 * 	Execute a BiSync command directly on the
 * 	sangoma adapter.  Refer to the 
 * 	/usr/share/doc/wanpipe/bisync.pdf for the
 * 	list and explanation of BiSync commands.
 * 
 */

int DoCommand (wan_mbox_t *mbox)
{
	return ioctl(sock, SIOC_WANPIPE_EXEC_CMD, mbox);
}

/*===================================================
 * MakeConnection
 *
 *   o Create a Socket
 *   o Bind a socket to a wanpipe network interface
 *       (Interface name is supplied by the user)
 *==================================================*/         

int MakeConnection(char *r_name, char *i_name ) 
{
	struct wan_sockaddr_ll 	sa;

	memset(&sa,0,sizeof(struct wan_sockaddr_ll));
	errno = 0;
   	sock = socket(AF_WANPIPE, SOCK_RAW, 0);
   	if( sock < 0 ) {
      		perror("Socket");
      		return( FALSE );
   	} /* if */
  
	printf("\nConnecting to router %s, interface %s\n", r_name, i_name);
	
	strcpy( sa.sll_device, i_name);
	strcpy( sa.sll_card, r_name);
	sa.sll_protocol = htons(PVC_PROT);
	sa.sll_family=AF_WANPIPE;

        if(bind(sock, (struct sockaddr *)&sa, sizeof(struct wan_sockaddr_ll)) < 0){
                perror("bind");
		printf("Failed to bind a socket to %s interface\n",i_name);
                exit(0);
        }
	printf("Socket bound to %s\n\n",i_name);

	return (TRUE);

}

int sdlc_open_addr(int station_addr)
{
	int err;

	
	memset(&mbox,0,sizeof(mbox));

	/* The user should add as many station as needed
	 * at this point */
	mbox.wan_command=ADD_STATION;
	mbox.wan_data_len=0;
	mbox.wan_sdlc_address=station_addr;
	mbox.wan_sdlc_poll_interval=30;

	if ((err=DoCommand(&mbox)) != 0){
		if (err != STATION_ALREADY_ADDED){
			printf("Failed to add station %i! rc=0x%X\n",
					station_addr,err);
			return FALSE;
		}else{
			printf("Station %i already added\n",
					mbox.wan_sdlc_address);
		}
	}else{
		printf("Added station %i\n",mbox.wan_sdlc_address);
	}
	
	mbox.wan_command=ACTIVATE_STATION;
	mbox.wan_data_len=0;
	mbox.wan_sdlc_address=station_addr;
	mbox.wan_sdlc_poll_interval=30;

	if ((err=DoCommand(&mbox)) != 0){
		if (err != STATION_ALREADY_ACTIVATED){
			printf("Failed to activate station %i! rc=0x%X\n",
					station_addr,err);
			return FALSE;
		}else{
			printf("Station %i already active\n",
					mbox.wan_sdlc_address);
		}
	}else{
		printf("Activated station %i\n",mbox.wan_sdlc_address);
	}

	return TRUE;
}


void sdlc_close_addr(int station_addr)
{
	int err;
	
	memset(&mbox,0,sizeof(mbox));

	/* The user should add as many station as needed
	 * at this point */
	mbox.wan_command=DEACTIVATE_STATION;
	mbox.wan_data_len=0;
	mbox.wan_sdlc_address=station_addr;

	if ((err=DoCommand(&mbox)) != 0){
		printf("Failed to deactivate station! 0x%X\n",err);
	}
	
	memset(&mbox,0,sizeof(mbox));

	/* The user should add as many station as needed
	 * at this point */
	mbox.wan_command=DELETE_STATION;
	mbox.wan_data_len=0;
	mbox.wan_sdlc_address=station_addr;

	if ((err=DoCommand(&mbox)) != 0){
		printf("Failed to delete station! 0x%X\n",err);
		return;
	}

	printf("Station %i deleted!\n",station_addr);

	return;

}


/*===========================================================
 * process_con 
 *
 *   o Tx/Rx data to and from the socket 
 *   o Cast received data to an api_rx_element_t data type 
 *   o The received packet contains 16 bytes header 
 *
 *	------------------------------------------
 *      |  16 bytes      |        X bytes        ...
 *	------------------------------------------
 * 	   Header              Data
 *
 * RX DATA:
 * --------
 * 	Each rx data packet contains the 16 byte header!
 * 	
 *   	o Rx 16 byte data structure:
 *
 *		typedef struct {
 *       	 	unsigned char   station      PACKED;
 *        		unsigned char   reserved[15]    PACKED;
 *		} api_rx_hdr_t; 
 *
 *		typedef struct {
 *        		api_rx_hdr_t    api_rx_hdr      PACKED;
 *        		void *          data            PACKED;
 *		} api_rx_element_t;
 *
 * 	station:
 * 		bisync station that received a packet
 *
 * TX_DATA:
 * --------
 *	Each tx data packet MUST contain a 16 byte header!
 *
 * 	o Tx 16 byte data structure
 * 	
 *		typedef struct {
 *			unsigned char 	station		PACKED;
 *			unsigned char 	misc_tx_rx_bits	PACKED;
 *			unsigned char  	reserved[14]	PACKED;
 *		} api_tx_hdr_t;
 *
 *		typedef struct {
 *			api_tx_hdr_t 	api_tx_hdr	PACKED;
 *			void *		data		PACKED;
 *		} api_tx_element_t;
 *
 * 	station:
 * 	
 * 		The station filed must be used to indicate 
 * 		the station number on which the packet 
 * 		should be transmitted.
 * 	 
 * 	misc_tx_rx_bits:
 * 		The misc_tx_rx_bits can be used to control 
 * 		the WRITE command.
 *
 * 	Refer to the /usr/share/doc/wanpipe/bisync.pdf
 */

void process_con(void) 
{
	wan_api_t* wan_api;
	fd_set 	ready,oob,write;
	int err,i,j;
	unsigned char *data;
	struct timeval tv;

	for (i=1;i<=max_sdlc_stations;i++){
		sdlc_ch_t *psdlc_ch = &sdlc_ch_list[i];
	
		memset(psdlc_ch,0,sizeof(sdlc_ch_t));
		
		err=sdlc_open_addr(i);
		if (err == FALSE){
			continue;
		}

		psdlc_ch->addr=i;
		psdlc_ch->tx_len=MAX_TX_DATA;

		/* Initialize the tx packet. The 16 byte header must
	 	 * be inserted before tx data. */  
		wan_api = (wan_api_t*)&psdlc_ch->tx_data[0];
		data = (unsigned char *)&wan_api->data;

		/* Create a data packet, as an example
		 * use addr value to fill the data packet */
		for (j=0;j<psdlc_ch->tx_len;j++){
			data[j] = (unsigned char)psdlc_ch->addr;
		}

		wan_api->wan_api_sdlc_station 		= psdlc_ch->addr;
		wan_api->wan_api_sdlc_pf      		= 0;
		wan_api->wan_api_sdlc_poll_interval 	= 0;
		wan_api->wan_api_sdlc_general_mb_byte	= 0;

		psdlc_ch->tx_delay    = MAX_TX_DELAY;
		psdlc_ch->pkts_to_tx  = MAX_INIT_TX_CNT;
		psdlc_ch->tx_delay_enabled = 0;
		psdlc_ch->tx_count = 1;
	}
	
	for(;;) {	

		FD_ZERO(&ready);
		FD_ZERO(&write);
		FD_ZERO(&oob);
		FD_SET(sock,&ready);
		FD_SET(sock,&oob);

		sdlc_check_tx_all_ch();
	
		if (g_tx_enabled){ 
			FD_SET(sock,&write);
			tv.tv_sec=0;
			tv.tv_usec=10000;

		}else if (g_tx_waiting){
			tv.tv_sec=0;
			tv.tv_usec=10000;
		}else{
			/* Nothing to Tx, thus use a large
			 * select() timeout value */
			tv.tv_sec=5;
			tv.tv_usec=10000;
		}

		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if the socket cannot send
		 * or there is nothing to receive.
		 *
		 * By using the last socket file descriptor +1 select will wait
		 * for all active sockets.
		 */ 

		if (sdlc_exit){
			break;
		}

		err = select(sock + 1,&ready, &write, &oob, &tv);

		if (err > 0){

			if (FD_ISSET(sock,&oob)){
				err=recv(sock, g_rx_data, sizeof(g_rx_data), MSG_OOB);
				perror("Recv OOB");
				break;
			}
			
			/* Check for rx packets */
                   	if (FD_ISSET(sock,&ready)){

				err = recv(sock, g_rx_data, sizeof(g_rx_data), 0);

				/* err indicates bytes received */
				if(err > 0) {
	
					/* Rx packet recevied OK
					 * Each rx packet will contain 16 bytes of
					 * rx header, that must be removed.  The
					 * first byte of the 16 byte header will
					 * indicate an error condition.
					 */

					wan_api = (wan_api_t*)g_rx_data;

					/* Check the packet length, after we remove
					 * the 16 bytes rx header */
					rx_len = err - sizeof(wan_api_hdr_t);

					if (wan_api->wan_api_sdlc_station > MAX_SDLC_LIST){
						printf("Warning: Rx data on invalid Station %i\n",
								wan_api->wan_api_sdlc_station);
					}else{
					
						sdlc_ch_t *psdlc_ch = &sdlc_ch_list[wan_api->wan_api_sdlc_station];
						if (!psdlc_ch->addr){
							printf("Warning: Rx data on unconfig Station %i\n",
								wan_api->wan_api_sdlc_station);
						}else{
							psdlc_ch->rx_count++;

							printf("Rx pkt %i, Station %i Pf=%i Len = %i\n", 
								    psdlc_ch->rx_count,
								    wan_api->wan_api_sdlc_station,
								    wan_api->wan_api_sdlc_pf,
								    rx_len);
						}
					}
						
				} else {
					printf("\nError receiving data\n");
					break;
				}

			 } 
		  
			/* Check if the socket is ready to tx data */
		   	if (FD_ISSET(sock,&write)){

				sdlc_tx_all_ch();

		   	}

		}else if (err == 0){

			//printf("Select Timeout %li\n",get_cur_time());
		
		}else{
			//perror("Select");
		}
	}

	sdlc_close_all_ch();

	close (sock);
} 


void sig_tx(int sig)
{
	sdlc_tx_all_enable();
}

void sig_end(int sig)
{
	sdlc_exit=1;	
}

int sdlc_close_all_ch (void)
{
	int i;
	
	for (i=1;i<=max_sdlc_stations;i++){
		sdlc_ch_t *psdlc_ch = &sdlc_ch_list[i];
		
		if (!psdlc_ch->addr){
			continue;
		}
		
		sdlc_close_addr(psdlc_ch->addr);

		psdlc_ch->addr=0;
	}

	return 0;
}


int sdlc_tx_all_enable (void)
{
	int i;

	for (i=1;i<=max_sdlc_stations;i++){
		sdlc_ch_t *psdlc_ch = &sdlc_ch_list[i];

		if (!psdlc_ch->addr){
			continue;
		}
		
		psdlc_ch->pkts_to_tx=100;
	}

	return 0;
}


int sdlc_check_tx_all_ch(void)
{
	int i;
	unsigned long timeout=get_cur_time();
	unsigned int diff;
	
	g_tx_enabled=0;
	g_tx_waiting=0;

	for (i=1;i<=max_sdlc_stations;i++){
		sdlc_ch_t *psdlc_ch = &sdlc_ch_list[i];

		if (!psdlc_ch->addr){
			continue;
		}
	
		diff=timeout>=psdlc_ch->tx_cur_time?
			(timeout-psdlc_ch->tx_cur_time):
			((1000000-psdlc_ch->tx_cur_time)+timeout);
#if 0	
		printf("%s: Checking Addr %i Pkts=%i TxDelay=%i Timeout=%i CurTiem=%i Diff=%i\n",
				__FUNCTION__,
				psdlc_ch->addr,
				psdlc_ch->pkts_to_tx,
				psdlc_ch->tx_delay_enabled,
				timeout,
				psdlc_ch->tx_cur_time,
				diff);
#endif		

		/* Check if there is anything to be 
		 * transmitted by any active channel,
		 * if we find anything, enable transmission*/
		if (psdlc_ch->pkts_to_tx){

			g_tx_waiting=1;
			
			if (psdlc_ch->tx_delay_enabled){

				/* The ch was busy last time we checked,thus
				 * make sure that we wait for "delay" time
				 * before we re-try */
			
				if (psdlc_ch->tx_delay <= diff){
					psdlc_ch->tx_delay_enabled=0;
					g_tx_enabled=1;
				}
			}else{
				g_tx_enabled=1;
			}
		}
	}

	return 0;
}



int sdlc_tx_all_ch (void)
{
	int i;
	int err;

	for (i=1;i<=max_sdlc_stations;i++){
		sdlc_ch_t *psdlc_ch = &sdlc_ch_list[i];

		if (!psdlc_ch->addr){
			continue;
		}

		if (psdlc_ch->tx_delay_enabled){
			continue;
		}

		err = send(sock,psdlc_ch->tx_data, psdlc_ch->tx_len + sizeof(wan_api_hdr_t), 0);

		if (err > 0){


			/* Packet sent ok */
			printf("Packet Sent Station %i Len=%i Cnt=%i \n",
				psdlc_ch->addr,
				err-sizeof(wan_api_hdr_t),
				psdlc_ch->tx_count);

			psdlc_ch->tx_count++;
			psdlc_ch->pkts_to_tx--;

		}

		if (errno == EBUSY){
			
			//printf("Send busy, try again ...\n");
			
			/* The driver is busy,  we must requeue this
			 * tx packet, and try resending it again 
	 	         * later. */

			psdlc_ch->tx_cur_time = get_cur_time();
			psdlc_ch->tx_delay_enabled=1;
#if 0	
			printf("Packet Busy: Cnt=%i Time=%li\n",
				psdlc_ch->tx_count,
				get_cur_time());
#endif			
		}else{

			/* Critical error: Should never happen */
			perror("Send");
		}
	}

	return 0;
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

	if (argc > 3){
		max_sdlc_stations=atoi(argv[3]);	
	}else if (argc > 2){
		max_sdlc_stations=MAX_INIT_SDLC_ADDR_CFG;
	}else{
			printf("\n");
		printf("Usage: sdlc_api <router name> <interface name> <num of stations>\n");
		printf("\n");
		printf("   eg: sdlc_api wanpipe1 sdlc0 2\n");
		printf("\n");
		printf("       Tx/Rx data on Station Station 1 and 2 \n\n");
		printf("       To enable Tx send a SIGUSR1 signal to sdlc_api\n");
		printf("       kill -SIGUSR1 $(pidof sdlc_api) or run ./tx_test\n\n");
		exit(0);
	}

	
	printf("Configuring for max sdlc stations = %i\n",max_sdlc_stations);

	signal(SIGUSR1,sig_tx);
	signal(SIGINT,sig_end);
	signal(SIGTERM,sig_end);

	proceed = MakeConnection(argv[1], argv[2]);
	if( proceed == TRUE ){
		process_con();
	}
	
	return 0;
};
