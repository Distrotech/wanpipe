/***************************************************************************
                          menu_net_interface_miscellaneous_options.cpp  -  description
                             -------------------
    begin                : Thu Mar 25 2004
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

#include "menu_net_interface_miscellaneous_options.h"
#include "text_box.h"
#include "input_box.h"
#include "text_box_yes_no.h"
#include "text_box_help.h"
#include "conf_file_reader.h"

enum NET_IF_MISC_OPTIONS{
  EMPTY_LINE,
  IF_START_SCRIPT,
  IF_STOP_SCRIPT,
  IF_MULTICAST,
  IPX_SUPPORT,
  IPX_NET_ADDR,
  TRUE_ENCODING,
  HDLC_ENABLE_DISABLE
};

char* net_if_start_script_help_str =
"Script called by /usr/sbin/wanrouter\n"
"after the interface has been started using\n"
"ifconfig.\n";

char* net_if_stop_script_help_str =
"Script called by /usr/sbin/wanrouter\n"
"after the interface has been stopped using\n"
"ifconfig.\n";

char* net_if_multicast_help_str =
"MULTICAST\n"
"\n"
"If enabled, driver sets the IFF_MULTICAST\n"
"bit in network device structure.\n"
"\n"
"Available options are:\n"
"\n"
"YES : Set the IFF_MULTICAST bit in device structure\n"
"NO  : Clear the IFF_MULTICAST bit in device structure\n"
"\n"
"Default: NO\n";

char* net_if_ipx_addr_help_str =
"IPX Address should be of \"0x????????\" patern.\nEach '?' is a Hex digit.";

char* net_if_true_encoding_help_str =
"TRUE ENCODING TYPE\n"
"\n"
"This parameter is used to set the interface\n"
"encoding type to its true value.\n"
"\n"
"By default, the interface type is set to PPP.\n"
"This tricks the tcpdump, ethereal and others to think\n"
"that a WANPIPE interface is just a RAW-Pure IP interface.\n"
"(Which it is from the kernel point of view). By leaving\n"
"this option set to NO, all packet sniffers, like tcpdump,\n"
"will work with WANPIPE interfaces.\n"
"However, in some cases interface type can be\n"
"used to determine the protocol on the line. By setting\n"
"this option the interface type will be set to\n"
"its true value.  However, for Frame Relay and CHDLC,\n"
"the tcpdump support will be broken :(\n"
"\n"
"Options are:\n"
" NO: Set the interface type to RAW-Pure IP (PPP)\n"
" YES: Set the interface type to its true value.\n"
"\n"
" Default: NO";


char wanpipe_name[100];

#define DBG_MENU_NET_INTERFACE_MISCELLANEOUS_OPTIONS 1

menu_net_interface_miscellaneous_options::
	menu_net_interface_miscellaneous_options( IN char * lxdialog_path,
						  IN list_element_chan_def* list_element_chan)
{
  Debug(DBG_MENU_NET_INTERFACE_MISCELLANEOUS_OPTIONS,
    ("menu_net_interface_miscellaneous_options::menu_net_interface_miscellaneous_options()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  
  this->list_element_chan = list_element_chan;
}

menu_net_interface_miscellaneous_options::~menu_net_interface_miscellaneous_options()
{
  Debug(DBG_MENU_NET_INTERFACE_MISCELLANEOUS_OPTIONS,
    ("menu_net_interface_miscellaneous_options::~menu_net_interface_miscellaneous_options()\n"));
}

int menu_net_interface_miscellaneous_options::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char exit_dialog;
  int conversion_count;
  conf_file_reader* cfr = (conf_file_reader*)global_conf_file_reader_ptr;
  chan_def_t* chandef;
  wanif_conf_t* chanconf;
	  
  //help text box
  text_box tb;

  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s", get_protocol_string(NO_PROTOCOL_NEEDED));

  Debug(DBG_MENU_NET_INTERFACE_MISCELLANEOUS_OPTIONS, ("menu_net_interface_setup::run()\n"));

again:
  rc = YES;
  exit_dialog = NO;
  chandef = &list_element_chan->data;
  chanconf = chandef->chanconf;

  menu_str = "";

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", IF_MULTICAST);
  menu_str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Multicast----------> %s\" ",
    (chanconf->mc == 0 ? "NO" : "YES"));
  menu_str += tmp_buff;

  Debug(DBG_MENU_NET_INTERFACE_MISCELLANEOUS_OPTIONS,
    ("chanconf->config_id: %d\n", chanconf->config_id));
  
  /////////////////////////////////////////////////
  if( chanconf->config_id == WANCONFIG_MFR ||
      chanconf->config_id == WANCONFIG_MPPP){

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", IPX_SUPPORT);
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"IPX Support--------> %s\" ",
      (chanconf->enable_IPX == 0 ? "Disabled" : "Enabled"));
    menu_str += tmp_buff;

    if(chanconf->enable_IPX){

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", IPX_NET_ADDR);
      menu_str += tmp_buff;

      //IPX Network Addr
      if(chanconf->network_number == 0x00){
        chanconf->network_number = 0xABCDEFAB;
      }
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"IPX Network Addr---> 0x%X\" ",
        chanconf->network_number);
      menu_str += tmp_buff;
    }
  }

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TRUE_ENCODING);
  menu_str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"True Encoding Type-> %s\" ",
    (chanconf->true_if_encoding == 0 ? "Disabled" : "Enabled"));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", HDLC_ENABLE_DISABLE);
  menu_str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"HDLC engine--------> %s\" ",
    (chanconf->hdlc_streaming == WANOPT_YES ? "Enabled" : "Disabled"));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  //empty line
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  //if(linkconf->config_id != WANCONFIG_CHDLC){
  
  snprintf(wanpipe_name, 100, "wanpipe%d", cfr->wanpipe_number);
  
    chandef->chan_start_script =
      check_net_interface_start_script_exist( wanpipe_name, chandef->name);

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", IF_START_SCRIPT);
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Start Script-------> %s\" ",
      (chandef->chan_start_script == YES ? "Enabled" : "Disabled"));
    menu_str += tmp_buff;

    /////////////////////////////////////////////////
    chandef->chan_stop_script =
      check_net_interface_stop_script_exist(  wanpipe_name,
                                              chandef->name);

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", IF_STOP_SCRIPT);
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Stop Script--------> %s\" ",
      (chandef->chan_stop_script == YES ? "Enabled" : "Disabled"));
    menu_str += tmp_buff;
  //}

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nAdvanced options for interface: %s",
chandef->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "ADVANCED INTERFACE OPTIONS",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          8,//number of items in the menu, including the empty lines
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
    Debug(DBG_MENU_NET_INTERFACE_MISCELLANEOUS_OPTIONS,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case IF_START_SCRIPT:
      if(handle_start_script_selection(chandef) == NO){
        rc = NO;
        exit_dialog = YES;
      }
      break;

    case IF_STOP_SCRIPT:
      if(handle_stop_script_selection(chandef) == NO){
        rc = NO;
        exit_dialog = YES;
      }
      break;

    case IF_MULTICAST:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s Multicast option?",
        (chanconf->mc == 0 ? "enable" : "disable"));

      if(yes_no_question(   selection_index,
                            lxdialog_path,
                            NO_PROTOCOL_NEEDED,
                            tmp_buff) == NO){
        return NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(chanconf->mc == 0){
          //was disabled - enable
          chanconf->mc = 1;
        }else{
          //was enabled - disable
          chanconf->mc = 0;
        }
        break;
      }
      break;

    case IPX_SUPPORT://IPX support - y/n
      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s IPX support?",
        (chanconf->enable_IPX == 0 ? "enable" : "disable"));

      if(yes_no_question(   selection_index,
                            lxdialog_path,
                            NO_PROTOCOL_NEEDED,
                            tmp_buff) == NO){
        return NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(chanconf->enable_IPX == 0){
          //was disabled - enable
          chanconf->enable_IPX = 1;
        }else{
          //was enabled - disable
          chanconf->enable_IPX = 0;
        }
        break;
      }
      break;

    case IPX_NET_ADDR://IPX Network Address
      /////////////////////////////////////////////////////////////
show_ipx_addr_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify you IPX Network Number");
      snprintf(initial_text, MAX_PATH_LENGTH, "0x%X", chanconf->network_number);

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
        Debug(DBG_MENU_NET_INTERFACE_MISCELLANEOUS_OPTIONS,
          ("IPX addr on return: %s\n", inb.get_lxdialog_output_string()));


        conversion_count = sscanf(  inb.get_lxdialog_output_string(),
                                    "%x",
                                    &chanconf->network_number);

        Debug(DBG_MENU_NET_INTERFACE_MISCELLANEOUS_OPTIONS,
          ("conversion_count: %d\n", conversion_count));

        Debug(DBG_MENU_NET_INTERFACE_MISCELLANEOUS_OPTIONS,
          ("IPX addr in Hex: 0x%X\n", chanconf->network_number));

        if(conversion_count != 1 || (strlen(inb.get_lxdialog_output_string()) != 8+2)){

          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
            "Invalid IPX address.\n%s", net_if_ipx_addr_help_str);
          goto show_ipx_addr_input_box;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:

        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_ipx_addr_help_str);
        goto show_ipx_addr_input_box;
        
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case TRUE_ENCODING://yes/no

      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s True Encoding Type?",
        (chanconf->true_if_encoding == 0 ? "enable" : "disable"));

      if(yes_no_question(   selection_index,
                            lxdialog_path,
                            NO_PROTOCOL_NEEDED,
                            tmp_buff) == NO){
        return NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(chanconf->true_if_encoding == 0){
          //was disabled - enable
          chanconf->true_if_encoding = 1;
        }else{
          //was enabled - disable
          chanconf->true_if_encoding = 0;
        }
        break;
      }
      break;

    case HDLC_ENABLE_DISABLE:

      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s HDLC engine?",
        (chanconf->hdlc_streaming == WANOPT_YES ? "disable" : "enable"));

      if(yes_no_question(   selection_index,
                            lxdialog_path,
                            NO_PROTOCOL_NEEDED,
                            tmp_buff) == NO){
        return NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(chanconf->hdlc_streaming == WANOPT_NO){
          //was disabled - enable
          chanconf->hdlc_streaming = WANOPT_YES;
        }else{
          //was enabled - disable
          chanconf->hdlc_streaming = WANOPT_NO;
        }
        break;
      }

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
    case 1://start script
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_start_script_help_str);
      break;
    case 2://stop script
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_stop_script_help_str);
      break;
    case 3://multicast
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_multicast_help_str);
      break;
    case 4://ipx - y/n
    case 5://ipx address
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_ipx_addr_help_str);
      break;
    case 6://true encoding - y/n
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_true_encoding_help_str);
      break;
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

int menu_net_interface_miscellaneous_options::handle_start_script_selection(chan_def_t* chandef)
{
      int rc = YES;
      int selection_index;

      if(chandef->chan_start_script == YES){
        //script enabled
        snprintf(tmp_buff, MAX_PATH_LENGTH,
"The 'Start Script' is enabled.\n"
"If you want to disable it select 'Yes'.\n"
"If you want to keep it enabled and edit the\n"
"script file select 'No'.");

        if(yes_no_question(   &selection_index,
                              lxdialog_path,
                              WANCONFIG_FR,
                              tmp_buff) == NO){
          rc = NO;
        }

        switch(selection_index)
        {
        case YES_NO_TEXT_BOX_BUTTON_YES:
          remove_net_interface_start_script(  wanpipe_name,
                                              chandef->name);
          break;

        case YES_NO_TEXT_BOX_BUTTON_NO:
          if(edit_net_interface_start_script(   wanpipe_name,
                                                chandef->name) == NO){
            rc = NO;
          }
          break;
        }
      }else{
        //create new script
        if(create_new_net_interface_start_script( wanpipe_name,
                                                  chandef->name) == NO){
          rc = NO;
        }
        //display it to the user for editing
        if(edit_net_interface_start_script(   wanpipe_name,
                                              chandef->name) == NO){
          rc = NO;
        }
      }
  return rc;
}

int menu_net_interface_miscellaneous_options::handle_stop_script_selection(chan_def_t* chandef)
{
      int rc = YES;
      int selection_index;

      if(chandef->chan_stop_script == YES){
        //script enabled
        snprintf(tmp_buff, MAX_PATH_LENGTH,
"The 'Stop Script' is enabled.\n"
"If you want to disable it select 'Yes'.\n"
"If you want to keep it enabled and edit the\n"
"script file select 'No'.");

        if(yes_no_question(   &selection_index,
                              lxdialog_path,
                              WANCONFIG_FR,
                              tmp_buff) == NO){
          rc = NO;
        }

        switch(selection_index)
        {
        case YES_NO_TEXT_BOX_BUTTON_YES:
          remove_net_interface_stop_script(  wanpipe_name, chandef->name);
          break;

        case YES_NO_TEXT_BOX_BUTTON_NO:
          if(edit_net_interface_stop_script(   wanpipe_name, chandef->name) == NO){
            rc = NO;
          }
          break;
        }
      }else{
        //create new script
        if(create_new_net_interface_stop_script( wanpipe_name, chandef->name) == NO){
          rc = NO;
        }
        //display it to the user for editing
        if(edit_net_interface_stop_script(   wanpipe_name, chandef->name) == NO){
          rc = NO;
        }
      }
      return rc;
}

