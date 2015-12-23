/***************************************************************************
                          menu_ppp_ip_mode.cpp  -  description
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

#include "menu_ppp_ip_mode.h"
#include "text_box.h"

#define DBG_MENU_PPP_IP_MODE 1


menu_ppp_ip_mode::menu_ppp_ip_mode( IN char * lxdialog_path)
{
  Debug(DBG_MENU_PPP_IP_MODE, ("menu_ppp_ip_mode::menu_ppp_ip_mode()\n"));
  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
}

menu_ppp_ip_mode::~menu_ppp_ip_mode()
{
  Debug(DBG_MENU_PPP_IP_MODE,
    ("menu_ppp_ip_mode::~menu_ppp_ip_mode()\n"));
}

int menu_ppp_ip_mode::run(OUT unsigned char* new_ip_mode)
{
  string menu_str;
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  int number_of_items;
  int abc;
  int* selection_index;
  //help text box
  text_box tb;

  Debug(DBG_MENU_PPP_IP_MODE, ("menu_ppp_ip_mode::run()\n"));

again:
  exit_dialog = NO;
  menu_str = " ";
  selection_index = &abc;
  
  number_of_items = 3;

  menu_str.sprintf(" \"%d\" ", WANOPT_PPP_STATIC);
  menu_str += " \"Static\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_PPP_HOST);
  menu_str += tmp_buff;
  menu_str += " \"Host\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_PPP_PEER);
  menu_str += tmp_buff;
  menu_str += " \"Peer\" ";

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect PPP IP Mode.");

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT PPP IP MODE",
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
    Debug(DBG_MENU_PPP_IP_MODE, ("ppp setup: option selected for editing: %s\n",
      get_lxdialog_output_string()));

    *new_ip_mode = atoi(get_lxdialog_output_string());
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

