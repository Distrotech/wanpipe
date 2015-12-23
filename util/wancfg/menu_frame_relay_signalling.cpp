/***************************************************************************
                          menu_frame_relay_signalling.cpp  -  description
                             -------------------
    begin                : Mon Mar 15 2004
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

#include "menu_frame_relay_signalling.h"
#include "wancfg.h"
#include "text_box.h"

#define MENU_FRAME_RELAY_SIGNALLING 0

menu_frame_relay_signalling::menu_frame_relay_signalling(IN char * lxdialog_path, IN wan_fr_conf_t* fr)
{
  Debug(MENU_FRAME_RELAY_SIGNALLING,
    ("menu_frame_relay_signalling::menu_frame_relay_signalling()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->fr = fr;
}

menu_frame_relay_signalling::~menu_frame_relay_signalling()
{
  Debug(MENU_FRAME_RELAY_SIGNALLING,
    ("menu_frame_relay_signalling::~menu_frame_relay_signalling()\n"));
}

int menu_frame_relay_signalling::run(OUT int * selection_index)
{
  string menu_str;
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog = NO;

  //help text box
  text_box tb;

  Debug(MENU_FRAME_RELAY_SIGNALLING, ("menu_frame_relay_signalling::run()\n"));

  /////////////////////////////////////////////////
  //insert "1" tag for "ANSI" into the menu string
  menu_str = " \"0\" ";
  //create the menu item
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Auto Detect\" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  //insert "1" tag for "ANSI" into the menu string
  menu_str += " \"1\" ";
  //create the menu item
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"ANSI\" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  //insert "2" tag for "Q933" into the menu string
  menu_str += " \"2\" ";
  //create the menu item
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Q933\" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  //insert "3" tag for "ANSI" into the menu string
  menu_str += " \"3\" ";
  //create the menu item
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"LMI\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\n\nSelect the Frame Relay Signalling mode.");

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_SELECT,
                          lxdialog_path,
                          "FRAME RELAY SIGNALLIG CONFIGURATION",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          4,//number of items in the menu, including the empty lines
                          (char*)menu_str.c_str()
                          ) == NO){
    rc = NO;
    goto cleanup;
  }

again:
  if(show(selection_index) == NO){
    rc = NO;
    goto cleanup;
  }
  //////////////////////////////////////////////////////////////////////////////////////

  exit_dialog = NO;
  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(MENU_FRAME_RELAY_SIGNALLING,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 0:
      fr->signalling = WANOPT_FR_AUTO_SIG;
      break;
      
    case 1:
       fr->signalling = WANOPT_FR_ANSI;
      break;

    case 2:
       fr->signalling = WANOPT_FR_Q933;
      break;

    case 3:
       fr->signalling = WANOPT_FR_LMI;
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
    }//switch(atoi(get_lxdialog_output_string()))
    exit_dialog = YES;
    break;

  case MENU_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
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
