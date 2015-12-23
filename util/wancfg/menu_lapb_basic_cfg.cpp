/***************************************************************************
                          menu_lapb_basic_cfg.cpp  -  description
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

#include "text_box.h"
#include "text_box_yes_no.h"
#include "input_box.h"
#include "menu_lapb_basic_cfg.h"

#define DBG_MENU_LAPB_BASIC_CFG 1

enum LAPB_OPTIONS{
  STATION,
  T1,
  T2,
  N2,
  T3,
  T4,
  WINDOW,
  MODE,
  MTU
};

char* station_type_help_str =
"STATION TYPE:\n"
"\n"
"This parameter defines whether the\n"
"adapter should operate as a Data Terminal Equipment (DTE)\n"
"or Data Circuit Equipment (DCE).\n"
"\n"
"Normally, you should select DTE mode.  DCE mode\n"
"is primarily used in back-to-back  testing configurations.\n"
"\n"
"Options:\n"
"--------\n"
"DTE     DTE mode (default)\n"
"DCE     DCE mode\n";
				  
char* t1_help_str = "Valid values are 1 to 30 seconds.";
	
char* t2_help_str = 
"Valid values are 0 to 29 seconds.\n"
"Note: T2 timer must be lower than T1 !!!";

char* n2_help_str = 
"N2 Counter:\n\n"
"Maximum number of transmissions and\n"
"retransmission of an HDLC frame at T1 interfals\n"
"before a state change occurs.\n\n"
"Valid values are 1 to 30.\n\n"
"Default: 10";

char* t4_help_str = 
"T4 Timer:\n"
"\n"
"T4 timer is used to allow the Sangoma HDLC\n"
"code to issue a link level Supervisory frame\n"
"periodically during the 'quiescent' ABM link phase.\n"
"\n"
"Options:\n"
"--------\n"
"0    : No unnecessary Supervisory frames\n"
"T4*T1: Seconds between each transmission\n"
"\n"
"ex: T1=3 and T4=10, Therefore, 30 seconds between\n"
"transmissions.\n"
"\n"
"Valid values for this parameter are 0 to 240.\n"
"\n"
"Default: 0";

char* window_help_str = 
"PACKET WINDOW:\n"
"\n"
"This parameter defines the default size of the X.25\n"
"packet window, i.e. the maximum number of sequentially\n"
"numbered data packets that can be sent without waiting\n"
"for acknowledgment.\n"
"\n"
"Valid values are from 1 to 7.\n"
"\n"
"Default is 7.\n";

menu_lapb_basic_cfg::menu_lapb_basic_cfg( IN char * lxdialog_path,
					    IN list_element_chan_def* parent_element_logical_ch)
{
  Debug(DBG_MENU_LAPB_BASIC_CFG,
    ("menu_lapb_basic_cfg::menu_lapb_basic_cfg()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  name_of_parent_layer = parent_element_logical_ch->data.name;
  this->parent_element_logical_ch = parent_element_logical_ch;
}

menu_lapb_basic_cfg::~menu_lapb_basic_cfg()
{
  Debug(DBG_MENU_LAPB_BASIC_CFG,
    ("menu_lapb_basic_cfg::~menu_lapb_basic_cfg()\n"));
}

int menu_lapb_basic_cfg::run(OUT int * selection_index)
{
  string menu_str;
  int rc, number_of_menu_items = 9;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog = NO;

  //help text box
  text_box tb;
  wanif_conf_t* chanconf;
  wan_lapb_if_conf_t *lapb_cfg;

  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility");
  
  list_element_chan_def* list_el_chan_def = NULL;
  objects_list * obj_list = NULL;

  Debug(DBG_MENU_LAPB_BASIC_CFG, ("menu_lapb_basic_cfg::run()\n"));

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
    ERR_DBG_OUT(("Invalid 'LAPB interface' pointer!!\n"));
    return NO;
  }

  chanconf = list_el_chan_def->data.chanconf;
  lapb_cfg = &list_el_chan_def->data.chanconf->u.lapb;
	
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", STATION);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Station-------> %s\" ",
    (lapb_cfg->station == WANOPT_DTE ? "DTE" : "DCE"));
  menu_str += tmp_buff;
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", T1);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"T1-------> %d\" ",lapb_cfg->t1);
  menu_str += tmp_buff;
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", T2);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"T2-------> %d\" ", lapb_cfg->t2);
  menu_str += tmp_buff;
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", N2);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"N2-------> %d\" ", lapb_cfg->n2);
  menu_str += tmp_buff;
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", T3);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"T3-------> %d\" ", lapb_cfg->t3);
  menu_str += tmp_buff;
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", T4);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"T4-------> %d\" ", lapb_cfg->t4);
  menu_str += tmp_buff;
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WINDOW);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Packet Window-> %d\" ", lapb_cfg->window);
  menu_str += tmp_buff;
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", MODE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Mode-----> %d\" ", lapb_cfg->mode);
  menu_str += tmp_buff;
  /////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", MTU);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"MTU------> %d\" ", lapb_cfg->mtu);
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nLAPB configuration options for\
\nWan Device: %s", 
chanconf->name);
//name_of_parent_layer);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "LAPB CONFIGURATION",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          number_of_menu_items,
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
    Debug(DBG_MENU_LAPB_BASIC_CFG, ("option selected for editing: %s\n", 
			    get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case STATION:
      {
        menu_lapb_station_type lapb_station_type(lxdialog_path);
        lapb_cfg->station = lapb_station_type.run(lapb_cfg->station);
      }
      break;
      
    case T1:
      {
	int t1;

show_t1_input_box:
        snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify T1 timer (default 3)");
        snprintf(initial_text, MAX_PATH_LENGTH, "%d", lapb_cfg->t1);

        inb.set_configuration(lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

        inb.show(selection_index);

        switch(*selection_index)
        {
        case INPUT_BOX_BUTTON_OK:
          t1 = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

          if(t1 < 1 || t1 > 30){
            tb.show_error_message(lxdialog_path, WANCONFIG_LAPB, "Invalid T1!\n\n%s\n", 
			     t1_help_str);
            goto show_t1_input_box;
          }else{
            lapb_cfg->t1 = t1;
          }
          break;

        case INPUT_BOX_BUTTON_HELP:
          tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, t1_help_str);
          goto show_t1_input_box;
        }//switch(*selection_index)
      }
      break;
	      
    case T2:
      {
	int t2;

show_t2_input_box:
        snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify T1 timer (default 0)");
        snprintf(initial_text, MAX_PATH_LENGTH, "%d", lapb_cfg->t2);

        inb.set_configuration(lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

        inb.show(selection_index);

        switch(*selection_index)
        {
        case INPUT_BOX_BUTTON_OK:
          t2 = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

          if(t2 < 0 || t2 > 29){
            tb.show_error_message(lxdialog_path, WANCONFIG_LAPB, "Invalid T2!\n\n%s\n", 
			     t2_help_str);
            goto show_t2_input_box;
          }else{
            lapb_cfg->t2 = t2;
          }
          break;

        case INPUT_BOX_BUTTON_HELP:
          tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, t2_help_str);
          goto show_t2_input_box;
        }//switch(*selection_index)
      }
      break;

    case N2:
      {
	int n2;
	
show_n2_input_box:
        snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify N2 counter (default 10)");
        snprintf(initial_text, MAX_PATH_LENGTH, "%d", lapb_cfg->n2);

        inb.set_configuration(lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

        inb.show(selection_index);

        switch(*selection_index)
        {
        case INPUT_BOX_BUTTON_OK:
          n2 = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

          if(n2 < 1 || n2 > 30){
            tb.show_error_message(lxdialog_path, WANCONFIG_LAPB, "Invalid N2!\n\n%s\n", 
			     n2_help_str);
            goto show_n2_input_box;
          }else{
            lapb_cfg->n2 = n2;
          }
          break;

        case INPUT_BOX_BUTTON_HELP:
          tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, n2_help_str);
          goto show_n2_input_box;
        }//switch(*selection_index)
      }
      break;
      
    case T3:
      {
show_t3_input_box:
        snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify T3 timer (default 6)");
        snprintf(initial_text, MAX_PATH_LENGTH, "%d", lapb_cfg->t3);

        inb.set_configuration(lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

        inb.show(selection_index);

        switch(*selection_index)
        {
        case INPUT_BOX_BUTTON_OK:
          lapb_cfg->t3 = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));
          break;

        case INPUT_BOX_BUTTON_HELP:
          tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, option_not_implemented_yet_help_str);
          goto show_t3_input_box;
        }//switch(*selection_index)
      }
      break;
      
    case T4:
      {
	int t4;
		  		  
show_t4_input_box:
        snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify T4 timer (default 0)");
        snprintf(initial_text, MAX_PATH_LENGTH, "%d", lapb_cfg->t4);

        inb.set_configuration(lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

        inb.show(selection_index);

        switch(*selection_index)
        {
        case INPUT_BOX_BUTTON_OK:
          t4 = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

          if(t4 < 0 || t4 > 240){
            tb.show_error_message(lxdialog_path, WANCONFIG_LAPB, "Invalid T4!\n\n%s\n", 
			     t4_help_str);
            goto show_t4_input_box;
          }else{
            lapb_cfg->t4 = t4;
          }
          break;

        case INPUT_BOX_BUTTON_HELP:
          tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, t4_help_str);
          goto show_t4_input_box;
        }//switch(*selection_index)
      }
      break;

    case WINDOW:
      {
	int window;
		
show_window_input_box:
        snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify Packet Window (default 7)");
        snprintf(initial_text, MAX_PATH_LENGTH, "%d", lapb_cfg->window);

        inb.set_configuration(lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

        inb.show(selection_index);

        switch(*selection_index)
        {
        case INPUT_BOX_BUTTON_OK:
          window = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

          if(window < 1 || window > 7){
            tb.show_error_message(lxdialog_path, WANCONFIG_LAPB, "Invalid Packet Window!\n\n%s\n", 
			     window_help_str);
            goto show_window_input_box;
          }else{
            lapb_cfg->window = window;
          }
          break;

        case INPUT_BOX_BUTTON_HELP:
          tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, window_help_str);
          goto show_window_input_box;
        }//switch(*selection_index)
      }
      break;
      
    case MODE:
      {
show_mode_input_box:
        snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify Mode");
        snprintf(initial_text, MAX_PATH_LENGTH, "%d", lapb_cfg->mode);

        inb.set_configuration(lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

        inb.show(selection_index);

        switch(*selection_index)
        {
        case INPUT_BOX_BUTTON_OK:
          lapb_cfg->mode = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));
          break;

        case INPUT_BOX_BUTTON_HELP:
          tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, option_not_implemented_yet_help_str);
          goto show_mode_input_box;
        }//switch(*selection_index)
      }
      break;

    case MTU:
      {
show_mtu_input_box:
        snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify MTU");
        snprintf(initial_text, MAX_PATH_LENGTH, "%d", lapb_cfg->mtu);

        inb.set_configuration(lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

        inb.show(selection_index);

        switch(*selection_index)
        {
        case INPUT_BOX_BUTTON_OK:
          lapb_cfg->mtu = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));
          break;

        case INPUT_BOX_BUTTON_HELP:
          tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, option_not_implemented_yet_help_str);
          goto show_mtu_input_box;
        }//switch(*selection_index)
      }
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
    }
    break;

  case MENU_BOX_BUTTON_EXIT:
    exit_dialog = YES;
    break;

  case MENU_BOX_BUTTON_HELP:
    switch(atoi(get_lxdialog_output_string()))
    {
    case STATION:
      tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, station_type_help_str);
      break;
    case T1:
      tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, t1_help_str);
      break;
    case T2:
      tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, t2_help_str);
      break;
    case N2:
      tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, n2_help_str);
      break;
    case T3:
      tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, option_not_implemented_yet_help_str);
      break;
    case T4:
      tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, t4_help_str);
      break;
    case WINDOW:
      tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, window_help_str);
      break;
    case MODE:
      tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, option_not_implemented_yet_help_str);
      break;
    case MTU:
      tb.show_help_message(lxdialog_path, WANCONFIG_LAPB, option_not_implemented_yet_help_str);
      break;
    }//switch()
    break;
  }//switch(*selection_index)

  if(exit_dialog == NO){
    goto again;
  }

cleanup:
  return rc;
}

