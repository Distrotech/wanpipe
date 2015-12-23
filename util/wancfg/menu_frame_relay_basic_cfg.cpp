/***************************************************************************
                          menu_frame_relay_basic_cfg.cpp  -  description
                             -------------------
    begin                : Fri Mar 12 2004
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
#include "text_box.h"

#include "menu_frame_relay_basic_cfg.h"
#include "menu_frame_relay_signalling.h"
#include "menu_frame_relay_manual_or_auto_dlci_cfg.h"
#include "menu_frame_relay_dlci_list.h"
#include "menu_frame_relay_advanced_global_configuration.h"

#define DBG_MENU_AFT_LOGICAL_CHANNELS_LIST  1

char* fr_signalling_help_str =
"Frame Relay: SIGNALLING\n"
"\n"
"This parameter specifies frame relay link management\n"
"type.  Available options are:\n"
"\n"
"ANSI    ANSI T1.617 Annex D (default)\n"
"Q933    ITU Q.933A\n"
"LMI     LMI\n"
"No      Turns off polling, thus DLCI becomes active\n"
"        regardless of the remote end.\n"
"        (This option is for NODE configuration only)\n"
"\n"
"Default: ANSI\n";

char* fr_station_type_help_str =
"Frame Relay: STATION\n"
"\n"
"This parameter specifies whether the  adapter\n"
"should operate as a Customer  Premises Equipment (CPE)\n"
"or emulate a  frame relay switch (Access Node).\n"
"\n"
"Available options are:\n"
"      CPE     CPE mode (default)\n"
"      Node    Access Node (switch emulation mode)\n"
"\n"
"Default: CPE\n";

char* fr_dlci_numbering_help_str =
"DLCI Configuration Mode\n"
"-------------------------\n"
"\n"
"Auto - active DLCIs detected from the Full Status Report.\n"
"       This option valid only for CPE.\n"
"\n"
"Manual - user must provide total number of DLCIs and\n"
"         number for each DLCI.\n";

char* fr_advanced_cfg_options_help_str =
"Advanced Frame Relay Configuration\n"
"----------------------------------\n"
" -Select Station Type (CPE/Node)\n"
" -Set T391 timer\n"
" -Set T392 timer\n"
" -Set N391 timer\n"
" -Set N392 timer\n"
" -Set N393 timer\n"
" -Set Fast Connect Option\n";

char* fr_dlci_list_help_str =
"Dispay list of configured DLCIs.\n"
"Allows set per-DLCI parameters.";


enum FR_BASIC_CFG_OPTIONS{
  FR_SIGNALLING=1,
  DLCI_NUMBERING,
  DLCI_LIST,
  ADVANCED_GLOBAL_FR_CFG,
  EMPTY_LINE
};

char * get_fr_signalling_str(unsigned signalling);

menu_frame_relay_basic_cfg::
		menu_frame_relay_basic_cfg( IN char * lxdialog_path,
					    IN list_element_chan_def* parent_element_logical_ch    )
{
  Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST,
    ("menu_frame_relay_basic_cfg::menu_frame_relay_basic_cfg()\n"));

  Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST,
    ("parent_element_logical_ch->data.name: %s\n", parent_element_logical_ch->data.name));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);

  this->parent_list_element_logical_ch = parent_element_logical_ch;

  name_of_parent_layer = parent_element_logical_ch->data.name;
}

menu_frame_relay_basic_cfg::~menu_frame_relay_basic_cfg()
{
  Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST,
    ("menu_frame_relay_basic_cfg::~menu_frame_relay_basic_cfg()\n"));
}

char * get_fr_signalling_str(unsigned signalling)
{
  switch(signalling)
  {
  case WANOPT_FR_AUTO_SIG:
    return "Auto Detect";
    
  case WANOPT_FR_ANSI:
    return "ANSI";

  case WANOPT_FR_Q933:
    return "Q933";

  case WANOPT_FR_LMI:
    return "LMI";

  default:
    Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, ("invalid signallig: %d\n", signalling));
    return "Invalid Signalling";
  }
}

int menu_frame_relay_basic_cfg::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  list_element_chan_def* tmp_dlci_ptr = NULL;
  objects_list* obj_list;
  int old_dlci_numbering_mode;
  int new_dlci_numbering_mode;
 
  wan_fr_conf_t fr_cfg_of_first;

  //help text box
  text_box tb;
  

  Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, ("menu_new_device_configuration::run()\n"));

again:
  rc = YES;
  exit_dialog = NO;
  menu_str = " ";

  obj_list = (objects_list*)parent_list_element_logical_ch->next_objects_list;
  if(obj_list == NULL){
    ERR_DBG_OUT(("Invalid 'obj_list' pointer!!\n"));
    return NO;
  }
  
  tmp_dlci_ptr = (list_element_chan_def*)obj_list->get_first();
  if(tmp_dlci_ptr == NULL){
    ERR_DBG_OUT(("Invalid 'dlci' pointer!!\n"));
    return NO;
  }
  // frame relay configuration - in 'profile' section. has to be THE SAME for all
  // dlcis belonging to this frame relay device, so we can use profile of the 1-st dlci in the list.
  fr = &tmp_dlci_ptr->data.chanconf->u.fr;
  memcpy(&fr_cfg_of_first, fr, sizeof(wan_fr_conf_t));
  
  Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, ("number_of_dlcis: %d\n", obj_list->get_size()));
  Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, ("fr->dlci_num: %d\n", fr->dlci_num));
  
  if(fr->dlci_num == 1){
	  
    //only one DLCI - there is possibility of 'auto dlci'  
    Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, ("tmp_dlci_ptr->data.name: %s\n",
		  tmp_dlci_ptr->data.name));

    //check 'addr' is '1' - a must for 'auto'
    if(strcmp(tmp_dlci_ptr->data.addr, "1") != 0){
      Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, ("address: %s - NOT 'auto dlci'.\n",
			     tmp_dlci_ptr->data.addr));
      
      old_dlci_numbering_mode = MANUAL_DLCI;

    }else{
      //auto dlci
      old_dlci_numbering_mode = AUTO_DLCI;
    }
  }else{
    //more than 1 dlci has to be manual
    old_dlci_numbering_mode = MANUAL_DLCI;
  }
  
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", FR_SIGNALLING);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Signalling--------> %s\" ",
    get_fr_signalling_str(fr->signalling));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", DLCI_NUMBERING);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"DLCI Numbering----> %s\" ",
    (old_dlci_numbering_mode == MANUAL_DLCI ? "Manual" : "Auto (Single DLCI)"));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", DLCI_LIST);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"DLCI Configuration\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ADVANCED_GLOBAL_FR_CFG);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Advanced Frame Relay Configuration\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\n      Frame Relay configuration options for\
\nWan Device: %s", name_of_parent_layer);
//list_element_logical_ch->data.name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "FRAME RELAY CONFIGURATION",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          5,//number of items in the menu, including the empty lines
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
    Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case FR_SIGNALLING:
      {
	menu_frame_relay_signalling fr_signalling(lxdialog_path, fr);
	fr_signalling.run(selection_index);
      }
      break;
    
    case DLCI_NUMBERING:
      {
	menu_frame_relay_manual_or_auto_dlci_cfg
	       dlci_cfg_mode( lxdialog_path, tmp_dlci_ptr->data.chanconf);
      
	if(dlci_cfg_mode.run(selection_index, &old_dlci_numbering_mode, &new_dlci_numbering_mode) == NO){
	  rc = NO;
	  goto cleanup;
	}

	Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, ("old_dlci_numbering_mode: %d, new_dlci_numbering_mode: %d\n",
	  old_dlci_numbering_mode, new_dlci_numbering_mode));

	Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, ("fr->dlci_num: %d\n", fr->dlci_num));

	if(old_dlci_numbering_mode != new_dlci_numbering_mode){

	  fr_config_info_t fr_config_info;
	  fr_config_info.name_of_parent_layer = name_of_parent_layer;
	    
	  if(new_dlci_numbering_mode == AUTO_DLCI){
	    fr_config_info.auto_dlci = YES;
	  }else{
	    //was auto, changed to manual
            fr_config_info.auto_dlci = NO;
	  }
      	  //remove all
	  adjust_number_of_logical_channels_in_list(WANCONFIG_MFR, obj_list, &fr_config_info, 0);
	  //and add one only!!
	  adjust_number_of_logical_channels_in_list(WANCONFIG_MFR, obj_list, &fr_config_info, 1);

	  Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, ("restoring original global configuration...\n"));
	
	  obj_list = (objects_list*)parent_list_element_logical_ch->next_objects_list;
  	  if(obj_list == NULL){
    		ERR_DBG_OUT(("Invalid 'obj_list' pointer!!\n"));
    		return NO;
  	  }
  
  	  tmp_dlci_ptr = (list_element_chan_def*)obj_list->get_first();
  	  if(tmp_dlci_ptr == NULL){
    		ERR_DBG_OUT(("Invalid 'dlci' pointer!!\n"));
    		return NO;
  	  }
  	  // frame relay configuration - in 'profile' section. has to be THE SAME for all
  	  // dlcis belonging to this frame relay device, so we can use profile of the 1-st dlci in the list.
  	  fr = &tmp_dlci_ptr->data.chanconf->u.fr;
  
	  //important to keep the global configuration untouched!
#if 1
	  fr->signalling = fr_cfg_of_first.signalling;
	  fr->t391 = fr_cfg_of_first.t391;		/* link integrity verification timer */
	  fr->t392 = fr_cfg_of_first.t392;		/* polling verification timer */
	  fr->n391 = fr_cfg_of_first.n391;		/* full status polling cycle counter */
	  fr->n392 = fr_cfg_of_first.n392;		/* error threshold counter */
	  fr->n393 = fr_cfg_of_first.n393;		/* monitored events counter */
	  //unsigned dlci_num;	/* number of DLCs (access node) */
	  //unsigned dlci[100];     /* List of all DLCIs */
	  fr->issue_fs_on_startup = fr_cfg_of_first.issue_fs_on_startup;
	  //unsigned char station;  /* Node or CPE */
#endif
          Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, ("2. address: %s.\n", tmp_dlci_ptr->data.addr));
	}//if()
      }
      break;
      
    case DLCI_LIST://dlci list to get to per-dlci configuration
      {
	menu_frame_relay_dlci_list dlci_lst(lxdialog_path,
					    parent_list_element_logical_ch);
	dlci_lst.run(selection_index);
      }
      break;

    case ADVANCED_GLOBAL_FR_CFG:
      {
        int old_station = fr->station;
	
	menu_frame_relay_advanced_global_configuration fr_global_cfg(lxdialog_path, parent_list_element_logical_ch);
	fr_global_cfg.run(selection_index);

	if(old_station != fr->station && fr->station == WANOPT_NODE){
          //user switched from CPE to NODE
          if(old_dlci_numbering_mode == AUTO_DLCI){
            //force user to change to manual dlci numbering
            tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
"Please change 'DLCI Numbering' to 'Manual'\n\
before configuring station as Access Node!\n\
Reseting automatically back to CPE!");
	    fr->station = WANOPT_CPE;
	  }
	}
      }
      break;

    case EMPTY_LINE:
      //do nothing
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
    case FR_SIGNALLING:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, fr_signalling_help_str);
      break;

    case DLCI_NUMBERING:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, fr_dlci_numbering_help_str);
      break;

    case ADVANCED_GLOBAL_FR_CFG:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, fr_advanced_cfg_options_help_str);
      break;

    case DLCI_LIST:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, fr_dlci_list_help_str);
      break;

    default:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
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

