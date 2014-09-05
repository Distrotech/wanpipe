/***************************************************************************
                          menu_s508_memory_addr.cpp  -  description
                             -------------------
    begin                : Mon May 3 2004
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

#include "menu_s508_memory_addr.h"
#include "text_box.h"
#include "input_box.h"

#define DBG_MENU_S508_MEMORY_ADDR  1

enum MEMORY_ADDR{
  CURRENT_SETTING,
  MEM_AUTO,
  MEM_CUSTOM
};

menu_s508_memory_addr::menu_s508_memory_addr( IN char * lxdialog_path,
                                              IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_S508_MEMORY_ADDR,
    ("menu_s508_memory_addr::menu_s508_memory_addr()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_s508_memory_addr::~menu_s508_memory_addr()
{
  Debug(DBG_MENU_S508_MEMORY_ADDR,
    ("menu_s508_memory_addr::menu_s508_memory_addr()\n"));
}

int menu_s508_memory_addr::run(OUT int * selection_index)
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

  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility");

  Debug(DBG_MENU_S508_MEMORY_ADDR, ("menu_s508_memory_addr::run()\n"));

again:

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_S508_MEMORY_ADDR, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;

  menu_str = " ";

  Debug(DBG_MENU_S508_MEMORY_ADDR, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  number_of_items = 3;

  menu_str.sprintf(" \"%d\" ", CURRENT_SETTING);
  if(linkconf->maddr == 0x00){
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Current Setting : Auto\" ");
  }else{
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Current Setting : 0x%lX\" ", linkconf->maddr);
  }
  menu_str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", MEM_AUTO);
  menu_str += tmp_buff;
  menu_str += " \"Use Auto Memory Address(default)\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", MEM_CUSTOM);
  menu_str += tmp_buff;
  menu_str += " \"Use Custom Memory Address\" ";


  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect Memory Address for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT MEMORY ADDRESS",
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
    Debug(DBG_MENU_S508_MEMORY_ADDR,
      ("s508 memory addr: option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case CURRENT_SETTING:
      //ignore this selection
      exit_dialog = NO;
      break;

    case MEM_AUTO:
      linkconf->maddr = 0x00;
      exit_dialog = YES;
      break;

    case MEM_CUSTOM:

      exit_dialog = YES;
show_memaddr_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH,
        "Please specify Memory Address in Hex (e.g.: 0xEE000)");
      if(linkconf->maddr == 0){
        //if was set to 'auto', suggest custom setting
        snprintf(initial_text, MAX_PATH_LENGTH, "0xEE000");
      }else{
        snprintf(initial_text, MAX_PATH_LENGTH, "0x%lX", linkconf->maddr);
      }

      inb.set_configuration(  lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

      inb.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_S508_MEMORY_ADDR,
          ("memory address on return: %s\n", inb.get_lxdialog_output_string()));

        linkconf->maddr = strtoul(inb.get_lxdialog_output_string(), NULL, 16);
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          option_not_implemented_yet_help_str);
        goto show_memaddr_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;
    }

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
