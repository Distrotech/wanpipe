/*****************************************************************************
* lib_api.c		Sangoma Command Line Arguments parsing library
*
* Author(s):	Nenad Corbic	<ncorbic@sangoma.com>
*				David Rokhvarg	<davidr@sangoma.com>
*
* Copyright:	(c) 2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ============================================================================
*/

#include "libsangoma.h"
#include "lib_api.h"

#define SINGLE_CHANNEL	0x2
#define RANGE_CHANNEL	0x1


char	read_enable=0;
char 	write_enable=0;
char 	primary_enable=0;
int 	tx_cnt=0;
int		rx_cnt=0;
int		tx_size=10;
int		tx_delay=0;
int		tx_data=-1;
int		buffer_multiplier=0;

int		 fe_read_cmd=0;
uint32_t fe_read_reg=0;
int		 fe_write_cmd=0;
uint32_t fe_write_reg=0;
uint32_t fe_write_data=0;

float 	rx_gain=0;
float 	tx_gain=0;
int 	rx_gain_cmd=0;
int 	tx_gain_cmd=0;

unsigned char tx_file[WAN_IFNAME_SZ];
unsigned char rx_file[WAN_IFNAME_SZ];

unsigned char daddr[TX_ADDR_STR_SZ];
unsigned char saddr[TX_ADDR_STR_SZ];
unsigned char udata[TX_ADDR_STR_SZ];

int	files_used=0;
int	verbose=0;

int	tx_connections;

int	ds_prot=0;
int	ds_prot_opt=0;
int	ds_max_mult_cnt=0;
unsigned int ds_active_ch=0;
int	ds_7bit_hdlc=0;
int 	direction=-1;

int 	tx_channels=1;
int		cause=0;
int 	diagn=0;

int 	wanpipe_port_no=0;
int 	wanpipe_if_no=0;

int		dtmf_enable_octasic = 0;
int		dtmf_enable_remora = 0;
int		remora_hook = 0;
int		usr_period = 0;
int		set_codec_slinear = 0;
int		set_codec_none = 0;
int		rbs_events = 0;
int		rx2tx = 0;
int 	flush_period=0;
int 	stats_period=0;
int		stats_test=0;
int		flush_stats_test=0;
int		hdlc_repeat=0;
int		ss7_cfg_status=0;
int		fe_read_test=0;
int 	hw_pci_rescan=0;
int 	force_open=0;

unsigned long parse_active_channel(char* val);

int init_args(int argc, char *argv[])
{
	int i;
	int c_cnt=0;

	sprintf((char*)daddr,"111");
	sprintf((char*)saddr,"222");
	sprintf((char*)udata,"C9");
	
	for (i = 0; i < argc; i++){
		
		if (!strcmp(argv[i],"-i") || !strcmp(argv[i],"-chan")){

			if (i+1 > argc-1){
				printf("ERROR: Number of Interfaces was NOT specified!\n");
				return WAN_FALSE;
			}
			wanpipe_if_no = atoi(argv[i+1]);
		
		}else if (!strcmp(argv[i],"-p") || !strcmp(argv[i],"-span")){

			if (i+1 > argc-1){
				printf("ERROR: Number of Ports was NOT specified!\n");
				return WAN_FALSE;
			}
			wanpipe_port_no = atoi(argv[i+1]);
			
		}else if (!strcmp(argv[i],"-r")){
			read_enable=1;
			c_cnt=1;
	
		}else if (!strcmp(argv[i],"-w")){
			write_enable=1;
			c_cnt=1;
		}else if (!strcmp(argv[i],"-hdlc_repeat")){
			hdlc_repeat=1;
		}else if (!strcmp(argv[i],"-stats_test")){
			stats_test=1;
			force_open=1;
			c_cnt=1;
		}else if (!strcmp(argv[i],"-flush_stats_test")){
			flush_stats_test=1;
			force_open=1;
			c_cnt=1;
		}else if (!strcmp(argv[i],"-ss7_cfg_status")){
			ss7_cfg_status=1;

		}else if (!strcmp(argv[i],"-fe_read_test")){
			fe_read_test=1;
		
		}else if (!strcmp(argv[i],"-hw_pci_rescan")){
			hw_pci_rescan=1;
			wanpipe_port_no=1;
			wanpipe_if_no=1;
			c_cnt=1;

		}else if (!strcmp(argv[i],"-force_open")){
			force_open=1;

		}else if (!strcmp(argv[i],"-pri")){
			primary_enable=1;
		
		}else if (!strcmp(argv[i],"-txcnt")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid tx cnt!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				tx_cnt = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid tx cnt!\n");
				return WAN_FALSE;
			}

		}else if (!strcmp(argv[i],"-rxcnt")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid rx cnt!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				rx_cnt = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid rx cnt!\n");
				return WAN_FALSE;
			}

		}else if (!strcmp(argv[i],"-txsize")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid tx size!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				tx_size = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid tx size, must be a digit!\n");
				return WAN_FALSE;
			}
		}else if (!strcmp(argv[i],"-txdelay")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid tx delay!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				tx_delay = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid tx delay, must be a digit!\n");
				return WAN_FALSE;
			}
		}else if (!strcmp(argv[i],"-txdata")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid tx data!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				tx_data = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid tx data, must be a digit!\n");
				return WAN_FALSE;
			}
		 
		}else if (!strcmp(argv[i],"-txfile")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid Tx File Name!\n");
				return WAN_FALSE;
			}

			strncpy((char*)tx_file, argv[i+1],WAN_IFNAME_SZ);
			files_used |= TX_FILE_USED;
			
		}else if (!strcmp(argv[i],"-rxfile")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid Rx File Name!\n");
				return WAN_FALSE;
			}

			strncpy((char*)rx_file, argv[i+1],WAN_IFNAME_SZ);
			files_used |= RX_FILE_USED;

		}else if (!strcmp(argv[i],"-daddr")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid daddr str!\n");
				return WAN_FALSE;
			}

			strncpy((char*)daddr, argv[i+1],TX_ADDR_STR_SZ);
		
		}else if (!strcmp(argv[i],"-saddr")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid saddr str!\n");
				return WAN_FALSE;
			}

			strncpy((char*)saddr, argv[i+1],TX_ADDR_STR_SZ);
		
		}else if (!strcmp(argv[i],"-udata")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid udata str!\n");
				return WAN_FALSE;
			}

			strncpy((char*)udata, argv[i+1],TX_ADDR_STR_SZ);

		}else if (!strcmp(argv[i],"-verbose")){
			verbose=1;

		}else if (!strcmp(argv[i],"-buffer_multiplier")) {
			if (i+1 > argc-1){
				printf("ERROR: Invalid prot!\n");
				return WAN_FALSE;
			}

			if(isdigit(argv[i+1][0])){
				buffer_multiplier = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid prot, must be a digit!\n");
				return WAN_FALSE;
			}

		}else if (!strcmp(argv[i],"-prot")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid prot!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				ds_prot = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid prot, must be a digit!\n");
				return WAN_FALSE;
			}

			
		}else if (!strcmp(argv[i],"-prot_opt")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid prot_opt!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				ds_prot_opt = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid prot_opt, must be a digit!\n");
				return WAN_FALSE;
			}
		}else if (!strcmp(argv[i],"-max_mult_cnt")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid max_mult_cnt!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				ds_max_mult_cnt = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid max_mult_cnt, must be a digit!\n");
				return WAN_FALSE;
			}
		}else if (!strcmp(argv[i],"-txchan")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid channels!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				tx_channels = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid channels, must be a digit!\n");
				return WAN_FALSE;
			}
		}else if (!strcmp(argv[i],"-diagn")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid diagn!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				diagn = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid diagn, must be a digit!\n");
				return WAN_FALSE;
			}
			
		}else if (!strcmp(argv[i],"-cause")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid cause!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				cause = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid cause, must be a digit!\n");
				return WAN_FALSE;
			}

		}else if (!strcmp(argv[i],"-7bit_hdlc")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid 7bit hdlc value!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				ds_7bit_hdlc = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid 7bit hdlc, must be a digit!\n");
				return WAN_FALSE;
			}

		}else if (!strcmp(argv[i],"-dir")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid direction value!\n");
				return WAN_FALSE;
			}		

			if(isdigit(argv[i+1][0])){
				direction = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid direction, must be a digit!\n");
				return WAN_FALSE;
			}
		}else if (!strcmp(argv[i],"-dtmf_octasic")){
			dtmf_enable_octasic = 1;
		}else if (!strcmp(argv[i],"-dtmf_remora")){
			dtmf_enable_remora = 1;
		}else if(!strcmp(argv[i],"-usr_period")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid 'usr_period' value!\n");
			}else{
				usr_period = atoi(argv[i+1]);
			}
		}else if(!strcmp(argv[i],"-flush_period")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid 'flush_period' value!\n");
			}else{
				flush_period = atoi(argv[i+1]);
			}
		}else if(!strcmp(argv[i],"-stats_period")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid 'stats_period' value!\n");
			}else{
				stats_period = atoi(argv[i+1]);
			}
		}else if(!strcmp(argv[i],"-rbs_events")){
			rbs_events = 1;
		}else if(!strcmp(argv[i],"-rx2tx")){
			rx2tx = 1;
		}else if(!strcmp(argv[i],"-fe_reg_read")) {
			if (i+1 > argc-1){
				printf("ERROR: Invalid 'flush_period' value!\n");
			}else{
				fe_read_cmd=1;
				sscanf(argv[i+1],"%x",(uint32_t*)&fe_read_reg);
			}
		}else if(!strcmp(argv[i],"-fe_reg_write")) {
			if (i+2 > argc-1){
				printf("ERROR: Invalid 'flush_period' value!\n");
			}else{
				fe_write_cmd=1;
				sscanf(argv[i+1],"%x",(uint32_t*)&fe_write_reg);
				sscanf(argv[i+2],"%x",(uint32_t*)&fe_write_data);
			}
		}else if (!strcmp(argv[i],"-rx_gain")) {
			if (i+1 > argc-1){
				printf("ERROR: Invalid 'rx_gain' value!\n");
			}else{
				rx_gain_cmd=1;
				sscanf(argv[i+1],"%f",&rx_gain);
			}
		}else if (!strcmp(argv[i],"-tx_gain")) {
			if (i+1 > argc-1){
				printf("ERROR: Invalid 'tx_gain' value!\n");
			}else{
				tx_gain_cmd=1;
				sscanf(argv[i+1],"%f",&tx_gain);
			}
		}
	}

	if (!wanpipe_port_no || !wanpipe_if_no){
		printf("ERROR: No Port Number AND No Interface Number! Both of them needed!\n");
		return WAN_FALSE;
	}

	if (!c_cnt){
		printf("Warning: No Read or Write Command\n");
	}

	return WAN_TRUE;
}

static char api_usage[]="\n"
"\n"
"<options>:\n"
"	-span  <port/span number>	#port/span number\n"
"	-chan  <if/chan number>		#if/chan number\n"
"	-r							#read enable\n"
"	-w							#write eable\n"
"\n"
"	example 1: sangoma_c -span 1 -chan 1 -r -verbose\n"
"	  in this example Wanpipe span 1, Interface chan 1 will be used for reading data\n"
"\n"
"<extra options>\n"
"	-txcnt   <digit>	#number of tx packets  (Dflt: 1)\n"
"	-txsize  <digit>	#tx packet size        (Dflt: 10)\n"
"	-txdelay <digit>	#delay in sec after each tx packet (Dflt: 0)\n"
"	-txdata  <digit>	#data to tx <1-255>\n"
"	-rx2tx	 			#transmit all received data\n"
"\n"
"	-dtmf_octasic		#Enable DTMF detection on Octasic chip\n"
"	-dtmf_remora		#Enable DTMF detection on A200 (SLIC) chip\n"
"	-remora_hook		#Enable On/Off hook events on A200\n"
"	-set_codec_slinear	#Enable SLINEAR codec\n"
"	-set_codec_none		#Disable codec\n"
"	-rbs_events			#Enable RBS change detection\n"
"\n"
"	-rxcnt   <digit>	#number of rx packets before exit\n"
"						#this number overwrites the txcnt\n"
"						#Thus, app will only exit after it\n"
"						#receives the rxcnt number of packets.\n"
"   -hdlc_repeat 		#enable hdlc repeat on tx hdlc write.\n"
"\n"
"	-verbose			#Enable verbose mode\n"
"\n";

void usage(char *api_name)
{
printf ("\n\nAPI %s USAGE:\n\n%s <options> <extra options>\n\n%s\n",
		api_name,api_name,api_usage);
}


/*============================================================================
 * TE1
 */
#if 0
unsigned long get_active_channels(int channel_flag, int start_channel, int stop_channel)
{
	int i = 0;
	unsigned long tmp = 0, mask = 0;

	if ((channel_flag & (SINGLE_CHANNEL | RANGE_CHANNEL)) == 0)
		return tmp;
	if (channel_flag & RANGE_CHANNEL) { /* Range of channels */
		for(i = start_channel; i <= stop_channel; i++) {
			mask = 1 << (i - 1);
			tmp |=mask;
		}
	} else { /* Single channel */ 
		mask = 1 << (stop_channel - 1);
		tmp |= mask; 
	}
	return tmp;
}
#endif


