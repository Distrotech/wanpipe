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

#include "menu_list_all_wanpipes.h"
#include "menu_list_existing_wanpipes.h"

char* main_cfg_options_help_str =
"CONFIGURATION OPTIONS\n"
"\n"
"Create a new Configuration File\n"
"-------------------------------\n"
"Creates a new wanpipe#.conf file from\n"
"scratch.\n"
"\n"
"Edit existing Configuration File\n"
"--------------------------------\n"
"Loads already existing configuration\n"
"file for  reconfiguration.\n";

#define DBG_MAIN_MENU_OPTIONS 1

menu_main_configuration_options::menu_main_configuration_options(IN char * lxdialog_path)
{
  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
}

menu_main_configuration_options::~menu_main_configuration_options()
{
}

/** No descriptions */
int menu_main_configuration_options::run(OUT int * selection_index)
{
  int rc = YES;
  char temp_buff[MAX_PATH_LENGTH];
  menu_list_existing_wanpipes existing_wanpipes_list(lxdialog_path);
  menu_list_all_wanpipes all_wanpipes_list(lxdialog_path);
  //help text box
  text_box tb;

again:
  //create the explanation text for the menu
  snprintf(temp_buff, MAX_PATH_LENGTH,
          "Please choose to Create or Edit Wanpipe configuration file.\n");
  snprintf(&temp_buff[strlen(temp_buff)], MAX_PATH_LENGTH,
          MENUINSTR_EXIT);

  if(set_configuration(   MENU_BOX_SELECT, lxdialog_path,
                          "CONFIGURATION OPTIONS",
                          WANCFG_PROGRAM_NAME,
                          temp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          2,
                          "2", "Edit existing Configuration File",
                          "1", "Create a new Configuration File",
                          END_OF_PARAMS_LIST) == NO){
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
    Debug(DBG_MAIN_MENU_OPTIONS,
      ("lxdialog output str: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1:
      //create new configuration
      if(all_wanpipes_list.run(selection_index) == YES){

        goto again;
      }else{
        //error. exit the program
        //ERR_DBG_OUT(("existing_wanpipes_list.run() - failed!!\n"));
        //goto cleanup;
        goto again;
      }
      break;

    case 2:
      //edit existing configuration
      if(existing_wanpipes_list.run(selection_index) == YES){

        goto again;
      }else{
        //error. exit the program
        //ERR_DBG_OUT(("existing_wanpipes_list.run() - failed!!\n"));
        //goto cleanup;
	goto again;
      }
      break;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, main_cfg_options_help_str);
    goto again;

  case MENU_BOX_BUTTON_EXIT:
    //exit_dialog = YES;
    break;

  default:
    //error
    rc = NO;
    break;
  }//switch(*selection_index)

cleanup:
  return rc;
}

