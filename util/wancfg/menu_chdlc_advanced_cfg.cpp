/***************************************************************************
                          menu_chdlc_interface_cfg.cpp  -  description
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

#include "menu_chdlc_advanced_cfg.h"
#include "text_box.h"
#include "text_box_yes_no.h"
#include "input_box.h"


char* chdlc_ignore_dcd_help_str =
"IGNORE DCD\n"
"\n"
"This parameter decides whether DCD will be ignored or\n"
"not when determining active link status for Cisco HDLC.\n"
"\n"
"Options:\n"
"\n"
"YES: Ignore DCD\n"
"NO : Do not ignore DCD\n"
"\n"
"Default: NO\n";

char* chdlc_ignore_cts_help_str =
"IGNORE CTS\n"
"\n"
"This parameter decides whether CTS will be ignored or\n"
"not when determining active link status for Cisco HDLC.\n"
"\n"
"Options:\n"
"\n"
"YES : Ignore CTS\n"
"NO  : Do not ignore CTS\n"
"\n"
"Default: NO\n";

char* chdlc_ignore_keepalive_help_str =
"IGNORE KEEPALIVE Packets\n"
"\n"
"This parameter decides whether Keep alives will be\n"
"ignored or not when determining active link status for\n"
"Cisco HDLC.\n"
"\n"
"Options:\n"
"YES : Ignore Keep Alive packets\n"
"NO  : Do not ignore Keep Alive packets\n"
"\n"
"Default: NO\n";

char* chdlc_keepalive_tx_timer_help_str =
"KEEPALIVE_TX_TIMER\n"
"\n"
"This parameter states the interval between keep alive.\n"
"If you are set to ignore keepalives then this value is\n"
"meaningless. The value of this parameter is given in\n"
"milliseconds.\n"
"\n"
"Range:   0 - 60000 ms.\n"
"Default: 10000 ms\n";

char* chdlc_keepalive_rx_timer_help_str =
"KEEPALIVE_RX_TIMER\n"
"This parameter states the interval to expect keepalives\n"
"If you are set to ignore keepalives then this value is\n"
"meaningless. The value of this parameter is given in\n"
"milliseconds.\n"
"\n"
"Range  : 10 - 60000 ms.\n"
"Default: 11000 ms\n";

char* chdlc_keepalive_err_margin_help_str =
"KEEPALIVE_ERR_MARGIN\n"
"\n"
"This parameter states the number of consecutive keep\n"
"alive timeouts before bringing down the link.  If  you\n"
"are set to ignore keepalives then this value is meaning\n"
"less.\n"
"\n"
"Range: 1 - 20.\n"
"Default: 5\n";

enum CHDLC_BASIC_CFG_OPTIONS{

  //Ignore DCD  --------------> NO          ? ?
  IGNORE_DCD=1,
  //Ignore CTS  --------------> NO          ? ?
  IGNORE_CTS,
  //Ignore Keepalive ---------> NO          ? ?
  IGNORE_KEEPALIVE,

  //Keep Alive Tx Timer ------> 10000       ? ?
  KEEPALIVE_TX_TIMER,
  //Keep Alive Rx Timer ------> 11000       ? ?
  KEEPALIVE_RX_TIMER,
  //Keep Alive Error Margin --> 5
  KEEPALIVE_ERR_MARGIN,

  TX_RX_KEEPALIVE_TIMER
};

#define DBG_MENU_CHDLC_ADVANCED_CFG  1

menu_chdlc_advanced_cfg::menu_chdlc_advanced_cfg(
				IN char * lxdialog_path,
                                IN list_element_chan_def* parent_element_logical_ch)
{
  Debug(DBG_MENU_CHDLC_ADVANCED_CFG, ("menu_chdlc_advanced_cfg::menu_chdlc_advanced_cfg()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  
  name_of_parent_layer = parent_element_logical_ch->data.name;
  this->parent_element_logical_ch = parent_element_logical_ch;
}

menu_chdlc_advanced_cfg::~menu_chdlc_advanced_cfg()
{
  Debug(DBG_MENU_CHDLC_ADVANCED_CFG, ("menu_chdlc_advanced_cfg::~menu_chdlc_advanced_cfg()\n"));
}

int menu_chdlc_advanced_cfg::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog = NO;
  int int_tmp;

  //help text box
  text_box tb;
  wanif_conf_t* chanconf;

  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s", get_protocol_string(WANCONFIG_CHDLC));

  list_element_chan_def* list_el_chan_def = NULL;
  objects_list * obj_list = NULL;

  Debug(DBG_MENU_CHDLC_ADVANCED_CFG, ("menu_chdlc_advanced_cfg::run()\n"));

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
/*
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", IGNORE_DCD);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Ignore DCD--------------> %s\" ",
    (chanconf->ignore_dcd == 0 ? "NO" : "YES"));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", IGNORE_CTS);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Ignore CTS--------------> %s\" ",
    (chanconf->ignore_cts == 0 ? "NO" : "YES"));
  menu_str += tmp_buff;
*/
  //////////////////////////////////////////////////////////////////////////////////////
/*
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", IGNORE_KEEPALIVE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Ignore Keepalive--------> %s\" ",
    (chanconf->ignore_keepalive == WANOPT_NO ? "NO" : "YES"));
  menu_str += tmp_buff;
*/

//  if(chanconf->ignore_keepalive == WANOPT_NO){
    //do NOT ignore keepalive
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TX_RX_KEEPALIVE_TIMER);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Keepalive Timer---------> %d\" ",
      chanconf->u.ppp.sppp_keepalive_timer);
    menu_str += tmp_buff;

    //////////////////////////////////////////////////////////////////////////////////////
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", KEEPALIVE_ERR_MARGIN);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Keepalive Error Margin--> %d\" ",
      chanconf->u.ppp.keepalive_err_margin);
    //  chanconf->keepalive_err_margin);
    menu_str += tmp_buff;
//  }

/*
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", KEEPALIVE_TX_TIMER);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Keepalive TX Timer------> %d\" ",
    chanconf->keepalive_tx_tmr);
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", KEEPALIVE_RX_TIMER);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Keepalive RX Timer------> %d\" ",
    chanconf->keepalive_rx_tmr);
  menu_str += tmp_buff;
*/

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nAdvanced CHDLC configuration options for\
\nWan Device: %s", name_of_parent_layer);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "ADVANCED CHDLC CONFIGURATION",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          6,//number of items in the menu, including the empty lines
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
    Debug(DBG_MENU_CHDLC_ADVANCED_CFG,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case IGNORE_DCD:

      snprintf(tmp_buff, MAX_PATH_LENGTH,
"Do you want to %s DCD Monitoring?\n",
(chanconf->ignore_dcd == 0 ?
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
        if(chanconf->ignore_dcd == 1){
          chanconf->ignore_dcd = 0;
        }else{
          chanconf->ignore_dcd = 1;
        }
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
        break;
      }
      break;

    case IGNORE_CTS:

      snprintf(tmp_buff, MAX_PATH_LENGTH,
"Do you want to %s CTS Monitoring?\n",
(chanconf->ignore_cts == 0 ?
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
        if(chanconf->ignore_cts == 1){
          chanconf->ignore_cts = 0;
        }else{
          chanconf->ignore_cts = 1;
        }
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
        break;
      }
      break;

    case IGNORE_KEEPALIVE:

      snprintf(tmp_buff, MAX_PATH_LENGTH,
"Do you want to %s Keep Alive Monitoring?\n",
(chanconf->ignore_keepalive == WANOPT_NO ?
"disable" : "enable"));

      if(yes_no_question(   selection_index,
                            lxdialog_path,
                            WANCONFIG_CHDLC,
                            tmp_buff) == NO){
        rc = NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(chanconf->ignore_keepalive == WANOPT_YES){
	  //was set to ignore
          chanconf->ignore_keepalive = WANOPT_NO;
	  chanconf->u.ppp.sppp_keepalive_timer = DEFAULT_Tx_KPALV_TIMER; 
          chanconf->u.ppp.keepalive_err_margin = DEFAULT_KPALV_ERR_TOL;
        }else{
	  //was set NOT to ingnore
          chanconf->ignore_keepalive = WANOPT_YES;
	  chanconf->u.ppp.sppp_keepalive_timer = 0;
          chanconf->u.ppp.keepalive_err_margin = 0;
        }
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
        break;
      }
      break;
#if 0
    case KEEPALIVE_TX_TIMER:

show_keepalive_tx_timer_input_box:

      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify Keepalive Tx timer");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d",
        chanconf->keepalive_tx_tmr);

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
        Debug(DBG_MENU_CHDLC_ADVANCED_CFG,
          ("keepalive tx timer on return: %s\n", inb.get_lxdialog_output_string()));

        int_tmp = atoi(inb.get_lxdialog_output_string());
        if(int_tmp < MIN_Tx_KPALV_TIMER || int_tmp > MAX_Tx_KPALV_TIMER){
          tb.show_error_message(lxdialog_path, WANCONFIG_CHDLC,
            "Invalid Keepalive Tx timer! Min: %d, Max %d.",
            MIN_Tx_KPALV_TIMER,
            MAX_Tx_KPALV_TIMER);
          goto show_keepalive_tx_timer_input_box;
        }else{
          chanconf->keepalive_tx_tmr = int_tmp;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, WANCONFIG_CHDLC,
          option_not_implemented_yet_help_str);
        goto show_keepalive_tx_timer_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case KEEPALIVE_RX_TIMER:
show_keepalive_rx_timer_input_box:

      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify Keepalive Rx timer");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d",
        chanconf->keepalive_rx_tmr);

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
        Debug(DBG_MENU_CHDLC_ADVANCED_CFG,
          ("keepalive rx timer on return: %s\n", inb.get_lxdialog_output_string()));

        int_tmp = atoi(inb.get_lxdialog_output_string());
        if(int_tmp < MIN_Rx_KPALV_TIMER || int_tmp > MAX_Rx_KPALV_TIMER){
          tb.show_error_message(lxdialog_path, WANCONFIG_CHDLC,
            "Invalid Keepalive Rx timer! Min: %d, Max: %d.",
            MIN_Rx_KPALV_TIMER,
            MAX_Rx_KPALV_TIMER);
          goto show_keepalive_rx_timer_input_box;
        }else{
          chanconf->keepalive_rx_tmr = int_tmp;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, WANCONFIG_CHDLC,
          option_not_implemented_yet_help_str);
        goto show_keepalive_rx_timer_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;
#endif

    case KEEPALIVE_ERR_MARGIN:
show_keepalive_err_margin_input_box:

      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify Keep Alive Error Margin");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d",
        chanconf->u.ppp.keepalive_err_margin);

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
        Debug(DBG_MENU_CHDLC_ADVANCED_CFG,
          ("keepalive err margin on return: %s\n", inb.get_lxdialog_output_string()));

        int_tmp = atoi(inb.get_lxdialog_output_string());
        if(int_tmp < MIN_KPALV_ERR_TOL || int_tmp > MAX_KPALV_ERR_TOL){
          tb.show_error_message(lxdialog_path, WANCONFIG_CHDLC,
            "Invalid Keep Alive Error Margin! Min: %d, Max %d.",
            MIN_KPALV_ERR_TOL,
            MAX_KPALV_ERR_TOL);
          goto show_keepalive_err_margin_input_box;
        }else{
          chanconf->u.ppp.keepalive_err_margin = int_tmp;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, WANCONFIG_CHDLC,
          option_not_implemented_yet_help_str);
        goto show_keepalive_err_margin_input_box;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case TX_RX_KEEPALIVE_TIMER:
show_tx_rx_keepalive_timer_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify Keep Alive Timer");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", chanconf->u.ppp.sppp_keepalive_timer);

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
        Debug(DBG_MENU_CHDLC_ADVANCED_CFG,("keepalive timer on return: %s\n",
		inb.get_lxdialog_output_string()));

        int_tmp = atoi(inb.get_lxdialog_output_string());
        if(int_tmp < MIN_Tx_KPALV_TIMER || int_tmp > MAX_Tx_KPALV_TIMER){
          tb.show_error_message(lxdialog_path, WANCONFIG_CHDLC,
            "Invalid Keepalive timer! Min: %d, Max %d.",
            MIN_Tx_KPALV_TIMER,
            MAX_Tx_KPALV_TIMER);
          goto show_tx_rx_keepalive_timer_input_box;
        }else{
          chanconf->u.ppp.sppp_keepalive_timer = int_tmp;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, WANCONFIG_CHDLC,
          option_not_implemented_yet_help_str);
        goto show_tx_rx_keepalive_timer_input_box;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
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
    case IGNORE_DCD:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, chdlc_ignore_dcd_help_str);
      break;

    case IGNORE_CTS:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, chdlc_ignore_cts_help_str);
      break;

    case IGNORE_KEEPALIVE:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, chdlc_ignore_keepalive_help_str);
      break;

    case KEEPALIVE_TX_TIMER:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, chdlc_keepalive_tx_timer_help_str);
      break;

    case KEEPALIVE_RX_TIMER:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, chdlc_keepalive_rx_timer_help_str);
      break;

    case KEEPALIVE_ERR_MARGIN:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, chdlc_keepalive_err_margin_help_str);
      break;
    }//switch()
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
