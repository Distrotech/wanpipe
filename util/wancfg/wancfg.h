/***************************************************************************
                          wancfg.h  -  description
                             -------------------
    begin                : Thu Feb 26 2004
    copyright            : (C) 2004 by David Rokhvarg
    email                : davidr@sangoma.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* WANCFG Structure diagram.
 * Notes:
 *  1. All LIP interfaces belonging to
 *     the same HW interface will have a redunant copy
 *     of the 'profile'. For example, each DLCI in Frame Relay
 *     will have the same copy of 'global' FR configuration.
 *     And, each interface will have it's own configuration:
 *     CIR, and so on.
 *  2. Each LIP interface in turn, may be 'used by' as STACK
 *     and the same logic apply recursively.
 *     
 -------------------------------
 |conf_file_reader(CFR)|	|
 |---------------------		|		   Hardware Interfaces			LIP Interfaces
 |				|		   --------------------			--------------
 |link_def_t* link_defs		|			    
 |				|		---------------------------
 |objects_list* main_obj_list	|-------------->|list_element_chan_def|	  |
 |------------------------------	|	----------------------	  |
					|	|			  |
					|	|chan_def_t data	  |
					|	|			  |	  --------------------------
					|	|void* next_objects_list  |------>|list_element_chan_def|   |
					|	|-------------------------|   |	  |----------------------   |
					|				      |	  |(DLCI 16)		    |
					|				      |	  |chan_def_t data	    |
					|				      |	  |			    |
					|				      |	  |void* next_objects_list  |
					|				      |	  --------------------------
					|				      |
					|	---------------------------   |   ---------------------------
					|------>|list_element_chan_def|	  |   |-->|list_element_chan_def|   |
 						----------------------	  |       |----------------------   |
						|			  |       |(DLCI 17)		    |
						|chan_def_t data	  |	  |chan_def_t data	    |
						|			  |	  |			    |
						|void* next_objects_list  |	  |void* next_objects_list  |
						|-------------------------|	  |-------------------------|
*/

#ifndef WANCFG_H
#define WANCFG_H

#include "cpp_string.h" //class for strings handling. had to do it because
                        //C++ 'string' is NOT istalled on all systems.

#if defined(__LINUX__)
//need by some compilers
#include <iostream>
#endif

//C includes:
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

//have to do it to avoid conflict with 'string' typedef in 'string.h'
//and to use 'cpp_string'.
#define string cpp_string

//set this to 1 to run in 'BSD' mode
#define BSD_DEBG 0
/*
BSD interface names rules: 
  1. must end with a digit
  2. no digits allowed in the middle of the name
*/

//#include <sys/time.h>

#if defined(__LINUX__)
# include <linux/wanpipe_defines.h>
# include <linux/sdla_fr.h>
# include <linux/wanpipe_cfg.h>
# include <linux/sdla_te1.h>
# include <linux/sdlasfm.h>
# include <linux/sdla_front_end.h>
# include <linux/sdla_remora.h>
#else
# include <time.h>
# include <sys/types.h>
# include <wanpipe_defines.h>
# include <sdla_fr.h>
# include <wanpipe_cfg.h>
# include <sdla_te1.h>
# include <sdlasfm.h>
# include <sdla_front_end.h>
# include <sdla_remora.h>
#endif

#define MAX_PATH_LENGTH 8192
#define NO  1
#define YES 2

#if defined(__LINUX__)
# if !defined(strlcpy)
#  define strlcpy(d,s,l) strcpy((d),(s))
# endif
#elif defined(__FreeBSD__)
# if !defined(strlcpy)
#  define strlcpy(d,s,l) strcpy((d),(s))
# endif
#endif

//end of settings for the 'adapter_type'
//////////////////////////////////////////////////////////////////////////
#define MAX_LEN_OF_ACTIVE_CHANNELS_STRING 100

typedef struct chan_def		/* Logical Channel definition */
{
  char name[WAN_IFNAME_SZ+1];	/* interface name for this channel */
  char* addr;			/* -> media address */
  char* conf;			/* -> configuration data */
  char* conf_profile;		/* -> protocol profile data */
  char	usedby;			/* used by WANPIPE or API */
  char* label;			/* -> user defined label/comment */
  char* virtual_addr;		/* -> X.25 virtual addr */
  char* real_addr;		/* -> X.25 real addr */
  wanif_conf_t  *chanconf;	/* channel configuration structure */
  char chan_start_script;
  char chan_stop_script;
  char active_channels_string[MAX_LEN_OF_ACTIVE_CHANNELS_STRING];
  char active_hwec_channels_string[MAX_LEN_OF_ACTIVE_CHANNELS_STRING];
//  char hwec_flag;//WANOPT_YES/WANOPT_NO
} chan_def_t;

typedef struct link_def		/* WAN Link definition */
{
  char name[WAN_DRVNAME_SZ+1];	/* link device name */
  int  sub_config_id;		/* configuration ID for internal use only */
  char* conf;			/* -> configuration data */
  char* descr;			/* -> description */
  wandev_conf_t* linkconf;	/* link configuration structure */
  
  char start_script;
  char stop_script;
  
  unsigned int card_version;	//S514 has many versions: S514-1/2/3/...7
  unsigned int card_sub_version;//A104 can be "Shark" 

  char active_channels_string[MAX_LEN_OF_ACTIVE_CHANNELS_STRING];
  char firmware_file_path[MAX_PATH_LENGTH];
} link_def_t;

//Flag used around variabales used ONLY for debugging.
//Set to zero in "release" version to avoid 'unused variable' message
//from the compiler.
//#define DBG_FLAG 1
#define DBG_FLAG 0

#if DBG_FLAG
#define Debug(dbg_flag, message)	if(dbg_flag){printf message;}
#define FUNC_DBG()			printf("%s(), Line: %d\n", __FUNCTION__, __LINE__)
#else
#define Debug(dbg_flag, message)
#define FUNC_DBG()
#endif

#define MENUINSTR_EXIT "------------------------------------------ \
Use arrows to navigate through the options. \
Press <Enter> to select highlighted opt. \
Press <Exit> to exit Wanpipe configuration. \
Press <ESC> to go back, <Help> for help."

#define MENUINSTR_BACK "------------------------------------------ \
Use arrows to navigate through the options. \
Press <Enter> to select highlighted opt. \
Press <Back> to return to previous menu. \
Press <ESC> to go back, <Help> for help."

#define WANCFG_PROGRAM_NAME "WANPIPE Configuration Utility"
#define WANCFG_EXECUTABLE_NAME "wancfg"

#define INVALID_PROTOCOL "Invalid Protocol!!"
#define INVALID_CARD_TYPE 0

#define NOT_SET 100 //can be used for storage as small as 1 byte.
#define NOT_SET_STR "Not Set"

#define NO_OPER_MODE YES+1

//defines for clarifying the source code
#define IN  //value passed to a function
#define OUT //function sets value on return

//The '/tmp' directory is ALWAYS writable, this important when running off CD-ROM
//because the current directory on CD is NOT writable.
#define LXDIALOG_OUTPUT_FILE_NAME "/tmp/lxdialog_output"

//SELECTION can be a button number or a selected menu number.
#define SELECTION_0  0
#define SELECTION_1  1
#define SELECTION_2  2

//delimeter for functions with variable length parameter list
#define END_OF_PARAMS_LIST "END_OF_PARAMS_LIST"

#define MAX_NUMBER_OF_WANPIPES		64
#define MAX_NUMBER_OF_NET_INTERFACES_PER_WANPIPE  512
#define WANPIPE_TOKEN   "wanpipe"

//Operation Mode (Used by):
#define OP_MODE_WANPIPE     	" \"WANPIPE\" "     //1
#define OP_MODE_API         	" \"API\" "         //2
#define OP_MODE_BRIDGE      	" \"BRIDGE\" "      //3
#define OP_MODE_BRIDGE_NODE 	" \"BRIDGE_NODE\" " //4
#define OP_MODE_SWITCH      	" \"SWITCH\" "      //5
#define OP_MODE_STACK       	" \"STACK\" "       //6
#define OP_MODE_ANNEXG      	" \"ANNEXG\" "      //7
#define OP_MODE_VoIP        	" \"TDM_VOICE\" "   //8
#define OP_MODE_PPPoE       	" \"PPPoE\" "       //9
#define OP_MODE_TTY		" \"TTY\" "         //10
#define OP_MODE_TDM_API  	" \"TDM_API\" "     //11
#define OP_MODE_NETGRAPH  	" \"NETGRAPH\" "    //12
#define OP_MODE_TDMV_API       	" \"TDM_VOICE_API\" "   //13

#define PPPoE TTY+10  //a valid Operation Mode for ADSL card.
		      //(but not defined in wanpipe_cfg.h !)	

#define QUOTED_1     " \"1\" "
#define QUOTED_2     " \"2\" "
#define QUOTED_3     " \"3\" "
#define QUOTED_4     " \"4\" "
#define QUOTED_5     " \"5\" "
#define QUOTED_6     " \"6\" "
#define QUOTED_7     " \"7\" "
#define QUOTED_8     " \"8\" "
#define QUOTED_9     " \"9\" "
#define QUOTED_10    " \"10\" "
#define QUOTED_11    " \"11\" "
#define QUOTED_12    " \"12\" "
#define QUOTED_13    " \"13\" "

////////////////////////////////////////////////////
//Frame Relay definitions
#define MAX_DLCI_NUMBER   100
#define MIN_CIR 1
#define MAX_CIR 512
#define MIN_BC  1
#define MAX_BC  512
#define MIN_BE  0
#define MAX_BE  512

#define MANUAL_DLCI 	1
#define AUTO_DLCI	2

////////////////////////////////////////////////////
//LIP ATM definitions
#define LIP_ATM_MTU_MRU	212	//4 cells: 4*53=212

////////////////////////////////////////////////////
//CHDLC definintions
//#include <linux/sdla_chdlc.h> - can not include!!
//because of conflicting type: 'api_rx_hdr_t' wich
//is also defined in 'sdla_fr.h'
//because of it copying some definintions here:
#define MIN_Tx_KPALV_TIMER	0	/* minimum transmit keepalive timer */
#define MAX_Tx_KPALV_TIMER	60	/* maximum transmit keepalive timer - in seconds */
#define DEFAULT_Tx_KPALV_TIMER	5	/* default transmit keepalive timer - in seconds */
//#define MAX_Tx_KPALV_TIMER		60000	  /* maximum transmit keepalive timer - in milliseconds */
//#define DEFAULT_Tx_KPALV_TIMER	10000	  /* default transmit keepalive timer - in milliseconds */
#if 0
#define MIN_Rx_KPALV_TIMER	10	  /* minimum receive keepalive timer */
#define MAX_Rx_KPALV_TIMER	60000	  /* maximum receive keepalive timer */
#define DEFAULT_Rx_KPALV_TIMER	10000	  /* default receive keepalive timer */
#endif
#define MIN_KPALV_ERR_TOL	1	  /* min kpalv error tolerance count */
#define MAX_KPALV_ERR_TOL	20	  /* max kpalv error tolerance count */
#define DEFAULT_KPALV_ERR_TOL	3	  /* default value */


////////////////////////////////////////////////////
//structure for reading and writing "interface" file
//in '/etc/wanpipe/interface'.
#define IF_CONFIG_BUF_LEN   100

typedef struct _if_config{
  char device[WAN_IFNAME_SZ+1];//wp10adsl
  char ipaddr[IF_CONFIG_BUF_LEN];
  char netmask[IF_CONFIG_BUF_LEN];//255.255.255.0
  char point_to_point_ipaddr[IF_CONFIG_BUF_LEN];
  char on_boot;//yes/no
  char gateway[IF_CONFIG_BUF_LEN];
}if_config_t;

////////////////////////////////////////////////////
//global functions in main.cpp:
char * get_protocol_string(int protocol);
#define NO_PROTOCOL_NEEDED  WANCONFIG_X25-1
#define PROTOCOL_NOT_SET  0
//the TDM_VOICE and TDM_VOICE_API is actually an Operation Mode (like API...)
//but there is a need to display and configure it as a Protocol.
//no such 'config_id' in wanpipe_cfg.h', so have to define my own
enum NOT_REAL_PROTOCOLS{
  PROTOCOL_TDM_VOICE = 60,
  PROTOCOL_IP,
  PROTOCOL_ETHERNET,
  PROTOCOL_TDM_VOICE_API
};

#define ADSL_IP_STR 	  "ADSL (IP)"
#define ADSL_ETHERNET_STR "ADSL (Ethernet)"

int check_file_exist(char * file_name, FILE ** file);
char * get_card_type_string(int card_type, int card_version);
void set_default_t3_configuration(sdla_fe_cfg_t* fe_cfg);
void set_default_e3_configuration(sdla_fe_cfg_t* fe_cfg);
void set_default_t1_configuration(sdla_fe_cfg_t* fe_cfg);

void set_default_adsl_configuration(wan_adsl_conf_t* adsl_cfg);

int get_config_id_from_profile(char* config_id);

int is_protocol_valid_for_AFT(int protocol);
int is_protocol_valid_for_S518_ADSL(int protocol);
int adjust_number_of_logical_channels_in_list(	int config_id,
						IN void* list,
						IN void* info,
                                            	IN unsigned int new_number_of_logical_channels);

typedef struct{
  char* name_of_parent_layer;
  char auto_dlci;

}fr_config_info_t;

////////////////////////////////////////////////////////////////////////////////

char check_wanpipe_start_script_exist(char* wanpipe_name);
char check_wanpipe_stop_script_exist(char* wanpipe_name);

int create_wanpipe_start_script(char* wanpipe_name);
int create_wanpipe_stop_script(char* wanpipe_name);

void remove_wanpipe_start_script(char* wanpipe_name);
void remove_wanpipe_stop_script(char* wanpipe_name);

int edit_wanpipe_start_script(char* wanpipe_name);
int edit_wanpipe_stop_script(char* wanpipe_name);
int is_protocol_valid_for_S514(int protocol, char card_version, int comm_port);

////////////////////////////////////////////////////////////////////////////////
char check_net_interface_start_script_exist(char* wanpipe_name, char* if_name);
char check_net_interface_stop_script_exist(char* wanpipe_name, char* if_name);

int create_new_net_interface_start_script(char* wanpipe_name, char* if_name);
int create_new_net_interface_stop_script(char* wanpipe_name, char* if_name);

void remove_net_interface_start_script(char* wanpipe_name, char* if_name);
void remove_net_interface_stop_script(char* wanpipe_name, char* if_name);

int edit_net_interface_start_script(char* wanpipe_name, char* if_name);
int edit_net_interface_stop_script(char* wanpipe_name, char* if_name);

int is_console_size_valid();
////////////////////////////////////////////////////////////////////////////////

char* remove_spaces_in_int_string(char* input);
int yes_no_question( OUT int* selection_index,
                      IN char * lxdialog_path,
                      IN int protocol,
                      IN char* format, ...);
int get_wanpipe_number_from_conf_file_name(char* conf_file_name);
void str_tolower(char * str);
void str_toupper(char * str);

int write_string_to_file(char * full_file_path, char* string);
int append_string_to_file(char * full_file_path, char* string);

int get_used_by_integer_value(char* used_by_str);
char* get_used_by_string(int used_by);
char* replace_new_line_with_zero_term(char* str);
char* replace_new_line_with_space(char* str);
char* get_S514_card_version_string(char card_version);
void tokenize_string(char* input_buff, char* delimeter_str, char* output_buff, int buff_length);
char* replace_char_with_other_char_in_str(char* str, char old_char, char new_char);
char* get_date_and_time();
unsigned int parse_active_channel(char* val, unsigned char media_type);
unsigned int get_active_channels(int channel_flag, int start_channel,
                                  int stop_channel, unsigned char media_type);
int check_channels(int channel_flag, unsigned int start_channel,
                   unsigned int stop_channel, unsigned char media_type);

char* validate_ipv4_address_string(char* str);
char* validate_authentication_string(char* str, int max_length);

char* replace_numeric_with_char(char* str);
///////////////////////////////////////////////////////////////////////
//errors display
void err_printf(char* format, ...);

//macro to display a fatal error and to exit the program
#define ERR_DBG_OUT(_x_)                                              \
{ printf("%s: Error in File: %s, Function: %s(), Line: %d. Text:\n",  \
    prognamed, __FILE__, __FUNCTION__, __LINE__);                     \
  printf _x_;                                                         \
  exit(1);							      \
}
///////////////////////////////////////////////////////////////////////


#define ADSL_ENCAPSULATION_DECODE(encapsulation)			\
  (encapsulation == RFC_MODE_BRIDGED_ETH_LLC) ? "Bridged Eth LLC over ATM (PPPoE)" : 	\
  (encapsulation == RFC_MODE_BRIDGED_ETH_VC)  ? "Bridged Eth VC over ATM" : 	\
  (encapsulation == RFC_MODE_ROUTED_IP_LLC)   ? "Classical IP LLC over ATM" : 	\
  (encapsulation == RFC_MODE_ROUTED_IP_VC)    ? "Routed IP VC over ATM" : 	\
  (encapsulation == RFC_MODE_RFC1577_ENCAP)   ? "RFC_MODE_RFC1577_ENCAP" : 	\
  (encapsulation == RFC_MODE_PPP_LLC)	      ? "PPP (LLC) over ATM" :		\
  (encapsulation == RFC_MODE_PPP_VC)	      ? "PPP (VC) over ATM (PPPoA)" : 	"Unkonwn"
  
#define ADSL_DECODE_STANDARD(standard)				\
  (standard == WANOPT_ADSL_T1_413) 	? "T1_413" : 		\
  (standard == WANOPT_ADSL_G_LITE) 	? "G_LITE" : 		\
  (standard == WANOPT_ADSL_G_DMT)  	? "G_DMT" : 		\
  (standard == WANOPT_ADSL_ALCATEL_1_4) ? "ALCATEL_1_4" : 	\
  (standard == WANOPT_ADSL_MULTIMODE) 	? "MULTIMODE" : 	\
  (standard == WANOPT_ADSL_ADI) 	? "ADI" : 		\
  (standard == WANOPT_ADSL_ALCATEL) 	? "ALCATEL" : 		\
  (standard == WANOPT_ADSL_T1_413_AUTO) ? "T1_413_AUTO" : "Unknown"

#define ADSL_DECODE_TRELLIS(trellis)				\
  (trellis == WANOPT_ADSL_TRELLIS_DISABLE) 	? "Disable" :	\
  (trellis == WANOPT_ADSL_TRELLIS_ENABLE)	? "Enable" : 	\
  (trellis == WANOPT_ADSL_TRELLIS_LITE_ONLY_DISABLE)	? "Lite only disable" : "Unknown"

#define ADSL_DECODE_GAIN(gain)			  \
  (gain == WANOPT_ADSL_0DB_CODING_GAIN)	? "0db" : \
  (gain == WANOPT_ADSL_1DB_CODING_GAIN)	? "1db"	: \
  (gain == WANOPT_ADSL_2DB_CODING_GAIN)	? "2db"	: \
  (gain == WANOPT_ADSL_3DB_CODING_GAIN)	? "3db"	: \
  (gain == WANOPT_ADSL_4DB_CODING_GAIN)	? "4db"	: \
  (gain == WANOPT_ADSL_5DB_CODING_GAIN)	? "5db" : \
  (gain == WANOPT_ADSL_6DB_CODING_GAIN)	? "6db"	: \
  (gain == WANOPT_ADSL_7DB_CODING_GAIN)	? "7db"	: \
  (gain == WANOPT_ADSL_AUTO_CODING_GAIN)	? "Auto coding (Default)" : "Unknown"

#define ADSL_DECODE_RX_BIN_ADJUST(rxbin)	      \
  (rxbin == WANOPT_ADSL_RX_BIN_ENABLE)	? "Enable" :  \
  (rxbin == WANOPT_ADSL_RX_BIN_DISABLE) ? "Disable (Default)" : "Unknown"

#define ADSL_DECODE_FRAMING_STRUCT(framing)    \
  (framing == WANOPT_ADSL_FRAMING_TYPE_0) ? "Type 0" : \
  (framing == WANOPT_ADSL_FRAMING_TYPE_1) ? "Type 1" : \
  (framing == WANOPT_ADSL_FRAMING_TYPE_2) ? "Type 2" : \
  (framing == WANOPT_ADSL_FRAMING_TYPE_3) ? "Type 3 (Default)" : "Unknown"

#define ADSL_DECODE_EXPANDED_EXCHANGE(exchange)	\
  (exchange == WANOPT_ADSL_EXPANDED_EXCHANGE) ? "Expanded (Default)" : \
  (exchange == WANOPT_ADSL_SHORT_EXCHANGE)  ? "Short" : "Unknown"

#define ADSL_DECODE_CLOCK_TYPE(clock_type)  \
  (clock_type == WANOPT_ADSL_CLOCK_OSCILLATOR)	? "Oscilator" : \
  (clock_type == WANOPT_ADSL_CLOCK_CRYSTAL)	? "Crystal (Default)" : "Unknown"

///////////////////////////////////////////////////////////////////////

#define LEN_OF_DBG_BUFF   4096
#define MAX_ADDR_STR_LEN  100
#define MAX_USEDBY_STR_LEN  100

#define TIME_SLOT_GROUP_STR "Timeslot Group"
#define TIME_SLOT_GROUPS_STR "Timeslot Groups"

#define INVALID_HELP_TOPIC_STR "Invalid Help topic was selected!!!"

//global variables in main.cpp
extern const char * wanpipe_cfg_dir;

extern char* invalid_help_str;
extern char* fr_cir_help_str;
extern char* option_not_implemented_yet_help_str;
extern char* no_configuration_onptions_available_help_str;

extern char* interfaces_cfg_dir;

extern int global_hw_ec_max_num;

//taken from wanconfig.c:
extern char prognamed[20];
extern char progname_sp[];
extern char def_conf_file[];
extern char def_adsl_file[];
extern char tmp_adsl_file[];
extern char router_dir[];
extern char banner[];
extern char *krnl_log_file;
extern char *verbose_log;

//menu_list_all_wanpipes.cpp, 
//make sure initialize it each time "new conf_file_reader()" is called
extern void* global_conf_file_reader_ptr;
//when writing conf file, TDMV_SPAN, TDMV_DCHAN and all other 
//Voice related stuff should be written only if at least one interface
//is actually configured for VOICE
extern char is_there_a_voice_if;
//LIP ATM flag - if there is at least one such interface
extern char is_there_a_lip_atm_if;

extern int global_card_type;
extern int global_card_version;

#endif//WANCFG_H


