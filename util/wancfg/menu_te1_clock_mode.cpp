/***************************************************************************
                          menu_te1_clock_mode.cpp  -  description
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

#include "menu_te1_clock_mode.h"
#include "text_box.h"

#define DBG_MENU_TE1_CLOCK_MODE 1

menu_te1_clock_mode::menu_te1_clock_mode( IN char * lxdialog_path,
                                          IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_TE1_CLOCK_MODE,
    ("menu_te1_clock_mode::menu_te1_clock_mode()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_te1_clock_mode::~menu_te1_clock_mode()
{
  Debug(DBG_MENU_TE1_CLOCK_MODE,
    ("menu_te1_clock_mode::menu_te1_clock_mode()\n"));
}

int menu_te1_clock_mode::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  int number_of_items;
  unsigned char old_media;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;

  Debug(DBG_MENU_TE1_CLOCK_MODE, ("menu_te1_clock_mode::run()\n"));

again:

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_TE1_CLOCK_MODE, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;
  old_media = linkconf->fe_cfg.media;

  menu_str = " ";

  Debug(DBG_MENU_TE1_CLOCK_MODE, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  number_of_items = 2;

  menu_str.sprintf(" \"%d\" ", WAN_NORMAL_CLK);
  menu_str += " \"Normal\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_MASTER_CLK);
  menu_str += tmp_buff;
  menu_str += " \"Master\" ";

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect T1/E1 Clock Mode for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT CLOCK MODDE",
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
    Debug(DBG_MENU_TE1_CLOCK_MODE,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    linkconf->fe_cfg.cfg.te_cfg.te_clock = atoi(get_lxdialog_output_string());
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
