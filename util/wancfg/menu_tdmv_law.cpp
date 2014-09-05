/***************************************************************************
                          menu_tdmv_law.cpp  -  description
                             -------------------
    begin                : Mon Nov 14 2005
    copyright            : (C) 2005 by David Rokhvarg
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

#include "menu_tdmv_law.h"
#include "text_box.h"

char* fxo_modes[] =
{
	"FCC", 
	"TBR21", 
	"ARGENTINA",
	"AUSTRALIA",
	"AUSTRIA", 
	"BAHRAIN", 
	"BELGIUM", 
	"BRAZIL", 
	"BULGARIA", 
	"CANADA",
	"CHILE",
	"CHINA",
	"COLUMBIA",
	"CROATIA", 
	"CYPRUS",
	"CZECH", 
	"DENMARK",
	"ECUADOR",
	"EGYPT",
	"ELSALVADOR",
	"FINLAND",
	"FRANCE",
	"GERMANY",
	"GREECE", 
	"GUAM",
	"HONGKONG",
	"HUNGARY", 
	"ICELAND", 
	"INDIA",
	"INDONESIA", 
	"IRELAND",
	"ISRAEL", 
	"ITALY", 
	"JAPAN", 
	"JORDAN",
	"KAZAKHSTAN",
	"KUWAIT", 
	"LATVIA",
	"LEBANON", 
	"LUXEMBOURG",
	"MACAO", 
	"MALAYSIA", 	
	"MALTA", 
	"MEXICO", 
	"MOROCCO", 
	"NETHERLANDS",
	"NEWZEALAND",
	"NIGERIA",
	"NORWAY",
	"OMAN", 
	"PAKISTAN", 
	"PERU",
	"PHILIPPINES", 
	"POLAND",
	"PORTUGAL", 
	"ROMANIA", 
	"RUSSIA", 
	"SAUDIARABIA",
	"SINGAPORE",
	"SLOVAKIA", 
	"SLOVENIA", 
	"SOUTHAFRICA", 
	"SOUTHKOREA",
	"SPAIN", 
	"SWEDEN",
	"SWITZERLAND", 
	"SYRIA", 
	"TAIWAN",
	"THAILAND", 
	"UAE", 	
	"UK",
	"USA", 
	"YEMEN",
	NULL
};

#define DBG_MENU_TDMV_LAW 1

menu_tdmv_law::menu_tdmv_law( IN char * lxdialog_path,
                              IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_TDMV_LAW, ("menu_tdmv_law::menu_tdmv_law()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_tdmv_law::~menu_tdmv_law()
{
  Debug(DBG_MENU_TDMV_LAW, ("menu_tdmv_law::menu_tdmv_law()\n"));
}

int menu_tdmv_law::run(OUT int * selection_index)
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

  Debug(DBG_MENU_TDMV_LAW, ("menu_tdmv_law::run()\n"));

again:

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_TDMV_LAW, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;

  menu_str = " ";

  Debug(DBG_MENU_TDMV_LAW, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  number_of_items = 2;

  menu_str.sprintf(" \"%d\" ", WAN_TDMV_MULAW);
  menu_str += " \"MuLaw\" ";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_TDMV_ALAW);
  menu_str += tmp_buff;
  menu_str += " \"ALaw\" ";

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect TDMV LAW for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT TDMV LAW",
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
    Debug(DBG_MENU_TDMV_LAW,
	("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    linkconf->fe_cfg.tdmv_law = atoi(get_lxdialog_output_string());
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

////////////////////////////////////////////////////////////////////
//opermode part
menu_tdmv_opermode::menu_tdmv_opermode( IN char * lxdialog_path,
                              IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_TDMV_LAW, ("menu_tdmv_opermode::menu_tdmv_opermode()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_tdmv_opermode::~menu_tdmv_opermode()
{
  Debug(DBG_MENU_TDMV_LAW, ("menu_tdmv_opermode::menu_tdmv_opermode()\n"));
}

int menu_tdmv_opermode::run(OUT int * selection_index)
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

  Debug(DBG_MENU_TDMV_LAW, ("menu_tdmv_opermode::run()\n"));

again:

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_TDMV_LAW, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;

  menu_str = " ";

  Debug(DBG_MENU_TDMV_LAW, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  number_of_items = 6;

  for(int ind = 0; fxo_modes[ind] != NULL; ind++){
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ind);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", fxo_modes[ind]);
    menu_str += tmp_buff;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect TDMV Operational Mode for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "TDMV OPERATIONAL MODE",
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
    Debug(DBG_MENU_TDMV_LAW,
	("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));
    snprintf(linkconf->fe_cfg.cfg.remora.opermode_name, WAN_RM_OPERMODE_LEN,
		fxo_modes[atoi(get_lxdialog_output_string())]);
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

