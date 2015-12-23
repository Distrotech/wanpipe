/***************************************************************************
                          menu_main_configuration_options.cpp  -  description
                             -------------------
    begin                : Tue Mar 2 2004
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

#include "menu_main_configuration_options.h"
#include "text_box.h"

#include "menu_protocol_mode_select.h"
#include "net_interface_file_reader.h"
#include "menu_net_interface_ip_configuration.h"
#include "menu_select_station_type.h"
#include "menu_select_trace_cmd.h"
#include "menu_select_status_statistics_cmd.h"

#define DBG_MAIN_MENU_OPTIONS 1


enum MAIN_CFG_OPTIONS{

  EMPTY_LINE,
  PROTOCOL_SELECT,
  STATION_TYPE,
  IP_CONFIG,
  START_WANPIPE,
  STOP_WANPIPE,
  TRACE_CMDS,
  STATUS_STATS_CMDS,
  WANPIPE_STATUS_LINE
};

menu_main_configuration_options::menu_main_configuration_options( IN char * lxdialog_path,
                                                                  IN conf_file_reader* cfr)
{
  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = cfr;
}

menu_main_configuration_options::~menu_main_configuration_options()
{
}

int menu_main_configuration_options::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int option_selected;
  char exit_dialog;
  //help text box
  text_box tb;
  int number_of_items = 9;

  unsigned char old_protocol;
  unsigned char old_station_type;
  unsigned char new_configuration;

  list_element_chan_def chan_def;

  char backtitle[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "%s: ", WANCFG_PROGRAM_NAME);
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s", get_protocol_string(NO_PROTOCOL_NEEDED));

  Debug(DBG_MAIN_MENU_OPTIONS, ("menu_net_interface_setup::run()\n"));

  //deside once and forever if it is a new configuraton. (no default conf file)
  if(cfr->protocol == PROTOCOL_NOT_SET || cfr->station_type == STATION_TYPE_NOT_KNOWN){
    new_configuration = YES;
  }else{
    new_configuration = NO;
  }

  old_protocol = cfr->protocol;
  old_station_type = cfr->station_type;

again:

  rc = YES;
  option_selected = 0;
  exit_dialog = NO;
  menu_str = " ";

  //if configuration file exist and valid, we can do the following:
  //  1. protocol - allows to read IP configuration files
  //  2. station type - only for display

  strcpy(chan_def.data.name, cfr->if_name);
  net_interface_file_reader interface_file_reader(cfr->if_name);

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", PROTOCOL_SELECT);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Protocol---------> %s\" ",
    get_protocol_string(cfr->protocol));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////

  switch(cfr->protocol)
  {
  case WANCONFIG_ATM:
  case WANCONFIG_CHDLC:
  case WANCONFIG_PPP:
  case WANCONFIG_FR:
  case WANCONFIG_X25:
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", STATION_TYPE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Router Location--> %s\" ",
      get_station_string(cfr->protocol, cfr->station_type));
    menu_str += tmp_buff;
    break;
  }

  if(new_configuration == YES){

    number_of_items = 2;

    //check that user provided new settings
    if(cfr->protocol != PROTOCOL_NOT_SET && cfr->station_type != STATION_TYPE_NOT_KNOWN){
      new_configuration = NO;

      old_protocol = cfr->protocol;
      old_station_type = cfr->station_type;

      //copy new configuration to default location
      if(copy_new_configuration_to_default_location(  cfr->protocol,
                                                      cfr->station_type,
                                                      cfr->card_type) == NO){
        return NO;
      }
    }
  }

  /////////////////////////////////////////////////
  if(new_configuration == NO){

    if(old_protocol != cfr->protocol || old_station_type != cfr->station_type){
      old_protocol = cfr->protocol;
      old_station_type = cfr->station_type;

      //copy new configuration to default location
      if(copy_new_configuration_to_default_location(  cfr->protocol,
                                                      cfr->station_type,
                                                      cfr->card_type) == NO){
        return NO;
      }
    }

    if(interface_file_reader.parse_net_interface_file() == NO){

      ERR_DBG_OUT(("Failed to parse 'Net Interface' file for %s!!\n",
        cfr->if_name));
      return NO;
    }

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", IP_CONFIG);
    menu_str += tmp_buff;

    /////////////////////////////////////////////////////////////////
    //file was parsed, check the contents is complete
    if( strlen(interface_file_reader.if_config.ipaddr) <  MIN_IP_ADDR_STR_LEN ||
        strlen(interface_file_reader.if_config.point_to_point_ipaddr) < MIN_IP_ADDR_STR_LEN){
                                                         //  v NO SPACE HERE!!
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"IP Configuration-> %s\" ", "Incomplete");
      cfr->ip_cfg_complete = NO;
    }else{
      //ip cfg is complete
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"IP Configuration-> %s\" ", "Complete");
      cfr->ip_cfg_complete = YES;
    }

    menu_str += tmp_buff;

    /////////////////////////////////////////////////
    cfr->current_wanrouter_state = get_wanpipe_status("wanpipe1");

    /////////////////////////////////////////////////
    if(cfr->current_wanrouter_state == WANPIPE_RUNNING){

      number_of_items = 9;

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
      menu_str += tmp_buff;

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TRACE_CMDS);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Line Trace Commands\" ");
      menu_str += tmp_buff;

      /////////////////////////////////////////////////
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", STATUS_STATS_CMDS);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Status/Statistics Commands\" ");
      menu_str += tmp_buff;
    }else{
      //if wanpipe is not running, the Trace and Status commands should not be an option.
      //number_of_items -= 2;
      number_of_items = 6;
    }

    /////////////////////////////////////////////////
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
    menu_str += tmp_buff;

    /////////////////////////////////////////////////
    if(cfr->current_wanrouter_state ==  WANPIPE_NOT_RUNNING){
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", START_WANPIPE);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Start WANPIPE\" ");
      menu_str += tmp_buff;
    }else{
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", STOP_WANPIPE);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Stop WANPIPE\" ");
      menu_str += tmp_buff;
    }

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANPIPE_STATUS_LINE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"WANPIPE Status---> %s\" ",
      (cfr->current_wanrouter_state == WANPIPE_RUNNING ? "Running" : "Stopped"));
    menu_str += tmp_buff;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------");
//\nSangoma Card Properties");

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "MAIN MENU",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          number_of_items,//number of items in the menu, including the empty lines
                          (char*)menu_str.c_str()
                          ) == NO){
    rc = NO;
    goto cleanup;
  }

  if(show(selection_index) == NO){
    rc = NO;
    goto cleanup;
  }
  //////////////////////////////////////////////////////////////////////////////////////

  exit_dialog = NO;
  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MAIN_MENU_OPTIONS,
      ("menu_net_interface_ip_configuration: option selected for editing: %s\n",
      get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case EMPTY_LINE:
    case WANPIPE_STATUS_LINE:
      ;//do nothing
      break;

    case PROTOCOL_SELECT:
      {
        menu_protocol_mode_select protocol_mode_select(lxdialog_path, cfr);
        if(protocol_mode_select.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case STATION_TYPE:
      {
        menu_select_station_type select_station_type(lxdialog_path, cfr);
        if(select_station_type.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case IP_CONFIG:
      {
        menu_net_interface_ip_configuration
          net_interface_ip_configuration(lxdialog_path, cfr, &chan_def);

        if(net_interface_ip_configuration.run(selection_index, interface_file_reader) == NO){
          rc = NO;
          exit_dialog = YES;
        }else{
          if(interface_file_reader.create_net_interface_file(
                              &interface_file_reader.if_config) == NO){
            Debug(DBG_MAIN_MENU_OPTIONS, ("create_net_interface_file() FAILED!!\n"));
            return NO;
          }
        }
      }
      break;

    //case OPERATIONAL_CMDS:
    case START_WANPIPE:

      if(cfr->ip_cfg_complete == NO){
        //if running don't let to change the protocol
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
        "IP Configuration is incomplete.\nWANPIPE can NOT be started.");
        return YES;
      }

      start_wanpipe();
      cfr->current_wanrouter_state = get_wanpipe_status("wanpipe1");

      //instead of showing this dialog add status line on the Main page
      if(cfr->current_wanrouter_state ==  WANPIPE_RUNNING){
//        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
//          "WAN Router was started.");
      }else{
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          "WAN Router failed to start. Please check Message Log.");
      }
      break;

    case STOP_WANPIPE:
      stop_wanpipe();
      cfr->current_wanrouter_state = get_wanpipe_status("wanpipe1");

      if(cfr->current_wanrouter_state ==  WANPIPE_NOT_RUNNING){
//        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
//          "WAN Router was stopped.");
      }else{
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          "WAN Router failed to stop. Please check Message Log.");
      }
      break;

    case TRACE_CMDS:
      {
        menu_select_trace_cmd select_trace_cmd(lxdialog_path, cfr);
        if(select_trace_cmd.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case STATUS_STATS_CMDS:
      {
        menu_select_status_statistics_cmd select_status_statistics_cmd(lxdialog_path, cfr);
        if(select_status_statistics_cmd.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    Debug(DBG_MAIN_MENU_OPTIONS,
      ("MENU_BOX_BUTTON_HELP: menu_net_interface_ip_configuration: option selected for editing: %s\n",
      get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case EMPTY_LINE:
    case PROTOCOL_SELECT:
    case STATION_TYPE:
    case IP_CONFIG:
    case START_WANPIPE:
    case STOP_WANPIPE:
    case TRACE_CMDS:
    case STATUS_STATS_CMDS:
    case WANPIPE_STATUS_LINE:
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
    }
    break;

  case MENU_BOX_BUTTON_EXIT:
    exit_dialog = YES;
    break;
  }//switch(*selection_index)

  if(exit_dialog == NO){
    goto again;
  }

cleanup:
  return rc;
}

/////////////////////////////////////////////////////////////////
#define ATM_CPE_SERIAL_FILE   "atm_cpe_serial.template"
#define ATM_NODE_SERIAL_FILE  "atm_node_serial.template"
#define ATM_CPE_T1_FILE       "atm_cpe_t1.template"
#define ATM_NODE_T1_FILE      "atm_node_t1.template"

#define CHDLC_CPE_SERIAL_FILE   "chdlc_cpe_serial.template"
#define CHDLC_NODE_SERIAL_FILE  "chdlc_node_serial.template"
#define CHDLC_CPE_T1_FILE       "chdlc_cpe_t1.template"
#define CHDLC_NODE_T1_FILE      "chdlc_node_t1.template"

#define FR_CPE_SERIAL_FILE    "frame_relay_cpe_serial.template"
#define FR_CPE_T1_FILE        "frame_relay_cpe_t1.template"
#define FR_NODE_SERIAL_FILE   "frame_relay_node_serial.template"
#define FR_NODE_T1_FILE       "frame_relay_node_t1.template"

#define PPP_CPE_SERIAL_FILE   "ppp_cpe_serial.template"
#define PPP_NODE_SERIAL_FILE  "ppp_node_serial.template"
#define PPP_CPE_T1_FILE       "ppp_cpe_t1.template"
#define PPP_NODE_T1_FILE      "ppp_node_t1.template"

#define X25_CPE_SERIAL_FILE   "x25_cpe_serial.template"
#define X25_NODE_SERIAL_FILE  "x25_node_serial.template"
/////////////////////////////////////////////////////////////////

int menu_main_configuration_options::
  copy_new_configuration_to_default_location( unsigned char protocol,
                                              unsigned char station_type,
                                              unsigned char card_type)
{
  string file_name_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];

  rc = get_new_configuration_file_name(protocol, station_type, card_type, file_name_str);
  if(rc == NO){
    return rc;
  }

  Debug(DBG_MAIN_MENU_OPTIONS, ("file_name_str: %s\n", file_name_str.c_str()));

  //create the copy command string
  snprintf(tmp_buff, MAX_PATH_LENGTH, "cp %s%s %swanpipe1.conf",
    wanctl_template_dir,
    file_name_str.c_str(),
    wanpipe_cfg_dir);

  rc = run_system_command(tmp_buff);

  return rc;
}

int menu_main_configuration_options::
    get_new_configuration_file_name( unsigned char protocol,
                                     unsigned char station_type,
                                     unsigned char card_type,
                                     string& file_name_str)
{
  int rc = YES;

  switch(protocol)
  {
  case WANCONFIG_ATM:
    //Station is not used
    if(card_type == S5141_ADPTR_1_CPU_SERIAL){
      if(station_type == WANOPT_CPE){
        file_name_str = ATM_CPE_SERIAL_FILE;
      }else{
        file_name_str = ATM_NODE_SERIAL_FILE;
      }
    }else{
      if(station_type == WANOPT_CPE){
        file_name_str = ATM_CPE_T1_FILE;
      }else{
        file_name_str = ATM_NODE_T1_FILE;
      }
    }
    break;

  case WANCONFIG_CHDLC:
    //Station is not used
    if(card_type == S5141_ADPTR_1_CPU_SERIAL){
      if(station_type == WANOPT_CPE){
        file_name_str = CHDLC_CPE_SERIAL_FILE;
      }else{
        file_name_str = CHDLC_NODE_SERIAL_FILE;
      }
    }else{
      if(station_type == WANOPT_CPE){
        file_name_str = CHDLC_CPE_T1_FILE;
      }else{
        file_name_str = CHDLC_NODE_T1_FILE;
      }
    }
    break;

  case WANCONFIG_PPP:
    //Station is not used
    if(card_type == S5141_ADPTR_1_CPU_SERIAL){
      if(station_type == WANOPT_CPE){
        file_name_str = PPP_CPE_SERIAL_FILE;
      }else{
        file_name_str = PPP_NODE_SERIAL_FILE;
      }
    }else{
      if(station_type == WANOPT_CPE){
        file_name_str = PPP_CPE_T1_FILE;
      }else{
        file_name_str = PPP_NODE_T1_FILE;
      }
    }
    break;

  case WANCONFIG_FR:
    if(card_type == S5141_ADPTR_1_CPU_SERIAL){

      if(station_type == WANOPT_CPE){
        file_name_str = FR_CPE_SERIAL_FILE;
      }else{
        file_name_str = FR_NODE_SERIAL_FILE;
      }
    }else{
      if(station_type == WANOPT_CPE){
        file_name_str = FR_CPE_T1_FILE;
      }else{
        file_name_str = FR_NODE_T1_FILE;
      }
    }
    break;

  case WANCONFIG_X25:
    if(card_type == S5141_ADPTR_1_CPU_SERIAL){

      if(station_type == WANOPT_CPE){
        file_name_str = X25_CPE_SERIAL_FILE;
      }else{
        file_name_str = X25_NODE_SERIAL_FILE;
      }
    }else{
      ERR_DBG_OUT(("Card type 0x%X is invalid for protocol: %d\n",
        card_type, protocol));
      rc = NO;
    }
    break;

  default:
    ERR_DBG_OUT(("Invalid protocol: %d\n", protocol));
    rc = NO;
  }
  return rc;
}

