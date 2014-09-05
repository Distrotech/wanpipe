/***************************************************************************
                          menu_frame_relay_manual_or_auto_dlci_cfg.cpp  -  description
                             -------------------
    begin                : Wed Mar 17 2004
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

#include "list_element_chan_def.h" //for 'AUTO_CHAN_CFG'
#include "menu_frame_relay_manual_or_auto_dlci_cfg.h"
#include "text_box_help.h"

extern char* fr_dlci_numbering_help_str;

#define DBG_MENU_FRAME_RELAY_MANUAL_OR_AUTO_DLCI_CFG  0

menu_frame_relay_manual_or_auto_dlci_cfg::
                      menu_frame_relay_manual_or_auto_dlci_cfg( IN char * lxdialog_path,
                                                                IN wanif_conf_t* wanif_conf)
{
  Debug(DBG_MENU_FRAME_RELAY_MANUAL_OR_AUTO_DLCI_CFG,
    ("menu_frame_relay_manual_or_auto_dlci_cfg::menu_frame_relay_manual_or_auto_dlci_cfg()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->wanif_conf = wanif_conf;
  fr = &wanif_conf->u.fr;
}

menu_frame_relay_manual_or_auto_dlci_cfg::~menu_frame_relay_manual_or_auto_dlci_cfg()
{
  Debug(DBG_MENU_FRAME_RELAY_MANUAL_OR_AUTO_DLCI_CFG,
    ("menu_frame_relay_manual_or_auto_dlci_cfg::~menu_frame_relay_manual_or_auto_dlci_cfg()\n"));
}

int menu_frame_relay_manual_or_auto_dlci_cfg::run(OUT int* selection_index,
						  IN  int* old_dlci_numbering_mode,
						  OUT int* new_dlci_numbering_mode)
{
  string menu_str;
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  

  Debug(DBG_MENU_FRAME_RELAY_MANUAL_OR_AUTO_DLCI_CFG,
    ("menu_frame_relay_manual_or_auto_dlci_cfg::run()\n"));

again:

  menu_str = " \"1\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"DLCI Numbering--> Auto (Single DLCI)\" ");
  menu_str += tmp_buff;

  menu_str += " \"2\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"DLCI Numbering--> Manual\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\
\nSelect DLCI Numbering Mode.");

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,
                          lxdialog_path,
                          "DLCI NUMBERING MODE",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          2,//number of visible items in the menu
                          (char*)menu_str.c_str()
                          ) == NO){
    rc = NO;
    goto cleanup;
  }

  if(show(selection_index) == NO){
    rc = NO;
    goto cleanup;
  }

  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_FRAME_RELAY_MANUAL_OR_AUTO_DLCI_CFG,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1://auto selected

      if(wanif_conf->u.fr.station == WANOPT_NODE){
        //the "auto" option is invalid for Access Node.
        text_box tb;
        tb.show_error_message(lxdialog_path, WANCONFIG_FR,
                            "Auto DLCI option is invalid for Access Node.");
        goto again;
      }

      fr->dlci_num = 1;
      *new_dlci_numbering_mode = AUTO_DLCI;
      break;

    case 2://manual selected
      //if old setting was 'auto', reset everything, otherwise do nothing.
      if(*old_dlci_numbering_mode == AUTO_DLCI){
        fr->dlci_num = 1;
      }
      *new_dlci_numbering_mode = MANUAL_DLCI;
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    {
      text_box tb;
      tb.show_error_message(lxdialog_path, WANCONFIG_FR, fr_dlci_numbering_help_str);
      goto again;
    }
    break;

  case MENU_BOX_BUTTON_EXIT:

    if( wanif_conf->u.fr.station == WANOPT_NODE && *new_dlci_numbering_mode == AUTO_DLCI){
	    
       //the "auto" option is invalid for Access Node.
       text_box tb;
       tb.show_error_message(lxdialog_path, WANCONFIG_FR,
                           "Auto DLCI option is invalid for Access Node.");
       goto again;
    }
    break;

  }//switch(*selection_index)

cleanup:
  return rc;
}

