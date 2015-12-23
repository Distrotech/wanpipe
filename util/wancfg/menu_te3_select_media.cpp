/***************************************************************************
                          menu_te3_select_media.cpp  -  description
                             -------------------
    begin                : Mon Apr 26 2004
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

#include "menu_te3_select_media.h"
#include "text_box_help.h"

char* te3_media_type_help_str =
"Select Physical Medium:\n"
"\n"
"T3 or E3.\n";

#define DBG_MENU_TE3_SELECT_MEDIA 1

menu_te3_select_media::menu_te3_select_media( IN char * lxdialog_path,
                                              IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_TE3_SELECT_MEDIA, ("menu_te3_select_media::menu_te3_select_media()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_te3_select_media::~menu_te3_select_media()
{
  Debug(DBG_MENU_TE3_SELECT_MEDIA, ("menu_te3_select_media::~menu_te3_select_media()\n"));
}

int menu_te3_select_media::run(OUT int * selection_index)
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

  Debug(DBG_MENU_TE3_SELECT_MEDIA, ("menu_net_interface_setup::run()\n"));

again:
  number_of_items = 2;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_TE3_SELECT_MEDIA, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;
  old_media = linkconf->fe_cfg.media;

  menu_str = " ";

  Debug(DBG_MENU_TE3_SELECT_MEDIA, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_MEDIA_DS3);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"T3\" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_MEDIA_E3);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"E3\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect Physical Medium for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT PHYSICAL MEDIUM",
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
    Debug(DBG_MENU_TE3_SELECT_MEDIA,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case WAN_MEDIA_DS3:
      set_default_t3_configuration(&linkconf->fe_cfg);
      exit_dialog = YES;
      break;

    case WAN_MEDIA_E3:
      set_default_e3_configuration(&linkconf->fe_cfg);
      exit_dialog = YES;
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
    }
    break;

  case MENU_BOX_BUTTON_HELP:

    switch(atoi(get_lxdialog_output_string()))
    {
    case WAN_MEDIA_DS3:
    case WAN_MEDIA_E3:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, te3_media_type_help_str);
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
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
