/***************************************************************************
                          menu_hardware_te3_card_advanced_options  -  description
                             -------------------
    begin                : Thu Apr 1 2004
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

#include "menu_hardware_te3_card_advanced_options.h"
#include "menu_te3_select_media.h"
#include "menu_te_select_line_decoding.h"
#include "menu_te_select_framing.h"
#include "menu_t1_lbo.h"
#include "input_box_active_channels.h"
#include "menu_te1_clock_mode.h"

enum TE3_ADVANCED_OPTIONS{
	TE3_MEDIA=1,
	TE3_LCODE,
	TE3_FRAME,
	TE3_IS_FRACTIONAL,
	TE3_RDEV_TYPE,
	TE3_FCS,
      	LIU_RX_EQUAL,
  	LIU_TAOS,
	LIU_LB_MODE,
	LIU_TX_LBO,
  	AFT_FE_TXTRISTATE
};

#define DBG_MENU_HARDWARE_TE3_CARD_ADVANCED_OPTIONS 1

menu_hardware_te3_card_advanced_options::
	menu_hardware_te3_card_advanced_options(  IN char * lxdialog_path,
                                            IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_HARDWARE_TE3_CARD_ADVANCED_OPTIONS,
    ("menu_hardware_te3_card_advanced_options::menu_hardware_te3_card_advanced_options()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_hardware_te3_card_advanced_options::~menu_hardware_te3_card_advanced_options()
{
  Debug(DBG_MENU_HARDWARE_TE3_CARD_ADVANCED_OPTIONS,
    ("menu_hardware_te3_card_advanced_options::~menu_hardware_te3_card_advanced_options()\n"));
}

int menu_hardware_te3_card_advanced_options::run(OUT int * selection_index)
{
  string menu_str;
  string question;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int option_selected;
  char exit_dialog;
  int number_of_items;
  sdla_te3_cfg_t* te3_cfg;
  sdla_te3_liu_cfg_t*	liu_cfg;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];
  input_box inb;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;

  input_box_active_channels act_channels_ip;

  Debug(DBG_MENU_HARDWARE_TE3_CARD_ADVANCED_OPTIONS, ("menu_net_interface_setup::run()\n"));

again:
  rc = YES;
  option_selected = 0;
  exit_dialog = NO;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  te3_cfg = &linkconf->fe_cfg.cfg.te3_cfg;
  liu_cfg = &te3_cfg->liu_cfg;
  
  Debug(DBG_MENU_HARDWARE_TE3_CARD_ADVANCED_OPTIONS,
    ("cfr->link_defs->name: %s\n", link_def->name));

  number_of_items = 9;
  menu_str = "";

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE3_MEDIA);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Physical Medium--> %s\" ",
    	MEDIA_DECODE(&linkconf->fe_cfg));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE3_LCODE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Line decoding----> %s\" ", 
		  LCODE_DECODE(&linkconf->fe_cfg));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE3_FRAME);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Framing----------> %s\" ", 
		  FRAME_DECODE(&linkconf->fe_cfg));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE3_IS_FRACTIONAL);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"is fractional mode----> %s\" ",
    (te3_cfg->fractional == WANOPT_YES ? "Yes" : "No"));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  if(te3_cfg->fractional == WANOPT_YES){
	  
    number_of_items = 10;
    
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE3_RDEV_TYPE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"rdevice----> %s\" ", 
		      RDEVICE_DECODE(te3_cfg->rdevice_type));
    menu_str += tmp_buff;
  }
 
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE3_FCS);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"FCS---------> %d\" ", te3_cfg->fcs);
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", LIU_RX_EQUAL);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"rx equal----> %s\" ",
    (liu_cfg->rx_equal == WANOPT_YES ? "Yes" : "No"));
  menu_str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", LIU_TAOS);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"taos--------> %s\" ",
    (liu_cfg->taos == WANOPT_YES ? "Yes" : "No"));
  menu_str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", LIU_LB_MODE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"lb mode-----> %s\" ",
    (liu_cfg->lb_mode == WANOPT_YES ? "Yes" : "No"));
  menu_str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", LIU_TX_LBO);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"tx lbo------> %s\" ",
    (liu_cfg->tx_lbo == WANOPT_YES ? "Yes" : "No"));
  menu_str += tmp_buff;
  
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_FE_TXTRISTATE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Disable Transmitter---> %s\" ",
    (linkconf->fe_cfg.tx_tristate_mode == WANOPT_YES ? "YES" : "NO"));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nAdvanced Hardware settings for: %s.", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "T3/E3 ADVANCED CONFIGURATION OPTIONS",
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
    Debug(DBG_MENU_HARDWARE_TE3_CARD_ADVANCED_OPTIONS,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case TE3_MEDIA:
      {
        menu_te3_select_media te3_select_media(lxdialog_path, cfr);
        if(te3_select_media.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case TE3_LCODE:
      {
        menu_te_select_line_decoding te3_select_line_decoding(lxdialog_path, cfr);
        if(te3_select_line_decoding.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case TE3_FRAME:
      {
        menu_te_select_framing te3_select_framing(lxdialog_path, cfr);
        if(te3_select_framing.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case TE3_IS_FRACTIONAL:
      if(te3_cfg->fractional == WANOPT_YES){

	if(yes_no_question( selection_index,
                            lxdialog_path,
                            NO_PROTOCOL_NEEDED,
                            "Do you want to disable Fractional Mode?") == NO){
	  //error running dialog        
          return NO;
	}
        switch(*selection_index)
        {
        case YES_NO_TEXT_BOX_BUTTON_YES:
          te3_cfg->fractional = WANOPT_NO;
          break;

        case YES_NO_TEXT_BOX_BUTTON_NO:
          //do nothing
          break;

        default:
          return NO;
        }
	
      }else{
	      
	if(yes_no_question( selection_index,
                            lxdialog_path,
                            NO_PROTOCOL_NEEDED,
                            "Do you want to enable Fractional Mode?") == NO){
	  //error running dialog        
          return NO;
	}
        switch(*selection_index)
        {
        case YES_NO_TEXT_BOX_BUTTON_YES:
          te3_cfg->fractional = WANOPT_YES;
	  te3_cfg->rdevice_type = WAN_TE3_RDEVICE_KENTROX;
          break;

        case YES_NO_TEXT_BOX_BUTTON_NO:
          //do nothing
          break;

        default:
          return NO;
        }
      }
      break;
      
    case TE3_RDEV_TYPE:

      break;
      
    case TE3_FCS:
      //number between 0 and 7, including.
      int fcs_no;
      snprintf(explanation_text, MAX_PATH_LENGTH, "Enter FCS Number");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", te3_cfg->fcs);

show_fcs_input_box:
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
        Debug(DBG_MENU_HARDWARE_TE3_CARD_ADVANCED_OPTIONS,
          ("fcs_no on return: %s\n", inb.get_lxdialog_output_string()));

        fcs_no = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));
	te3_cfg->fcs = fcs_no;
/*
 	//FIXME: check if input is valid
        if(fcs_no < 0 || fcs_no > 7){

          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
                                "Invalid FCS Number. Min: %d, Max: %d.",
                                0, 7);
          goto show_fcs_input_box;
        }else{
       	  te3_cfg->fcs = fcs_no;
        }
*/
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
        goto show_fcs_input_box;
      }//switch(*selection_index)
      break;
      
    case LIU_RX_EQUAL:
      
      question = (liu_cfg->rx_equal == WANOPT_YES ? 
	"Do you want to disable Rx Equal Mode?" :
       	"Do you want to enable Rx Equal Mode?");

      if(yes_no_question( selection_index,
                          lxdialog_path,
                          NO_PROTOCOL_NEEDED,
                          question.c_str()) == NO){
        //error running dialog        
        return NO;
      }
      
      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
  	if(liu_cfg->rx_equal == WANOPT_YES){
		liu_cfg->rx_equal = WANOPT_NO;
	}else{
		liu_cfg->rx_equal = WANOPT_YES;
	}
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
        //do nothing
        break;

      default:
        return NO;
      }
      break;
      
    case LIU_TAOS:

     question = (liu_cfg->taos == WANOPT_YES ? 
	"Do you want to disable Taos Mode?" :
       	"Do you want to enable Taos Mode?");

      if(yes_no_question( selection_index,
                          lxdialog_path,
                          NO_PROTOCOL_NEEDED,
                          question.c_str()) == NO){
        //error running dialog        
        return NO;
      }
      
      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
  	if(liu_cfg->taos == WANOPT_YES){
		liu_cfg->taos = WANOPT_NO;
	}else{
		liu_cfg->taos = WANOPT_YES;
	}
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
        //do nothing
        break;

      default:
        return NO;
      }
      break;
      
    case LIU_LB_MODE:
      question = (liu_cfg->lb_mode == WANOPT_YES ? 
	"Do you want to disable LB Mode?" :
       	"Do you want to enable LB Mode?");

      if(yes_no_question( selection_index,
                          lxdialog_path,
                          NO_PROTOCOL_NEEDED,
                          question.c_str()) == NO){
        //error running dialog        
        return NO;
      }
      
      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
  	if(liu_cfg->lb_mode == WANOPT_YES){
		liu_cfg->lb_mode = WANOPT_NO;
	}else{
		liu_cfg->lb_mode = WANOPT_YES;
	}
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
        //do nothing
        break;

      default:
        return NO;
      }
      break;
      
    case LIU_TX_LBO:
      question = (liu_cfg->tx_lbo == WANOPT_YES ? 
	"Do you want to disable TX LBO Mode?" :
       	"Do you want to enable TX LBO Mode?");

      if(yes_no_question( selection_index,
                          lxdialog_path,
                          NO_PROTOCOL_NEEDED,
                          question.c_str()) == NO){
        //error running dialog        
        return NO;
      }
      
      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
  	if(liu_cfg->tx_lbo == WANOPT_YES){
		liu_cfg->tx_lbo = WANOPT_NO;
	}else{
		liu_cfg->tx_lbo = WANOPT_YES;
	}
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
        //do nothing
        break;

      default:
        return NO;
      }
      break;

    case AFT_FE_TXTRISTATE:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s transmitter?",
        (linkconf->fe_cfg.tx_tristate_mode == WANOPT_NO ? "Disable" : "Enable"));

      if(yes_no_question( selection_index,
                          lxdialog_path,
                          NO_PROTOCOL_NEEDED,
                          tmp_buff) == NO){
        //error displaying dialog
        rc = NO;
	goto cleanup;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(linkconf->fe_cfg.tx_tristate_mode == WANOPT_NO){
	  //transmitter enabled, user wants to disable
	  linkconf->fe_cfg.tx_tristate_mode = WANOPT_YES;
	}else{
	  linkconf->fe_cfg.tx_tristate_mode = WANOPT_NO;
	}
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
        //don't do anything
	break;
      }
      break;
      
    default:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
    }
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
