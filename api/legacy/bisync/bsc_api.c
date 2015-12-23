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
#include <linux/wanpipe.h>
#include <linux/sdla_bscmp.h>

#define FALSE	0
#define TRUE	1

#define LGTH_CRC_BYTES	2
#define MAX_TX_DATA     10	/* Size of tx data */  
#define MAX_FRAMES 	5000     /* Number of frames to transmit */  

#define WRITE 1

/*===================================================
 * Golobal data
 *==================================================*/

unsigned short Rx_lgth;
unsigned char Rx_data[16000];
unsigned char Tx_data[MAX_TX_DATA + sizeof(api_tx_hdr_t)];
int 	sock;

/* Structure used to execute BiSync commands */
BSC_MAILBOX_STRUCT mbox;


/*=================================================== 
 * Function Prototypes 
 *==================================================*/
int MakeConnection(char *, char *);
void process_con( void);



/*===================================================
 * DoCommand
 *
 * 	Execute a BiSync command directly on the
 * 	sangoma adapter.  Refer to the 
 * 	/usr/share/doc/wanpipe/bisync.pdf for the
 * 	list and explanation of BiSync commands.
 * 
 */

int DoCommand (BSC_MAILBOX_STRUCT *mbox)
{
	return ioctl(sock, SIOC_WANPIPE_BSC_CMD, mbox);
}


/*===================================================
 * ConfigBisync
 * 
 * 
 * 
 */ 

int ConfigBisync(BSC_MAILBOX_STRUCT *mbox)
{
	int err;
	BSC_CONFIG_STRUCT cfg = { 0, 		//line_speed_number
				  500, 		//max_data_frame_size
				  1,		//secondary_station
				  5, 		//num_consec_PAD_eof
				  2, 		//num_add_lead_SYN
				  0, //1	//conversational_mode
				  0,		//pp_dial_up_operation
				  0, 		//switched_CTS_RTS
				  1, 		//EBCDIC_encoding
				  1, 		//auto_open
				  0, 		//misc_bits
				  1, //0	//protocol_options1  
				  0, 		//protocol_options2
				  0,		//reserved_pp
				  10, 		//max_retransmissions
				  10, 		//fast_poll_retries
				  3, 		//TTD_retries
				  10, 		//restart_timer
				  1000, 	//pp_slow_restart_timer
				  50, 		//TTD_timer
				  100, 		//pp_delay_between_EOT_ENQ
				  50, 		//response_timer
				  100, 		//rx_data_timer
				  40, 		//NAK_retrans_delay_timer
				  50, 		//wait_CTS_timer
				  5, 		//mp_max_consec_ETX
				  0x7f, 	//mp_general_poll_address
				  2000, 	//sec_poll_timeout
				  20, 		//pri_poll_skips_inactive
				  3, 		//sec_additional_stn_send_gpoll
				  10, 		//pri_select_retries
				  0, 		//mp_multipoint_options
				  1		//reserved 0=RS232 1=V35
				};
	
	
	memset(mbox,0,MBOX_HEADER_SZ);

	mbox->command= CLOSE_LINK;
	DoCommand(mbox);

        mbox->buffer_length = sizeof(BSC_CONFIG_STRUCT);
        mbox->command = SET_CONFIGURATION;
	memcpy(mbox->data,&cfg,sizeof(BSC_CONFIG_STRUCT));

	if ((err=DoCommand(mbox)) != 0){
		printf("Failed to configure bisync station!\n");
	}

        return err;
	
	
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
	int err;

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

	memset(&mbox,0,sizeof(mbox));
	err=ConfigBisync(&mbox);
	if (err){
		return (FALSE);
	}
	
	memset(&mbox,0,sizeof(mbox));
	
	/* The user should add as many station as needed
	 * at this point */
	mbox.command=ADD_STATION;
	mbox.buffer_length=3;
	mbox.poll_address=0x40;
	mbox.select_address=0x60;
	mbox.device_address=0xc1;
	mbox.data[0]=0;
	mbox.data[1]=0;
	mbox.data[2]=0x80;

	if (DoCommand(&mbox) != 0){
		printf("Failed to add station!\n");
	}
	
	return (TRUE);

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
	unsigned int Rx_count,Tx_count,Tx_length;
	api_rx_element_t* api_rx_el;
	api_tx_element_t * api_tx_el;
	fd_set 	ready,write;
	int err,i;
	unsigned char *data;
	int tx_ok=0;

        Rx_count = 0;
	Tx_count = 0;
	Tx_length = MAX_TX_DATA;
	
	/* If running HDLC_STREAMING then the received CRC bytes
         * will be passed to the application as part of the 
         * received data.  The CRC bytes will be appended as the 
	 * last two bytes in the rx packet.
	 */

	memset(&Tx_data[0],0,MAX_TX_DATA + sizeof(api_tx_hdr_t));


	/* Initialize the tx packet. The 16 byte header must
	 * be inserted before tx data. */  
	api_tx_el = (api_tx_element_t*)&Tx_data[0];
	data = (unsigned char *)&api_tx_el->data;

	/* Send on a particular BiSync station */
	api_tx_el->wan_api_tx_hdr.station = 0;
	api_tx_el->wan_api_tx_hdr.misc_tx_rx_bits=0;


	/* Create a data packet */
	for (i=0;i<Tx_length;i++){
		data[i] = (unsigned char)1;
	}
	

	for(;;) {	
		FD_ZERO(&ready);
		FD_ZERO(&write);
		FD_SET(sock,&ready);
#ifdef WRITE
		if (tx_ok)
			FD_SET(sock,&write);
#endif

		/* The select function must be used to implement flow control.
	         * WANPIPE socket will block the user if the socket cannot send
		 * or there is nothing to receive.
		 *
		 * By using the last socket file descriptor +1 select will wait
		 * for all active sockets.
		 */ 

  		if(select(sock + 1,&ready, &write, NULL, NULL)){

			/* Check for rx packets */
                   	if (FD_ISSET(sock,&ready)){

				err = recv(sock, Rx_data, sizeof(Rx_data), 0);

				/* err indicates bytes received */
				if(err > 0) {
	
					/* Rx packet recevied OK
					 * Each rx packet will contain 16 bytes of
					 * rx header, that must be removed.  The
					 * first byte of the 16 byte header will
					 * indicate an error condition.
					 */

					api_rx_el = (api_rx_element_t*)Rx_data;

					/* Check the packet length, after we remove
					 * the 16 bytes rx header */
					Rx_lgth = err - sizeof(api_rx_hdr_t);

					
					printf("Received packet %i, Station=%i Length = %i\n", 
							++Rx_count,
							api_rx_el->hdr.wp_api_rx_hdr_station,
							Rx_lgth);

					/* In this example I an sending data after
					 * 4 received packets */
					if (!(Rx_count%4)){
						tx_ok=1;
					}
				} else {
					printf("\nError receiving data\n");
					break;
				}

			 } 
		  
			/* Check if the socket is ready to tx data */
		   	if (FD_ISSET(sock,&write)){

				err = send(sock,Tx_data, Tx_length + sizeof(api_tx_hdr_t), 0);

				if (err > 0){
					/* Packet sent ok */
					printf("Packet sent: Sent %i : %i\n",
							err,++Tx_count);
					tx_ok=0;
				}else{
					printf("Failed to send, try again ...\n");
					/* The driver is busy,  we must requeue this
					 * tx packet, and try resending it again 
					 * later. */
				}
				
		   	}


		   	/* Stop the program afer transmitting MAX_FRAMES */
			if (Tx_count == MAX_FRAMES)
				break;

		} else {
                	printf("\nError selecting socket\n");
			break;
		}
	}
	close (sock);
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

	if (argc != 3){
		printf("Usage: rec_wan_sock <router name> <interface name>\n");
		exit(0);
	}	

	proceed = MakeConnection(argv[argc - 2], argv[argc - 1]);
	if( proceed == TRUE ){
		process_con();
		return 0;
	}

	return 1;
};
