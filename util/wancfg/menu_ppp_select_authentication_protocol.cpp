/***************************************************************************
                          menu_ppp_select_authentication_protocol.cpp  -  description
                             -------------------
    begin                : Fri Apr 30 2004
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

#include "menu_ppp_select_authentication_protocol.h"
#include "text_box.h"

enum PPP_AUTHENTICATION_MODE{
  PPP_CHAP,
  PPP_PAP,
  PPP_NO_AUTHENTICATION
};

#define DBG_MENU_PPP_SELECT_AUTHENTICATION_PROTOCOL 1

menu_ppp_select_authentication_protocol::
  menu_ppp_select_authentication_protocol(  IN char * lxdialog_path,
                                            IN list_element_chan_def* list_el_chan_def)
{
  Debug(DBG_MENU_PPP_SELECT_AUTHENTICATION_PROTOCOL,
    ("menu_ppp_select_authentication_protocol::menu_ppp_select_authentication_protocol()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->list_el_chan_def = list_el_chan_def;
}

menu_ppp_select_authentication_protocol::~menu_ppp_select_authentication_protocol()
{
  Debug(DBG_MENU_PPP_SELECT_AUTHENTICATION_PROTOCOL,
    ("menu_ppp_select_authentication_protocol::~menu_ppp_select_authentication_protocol()\n"));
}

int menu_ppp_select_authentication_protocol::run(OUT int * selection_index)
{
  string menu_str;
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  int number_of_items;

  //help text box
  text_box tb;

  Debug(DBG_MENU_PPP_SELECT_AUTHENTICATION_PROTOCOL,
    ("menu_ppp_select_authentication_protocol::run()\n"));

again:

  rc = YES;
  exit_dialog = NO;
  menu_str = " ";

  number_of_items = 3;

  menu_str.sprintf(" \"%d\" ", PPP_NO_AUTHENTICATION);
  menu_str += " \"Authentication disabled (default)\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", PPP_PAP);
  menu_str += tmp_buff;
  menu_str += " \"PAP\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", PPP_CHAP);
  menu_str += tmp_buff;
  menu_str += " \"CHAP\" ";

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect PPP Authentication Protocol");

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT PPP AUTHENTICATION PROTOCOL",
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
    Debug(DBG_MENU_PPP_SELECT_AUTHENTICATION_PROTOCOL, ("ppp setup: option selected for editing: %s\n",
      get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case PPP_CHAP:
      list_el_chan_def->data.chanconf->u.ppp.pap = 0;
      list_el_chan_def->data.chanconf->u.ppp.chap = 1;
      break;

    case PPP_PAP:
      list_el_chan_def->data.chanconf->u.ppp.pap = 1;
      list_el_chan_def->data.chanconf->u.ppp.chap = 0;
      break;

    case PPP_NO_AUTHENTICATION:
      list_el_chan_def->data.chanconf->u.ppp.pap = 0;
      list_el_chan_def->data.chanconf->u.ppp.chap = 0;
      break;
    }
    exit_dialog = YES;
    break;

  case MENU_BOX_BUTTON_HELP:

    switch(atoi(get_lxdialog_output_string()))
    {
    default:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
        option_not_implemented_yet_help_str);
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
