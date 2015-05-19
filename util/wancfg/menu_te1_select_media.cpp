/***************************************************************************
                          menu_te1_select_media.cpp  -  description
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

#include "menu_te1_select_media.h"
#include "text_box_help.h"

char* media_type_help_str =
"Select Physical Medium:\n"
"\n"
"T1 or E1.\n";

#define DBG_MENU_TE1_SELECT_MEDIA 1

menu_te1_select_media::menu_te1_select_media( IN char * lxdialog_path,
                                              IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_TE1_SELECT_MEDIA, ("menu_te1_select_media::menu_te1_select_media()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_te1_select_media::~menu_te1_select_media()
{
  Debug(DBG_MENU_TE1_SELECT_MEDIA, ("menu_te1_select_media::~menu_te1_select_media()\n"));
}

int menu_te1_select_media::run(OUT int * selection_index)
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

  Debug(DBG_MENU_TE1_SELECT_MEDIA, ("menu_net_interface_setup::run()\n"));

again:
  number_of_items = 2;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_TE1_SELECT_MEDIA, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;
  old_media = linkconf->fe_cfg.media;

  menu_str = " ";

  Debug(DBG_MENU_TE1_SELECT_MEDIA, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_MEDIA_T1);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"T1\" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_MEDIA_E1);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"E1\" ");
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
    Debug(DBG_MENU_TE1_SELECT_MEDIA,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case WAN_MEDIA_T1:
      linkconf->fe_cfg.media = WAN_MEDIA_T1;
      if(old_media != linkconf->fe_cfg.media){
        //set default values for T1
        linkconf->fe_cfg.lcode = WAN_LCODE_B8ZS;
        linkconf->fe_cfg.frame = WAN_FR_ESF;
        linkconf->fe_cfg.cfg.te_cfg.lbo = WAN_T1_LBO_0_DB;
        linkconf->fe_cfg.cfg.te_cfg.te_clock = WAN_NORMAL_CLK;

      	if(cfr->link_defs->linkconf->card_type != WANOPT_ADSL &&
          cfr->link_defs->linkconf->card_type != WANOPT_S50X &&
          cfr->link_defs->linkconf->card_type != WANOPT_S51X){
          
	  wan_tdmv_conf_t *tdmv_conf = &cfr->link_defs->linkconf->tdmv_conf;
          //wan_xilinx_conf->tdmv_dchan = 24;
          tdmv_conf->dchan = 0;//user will have manually set/enable this option
        }	
      }
      exit_dialog = YES;
      break;

    case WAN_MEDIA_E1:
      linkconf->fe_cfg.media = WAN_MEDIA_E1;
      if(old_media != linkconf->fe_cfg.media){
        //set default values for E1
        linkconf->fe_cfg.lcode = WAN_LCODE_HDB3;
        linkconf->fe_cfg.frame = WAN_FR_CRC4;
        linkconf->fe_cfg.cfg.te_cfg.lbo = WAN_E1_120;
        linkconf->fe_cfg.cfg.te_cfg.te_clock = WAN_NORMAL_CLK;
        linkconf->fe_cfg.cfg.te_cfg.sig_mode = WAN_TE1_SIG_CCS;

      	if(cfr->link_defs->linkconf->card_type != WANOPT_ADSL &&
          cfr->link_defs->linkconf->card_type != WANOPT_S50X &&
          cfr->link_defs->linkconf->card_type != WANOPT_S51X){
          
	  wan_tdmv_conf_t *tdmv_conf = &cfr->link_defs->linkconf->tdmv_conf;
          //wan_xilinx_conf->tdmv_dchan = 16;
          tdmv_conf->dchan = 0;//user will have to manually set/enable this option
        }
      }
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
    case WAN_MEDIA_T1:
    case WAN_MEDIA_E1:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, media_type_help_str);
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
