/*****************************************************************************
* wanpipemon.c	Sangoma WANPIPE Monitor
*
* Author:       Alex Feldman <al.feldman@sangoma.com>	
* 		Nenad Corbic <ncorbic@sangoma.com>
*
* Copyright:	(c) 1995-2002 Sangoma Technologies Inc.
*
* ----------------------------------------------------------------------------
* Aug 02, 2002  Nenad Corbic	Updated all linux utilites and added 
*                               GUI features.
* May 30, 2002	Alex Feldman	Initial version based on ppipemon
*****************************************************************************/

/******************************************************************************
 * 			INCLUDE FILES					      *
 *****************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#if defined(__LINUX__)
# include <linux/version.h>
# include <linux/types.h>
# include <linux/if_packet.h>
# include <linux/if_wanpipe.h>
# include <linux/if_ether.h>
# include <linux/wanpipe_defines.h>
# include <linux/wanpipe_cfg.h>
# include <linux/wanpipe.h>
# include <linux/sdla_fr.h>
#else
# include <netinet/in_systm.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/udp.h>  
# include <wanpipe_defines.h>
# include <wanpipe_cfg.h>
# include <wanpipe.h>
#endif
#include "fe_lib.h"
#include "wanpipemon.h"

/******************************************************************************
 * 			DEFINES/MACROS					      *
 *****************************************************************************/
#define MDATALEN 2024

#ifndef SIOC_WANPIPE_MONITOR 
#define SIOC_WANPIPE_MONITOR SIOC_WANPIPE_PIPEMON
#endif


/******************************************************************************
 * 			GLOBAL VARIABLES				      *
 *****************************************************************************/
#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
char*	progname = "wanutil";
#else
char*	progname = "wanpipemon";
#endif

wan_udp_hdr_t			wan_udp;
int				sock = 0;
int 				wan_protocol = 0;
int				protocol_cb_size = sizeof(wan_mgmt_t)+sizeof(wan_cmd_t)+1;

int 				is_508;
int				gui_interface=0;


static sa_family_t		af = AF_INET;
static struct sockaddr_in	soin;
 char ipaddress[16];
int  udp_port = 9000;

short dlci_number;
char if_name[WAN_IFNAME_SZ+1];
int ip_addr = -1;
int start_xml_router=0;
int stop_xml_router=1;
int raw_data=0;
int trace_all_data=0;
int lcn_number=0;
unsigned char par_port_A_byte, par_port_B_byte;

int sys_timestamp=0;
int annexg_trace =0;

int pcap_output=0;
int pcap_prot=0;
FILE *pcap_output_file;
char pcap_output_file_name[50];

trace_prot_t trace_prot_opt[]={ 
	{"FR", FRAME,107},
	{"LAPB", LAPB, 0},
	{"X25", X25, 200},
	{"PPP", PPP,9},
	{"CHDLC", CHDLC,104},
	{"ETH", ETH, 1},
	{"IP", IP,12},
	{"",-1},
};

trace_prot_t trace_x25_prot_opt[]={
	{"DATA",DATA},
	{"PROT",PROT},
	{"",-1},
};

/* tracing global variables */
unsigned int TRACE_DLCI=0;
unsigned char TRACE_PROTOCOL=ALL_PROT;
unsigned int TRACE_X25_LCN=0;
unsigned char TRACE_X25_OPT=ALL_X25;
char TRACE_EBCDIC=0;
char TRACE_ASCII=0;
char TRACE_HEX=0;

char *cmd[MAX_CMD_ARG];
FT1_LED_STATUS FT1_LED;

/******************************************************************************
 * 			FUNCTION PROTOTYPES				      *
 *****************************************************************************/
void sig_end(int signal);
int		main(int, char**);
static int 	init(int, char**,char*, int len);
static void 	usage(void);
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
static void 	usage_trace_info(void);
static void 	usage_long(void);
#endif
static int  	MakeConnection(void);
static int 	GetWANConfig(void);
#if 0
static void 	sig_handler(int sigint);
#endif
int      	fail=0;
int 		xml_output=0;
//static sa_family_t	get_if_family(char*);

#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
struct fun_protocol function_lookup[] = {
	{ WANCONFIG_CHDLC,	"hdlc", CHDLCConfig, CHDLCUsage, CHDLCMain, CHDLCDisableTrace,
		                         CHDLCget_main_menu, CHDLCget_cmd_menu, 
					 CHDLC_read_FT1_status, CHDLC_set_FT1_mode, 2}, 
					 
	{ WANCONFIG_AFT,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },
	
	{ WANCONFIG_AFT_TE1,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },

	{ WANCONFIG_AFT_TE3,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },

	{ 0, "N/A", 0, 0, 0, 0,0 }, 
};
#else
struct fun_protocol function_lookup[] = {
	{ WANCONFIG_FR,		"fr",    FRConfig, FRUsage, FRMain, FRDisableTrace, 
		                         FRget_main_menu, FRget_cmd_menu, 
					 FR_read_FT1_status, FR_set_FT1_mode, 0 }, 
#if defined(__LINUX__)
	{ WANCONFIG_MFR,	"fr",    FRConfig, FRUsage, FRMain, FRDisableTrace, 
		                         FRget_main_menu, FRget_cmd_menu,
					 FR_read_FT1_status, FR_set_FT1_mode,0 },
#else
	{ WANCONFIG_MFR, 	"fr",  NULL, NULL, NULL, NULL,NULL,NULL,0 }, 
#endif
	{ WANCONFIG_PPP,	"ppp",   PPPConfig, PPPUsage, PPPMain, PPPDisableTrace, 
		                         PPPget_main_menu, PPPget_cmd_menu, 
					 PPP_read_FT1_status, PPP_set_FT1_mode,0 }, 
					 
	{ WANCONFIG_CHDLC,	"chdlc", CHDLCConfig, CHDLCUsage, CHDLCMain, CHDLCDisableTrace,
		                         CHDLCget_main_menu, CHDLCget_cmd_menu, 
					 CHDLC_read_FT1_status, CHDLC_set_FT1_mode, 2}, 
					 
	{ WANCONFIG_ATM,	"atm",   ATMConfig, ATMUsage, ATMMain, ATMDisableTrace, 
		                         ATMget_main_menu, ATMget_cmd_menu, 
					 ATM_read_FT1_status, ATM_set_FT1_mode, 2 }, 
					 
	{ WANCONFIG_AFT,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },

	{ WANCONFIG_AFT_TE1,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },

	{ WANCONFIG_AFT_TE3,	"aft",   AFTConfig, AFTUsage, AFTMain, AFTDisableTrace, 
		                         AFTget_main_menu, AFTget_cmd_menu, 
					 NULL,NULL, 2 },


	{ WANCONFIG_ADSL,	"adsl",  ADSLConfig, ADSLUsage, ADSLMain, ADSLDisableTrace, 
		                         ADSLget_main_menu, ADSLget_cmd_menu, 
					 NULL, NULL, 2 }, 

#if defined(__LINUX__)
	{ WANCONFIG_SS7,	"ss7",   SS7Config, SS7Usage, SS7Main, SS7DisableTrace, 
		                         SS7get_main_menu, SS7get_cmd_menu, 
					 NULL, NULL, 2 },
	{ WANCONFIG_X25, 	"x25",   X25Config, X25Usage, X25Main, X25DisableTrace, 
					 X25get_main_menu, X25get_cmd_menu , 
					 NULL, NULL, 0}, 
	{ WANCONFIG_BITSTRM, "bitstrm",  BITSTRMConfig, BITSTRMUsage, BITSTRMMain, NULL, 
					 BITSTRMget_main_menu, BITSTRMget_cmd_menu , 
					 NULL, NULL, 0},
#else
	{ WANCONFIG_SS7, 	"ss7",  NULL, NULL, NULL, NULL,NULL,NULL,0 }, 
	{ WANCONFIG_X25, 	"x25",  NULL, NULL, NULL, NULL,NULL,NULL,0 }, 
#endif
	{ WANCONFIG_BSC, 	"bsc",  NULL, NULL, NULL, NULL,NULL,NULL,0 }, 
	{ WANCONFIG_HDLC, 	"hdlc", NULL, NULL, NULL, NULL,NULL,NULL,0 }, 
	{ 0, "N/A", 0, 0, 0, 0,0 }, 
};
#endif

/******************************************************************************
 * 			FUNCTION DEFINITION				      *
 *****************************************************************************/
static int MakeConnection( void )
{
	if (ip_addr == WAN_TRUE){

		sock = socket((af == AF_INET) ? PF_INET : PF_INET6, SOCK_DGRAM, 0);
		if (sock < 0){
			perror("socket");
			return WAN_FALSE;
		} /* if */

		if (af == AF_INET){
			soin.sin_addr.s_addr = inet_addr(ipaddress);
			soin.sin_family = AF_INET;
			soin.sin_port = htons((u_short)udp_port);
			if (soin.sin_addr.s_addr < 0){
	                	printf("Error: Invalid IP address!\n");
        	        	return WAN_FALSE;
        		}
			if (connect( sock, (struct sockaddr *)&soin, sizeof(soin)) != 0){
				perror("connect");	
				printf("Make sure the IP address is correct\n");
				return( WAN_FALSE );
			} /* if */
     		}else{
#ifdef ENABLE_IPV6
			inet_pton(AF_INET6, ipaddress, &soin6.sin6_addr);
			//soin6.sin6_len = sizeof(struct sockaddr_in6);
			soin6.sin6_family = AF_INET6;
			soin6.sin6_scope_id = 0xc;
			soin6.sin6_port = htons((u_short)udp_port);
			if (connect( sock, (struct sockaddr *)&soin6, sizeof(soin6)) != 0){
				perror("connect");
				printf("Make sure the IP address is correct\n");
				return WAN_FALSE;
			} /* if */
#else
			printf("Ipv6 protocol not supported!\n");
			return WAN_FALSE;
#endif

		}
	}else{
	    	sock = socket((af == AF_INET) ? PF_INET : PF_INET6, SOCK_STREAM, IPPROTO_IP);
	    	if (sock < 0){
			perror("socket");
			return WAN_FALSE;
    		}

	}

	if (!GetWANConfig()){
		printf("Error: Unable to obtain network interface information.\n");
		printf("Make sure the IP and UDP port are correct.\n");
		close(sock);
		return WAN_FALSE;
	}   

	return WAN_TRUE;
} /* MakeConnection */

static int GetWANConfig( void )
{
	int cnt = 0, err = 0;

	if (wan_protocol != 0){

		/* If the user specified protocol, do not try to
	 	* autodetect it.  Old drivers do not have protocol
	 	* detect option. */
		goto skip_protocol_detect;
	}
	
	/* Get Protocol type */ 
	wan_udp.wan_udphdr_command	= WAN_GET_PROTOCOL;
	wan_udp.wan_udphdr_data_len 	= 0;
	wan_udp.wan_udphdr_return_code 	= WAN_UDP_TIMEOUT_CMD;
	while (DO_COMMAND(wan_udp) < 0 && ++cnt < 4) {
		if (wan_udp.wan_udphdr_return_code == WAN_UDP_TIMEOUT_CMD) {
			printf("Error: Command timeout occurred\n"); 
			return WAN_FALSE;
		}
		if (wan_udp.wan_udphdr_return_code == 0xCC ) 
			return WAN_FALSE;
	}

	if (cnt >= 4){
		return WAN_FALSE;
	}


	wan_protocol = wan_udp.wan_udphdr_data[0];


skip_protocol_detect:
#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
	printf("Network interface running %s protocol...\n",
				DECODE_PROT(wan_protocol));
#endif

	EXEC_PROT_FUNC(config,wan_protocol,err,());
	return err;
} 

int DoCommand(wan_udp_hdr_t* wan_udp)
{
        static unsigned char id = 0;
        fd_set ready;
        struct timeval to;
	int data_len = protocol_cb_size+wan_udp->wan_udphdr_data_len;
        int x, err=0;

	if (ip_addr == WAN_TRUE){
		for( x = 0; x < 4; x += 1 ) {
			wan_udp->wan_udphdr_request_reply = 0x01;
			wan_udp->wan_udphdr_id = id;
			/* 0xAA is our special return code indicating packet timeout */
			//wan_udp.wan_udphdr_return_code = WAN_UDP_TIMEOUT_CMD;
			//err = send(sock, &wan_udp. 35 + cb.cblock.buffer_length, 0);
			err = send(sock, (caddr_t)wan_udp, data_len, 0); 
			if (err < 0){
				perror("Send: ");	
				continue;
			}
			
			FD_ZERO(&ready);
			FD_SET(sock,&ready);
			to.tv_sec = 5;
			to.tv_usec = 0;
	
			if((err = select(sock + 1,&ready, NULL, NULL, &to))){
				err = recv(sock, (caddr_t)wan_udp, sizeof(wan_udp_hdr_t), 0);
				if (err < 0){
					perror("Recv: ");
				}
        	               	break;
	               	}else{
				printf("Error: No Data received from the connected socket.\n");
               		}
		}
	}else{
		struct ifreq ifr;
		wan_udp->wan_udphdr_request_reply = 0x01;
		wan_udp->wan_udphdr_id = id;
		strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));
		ifr.ifr_data = (caddr_t)wan_udp;

        	if ((err=ioctl(sock, SIOC_WANPIPE_MONITOR , &ifr)) < 0){
			perror("Ioctl: ");
			return err;
    		}
	}

#if 0
	{
		int i;
		printf("Rx Packet :\n");
		for (i=0;i<sizeof(wan_udp);i++){
			printf("%x ",*((unsigned char *)wan_udp+i));
		}
		printf("\n");
	}
#endif
        return err;

} /* DoCommand */

static int init(int argc, char *argv[], char* command, int cmd_len)
{
	int i = 0, i_cnt = 0, u_cnt = 0, c_cnt = 0, d_cnt = 0;
	struct in_addr *ip_str = NULL;
#ifdef ENABLE_IPV6
	struct in6_addr ip6_str;
#endif

	for (i = 0; i < argc; i++){

		
		if (!strcmp(argv[i],"-h")){
#if defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
			EXEC_NAME_FUNC(usage,"hdlc",());
#else
			usage_long();
#endif
			return WAN_FALSE;

#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
		}else if (!strcmp(argv[i],"-traceinfo")){
			usage_trace_info();
			return WAN_FALSE;
#endif			
		}else if (!strcmp(argv[i],"-i")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid IP or Interface Name!\n");
				printf("Type wanpipemon <cr> for help\n");
				return WAN_FALSE;
			}

			strlcpy(ipaddress,argv[i+1], 16);
			if (inet_aton(ipaddress,ip_str) != 0 ){
				ip_addr = WAN_TRUE;
			}else{
#ifdef ENABLE_IPV6
				if (inet_pton(AF_INET6,ipaddress,(void*)&ip6_str) == 1){
					af = AF_INET6;
					ip_addr = WAN_TRUE;
				}else{		
					ip_addr = WAN_FALSE;
					strncpy(if_name, ipaddress, WAN_IFNAME_SZ);
				}
#else
				ip_addr = WAN_FALSE;
				strncpy(if_name, ipaddress, WAN_IFNAME_SZ);
#endif
			}
			i_cnt=1;

		}else if (!strcmp(argv[i],"-g")){
#ifdef WANPIPEMON_GUI			
			gui_interface=1;
			trace_all_data=1;
			return WAN_TRUE;
#else
			printf("wanpipemon: Warning GUI interface not compiled in!\n\n");
#endif			
		}else if (!strcmp(argv[i],"-u")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid UDP PORT!\n");
				printf("Type wanpipemon <cr> for help\n");
				return WAN_FALSE;
			}

		 	if(isdigit(argv[i+1][0])){
				udp_port = atoi(argv[i+1]);
			}else{
				printf("ERROR: Invalid UDP Port!\n");
				printf("Type wanpipemon <cr> for help\n");
				return WAN_FALSE;
			}
			u_cnt=1;
		}else if (!strcmp(argv[i],"-c")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid Command!\n");
				printf("Type wanpipemon <cr> for help\n");
				return WAN_FALSE;
			}

			strlcpy(command,argv[i+1],cmd_len);
			c_cnt=1;
		}else if (!strcmp(argv[i], "-d")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid Command!\n");
				printf("Type wanpipemon <cr> for help\n\n");
				return WAN_FALSE;
			}
			if (isdigit(argv[i+1][0])){
				dlci_number = atoi(argv[i+1]);
				TRACE_DLCI=dlci_number;
			}else{
				printf("ERROR: DLCI must be an integer!\n");
				printf("Type wanpipemon <cr> for help\n");
				return WAN_FALSE;
			} 
			d_cnt=1;
		
		}else if (!strcmp(argv[i],"-pv6")){
			af = AF_INET6;	// IPV6

		}else if (!strcmp(argv[i],"-p")){

			if (i+1 > argc-1){
				printf("ERROR: Invalid Command!\n");
				printf("Type wanpipemon <cr> for help\n");
				return WAN_FALSE;
			}

			if (argc == 3){
				char *name=argv[i+1];
				EXEC_NAME_FUNC(usage,name,());
				return WAN_FALSE;
			}

			if (strcmp(argv[i+1], "chdlc") == 0){
				strlcpy((char*)wan_udp.wan_udphdr_signature, UDP_CHDLC_SIGNATURE, 8);
				wan_protocol=WANCONFIG_CHDLC;
			}else if (strcmp(argv[i+1], "fr") == 0){
				strlcpy((char*)wan_udp.wan_udphdr_signature, UDP_FR_SIGNATURE,8);
				wan_protocol=WANCONFIG_FR;
			}else if (strcmp(argv[i+1], "ppp") == 0){
				strlcpy((char*)wan_udp.wan_udphdr_signature, UDP_PPP_SIGNATURE,8);
				wan_protocol=WANCONFIG_PPP;
			}else if (strcmp(argv[i+1], "x25") == 0){
				strlcpy((char*)wan_udp.wan_udphdr_signature, UDP_X25_SIGNATURE,8);
				wan_protocol=WANCONFIG_X25;
			}else if (strcmp(argv[i+1], "adsl") == 0){
				strlcpy((char*)wan_udp.wan_udphdr_signature, GLOBAL_UDP_SIGNATURE,8);
				wan_protocol=WANCONFIG_ADSL;
			}else{
				usage();
				return WAN_FALSE;
			}		
		
		}else if (!strcmp(argv[i], "-x")){
			xml_output=1;

		}else if (!strcmp(argv[i], "-full")){
			trace_all_data=1;
		
		}else if (!strcmp(argv[i], "-systime")){
			sys_timestamp=1;

		}else if (!strcmp(argv[i], "-prot")){
			int x;
			if (i+1 > argc-1){
				printf("ERROR: Invalid Command -prot\n");
				printf("ERROR: Type %s <cr> for help\n\n",
						progname);
				exit(1);
			}
			TRACE_PROTOCOL=0;
			for (x=0;;x++){
				if (trace_prot_opt[x].prot_index == -1)
					break;
			
				if (strstr(argv[i+1],(char*)trace_prot_opt[x].prot_name) != NULL){
					TRACE_PROTOCOL|=trace_prot_opt[x].prot_index;
					pcap_prot=trace_prot_opt[x].pcap_prot;
				}
			}

			if (!TRACE_PROTOCOL){
				printf("ERROR: Invalid Protocol in Command -prot!\n");
				printf("ERROR: Type %s <cr> for help\n\n",
						progname);
				exit(1);
			}
		}else if (!strcmp(argv[i], "-lcn")){
			if (i+1 > argc-1){
				printf("ERROR: Invalid Command -lcn, Type %s <cr> for help\n\n", progname);
				exit(1);
			}

			if (isdigit(argv[i+1][0])){
				TRACE_X25_LCN=atoi(argv[i+1]);
			}else{
				printf("ERROR: LCN must be an integer,Type %s <cr> for help\n\n", progname);
				exit(0);
			} 

		}else if (!strcmp(argv[i], "-ascii")){

			TRACE_ASCII=1;
			
		}else if (!strcmp(argv[i], "-ebcdic")){

			TRACE_EBCDIC=1;

		}else if (!strcmp(argv[i], "-hex")){

			TRACE_HEX=1;

		}else if (!strcmp(argv[i], "-pcap")){

			pcap_output=1;
		
		}else if (!strcmp(argv[i], "-pcap_file")){	
			
			if (i+1 > argc-1){
				printf("ERROR: Invalid Command -pcap_file, Type %s <cr> for help\n\n", progname);
				exit(1);
			}

			strncpy(pcap_output_file_name, argv[i+1], WAN_IFNAME_SZ);
			
		}else if (!strcmp(argv[i], "-x25opt")){
			int x;
			if (i+1 > argc-1){
				printf("ERROR: Invalid Command -x25prot, Type %s <cr> for help\n\n", progname);
				exit(1);
			}
		
			TRACE_X25_OPT=0;
			for (x=0;;x++){
				if (trace_x25_prot_opt[x].prot_index == -1)
					break;
				
				if (strstr(argv[i+1],(char*)trace_x25_prot_opt[x].prot_name) != NULL){
					TRACE_X25_OPT|=trace_x25_prot_opt[x].prot_index;
				}
			}

			if (!TRACE_X25_OPT){
				printf("ERROR: Invalid X25 Option in Command -x25opt, Type %s <cr> for help\n\n",
					       progname);
				exit(1);
			}
		}
	}

	if (!i_cnt){
		printf("ERROR: No IP address or Interface Name!\n");
		printf("Type %s <cr> for help\n", progname);
		return WAN_FALSE;
	}
	if (!u_cnt && ip_addr == WAN_TRUE){
		printf("ERROR: No UDP Port!\n");
		printf("Type %s <cr> for help\n", progname);
		return WAN_FALSE;
	}
	if (!c_cnt){
		printf("ERROR: No Command!\n");
		printf("Type %s <cr> for help\n", progname);
		return WAN_FALSE;
	}
	if (!d_cnt){
		/* Default DLCI Number to 0 */
		dlci_number = 0;
	}

	return WAN_TRUE;
}

void ResetWanUdp(wan_udp_hdr_t *wan)
{
	memset((void *)&wan->wan_cmd, 0, sizeof(wan_cmd_t));
} 

void banner (char *title,int dlci_number){
	
	int len,i;
	
	len = strlen(title) + strlen(if_name);
	printf("\n\t");
	for (i=0;i<(len+16);i++)
		printf("-");
	printf("\n\t\t%s: %s", if_name, title);
	printf("\n\t");
	for (i=0;i<(len+16);i++)
		printf("-");
	printf("\n\n");

}


#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
static unsigned char usage_info[]="\n"
"Wanpipemon Verison 1.0\n"
"Copyright (C) 2002 - Sangoma Technologies\n"
"www.sangoma.com\n"
"This may be freely redistributed under the terms of the GNU GPL\n"
"	\n"
"Usage: 	(Options in {} brackets are optional)\n"
"\n"
"	#Command prompt Local debugging\n"
"	wanpipemon -i <interface> -c <command> { -p <protocol> }\n"
"	\n"
"	#Command prompt Remote debugging\n"
"	wanpipemon -i <remote IP> -u <port> -c <command> { -p <protocol> }\n"
"\n"
"	#GUI interface\n"
"	wanpipemon -g { -i <interface|IP> -u <port> -p <protocol> }  \n"
"	\n"
"	#Display usage for each protocol\n"
"	wanpipemon -p [fr|ppp|chdlc|x25|adsl|aft]			\n"
"\n"
"	#Detailed usage for each option\n"
"	wanpipemon -h\n"
"\n"
"	#Detailed usage for advanced tracing\n"
"	wanpipemon -traceinfo\n";
#else
static unsigned char usage_info[]="\n"
"Wanutil Verison 1.0\n"
"Copyright (C) 2004 - Sangoma Technologies\n"
"www.sangoma.com\n"
"This may be freely redistributed under the terms of the GNU GPL\n"
"	\n"
"Usage: 	(Options in {} brackets are optional)\n"
"\n"
"	#Command prompt Local debugging\n"
"	wanutil -i <interface> -c <command>\n"
"	\n"
"	#Command prompt Remote debugging\n"
"	wanutil -i <remote IP> -u <port> -c <command>\n"
"\n"
"	#Detailed usage for each option\n"
"	wanutil -h\n"
"\n"
"	#Detailed usage for advanced tracing\n"
"	wanutil -traceinfo\n";
#endif


static void usage(void)
{
	printf("%s\n", usage_info);
}


#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
static unsigned char usage_long_buf[]= "\n"
"\n"
"Wanpipemon Verison 1.0\n"
"Copyright (C) 2002 - Sangoma Technologies\n"
"www.sangoma.com\n"
"This may be freely redistributed under the terms of the GNU GPL\n"
"\n"
"Usage: 	(Options in {} brackets are optional)\n"
"\n"
"	#Command prompt Local debugging\n"
"	wanpipemon -i <interface> -c <command> { -p <protocol> }\n"
"	\n"
"	#Command prompt Remote debugging\n"
"	wanpipemon -i <remote IP> -u <port> -c <command> { -p <protocol> }\n"
"\n"
"	#GUI interface\n"
"	wanpipemon -g { -i <interface|IP> -u <port> -p <protocol> }  \n"
"	\n"
"	#Display usage for each protocol\n"
"	wanpipemon -p [fr|ppp|chdlc|x25|adsl|aft]			\n"
"\n"
"	#Detailed usage for each option\n"
"	wanpipemon -h\n"
"\n"
"-----------------------------------------------------------\n"
"Option -p: \n"
"\n"
"    wanpipemon -p [fr|ppp|chdlc|x25|adsl|aft]\n"
"\n"
"	When used by itself:\n"
"		Displays the command list for each protocol\n"
"\n"
"    wanpipemon -p fr -i wp1_fr16 -c xm	\n"
"	\n"
"	When used with other commands:\n"
"		Specifies the protocol and skips the protocol\n"
"		autodetect feature.  \n"
"		\n"
"		Used for backward compatibility, since\n"
"		old drivers do not support protocol detect option.\n"
"\n"
"-----------------------------------------------------------		\n"
"Option 	-g:\n"
"\n"
"   Start wanpipemon in GUI mode\n"
"\n"
"   wanpipemon -g\n"
"   \n"
"	For local debugging: \n"
"		GUI will display list of active interfaces.\n"
"			\n"
"	For remote debugging:\n"
"		GUI will ask for an IP address and UDP port.\n"
"	\n"
"   wanpipemon -i wp1_fr16 -g\n"
"   \n"
"   	Specifies the interface, do not show the list of interfaces\n"
"\n"
"   wanpipemon -p fr -i wp1_fr16 -g\n"
"        \n"
"   	Specify the interface and protocol. \n"
"	\n"
"-----------------------------------------------------------\n"
"Option -i: \n"
"\n"
"   wanpipemon -i wp1_fr16\n"
"   wanpipemon -i 192.168.1.1\n"
"\n"
"	Specify the Local interface name or Remote IP\n"
"	address which will be used to connect to the driver.\n"
"   \n"
"	\n"
"-----------------------------------------------------------\n"
"Option -u: \n"
"\n"
"   wanpiemon -i 192.168.1.1 -u 9000 -c xm\n"
"\n"
"   	Wanpipe UDPPORT specified in /etc/wanpipe/wanpipe?.conf\n"
"	Note: This option is only valid when using Remote IP address.\n"
"\n"
"\n"
"-----------------------------------------------------------\n"
"Option -c: \n"
"\n"
"	Wanpipe monitor command (for list of command per protocol)\n"
"	For list of commands run: \n"
"		wanpipemon -p [fr|ppp|chdlc|x25|adsl|aft]\n"
"\n"
"\n";

static void usage_long(void)
{
	printf("%s\n",usage_long_buf);
}
#endif


#if !defined(CONFIG_PRODUCT_WANPIPE_GENERIC)
static unsigned char trace_info[]="\n"
"\n"
"Wanpipemon Verison 1.1\n"
"Copyright (C) 2004 - Sangoma Technologies\n"
"www.sangoma.com\n"
"This may be freely redistributed under the terms of the GNU GPL\n"
"	\n"
"Advanced Line Trace Options\n"
"===========================\n"
"\n"
"Advanced trace options include protocol decoding\n"
"and parsing, system timestamping as well as\n"
"packet filtering.\n"
"\n"
"The Advanced trace command is: 'tri'\n"
"	\n"
"	wanpipemon -i <ifname> -c tri { trace options }\n"
"\n"
"\n"
"trace options:\n"
"-------------	\n"
"\n"
"	-prot 	[FR|LAPB|X25]	#Filter packets based on\n"
"				#protocol. Multiple protocols can\n"
"				#be selected:  \n"
"				# <prot>-<prot>...\n"
"				# eg: -prot LAPB-X25\n"
"				#Default: All frames\n"
"\n"
"		[FR|PPP|CHDLC|IP|ETH|LAPB|X25]\n"
"				#Also used by -pcap option to\n"
"				#specify what protocol we are\n"
"				#capturing. By default protocol is\n"
"				#autodetected, but in datascoping\n"
"				#this option is a must.\n"
"\n"
"	-pcap\n"
"				#Trace to a pcap type file\n"
"				#that can be read by Ethereal\n"
"				#By default file name is wp_trace_pcap.bin\n"
"				#writen in current/local directory\n"
"\n"
"	-pcap_file <filename>\n"
"\n"
"				#Specify your own pcap file name\n"
"\n"
"	-x25opt [DATA|PROT]	#Filter x25 packets based on \n"
"				#protcol or information frames\n"
"				#Default: All frames\n"
"\n"
"	-lcn	<number>	#Filter x25 packets based on\n"
"	                        #specific lcn number\n"
"	                        #Default: All lcns\n"
"\n"
"	-hex			#Display packet info in HEX \n"
"                                #Default: Hex\n"
"\n"
"	-ascii			#Display packet info in ASCII\n"
"\n"
"	-ebcdic			#Display packet info in EBCDIC	\n"
"\n"
"	-systime		#Display timestamp as system time\n"
"	                        #instead of absolute number\n"
"	\n"
"	-full			#Display packet data in full.\n"
"\n"
"Examples:\n"
"---------\n"
"\n"
"#Trace and decode all frames, and display packets\n"
"#in full with timestampe decoded into system time.\n"
"\n"
"  wanpipemon -i wp1mp -c tri -full -systime\n"
"\n"
"#Trace LAPB and X25 protocol frames. Futhtermore, \n"
"#only decode x25 frames with LCN=1\n"
"\n"
"  wanpipemon -i wp1mp -c tri -prot LAPB-X25 -lcn 1\n"
"\n"
"#Trace X25 protocol frames and display x25 data\n"
"#in ASCII.\n"
"\n"
"  wanpipemon -i wp1mp -c tri -prot X25 -ascii -full -systime \n"
"\n"
"#Trace data to a pcap type file\n"
"\n"
"  wanpipemon -i wan0 -pcap -c tr\n"
"  wanpipemon -i wan0 -pcap -pcap_file myfile.bin -c tr\n"
"  wanpipemon -i wan0 -pcap -prot FRAME -c tr\n\n";

static void usage_trace_info(void)
{
	printf("%s\n",trace_info);
}
#endif

#if 0
static void sig_handler(int sigint)
{
	int err;
	EXEC_PROT_FUNC(disable_trace,wan_protocol,err,());
}
#endif

void sig_end(int signal)
{
	if (pcap_output_file){
		fclose(pcap_output_file);
		pcap_output_file=NULL;
	}

	if (sock)
		close(sock);

	//printf("\n\nSignal: Terminating wanpipemon\n");

	exit(0);
}


int main(int argc, char* argv[])
{
	char command[6];

	strlcpy((char*)wan_udp.wan_udphdr_signature, GLOBAL_UDP_SIGNATURE, 8);
	snprintf(pcap_output_file_name,50,"wp_trace_pcap.bin");

	signal(SIGHUP,sig_end);
	signal(SIGINT,sig_end);
	signal(SIGTERM,sig_end);
	
  	printf("\n");
   	if (argc >= 2){
		int err=0;
    
		if (init(argc, argv, command, 6) == WAN_FALSE){
			return -EINVAL;		
		}

		if (pcap_output){

			if (pcap_prot){
				printf("Using custom pcap_prot=%d\n",pcap_prot);
			}
			
			unlink(pcap_output_file_name);
			pcap_output_file=fopen(pcap_output_file_name,"wb");
			if (!pcap_output_file){
				printf("wanpipemon: Failed to open %s binary file!\n",
						pcap_output_file_name);
				return -EINVAL;
			}
		}

#ifdef WANPIPEMON_GUI
		if (gui_interface && ip_addr==-1){
			if (wan_main_gui() == WAN_FALSE){
				return -EINVAL;
			}
		}
gui_loop:		
#endif
		if (MakeConnection() == WAN_FALSE){ 
			close(sock);
			return -ENODEV;	
		}

#ifdef WANPIPEMON_GUI
		if (gui_interface && wan_protocol != 0){
			if (wan_main_gui() == WAN_FALSE){
				return 0;
			}
			wan_protocol=0;
			goto gui_loop;		
		}
#endif
		EXEC_PROT_FUNC(main,wan_protocol,err,(command,argc,argv));
     		close(sock);

		if (pcap_output_file){
			fclose(pcap_output_file);
			pcap_output_file=NULL;
		}
   	}else{
    		usage();
   	}
  
	printf("\n");
   	return 0;
}


