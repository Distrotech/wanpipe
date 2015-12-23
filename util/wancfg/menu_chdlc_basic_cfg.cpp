/***************************************************************************
                          menu_chdlc_basic_cfg.cpp  -  description
                             -------------------
    begin                : Mon Apr 5 2004
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

#include "menu_chdlc_basic_cfg.h"
#include "menu_chdlc_advanced_cfg.h"
#include "text_box.h"
#include "text_box_yes_no.h"

char* chdlc_dynamic_ip_help_str =
"CHDLC IP ADDRESSING MODE\n"
"\n"
"Used to enable STATIC or DYNAMIC IP addressing\n"
"\n"
"Options:\n"
"-------\n"
"YES : Use Dynamic IP Addressing\n"
"NO  : Use Static IP Addressing\n"
"\n"
"Note: Dummy IP addresses must be supplied for\n"
"LOCAL and POINTOPOINT IP addresses, in\n"
"ip interface setup, regardless of the IP MODE\n"
"used.\n";

char* chdlc_advanced_options_help_str =
"Advanced CHDLC options:\n"
"\n"
" -Ignore DCD\n"
" -Ignore CTS\n"
" -Ignore Keepalive\n"
" -Keepalive TX Timer\n"
" -Keepalive RX Timer\n"
" -Keepalive Error Margin\n";


#define DBG_MENU_CHDLC_BASIC_CFG 1

enum CHDLC_IF_OPTIONS{

  //Dynamic IP Addressing ----> NO
  DYNAMIC_IP_CFG,
  //if dynamic IP is 'on' display value of the SLARP timer
  SLARP_TIMER_VALUE,
  ADVANCED_CHDLC_CFG,
  EMPTY_LINE
};
  
menu_chdlc_basic_cfg::menu_chdlc_basic_cfg( IN char * lxdialog_path,
					    IN list_element_chan_def* parent_element_logical_ch)
{
  Debug(DBG_MENU_CHDLC_BASIC_CFG,
    ("menu_chdlc_basic_cfg::menu_chdlc_basic_cfg()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  name_of_parent_layer = parent_element_logical_ch->data.name;
  this->parent_element_logical_ch = parent_element_logical_ch;
}

menu_chdlc_basic_cfg::~menu_chdlc_basic_cfg()
{
  Debug(DBG_MENU_CHDLC_BASIC_CFG,
    ("menu_chdlc_basic_cfg::~menu_chdlc_basic_cfg()\n"));
}

int menu_chdlc_basic_cfg::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog = NO;

  //help text box
  text_box tb;
  wanif_conf_t* chanconf;
  
  list_element_chan_def* list_el_chan_def = NULL;
  objects_list * obj_list = NULL;

  Debug(DBG_MENU_CHDLC_BASIC_CFG, ("menu_chdlc_basic_cfg::run()\n"));

again:
  rc = YES;
  menu_str = " ";

  obj_list = (objects_list*)parent_element_logical_ch->next_objects_list;
  if(obj_list == NULL){
    ERR_DBG_OUT(("Invalid 'obj_list' pointer!!\n"));
    return NO;
  }
  
  list_el_chan_def = (list_element_chan_def*)obj_list->get_first();
  if(list_el_chan_def == NULL){
    ERR_DBG_OUT(("Invalid 'ppp interface' pointer!!\n"));
    return NO;
  }

  chanconf = list_el_chan_def->data.chanconf;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", DYNAMIC_IP_CFG);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Dynamic IP Addressing-------> %s\" ",
    (chanconf->slarp_timer == 0 ? "NO" : "YES"));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ADVANCED_CHDLC_CFG);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Advanced CHDLC configuration\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nCHDLC configuration options for\
\nWan Device: %s", name_of_parent_layer);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "CHDLC CONFIGURATION",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          3,//number of items in the menu, including the empty lines
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
    Debug(DBG_MENU_CHDLC_BASIC_CFG,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case DYNAMIC_IP_CFG:

      snprintf(tmp_buff, MAX_PATH_LENGTH,
"Do you want to %s Dynamic IP Addressing?\n",
(chanconf->slarp_timer == 0 ?
"enable" : "disable"));

      if(yes_no_question(   selection_index,
                            lxdialog_path,
                            WANCONFIG_CHDLC,
                            tmp_buff) == NO){
        rc = NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(chanconf->slarp_timer == 1){
          chanconf->slarp_timer = 0;
        }else{
          chanconf->slarp_timer = 1;
        }
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
        break;
      }
      break;

    case EMPTY_LINE:
      //do nothing
      break;

    case ADVANCED_CHDLC_CFG:
      {
        menu_chdlc_advanced_cfg chdlc_advanced_cfg(lxdialog_path, parent_element_logical_ch);
        chdlc_advanced_cfg.run(selection_index);
      }
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
    }
    break;

  case MENU_BOX_BUTTON_HELP:

    switch(atoi(get_lxdialog_output_string()))
    {
    case DYNAMIC_IP_CFG:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, chdlc_dynamic_ip_help_str);
      break;

    case ADVANCED_CHDLC_CFG:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, chdlc_advanced_options_help_str);
      break;

    case EMPTY_LINE:
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

