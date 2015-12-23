/*****************************************************************************
* bsc_api.c	CHDLC API: Receive Module
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


#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <linux/wanpipe.h>
#include <linux/if_wanpipe.h>
#include <linux/sdla_bscmp.h>

#define FALSE	0
#define TRUE	1

#define MAX_TX_DATA     1000	/* Size of tx data */  
#define MAX_FRAMES 	5000     /* Number of frames to transmit */  

#define WRITE 1

/*===================================================
 * Golobal data
 *==================================================*/

unsigned short Rx_lgth;
unsigned char Rx_data[16000];
unsigned char Tx_data[MAX_TX_DATA + sizeof(api_tx_hdr_t)];
int sock=0;


/*=================================================== 
 * Function Prototypes 
 *==================================================*/
int MakeConnection(char *, char *);
void print_menu (void);


/*=================================================== 
 * Global Declerations 
 *==================================================*/

#define MAX_CMD_INDEX 24

typedef struct {
	int offset;
	int default_val;
	char var_name[50];
}cmd_args_t;

typedef struct {
	int cmd;
	char cmd_string[50];
	cmd_args_t *args;
	int length;
}cmd_key_t;



cmd_args_t add_station[] = {
	{offsetof(BSC_MAILBOX_STRUCT,poll_address), 0xC1, "poll_address"},
	{offsetof(BSC_MAILBOX_STRUCT,select_address), 0xC1, "select_address"},
	{offsetof(BSC_MAILBOX_STRUCT,device_address), 0xC1, "device_address"},
	{(offsetof(BSC_MAILBOX_STRUCT,data)+offsetof(ADD_STATION_STRUCT,max_tx_queue)),0,"max_tx_queue"},
	{(offsetof(BSC_MAILBOX_STRUCT,data)+offsetof(ADD_STATION_STRUCT,max_rx_queue)),0,"max_rx_queue"},
	{(offsetof(BSC_MAILBOX_STRUCT,data)+offsetof(ADD_STATION_STRUCT,station_flags)),0,"station_flags"},
	{-1,-1,""},
};

cmd_args_t write_data[] = {
	{offsetof(BSC_MAILBOX_STRUCT,station), 0, "station"},
	{offsetof(BSC_MAILBOX_STRUCT,misc_tx_rx_bits), 0, "misc_tx_rx_bits"},
	{-1,-1,""},
};

cmd_args_t read_data[] = {
	{offsetof(BSC_MAILBOX_STRUCT,station), 0, "station"},
	{-1,-1,""},
};

cmd_args_t set_modem[] = {
	{offsetof(BSC_MAILBOX_STRUCT,data),0,"DCD=bit 1, CTS=bit 2"},
	{-1,-1,""},
};



cmd_key_t cmd_list[] = {
{0,"EXIT",NULL},
{SET_CONFIGURATION, "SET_CONFIGURATION",NULL}, 	
{READ_CONFIGURATION,"READ_CONFIGURATION",NULL}, 
{ADD_STATION,"ADD_STATION",add_station,3}, 	
{DELETE_STATION,"DELETE_STATION",NULL}, 
{DELETE_ALL_STATIONS,"DELETE_ALL_STATIONS",NULL},
{LIST_STATIONS,"LIST_STATIONS",NULL},
{OPEN_LINK,"OPEN_LINK",NULL},
{CLOSE_LINK,"CLOSE_LINK",NULL},
{LINK_STATUS,"LINK_STATUS",NULL}, 
{READ_OPERATIONAL_STATISTICS,"READ_OPERATIONAL_STATISTICS",NULL},
{READ_CODE_VERSION,"READ_CODE_VERSION",NULL},
{SET_STATION_STATUS,"SET_STATION_STATUS",NULL}, 
{SET_GENERAL_OR_SPECIFIC_POLL,"SET_GENERAL_OR_SPECIFIC_POLL",NULL}, 
{READ_MODEM_STATUS,"READ_MODEM_STATUS",NULL}, 	
{SET_MODEM_STATUS,"SET_MODEM_STATUS",set_modem,1},
{READ_OPERATIONAL_STATISTICS,"READ_OPERATIONAL_STATISTICS",NULL},
{READ_COMMS_ERROR_STATISTICS,"READ_COMMS_ERROR_STATISTICS",NULL},
{READ_BSC_ERROR_STATISTICS,"READ_BSC_ERROR_STATISTICS",NULL},		 
{FLUSH_OPERATIONAL_STATISTICS,"FLUSH_OPERATIONAL_STATISTICS",NULL},		
{FLUSH_COMMS_ERROR_STATISTICS,"FLUSH_COMMS_ERROR_STATISTICS",NULL},
{FLUSH_BSC_ERROR_STATISTICS,"FLUSH_BSC_ERROR_STATISTICS",NULL},	
{FLUSH_BSC_TEXT_BUFFERS,"FLUSH_BSC_TEXT_BUFFERS",NULL},
{BSC_READ,"READ_DATA",read_data,0}, 
{BSC_WRITE,"WRITE_DATA",write_data,10},
};

static unsigned char menu[]="\n"\
"\n" \
"		BiSync Command Menu \n" \
"		=================== \n" \
"\n" \
" 1. SET_CONFIGURATION			14. READ_MODEM_STATUS \n" \
" 2. READ_CONFIGURATION			15. SET_MODEM_STATUS \n" \
"\n" \
" 3. ADD_STATION 			16. READ_OPERATIONAL_STATISTICS\n" \
" 4. DELETE_STATION 			17. READ_COMMS_ERROR_STATISTICS\n" \
" 5. DELETE_ALL_STATIONS 		18. READ_BSC_ERROR_STATISTICS	\n" \
" 6. LIST_STATIONS \n" \
" 					19. FLUSH_OPERATIONAL_STATISTICS\n" \
"\n" \
" 7. OPEN_LINK 				20. FLUSH_COMMS_ERROR_STATISTICS \n" \
" 8. CLOSE_LINK 				21. FLUSH_BSC_ERROR_STATISTICS \n" \
" 					22. FLUSH_BSC_TEXT_BUFFERS\n" \
" 9. LINK_STATUS \n" \
"10. READ_OPERATIONAL_STATISTICS 	23. READ DATA \n" \
"11. READ_CODE_VERSION 			24. WRITE DATA \n" \
"\n" \
"12. SET_STATION_STATUS 			0.  Exit \n" \
"13. SET_GENERAL_OR_SPECIFIC_POLL \n" \
" \n" \
"	Please Select a command (1-24): ";



void print_menu (void)
{

	printf (menu);
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

	return( TRUE );

}

int command_map (int cmd)
{
	int i;
	for (i=0;i<=MAX_CMD_INDEX;i++){
		if (cmd == cmd_list[i].cmd)
			return i;
	}
	return 0;
}


int DoCommand (BSC_MAILBOX_STRUCT *mbox)
{

	return ioctl(sock, SIOC_WANPIPE_BSC_CMD, mbox);
}


void DisplayResults (BSC_MAILBOX_STRUCT *mbox)
{
	int i;
	printf ("\n");
	switch (mbox->return_code){

	case 0:
		printf("Command %s executed ok!\n",
				cmd_list[command_map(mbox->command)].cmd_string);

		if (mbox->buffer_length){
			printf("Command Data: \n");
			for (i=0;i<mbox->buffer_length;i++){
				printf ("0x%x ",mbox->data[i]);
			}
			printf ("\n");
		}
		break;
	default:
		printf("Command %s failed!\n",
				cmd_list[command_map(mbox->command)].cmd_string);
	}
}

void mypause(void)
{
	printf("\nPlease press any key to continue...");
	fflush(stdout);

	getchar();
	getchar();
	//system("clear");
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
	int command;
	int i;
	char input[100];
	BSC_MAILBOX_STRUCT mbox;

	if (argc != 3){
		printf("Usage: rec_wan_sock <router name> <interface name>\n");
		exit(0);
	}	

	proceed = MakeConnection(argv[argc - 2], argv[argc - 1]);
	if( proceed == FALSE ){
		return -EINVAL;
	}
	
	for (;;){
		print_menu();
		fflush(stdout);
		scanf("%s",input);
		command=strtoul(input, NULL, 0);
		
		if (command < 0 || command > MAX_CMD_INDEX){
			printf("\nIllegal Command, Try again!\n");
			mypause();
			continue;
		}
		
		memset(&mbox,0,sizeof(BSC_MAILBOX_STRUCT));
		printf("\nExecuting Command: %s\n",cmd_list[command].cmd_string);

		if (!command)
			break;
		
		mbox.command = cmd_list[command].cmd;
	
		if (cmd_list[command].args){
			char input[100];
			int arg_val;
			cmd_args_t *args=cmd_list[command].args;
			for (i=0;;i++){
				if (args[i].offset==-1){
					break;
				}
				if (args[i].default_val==-1)
					break;

				printf("%s: ",args[i].var_name);
				scanf("%s",input);
				arg_val=strtoul(input, NULL, 0);
				*((unsigned char*)&mbox+args[i].offset)=(unsigned char)arg_val;
			}
			mbox.buffer_length=cmd_list[command].length;
			if (mbox.command == BSC_WRITE){
				for (i=0;i<mbox.buffer_length;i++){
					mbox.data[i]=1;
				}
			}
		}
		
		if (DoCommand(&mbox) != 0){
			printf("Command Failed!\n");
		}else{
			DisplayResults(&mbox);
		}
		mypause();
	}	

	close(sock);
	return 0;
};


