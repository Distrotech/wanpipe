/***************************************************************************
                          menu_hardware_select_comms_port.cpp  -  description
                             -------------------
    begin                : Wed Apr 28 2004
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

#include "menu_hardware_select_comms_port.h"
#include "text_box.h"

#define DBG_MENU_HARDWARE_SELECT_COMMS_PORT 1

menu_hardware_select_comms_port::menu_hardware_select_comms_port(IN char * lxdialog_path,
                                                                 IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_HARDWARE_SELECT_COMMS_PORT,
    ("menu_hardware_select_comms_port::menu_hardware_select_comms_port()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_hardware_select_comms_port::~menu_hardware_select_comms_port()
{
  Debug(DBG_MENU_HARDWARE_SELECT_COMMS_PORT,
    ("menu_hardware_select_comms_port::~menu_hardware_select_comms_port()\n"));
}

int menu_hardware_select_comms_port::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  int number_of_items;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;

  Debug(DBG_MENU_HARDWARE_SELECT_COMMS_PORT, ("menu_net_interface_setup::run()\n"));

again:
  number_of_items = 2;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_HARDWARE_SELECT_COMMS_PORT, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;
  menu_str = " ";

  Debug(DBG_MENU_HARDWARE_SELECT_COMMS_PORT, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_PRIMARY);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Primary\" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_SECONDARY);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Secondary\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect Communications Port for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT COMMUNICATIONS PORT",
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
    rc = NO;
    goto cleanup;
  }
  //////////////////////////////////////////////////////////////////////////////////////

  exit_dialog = NO;
  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_HARDWARE_SELECT_COMMS_PORT,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    Debug(DBG_MENU_HARDWARE_SELECT_COMMS_PORT,
      ("comms_port: atoi(get_lxdialog_output_string(): %d\n", atoi(get_lxdialog_output_string())));

    switch(atoi(get_lxdialog_output_string()))
    {
    case WANOPT_SECONDARY:
      //V2 - all protocols in the LIP layer. all runs on both ports
      /*
      if(is_protocol_valid_for_card_type(
                          link_def->linkconf->config_id,
                          linkconf->card_type,
                          link_def->card_version,
                          1) == NO){
        tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
"Currently selected protocol (%s)\n\
is invalid for Secondary Port.",
          get_protocol_string(link_def->linkconf->config_id));
        goto again;
      }

      linkconf->comm_port = 1;
      exit_dialog = YES;
      break;
      */
    case WANOPT_PRIMARY:
      linkconf->comm_port = 0;
      exit_dialog = YES;
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
    case WANOPT_SECONDARY:
    case WANOPT_PRIMARY:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, "Please select Communications port");
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
