/***************************************************************************
                          menu_lapb_basic_cfg.h  -  description
                             -------------------
    begin                : Mon Apr 5 2004
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

#ifndef MENU_LAPB_BASIC_CFG_H
#define MENU_LAPB_BASIC_CFG_H

#include "menu_base.h"
#include "wancfg.h"
#include "objects_list.h"
#include "list_element_chan_def.h"
#include "text_box.h"

/**
  *@author David Rokhvarg
  */

class menu_lapb_basic_cfg : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  
  list_element_chan_def* parent_element_logical_ch;
  
  char* name_of_parent_layer;
  
public:

  menu_lapb_basic_cfg(	IN char* lxdialog_path,
			IN list_element_chan_def* parent_element_logical_ch);
  
  ~menu_lapb_basic_cfg();
  
  int run(OUT int * selection_index);
};

////////////////////////////////////////////////////////////////////////////
#define DBG_MENU_LAPB_STATION_TYPE 1

class menu_lapb_station_type : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  
public:

  menu_lapb_station_type(IN char* lxdialog_path)
  {
    snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  }
  
  ~menu_lapb_station_type()
  {
  }
 
  //returns the 'new_station_type' 
  int run(IN int old_station_type)
  {
    string menu_str;
    int rc = old_station_type, selection_index;
    char tmp_buff[MAX_PATH_LENGTH];
  
    Debug(DBG_MENU_LAPB_STATION_TYPE,
      ("menu_frame_relay_manual_or_auto_dlci_cfg::run()\n"));

again:
    menu_str = " \"1\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"DTE\" ");
    menu_str += tmp_buff;

    menu_str += " \"2\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"DCE\" ");
    menu_str += tmp_buff;

    //////////////////////////////////////////////////////////////////////////////////////
    //create the explanation text for the menu
    snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\
\nSelect Station Type.");

    if(set_configuration( YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,
                          lxdialog_path,
                          "STATION TYPE",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          2,//number of visible items in the menu
                          (char*)menu_str.c_str()
                          ) == NO){
      goto cleanup;
    }

    if(show(&selection_index) == NO){
      goto cleanup;
    }

    switch(selection_index)
    {
    case MENU_BOX_BUTTON_SELECT:
      Debug(DBG_MENU_LAPB_STATION_TYPE,
        ("option selected for editing: %s\n", get_lxdialog_output_string()));

      switch(atoi(get_lxdialog_output_string()))
      {
      case 1:
	rc = WANOPT_DTE;
        break;

      case 2:
	rc = WANOPT_DCE;
        break;

      default:
        ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
          get_lxdialog_output_string()));
      }//switch()
      break;

    case MENU_BOX_BUTTON_HELP:
      {
        text_box tb;
        tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
        goto again;
      }
      break;

    case MENU_BOX_BUTTON_EXIT:
      break;
     
    }//switch(selection_index)

cleanup:
    return rc;
  }
};

#endif
