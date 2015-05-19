/***************************************************************************
                          menu_hardware_card_type.cpp  -  description
                             -------------------
    begin                : Tue Mar 30 2004
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

#include "input_box.h"
#include "text_box_help.h"
#include "menu_hardware_card_type.h"
#include "menu_select_card_type_manualy.h"
#include "menu_hardware_probe.h"

extern char* hw_probe_help_str;

char* adapter_type_help_str =
"ADAPTER TYPE\n"
"\n"
"Adapter type for Sangoma wanpipe cards.\n"
"\n"
"Options:\n"
"-------\n"
"S508 : This option includes:\n"
"S508FT1:  ISA card with an onboard Fractional T1 CSU/DSU\n"
"S508   :  ISA card with V35/RS232 interface.\n"
"\n"
"S514 : This option includes:\n"
"S514FT1:  PCI card with an onboard Fractional T1 CSU/DSU\n"
"S514-1 :  PCI card with V35/RS232 interface, 1 CPU.\n"
"S514-2 :  PCI card with V35/RS232 interface, 2 CPU.\n"
"\n"
"S514 T1/E1: PCI card with an onboard Fractional T1/E1 CSU/DSU.\n"
"\n"
"S514 56K DDS:   PCI card with an onboard 56K DDS CSU/DSU.\n"
"AFT : This option includes:\n"
"A101: PCI card with T1/E1 interface, 1 CPU.\n"
"A102: PCI card with T1/E1 interface, 2 CPU.\n";


enum CARD_TYPE_OPTIONS {
  CURRENT_CARD_TYPE=1,
  MANUALY_SELECT_CARD_TYPE,
  HW_PROBE,
  EMPTY_LINE
};

#define DBG_MENU_HARDWARE_CARD_TYPE 1

menu_hardware_card_type::menu_hardware_card_type( IN char * lxdialog_path,
                                                  IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_HARDWARE_CARD_TYPE, ("menu_hardware_card_type::menu_hardware_card_type()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_hardware_card_type::~menu_hardware_card_type()
{
  Debug(DBG_MENU_HARDWARE_CARD_TYPE, ("menu_hardware_card_type::~menu_hardware_card_type()\n"));
}

int menu_hardware_card_type::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int option_selected;
  char exit_dialog;
  int number_of_items;
  unsigned int old_card_type;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;
  sdla_fe_cfg_t*  fe_cfg;
  sdla_te_cfg_t*  te_cfg;
  wan_adsl_conf_t* adsl_cfg;

  Debug(DBG_MENU_HARDWARE_CARD_TYPE, ("menu_net_interface_setup::run()\n"));

again:
  number_of_items = 4;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;
  fe_cfg = &linkconf->fe_cfg;
  te_cfg = &fe_cfg->cfg.te_cfg;

  adsl_cfg = &linkconf->u.adsl;

  Debug(DBG_MENU_HARDWARE_CARD_TYPE, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  option_selected = 0;
  exit_dialog = NO;

  menu_str = " ";

  Debug(DBG_MENU_HARDWARE_CARD_TYPE, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  old_card_type = linkconf->card_type;

  ////////////////////////////////////////////////
  //depending on 'current hardware' display it's settings
  //e.g.: CHANNELS is irrelevant for S5141
  //      V.35 is irrelevant for S5147
  if(linkconf->card_type == WANOPT_S51X){

    if(get_S514_card_version_string(link_def->card_version) == NULL){
      ERR_DBG_OUT(("Invalid S514 Card Version!!"));
      rc = NO;
      goto cleanup;
    }
    /////////////////////////////////////////////////
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CURRENT_CARD_TYPE);
    menu_str += tmp_buff;
    
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"[Card Type: %s]\" ",
      get_S514_card_version_string(link_def->card_version));
    menu_str += tmp_buff;

  }else if( linkconf->card_type == WANOPT_AFT ||
            linkconf->card_type == WANOPT_ADSL){

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CURRENT_CARD_TYPE);
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"[Card Type: %s]\" ",
      get_card_type_string(linkconf->card_type, link_def->card_version));
    menu_str += tmp_buff;
      
  }else if(linkconf->card_type == WANOPT_S50X){

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CURRENT_CARD_TYPE);
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"[Card Type: %s]\" ",
      get_card_type_string(linkconf->card_type, link_def->card_version));
    menu_str += tmp_buff;

  }else if(linkconf->card_type == NOT_SET){
/*
    //for creating new conf file from scratch
    Debug(DBG_MENU_HARDWARE_CARD_TYPE, ("Card Type NOT_SET\n"));
    rc = YES;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CURRENT_CARD_TYPE);
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"[Card Type: %s]\" ",
      get_card_type_string(linkconf->card_type, link_def->card_version));
    menu_str += tmp_buff;
*/
  }

  if(linkconf->card_type != NOT_SET){
    /////////////////////////////////////////////////
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
    menu_str += tmp_buff;
  }
  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", HW_PROBE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Select from detected cards list\" ");
  menu_str += tmp_buff;


  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", MANUALY_SELECT_CARD_TYPE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Select from a list of valid Card Types\" ");
  menu_str += tmp_buff;
  ////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect card type for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT CARD TYPE",
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

  Debug(DBG_MENU_HARDWARE_CARD_TYPE, ("*selection_index: %d\n", *selection_index));
  
  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_HARDWARE_CARD_TYPE,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case CURRENT_CARD_TYPE:
    case EMPTY_LINE:
      //do nothing
      break;
      
    case MANUALY_SELECT_CARD_TYPE:
      {
        menu_select_card_type_manualy select_card_type_manualy(lxdialog_path, cfr);
        if(select_card_type_manualy.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
          break;
        }
      }
      Debug(DBG_MENU_HARDWARE_CARD_TYPE, ("1. old_card_type: 0x%X, linkconf->card_type: 0x%X\n",
        old_card_type, linkconf->card_type));

      Debug(DBG_MENU_HARDWARE_CARD_TYPE, ("link_def->card_version: 0x%X\n",
        link_def->card_version));
      
      if(old_card_type != linkconf->card_type){
        linkconf->config_id = PROTOCOL_NOT_SET;

	switch(linkconf->card_type)
	{
	case WANOPT_S51X:
	case WANOPT_S50X:
	  switch(link_def->card_version)
	  {
	  case S5145_ADPTR_1_CPU_56K: 
	    //if 56K card, set media to 56K
	    fe_cfg->media = WAN_MEDIA_56K;
	    break;

	  case S5144_ADPTR_1_CPU_T1E1:
	    set_default_t1_configuration(fe_cfg);
	    break;
	  }
	  break;

	case WANOPT_AFT:
	  switch(link_def->card_version)
	  {
	  case A101_ADPTR_1TE1://WAN_MEDIA_T1:
	  case A104_ADPTR_4TE1://WAN_MEDIA_T1:
	    set_default_t1_configuration(fe_cfg);
	    break;

	  case A300_ADPTR_U_1TE3://WAN_MEDIA_DS3:
	    set_default_t3_configuration(fe_cfg);
	    break;

      case A200_ADPTR_ANALOG:
	    fe_cfg->media = WAN_MEDIA_FXOFXS;
	    fe_cfg->tdmv_law = WAN_TDMV_MULAW;
	    snprintf(fe_cfg->cfg.remora.opermode_name, WAN_RM_OPERMODE_LEN, "%s", "FCC");
		fe_cfg->cfg.remora.battthresh = 3;
		fe_cfg->cfg.remora.battdebounce = 16;
	    break;

	  case AFT_ADPTR_56K:
	    fe_cfg->media = WAN_MEDIA_56K;
	    break;

	  case AFT_ADPTR_ISDN:
	    fe_cfg->media = WAN_MEDIA_BRI;
	    fe_cfg->line_no = 1;//for manual card type selection assume line 1
	    fe_cfg->tdmv_law = WAN_TDMV_ALAW;
	    break;

	  case AFT_ADPTR_2SERIAL_V35X21:
	    fe_cfg->media = WAN_MEDIA_SERIAL;
			break;
	  }
	  break;
	  
	case WANOPT_ADSL:
	  set_default_adsl_configuration(adsl_cfg);
	  break;
	  
	default:
	  ERR_DBG_OUT(("Invalid 'card_type' (%d) selected!\n", linkconf->card_type));
	}
      }
      //???
      exit_dialog = YES;
      rc = YES;
      break;

    case HW_PROBE:
      {
        menu_hardware_probe hardware_probe(lxdialog_path, cfr);
        if(hardware_probe.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
          break;
        }
      }
      Debug(DBG_MENU_HARDWARE_CARD_TYPE, ("2. old_card_type: 0x%X, linkconf->card_type: 0x%X\n",
        old_card_type, linkconf->card_type));

      if(old_card_type != linkconf->card_type){
        linkconf->config_id = PROTOCOL_NOT_SET;
	
	switch(linkconf->card_type)
	{
	case WANOPT_S51X:
	case WANOPT_S50X:
	  switch(link_def->card_version)
	  {
	  case S5145_ADPTR_1_CPU_56K: 
	  //if 56K card, set media to 56K
	  fe_cfg->media = WAN_MEDIA_56K;
	  break;

	  case S5144_ADPTR_1_CPU_T1E1:
	    set_default_t1_configuration(fe_cfg);
	    break;
	  }
	  break;

	case WANOPT_AFT:
	  switch(link_def->card_version)
	  {
	  case A101_ADPTR_1TE1://WAN_MEDIA_T1:
	  case A104_ADPTR_4TE1://WAN_MEDIA_T1:

	    set_default_t1_configuration(fe_cfg);
	    break;

	  case A300_ADPTR_U_1TE3://WAN_MEDIA_DS3:
	    set_default_t3_configuration(fe_cfg);
	    break;

      case A200_ADPTR_ANALOG:
	    fe_cfg->media = WAN_MEDIA_FXOFXS;
	    fe_cfg->tdmv_law = WAN_TDMV_MULAW;
	    snprintf(fe_cfg->cfg.remora.opermode_name, WAN_RM_OPERMODE_LEN, "%s", "FCC");
	    fe_cfg->cfg.remora.battthresh = 3;
	    fe_cfg->cfg.remora.battdebounce = 16;
	    break;

	  case AFT_ADPTR_56K:
	    fe_cfg->media = WAN_MEDIA_56K;
	    break;

	  case AFT_ADPTR_ISDN:
	    fe_cfg->media = WAN_MEDIA_BRI;
	    fe_cfg->tdmv_law = WAN_TDMV_ALAW;
	    break;

	  case AFT_ADPTR_2SERIAL_V35X21:
	    fe_cfg->media = WAN_MEDIA_SERIAL;
			break;
	  }
	  break;

	case WANOPT_ADSL:
	  set_default_adsl_configuration(adsl_cfg);
	  break;	  

	default:
	  ERR_DBG_OUT(("Invalid 'card_type' (%d) selected!\n", linkconf->card_type));
	}
      }//switch()
      
      exit_dialog = YES;
      rc = YES;
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
    case MANUALY_SELECT_CARD_TYPE:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, adapter_type_help_str);
      break;

    case HW_PROBE:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, hw_probe_help_str);
      break;

    case CURRENT_CARD_TYPE:
    case EMPTY_LINE:
      ;//do nothing
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
    }
    break;

  case MENU_BOX_BUTTON_BACK:
    Debug(DBG_MENU_HARDWARE_CARD_TYPE, ("MENU_BOX_BUTTON_BACK selected\n"));
    rc = YES;
    exit_dialog = YES;
    
    break;
  }//switch(*selection_index)

  if(exit_dialog == NO){
    goto again;
  }

cleanup:
  return rc;
}
