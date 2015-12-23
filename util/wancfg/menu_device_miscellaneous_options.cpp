/***************************************************************************
                          menu_device_miscellaneous_options.cpp  -  description
                             -------------------
    begin                : Fri Apr 2 2004
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

#include "menu_device_miscellaneous_options.h"
#include "text_box.h"
#include "text_box_yes_no.h"
#include "input_box.h"

#define DBG_MENU_DEVICE_MISCELLANEOUS_OPTIONS 1

char* mtu_help_str = {
"\n"
"MTU Definition:\n"
"\n"
"Maximum transmit unit size (in bytes).  This value\n"
"limits the maximum size of the data packet that can be\n"
"sent over the WAN link, including any encapsulation\n"
"header that router may add.\n"
"\n"
"The MTU value is relevant to the IP interface.\n"
"This value informs the kernel IP stack that\n"
"of the maximum IP packet size this interface\n"
"will allow.\n"
"\n"
"Default: 1500\n"
};

char* udp_port_help_str = {
"\n"
"UDPPORT\n"
"\n"
"The UDP port used by the WANPIPE UDP management/debug\n"
"utilites.\n"
"The UDP debugging could be seen as a security issue.\n"
"To disable UDP debugging set the value of the\n"
"UDP Port to 0.\n"
"\n"
"Options:\n"
"--------\n"
"WANPIPE UDP Ports: 9000 to 9999\n"
"To disable WANPIPE UDP debugging set this value to 0.\n"
"\n"
"Default 9000\n"
};

char* ttl_help_str = {
"TTL\n"
"\n"
"This keyword defines the Time To Live for a UDP\n"
"packet used by the WANPIPE debugging/monitoring system.\n"
"The user can control the scope of a UDP packet by\n"
"this number, which decrements with each hop.\n"
};

char* front_end_help_str = {
"DISABLE/ENABLE FRONT END STATUS\n"
"\n"
"This option can ether Enable\n"
"or Disable FRONT END (CSU/DSU) status.\n"
"\n"
"Enable:  The front end status will affect\n"
"the link state.  Thus if CSU/DSU looses\n"
"sync, the link state will become\n"
"disconnected.\n"
"\n"
"Disable: No matter what state the front end (CSU/DSU)\n"
"is in, the link state will only depend\n"
"on the state of the protocol.\n"
"Options to ignore front end:\n"
"----------------------------\n"
"\n"
"NO: This option enables the front end status\n"
"and the link will be affected by it.\n"
"\n"
"YES: This option disables the front end\n"
"status fron affecting the link state.\n"
"\n"
"Default: NO\n"
};

char* wanpipe_start_script_help_str =
"Script called by /usr/sbin/wanrouter\n"
"after the WANPIPE has been started.\n";

char* wanpipe_stop_script_help_str =
"Script called by /usr/sbin/wanrouter\n"
"after the WANPIPE has been stopped.\n";


char* front_end_status_monitoring_help_str =
"DISABLE/ENABLE FRONT END STATUS MONITORING\n"
"\n"
"Enable:  The front end status will affect\n"
"the link state.  Thus if CSU/DSU looses\n"
"sync, the link state will become \"disconnected\".\n"
"\n"
"Disable: No matter what state the front end (CSU/DSU)\n"
"is in, the link state will only depend\n"
"on the state of the protocol.\n";

char* protocol_firmware_file_help_str =
"PROTOCOL FIRMWARE\n"
"\n"
"Full, absolute path to wanpipe\n"
"firmware files (\".sfm\" files).\n";

enum DEV_MISC_OPTIONS{
  EMPTY_LINE=1,
  DEV_START_SCRIPT,
  DEV_STOP_SCRIPT,
  MTU,
  UDP_PORT,
  TTL,
  IGNORE_FRONT_END,
  FIMWARE_FILE,
  //DEBUG_LEVEL//not implemented yet
};

menu_device_miscellaneous_options::menu_device_miscellaneous_options( IN char * lxdialog_path,
                                                                      IN conf_file_reader* cfr)
{
  Debug(DBG_MENU_DEVICE_MISCELLANEOUS_OPTIONS,
    ("menu_device_miscellaneous_options::menu_device_miscellaneous_options()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = cfr;
}

menu_device_miscellaneous_options::~menu_device_miscellaneous_options()
{
  Debug(DBG_MENU_DEVICE_MISCELLANEOUS_OPTIONS,
    ("menu_device_miscellaneous_options::~menu_device_miscellaneous_options()\n"));
}

int menu_device_miscellaneous_options::run(OUT int * selection_index)
{
  string menu_str;
  string err_str;
  int rc, number_of_items;
  int exit_dialog;
  char tmp_buff[MAX_PATH_LENGTH];

  //help text box
  text_box tb;

  //input box for : 1. MTU 2. UDP Port 3. TTL 4. Firmware file
  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility");

  link_def_t * link_def = cfr->link_defs;
  wandev_conf_t *linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_DEVICE_MISCELLANEOUS_OPTIONS, ("menu_new_device_configuration::run()\n"));
  Debug(DBG_MENU_DEVICE_MISCELLANEOUS_OPTIONS, ("cfr->link_defs->name: %s\n", link_def->name));

again:
  rc = YES;
  exit_dialog = NO;
  number_of_items = 9;
  menu_str = "";

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", MTU);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"MTU ---------------> %d\" ",
    linkconf->mtu);
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", UDP_PORT);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"UDP Debugging Port-> %d\" ",
    linkconf->udp_port);
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TTL);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"UDP Debugging TTL--> %d\" ",
    linkconf->ttl);
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", IGNORE_FRONT_END);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Ignore Front End---> %s\" ",
    (linkconf->ignore_front_end_status == 0 ? "NO" : "YES"));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  //firmware not used by the AFT and ADSL cards
  switch(linkconf->card_type)
  {
  case WANOPT_S50X:
  case WANOPT_S51X:
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", FIMWARE_FILE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Firmware File:\" ");
    menu_str += tmp_buff;
    //file path is usually long, go to the next line
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", FIMWARE_FILE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \" %s\" ",  link_def->firmware_file_path);
    menu_str += tmp_buff;
    break;
  }
  
  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
  menu_str += tmp_buff;

  ////////////////////////////////////////////////
  link_def->start_script = check_wanpipe_start_script_exist(link_def->name);

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", DEV_START_SCRIPT);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Start Script-------> %s\" ",
    (link_def->start_script == YES ? "Enabled" : "Disabled"));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  link_def->stop_script = check_wanpipe_stop_script_exist(link_def->name);

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", DEV_STOP_SCRIPT);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Stop Script--------> %s\" ",
    (link_def->stop_script == YES ? "Enabled" : "Disabled"));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////

  Debug(DBG_MENU_DEVICE_MISCELLANEOUS_OPTIONS, ("\nmenu_str:%s\n", menu_str.c_str()));

  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
    "Advanced options for Wan Device: %s\n\n", cfr->link_defs->name);

  //snprintf(&tmp_buff[strlen(tmp_buff)], MAX_PATH_LENGTH - strlen(tmp_buff), "%s", MENUINSTR_EXIT);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "ADVANCED DEVICE OPTIONS",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          number_of_items,
                          (char*)menu_str.c_str()
                          ) == NO){
    rc = NO;
    goto cleanup;
  }

  if(show(selection_index) == NO){
    Debug(DBG_MENU_DEVICE_MISCELLANEOUS_OPTIONS,
      ("menu_new_device_configuration.show() - failed!!\n"));
    rc = NO;
    goto cleanup;
  }

  exit_dialog = NO;
  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_DEVICE_MISCELLANEOUS_OPTIONS,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case DEV_START_SCRIPT:
      if(handle_start_script_selection() == NO){
        rc = NO;
        exit_dialog = YES;
      }
      break;

    case DEV_STOP_SCRIPT:
      if(handle_stop_script_selection() == NO){
        rc = NO;
        exit_dialog = YES;
      }
      break;

    case MTU:
show_mtu_input_box:

      snprintf(explanation_text, MAX_PATH_LENGTH,
        "Please specify IP Maximum Packet Size (MTU)");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", linkconf->mtu);

      inb.set_configuration(  lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

      inb.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_DEVICE_MISCELLANEOUS_OPTIONS,
          ("mtu on return: %s\n", inb.get_lxdialog_output_string()));

        if(atoi(inb.get_lxdialog_output_string()) == 0){
          //ivalid input
          err_str = "Invalid MTU value!!\n";
          err_str += mtu_help_str;

          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
            err_str.c_str());
          goto show_mtu_input_box;
        }else{
          linkconf->mtu = atoi(inb.get_lxdialog_output_string());
        }
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, mtu_help_str);
        goto show_mtu_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case UDP_PORT:
show_udp_port_input_box:

      snprintf(explanation_text, MAX_PATH_LENGTH,
        "UDP Port (default 9000, 0 to  disable)");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", linkconf->udp_port);

      inb.set_configuration(  lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

      inb.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_DEVICE_MISCELLANEOUS_OPTIONS,
          ("udp_port on return: %s\n", inb.get_lxdialog_output_string()));

        linkconf->udp_port = atoi(inb.get_lxdialog_output_string());
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          udp_port_help_str);
        goto show_udp_port_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case TTL:

show_ttl_input_box:

      snprintf(explanation_text, MAX_PATH_LENGTH,
        "UDP debugging TTL value (default: 255, to disable: 0)");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", linkconf->ttl);

      inb.set_configuration(  lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

      inb.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_DEVICE_MISCELLANEOUS_OPTIONS,
          ("mtu on return: %s\n", inb.get_lxdialog_output_string()));

        linkconf->ttl = atoi(inb.get_lxdialog_output_string());
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, ttl_help_str);
        goto show_ttl_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case IGNORE_FRONT_END:

      snprintf(tmp_buff, MAX_PATH_LENGTH,
"Do you want to ignore the front end status\n"
"when determining the link state?\n");

      if(yes_no_question(   selection_index,
                            lxdialog_path,
                            NO_PROTOCOL_NEEDED,
                            tmp_buff) == NO){
        rc = NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        linkconf->ignore_front_end_status = 1;
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
        linkconf->ignore_front_end_status = 0;
        break;
      }
      break;

    case FIMWARE_FILE:

show_firmware_path_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH,
        "Specify the location of the Firmware (.sfm) module: Absolute Path");
      snprintf(initial_text, MAX_PATH_LENGTH, "%s", link_def->firmware_file_path);

      inb.set_configuration(  lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

      inb.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_DEVICE_MISCELLANEOUS_OPTIONS,
          ("firmware path on return: %s\n", inb.get_lxdialog_output_string()));

        snprintf( link_def->firmware_file_path, MAX_PATH_LENGTH,
                  inb.get_lxdialog_output_string());
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message( lxdialog_path, NO_PROTOCOL_NEEDED,
                              option_not_implemented_yet_help_str);
        goto show_firmware_path_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case EMPTY_LINE:
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    switch(atoi(get_lxdialog_output_string()))
    {
    case EMPTY_LINE:
      //do nothing
      break;

    case DEV_START_SCRIPT:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, wanpipe_start_script_help_str);
      break;

    case DEV_STOP_SCRIPT:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, wanpipe_stop_script_help_str);
      break;

    case MTU:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, mtu_help_str);
      break;

    case UDP_PORT:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, udp_port_help_str);
      break;

    case TTL:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, ttl_help_str);
      break;

    case IGNORE_FRONT_END:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, front_end_status_monitoring_help_str);
      break;

    case FIMWARE_FILE:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, protocol_firmware_file_help_str);
      break;

    default:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
      //front_end_help_str
      break;
    }
    break;

  case MENU_BOX_BUTTON_EXIT:
    //run checks on the configuration here. notify user about errors.

    //if no errors found ask confirmation to save to file.

    rc = YES;
    exit_dialog = YES;
    break;
  }//switch(*selection_index)

  if(exit_dialog == NO){
    goto again;
  }

cleanup:
  return rc;
  ////////////////////////////////////////////////////
}

int menu_device_miscellaneous_options::handle_start_script_selection()
{
  int rc = YES;
  int selection_index;
  char tmp_buff[MAX_PATH_LENGTH];
  link_def_t * link_def = cfr->link_defs;
  //wandev_conf_t *linkconf = cfr->link_defs->linkconf;

  if(link_def->start_script == YES){
        //script enabled
        snprintf(tmp_buff, MAX_PATH_LENGTH,
"The 'Start Script' is enabled.\n"
"If you want to disable it select 'Yes'.\n"
"If you want to keep it enabled and edit the\n"
"script file select 'No'.");

        if(yes_no_question(   &selection_index,
                              lxdialog_path,
                              NO_PROTOCOL_NEEDED,
                              tmp_buff) == NO){
          rc = NO;
        }

        switch(selection_index)
        {
        case YES_NO_TEXT_BOX_BUTTON_YES:
          remove_wanpipe_start_script(cfr->link_defs->name);
          break;

        case YES_NO_TEXT_BOX_BUTTON_NO:
          if(edit_wanpipe_start_script(cfr->link_defs->name) == NO){
            rc = NO;
          }
          break;
        }
  }else{
        //create new script
        if(create_wanpipe_start_script( cfr->link_defs->name) == NO){
          rc = NO;
        }
        //display it to the user for editing
        if(edit_wanpipe_start_script( cfr->link_defs->name) == NO){
          rc = NO;
        }
  }
  return rc;
}

int menu_device_miscellaneous_options::handle_stop_script_selection()
{
  int rc = YES;
  int selection_index;
  char tmp_buff[MAX_PATH_LENGTH];
  link_def_t * link_def = cfr->link_defs;
  //wandev_conf_t *linkconf = cfr->link_defs->linkconf;

  if(link_def->stop_script == YES){
        //script enabled
        snprintf(tmp_buff, MAX_PATH_LENGTH,
"The 'Stop Script' is enabled.\n"
"If you want to disable it select 'Yes'.\n"
"If you want to keep it enabled and edit the\n"
"script file select 'No'.");

        if(yes_no_question(   &selection_index,
                              lxdialog_path,
                              NO_PROTOCOL_NEEDED,
                              tmp_buff) == NO){
          rc = NO;
        }

        switch(selection_index)
        {
        case YES_NO_TEXT_BOX_BUTTON_YES:
          remove_wanpipe_stop_script(cfr->link_defs->name);
          break;

        case YES_NO_TEXT_BOX_BUTTON_NO:
          if(edit_wanpipe_stop_script(cfr->link_defs->name) == NO){
            rc = NO;
          }
          break;
        }
  }else{
        //create new script
        if(create_wanpipe_stop_script( cfr->link_defs->name) == NO){
          rc = NO;
        }
        //display it to the user for editing
        if(edit_wanpipe_stop_script( cfr->link_defs->name) == NO){
          rc = NO;
        }
  }
  return rc;
}
