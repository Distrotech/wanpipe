/***************************************************************************
                          menu_s508_io_port_select.cpp  -  description
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

#include "menu_s508_io_port_select.h"
#include "text_box.h"

#define DBG_MENU_S508_IO_PORT_SELECT  1

menu_s508_io_port_select::menu_s508_io_port_select( IN char * lxdialog_path,
                                                    IN conf_file_reader* ptr_cfr)

{
  Debug(DBG_MENU_S508_IO_PORT_SELECT,
    ("menu_s508_io_port_select::menu_s508_io_port_select()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_s508_io_port_select::~menu_s508_io_port_select()
{
  Debug(DBG_MENU_S508_IO_PORT_SELECT,
    ("menu_s508_io_port_select::~menu_s508_io_port_select()\n"));
}

int menu_s508_io_port_select::run(OUT int * selection_index)
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

  Debug(DBG_MENU_S508_IO_PORT_SELECT, ("menu_te1_clock_mode::run()\n"));

again:

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_S508_IO_PORT_SELECT, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;
  old_media = linkconf->fe_cfg.media;

  menu_str = " ";

  Debug(DBG_MENU_S508_IO_PORT_SELECT, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  number_of_items = 8;

  menu_str.sprintf(" \"%d\" ", 0x360);
  menu_str += " \"0x360-0x363 (default)\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", 0x390);
  menu_str += tmp_buff;
  menu_str += " \"0x390-0x393\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", 0x380);
  menu_str += tmp_buff;
  menu_str += " \"0x380-0x383\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", 0x350);
  menu_str += tmp_buff;
  menu_str += " \"0x350-0x353\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", 0x300);
  menu_str += tmp_buff;
  menu_str += " \"0x300-0x303\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", 0x280);
  menu_str += tmp_buff;
  menu_str += " \"0x280-0x283\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", 0x270);
  menu_str += tmp_buff;
  menu_str += " \"0x270-0x273\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", 0x250);
  menu_str += tmp_buff;
  menu_str += " \"0x250-0x253\" ";


  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect IO Port for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT IO PORT",
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
    Debug(DBG_MENU_S508_IO_PORT_SELECT,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    linkconf->ioport = atoi(get_lxdialog_output_string());
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
