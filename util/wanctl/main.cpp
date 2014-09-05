/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Wed Feb 25 16:17:53 EST 2004
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DBG_WANCFG_MAIN 0

#include <ctype.h>

#include "wanctl.h"
#include "menu_base.h"
#include "dialog_yes_no.h"
#include "text_box.h"
#include "text_box_yes_no.h"

#include "text_box_help.h"

#include "conf_file_reader.h"

#include "list_element_chan_def.h"
#include "objects_list.h"

#include "menu_main_configuration_options.h"

//globals
char * lxdialog_path = "/usr/sbin/wanpipe_lxdialog";
char * wanpipe_cfg_dir = "/etc/wanpipe/";
char * wancfg_cfg_dir = "/etc/wanpipe/wancfg_templates/";
char * start_stop_scripts_dir = "scripts";

char * interfaces_cfg_dir = "/etc/wanpipe/interfaces/";

char * wanctl_template_dir = "/etc/wanpipe/wanctl_templates/";
char wanrouter_list_file_full_path[MAX_PATH_LENGTH];
char tmp_hwprobe_file_full_path[MAX_PATH_LENGTH];
int wanpipe_number = 1;

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//help messages
//indicates error.
char* invalid_help_str =
"Invalid Selection!!!\n"
"--------------------\n";

char* option_not_implemented_yet_help_str =
"Selected option not implemented yet.\n";


//Frame Relay
char* fr_cir_help_str =
"  Frame Relay: Committed Information Rate (CIR)\n"
"  ---------------------------------------------\n"
"  \n"
"  Enable or Disable Committed Information Rate\n"
"  on the board.\n"
"  Valid values: 1 - 512 kbps.\n"
"  \n"
"  Default: Disabled";


////////////////////////////////////////////////
//globals from wanconfig.c
char prognamed[20] =	"wanctl";
char progname_sp[] = 	"         ";
char def_conf_file[] =	"/etc/wanpipe/wanpipe1.conf";	/* default name */
char def_adsl_file[] =	"/etc/wanpipe/wan_adsl.list";	/* default name */
char tmp_adsl_file[] =	"/etc/wanpipe/wan_adsl.tmp";	/* default name */
#if defined(__LINUX__)
char router_dir[] =	"/proc/net/wanrouter";	/* location of WAN devices */
#else
char router_dir[] =	"/var/lock/wanrouter";	/* location of WAN devices */
#endif

char banner[] =		"WAN Router Configurator"
			"(c) 1995-2003 Sangoma Technologies Inc."
;

char *krnl_log_file = "/var/log/messages";
char *verbose_log = "/var/log/wanrouter";
///////////////////////////////////////////////

int active_channels_str_invalid_characters_check(char* active_ch_str);
int is_TE1_card(char * str_buff);
int hardware_probe(int* card_type);

///////////////////////////////////////////////

//convert int definition of a protocol to string
char * get_protocol_string(int protocol)
{
  volatile static char protocol_name[MAX_PATH_LENGTH];

  switch(protocol)
  {
  case PROTOCOL_NOT_SET:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Not Set");
    break;

  case NO_PROTOCOL_NEEDED:
    //special case, when no protocol name actually needed
    protocol_name[0] = '\0';
    break;

  case WANCONFIG_X25:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "X25");
    break;

  case WANCONFIG_FR:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Frame Relay");
    break;

  case WANCONFIG_PPP:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "PPP");
    break;

  case WANCONFIG_CHDLC:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Cisco HDLC");
    break;

  case WANCONFIG_BSC:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "BiSync Streaming");
    break;

  case WANCONFIG_HDLC:
    //used with CHDLC firmware
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "HDLC Streaming");
    break;

  case WANCONFIG_MPPP://and WANCONFIG_MPROT too
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Multi Port PPP");
    break;

  case WANCONFIG_BITSTRM:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Bit Stream");
    break;

  case WANCONFIG_EDUKIT:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Educational Kit");
    break;

  case WANCONFIG_SS7:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "SS7");
    break;

  case WANCONFIG_BSCSTRM:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Bisync Streaming Nasdaq");
    break;

  case WANCONFIG_MFR:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Multi-Port Frame Relay");
    break;

  case WANCONFIG_ADSL:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "LLC Ethernet Support (ADSL)");
    break;

  case WANCONFIG_SDLC:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "SDLC");
    break;

  case WANCONFIG_ATM:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "ATM");
    break;

  case WANCONFIG_POS:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Point-of-Sale");
    break;

  case WANCONFIG_AFT:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "AFT");
    break;

  case WANCONFIG_DEBUG:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Real Time Debugging");
    break;

  case WANCONFIG_ADCCP:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Special HDLC LAPB");
    break;

  case WANCONFIG_MLINK_PPP:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Multi-Link PPP");
    break;

  case WANCONFIG_GENERIC:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "WANPIPE Generic driver");
    break;

  case WANCONFIG_MPCHDLC:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Multi-Port CHDLC");
    break;

  default:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, INVALID_PROTOCOL);
    break;
  }

  return (char*)protocol_name;
}

//for different protocols the "Station string" may be different.
//but for simplisity only two variations will be used.
char* get_station_string(unsigned char protocol, unsigned char station_type)
{
  /////////////////////////////////////////////////
  if(station_type == STATION_TYPE_NOT_KNOWN){
    //new configuration
    return "Not Set";
  }
/*
  switch(protocol)
  {
  case WANCONFIG_ATM:
  case WANCONFIG_CHDLC:
  case WANCONFIG_PPP:
    return "Not applicable";
  }
*/
  switch(station_type)
  {
  case WANOPT_CPE:
    return "Local (CPE)";

  case WANOPT_NODE:
    return "Telco (Switch)";

  default:
    return "Unknown";
  }
}

int check_file_exist(char * file_name, FILE ** file)
{
  int rc;

  *file = fopen(file_name, "r+");

  if(*file == NULL){
    Debug(DBG_WANCFG_MAIN, ("File '%s' does not exist.\n", file_name));
    rc = NO;
  }else{
    Debug(DBG_WANCFG_MAIN, ("File '%s' exist.\n", file_name));
    rc = YES;
    fclose(*file);
  }
  return rc;
}

//////////////////////////////////////////////////////////

int write_string_to_file(char * full_file_path, char* string)
{
  FILE * file;

  Debug(DBG_WANCFG_MAIN, ("write_string_to_file():\n path: %s\n str: %s\n",
    full_file_path, string));

  file = fopen(full_file_path, "w");
  if(file == NULL){
    ERR_DBG_OUT(("Failed to open %s file for writing!\n", full_file_path));
    return NO;
  }

  fputs(string, file);
  fclose(file);
  return YES;
}

//////////////////////////////////////////////////////////

//function used for printing errors in the release version.
void err_printf(char* format, ...)
{
  char dbg_tmp_buff[LEN_OF_DBG_BUFF];
	va_list ap;

	va_start(ap, format);
  vsnprintf(dbg_tmp_buff, LEN_OF_DBG_BUFF, format, ap);
	va_end(ap);

  printf(dbg_tmp_buff);
  printf("\n");
}


//caller must be sure 'input' string is actually an integer.
char* remove_spaces_in_int_string(char* input)
{
  char output [LEN_OF_DBG_BUFF];

  //remove spaces and zero terminate.
  snprintf(output, LEN_OF_DBG_BUFF, "%d", atoi(input));
  //copy back to caller's buffer
  strcpy(input, output);

  return input;
}

int yes_no_question(  OUT int* selection_index,
                      IN char * lxdialog_path,
                      IN int protocol,
                      IN char* format, ...)
{
  FILE * yes_no_question_file;
  char yes_no_question_buff[LEN_OF_DBG_BUFF];
	va_list ap;
  char* yes_no_question_file_name = "wancfg_yes_no_question_file_name";
  //char* remove_yes_no_question_file = "rm -rf wancfg_yes_no_question_file_name";
  int rc = YES;
  char file_path[MAX_PATH_LENGTH];
  char remove_tmp_file[MAX_PATH_LENGTH];

  text_box_yes_no yn_tb;

  snprintf(file_path, MAX_PATH_LENGTH, "%s%s", wancfg_cfg_dir,
                                               yes_no_question_file_name);

  snprintf(remove_tmp_file, MAX_PATH_LENGTH, "rm -rf %s", file_path);

	va_start(ap, format);
  vsnprintf(yes_no_question_buff, LEN_OF_DBG_BUFF, format, ap);
	va_end(ap);

  //open temporary file
  yes_no_question_file = fopen(file_path, "w");
  if(yes_no_question_file == NULL){
    ERR_DBG_OUT(("Failed to open 'yes_no_question_file' %s file for writing!\n",
      file_path));
    return NO;
  }

  fputs(yes_no_question_buff, yes_no_question_file);
  fclose(yes_no_question_file);

  if(yn_tb.set_configuration( lxdialog_path,
                              protocol,
                              yes_no_question_file_name) == NO){
    rc = NO;
    goto cleanup;
  }

  if(yn_tb.show(selection_index) == NO){
    rc = NO;
    goto cleanup;
  }

cleanup:
  //remove the temporary file
  system(remove_tmp_file);

  return rc;
}

//converts all characters in the string to lower case.
void str_tolower(char * str)
{
  unsigned int i;

  //Debug(DBG_WANCFG_MAIN, "str before str_tolower: %s\n", str);

  for(i = 0; i < strlen(str); i++){
    str[i] = (char)tolower(str[i]);
  }

  //Debug(DBG_WANCFG_MAIN, "str after str_tolower: %s\n", str);
}

//converts all characters in the string to upper case.
void str_toupper(char * str)
{
  unsigned int i;

  //Debug(DBG_WANCFG_MAIN, "str before str_toupper: %s\n", str);

  for(i = 0; i < strlen(str); i++){
    str[i] = (char)toupper(str[i]);
  }

  //Debug(DBG_WANCFG_MAIN, "str after str_toupper: %s\n", str);
}

char* replace_new_line_with_zero_term(char* str)
{
  unsigned int i;

  for(i = 0; i < strlen(str); i++){
    if(str[i] == '\n'){
      str[i] = '\0';
    }
  }
  return str;
}

char* replace_new_line_with_space(char* str)
{
  unsigned int i;

  for(i = 0; i < strlen(str); i++){
    if(str[i] == '\n'){
      str[i] = ' ';
    }
  }
  return str;
}

void tokenize_string(char* input_buff, char* delimeter_str, char* output_buff, int buff_length)
{
  char* p = strtok(input_buff, delimeter_str);

  output_buff[0] = '\0';

  while(p != NULL){
    //printf(p);
    snprintf(&output_buff[strlen(output_buff)], buff_length - strlen(output_buff), p);
    p = strtok(NULL, delimeter_str);
  }
  printf("\ntokenized str: %s\n", output_buff);
}

char* replace_char_with_other_char_in_str(char* str, char old_char, char new_char)
{
  unsigned int i;

  for(i = 0; i < strlen(str); i++){
    if(str[i] == old_char){
      str[i] = new_char;
    }
  }
  return str;
}

//////////////////////////////////////////////////////////
char* get_date_and_time()
{
  int system_exit_status;
  static char date_and_time_str[MAX_PATH_LENGTH];
  char command_line[MAX_PATH_LENGTH];
  char* date_and_time_file_name = "date_and_time_file.tmp";
  FILE* date_and_time_file;

  snprintf(command_line, MAX_PATH_LENGTH, "date > %s", date_and_time_file_name);
  Debug(DBG_WANCFG_MAIN, ("get_date_and_time() command line: %s\n", command_line));

  system_exit_status = system(command_line);
  system_exit_status = WEXITSTATUS(system_exit_status);

  if(system_exit_status != 0){
    ERR_DBG_OUT(("The following command failed: %s\n", command_line));
    return "Unknown";
  }

  date_and_time_file = fopen(date_and_time_file_name, "r+");
  if(date_and_time_file == NULL){
    ERR_DBG_OUT(("Failed to open the file '%s' for reading!\n",
      date_and_time_file_name));
    return "Unknown";
  }

  do{
    fgets(date_and_time_str, MAX_PATH_LENGTH, date_and_time_file);

    if(!feof(date_and_time_file)){
      Debug(DBG_WANCFG_MAIN, ("date_and_time_str: %s\n", date_and_time_str));
    }//if()
  }while(!feof(date_and_time_file));

  return date_and_time_str;
}

//returns:  NULL - if string is valid
//          pointer to err explanation if string is invalid
char* validate_ipv4_address_string(char* str)
{
  char* token;
  char seps[] = ".";
  int field_counter = 0;
  static char err_buf[MAX_PATH_LENGTH];

#define MAX_IPV4_LEN_STR  3*4+4
#define MIN_IPV4_LEN_STR  3*1+4

  if(strlen(str) > MAX_IPV4_LEN_STR){
    snprintf(err_buf, MAX_PATH_LENGTH,
      "Number of characters in IP string is greater than %d!\n", MAX_IPV4_LEN_STR);
    return err_buf;
  }
  if(strlen(str) < MIN_IPV4_LEN_STR){
    snprintf(err_buf, MAX_PATH_LENGTH,
      "Number of characters in IP string is less than %d!\n", MIN_IPV4_LEN_STR);
    return err_buf;
  }

  token = strtok((char*)str, seps);
  while(token != NULL){
    Debug(DBG_WANCFG_MAIN, ("token: %s\n", token));

    if(strlen(token) > 3){
      snprintf(err_buf, MAX_PATH_LENGTH,
        "Invalid number of characters in IP string field (%s).\n", token);
      return err_buf;
    }

    for(unsigned int i = 0; i < strlen(token); i++){
      if(isdigit(token[i]) == 0){
        snprintf(err_buf, MAX_PATH_LENGTH,
          "Invalid characters in IP string field (%s).\n", token);
        return err_buf;
      }
    }

    field_counter++;
    token = strtok(NULL, seps);
  }

  if(field_counter != 4){
    snprintf(err_buf, MAX_PATH_LENGTH,
      "Invalid number of \".\" separated fields in IP string.\n");
    return err_buf;
  }
  return NULL;
}

int run_system_command(char* command_line)
{
  int system_exit_status;

  Debug(DBG_WANCFG_MAIN, ("run_system_command(): command line: %s\n", command_line));

  system("clear");

  system_exit_status = system(command_line);
  if(WIFEXITED(system_exit_status) == 0){
    //command crashed or killed
    ERR_DBG_OUT(("Command '%s' exited abnormally (sytem_call_status: %d).\n",
      command_line, system_exit_status));
    exit(1);
  }

  system_exit_status = WEXITSTATUS(system_exit_status);
  return system_exit_status;
}

//return - Running or Not
int get_wanpipe_status(char* wanpipe_name)
{
  int rc;
  FILE * list_file;
  char str_buff[MAX_PATH_LENGTH];

  sprintf(str_buff, "wanrouter list > %s", wanrouter_list_file_full_path);

  if(run_system_command(str_buff)){
    ERR_DBG_OUT(("Command '%s' failed!\n", str_buff));
    exit(1);
  }

  list_file = fopen(wanrouter_list_file_full_path, "r+");
  if(list_file == NULL){
    Debug(DBG_WANCFG_MAIN, ("Failed to open list file %s!\n", wanrouter_list_file_full_path));
    rc = WANPIPE_NOT_RUNNING;
    goto done;
  }

  do{
    fgets(str_buff, MAX_PATH_LENGTH, list_file);

    if(!feof(list_file)){

      if(strstr(str_buff, wanpipe_name) != NULL){

        Debug(DBG_WANCFG_MAIN, ("%s Found in list of active WANPIPEs.\n", wanpipe_name));
        rc = WANPIPE_RUNNING;
        goto done;
      }
    }

  }while(!feof(list_file));

  Debug(DBG_WANCFG_MAIN, ("%s NOT Found in list of active WANPIPEs.\n", wanpipe_name));
  fclose(list_file);
  rc = WANPIPE_NOT_RUNNING;

done:

  snprintf(str_buff, MAX_PATH_LENGTH, "rm %s > /dev/null", wanrouter_list_file_full_path);
  run_system_command(str_buff);

  return rc;
}

int start_wanpipe()
{
  int system_rc;
  char shell_command_line[MAX_PATH_LENGTH];

  //sprintf(shell_command_line, "wanrouter start wanpipe%d > /dev/null", wanpipe_number);
  sprintf(shell_command_line, "wanrouter start wanpipe%d", wanpipe_number);
  system_rc = run_system_command(shell_command_line);
  if(system_rc){
    ERR_DBG_OUT(("\nFailed to start wanpipe%d. Please make sure the card is NOT in use\n\
and WANPIPE is installed properly. Also check system Message Log.\n\n", wanpipe_number));
    exit(1);
  }
  return system_rc;
}

int stop_wanpipe()
{
  int system_rc;
  char shell_command_line[MAX_PATH_LENGTH];

  //sprintf(shell_command_line, "wanrouter stop wanpipe%d > /dev/null", wanpipe_number);
  sprintf(shell_command_line, "wanrouter stop wanpipe%d", wanpipe_number);
  system_rc = system(shell_command_line);
  if(system_rc){
    ERR_DBG_OUT(("\nFailed to stop wanpipe%d. Please stop it manualy.\n\n", wanpipe_number));
    exit(1);
  }
  return system_rc;
}

//HW probe returns all cards.
//We are interested only in S514 cards.
//Must be maximum one card (discarding secondary ports).
int hardware_probe(int* card_type)
{
  int system_rc;
  FILE * hwprobe_file_tmp;
  char str_buff[MAX_PATH_LENGTH];
  char shell_command_line[MAX_PATH_LENGTH];
  int probed_cards_count=0;

  sprintf(shell_command_line, "wanrouter hwprobe > %s", tmp_hwprobe_file_full_path);

  Debug(DBG_WANCFG_MAIN, ("hardware_probe(): shell_command_line : %s\n",
    shell_command_line));

  system_rc = run_system_command(shell_command_line);
  if(system_rc){
    ERR_DBG_OUT(("Failed to detect Sangoma hardware! (system_rc : 0x%X)\n", system_rc));
    return NO;
  }

  hwprobe_file_tmp = fopen(tmp_hwprobe_file_full_path, "r+");
  if(hwprobe_file_tmp == NULL){
    ERR_DBG_OUT(("Failed to open %s file for reading!!\n", tmp_hwprobe_file_full_path));
    return NO;
  }

  do{
    fgets(str_buff, MAX_PATH_LENGTH, hwprobe_file_tmp);

    if(!feof(hwprobe_file_tmp)){

      //filter out all "secondary" port cards
      if( strstr(str_buff, "PORT=SEC") == NULL &&
          strstr(str_buff, "PORT=PRI") != NULL){// &&
          //is_TE1_card(str_buff) == YES){

        char * S514_str = strstr(str_buff, "S514");
        if(S514_str != NULL){

          sprintf(str_buff, "%d. %s", ++probed_cards_count, S514_str);
          Debug(DBG_WANCFG_MAIN, ("%s\n", str_buff));
          //break;//if want to exit on the first probed card
        }
      }
    }
  }while(!feof(hwprobe_file_tmp));

  if(probed_cards_count == 0){
    ERR_DBG_OUT(("Failed to detect S5141/S5142/S5144/S5147/S5148 cards! (probed_cards_count : %d)\n",
      probed_cards_count));
    return NO;
  }

  if(probed_cards_count > 1){
    ERR_DBG_OUT(("Detected more than one S514 card! (probed_cards_count : %d)\n",
      probed_cards_count));
    return NO;
  }

  /////////////////////////////////////////////////////
  //now, when it is known that number of cards is valid (one card)
  //check the type of the card
  do{
    fgets(str_buff, MAX_PATH_LENGTH, hwprobe_file_tmp);

    if(!feof(hwprobe_file_tmp)){

      //filter out all "secondary" port cards
      if( strstr(str_buff, "PORT=SEC") == NULL &&
          strstr(str_buff, "PORT=PRI") != NULL &&
          strstr(str_buff, "S514") != NULL){

        if(is_TE1_card(str_buff) == YES){

          Debug(DBG_WANCFG_MAIN, ("TE1 card detected\n"));
          *card_type = S5144_ADPTR_1_CPU_T1E1;
        }else{
          Debug(DBG_WANCFG_MAIN, ("Serial card detected\n"));
          *card_type = S5141_ADPTR_1_CPU_SERIAL;
        }
      }
    }//if()
  }while(!feof(hwprobe_file_tmp));

  fclose(hwprobe_file_tmp);

  return YES;
}

int is_TE1_card(char * str_buff)
{
  Debug(DBG_WANCFG_MAIN, ("is_TE1_card(): str_buff: %s\n", str_buff));

  if( strstr(str_buff, "S514-4") != NULL){
    return YES;
  }

  if( strstr(str_buff, "S514-7") != NULL){
    return YES;
  }

  if( strstr(str_buff, "S514-8") != NULL){
    return YES;
  }

  return NO;
}

#if 1

int main(int argc, char *argv[])
{
  int selection_index, rc, card_type;

  ///////////////////////////////////////////////////////////////////////////
  //initialize globals
  snprintf(wanrouter_list_file_full_path, MAX_PATH_LENGTH, "%slist.tmp",
    wanctl_template_dir);

  snprintf(tmp_hwprobe_file_full_path, MAX_PATH_LENGTH, "%scard_list.tmp",
    wanctl_template_dir);
  ///////////////////////////////////////////////////////////////////////////
  
  conf_file_reader cfr;
  menu_main_configuration_options main_menu_box(lxdialog_path, &cfr);

  Debug(DBG_WANCFG_MAIN, ("%s: main()\n", WANCFG_PROGRAM_NAME));

again:

#if DBG_WANCFG_MAIN
  //card_type = S5141_ADPTR_1_CPU_SERIAL;
  card_type = S5144_ADPTR_1_CPU_T1E1;
#else
  if(hardware_probe(&card_type) == NO){
    return 1;
  }
#endif
  
  cfr.card_type = card_type;

  /////////////////////////////////////////////////////////////////////////////////
  //try to read the default conf file 'wanpipe1.conf'
  rc = cfr.read_conf_file();
  switch(rc)
  {
  case 0:
    //defualt conf file exist and was successfuly parsed
    Debug(DBG_WANCFG_MAIN, ("protocol: %d, station_type: %d\n",
      cfr.protocol, cfr.station_type));

    /////////////////////////////////////////////////////////////////////////////////
    if(cfr.form_interface_name(cfr.protocol) == NO){
      return 1;
    }
    Debug(DBG_WANCFG_MAIN, ("cfr.if_name: %s\n", cfr.if_name));
  case FILE_DOES_NOT_EXIST:
    //file does not exist - not an error
    break;

  default:
    Debug(DBG_WANCFG_MAIN, ("read_conf_file() failed. rc: %d\n", rc));
    return rc;
  }

  /////////////////////////////////////////////////////////////////////////////////

  if(main_menu_box.run(&selection_index) == NO){
    Debug(DBG_WANCFG_MAIN, ("main(): main_menu_box.run() - failed!!\n"));
    goto cleanup;
  }

  switch(selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
  case MENU_BOX_BUTTON_HELP:
    goto again;
    break;

  case MENU_BOX_BUTTON_EXIT:
    //exit the program
    break;

  default:
    ERR_DBG_OUT(("Unknown selection_index: %d\n", selection_index));
    goto cleanup;
  }

cleanup:
  return EXIT_SUCCESS;
}
#endif

/////////////////////////////////////////////////////////////////////////
//code from 'unixio.c'
//kbdhit()
#include "unixio.h"
#include <stdlib.h>
#include <ctype.h>
#include <termcap.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>

#include <unistd.h>
#include <signal.h>
#include <termios.h>

static enum {g_RAW, g_CBREAK, g_RESET} tty_status = g_RESET;
static int no_block = 0;

static int timeout(void);

#define ESC 0X1B

/*
 * kbdhit :
 * return non-zero if a key on the keyboard was hit; 0 otherwise
 * the key that was typed will be stored in input iff it is a valid character
 * otherwise -1 is stored in input: valid characters are those that fit
 * into one char; if input is null nothing is stored in input
 * might be a good idea to turn off echo when using this function; you can
 * turn echo back on when you need to
 * IN  : pointer to variable to store input
 * OUT : non-zero if keyboard was hit, 0 if not; IOERROR on error
 */
int
kbdhit(int *input)
{
	unsigned char ch;
	int n, retval;
	struct termios oattr, attr;


    if(tty_status == g_RESET) {
        if((tcgetattr)(STDIN_FILENO,&oattr) < 0)
            return IOERROR;
        attr = oattr;
        attr.c_lflag &= ~(ICANON | ECHO);
        attr.c_cc[VMIN] = 1;
        attr.c_cc[VTIME] = 0;
        if((tcsetattr)(STDIN_FILENO,TCSAFLUSH,&attr) < 0)
            return IOERROR;
    } else if(!(isatty)(STDIN_FILENO))
        return IOERROR;

    no_block = 1;
	if((n = timeout()) < 0)
		return IOERROR;
    retval = n;
	if(retval) {
		if((read)(STDIN_FILENO,&ch,1) != 1)
			return IOERROR;
        if(input)
            *input = ch;
		if(ch == ESC) {
			if((n = timeout()) < 0)
				return IOERROR;
			if(n) {
				do {
					if((read)(STDIN_FILENO,&ch,1) != 1)
						return IOERROR;
					if((n = timeout()) < 0)
						return IOERROR;
				} while(n);
				if(input)
					*input = (-1);
			}
		}
	}
    if(tty_status == g_RESET)
        if((tcsetattr)(STDIN_FILENO,TCSAFLUSH,&oattr) < 0)
            return IOERROR;

	return retval;
}

static int
timeout(void)
{
	struct timeval tv;
	fd_set readfds;
	int n;


    if(!(isatty)(STDIN_FILENO))
        return IOERROR;

	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO,&readfds);

	tv.tv_sec = TIMEOUT_SEC;
	tv.tv_usec = TIMEOUT_USEC + no_block;

	if((n = (select)(1,&readfds,(fd_set*)NULL,(fd_set*)NULL,&tv)) < 0)
		return IOERROR;

	return n;
}
/////////////////////////////////////////////////////////////////////////

//ask user repeat command or exit.
int repeat_command()
{
  int input;

  //printf("\nPress ENTER to exit. SPACE to repeat the command.\n");
   printf("\nPress ENTER to return to menu / SPACE to repeat\n");

user_input:

  input = 0;
  while(kbdhit(&input) == 0){
    ;//do nothing
  }

  //printf("input : 0x%X\n", input);
  //printf("input : %c\n", tolower(input));

  switch(input)
  {
  case SPACE_BAR:
    return YES;

  case ENTER_KEY:
    return NO;

  default:
    //some other key - wait for new input
    goto user_input;
  }
  return NO;
}
