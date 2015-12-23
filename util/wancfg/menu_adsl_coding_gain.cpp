/***************************************************************************
                          menu_adsl_coding_gain.cpp  -  description
                             -------------------
    begin                : Mon Mar 15 2004
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

#include "wancfg.h"
#include "menu_adsl_coding_gain.h"
#include "text_box.h"

#define DBG_ADSL_CODING_GAIN 1

menu_adsl_coding_gain::menu_adsl_coding_gain(IN char * lxdialog_path, IN wan_adsl_conf_t* adsl_cfg)
{
  Debug(DBG_ADSL_CODING_GAIN,
    ("menu_adsl_coding_gain::menu_adsl_coding_gain()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->adsl_cfg = adsl_cfg;
}

menu_adsl_coding_gain::~menu_adsl_coding_gain()
{
  Debug(DBG_ADSL_CODING_GAIN,
    ("menu_adsl_coding_gain::~menu_adsl_coding_gain()\n"));
}

int menu_adsl_coding_gain::run(OUT int * selection_index)
{
  string menu_str = "";
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog = NO;
  int number_of_items = 0;

  //help text box
  text_box tb;

again:
  Debug(DBG_ADSL_CODING_GAIN, ("menu_adsl_coding_gain::run()\n"));
 
  menu_str = "";
  rc = YES;
  exit_dialog = NO;
  number_of_items = 0;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_0DB_CODING_GAIN);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"0DB\" ");
  menu_str += tmp_buff;
  number_of_items++;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_1DB_CODING_GAIN);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"1DB\" ");
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_2DB_CODING_GAIN);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"2DB\" ");
  menu_str += tmp_buff;
  number_of_items++;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_3DB_CODING_GAIN);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"3DB\" ");
  menu_str += tmp_buff;
  number_of_items++;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_4DB_CODING_GAIN);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"4DB\" ");
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_5DB_CODING_GAIN);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"5DB\" ");
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_6DB_CODING_GAIN);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"6DB\" ");
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_7DB_CODING_GAIN);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"7DB\" ");
  menu_str += tmp_buff;
  number_of_items++;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_AUTO_CODING_GAIN);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Auto Coding (Default)\" ");
  menu_str += tmp_buff;
  number_of_items++;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\n\nSelect ADSL Coding.");

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_SELECT,
                          lxdialog_path,
                          "ADSL CODING OPTIONS",
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
    Debug(DBG_ADSL_CODING_GAIN,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    adsl_cfg->CodingGain = atoi(get_lxdialog_output_string());
    
    Debug(DBG_ADSL_CODING_GAIN,
      ("adsl_cfg->CodingGain: %s\n", ADSL_DECODE_GAIN(adsl_cfg->CodingGain)));
    exit_dialog = YES;
    break;

  case MENU_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
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
