/***************************************************************************
                          wanctl.h  -  description
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

#ifndef WANCTL_H
#define WANCTL_H

#include "cpp_string.h" //class for strings handling. had to do it because
                        //C++ 'string' is NOT istalled on all systems.

//C includes:
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <ctype.h>

//have to do it to avoid conflict with 'string' typedef in 'string.h'
//and to use 'cpp_string'.
#define string cpp_string

//#include <time.h>
//#include <sys/time.h>

#if !defined(__LINUX__)
#define __LINUX__
#endif

#include <linux/sdla_fr.h>
#include <linux/wanpipe_cfg.h>
#include <linux/sdla_te1.h>

//in 'sdla_front_end.h' defined only for kernel code,
//so redefining here.
//not all of them used. the unused ones are commented out
/* settings for the 'adapter_type' */
#define S5141_ADPTR_1_CPU_SERIAL	0x0011	/* S5141, single CPU, serial */
//#define S5142_ADPTR_2_CPU_SERIAL	0x0012	/* S5142, dual CPU, serial */
#define S5143_ADPTR_1_CPU_FT1		0x0013	/* S5143, single CPU, FT1 */
#define S5144_ADPTR_1_CPU_T1E1		0x0014	/* S5144, single CPU, T1/E1 */
#define S5145_ADPTR_1_CPU_56K		0x0015	/* S5145, single CPU, 56K */
//#define S5147_ADPTR_2_CPU_T1E1		0x0017  /* S5147, dual CPU, T1/E1 */
//#define S5148_ADPTR_1_CPU_T1E1		0x0018	/* S5148, single CPU, T1/E1 */

#define S518_ADPTR_1_CPU_ADSL		0x0018	/* S518, adsl card */

//#define A100_ADPTR_T1E1_MASK		0x0040	/* T1/E1 type mask */
//#define A100_ADPTR_1_CHN_T1E1		0x0041	/* 1 Channel T1/E1 (Prototype) */
//#define A100_ADPTR_2_CHN_T1E1		0x0042	/* 2 Channels T1/E1 (Prototype) */
#define A101_ADPTR_1_CHN_T1E1		0x0044	/* 1 Channel T1/E1 */
//#define A101_ADPTR_2_CHN_T1E1		0x0045	/* 2 Channels T1/E1 */

//#define A100_ADPTR_T3E3_MASK		0x0080	/* T3/E3  type mask */
//#define A100_ADPTR_1_CHN_T3E3		0x0081	/* 1 Channel T3/E3 (Prototype) */
#define A102_ADPTR_1_CHN_T3E3		0x0082	/* 1 Channel T3/E3 */

#define S508_ADPTR              0x0508	/* S508 ISA card */

//#define MAX_PATH_LENGTH 2048
#define MAX_PATH_LENGTH 4096
#define NO  1
#define YES 2

//end of settings for the 'adapter_type'
//////////////////////////////////////////////////////////////////////////
#define MAX_LEN_OF_ACTIVE_CHANNELS_STRING 100

typedef struct chan_def			/* Logical Channel definition */
{
	char name[WAN_IFNAME_SZ+1];	/* interface name for this channel */
	char* addr;			/* -> media address */
	char* conf;			/* -> configuration data */
	char* conf_profile;		/* -> protocol profile data */
	char* usedby;			/* used by WANPIPE or API */
	char  annexg;			/* -> anexg interface */
	char* label;			/* -> user defined label/comment */
	char* virtual_addr;		/* -> X.25 virtual addr */
	char* real_addr;		/* -> X.25 real addr */
	wanif_conf_t  *chanconf;	/* channel configuration structure */
//	struct chan_def* next;		/* -> next channel definition */
  char chan_start_script;
  char chan_stop_script;
  char active_channels_string[MAX_LEN_OF_ACTIVE_CHANNELS_STRING];
  //FIXIT: variable to hold the one and only dlci_number supported
  //by current version of WANCONFIG_MFR on AFT card.
  //The 'addr' member can NOT be used because it holds the logical
  //channel number (for channelized device). This number is generated
  //when 'conf' file is read.
  char aft_dlci_number[WAN_IFNAME_SZ+1];
  char LIP_protocol;
  void* LIP_root_device;//points to the 'real' channelized device on AFT card. i.g.: wp1aft1
                        //if this is the 'real' channelized device device it self, this pointer
                        //is NULL.
} chan_def_t;

typedef struct link_def			/* WAN Link definition */
{
	char name[WAN_DRVNAME_SZ+1];	/* link device name */
	int config_id;			/* configuration ID */
	char* conf;			    /* -> configuration data */
	char* descr;			  /* -> description */
	unsigned long checksum;
	time_t modified;
//	chan_def_t* chan;		/* list of channel definitions */
	wandev_conf_t *linkconf;	/* link configuration structure */
//	struct link_def* next;		/* -> next link definition */
  char start_script;
  char stop_script;
  char card_version;  //S514 has many versions: S514-1/2/3/...7
  char active_channels_string[MAX_LEN_OF_ACTIVE_CHANNELS_STRING];
  char firmware_file_path[MAX_PATH_LENGTH];
} link_def_t;

//#define Debug(dbg_flag, message) if(dbg_flag){printf message;}
#define Debug(dbg_flag, message)

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

#define WANCFG_PROGRAM_NAME "WANPIPE Control Utility"
#define WANCFG_EXECUTABLE_NAME "wancfg_new"

#define INVALID_PROTOCOL "Invalid Protocol!!"
#define INVALID_CARD_TYPE 0

#define NOT_SET 100 //can be used for storage as small as 1 byte.
#define NOT_SET_STR "Not Set"

#define IF_NAME_NOT_INIT  "noname"
#define STATION_TYPE_NOT_KNOWN 0xfe

//it is the count of 'config_id' definitions in wanpipe_cfg.h
#define NUMBER_OF_PROTOCOLS 21

#define NO_OPER_MODE YES+1

//defines for clarifying the source code
#define IN  //value passed to a function
#define OUT //function sets value on return

#define LXDIALOG_OUTPUT_FILE_NAME "lxdialog_output"

//SELECTION can be a button number or a selected menu number.
#define SELECTION_0  0
#define SELECTION_1  1
#define SELECTION_2  2

//delimeter for functions with variable length parameter list
#define END_OF_PARAMS_LIST "END_OF_PARAMS_LIST"

#define MAX_NUMBER_OF_WANPIPES  16
#define MAX_NUMBER_OF_NET_INTERFACES_PER_WANPIPE  512
#define WANPIPE_TOKEN   "wanpipe"

//Operation Mode (Used by):
#define OP_MODE_WANPIPE     " \"WANPIPE\" "     //1
#define OP_MODE_API         " \"API\" "         //2
#define OP_MODE_BRIDGE      " \"BRIDGE\" "      //3
#define OP_MODE_BRIDGE_NODE " \"BRIDGE_NODE\" " //4
#define OP_MODE_SWITCH      " \"SWITCH\" "      //5
#define OP_MODE_STACK       " \"STACK\" "       //6
#define OP_MODE_ANNEXG      " \"ANNEXG\" "      //7
#define OP_MODE_VoIP        " \"VoIP\" "        //8
#define OP_MODE_PPPoE       " \"PPPoE\" "       //9

#define PPPoE VoIP+1 //used only to skip IP address configuration on ADSL card

#define QUOTED_1     " \"1\" "
#define QUOTED_2     " \"2\" "
#define QUOTED_3     " \"3\" "
#define QUOTED_4     " \"4\" "
#define QUOTED_5     " \"5\" "
#define QUOTED_6     " \"6\" "
#define QUOTED_7     " \"7\" "
#define QUOTED_8     " \"8\" "
#define QUOTED_9     " \"9\" "

//Frame Relay definitions
#define MAX_DLCI_NUMBER   100
#define MIN_CIR 1
#define MAX_CIR 512
#define MIN_BC  1
#define MAX_BC  512
#define MIN_BE  0
#define MAX_BE  512

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

char* get_station_string(unsigned char protocol, unsigned char station_type);

int check_file_exist(char * file_name, FILE ** file);
char * get_card_type_string(int card_type);

int get_wanpipe_status(char* wanpipe_name);
int start_wanpipe();
int stop_wanpipe();
int run_system_command(char* command_line);
int repeat_command();

////////////////////////////////////////////////////////////////////////////////

char check_wanpipe_start_script_exist(char* wanpipe_name);
char check_wanpipe_stop_script_exist(char* wanpipe_name);

int create_wanpipe_start_script(char* wanpipe_name);
int create_wanpipe_stop_script(char* wanpipe_name);

void remove_wanpipe_start_script(char* wanpipe_name);
void remove_wanpipe_stop_script(char* wanpipe_name);

int edit_wanpipe_start_script(char* wanpipe_name);
int edit_wanpipe_stop_script(char* wanpipe_name);

////////////////////////////////////////////////////////////////////////////////
char check_net_interface_start_script_exist(char* wanpipe_name, char* if_name);
char check_net_interface_stop_script_exist(char* wanpipe_name, char* if_name);

int create_new_net_interface_start_script(char* wanpipe_name, char* if_name);
int create_new_net_interface_stop_script(char* wanpipe_name, char* if_name);

void remove_net_interface_start_script(char* wanpipe_name, char* if_name);
void remove_net_interface_stop_script(char* wanpipe_name, char* if_name);

int edit_net_interface_start_script(char* wanpipe_name, char* if_name);
int edit_net_interface_stop_script(char* wanpipe_name, char* if_name);

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
int get_used_by_integer_value(char* used_by_str);
char* get_used_by_string(int used_by);
char* replace_new_line_with_zero_term(char* str);
char* replace_new_line_with_space(char* str);
char* get_S514_card_version_string(char card_version);
void tokenize_string(char* input_buff, char* delimeter_str, char* output_buff, int buff_length);
char* replace_char_with_other_char_in_str(char* str, char old_char, char new_char);
char* get_date_and_time();
unsigned long parse_active_channel(char* val, unsigned char media_type);
unsigned long get_active_channels(int channel_flag, int start_channel,
                                  int stop_channel, unsigned char media_type);
int check_channels(int channel_flag, unsigned int start_channel,
                   unsigned int stop_channel, unsigned char media_type);

char* validate_ipv4_address_string(char* str);
char* validate_authentication_string(char* str, int max_length);
///////////////////////////////////////////////////////////////////////
//errors display
void err_printf(char* format, ...);
#define ERR_DBG_OUT(_x_)                                              \
{ printf("%s: Error in File: %s, Function: %s(), Line: %d. Text:\n",  \
    prognamed, __FILE__, __FUNCTION__, __LINE__);                     \
  printf _x_;                                                         \
}
///////////////////////////////////////////////////////////////////////

#define LEN_OF_DBG_BUFF   4096
#define MAX_ADDR_STR_LEN  100
#define MAX_USEDBY_STR_LEN  100

#define TIME_SLOT_GROUP_STR "Timeslot Group"
#define TIME_SLOT_GROUPS_STR "Timeslot Groups"

#define INVALID_HELP_TOPIC_STR "Invalid Help topic was selected!!!"

//global variables in main.cpp
extern char * wanpipe_cfg_dir;
extern char * wancfg_cfg_dir;

extern char* invalid_help_str;
extern char* fr_cir_help_str;
extern char* option_not_implemented_yet_help_str;

extern char* interfaces_cfg_dir;
extern char* wanctl_template_dir;

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

///////////////////////////////////////

enum GENERAL_RETURN_CODES{
  FILE_DOES_NOT_EXIST= 1,
  UNKNOWN_PROTOCOL,
  UNKNOWN_STATION
};

#define WANPIPE_NOT_RUNNING 0
#define WANPIPE_RUNNING     1

#define SPACE_BAR 0x20  //0x02
#define ENTER_KEY 0x0A

#endif//WANCTL_H
