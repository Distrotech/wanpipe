/***************************************************************************
                          menu_list_existing_wanpipes.cpp  -  description
                             -------------------
    begin                : Wed Mar 3 2004
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

#include "menu_list_existing_wanpipes.h"
#include "text_box.h"
#include "conf_file_reader.h"
#include "menu_new_device_configuration.h"

char* list_of_existing_conf_files_help_str =
"Edit existing Configuration File\n"
"--------------------------------\n"
"Select already existing configuration\n"
"file for  reconfiguration.\n";


menu_list_existing_wanpipes::menu_list_existing_wanpipes(IN char * lxdialog_path)
{
  Debug(DBG_MENU_LIST_EXISTING_WANPIPES,
    ("menu_list_existing_wanpipes::menu_list_existing_wanpipes()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
}

menu_list_existing_wanpipes::~menu_list_existing_wanpipes()
{
  Debug(DBG_MENU_LIST_EXISTING_WANPIPES,
    ("menu_list_existing_wanpipes::~menu_list_existing_wanpipes()\n"));
}

int menu_list_existing_wanpipes::run(OUT int * selection_index)
{
  int existing_wanpipes_counter;
  FILE * file;
  string menu_str;
  int rc;
  objects_list* obj_list;

  //help text box
  text_box tb;

  conf_file_reader* cfr = NULL;
  menu_new_device_configuration * new_dev = NULL;

again:

  rc = YES;
  existing_wanpipes_counter = 0;
  menu_str = " ";

  if(cfr != NULL){
    delete cfr;
    cfr = NULL;
  }
  if(new_dev != NULL){
    delete new_dev;
    new_dev = NULL;
  }

  for(int i = 1; i <= MAX_NUMBER_OF_WANPIPES; i++){

    snprintf(conf_file_path, MAX_PATH_LENGTH, "%swanpipe%d.conf",
      wanpipe_cfg_dir, i);

    if(check_file_exist(conf_file_path, &file) == YES){
      existing_wanpipes_counter++;

      //create the tag
      snprintf(conf_file_path, MAX_PATH_LENGTH, "%d ", i);
      //insert tag into the menu string
      menu_str += conf_file_path;
      //create the menu item
      snprintf(conf_file_path, MAX_PATH_LENGTH, "wanpipe%d.conf ", i);
      //insert item into the menu string
      menu_str += conf_file_path;
    }
  }

  Debug(DBG_MENU_LIST_EXISTING_WANPIPES, ("\nmenu_str:%s\n", menu_str.c_str()));

  if(existing_wanpipes_counter == 0){
    //it is not an error if no config files exist, so return YES.
    //indicate to the caller that there is nothing to edit by setting
    //'selection_index' to 'MAX_NUMBER_OF_WANPIPES+1'.

    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
      "No existing Wanpipe configuration files found.");

    *selection_index = MAX_NUMBER_OF_WANPIPES+1;
    rc = YES;
    goto cleanup;
  }
  ////////////////////////////////////////////////////

  //create the explanation text for the menu
  snprintf(conf_file_path, MAX_PATH_LENGTH,
          "Please select Wanpipe configuration file you wish to edit.\n\n");
  snprintf(&conf_file_path[strlen(conf_file_path)], MAX_PATH_LENGTH, "%s%s",
    "------------------------------------------\n",
    "Use arrows to navigate through the options.");
    //MENUINSTR_EXIT);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_SELECT,
                          lxdialog_path,
                          "EXISTING WANPIPE CONFIGURATION FILES",
                          WANCFG_PROGRAM_NAME,
                          conf_file_path,
                          MENU_HEIGTH, MENU_WIDTH,
                          existing_wanpipes_counter,
                          (char*)menu_str.c_str()
                          ) == NO){
    rc = NO;
    goto cleanup;
  }

  if(show(selection_index) == NO){
    rc = NO;
    goto cleanup;
  }
#if 1
  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:

    Debug(DBG_MENU_LIST_EXISTING_WANPIPES,
      ("file selected for editing: wanpipe%s\n", get_lxdialog_output_string()));
    //set the value for the caller
    *selection_index = atoi(get_lxdialog_output_string());
    rc = YES;

    //////////////////////////////////////////////////////
    //allocate object which will read the configuration file.
    global_conf_file_reader_ptr = cfr = new conf_file_reader(*selection_index);
    if(cfr->init() != 0){
      rc = NO;
      goto cleanup;
    }

    //read the conf file.
    if(cfr->read_conf_file() != 0){
      tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
        "Failed to parse 'wanpipe%s.conf' file!!",
        replace_new_line_with_zero_term(get_lxdialog_output_string()));
      rc = NO;
      goto cleanup;
    }

    Debug(DBG_AFT_READ, ("cfr->link_defs->config_id: %d\n",  cfr->link_defs->linkconf->config_id));
    obj_list = cfr->main_obj_list;
    
    /////////////////////////////////////////////////////
    //
    new_dev = new menu_new_device_configuration(lxdialog_path, &cfr);
    if(new_dev->run(selection_index) == NO){
      Debug(DBG_MENU_LIST_EXISTING_WANPIPES,
        ("1. menu_list_existing_wanpipes.run() - failed!!\n"));
      rc = NO;
      goto cleanup;
    }
    //////////////////////////////////////////////////////
    goto again;
    break;

  case MENU_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, list_of_existing_conf_files_help_str);
    goto again;
    break;

  case MENU_BOX_BUTTON_EXIT:
    //exit the dialog
    break;
  }
#endif

cleanup:
  if(cfr != NULL){
    delete cfr;
    global_conf_file_reader_ptr = cfr = NULL;
  }
  if(new_dev != NULL){
    delete new_dev;
  }
  return rc;
  ////////////////////////////////////////////////////
}

