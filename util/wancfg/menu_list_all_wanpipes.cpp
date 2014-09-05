/***************************************************************************
                          menu_list_all_wanpipes.cpp  -  description
                             -------------------
    begin                : Tue Apr 13 2004
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

#include "menu_list_all_wanpipes.h"
#include "text_box.h"
#include "text_box_yes_no.h"
#include "conf_file_reader.h"
#include "menu_new_device_configuration.h"

char* create_new_configuration_file_help_str =
"Create a new Configuration File\n"
"-------------------------------\n"
"Creates a new wanpipe#.conf file from\n"
"scratch.\n"
"OR\n"
"Erase contents of existing Configuration File\n"
"and create new configuration from scratch.\n";


void* global_conf_file_reader_ptr;

#define DBG_MENU_LIST_ALL_WANPIPES  1

char* conf_file_exist_str = " - exist ";
char* conf_file_not_exist_str = " - does not exist ";

char* warning_confirm_overwriting_of_existing_conf_file_str =
"Warning\n"
"-------\n"
"Are you sure you want overwrite existing\n"
"configuration file?\n";

menu_list_all_wanpipes::menu_list_all_wanpipes(IN char * lxdialog_path)
{
  Debug(DBG_MENU_LIST_ALL_WANPIPES,
    ("menu_list_all_wanpipes::menu_list_all_wanpipes()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
}

menu_list_all_wanpipes::~menu_list_all_wanpipes()
{
  Debug(DBG_MENU_LIST_ALL_WANPIPES,
    ("menu_list_all_wanpipes::~menu_list_all_wanpipes()\n"));
}

int menu_list_all_wanpipes::run(OUT int * selection_index)
{
  int existing_wanpipes_counter;
  FILE * file;
  string menu_str;
  int rc;
  int num_of_visible_items;
  int exit_dialog;
  char file_exist;
  int wanpipe_number;

  //help text box
  text_box tb;

again:

  rc = YES;
  exit_dialog = NO;
  existing_wanpipes_counter = 0;
  menu_str = " ";

  for(int i = 1; i <= MAX_NUMBER_OF_WANPIPES; i++){

    snprintf(conf_file_path, MAX_PATH_LENGTH, "%swanpipe%d.conf",
      wanpipe_cfg_dir, i);

    file_exist = check_file_exist(conf_file_path, &file);

    //create the tag
    snprintf(conf_file_path, MAX_PATH_LENGTH, " \"%d\" ", i);
    //insert tag into the menu string
    menu_str += conf_file_path;

    existing_wanpipes_counter++;

    if(file_exist == YES){
      //create the menu item
      snprintf(conf_file_path, MAX_PATH_LENGTH, " \"wanpipe%d.conf\t%s\" ", i,
        conf_file_exist_str);
    }else{
      //create the menu item
      snprintf(conf_file_path, MAX_PATH_LENGTH, " \"wanpipe%d.conf\t%s\" ", i,
        conf_file_not_exist_str);
    }

    //insert item into the menu string
    menu_str += conf_file_path;
  }

  Debug(DBG_MENU_LIST_ALL_WANPIPES, ("\nmenu_str:%s\n", menu_str.c_str()));

  //adjust number of items in the menu depending on number of dlcis
  num_of_visible_items = existing_wanpipes_counter;
  //8 items looks best
  if(num_of_visible_items > 8){
    num_of_visible_items = 8;
  }

  ////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(conf_file_path, MAX_PATH_LENGTH,
          "Please select Wanpipe configuration file you wish to create or edit.\n\n");
  snprintf(&conf_file_path[strlen(conf_file_path)], MAX_PATH_LENGTH, "%s%s",
    "------------------------------------------\n",
    "Use arrows to navigate through the options.");
    //MENUINSTR_EXIT);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_SELECT,
                          lxdialog_path,
                          "NEW WANPIPE CONFIGURATION FILE SELECTION",
                          WANCFG_PROGRAM_NAME,
                          conf_file_path,
                          MENU_HEIGTH, MENU_WIDTH,
                          num_of_visible_items,
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
    Debug(DBG_MENU_LIST_ALL_WANPIPES, ("file selected for editing: wanpipe%s\n",
      get_lxdialog_output_string()));

    //set the value for the caller
    wanpipe_number = atoi(get_lxdialog_output_string());

    snprintf(conf_file_path, MAX_PATH_LENGTH, "%swanpipe%d.conf",
      wanpipe_cfg_dir, wanpipe_number);

    file_exist = check_file_exist(conf_file_path, &file);
    if(file_exist == YES){
      yes_no_question(  selection_index,
                        lxdialog_path,
                        NO_PROTOCOL_NEEDED,
                        warning_confirm_overwriting_of_existing_conf_file_str);

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(create_new_configuration_file(wanpipe_number) == NO){
          rc = NO;
          exit_dialog = YES;
        }else{


        }
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
        //do nothing
        break;
      }
    }else{
      if(create_new_configuration_file(wanpipe_number) == NO){
        rc = NO;
        exit_dialog = YES;
      }
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
      create_new_configuration_file_help_str);
    break;

  case MENU_BOX_BUTTON_EXIT:
    //exit the dialog
    exit_dialog = YES;
    break;
  }

  if(exit_dialog == NO){
    goto again;
  }

cleanup:
  return rc;
}

int menu_list_all_wanpipes::create_new_configuration_file(int wanpipe_number)
{
  int rc = YES;
  int selection_index;
  //help text box
  text_box tb;

  //////////////////////////////////////////////////////////////////////////////////////
  //allocate object which will be used to hold temporary configuration.
  global_conf_file_reader_ptr = cfr_tmp = new conf_file_reader(wanpipe_number);
  if(cfr_tmp->init() != 0){
    rc = NO;
    goto cleanup;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  //set configuration for the new protocol
  {
    menu_new_device_configuration new_dev(lxdialog_path, &cfr_tmp);
    if(new_dev.run(&selection_index) == NO){
      Debug(DBG_MENU_LIST_ALL_WANPIPES,
        ("2. menu_list_existing_wanpipes.run() - failed!!\n"));
      rc = NO;
      goto cleanup;
    }
  }
  //////////////////////////////////////////////////////////////////////////////////////
cleanup:
  if(cfr_tmp != NULL){
    delete cfr_tmp;
    global_conf_file_reader_ptr = cfr_tmp = NULL;
  }
  return rc;
}

