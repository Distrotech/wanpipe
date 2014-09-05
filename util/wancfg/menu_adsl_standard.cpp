/***************************************************************************
                          menu_adsl_standard.cpp  -  description
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
#include "menu_adsl_standard.h"
#include "text_box.h"

#define DBG_ADSL_STANDARD 1


menu_adsl_standard::menu_adsl_standard(IN char * lxdialog_path, IN wan_adsl_conf_t* adsl_cfg)
{
  Debug(DBG_ADSL_STANDARD,
    ("menu_adsl_standard::menu_adsl_standard()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->adsl_cfg = adsl_cfg;
}

menu_adsl_standard::~menu_adsl_standard()
{
  Debug(DBG_ADSL_STANDARD,
    ("menu_adsl_standard::~menu_adsl_standard()\n"));
}

int menu_adsl_standard::run(OUT int * selection_index)
{
  string menu_str = "";
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog = NO;
  int number_of_items = 0;

  //help text box
  text_box tb;

again:
  Debug(DBG_ADSL_STANDARD, ("menu_adsl_standard::run()\n"));
 
  menu_str = "";
  rc = YES;
  exit_dialog = NO;
  number_of_items = 0;
 
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_T1_413);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"T1_413\" ");
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_G_LITE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"G_LITE\" ");
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_G_DMT);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"G_DMT\" ");
  menu_str += tmp_buff;
  number_of_items++;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_ALCATEL_1_4);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"ALCATEL_1_4\" ");
  menu_str += tmp_buff;
  number_of_items++;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_MULTIMODE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"MULTIMODE\" ");
  menu_str += tmp_buff;
  number_of_items++;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_ADI);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"ADI\" ");
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_ALCATEL);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"ALCATEL\" ");
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_ADSL_T1_413_AUTO);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"T1_413_AUTO\" ");
  menu_str += tmp_buff;
  number_of_items++;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\n\nSelect ADSL standard.");

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_SELECT,
                          lxdialog_path,
                          "ADSL STANDARD",
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
    Debug(DBG_ADSL_STANDARD,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    adsl_cfg->Standard = atoi(get_lxdialog_output_string());
    
    Debug(DBG_ADSL_STANDARD,
      ("adsl_cfg->Standard: %s\n", ADSL_DECODE_STANDARD(adsl_cfg->Standard)));
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
