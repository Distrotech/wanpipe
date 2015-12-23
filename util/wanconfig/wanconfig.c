/*****************************************************************************
* wanconfig.c	WAN Multiprotocol Router Configuration Utility.
*
* Author:	Nenad Corbic	<ncorbic@sangoma.com>
*               Gideon Hack
*
* Copyright:	(c) 1995-2003 Sangoma Technologies Inc.
*
*		This program is free software; you can redistribute it and/or
*		modify it under the terms of the GNU General Public License
*		as published by the Free Software Foundation; either version
*		2 of the License, or (at your option) any later version.
* ----------------------------------------------------------------------------
* Jan 11, 2010  David Rokhvarg	Moved data type declarations and lookup tables
*								to wanconfig.h.
*								Moved code for configuration of hardware echo
*								canceller to wanconfig_hwec.c.
*								Both changes needed for code re-use in
*								cross-platform software (Linux:wanconfig;
*								Windows:wanrouter.exe).
* Sep 25, 2007  Alex Feldman	Verify return code from wanec client.
* Jun 25, 2007  David Rokhvarg  Added support for AFT-ISDN BRI (A500) card.
* Mar 29, 2007  David Rokhvarg  Added support for AFT-56K card.
* May 11, 2001  Alex Feldman    Added T1/E1 support (TE1).
* Apr 16, 2001  David Rokhvarg  Added X25_SRC_ADDR and ACCEPT_CALLS_FROM to 'chan_conftab'
* Nov 15, 2000  Nenad Corbic    Added true interface encoding type option
* Arp 13, 2000  Nenad Corbic	Added the command line argument, startup support.
*                               Thus, one can configure the router using
*                               the command line arguments.
* Jan 28, 2000  Nenad Corbic    Added support for ASYCN protocol.
* Sep 23, 1999  Nenad Corbic    Added support for HDLC STREAMING, Primary
*                               and secondary ports. 
* Jun 02, 1999 	Gideon Hack	Added support for the S514 PCI adapter.
* Jan 07, 1998	Jaspreet Singh	Made changes for 2.1.X.
*				Added support for WANPIPE and API integration.
* Jul 20, 1998	David Fong	Added Inverse ARP option to channel config.
* Jun 26, 1998	David Fong	Added IP_MODE to PPP Configuration.
*				Used for Dynamic IP Assignment.
* Jun 18, 1998	David Fong	Added Cisco HDLC definitions and structures
* Dec 16, 1997	Jaspreet Singh 	Moved IPX and NETWORK to 'chan_conftab'
* Dec 08, 1997	Jaspreet Singh	Added USERID, PASSWD and SYSNAME in 
*				'chan_conftab' 
* Dec 05, 1997	Jaspreet Singh  Added PAP and CHAP in 'chan_conftab'
*				Added AUTHENTICATOR in 'ppp_conftab' 
* Oct 12, 1997	Jaspreet Singh	Added IPX and NETWORK to 'common_conftab'
*				Added MULTICAST to 'chan_conftab'
* Oct 02, 1997  Jaspreet Singh	Took out DLCI from 'fr_conftab'. Made changes 
*				so that a list of DLCI is prepared to 
*				configuring them when emulating a NODE 
* Jul 07, 1997	Jaspreet Singh	Added 'ttl' to 'common_conftab'
* Apr 25, 1997  Farhan Thawar   Added 'udp_port' to 'common_conftab'
* Jan 06, 1997	Gene Kozin	Initial version based on WANPIPE configurator.
*****************************************************************************/

/*****************************************************************************
* Usage:
*   wanconfig [-f {conf_file}]	Configure WAN links and interfaces
*   wanconfig -d {device}	Shut down WAN link
*   wanconfig -h|?		Display help screen
*
* Where:
*   {conf_file}		configuration file name. Default is /etc/wanpipe1.conf
*   {device}		name of the WAN device in /proc/net/wanrouter directory
*
* Optional switches:
*   -v			verbose mode
*   -h or -?		display help screen
*****************************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "lib/safe-read.h"
#include "wanpipe_version.h"
#include "wanpipe_defines.h"
#include "wanpipe_cfg.h"
#include "wanproc.h"

#include "wanpipe_events.h"
#include "wanec_api.h"

#include "wanpipe.h"
#include "wanconfig.h"

#define smemof(TYPE, MEMBER) offsetof(TYPE,MEMBER),(sizeof(((TYPE *)0)->MEMBER))



/****** Defines *************************************************************/

#ifndef	min
#define	min(a,b)	(((a)<(b))?(a):(b))
#endif

#define	is_digit(ch) (((ch)>=(unsigned)'0'&&(ch)<=(unsigned)'9')?1:0)

#define show_error(x) { show_error_dbg(x, __LINE__);}

/****** Function Prototypes *************************************************/

int arg_proc (int argc, char* argv[]);
void show_error_dbg	(int err, int line);
int startup(void);
int wp_shutdown (void);
int configure (void);
int parse_conf_file (char* fname);
int build_linkdef_list (FILE* file);
int build_chandef_list (FILE* file);
char* read_conf_section (FILE* file, char* section);
int read_conf_record (FILE* file, char* key, int max_len);
int configure_link (link_def_t* def, char init);
int configure_chan (int dev, chan_def_t* def, char init, int id);
int set_conf_param (char* key, char* val, key_word_t* dtab, void* conf, int max_len);
void init_first_time_tokens(char **token);
void init_tokens(char **token);
int tokenize (char* str, char **tokens);
char* str_strip (char* str, char* s);
char* strupcase	(char* str);
void* lookup (int val, look_up_t* table);
int name2val (char* name, look_up_t* table);
int read_data_file (char* name, data_buf_t* databuf);
int read_oct_chan_config(char*, char*, wan_custom_conf_t *conf);
unsigned long filesize (FILE* file);
unsigned int dec_to_uint (char* str, int len);
unsigned int get_config_data (int, char**);
void show_help(void);
void show_usage(void);
void set_action(int new_action);
int gencat (char *filename); 

int show_status(void);
int show_config(void);
int show_hwprobe(int);
int debugging(void);
int debug_read(void);


int start_daemon(void);
int exec_link_cmd(int dev, link_def_t *def);
int exec_chan_cmd(int dev, chan_def_t *def);
void free_linkdefs(void);


/* Parsing the command line arguments, in order
 * to starting WANPIPE from the command line. 
 */ 
int get_devices (FILE *fp, int *argc, char ***argv, char**);
int get_interfaces (FILE *fp, int *argnum, char ***argv_ptr, char *device_name);
int get_hardware (FILE *fp, int *argc_ptr, char ***argv_ptr, char *device_name);
int get_intr_setup (FILE *fp, int *argc_ptr, char ***argv_ptr);
int router_down (char *devname, int ignore_error);
int router_ifdel (char *card_name, char *dev_name);
int conf_file_down (void);


unsigned int get_active_channels(int channel_flag, int start_channel, int stop_channel);

void read_adsl_vci_vpi_list(wan_adsl_vcivpi_t* vcivpi_list, unsigned short* vcivpi_num);
void update_adsl_vci_vpi_list(wan_adsl_vcivpi_t* vcivpi_list, unsigned short vcivpi_num);

void wakeup_java_ui(void);

extern	int close (int);

char *time_string(time_t t,char *buf);
int has_config_changed(link_def_t *linkdef, char *name);
void free_device_link(char *devname);
int device_syncup(char *devname);
void sig_func (int sigio);
int start_chan (int dev, link_def_t *def);
int start_link (void);
int stop_link(void);
int exec_command(char *rx_data);
void catch_signal(int signum,int nomask);

/****** Global Data *********************************************************/

char master_lapb_dev[WAN_DRVNAME_SZ+1];
char master_x25_dev[WAN_DRVNAME_SZ+1];
char master_dsp_dev[WAN_DRVNAME_SZ+1];
char master_lip_dev[WAN_DRVNAME_SZ+1];

#define MAX_WAKEUI_WAIT 2
static char wakeup_ui=0;
static time_t time_ui=0;

#define MAX_FIRMW_SIZE	40000
static unsigned char firmware_file_buffer[MAX_FIRMW_SIZE];

char prognamed[20] =	"wanconfig";
char progname_sp[] = 	"         ";
char def_conf_file[] =	"/etc/wanpipe/wanpipe1.conf";	/* default name */
char def_adsl_file[] =	"/etc/wanpipe/wan_adsl.list";	/* default name */
char tmp_adsl_file[] =	"/etc/wanpipe/wan_adsl.tmp";	/* default name */
#if defined(__LINUX__)
char router_dir[] =	"/proc/net/wanrouter";	/* location of WAN devices */
#else
char router_dir[] =	"/var/lock/wanrouter";	/* location of WAN devices */
#endif
char conf_dir[] =	"/etc/wanpipe";
char banner[] =		"WAN Router Configurator"
			"(c) 1995-2003 Sangoma Technologies Inc.";

static char wan_version[100];


char usagetext[] = 
	"\n"
	"  wanconfig: Wanpipe device driver configuration tool\n\n"
	"  Usage: wanconfig [ -hvw ] [ -f <config-file> ] [ -U {arg options} ]\n"
       	"                   [ -y <verbose-log> ] [ -z <kernel-log> ]\n"
	"                   [ card <wan-device-name> [ dev <dev-name> | nodev ] ]\n"
	"                   [ help | start | stop | up | down | add | del | reload\n"
	"                     | restart | status | show | config ]\n"
       	"\n";	

char helptext[] =
	"Usage:\n"
	"------\n"
	"\twanconfig [ -hvw ] [ -f <config-file> ] [ -U {arg-options} ]\n"
       	"\t                   [ -y <verbose-log> ] [ -z <kernel-log> ]\n"
	"\t                   [ card <wan-dev-name> [ dev <dev-name> | nodev ] ]\n"
	"\t                   [ help | start | stop | up | down | add | del\n"
       	"\t                     | reload | restart | status | show | config ]\n"
	"\n"
	"\tWhere:\n"
	"\n"
	"\t          [-f {config-file}] Specify an overriding configuration file.\n"
	"\t          [-U {arg-options}] Configure WAN link using command line\n"
	"\t                             arguments.\n"
	"\t          [-v]               Verbose output to stdout.\n"
	"\t          [-h] | help        Show this help.\n"
	"\t          [-w]               Enable extra error messages about debug\n"
	"\t                             log locations.\n"
	"\t          [-y]               Give location of wanconfig verbose log\n"
       	"\t                             for -w switch.\n"
	"\t          [-z]               Give location of syslogd kernel logging\n"
       	"\t                             for -w switch.\n"
	"\t          card <wan-dev-name>\n"
	"\t                             Specify which WAN interface/card to\n"
	"\t                             operate on. Name given in\n"
       	"\t                             /proc/net/wanrouter directory.\n"
	"\t          dev <dev-name>     Specify which linux network interface\n"
	"\t                             to operate on.\n"
	"\t          nodev              Turns off creation of interface(s)\n"
	"\t                             when (re)starting card(s).\n"
	"\t          start | up | add   Configure WAN interface(s)/card(s)\n"
	"\t                             and/or create linux network interface(s).\n"
	"\t          stop | down | del  Shut down WAN interface(s)/card(s) and/or\n"
	"\t                             destroy linux network interface(s).\n"
	"\t          reload | restart   Restart WAN interface(s)/card(s) and/or\n"
       	"\t                             recreate linux network interface(s).\n"
	"\t          status | stat | show\n"
	"\t                             Display status information on all\n"
	"\t                             interfaces/cards or for a specific\n"
       	"\t                             WAN interface/card.\n"
	"\t          config             Display configuration information for all\n"
	"\t                             WAN interfaces/cards.\n"
	"\n"
	"\t{config-file}\tconfiguration file (default is\n"
       	"\t\t\t/etc/wanpipe/wanpipe#.conf or\n"
	"\t\t\t/etc/wanpipe/wanpipe.conf in that order)\n"
	"\t{arg-options}\tare as given below\n"
	"\nArg Options:\n"
	"	[devices] \\ \n"
	" 	 <devname> <protocol> \\ \n"
	"	[interfaces] \\ \n"
	"	 <if_name> <devname> <{addr/-}> <operation_mode> \\ \n"
	"	 <if_name> <devname> <{addr/-}> <operation_mode> \\ \n"
        "	 ...   \\ \n"
	"	[devname] \\ \n"
	"	 <hw_option> <hw_option_value> \\ \n"
	"	 <hw_option> <hw_option_value> \\ \n"
	"	 ...  \\ \n"
	"	[if_name] \\ \n"
	"	 <protocol_opton> <protocol_option_value> \\ \n"
	"	 <protocol_opton> <protocol_option_value> \\ \n"
	"	 ... \\ \n\n"
	"	devname		device name. ex:wanpipe# (#=1..16) \n"
	"	protocol	wan protocol. ex: WAN_FR,WAN_CHDLC,WAN_PPP,WAN_X25 \n"
	"	if_name		interface name. ex: wp1_fr16, wp1_ppp \n"
	"	addr/-		For Frame Relay: DLCI number ex: 16, 25\n"
	"			    X25 PVC :    LCN number ex: 1, 2, 4\n"
	"			    X25 SVC :    X25 address ex: @123, @4343 \n"
	"			For other protocol set to: '-'\n"
	"	operation_mode  Mode of operation: WANPIPE - for routing \n"
	"					   API     - raw api interface\n"
	"			The API only applies to WAN_CHDLC, WAN_FR and \n" 
	"			WAN_X25 protocols.\n"
	"	hw_option\n"
	"	protocol_option \n"
	"			Please refer to sample wanpipe#.conf files in \n"
	"			/etc/wanpipe/samples directory for the\n"
	"			appropriate HW/Protocol optoins. \n" 
	"			ex: ioprot, irq, s514_cpu, pci_slot ... \n"
	"	hw_option_value \n"
	"	protocol_option_value \n"
	"			Value of the above options. Refer to the above \n"
	"			sample files. ex: ioport = '0x360' multicast=YES  \n\n"
	"	Example 1: Bring up wanpipe1 device from the command line\n"
	"	wanconfig -U 	[devices] \\ \n"
        "			wanpipe1 WAN_CHDLC \\ \n"
        "			[interfaces] \\ \n"
        "			wp1_chdlc wanpipe1 - WANPIPE \\ \n" 
        "			[wanpipe1] \\ \n"
        "			IOPORT 0x360 \\ \n"
        "			IRQ 7 \\ \n"
        "			Firmware  /etc/wanpipe/firmware/cdual514.sfm \\ \n"
        "			CommPort PRI \\ \n"
        "			Receive_Only NO \\ \n"
        "			Interface V35 \\ \n"
        "			Clocking External \\ \n"
        "			BaudRate 1600000 \\ \n"
        "			MTU 1500 \\ \n"
        "			UDPPORT 9000 \\ \n" 
        "			TTL 255 \\ \n"
        "			[wp1_chdlc] \\ \n"
        "			MULTICAST  NO \\ \n"
        "			IGNORE_DCD YES \\ \n"
        "			IGNORE_CTS YES \\ \n"
        "			IGNORE_KEEPALIVE YES \\ \n" 
        "			HDLC_STREAMING YES \\ \n"
        "			KEEPALIVE_TX_TIMER 10000 \n\n"   
	"	Example 2: Shutdown wanpipe1 device from the command line \n"
	"	\n\t\t#> wanconfig card wanpipe1 stop\n\n"
	"	Example 3: Create fr17 linux network interface and \n"
	"                  start wanpipe1 device (if not already started)\n"
	"                  from the command line \n"
	"	\n\t\t#> wanconfig card wanpipe1 dev fr17 start\n\n"
	"	wanconfig card wanpipe1 dev fr17 start\n\n"
	"	Example 4: Shutdown all WAN devices from the command line \n"
	"	\n\t\t#> wanconfig stop\n"
	

;
char* err_messages[] =				/* Error messages */
{
	"Invalid command line syntax",		/* ERR_SYNTAX */
	"Invalid configuration file syntax",	/* ERR_CONFIG */
	"Wanpipe module not loaded",		/* ERR_MODULE */
	"Unknown error code",			/* ERR_LIMIT */
};
enum	/* modes */
{
	DO_UNDEF,
	DO_START,
	DO_STOP,
	DO_RESTART,
	DO_HELP,
	DO_ARG_CONFIG,
	DO_ARG_DOWN,
	DO_SHOW_STATUS,
	DO_SHOW_CONFIG,
	DO_SHOW_HWPROBE,
	DO_SHOW_HWPROBE_LEGACY,
	DO_SHOW_HWPROBE_VERBOSE,
	DO_DEBUGGING,
	DO_DEBUG_READ,
} action;				/* what to do */

#define	SLEEP_TIME 1			/* Sleep time after executing ioctl */

int verbose = 0;			/* verbosity level */
char* conf_file = NULL;			/* configuration file */
char* adsl_file = NULL;			/* ADSL VCI/VPI list */
char *dev_name = NULL;
char *card_name = NULL;
static char *command=NULL;
char *krnl_log_file = "/var/log/messages";
char *verbose_log = "/var/log/wanrouter";
int  weanie_flag = 1;
int  nodev_flag = 0;
link_def_t* link_defs;			/* list of WAN link definitions */
union
{
	wandev_conf_t linkconf;		/* link configuration structure */
	wanif_conf_t chanconf;		/* channel configuration structure */
} u;
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
wan_conf_t	config;
#endif


static void SIZEOFASSERT (key_word_t* dtab, int type_size)
{
 	if (dtab->size != type_size) { 
		printf("\n==========CRITICAL ERROR============\n\n");  
		printf("Size Mismatch: Type Size %i != %i\n",dtab->size, type_size); 
		printf("======================================\n\n");  
		fprintf(stderr,"\n==========CRITICAL ERROR============\n\n"); 
	        fprintf(stderr,"Size Mismatch: Type Size %i != %i\n",dtab->size, type_size);
		fprintf(stderr,"======================================\n\n");	
		fprintf(stderr,"Plese email /var/log/wanrouter file to Sangoma Support\n");	
		fprintf(stderr,"Email to techdesk@sangoma.com\n");	
		fprintf(stderr,"======================================\n\n");	
       	}         
}   

void init_first_time_tokens(char **token)
{
	int i;

	for (i=0;i<MAX_TOKENS;i++){
		token[i]=NULL;
	}
}

/****** Entry Point *********************************************************/

int main (int argc, char *argv[])
{
	int err = 0;	/* return code */
	int c;

  	snprintf(wan_version, 100, "%s.%s",
				WANPIPE_VERSION, WANPIPE_SUB_VERSION);


	/* Process command line switches */
	action = DO_SHOW_STATUS;
	while(1) {
		c = getopt(argc, argv, "D:hvwUf:a:y:zd:y:z;V;x:r:");

		if( c == -1)
			break;

		switch(c) {
	

		case 'x':
			return start_daemon();
			
		case 'd':
			conf_file = optarg;
			set_action(DO_STOP);
			break;

		case 'a':
			adsl_file = optarg;
			break;

		case 'f':
			conf_file = optarg;
			set_action(DO_START);
			break;

		case 'v':
			verbose = 1;
			break;
		
		case 'w':
			weanie_flag = 1;
			break;
			
		case 'U':
			set_action(DO_ARG_CONFIG);
			break;
				
		case 'y':
			verbose_log = optarg;
			break;
			
		case 'z':
			krnl_log_file = optarg;
			break;
			
		case 'h':
			show_help();
			break;

		case 'D':
			dev_name = optarg;
			set_action(DO_DEBUGGING);
			break;

		case 'r':
			dev_name = optarg;
			set_action(DO_DEBUG_READ);
			break;
			
		case 'V':

				printf("wanconfig: %s-%s %s %s\n",
					WANPIPE_VERSION, WANPIPE_SUB_VERSION,
					WANPIPE_COPYRIGHT_DATES,WANPIPE_COMPANY);

			printf("\n");

			show_usage();
			break;

		case '?':
		case ':':
		default:
			show_usage();
			break;
		}	
	}
	
	/* Check that the no action is given with non-swtiched arguments 
	 */
	
	if( action != DO_SHOW_STATUS && action != DO_ARG_CONFIG ) {
		if(optind < argc)
			show_usage();
		
	}

	/* Process interface control command arguments */
	if( action == DO_SHOW_STATUS && optind  < argc ) {
		
		for( ; optind < argc; optind++ ){
			
			if( strcmp( argv[optind], "wan" ) == 0 \
			 	|| strcmp( argv[optind], "card") == 0 ) {
				
				/* Check that card name arg is given */
				if((optind+1) >= argc ) show_usage();
			
				card_name = argv[optind+1];
				optind++;
			}
			
			else if( strcmp( argv[optind], "dev" ) == 0 ) {
				
				/* Make sure nodev argument is not given */
				if(nodev_flag) show_usage();
				
				/* Make sure card argument is given */
				if(card_name == NULL) show_usage();
				
				/* Check that interface name arg is given */
				if((optind+1) >= argc ) show_usage(); 

				dev_name = argv[optind+1];
				optind++;
			}
			
			else if( strcmp( argv[optind], "nodev" ) == 0 ) {
				
				/* Make sure dev argument is not given */
				if(dev_name != NULL) show_usage();
				
				nodev_flag = 1;
			}
			
			else if( strcmp( argv[optind], "start" ) == 0 
				|| strcmp( argv[optind], "up") == 0
				|| strcmp( argv[optind], "add") == 0 ) {
				set_action(DO_START);
			}
			
			else if( strcmp( argv[optind], "stop" ) == 0
				|| strcmp( argv[optind], "del" ) == 0	
				|| strcmp( argv[optind], "delete" ) == 0	
				|| strcmp( argv[optind], "down") == 0 ) {
				set_action(DO_STOP);
			}
			
			else if( strcmp( argv[optind], "restart" ) == 0 
				|| strcmp( argv[optind], "reload") == 0 ) {
				set_action(DO_RESTART);
			}
			
			else if( strcmp( argv[optind], "status" ) == 0
				|| strcmp( argv[optind], "stat" ) == 0
				|| strcmp( argv[optind], "show" ) == 0 ) {
				set_action(DO_SHOW_STATUS);
			}
			
			else if( strcmp( argv[optind], "config" ) == 0 ) {
				set_action(DO_SHOW_CONFIG);
			}
		
			else if( strcmp( argv[optind], "hwprobe" ) == 0 ) {
				if ((optind + 1 < argc) && (strcmp( argv[optind+1], "verbose" ) == 0 )){
					set_action(DO_SHOW_HWPROBE_VERBOSE);
					optind++;
				}else if ((optind + 1 < argc) && (strcmp( argv[optind+1], "legacy" ) == 0 )){
					set_action(DO_SHOW_HWPROBE_LEGACY);
					optind++;
				}else{
					set_action(DO_SHOW_HWPROBE);
				}
			}
		
			else if( strcmp( argv[optind], "help" ) == 0 ) {
				show_help();
			}
			
			else {
				show_usage();
			}
			
		}
	}

	/* Check length of names */
	if( card_name != NULL && strlen(card_name) > WAN_DRVNAME_SZ) {
	       	fprintf(stderr, "%s: WAN device names must be %d characters long or less\n",
			prognamed, WAN_DRVNAME_SZ);
		exit(1);
	}
		
	if( dev_name != NULL && strlen(dev_name) > WAN_IFNAME_SZ) {
	       	fprintf(stderr, "%s: Interface names must be %d characters long or less\n",
			prognamed, WAN_IFNAME_SZ);
		exit(1);
	}
 		
	/* Perform requested action */
	if (verbose) puts(banner);
	if (!err)
	switch (action) {

	case DO_START:
		err = startup();
		break;

	case DO_STOP:
		err = wp_shutdown();
		break;

	case DO_RESTART:
		err = wp_shutdown();
		if(err)
			break;
		sleep(SLEEP_TIME);
		err = startup();
		break;
		
	case DO_ARG_CONFIG:
		conf_file = "wanpipe_temp.conf";
		argc = argc - optind;
		argv = &argv[optind];
		err = get_config_data(argc,argv);
		if (err)
			break;
		err = configure();
		remove(conf_file);
		break;

	case DO_ARG_DOWN:
		printf("DO_ARG_DOWN\n");
		wp_shutdown();
		break;

	case DO_HELP:
		show_help();
		break;
	
	case DO_SHOW_STATUS:
		return show_status();
		break;

	case DO_SHOW_CONFIG:
		return show_config();
		break;

	case DO_SHOW_HWPROBE:
	case DO_SHOW_HWPROBE_LEGACY:
	case DO_SHOW_HWPROBE_VERBOSE:
		return show_hwprobe(action);
		break;

	case DO_DEBUGGING:
		return debugging();
		break;

	case DO_DEBUG_READ:
		return debug_read();
		break;

	default:
		err = ERR_SYNTAX;
		break;
	}

	
	/* Always a good idea to sleep a bit - easier on system and link
	 * we could be operating 2 cards and 1st one improperly configured
	 * so dwell a little to make things easier before wanconfig reinvoked.
	 */
	if( !err || err == ERR_SYSTEM ) usleep(SLEEP_TIME);
	return err;
}

/*============================================================================
 * Show help text
 */
void show_help(void) {
	puts(helptext);
	exit(1);
}

/*============================================================================
 * Show usage text
 */
void show_usage(void) {
	fprintf(stderr, usagetext);
	exit(1);
}

/*============================================================================
 * set action
 */
void set_action(int new_action) {
	if(action == DO_SHOW_STATUS ) 
		action = new_action;
	else{
		show_usage();
	}
}

/*============================================================================
 * Show error message.
 */
void show_error_dbg (int err, int line)
{
	switch(err){
	case ERR_SYSTEM: 
		fprintf(stderr, "%s:%d: SYSTEM ERROR %d: %s!\n",
	 		prognamed, line, errno, strerror(errno));
		break;
	case ERR_MODULE:
		fprintf(stderr, "%s:%d: ERROR: %s!\n",
			prognamed, line, err_messages[min(err, ERR_LIMIT) - 2]);
		break;
	default:
		fprintf(stderr, "%s:%d: ERROR: %s : %s!\n", prognamed,
			line, err_messages[min(err, ERR_LIMIT) - 2], conf_file);
	}
}

/*============================================================================
 * cat a given  file to stdout - this is used for status and config
 */
int gencat (char *filename) 
{
	FILE *file;
	char buf[256];
	
	file = fopen(filename, "r");
	if( file == NULL ) {
		fprintf( stderr, "%s: cannot open %s\n",
				prognamed, filename);
		show_error(ERR_SYSTEM);
		exit(ERR_SYSTEM);
	}
	
	while(fgets(buf, sizeof(buf) -1, file)) {
		printf(buf);
	}	

	fclose(file);
	return(0);
}

/*============================================================================
 * Start up - This is a shell function to select the configuration source
 */
int startup(void)
{
	char buf[256];
	int err = 0;
	int res = 0;
        int found = 0;	
	DIR *dir;
	struct dirent *dentry;
	struct stat file_stat;

	if(conf_file != NULL ) {
		/* Method I. Observe given config file name
		 */
		return(configure());
		
	}else{
     		/* Method II. Scan /proc/net/wanrouter for device names
     		 */
		
     		/* Open directory */
                dir = opendir(router_dir);

                if(dir == NULL) {
	        	err = ERR_SYSTEM;
	                show_error(err);
	                return(err);
	        }

		while( (dentry = readdir(dir)) ) {

			if( strlen(dentry->d_name) > WAN_DRVNAME_SZ)
				continue;

			/* skip directory dots */
			if( strcmp(dentry->d_name, ".") == 0 \
				|| strcmp(dentry->d_name, "..") == 0)
				continue;

			/* skip /prroc/net/wanrouter/{status,config} */
			if( strcmp(dentry->d_name, "status") == 0 \
				|| strcmp(dentry->d_name, "config") == 0)
				continue;
			
			/* skip interfaces we are not configuring */
			if( card_name != NULL && strcmp(dentry->d_name, card_name) != 0 )
				continue;
			
			snprintf(buf, sizeof(buf), "%s/%s.conf", conf_dir, dentry->d_name);
			
			/* Stat config file to see if it is there */
			res = stat(buf, &file_stat);
			if( res < 0 ) {
				if( errno == ENOENT )
					continue;
				
				show_error(ERR_SYSTEM);
				closedir(dir);
				exit(ERR_SYSTEM);
			}			

			found = 1;
			conf_file = buf;	
			err = configure();
		}
		closedir(dir);
	}

	if (!found){
		/* Method III. Use default configuration file */
		conf_file = def_conf_file;
		err = configure();
	}
	
	return(err);
}

/*============================================================================
* Shut down link.
*/
int wp_shutdown (void)
{
	int err = 0;
	DIR *dir;
	struct dirent *dentry;

	if (conf_file){
		err = conf_file_down();

	}else if( card_name != NULL && dev_name == NULL ) {
		err = router_down(card_name, 0);

	}else if( card_name != NULL && dev_name != NULL ) {
		err = router_ifdel(card_name, dev_name);
	
	}else{
		/* Open directory */
		dir = opendir(router_dir);

		if(dir == NULL) {
			err = ERR_SYSTEM;
			show_error(err);
			return(err);
		}

		while( (dentry = readdir(dir)) ) {
			int res = 0;
			
			if( strlen(dentry->d_name) > WAN_DRVNAME_SZ)
				continue;

			if( strcmp(dentry->d_name, ".") == 0 \
				|| strcmp(dentry->d_name, "..") == 0)
				continue;

			res = router_down(dentry->d_name, 1);	
			if(res){ 
				err = res;
			}
		}
        	
		closedir(dir);
	}

	return err;
}


int router_down (char *devname, int ignore_error)
{
	char filename[sizeof(router_dir) + WAN_DRVNAME_SZ + 2];
        int dev, err=0;
	link_def_t def;

	def.linkconf = malloc(sizeof(wandev_conf_t));
	if (!def.linkconf){
		printf("%s: Error failed to allocated memory on router down\n", 
				devname);
		return -ENOMEM;
	}

	if (verbose)
        	printf(" * Shutting down WAN device %s ...\n", devname);                
	
	if (strlen(devname) > WAN_DRVNAME_SZ) {
                show_error(ERR_SYNTAX);
                return -EINVAL;
	}

#if defined(WAN_HWEC)
	// FIXME: We don't know if wanpipe has HWEC enabled or not....
	//wanconfig_hwec_release(devname, NULL);
#endif

#if defined(__LINUX__)
        snprintf(filename, sizeof(filename), "%s/%s", router_dir, devname);
#else
        snprintf(filename, sizeof(filename), "%s", WANDEV_NAME);
#endif

	dev = open(filename, O_RDONLY);
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	strlcpy(config.devname, &devname[0], WAN_DRVNAME_SZ);
    	def.linkconf->magic = ROUTER_MAGIC;
	config.arg = (void*)def.linkconf;
        if ((dev < 0) || (ioctl(dev, ROUTER_DOWN, &config) < 0 
				&& ( !ignore_error || errno != ENOTTY))) {
#else
        if ((dev < 0) || (ioctl(dev, ROUTER_DOWN, def.linkconf) < 0 
				&& ( !ignore_error || errno != ENOTTY))) {
#endif
		err = ERR_SYSTEM;
		fprintf(stderr, "\n\n\t%s: WAN device %s did not shutdown\n", prognamed, devname);
		fprintf(stderr, "\t%s: ioctl(%s,ROUTER_DOWN) failed:\n", 
				progname_sp, devname);
		fprintf(stderr, "\t%s:\t%d - %s\n", progname_sp, errno, strerror(errno));
		if( weanie_flag ) {
			fprintf(stderr, "\n");
			fprintf(stderr, "\n\tIf you router was not running ignore this message\n !!");
			fprintf(stderr, "\tOtherwise, check the %s and \n", verbose_log);
			fprintf(stderr, "\t%s for errors\n", krnl_log_file);
		}
 		fprintf( stderr, "\n" );
 		if(dev >=0){
			close (dev);
		}
 		return errno;
       	}
 

	if (def.linkconf->config_id == WANCONFIG_ADSL){
		update_adsl_vci_vpi_list(&def.linkconf->u.adsl.vcivpi_list[0], 	
					 def.linkconf->u.adsl.vcivpi_num);
	}

	free(def.linkconf);
	def.linkconf=NULL;

        if (dev >= 0){ 
		close(dev);
	}
	return 0;

}


int router_ifdel (char *card_name, char *dev_name)
{
	char filename[sizeof(router_dir) + WAN_DRVNAME_SZ + 2];
	char devicename[WAN_IFNAME_SZ +2];
	int dev, err=0;

	if (verbose)
        	printf(" * Shutting down WAN device %s ...\n", card_name);                

	if (strlen(card_name) > WAN_DRVNAME_SZ) {
                show_error(ERR_SYNTAX);
                return -EINVAL;
	}

#if defined(__LINUX__)
 	snprintf(filename, sizeof(filename), "%s/%s", router_dir, card_name);
#else
        snprintf(filename, sizeof(filename), "%s", WANDEV_NAME);
#endif
	if (strlen(dev_name) > WAN_IFNAME_SZ) {
		show_error(ERR_SYNTAX);
		return -EINVAL;
	}

#if defined(WAN_HWEC)
	//FIXME : We don't know if wanpipe has HWEC enabled or not....
	//wanconfig_hwec_release(card_name, dev_name);
#endif

#if defined(__LINUX__)
	snprintf(devicename, sizeof(devicename), "%s", dev_name);
#else
	strlcpy(config.devname, dev_name, WAN_DRVNAME_SZ);
    	config.arg = NULL;
#endif
	
        dev = open(filename, O_RDONLY);

#if defined(__LINUX__)
        if ((dev < 0) || (ioctl(dev, ROUTER_IFDEL, devicename) < 0)) {
#else
        if ((dev < 0) || (ioctl(dev, ROUTER_IFDEL, &config) < 0)) {
#endif
		err = errno;
		fprintf(stderr, "\n\n\t%s: Interface %s not destroyed\n", prognamed, devicename);
		fprintf(stderr, "\t%s: ioctl(%s,ROUTER_IFDEL,%s) failed:\n", 
				progname_sp, card_name, devicename);
		fprintf(stderr, "\t%s:\t%d - %s\n", progname_sp, errno, strerror(errno));
		if( weanie_flag ) {
			fprintf(stderr, "\n");
			fprintf(stderr, "\n\tIf you router was not running ignore this message\n !!");
			fprintf(stderr, "\tOtherwise, check the %s and \n", verbose_log);
			fprintf(stderr, "\t%s for errors\n", krnl_log_file);
		}
 		fprintf( stderr, "\n" );
 		if(dev >=0){ 
			close (dev);
		}
                return errno;
       	}


        if (dev >= 0){
		close(dev);
	}
	return 0;

}

/*============================================================================
 * Configure router.
 * o parse configuration file
 * o configure links
 * o configure logical channels (interfaces)
 */
int configure (void)
{
	int err = 0;
	int res;

	/* Parse configuration file */
	if (verbose)
		printf(" * Parsing configuration file %s ...\n", conf_file);

	err = parse_conf_file(conf_file);
	if (err) return err;

	if (link_defs == NULL) {
		if (verbose) printf(" * No link definitions found...\n");
		return 0;
	}

	if( card_name != NULL && strcmp(card_name, link_defs->name) != 0 )
		return ERR_CONFIG;

	if (verbose) printf(
		" * Configuring device %s (%s)\n",
		link_defs->name,
		"no description");
	res = configure_link(link_defs,0);
	if (res) {
		return res;
	}

	free_linkdefs();

	return err;
}

/*============================================================================
 * Parse configuration file.
 *	Read configuration file and create lists of link and channel
 *	definitions.
 */
int parse_conf_file (char* fname)
{
	int err = 0;
	FILE* file;

	file = fopen(fname, "r");
	if (file == NULL) {
		fprintf(stderr, "\nError: %s not found in %s directory\n",
				fname, conf_dir);
		show_error(ERR_SYSTEM);
		return ERR_SYSTEM;
	}

	/* Build a list of link and channel definitions */
	err = build_linkdef_list(file);
	if (!err && link_defs)
		err = build_chandef_list(file);

	fclose(file);
	return err;
}

/*============================================================================
 * Build a list of link definitions.
 */
int build_linkdef_list (FILE* file)
{
	int err = 0;
	char* conf;		/* -> section buffer */
	char* key;		/* -> current configuration record */
	int len;		/* record length */
	char *token[MAX_TOKENS];


	/* Read [devices] section */
	conf = read_conf_section(file, "devices");
	if (conf == NULL) return ERR_CONFIG;

	/* For each record in [devices] section create a link definition
	 * structure and link it to the list of link definitions.
	 */
	for (key = conf; !err && *key; key += len) {
		int toknum;
		
		link_def_t* linkdef;	/* -> link definition */
		int config_id = 0;

		len = strlen(key) + 1;
		toknum = tokenize(key, token);
		if (toknum < 2) continue;

		strupcase(token[1]);
		config_id = name2val(token[1], config_id_str);
		if (!config_id) {
			if (verbose) printf(
			        " * Media ID %s is invalid!\n", token[1]);
			err = ERR_CONFIG;
			show_error(err);
			break;
		}
		linkdef = calloc(1, sizeof(link_def_t));
		if (linkdef == NULL) {
			err = ERR_SYSTEM;
			show_error(err);
			break;
		}

		memset(linkdef,0,sizeof(link_def_t));

	 	strncpy(linkdef->name, token[0], WAN_DRVNAME_SZ);
		linkdef->config_id = config_id;
	if ((toknum > 2) && token[2])
		linkdef->descr = strdup(token[2]);

		linkdef->conf = read_conf_section(file, linkdef->name);
                if (link_defs) {
			linkdef->next=link_defs;
			link_defs=linkdef;
			//link_def_t* last;
			//for (last = link_defs; last->next; last = last->next);
			//last->next = linkdef;
		}
		else link_defs = linkdef;
	}
	free(conf);
	return err;
}

/*============================================================================
 * Build a list of channel definitions.
 */
int build_chandef_list (FILE* file)
{
	int err = 0;
	char* conf;		/* -> section buffer */
	char* key;		/* -> current configuration record */
	int len;		/* record length */
	char *token[MAX_TOKENS];

	link_def_t* linkdef = link_defs;

	if (linkdef == NULL)
		return ERR_CONFIG;


	/* Read [interfaces] section */
	conf = read_conf_section(file, "interfaces");
	if (conf == NULL) 
		return 0;

	/* For each record in [interfaces] section create channel definition
	 * structure and link it to the list of channel definitions for
	 * appropriate link.
	 */
	for (key = conf; !err && *key; key += len) {
		int toknum;
		char *channel_sect;
		
		chan_def_t* chandef;		/* -> channel definition */

		len = strlen(key) + 1;

		toknum = tokenize(key, token);
		if (toknum < 4) continue;

		/* allocate and initialize channel definition structure */
		chandef = calloc(1, sizeof(chan_def_t));
		if (chandef == NULL) {
			err = ERR_SYSTEM;
			show_error(err);
			break;
		}
		memset(chandef,0,sizeof(chan_def_t));

	 	strncpy(chandef->name, token[0], WAN_IFNAME_SZ);
		chandef->addr   = strdup(token[2]);
		chandef->usedby = strdup(token[3]);
		chandef->annexg = NO_ANNEXG;

		/* These are Anexg Connections */
		if (toknum > 5){

			if (!strcmp(token[4],"lapb")){
				chandef->annexg = ANNEXG_LAPB;
			}else if (!strcmp(token[4],"x25")){
				chandef->annexg = ANNEXG_X25;
			}else if (!strcmp(token[4],"dsp")){
				chandef->annexg = ANNEXG_DSP;
				
			}else if (!strcmp(token[4],"fr")){
				chandef->annexg = ANNEXG_FR;
				chandef->protocol = strdup("MP_FR");	
			}else if (!strcmp(token[4],"ppp")){
				chandef->annexg = ANNEXG_PPP;
				chandef->protocol = strdup("MP_PPP");
			}else if (!strcmp(token[4],"chdlc")){
				chandef->annexg = ANNEXG_CHDLC;	
				chandef->protocol = strdup("MP_CHDLC");

			}else if (!strcmp(token[4],"lip_atm")){
				printf("%s: CONIGURING FOR LIP ATM\n",chandef->name);
				chandef->annexg = ANNEXG_LIP_ATM;	
				chandef->protocol = strdup("MP_ATM");

			}else if (!strcmp(token[4],"lip_katm")){
				printf("%s: CONIGURING FOR LIP KATM\n",chandef->name);
				chandef->annexg = ANNEXG_LIP_KATM;	
				chandef->protocol = strdup("MP_KATM");

			}else if (!strcmp(token[4],"lip_x25")){
				chandef->annexg = ANNEXG_LIP_X25;	
				chandef->protocol = strdup("MP_X25");
			
			}else if (!strcmp(token[4],"lip_lapb")){
				chandef->annexg = ANNEXG_LIP_LAPB;
				chandef->protocol = strdup("MP_LAPB");
			}else if (!strcmp(token[4],"lip_xmtp2")){
				chandef->annexg = ANNEXG_LIP_XMTP2;
				chandef->protocol = strdup("MP_XMTP2");
			}else if (!strcmp(token[4],"lip_lapd")){
				chandef->annexg = ANNEXG_LIP_LAPD;
				chandef->protocol = strdup("MP_LAPD");
			}else if (!strcmp(token[4],"lip_hdlc")){
				chandef->annexg = ANNEXG_LIP_HDLC;
				chandef->protocol = strdup("MP_HDLC");
			}else if (!strcmp(token[4],"lip_xdlc")){
				chandef->annexg = ANNEXG_LIP_XDLC;
				chandef->protocol = strdup("MP_XDLC");
			}else if (!strcmp(token[4],"tty")){
				chandef->annexg = ANNEXG_LIP_XDLC;
				chandef->protocol = strdup("MP_TTY");
			}else{
				if (verbose) printf(" * %s defined with invalid protocol %s\n", 
									chandef->name, token[4]);
				return ERR_CONFIG;
			}

		}
		
		if (!(chandef->usedby) ||(( strcmp(chandef->usedby, "WANPIPE")     != 0 ) &&
					  ( strcmp(chandef->usedby, "API")         != 0 ) && 
					  ( strcmp(chandef->usedby, "API_LEGACY")         != 0 ) && 
					  ( strcmp(chandef->usedby, "DATA_API")         != 0 ) && 
					  ( strcmp(chandef->usedby, "BRIDGE")      != 0 ) &&
					  ( strcmp(chandef->usedby, "SWITCH")      != 0 ) &&
					  ( strcmp(chandef->usedby, "PPPoE")       != 0 ) &&
					  ( strcmp(chandef->usedby, "STACK")       != 0 ) &&
					  ( strcmp(chandef->usedby, "XMTP2_API")       != 0 ) &&
					  ( strcmp(chandef->usedby, "NETGRAPH")       != 0 ) &&
					  ( strcmp(chandef->usedby, "TTY")         != 0 ) &&
					  ( strcmp(chandef->usedby, "BRIDGE_NODE") != 0 ) &&
					  ( strcmp(chandef->usedby, "TDM_VOICE") 	   != 0 ) &&
					  ( strcmp(chandef->usedby, "TDM_VOICE_API") 	   != 0 ) &&
					  ( strcmp(chandef->usedby, "TDM_SPAN_VOICE_API") 	   != 0 ) &&
					  ( strcmp(chandef->usedby, "TDM_CHAN_VOICE_API") 	   != 0 ) &&
					  ( strcmp(chandef->usedby, "MTP2_LSL_API") 	   != 0 ) &&
					  ( strcmp(chandef->usedby, "MTP2_HSL_API") 	   != 0 ) &&
					  ( strcmp(chandef->usedby, "TDM_API") 	   != 0 ) &&
					  ( strcmp(chandef->usedby, "TRUNK") 	   != 0 ) &&
					  ( strcmp(chandef->usedby, "ANNEXG")      != 0 )))
		{

			if (verbose) printf(" * %s to be used by an Invalid entity: %s\n", 
									chandef->name, chandef->usedby);
			return ERR_CONFIG;
		}
	
		if (verbose)
			printf(" * %s to used by %s\n",
				chandef->name, chandef->usedby);
		
		channel_sect=strdup(token[0]);
		
		if (toknum > 5){
			char *section=strdup(token[5]);
			/* X25_SW */
			if (toknum > 7){
				if (toknum > 6)
					chandef->virtual_addr = strdup(token[6]);
				if (toknum > 7)
					chandef->real_addr = strdup(token[7]);
				if (toknum > 8)
					chandef->label = strdup(token[8]);
			}else{
				if (toknum > 6)
					chandef->label = strdup(token[6]);
			}

			chandef->conf_profile = read_conf_section(file, section);
			free(section);
			
		}else{
			chandef->conf_profile = NULL;
		}
		
		
		chandef->conf = read_conf_section(file, channel_sect);	
		free(channel_sect);
		
		/* append channel definition structure to the list */
		if (linkdef->chan) {
			chan_def_t* last;

			for (last = linkdef->chan;
			     last->next;
			     last = last->next)
			;
			last->next = chandef;
		}
		else linkdef->chan = chandef;
		/* Alex */
		chandef->link = linkdef;

	}
	free(conf);
	return err;
}


/*============================================================================
 * Configure WAN link.
 */
int configure_link (link_def_t* def, char init)
{
	key_word_t* conf_table = lookup(def->config_id, conf_def_tables);
	int err = 0;
	int len = 0, i, max_len = sizeof(router_dir) + WAN_DRVNAME_SZ;
	char filename[max_len + 2];
	chan_def_t* chandef;
	char* conf_rec;
	int dev=-1;
	unsigned short hi_DLCI = 15;
	char *token[MAX_TOKENS];

	if (def->conf == NULL) {
		show_error(ERR_CONFIG);
		return ERR_CONFIG;
	}

	/* Clear configuration structure */
	if (def->linkconf){
		if (def->linkconf->data) free(def->linkconf->data);
		free(def->linkconf);
		def->linkconf=NULL;
	}

	def->linkconf = malloc(sizeof(wandev_conf_t));
	if (!def->linkconf){
		printf("%s: Failed to allocated linkconf!\n",prognamed);
		return -ENOMEM;
	}
	memset(def->linkconf, 0, sizeof(wandev_conf_t));
	def->linkconf->magic = ROUTER_MAGIC;
	def->linkconf->config_id = def->config_id;

	/* Parse link configuration */
	for (conf_rec = def->conf; *conf_rec; conf_rec += len) {
		int toknum;

		len = strlen(conf_rec) + 1;
		toknum = tokenize(conf_rec, token);
		if (toknum < 2) {
			show_error(ERR_CONFIG);
			return ERR_CONFIG;
		}

		/* Look up a keyword first in common, then in media-specific
		 * configuration definition tables.
		 */
		strupcase(token[0]);
		err = set_conf_param(
			token[0], token[1], common_conftab, 
			def->linkconf, sizeof(wandev_conf_t));
		if ((err < 0) && (conf_table != NULL))
			err = set_conf_param(
				token[0], token[1], conf_table, 
				&def->linkconf->u, sizeof(def->linkconf->u));
		if (err < 0) {
			printf(" * Unknown parameter %s\n", token[0]);
			fprintf(stderr, "\n\n\tERROR in %s !!\n",conf_file);
			if(weanie_flag) fprintf(stderr, "\tPlease check %s for errors\n", verbose_log);
			fprintf(stderr, "\n");
			return ERR_CONFIG;
		}
		if (err){ 
			fprintf(stderr,"\n\tError parsing %s configuration file. Token (%s = %s)\n", conf_file, token[0], token[1]);
			if(weanie_flag) fprintf(stderr,"\tPlease check %s for errors\n", verbose_log);
			fprintf(stderr, "\n");
			return err;
		}
	}

	/* Open SDLA device and perform link configuration */
	snprintf(filename, max_len, "%s/%s", router_dir, def->name);
	
	/* prepare a list of DLCI(s) and place it in the wandev_conf_t structure
	 * This is done so that we have a list of DLCI(s) available when we 
	 * call the wpf_init() routine in sdla_fr.c (triggered by the ROUTER_
	 * SETUP ioctl call) for running global SET_DLCI_CONFIGURATION cmd. This
	 * is only relevant for a station being a NODE, for CPE it polls the 
	 * NODE (auto config DLCI)
	 */
	
	if ((def->linkconf->config_id == WANCONFIG_FR)){
		i = 0;

		if(err)
			return err;
		
		for (chandef = def->chan; chandef; chandef = chandef->next){
		
			if (strcmp(chandef->addr,"auto")==0){
				continue;
			}

			if (verbose) 
				printf(" * Reading DLCI(s) Included : %s\n",
					chandef->addr ? chandef->addr : "not specified");

			if (chandef->annexg != NO_ANNEXG) 
				continue;

			def->linkconf->u.fr.dlci[i] = dec_to_uint(chandef->addr,0);
			if(def->linkconf->u.fr.dlci[i] > hi_DLCI) {
				hi_DLCI = def->linkconf->u.fr.dlci[i];
				i++;
			}else {
				fprintf(stderr,"\n\tDLCI(s) specified in %s are not in order\n",conf_file); 
				fprintf(stderr,"\n\tor duplicate DLCI(s) assigned!\n");
				return ERR_SYSTEM;
			}
		}
	}else{
		/* Read standard VCI/VPI list and send it */
		if ((def->linkconf->config_id == WANCONFIG_ADSL)){
			read_adsl_vci_vpi_list(&def->linkconf->u.adsl.vcivpi_list[0], 
					       &def->linkconf->u.adsl.vcivpi_num);
		}
	}

#if defined(__LINUX__)
	dev = open(filename, O_RDONLY);
#else
	dev = open(WANDEV_NAME, O_RDONLY);
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	strlcpy(config.devname, def->name, WAN_DRVNAME_SZ);
    	config.arg = NULL;
#endif

	if (conf_file && strstr(conf_file, "ft1.conf") != NULL){
		def->linkconf->ft1 = 1;
	}else{
		def->linkconf->ft1 = 0;
	}

 	if (!init){
		/* copy firmware onto S514 card */
		def->linkconf->data = firmware_file_buffer;/* must initialize it again here!!! */
		err=exec_link_cmd(dev,def);
 	}

	/* Configure logical channels */
	if( !err && !nodev_flag ){ 
		for (chandef = def->chan; chandef; chandef = chandef->next) {

			switch (chandef->annexg){
				case NO_ANNEXG:
					strncpy(master_lapb_dev,chandef->name, WAN_IFNAME_SZ);
					strncpy(master_lip_dev,chandef->name, WAN_IFNAME_SZ);
					break;
				case ANNEXG_LAPB:
					strncpy(master_x25_dev,chandef->name, WAN_IFNAME_SZ);
					break;
				case ANNEXG_X25:
					strncpy(master_dsp_dev,chandef->name, WAN_IFNAME_SZ);
					break;
			}

			if (!init){
				if(dev_name != NULL && strcmp(dev_name, chandef->name) != 0 )
					continue;
			}
			
			if (verbose) printf(
				" * Configuring channel %s (%s). Media address: %s\n",
				chandef->name,
				"no description",
				chandef->addr ? chandef->addr : "not specified");

			if (chandef->chanconf){
				free(chandef->chanconf);
				chandef->chanconf=NULL;
			}
			chandef->chanconf=malloc(sizeof(wanif_conf_t));
			if (!chandef->chanconf){
				printf("%s: Failed to allocate chandef\n",prognamed);
				close(dev);
				return -ENOMEM;
			}
			memset(chandef->chanconf, 0, sizeof(wanif_conf_t));
			chandef->chanconf->magic = ROUTER_MAGIC;
			chandef->chanconf->config_id = def->config_id;
			err=configure_chan(dev, chandef, init, def->config_id);
			if (err){
				break;
			}

		        if (strcmp(chandef->usedby, "STACK") == 0){
				strncpy(master_lip_dev,chandef->name, WAN_IFNAME_SZ);
			}
		}
	}
	/* clean up */
	close(dev);
	return err;
}

int exec_link_cmd(int dev, link_def_t *def)
{
	if (!def->linkconf){
		printf("%s: Error: Device %s has no config structure\n",
				prognamed,def->name);
		return -EFAULT;
	}

	{
		char wanpipe_version[50];
		memset(wanpipe_version,0,sizeof(wanpipe_version));

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		config.arg = wanpipe_version;
		if ((dev < 0) || (ioctl(dev, ROUTER_VER, &config))){
#else
		if ((dev < 0) || (ioctl(dev, ROUTER_VER, wanpipe_version))){
#endif

			fprintf(stderr, "\n\n\t%s: WAN device %s driver load failed !!\n", 
							prognamed, def->name);
			fprintf(stderr, "\t%s: ioctl(%s,ROUTER_VER) failed:\n", 
				progname_sp, def->name);
			fprintf(stderr, "\t%s:\t%d - %s\n", progname_sp, errno, strerror(errno));


			fprintf(stderr, "\n");

			switch (errno){

			case EPERM:
				fprintf(stderr, "\n\t%s: Wanpipe Driver Security Check Failure!\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   Linux headers used to compile Wanpipe\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   Drivers, do not match the Kernel Image\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   Please call Sangoma Tech Support\n",
					progname_sp);
		
				break;

			default:
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
				fprintf(stderr, "\n\t%s: Wanpipe Module Installation Failure!\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   1. New Wanpipe modules failed to install\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   during wanpipe installation.\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   2. Wanpipe device doesn't exists in a list.\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   3. Wanpipe data structure is corrupted.\n",
					progname_sp);
#else
				fprintf(stderr, "\n\t%s: Wanpipe Module Installation Failure!\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   New Wanpipe modules failed to install during\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   wanpipe installation.  Current wanpipe modules\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   (lsmod) are obselete.\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   Please check linux source name vs the name\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   of the currently running image.\n",
					progname_sp);
				fprintf(stderr, "\n\t%s:   Then proceed to install Wanpipe again!\n",
					progname_sp);
#endif
				break;	
			}

			
			fprintf(stderr, "\n");
			return -EINVAL;

		}

		if (strstr(wanpipe_version, wan_version) == NULL){
			fprintf(stderr, "\n\n\t%s: WAN device %s driver load failed !!\n", 
							prognamed, def->name);
			fprintf(stderr, "\n\t%s: Wanpipe version mismatch between utilites\n",
					progname_sp);
			fprintf(stderr, "\t%s: and kernel drivers:\n",
					progname_sp);
			fprintf(stderr, "\t%s: \tUtil Ver=%s, Driver Ver=%s\n",
					progname_sp,wan_version,wanpipe_version);
		
			fprintf(stderr, "\n");
			return -EINVAL;
		}
	}

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	config.arg = def->linkconf;
	if ((dev < 0) || (ioctl(dev, ROUTER_SETUP, &config) < 0
				&& ( dev_name == NULL || errno != EBUSY ))) {
#else
	if ((dev < 0) || (ioctl(dev, ROUTER_SETUP, def->linkconf) < 0
						&& ( dev_name == NULL || errno != EBUSY ))) {
#endif
		fprintf(stderr, "\n\n\t%s: WAN device %s driver load failed !!\n", prognamed, def->name);
		fprintf(stderr, "\t%s: ioctl(%s,ROUTER_SETUP) failed:\n", 
				progname_sp, def->name);
		fprintf(stderr, "\t%s:\t%d - %s\n", progname_sp, errno, strerror(errno));
		if( weanie_flag ) {
			fprintf(stderr, "\n");
			fprintf(stderr, "\n\tWanpipe driver did not load properly\n");
			fprintf(stderr, "\tPlease check %s and\n", verbose_log);
			fprintf(stderr, "\t%s for errors\n", krnl_log_file); 
		}
		fprintf(stderr, "\n");
		return errno;
	}
	
	return 0;
}

/*============================================================================
 * Configure WAN logical channel.
 */
int configure_chan (int dev, chan_def_t* def, char init, int id)
{
	int err = 0;
	int len = 0;
	char* conf_rec;
	char *token[MAX_TOKENS];
	key_word_t* conf_annexg_table=NULL;
	key_word_t* conf_table = lookup(id, conf_if_def_tables);
	
	if (def->annexg && def->protocol){
		conf_annexg_table=lookup(def->annexg, conf_annexg_def_tables);
		if (conf_annexg_table){
			set_conf_param(
				"PROTOCOL", def->protocol, chan_conftab, 
				def->chanconf, sizeof(wanif_conf_t));	
		}
	}
	
	/* Prepare configuration data */
	strncpy(def->chanconf->name, def->name, WAN_IFNAME_SZ);
	if (def->addr)
		strncpy(def->chanconf->addr, def->addr, WAN_ADDRESS_SZ);

	if (def->usedby)
		strncpy(def->chanconf->usedby, def->usedby, USED_BY_FIELD);

	if (def->label){
		if (conf_annexg_table){
			set_conf_param(
				"LABEL", def->label, conf_annexg_table, 
				&def->chanconf->u, sizeof(def->chanconf->u));
		}else{
			memcpy(def->chanconf->label,def->label,WAN_IF_LABEL_SZ);
		}
	}
		
	if (def->virtual_addr){
		if (conf_annexg_table){
			set_conf_param(
				"VIRTUAL_ADDR",def->virtual_addr, conf_annexg_table, 
				&def->chanconf->u, sizeof(def->chanconf->u));
		}
	}	
	if (def->real_addr){
		if (def->annexg){
			if (conf_annexg_table){
				set_conf_param(
					"REAL_ADDR",def->real_addr, conf_annexg_table, 
					&def->chanconf->u,sizeof(def->chanconf->u));
			}	
		}
	}
				
	if (def->conf) for (conf_rec = def->conf; *conf_rec; conf_rec += len) {
		int toknum;
		
		len = strlen(conf_rec) + 1;
		toknum = tokenize(conf_rec, token);
		if (toknum < 2) {
			show_error(ERR_CONFIG);
			return -EINVAL;
		}

		/* Look up a keyword first in common, then media-specific
		 * configuration definition tables.
		 */
		strupcase(token[0]);
		if (set_conf_param(token[0], token[1], chan_conftab, def->chanconf,sizeof(wanif_conf_t))) {

			if (def->annexg && conf_annexg_table){

				if (!conf_annexg_table || 
				    set_conf_param(token[0], token[1], conf_annexg_table, &def->chanconf->u, sizeof(def->chanconf->u))) {
				
					printf("Invalid Annexg/Lip parameter %s\n", token[0]);
					show_error(ERR_CONFIG);
					return ERR_CONFIG;
				}
				
			}else if (conf_table){
				if (set_conf_param(token[0], token[1], conf_table, &def->chanconf->u, sizeof(def->chanconf->u))) {
					printf("Invalid Iface parameter %s\n", token[0]);
					show_error(ERR_CONFIG);
					return ERR_CONFIG;
				}
			}else{
				printf("Invalid config table !!! parameter %s\n", token[0]);
				printf("Conf_table %p  %p\n",
						conf_table,
						bitstrm_if_conftab);
				show_error(ERR_CONFIG);
				return ERR_CONFIG;
			}
		}else{

			if (strcmp(token[0], "ACTIVE_CH") == 0){
				strlcpy(def->active_ch, token[1], 50);
			}
		}
	}

	len=0;
	
	if (def->annexg){

		conf_annexg_table=lookup(def->annexg, conf_annexg_def_tables);

		if (def->conf_profile) for (conf_rec = def->conf_profile; *conf_rec; conf_rec += len) {
			int toknum;

			len = strlen(conf_rec) + 1;
			toknum = tokenize(conf_rec, token);
			if (toknum < 2) {
				show_error(ERR_CONFIG);
				return -EINVAL;
			}

			/* Look up a keyword first in common, then media-specific
			 * configuration definition tables.
			 */
			strupcase(token[0]);

			if (!conf_annexg_table ||
			     set_conf_param(token[0], token[1], conf_annexg_table, &def->chanconf->u, sizeof(def->chanconf->u))) {
				if (set_conf_param(token[0], token[1], chan_conftab, def->chanconf, sizeof(wanif_conf_t))) {
					printf("Invalid parameter %s\n", token[0]);
					show_error(ERR_CONFIG);
					return ERR_CONFIG;
				}
			}

			
		}
	}
	err=0;

	if (init){
		return err;
	}
	
	return exec_chan_cmd(dev,def);
}


int exec_chan_cmd(int dev, chan_def_t *def)
{
#if defined(WAN_HWEC)
	int	err;
#endif
	
	if (!def->chanconf){
		printf("%s: Error: Device %s has no config structure\n",
				prognamed,def->name);
		return -EFAULT;
	}

	fflush(stdout);

	switch (def->annexg){

	case NO_ANNEXG:
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		config.arg = (void*)def->chanconf;
		if (ioctl(dev, ROUTER_IFNEW, &config) < 0) {
#else

		if (ioctl(dev, ROUTER_IFNEW, def->chanconf) < 0) {
#endif
			fprintf(stderr, "\n\n\t%s: Interface %s setup failed\n", prognamed, def->name);
			fprintf(stderr, "\t%s: ioctl(ROUTER_IFNEW,%s) failed:\n", 
					progname_sp, def->name);
			fprintf(stderr, "\t%s:\t%d - %s\n", progname_sp, errno, strerror(errno));
			if( weanie_flag ) {
				fprintf(stderr, "\n\tWanpipe drivers could not setup network interface\n");
				fprintf(stderr, "\tPlease check %s and\n", verbose_log);
				fprintf(stderr, "\t%s for errors\n", krnl_log_file); 
			}
			fprintf(stderr, "\n");
			return errno;
		}

#if defined(WAN_HWEC)
		if ((err = wanconfig_hwec(def))){
			fprintf(stderr, "\n\n\t%s: HWEC configuration failed on %s\n", prognamed, def->name);
			fprintf(stderr, "\n\tPlease check %s and\n", verbose_log);
			fprintf(stderr, "\t%s for errors.\n", krnl_log_file); 
			fprintf(stderr, "\n");
			return err;
		}
#endif
		break;

#if defined(__LINUX__)
	case ANNEXG_LAPB:
		strncpy((char*)def->chanconf->master,(char*)master_lapb_dev, WAN_IFNAME_SZ);

		if (ioctl(dev, ROUTER_IFNEW_LAPB, def->chanconf) < 0) {
			fprintf(stderr, "\n\n\t%s: Interface %s setup failed\n", prognamed, def->name);
			fprintf(stderr, "\t%s: ioctl(ROUTER_IFNEW_LAPB,%s) failed:\n", 
					progname_sp, def->name);
			fprintf(stderr, "\t%s:\t%d - %s\n", progname_sp, errno, strerror(errno));
			if( weanie_flag ) {
				fprintf(stderr, "\n\tWanpipe drivers could not setup network interface\n");
				fprintf(stderr, "\tPlease check %s and\n", verbose_log);
				fprintf(stderr, "\t%s for errors\n", krnl_log_file); 
			}
			fprintf(stderr, "\n");
			return errno;
		}
		break;

	case ANNEXG_X25:
		strncpy((char*)def->chanconf->master, (char*)master_x25_dev, WAN_IFNAME_SZ);

		if (ioctl(dev, ROUTER_IFNEW_X25, def->chanconf) < 0) {
			fprintf(stderr, "\n\n\t%s: Interface %s setup failed\n", prognamed, def->name);
			fprintf(stderr, "\t%s: ioctl(ROUTER_IFNEW_X25,%s) failed:\n", 
					progname_sp, def->name);
			fprintf(stderr, "\t%s:\t%d - %s\n", progname_sp, errno, strerror(errno));
			if( weanie_flag ) {
				fprintf(stderr, "\n\tWanpipe drivers could not setup network interface\n");
				fprintf(stderr, "\tPlease check %s and\n", verbose_log);
				fprintf(stderr, "\t%s for errors\n", krnl_log_file); 
			}
			fprintf(stderr, "\n");
			return errno;
		}
		break;

	// DSP_20
	case ANNEXG_DSP:
		strncpy((char*)def->chanconf->master, (char*)master_dsp_dev, WAN_IFNAME_SZ);
		if (ioctl(dev, ROUTER_IFNEW_DSP, def->chanconf) < 0) {
			fprintf(stderr, "\n\n\t%s: Interface %s setup failed\n", prognamed, def->name);
			fprintf(stderr, "\t%s: ioctl(ROUTER_IFNEW_DSP,%s) failed:\n", 
					progname_sp, def->name);
			fprintf(stderr, "\t%s:\t%d - %s\n", progname_sp, errno, strerror(errno));
			if( weanie_flag ) {
				fprintf(stderr, "\n\tWanpipe drivers could not setup network interface\n");
				fprintf(stderr, "\tPlease check %s and\n", verbose_log);
				fprintf(stderr, "\t%s for errors\n", krnl_log_file); 
			}
			fprintf(stderr, "\n");
			return errno;
		}
		break;
#endif

	case ANNEXG_FR:
	case ANNEXG_PPP:
	case ANNEXG_CHDLC:		
	case ANNEXG_LIP_LAPB:
	case ANNEXG_LIP_XDLC:
	case ANNEXG_LIP_TTY:
	case ANNEXG_LIP_XMTP2:
	case ANNEXG_LIP_LAPD:
	case ANNEXG_LIP_X25:
	case ANNEXG_LIP_ATM:
	case ANNEXG_LIP_KATM:
	case ANNEXG_LIP_HDLC:
		strncpy((char*)def->chanconf->master, (char*)master_lip_dev, WAN_IFNAME_SZ);

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		config.arg = (void*)def->chanconf;
		if (ioctl(dev, ROUTER_IFNEW_LIP, &config) < 0) {
#else
		if (ioctl(dev, ROUTER_IFNEW_LIP, def->chanconf) < 0) {
#endif
			fprintf(stderr, "\n\n\t%s: Interface %s setup failed\n", prognamed, def->name);
			fprintf(stderr, "\t%s: ioctl(ROUTER_IFNEW_LIP,%s) failed:\n", 
					progname_sp, def->name);
			fprintf(stderr, "\t%s:\t%d - %s\n", progname_sp, errno, strerror(errno));
			if( weanie_flag ) {
				fprintf(stderr, "\n\tWanpipe drivers could not setup network interface\n");
				fprintf(stderr, "\tPlease check %s and\n", verbose_log);
				fprintf(stderr, "\t%s for errors\n", krnl_log_file); 
			}
			fprintf(stderr, "\n");
			return errno;
		}
		break;
	default:
		show_error(ERR_CONFIG);
		return -EINVAL;
	}
	return 0;
}



#if 0

/*============================================================================
 * Set configuration parameter.
 *	Look up a keyword in configuration description table to find offset
 *	and data type of the configuration parameter into configuration data
 *	buffer.  Convert parameter to apporriate data type and store it the
 *	configuration data buffer.
 *
 *	Return:	0 - success
 *		1 - error
 *		-1 - not found
 */
int set_conf_param (char* key, char* val, key_word_t* dtab, void* conf)
{
	ulong tmp = 0;

	/* Search a keyword in configuration definition table */
	for (; dtab->keyword && strcmp(dtab->keyword, key); ++dtab);
	if (dtab->keyword == NULL) return -1;	/* keyword not found */

	/* Interpert parameter value according to its data type */

	if (dtab->dtype == DTYPE_FILENAME) {
		return read_data_file(
			val, (void*)((char*)conf + dtab->offset));
	}

	if (verbose) printf(" * Setting %s to %s offset %i\n", key, val, dtab->offset);
	if (dtab->dtype == DTYPE_STR) {
		//wanif_conf_t *chanconf = (wanif_conf_t *)conf;
		strcpy((char*)conf + dtab->offset, val);

		//???????????????

		//if (!strcmp(key,"X25_CALL_STRING"))
		//	printf ("Setting STRING: %s to %s offset %i\n",
		//			key,chanconf->x25_call_string,dtab->offset);
		return 0;
	}

	if (!isdigit(*val)) {
		look_up_t* sym;

		strupcase(val);
		for (sym = sym_table;
		     sym->ptr && strcmp(sym->ptr, val);
		     ++sym)
		;
		if (sym->ptr == NULL) {
			if (verbose) printf(" * invalid term %s ...\n", val);
			return 1;
		}
		else tmp = sym->val;
	}
	else tmp = strtoul(val, NULL, 0);

	switch (dtab->dtype) {

	case DTYPE_INT:
		*(int*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_UINT:
		*(uint*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_LONG:
		*(long*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_ULONG:
		*(ulong*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_SHORT:
		*(short*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_USHORT:
		*(ushort*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_CHAR:
		*(char*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_UCHAR:
		*(unsigned char*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_PTR:
		*(void**)((char*)conf + dtab->offset) = (void*)tmp;
		break;
	}
	return 0;
}
#endif

/*============================================================================
 * Set configuration parameter.
 *	Look up a keyword in configuration description table to find offset
 *	and data type of the configuration parameter into configuration data
 *	buffer.  Convert parameter to apporriate data type and store it the
 *	configuration data buffer.
 *
 *	Return:	0 - success
 *		1 - error
 *		-1 - not found
 */
int set_conf_param (char* key, char* val, key_word_t* dtab, void* conf, int max_len)
{
	unsigned int tmp = 0;

	/* Search a keyword in configuration definition table */
	for (; dtab->keyword && strcmp(dtab->keyword, key); ++dtab);
	
	if (dtab->keyword == NULL) return -1;	/* keyword not found */

	/* Interpert parameter value according to its data type */

	if (dtab->dtype == DTYPE_FILENAME) {
		return read_data_file(
			val, (void*)((char*)conf + dtab->offset));
	}
	
	/* FIXME: Add code that will parse Octasic Chip configuration file */
	if (dtab->dtype == DTYPE_OCT_FILENAME) {
		return 0;
	}
	if (dtab->dtype == DTYPE_OCT_CHAN_CONF) {
		return read_oct_chan_config(
			key, val, (void*)((char*)conf + dtab->offset));
	}

	if (verbose) printf(" * Setting %s to %s\n", key, val);

	if (strcmp(key, "RTP_TAP_IP") == 0) {
		int err;
		struct hostent *result = NULL;
#if defined(__LINUX__)
		char buf[512];
		struct hostent hp;
		gethostbyname_r(val, &hp, buf, sizeof(buf), &result, &err);	
#else
		result = gethostbyname(val);
		err = h_errno;
#endif
		if (result) {
#if defined(__LINUX__)
			memcpy((char*)conf + dtab->offset, hp.h_addr_list[0], hp.h_length);
#else
			memcpy((char*)conf + dtab->offset, result->h_addr_list[0], result->h_length);
#endif
		} else {
			printf("Error: Invalid IP Address %s (%s)\n",
						val, hstrerror(err));
			memset((char*)conf + dtab->offset, 0, sizeof(unsigned int));
			err=1;
		}
		return err;
	}
	
	if (strcmp(key, "RTP_TAP_MAC") == 0) {
		char *token;
		char *cp = strdup(val); 
		int cnt=0;
		unsigned char *mac =(unsigned char*)conf + dtab->offset;
		token=strtok(cp, ":");
		if (token) {
			mac[cnt] = (unsigned char)strtoul(token,NULL,16);
			cnt++;
		
			while ((token=strtok(NULL, ":")) != NULL){
				mac[cnt] = strtoul(token,NULL,16);	
				cnt++;
				if (cnt >= 6) {
					break;
				}
			}	
		}
		free(cp);
		
		if (cnt != 6) {
			printf("Error: Invalid MAC Address %s\n",val);
			return 1;
		}
		
		return 0;
	}

	if (dtab->dtype == DTYPE_STR) {
		strlcpy((char*)conf + dtab->offset, val, max_len-dtab->offset);
		return 0;
	}

	if (!(isdigit(*val) || (*val=='-' && isdigit(*(val+1)))) || 
	    strcmp(key, "TE_ACTIVE_CH") == 0 || strcmp(key, "ACTIVE_CH") == 0 || 
	    strcmp(key, "TE_LBO") == 0 || strcmp(key, "LBO") == 0 ||
	    strcmp(key, "FE_MEDIA") == 0 || strcmp(key, "MEDIA") == 0 || 
	    strcmp(key, "RBS_CH_MAP") == 0 || strcmp(key, "TE_RBS_CH") == 0 ||
	    strcmp(key, "TDMV_DCHAN") == 0){
		look_up_t* sym;
		unsigned int tmp_ch;

		strupcase(val);
		for (sym = sym_table;
		     sym->ptr && strcmp(sym->ptr, val);
		     ++sym);
		     
		if (sym->ptr == NULL) {
			int ok_zero=0;

			/* TE1 */
			if (strcmp(key, "TE_ACTIVE_CH") && strcmp(key, "ACTIVE_CH") && 
			    strcmp(key, "RBS_CH_MAP") && strcmp(key, "TE_RBS_CH") &&
			    strcmp(key, "TDMV_DCHAN")) {	
				if (verbose) printf(" * invalid term %s ...\n", val);
				return 1;
			}

			if (strcmp(key, "TDMV_DCHAN") == 0) {
                         	ok_zero=1;
			}

			/* TE1 Convert active channel string to UINT */
		 	tmp_ch = parse_active_channel(val);	
			if (tmp_ch == 0 && !ok_zero){
				if (verbose) printf("Illegal active channel range! %s ...\n", val);
				fprintf(stderr, "Illegal active channel range! min=1 max=31\n");
				return -1;
			}
			tmp = (unsigned int)tmp_ch;
		} else {
		 	tmp = sym->val;
		}
	} else {
		tmp = strtoul(val, NULL, 0);
	}
	/* SANITY CHECK */
	switch (dtab->dtype) {

	case DTYPE_INT:
		SIZEOFASSERT(dtab, sizeof(int));
		*(int*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_UINT:
		SIZEOFASSERT(dtab, sizeof(unsigned int));
		*(unsigned int*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_LONG:
		SIZEOFASSERT(dtab, sizeof(long));
		*(long*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_ULONG:
		SIZEOFASSERT(dtab, sizeof(unsigned long));
		*(unsigned long*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_SHORT:
		SIZEOFASSERT(dtab, sizeof(short));
		*(short*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_USHORT:
		SIZEOFASSERT(dtab, sizeof(unsigned short));
		*(ushort*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_CHAR:
		SIZEOFASSERT(dtab, sizeof(char));
		*(char*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_UCHAR:
		SIZEOFASSERT(dtab, sizeof(unsigned char));
		*(unsigned char*)((char*)conf + dtab->offset) = tmp;
		break;

	case DTYPE_PTR:
		*(void**)((char*)conf + dtab->offset) = (void*)(uintptr_t)tmp;
		break;
	}
	return 0;
}


/*============================================================================
 * Read configuration file section.
 *	Return a pointer to memory filled with key entries read from given
 *	section or NULL if section is not found. Each key is a zero-terminated
 *	ASCII string with no leading spaces and comments. The last key is
 *	followed by an empty string.
 *	Note:	memory is allocated dynamically and must be released by the
 *		caller.
 */
char* read_conf_section (FILE* file, char* section)
{
	char key[MAX_CFGLINE];		/* key buffer */
	char* buf = NULL;
	int found = 0, offs = 0, len, max_len = 0;
	
	rewind(file);
	while ((len = read_conf_record(file, key, MAX_CFGLINE)) > 0) {
		char* tmp;

		if (found) {
			if (*key == '[') break;	/* end of section */

			max_len = len;
			if (buf){
				max_len += offs;
				tmp = realloc(buf, max_len + 1);
			}else{
				tmp = malloc(max_len + 1);
			}
			if (tmp) {
				buf = tmp;
				strlcpy(&buf[offs], key, max_len-offs);
				offs += len;
				buf[offs] = '\0';
			}
			else {	/* allocation failed */
				show_error(ERR_SYSTEM);
 				break;
			}
		}
		else if (*key == '[') {
			tmp = strchr(key, ']');
			if (tmp != NULL) {
				*tmp = '\0';
				if (strcmp(&key[1], section) == 0) {
 					if (verbose) printf(
						" * Reading section [%s]...\n",
						section);
					found = 1;
				}
			}
		}
	}
 	if (verbose && !found) printf(
		" * section [%s] not found!\n", section);
	return buf;
 }

/*============================================================================
 * Read a single record from the configuration file.
 *	Read configuration file stripping comments and leading spaces until
 *	non-empty line is read.  Copy it to the destination buffer.
 *
 * Return string length (incl. terminating zero) or 0 if end of file has been
 * reached.
 */
int read_conf_record (FILE* file, char* key, int max_len)
{
	char buf[MAX_CFGLINE];		/* line buffer */

	while (fgets(buf, MAX_CFGLINE, file)) {
		char* str;
		int len;

		/* Strip leading spaces and comments */
		for (str = buf; *str && strchr(" \t", *str); ++str);
		len = strcspn(str, "#;\n\r");
		if (len) {
			str[len] = '\0';
			strlcpy(key, str, max_len);
			return len + 1;
		}
	}
	return 0;
}

/*============================================================================
 * Tokenize string.
 *	Parse a string of the following syntax:
 *		<tag>=<arg1>,<arg2>,...
 *	and fill array of tokens with pointers to string elements.
 *
 *	Return number of tokens.
 */

void init_tokens(char **token)
{
	int i;

	for (i=0;i<MAX_TOKENS;i++){
		token[i]=NULL;
	}
}


/* Bug Fix by Ren�Scharfe <l.s.r@web.de>
 * removed strtok 
 */
int tokenize (char *str, char **tokens)
{
       	int cnt = 0;
       	char *tok;

	init_tokens(tokens);
	
       	if (!str){
		return 0;
	}

       	tok = strchr(str, '=');
        if (!tok)
		return 0;


	*tok='\0';

       	tokens[cnt] = str;
	str=++tok;
	
       	while (tokens[cnt] && (cnt < MAX_TOKENS-1)) {
	      
		tokens[cnt] = str_strip(tokens[cnt], " \t");
		
               	if ((tok = strchr(str, ',')) == NULL){
			tokens[++cnt] = str_strip(str, " \t");
			goto end_tokenize;
		}

		*tok='\0';

		tokens[++cnt] = str_strip(str, " \t");
		str=++tok;
	
       	}
end_tokenize:
       	return ++cnt;
 }

/*============================================================================
 * Strip leading and trailing spaces off the string str.
 */
char* str_strip (char* str, char* s)
{
	char* eos = str + strlen(str);		/* -> end of string */

	while (*str && strchr(s, *str))
		++str;				/* strip leading spaces */
	while ((eos > str) && strchr(s, *(eos - 1)))
		--eos;				/* strip trailing spaces */
	*eos = '\0';
	return str;
}

/*============================================================================
 * Uppercase string.
 */
char* strupcase (char* str)
{
	char* s;

	for(s = str; *s; ++s) *s = toupper(*s);
	return str;
}

/*============================================================================
 * Get a pointer from a look-up table.
 */
void* lookup (int val, look_up_t* table)
{
	while (table->val && (table->val != val)) table++;
	return table->ptr;
}

/*============================================================================
 * Look up a symbolic name in name table.
 *	Return a numeric value associated with that name or zero, if name was
 *	not found.
 */
int name2val (char* name, look_up_t* table)
{
	while (table->ptr && strcmp(name, table->ptr)) table++;
	return table->val;
}

/*============================================================================
 * Create memory image of a file.
 */
int read_data_file (char* name, data_buf_t* databuf)
{
	int err = 0;
	FILE* file;
	unsigned long fsize;

	databuf->data = NULL;
	databuf->size = 0;
	file = fopen(name, "rb");
	if (file == NULL) {
		if (verbose) printf(" * Can't open data file %s!\n", name);
		fprintf(stderr, "%s: Can't open data file %s!\n", prognamed, name);
		show_error(ERR_SYSTEM);
		return ERR_SYSTEM;
	}

	fsize = filesize(file);
	if (!fsize) {
		if (verbose) printf(" * Data file %s is empty!\n", name);
		err = ERR_CONFIG;
		show_error(err);
	}
	else {

		if (verbose){
			printf(" * Reading %lu bytes from %s ...\n",
				fsize, name);
		}
		if (fread(firmware_file_buffer, 1, fsize, file) != fsize) {
			err = ERR_SYSTEM;
			show_error(err);
			goto done;
		}
		databuf->data = firmware_file_buffer;
		databuf->size = fsize;
	}
done:
	fclose(file);
	return err;
}

int read_oct_chan_config (char *key, char *val, wan_custom_conf_t *conf)
{

	if ((!conf->param_no && conf->params) || (conf->param_no && conf->params == NULL)){
		if (verbose)
			printf(" * INTERNAL ERROR [%s:%d]: Reading OCT6100 config param %s:%s!\n",
						 __FUNCTION__,__LINE__, key,val);
		fprintf(stderr, "%s: INTERNAL ERROR [%s:%d]: Reading OCT6100 config param %s:%s!\n",
						prognamed,
						__FUNCTION__,__LINE__,
						key,val);
		show_error(ERR_SYSTEM);
		return ERR_SYSTEM;
	}

	if (conf->param_no == 0){
		conf->params = malloc(sizeof(wan_custom_param_t));
		if (conf->params == NULL){
			if (verbose) printf(" * Can't allocate memory for OCT6100 config (%s:%s)!\n",
							 key,val);
			fprintf(stderr, "%s: Can't allocate memory for OCT6100 config (%s:%s)!\n",
							prognamed, key, val);
			show_error(ERR_SYSTEM);
			return ERR_SYSTEM;
		}
		memset(conf->params, 0, sizeof(wan_custom_param_t));
	}
	strncpy(conf->params[conf->param_no].name, key, MAX_PARAM_LEN);
	strncpy(conf->params[conf->param_no].sValue, val, MAX_VALUE_LEN);
	conf->param_no++;
	return 0;
}

/*============================================================================
 * Get file size
 *	Return file length or 0 if error.
 */
unsigned long filesize (FILE* file)
{
	unsigned long size = 0;
	unsigned long cur_pos;

	cur_pos = ftell(file);
	if ((cur_pos != -1L) && !fseek(file, 0, SEEK_END)) {
		size = ftell(file);
		fseek(file, cur_pos, SEEK_SET);
	}
	return size;
}

/*============================================================================
 * Convert decimal string to unsigned integer.
 * If len != 0 then only 'len' characters of the string are converted.
 */
unsigned int dec_to_uint (char* str, int len)
{
	unsigned val;

	if (!len) len = strlen((char*)str);
	for (val = 0; len && is_digit(*str); ++str, --len)
		val = (val * 10) + (*str - (unsigned)'0')
	;
	return val;
}

unsigned int get_config_data (int argc, char **argv)
{

	FILE *fp;
	char *device_name=NULL;

	fp = fopen (conf_file,"w");
	if (fp == NULL){
		printf("Could not open file\n");
		return 1;
	}

	get_devices(fp,&argc,&argv,&device_name);
	get_interfaces(fp,&argc,&argv,device_name);
	get_hardware(fp,&argc,&argv,device_name);
	get_intr_setup (fp,&argc,&argv);

	fclose(fp);	
	return 0;

}

int get_devices (FILE *fp, int *argc_ptr, char ***argv_ptr, char **device_name){

	int i;
	int 
						start=0, stop=0;
	int argc = *argc_ptr;
	char **argv = *argv_ptr;

	for (i=0;i<argc;i++){
		if (!strcmp(argv[i],"[devices]")){
			start = i;
			continue;
		}
		if (strstr(argv[i],"[") != NULL){
			stop = i;
			break;
		}	
	}

	if (!stop){
		printf("ERROR: No devices found, Incomplete Argument List\n");
		return 1;
	}

	if (stop == 3){
		fprintf(fp,"%s\n",argv[start]);
		*device_name = argv[start+1];					
		fprintf(fp,"%s = %s\n",argv[start+1],argv[start+2]);	
	}else if (stop > 3) {
		fprintf(fp,"%s\n",argv[start]);
		fprintf(fp,"%s = %s, %s\n",argv[start+1],argv[start+2],argv[start+3]);		
	}else{
		printf("ERROR: No devices found, Too many devices arguments\n");
		return 1;
	}		
	
	*argv_ptr += stop;
	*argc_ptr -= stop;

	//???????????????
	//printf("Start is %i and Stop is %i\n",start,stop);
	return 0;

}

int get_interfaces (FILE *fp, int *argc_ptr, char ***argv_ptr, char *device_name){

	char **argv = *argv_ptr;
	int argc = *argc_ptr;
	int i, start=0, stop=0;
	
	for (i=0;i<argc;i++){
		
		//?????????????????????
		//printf("INTR: Argv %i is %s\n",i,argv[i]);
		if (!strcmp(argv[i],"[interfaces]")){
			start = i;
			continue;
		}
		if (strstr(argv[i],"[") != NULL){
			stop = i;
			break;
		}	
	}
	
	if (!stop){
		printf("ERROR: No interfaces found, Incomplete Argument List\n");
		return 1;
	}

	if ((stop-1)%4){
		printf("ERROR: Insuficient/Too many Interface arguments\n");
		return 1;
	}

	fprintf(fp, "\n%s\n", argv[start]);
	for (i=(start+1);i<stop;i+=4){
		fprintf(fp, "%s = %s, ",argv[i],argv[i+1]);
		if (!strcmp(argv[i+2],"-")){
			fprintf(fp, " ,%s\n",argv[i+3]);
		}else{
			fprintf(fp, "%s, %s\n",argv[i+2],argv[i+3]);
		}
	}

	*argv_ptr += stop;
	*argc_ptr -= stop;

	//???????????????
	//printf("Interface Start is %i and Stop is %i\n",start,stop);
	return 0;
}

int get_hardware (FILE *fp, int *argc_ptr, char ***argv_ptr, char *device_name)
{

	char **argv = *argv_ptr;
	int argc = *argc_ptr;
	int i, start=0, stop=0;

	for (i=0;i<argc;i++){
		
		//?????????????????????
		//printf("HRDW: Argv %i is %s\n",i,argv[i]);
		if (strstr(argv[i],"[wanpipe") != NULL){
			start = i;
			continue;
		}
		if (strstr(argv[i],"[") != NULL){
			stop = i;
			break;
		}	
	}

	if (!stop){
		stop = argc;
	}

	if ((stop-1)%2){
		printf("ERROR: Insuficient/Too many Hardware arguments\n");
		return 1;
	}

	fprintf(fp, "\n%s\n", argv[start]);

	for (i=(start+1);i<stop;i+=2){
		fprintf(fp, "%s = %s\n",argv[i],argv[i+1]);
	}

	*argv_ptr += stop;
	*argc_ptr -= stop;

	//?????????????????
	//printf("Hardware Start is %i and Stop is %i\n",start,stop);
	return 0;
}

int get_intr_setup (FILE *fp, int *argc_ptr, char ***argv_ptr)
{

	char **argv = *argv_ptr;
	int argc = *argc_ptr;
	int i, start=0, stop=0, sfound=0;

	for (i=0;i<argc;i++){
		
		//?????????????????????
		//printf("INTR SETUP: Argv %i is %s\n",i,argv[i]);
		if ((strstr(argv[i],"[") != NULL) && !sfound){
			start = i;
			sfound = 1;
			continue;
		}
		if (strstr(argv[i],"[") != NULL){
			stop = i;
			break;
		}	
	}

	if (!stop){
		stop=argc;
	}

	if ((stop-1)%2){
		printf("ERROR: Insuficient/Too many Interface Setup arguments\n");
		return 1;
	}

	fprintf(fp, "\n%s\n", argv[start]);

	for (i=(start+1);i<stop;i+=2){
		fprintf(fp, "%s = %s\n",argv[i],argv[i+1]);
	}

	*argv_ptr += stop;
	*argc_ptr -= stop;

	//?????????????????
	//printf("Interface SETUP Start is %i and Stop is %i\n",start,stop);

	if (stop != argc){
		get_intr_setup (fp, argc_ptr, argv_ptr);
	}

	return 0;
}

int conf_file_down (void)
{
	FILE* file;
	char* conf;             /* -> section buffer */
	char devname[WAN_DRVNAME_SZ];
	char *token[MAX_TOKENS];
	int toknum, err;

	//printf("Shutting down Device %s\n",conf_file);
        file = fopen(conf_file, "r");
        if (file == NULL) {
                show_error(ERR_SYSTEM);
                return ERR_SYSTEM;
        }

	/* Read [devices] section */
        conf = read_conf_section(file, "devices");
        if (conf == NULL){
		fclose(file);
		return ERR_CONFIG;
	}
	
        toknum = tokenize(conf, token);
        if (toknum < 2){
		fclose(file);
		free(conf);
		return 1;
	}
	
        strncpy(devname, token[0], WAN_DRVNAME_SZ);

	err = router_down (devname,0);
	if (!err && verbose)
		printf ("Done\n");

	free(conf);
	fclose(file);

	return err;
}

/* ========================================================================
 * Part of the Dameon code
 */

#if 0
#define SET_BINARY(f) (void)0

static unsigned long sysv_sum_file (const char *file)
{
	int fd;
	unsigned char buf[8192];
	register unsigned long checksum = 0;
	int bytes_read;

	fd = open (file, O_RDONLY);
	if (fd < 0){
		//perror("checksum file: ");
	  	return 0;
	}

	/* Need binary I/O, or else byte counts and checksums are incorrect.  */
	SET_BINARY (fd);

	while ((bytes_read = safe_read (fd, buf, sizeof buf)) > 0){
		register int i;

		for (i = 0; i < bytes_read; i++){
			checksum += buf[i];
		}
	}

	if (bytes_read < 0){
		perror("bytes read: ");
		close(fd);
		return 0;
	}

	close(fd);
	return checksum;
}
#endif

#define TIME_STRING_BUF 50

char *time_string(time_t t,char *buf)
{
	struct tm *local;
	local = localtime(&t);
	strftime(buf,TIME_STRING_BUF,"%d",local);
	return buf;
}

int has_config_changed(link_def_t *linkdef, char *name)
{
	char filename[50];
	//unsigned long checksum;
	time_t modified;
	struct stat statbuf;
	char timeBuf[TIME_STRING_BUF];

	snprintf(filename,50,"/etc/wanpipe/%s.conf",name);

	if (lstat(filename,&statbuf)){
		return -1;
	}
	
//	printf("Access Time  : %s %i\n",time_string(statbuf.st_atime,timeBuf),statbuf.st_atime);
//	printf("Modified Time: %s %i\n",time_string(statbuf.st_mtime,timeBuf),statbuf.st_mtime);
//	printf("Creation Time: %s %i\n",time_string(statbuf.st_ctime,timeBuf),statbuf.st_ctime);
	
	modified = statbuf.st_mtime;
//	checksum = sysv_sum_file(filename);

	//printf("%s Check sum %u\n",name,checksum);

	if (!linkdef){
		link_def_t *def;
		for (def=link_defs;def;def=def->next){
			if (!strcmp(def->name,name)){
				linkdef=def;
				break;
			}
		}
		if (!linkdef){
			return -1;
		}
	}

//	if (linkdef->checksum != checksum){
//		
//		linkdef->checksum = checksum;
//		printf("%s: Configuration file %s changed\n",prognamed,filename);
//		return 1;
//	}
	if (linkdef->modified != modified){
		
		linkdef->modified = modified;
		printf("%s: Configuration file %s changed: %s \n",
				prognamed,filename,time_string(modified,timeBuf));
		return 1;
	}

	return 0;
}

void free_device_link(char *devname)
{
	link_def_t *linkdef,*prev_lnk=link_defs;

	for (linkdef = link_defs; linkdef; linkdef = linkdef->next){
		if (!strcmp(linkdef->name, devname)){
		
			while (linkdef->chan) {
				chan_def_t* chandef = linkdef->chan;
		
				if (chandef->conf) free(chandef->conf);
				if (chandef->conf_profile) free(chandef->conf_profile);
				if (chandef->addr) free(chandef->addr);
				if (chandef->usedby) free(chandef->usedby);
				if (chandef->protocol) free(chandef->protocol);
				if (chandef->label) free(chandef->label);
				if (chandef->virtual_addr) free(chandef->virtual_addr);
				if (chandef->real_addr) free(chandef->real_addr);
				linkdef->chan = chandef->next;

				if (chandef->chanconf){
					free(chandef->chanconf);
				}
				free(chandef);
			}
			if (linkdef->conf) free(linkdef->conf);
			if (linkdef->descr) free(linkdef->descr);
		
			if (linkdef == link_defs){
				link_defs = linkdef->next;
			}else{
				prev_lnk->next = linkdef->next;	
			}
			printf("%s: Freeing Link %s\n",prognamed,linkdef->name);
			if (linkdef->linkconf){
				free(linkdef->linkconf);
			}
			free(linkdef);
			return;
		}

		prev_lnk=linkdef;
	}
}

void free_linkdefs(void)
{
	/* Clear definition list */
	while (link_defs != NULL) {
		link_def_t* linkdef = link_defs;

		while (linkdef->chan) {
			chan_def_t* chandef = linkdef->chan;
		
			if (chandef->conf) free(chandef->conf);
			if (chandef->conf_profile) free(chandef->conf_profile);
			if (chandef->addr) free(chandef->addr);
			if (chandef->usedby) free(chandef->usedby);
			if (chandef->protocol) free(chandef->protocol);
			if (chandef->label) free(chandef->label);
			if (chandef->virtual_addr) free(chandef->virtual_addr);
			if (chandef->real_addr) free(chandef->real_addr);
			linkdef->chan = chandef->next;

			if (chandef->chanconf){
				free(chandef->chanconf);
			}
			free(chandef);
		}
		if (linkdef->conf) free(linkdef->conf);
		if (linkdef->descr) free(linkdef->descr);
		

		if (linkdef->linkconf){
			free(linkdef->linkconf);
		}

		link_defs = linkdef->next;
		free(linkdef);
	}
}

int device_syncup(char *devname)
{
	char filename[100];
	int err;

	free_device_link(devname);

	snprintf(filename,100,"%s/%s.conf",conf_dir,devname);

	printf("%s: Parsing configuration file %s\n",prognamed,filename);
	err = parse_conf_file(filename);
	if (err){
		return -EINVAL;
	}
	
	has_config_changed(link_defs,link_defs->name);
	
	err = configure_link(link_defs,1);
	if (err){
		return -EINVAL;
	}

	return 0;
}

void sig_func (int sigio)
{
	if (sigio == SIGUSR1){
		printf("%s: Rx signal USR1, User prog ok\n",prognamed);
		return;
	}

	if (sigio == SIGUSR2){
		time_t time_var;
		time(&time_var);
		if ((time_var-time_ui) > MAX_WAKEUI_WAIT){
			time_ui=time_var;
			wakeup_ui=1;
		}
		//else{
		//	printf("%s: Ignoring Wakeup UI %i\n",
		//			prognamed,time_var-time_ui);
		//}
		return;
	}

	if (sigio == SIGHUP){
		printf("%s: Rx signal HUP: Terminal closed!\n",prognamed);
		return;
	}
		
	printf("%s: Rx TERM/INT (%i) singal, freeing links\n",prognamed,sigio);
	free_linkdefs();
	unlink(WANCONFIG_SOCKET);
	unlink(WANCONFIG_PID);
	exit(1);
}


int start_chan (int dev, link_def_t *def)
{
	int err=ERR_SYSTEM;
	chan_def_t *chandef;

	for (chandef = def->chan; chandef; chandef = chandef->next) {

		switch (chandef->annexg){
			case NO_ANNEXG:
				strncpy(master_lapb_dev,chandef->name, WAN_IFNAME_SZ);
				strncpy(master_lip_dev,chandef->name, WAN_IFNAME_SZ);
				break;
			case ANNEXG_LAPB:
				strncpy(master_x25_dev,chandef->name, WAN_IFNAME_SZ);
				break;
			case ANNEXG_X25:
				strncpy(master_dsp_dev,chandef->name, WAN_IFNAME_SZ);
				break;
		}

		if (!dev_name || !strcmp(chandef->name,dev_name)){

			if (verbose){
				printf(
				" * Configuring channel %s (%s). Media address: %s\n",
				chandef->name,
				"no description",
				chandef->addr ? chandef->addr : "not specified");
			}

			err=exec_chan_cmd(dev,chandef);
			if (err){
				return err;
			}
			
			if (!dev_name){
				continue;
			}else{
				return err;
			}
		}
	}
	return err;
}

int start_link (void)
{
	link_def_t* linkdef; 
	int err=-EINVAL;
	int dev=-1;
	char filename[100];

	if (!link_defs){
		if (card_name){
			err=device_syncup(card_name);
			if (err){
				return err;
			}
		}else{
			printf("%s: No wanpipe links initialized!\n",prognamed);
			return err;
		}
	}else{
		if (card_name){
			if (has_config_changed(NULL,card_name)!=0){
				err=device_syncup(card_name);
				if (err){
					return err;
				}
			}
		}else{
start_cfg_chk:			
			for (linkdef = link_defs; linkdef; linkdef = linkdef->next){
				if (has_config_changed(linkdef,linkdef->name)!=0){
					err=device_syncup(linkdef->name);
					if (err){
						return err;
					}
					goto start_cfg_chk;
				}
			}
		}
	}
	
	printf("\n%s: Starting Card %s\n",prognamed,card_name?card_name:"All Cards");
	
	for (linkdef = link_defs; linkdef; linkdef = linkdef->next){

		if (!card_name || !strcmp(linkdef->name,card_name)){

			if (verbose){ 
				printf(" * Configuring device %s (%s)\n",
						linkdef->name,
						card_name?card_name:linkdef->name);
			}
			fflush(stdout);
			
			snprintf(filename, 100,"%s/%s", router_dir, linkdef->name);
			dev = open(filename, O_RDONLY);
			if (dev<0){
				printf("%s: Failed to open file %s\n",prognamed,filename);
				return -EIO;
			}

			linkdef->linkconf->ft1 = 0;

			err=exec_link_cmd(dev,linkdef);
			if (err){
				close(dev);
				return err;
			}
		
			err=start_chan(dev,linkdef);
			if (err){
				close(dev);
				return err;
			}
			
			close(dev);
			if (!card_name){
				continue;
			}else{
				break;
			}
		}
	}

	return err;
}


int stop_link(void)
{
	link_def_t* linkdef; 
	int err=3;

	if (card_name){
		if (!dev_name){
			return router_down(card_name,0);
		}else{
			return router_ifdel(card_name,dev_name);
		}
	}

	if (!link_defs){
		printf("%s: Stop Link Error: No links initialized!\n",prognamed);
		return err;
	}
	
	for (linkdef = link_defs; linkdef; linkdef = linkdef->next){

		if (verbose){ 
			printf(" * Configuring device %s (%s)\n",
					linkdef->name,
					dev_name?dev_name:"card");
		}
		err=router_down(linkdef->name,0);			
		if (err){
			return err;
		}
	}

	return err;
}


/*======================================================
 * exec_command
 *
 * cmd=[sync|start|stop|restart]
 * card=<device name>		# wanpipe# {#=1,2,3...}
 * dev=<interface name>  	# wp1_fr16
 *
 */

int exec_command(char *rx_data)
{
	int toknum;
	char *card_str=NULL;
	char *dev_str=NULL;
	int err=-ENOEXEC;
	char *token[MAX_TOKENS];

	toknum = tokenize((char*)rx_data, token);
        if (toknum < 2){ 
		printf("%s: Invalid client cmd = %s\n",prognamed,rx_data);
		return -ENOEXEC;
	}

	//printf("FIRST TOKNUM = %i\n",toknum);
	
	if (!strcmp(token[0],"cmd")){
		//printf("Command is %s\n",token[1]);
		command=strdup(token[1]);
	}else{
		printf("%s: Invalid client command syntax : %s!\n",prognamed,rx_data);
		goto exec_cmd_exit;
	}

	if (toknum > 2){
		card_str=strdup(token[2]);
	}
	if (toknum > 3){
		dev_str=strdup(token[3]);
	}
	
	if (card_str){
		toknum = tokenize(card_str, token);
		if (toknum < 2){ 
			printf("%s: Invalid client command syntax : %s!\n",prognamed,card_str);
			goto exec_cmd_exit;
		}	

		if (!strcmp(token[0],"card")){
			card_name=strdup(token[1]);
			//printf("Card is %s\n",card_name);
		}

		if (dev_str){
			toknum = tokenize(dev_str, token);
			if (toknum < 2){ 
				printf("%s: Invalid client command syntax : %s!\n",prognamed,dev_str);
				goto exec_cmd_exit;
			}	
			if (!strcmp(token[0],"dev")){
				dev_name=strdup(token[1]);
			}
		}
	}

	err=-EINVAL;
	
	if (!strcmp(command,"start")){
		err=start_link();
	}else if (!strcmp(command,"stop")){
		err=stop_link();
	}else if (!strcmp(command,"restart")){
		err=stop_link();
		err=start_link();
	}else if (!strcmp(command,"init")){
		if (card_name){
			err=device_syncup(card_name);
		}else{
			printf("%s: Error: cmd=init : invalid card name!\n",prognamed);
		}
	}else{
		printf("%s: Error invalid client cmd=%s!\n",prognamed,command);
	}
	
exec_cmd_exit:	
	
	fflush(stdout);

	if (card_str){
		free(card_str);
		card_str=NULL;
	}

	if (dev_str){
		free(dev_str);
		dev_str=NULL;
	}
	
	if (command){ 
		free(command);
		command=NULL;
	}
	if (card_name){
		free(card_name);
		card_name=NULL;
	}
	if (dev_name){
		free(dev_name);
		dev_name=NULL;
	}
	return err;
}

void catch_signal(int signum,int nomask)
{
	struct sigaction sa;
	memset(&sa,0,sizeof(sa));

	sa.sa_handler=sig_func;

	if (nomask){
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
		sa.sa_flags |= SA_NODEFER;
#else
		sa.sa_flags |= SA_NOMASK;
#endif
	}
	
	if (sigaction(signum,&sa,NULL))
		perror("sigaction");
}


int start_daemon(void)
{
	struct sockaddr_un address;
	int sock,conn;
	socklen_t addrLength;
	char rx_data[100];
	int err;
	int fp;
	struct stat file_stat;
	
	verbose=1;

	if (fork() !=0){
		exit(0);
	}
	
	setsid();
	setuid(0);		/* set real UID = root */
	setgid(getegid());
	
	snprintf(prognamed,20,"wanconfigd[%i]",getpid());

	memset(&rx_data[0],0,100);
	memset(&address,0,sizeof(struct sockaddr_un));
#if 0
	signal(SIGTERM,sig_func);
	signal(SIGINT,sig_func);
	signal(SIGUSR1,sig_func);
	signal(SIGUSR2,sig_func);
#endif
	catch_signal(SIGTERM,0);
	catch_signal(SIGINT,0);
	catch_signal(SIGUSR1,0);
	catch_signal(SIGUSR2,0);
	catch_signal(SIGHUP,0);

	if ((sock=socket(PF_UNIX,SOCK_STREAM,0)) < 0){
		perror("socket: ");
		return sock;
	}

	address.sun_family=AF_UNIX;
	strlcpy(address.sun_path, WANCONFIG_SOCKET, sizeof(address.sun_path));

	addrLength = sizeof(address.sun_family) + strlen(address.sun_path);

	if (bind(sock,(struct sockaddr*) &address, addrLength)){
		perror("bind: ");
		return -1;
	}

	if (listen(sock,10)){
		perror("listen: ");
		return -1;
	}

	fp=open(WANCONFIG_PID,(O_CREAT|O_WRONLY),0644);
	if (fp){
		char pid_str[10];
		snprintf(pid_str,10,"%i",getpid());
		write(fp,&pid_str,strlen(pid_str));
		close(fp);
	}


	err = stat(WANCONFIG_PID_FILE, &file_stat);
	if (err<0){
		printf("\n\nWarning: Failed pid write: rc=%i\n\n",err);
	}else{
		snprintf(rx_data,100,"echo %i > %s",getpid(),WANCONFIG_PID_FILE);
		if ((err=system(rx_data)) != 0){
			printf("\n\nWarning: Failed pid write: rc=%i\n\n",err);
		}
	}
	
	memset(&rx_data[0],0,100);
	
	printf("%s: ready and listening:\n\n",prognamed);

tryagain:
	while ((conn=accept(sock,(struct sockaddr*)&address,&addrLength))>=0){

		memset(rx_data,0,100);

		/* Wait for a command from client */
		err=recv(conn,rx_data,100,0);
		if (err>0){
			printf("\n%s: rx cmdstr: %s\n",
					prognamed,rx_data);
		}else{
			printf("%s: Error: received %i from new descriptor %i !\n",
						prognamed,conn,err);
			perror("recv: ");	
			close(conn);
			continue;
		}

		fflush(stdout);

		err=exec_command(rx_data);

		err=abs(err);
		if (err){
			printf("%s: Cmd Error err=%i: %s\n\n\n",
					prognamed,err,strerror(err));
		}else{
			printf("%s: Cmd OK err=%i\n\n\n",prognamed,err);
		}	
		
		
		/* Reply to client the result of the command */
		{
			short l_err = err;
			memcpy(rx_data, &l_err, sizeof(l_err));
		}


		err=send(conn,rx_data,2,0);
		if (err != 2){
			perror("send: ");
			close(conn);
			fflush(stdout);
			continue;
		}
		
		close(conn);

		if (wakeup_ui){
			wakeup_ui=0;
			wakeup_java_ui();	
		}
		fflush(stdout);
	}

	if (conn<0){
		if (errno == EINTR){
			if (wakeup_ui){
				wakeup_ui=0;
				wakeup_java_ui();	
			}
			goto tryagain;
		}
		printf("%s: Accept err = %i\n",prognamed,errno);
		perror("Accept: ");
	}

	printf("%s: Warning: Tried to exit! conn=%i  errno=%i\n",
			prognamed,conn,errno);
	goto tryagain;
	
	close(sock);
	unlink(WANCONFIG_SOCKET);
	unlink(WANCONFIG_PID);
	return 0;
}

#define UI_JAVA_PID_FILE "/var/run/ui.pid"
static time_t ui_checksum=0;
static pid_t ui_pid=0;

void wakeup_java_ui(void)
{
	time_t checksum;
	FILE *file;
	char buf[50];
	struct stat statbuf;

	if (lstat(UI_JAVA_PID_FILE,&statbuf)){
		return;
	}

	checksum = statbuf.st_mtime;	
	if (ui_checksum != checksum){
		
		ui_checksum = checksum;
		
		file = fopen(UI_JAVA_PID_FILE, "r");
		if( file == NULL ) {
			fprintf( stderr, "%s: cannot open %s\n",
					prognamed, UI_JAVA_PID_FILE);
			return;
		}
		
		fgets(buf, sizeof(buf)-1, file);

		ui_pid=atoi(buf);

		fclose(file);
	}

	if (ui_pid){
		if (kill(ui_pid,SIGUSR2) == 0){
			printf("%s: Kicked Java UI: %u\n",prognamed,ui_pid);
		}else{
			printf("%s: Failed to kick UI: %u : %s\n",prognamed,ui_pid,strerror(errno));
		}
	}
	return;
}

int show_config(void)
{
	int 	err = 0;

#if defined(__LINUX__)
	char statbuf[80];
	snprintf(statbuf, sizeof(statbuf), "%s/%s", router_dir, "config");
	gencat(statbuf);
#else
	int 		dev;
	wan_procfs_t	procfs;

	dev = open(WANDEV_NAME, O_RDONLY);
	if (dev < 0){
		show_error(ERR_MODULE);
		return errno;
	}
	config.arg = &procfs;
	memset(&procfs, 0, sizeof(wan_procfs_t));
	procfs.magic 	= ROUTER_MAGIC;
	procfs.max_len 	= 2048;
	procfs.cmd	= WANPIPE_PROCFS_CONFIG;
	procfs.data	= malloc(2048);
	if (procfs.data == NULL){
		show_error(ERR_SYSTEM);
		return -EINVAL;
	}
	if (ioctl(dev, ROUTER_PROCFS, &config) < 0){
		show_error(ERR_SYSTEM);
		goto show_config_end;
	}else{
		if (procfs.offs){
			int i = 0;
			for (i=0;i<procfs.offs;i++){
				putchar(*((unsigned char *)procfs.data + i));
				fflush(stdout);
			}
		}
	}
show_config_end:
	if (dev >= 0) close(dev);
	free(procfs.data);

#endif
	return err;
}

int show_status(void)
{
	int 	err = 0;

#if defined(__LINUX__)
	char statbuf[80];
	if (card_name != NULL){
		snprintf(statbuf, sizeof(statbuf), "%s/%s", router_dir, card_name);
		gencat(statbuf);
	}else{
		snprintf(statbuf, sizeof(statbuf), "%s/%s", router_dir, "status");
		gencat(statbuf);
	}
#else
	int 		dev;
	wan_procfs_t	procfs;

	dev = open(WANDEV_NAME, O_RDONLY);
	if (dev < 0){
		show_error(ERR_MODULE);
		return -ENODEV;
	}
	config.arg = &procfs;
	memset(&procfs, 0, sizeof(wan_procfs_t));
	procfs.magic 	= ROUTER_MAGIC;
	procfs.max_len 	= 2048;
	procfs.cmd	= WANPIPE_PROCFS_STATUS;
	procfs.data	= malloc(2048);
	if (procfs.data == NULL){
		show_error(ERR_SYSTEM);
		return -EINVAL;
	}
	if (ioctl(dev, ROUTER_PROCFS, &config) < 0){
		show_error(ERR_SYSTEM);
		goto show_status_end;
	}else{
		if (procfs.offs){
			int i = 0;
			for (i=0;i<procfs.offs;i++){
				putchar(*((unsigned char *)procfs.data + i));
				fflush(stdout);
			}
		}
	}
show_status_end:
	if (dev >= 0) close(dev);
	free(procfs.data);
#endif
	return err;
}

int show_hwprobe(int action)
{
	int 	err = 0;

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	int 		dev;
	wan_procfs_t	procfs;

	dev = open(WANDEV_NAME, O_RDONLY);
	if (dev < 0){
		show_error(ERR_MODULE);
		return -ENODEV;
	}
	config.arg = &procfs;
	memset(&procfs, 0, sizeof(wan_procfs_t));
	procfs.magic 	= ROUTER_MAGIC;
	procfs.max_len 	= 2048;
	procfs.cmd	= (action == DO_SHOW_HWPROBE) ? WANPIPE_PROCFS_HWPROBE : 
			  (action == DO_SHOW_HWPROBE_LEGACY) ? WANPIPE_PROCFS_HWPROBE_LEGACY : 
									WANPIPE_PROCFS_HWPROBE_VERBOSE ;
	procfs.data	= malloc(2048);
	if (procfs.data == NULL){
		 show_error(ERR_SYSTEM);
		 err=-EINVAL;
		goto show_probe_end;
	}
	if (ioctl(dev, ROUTER_PROCFS, &config) < 0){
		show_error(ERR_SYSTEM);
		err=-EINVAL;
		goto show_probe_end;
	}else{
		if (procfs.offs){
			int i = 0;
			for (i=0;i<procfs.offs;i++){
				putchar(*((unsigned char *)procfs.data + i));
				fflush(stdout);
			}
		}
	}
show_probe_end:
	if (dev >= 0) close(dev);
	free(procfs.data);
#endif
	return err;
}

int debugging(void)
{
    	int	dev;
	int	err = 0, max_len = sizeof(router_dir) + WAN_DRVNAME_SZ;
	char filename[max_len + 2];

	if (dev_name == NULL){
		fprintf(stderr, "\n\n\tPlease specify device name!\n"); 
		show_error(ERR_SYSTEM);
		return -EINVAL;
	}
#if defined(__LINUX__)
	snprintf(filename, max_len, "%s/%s", router_dir, dev_name);
#else
	snprintf(filename, max_len, "%s", WANDEV_NAME);
#endif
	
        dev = open(filename, O_RDONLY); 
        if (dev < 0){
		fprintf(stderr, "\n\n\tFAILED open device %s!\n", 
					WANDEV_NAME);
		show_error(ERR_MODULE);
		return -EINVAL;
	}
 	/* Clear configuration structure */
	memset(&u.linkconf, 0, sizeof(wandev_conf_t));
	u.linkconf.magic = ROUTER_MAGIC;
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
        strlcpy(config.devname, dev_name, WAN_DRVNAME_SZ);
    	config.arg = NULL;
	if (ioctl(dev, ROUTER_DEBUGGING, &config) < 0){ 
	//if (ioctl(dev, ROUTER_DEBUGGING, NULL) < 0){ 

		fprintf(stderr, "\n\n\tROUTER DEBUGGING failed!!\n");
		show_error(ERR_SYSTEM);
		err=-EINVAL;
        }
#else
	/* Linux currently doesn't support this option */
	err=-EINVAL;	
#endif
        if (dev >= 0){ 
		close(dev);
       	}
	return err;
}


int debug_read(void)
{
    	int			dev;
	int			err = 0, max_len = sizeof(router_dir) + WAN_DRVNAME_SZ;
	char 			filename[max_len + 2];
	wan_kernel_msg_t	wan_kernel_msg;

	if (dev_name == NULL){
		fprintf(stderr, "\n\n\tPlease specify device name!\n"); 
		show_error(ERR_SYSTEM);
		return -EINVAL;
	}
#if defined(__LINUX__)
	snprintf(filename, max_len, "%s/%s", router_dir, dev_name);
#else
	snprintf(filename, max_len, "%s", WANDEV_NAME);
#endif
	
        dev = open(filename, O_RDONLY); 
        if (dev < 0){
		fprintf(stderr, "\n\n\tFAILED open device %s!\n", 
					WANDEV_NAME);
		show_error(ERR_MODULE);
		return -EINVAL;
	}
 	/* Clear configuration structure */
	memset(&u.linkconf, 0, sizeof(wandev_conf_t));
	u.linkconf.magic = ROUTER_MAGIC;
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
        strlcpy(config.devname, dev_name, WAN_DRVNAME_SZ);
    	config.arg = &wan_kernel_msg;
#endif
debug_read_again:
	memset(&wan_kernel_msg, 0, sizeof(wan_kernel_msg_t));
	wan_kernel_msg.magic 	= ROUTER_MAGIC;
	wan_kernel_msg.max_len 	= 2048;
	wan_kernel_msg.data	= malloc(2048);
	if (wan_kernel_msg.data == NULL){
		 show_error(ERR_SYSTEM);
		 err=-EINVAL;
		goto debug_read_end;
	}
#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
	if (ioctl(dev, ROUTER_DEBUG_READ, &config) < 0){ 
#else
	if (ioctl(dev, ROUTER_DEBUG_READ, &wan_kernel_msg) < 0){
#endif

		fprintf(stderr, "\n\n\tROUTER DEBUG_READ failed!!\n");
		show_error(ERR_SYSTEM);
		err=-EINVAL;
        }else{
		if (wan_kernel_msg.len){
			int i = 0;
			for (i=0;i<wan_kernel_msg.len;i++){
				putchar(*((unsigned char *)wan_kernel_msg.data + i));
				fflush(stdout);
			}
			if (wan_kernel_msg.is_more){
				goto debug_read_again;
			}
		}
	}
debug_read_end:
        if (dev >= 0){ 
		close(dev);
       	}
	free(wan_kernel_msg.data);
	return err;
}



void update_adsl_vci_vpi_list(wan_adsl_vcivpi_t* vcivpi_list, unsigned short vcivpi_num)
{
	FILE*		file = NULL, *tmp_file = NULL;
	char		buf[256];
	int 		x = 0;

	file = fopen(adsl_file, "r");
	tmp_file = fopen(tmp_adsl_file, "w");
	if (file == NULL || tmp_file == NULL){
		printf(" * ADSL VCI/VPI list file doens't exists (skip)\n");
		if (file) fclose(file);
		if (tmp_file) fclose(tmp_file);
		return;
#if 0
		fprintf( stderr, "%s: cannot open %s or %s\n",
				prognamed, adsl_file, tmp_adsl_file);
		if (file) fclose(file);
		if (tmp_file) fclose(tmp_file);
		show_error(ERR_SYSTEM);
		exit(ERR_SYSTEM);
#endif
	}

	while(fgets(buf, sizeof(buf) -1, file)){
		if (buf[0] != '#'){
			break;
		}else{
			fprintf(tmp_file, buf);
		}
	}	
	for(x = 0; x < vcivpi_num; x++){
		fprintf(tmp_file, "%d %d\n", 
				vcivpi_list[x].vci, 
				vcivpi_list[x].vpi);
	}
	fclose(file);
	fclose(tmp_file);
	rename(tmp_adsl_file, adsl_file);
	return;
}


void read_adsl_vci_vpi_list(wan_adsl_vcivpi_t* vcivpi_list, unsigned short* vcivpi_num)
{
	FILE*		file = NULL;
	char		buf[256];
	int		num = 0;
	int 	vci, vpi;

	file = fopen(adsl_file, "r");
	if (file == NULL){
		printf(" * ADSL VCI/VPI list file doens't exists (skip)\n");
		*vcivpi_num = 0;
		return;
#if 0
		fprintf( stderr, "%s: cannot open %s\n",
				prognamed, adsl_file);
		show_error(ERR_SYSTEM);
		exit(ERR_SYSTEM);
#endif
	}
	
	while(fgets(buf, sizeof(buf) -1, file)){
		if (buf[0] != '#'){
			sscanf(buf, "%d %d", &vci, &vpi);
			vcivpi_list[num].vci = (unsigned short)vci;
			vcivpi_list[num].vpi = (unsigned char)vpi;
			num++;
			if (num >= 100){
				break;
			}
		}
	}	
	*vcivpi_num = num;
	fclose(file);
	return;
}


/*============================================================================
 * TE1
 * Parse active channel string.
 *
 * Return ULONG value, that include 1 in position `i` if channels i is active.
 */
unsigned int parse_active_channel(char* val)
{
#define SINGLE_CHANNEL	0x2
#define RANGE_CHANNEL	0x1
	int channel_flag = 0;
	char* ptr = val;
	int channel = 0, start_channel = 0;
	unsigned int tmp = 0;

	if (strcmp(val,"ALL") == 0)
		return ENABLE_ALL_CHANNELS;
	
	if (strcmp(val,"0") == 0)
		return 0;

	while(*ptr != '\0') {
       		//printf("\nMAP DIGIT %c\n", *ptr);
		if (isdigit(*ptr)) {
			channel = strtoul(ptr, &ptr, 10);
			channel_flag |= SINGLE_CHANNEL;
		} else {
			if (*ptr == '-') {
				channel_flag |= RANGE_CHANNEL;
				start_channel = channel;
			} else {
				tmp |= get_active_channels(channel_flag, start_channel, channel);
				channel_flag = 0;
			}
			ptr++;
		}
	}
	if (channel_flag){
		tmp |= get_active_channels(channel_flag, start_channel, channel);
	}

	return tmp;
}

/*============================================================================
 * TE1
 */
unsigned int get_active_channels(int channel_flag, int start_channel, int stop_channel)
{
	int i = 0;
	unsigned int tmp = 0, mask = 0;

	/* If the channel map is set to 0 then
	 * stop_channel will be zero. In this case just return
	 * 0 */
	if (stop_channel < 1) {
        	return 0;
	}

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


//****** End *****************************************************************/

