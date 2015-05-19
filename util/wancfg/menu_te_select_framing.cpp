/***************************************************************************
                          menu_te_select_framing.cpp  -  description
                             -------------------
    begin                : Wed Apr 28 2004
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

#include "menu_te_select_framing.h"
#include "text_box.h"

#define DBG_MENU_TE1_SELECT_FRAMING 1

menu_te_select_framing::menu_te_select_framing( IN char * lxdialog_path,
                                                  IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_TE1_SELECT_FRAMING,
    ("menu_te_select_framing::menu_te_select_framing()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_te_select_framing::~menu_te_select_framing()
{
  Debug(DBG_MENU_TE1_SELECT_FRAMING,
    ("menu_te_select_framing::~menu_te_select_framing()\n"));
}

int menu_te_select_framing::run(OUT int * selection_index)
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

  Debug(DBG_MENU_TE1_SELECT_FRAMING, ("menu_te1_select_line_framing::run()\n"));

again:

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_TE1_SELECT_FRAMING, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;
  old_media = linkconf->fe_cfg.media;

  menu_str = " ";

  Debug(DBG_MENU_TE1_SELECT_FRAMING, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  number_of_items = 3;
  switch(linkconf->fe_cfg.media)
  {
  case WAN_MEDIA_T1:

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_FR_ESF);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"ESF\" ");
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_FR_D4);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"D4\" ");
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_FR_UNFRAMED);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Unframed\" ");
    menu_str += tmp_buff;
    break;

  case WAN_MEDIA_E1: 

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_FR_NCRC4);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"non-CRC4\" ");
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_FR_CRC4);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"CRC4\" ");
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_FR_UNFRAMED);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Unframed\" ");
    menu_str += tmp_buff;
    break;

  case WAN_MEDIA_DS3: 

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_FR_DS3_Cbit);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Cbit\" ");
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_FR_DS3_M13);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"M13\" ");
    menu_str += tmp_buff;
    break;
    
  case WAN_MEDIA_E3: 

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_FR_E3_G751);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"G751\" ");
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_FR_E3_G832);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"G832\" ");
    menu_str += tmp_buff;
    break;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect Line Framing for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT LINE FRAMING",
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
    Debug(DBG_MENU_TE1_SELECT_FRAMING,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    linkconf->fe_cfg.frame = atoi(get_lxdialog_output_string());
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
