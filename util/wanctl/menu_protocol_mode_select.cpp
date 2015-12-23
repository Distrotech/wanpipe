/***************************************************************************
                          menu_protocol_mode_select.cpp  -  description
                             -------------------
    begin                : Fri Mar 12 2004
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

#include "menu_protocol_mode_select.h"
#include "text_box.h"

menu_protocol_mode_select::menu_protocol_mode_select(IN char * lxdialog_path,
                                                     IN conf_file_reader* cfr)
{
  Debug(DBG_MENU_PROTOCOL_MODE_SELECT,
    ("menu_protocol_mode_select::menu_protocol_mode_select()\n"));
  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = cfr;
}

menu_protocol_mode_select::~menu_protocol_mode_select()
{
  Debug(DBG_MENU_PROTOCOL_MODE_SELECT,
    ("menu_protocol_mode_select::~menu_protocol_mode_select()\n"));
}

int menu_protocol_mode_select::run(OUT int * selected_protocol)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  //help text box
  text_box tb;

  Debug(DBG_MENU_PROTOCOL_MODE_SELECT, ("menu_new_device_configuration::run()\n"));

again:
  rc = YES;
  menu_str = "";

  if(cfr->current_wanrouter_state == WANPIPE_RUNNING){
    //if running don't let to change the protocol
    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
      "Protocol can not be changed while WANPIPE\nis running.");
    return YES;
  }

  snprintf(tmp_buff, MAX_PATH_LENGTH, "\"%d\" ", WANCONFIG_CHDLC);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, "\"%s\" ", get_protocol_string(WANCONFIG_CHDLC));
  menu_str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, "\"%d\" ", WANCONFIG_ATM);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, "\"%s\" ", get_protocol_string(WANCONFIG_ATM));
  menu_str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, "\"%d\" ", WANCONFIG_FR);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, "\"%s\" ", get_protocol_string(WANCONFIG_FR));
  menu_str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, "\"%d\" ", WANCONFIG_PPP);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, "\"%s\" ", get_protocol_string(WANCONFIG_PPP));
  menu_str += tmp_buff;
/*
  snprintf(tmp_buff, MAX_PATH_LENGTH, "\"%d\" ", WANCONFIG_X25);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, "\"%s\" ", get_protocol_string(WANCONFIG_X25));
  menu_str += tmp_buff;
*/

  snprintf(tmp_buff, MAX_PATH_LENGTH,
"Please select a WAN Protocol.");

  //snprintf(&tmp_buff[strlen(tmp_buff)], MAX_PATH_LENGTH, "%s", MENUINSTR_EXIT);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,
                          lxdialog_path,
                          "PROTOCOL SELECTION",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          5,//MAX_NUM_OF_VISIBLE_ITEMS_IN_MENU,
                          (char*)menu_str.c_str()
                          ) == NO){
    rc = NO;
    goto cleanup;
  }

  int selection_index;
  if(show(&selection_index) == NO){
    rc = NO;
    goto cleanup;
  }

  switch(selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_PROTOCOL_MODE_SELECT,
      ("option selected: %s\n", get_lxdialog_output_string()));

    cfr->protocol = *selected_protocol = atoi(get_lxdialog_output_string());

    if(cfr->form_interface_name(cfr->protocol) == NO){
      rc = NO;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
    goto again;
    break;

  case MENU_BOX_BUTTON_EXIT:
    //exit the dialog
    break;
  }

cleanup:
  return rc;
}

