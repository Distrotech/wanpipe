/***************************************************************************
                          menu_wan_channel_cfg.cpp  -  description
                             -------------------
    begin                : Mon December  6 2004
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
#include "input_box_active_channels.h"

#include "menu_wan_channel_cfg.h"
#include "menu_select_protocol.h"

#include "menu_frame_relay_basic_cfg.h"
#include "menu_ppp_basic_cfg.h"
#include "menu_chdlc_basic_cfg.h"
#include "menu_lapb_basic_cfg.h"

#if defined(CONFIG_PRODUCT_WANPIPE_LIP_ATM)
#include "menu_atm_basic_cfg.h"
#endif

#include "menu_net_interfaces_list.h"

#define DBG_MENU_WAN_CHANNEL_CFG 1

char* aft_logical_channel_protocol_select_help_str =
"Protocol Selection\n"
"------------------\n"
"\n"
"Please select communications protocol.\n";

enum AFT_LOGICAL_CHANNEL_CONFIGURATION{

  CURRENT_PROTOCOL,
  //Protocol  --------------> MP_PPP
  CONFIGURE_CURRENT_PROTOCOL,
  SELECT_NEW_PROTOCOL,

  NET_IF_SETUP,
  EMPTY_LINE
};

menu_wan_channel_cfg::menu_wan_channel_cfg( IN char * lxdialog_path,
					    IN conf_file_reader* cfr,
					    IN list_element_chan_def* list_element_logical_ch)
{
  Debug(DBG_MENU_WAN_CHANNEL_CFG,
    ("menu_wan_channel_cfg::menu_wan_channel_cfg()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);

  this->cfr = cfr;
  this->obj_list = cfr->main_obj_list;
  this->list_element_logical_ch = list_element_logical_ch;
}

menu_wan_channel_cfg::~menu_wan_channel_cfg()
{
  Debug(DBG_MENU_WAN_CHANNEL_CFG,
    ("menu_wan_channel_cfg::~menu_wan_channel_cfg()\n"));
}

int menu_wan_channel_cfg::run(IN int wanpipe_number,
			      OUT int * selection_index,
                              IN int number_of_timeslot_groups)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  int num_of_visible_items = 1;
  //help text box
  text_box tb;
  int old_protocol, new_protocol;
  int exit_dialog;
  link_def_t*		  link_def;
  wandev_conf_t*	  linkconf; 
  
  list_element_chan_def* next_level_list_el_chan_def;
  objects_list* next_level_obj_list;
  chan_def_t *chandef, *chandef_1st_level;
  
  input_box input_bx;
  char backtitle[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility");

  Debug(DBG_MENU_WAN_CHANNEL_CFG, ("menu_wan_channel_cfg::run()\n"));

  input_box_active_channels act_channels_ip;

again:
  rc = YES;
  menu_str = " ";
  exit_dialog = NO;
  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;
  
  if(list_element_logical_ch ==  NULL){
    ERR_DBG_OUT(("%s configuration pointer is NULL (list_element_logical_ch) !!!\n",
      TIME_SLOT_GROUP_STR));
    rc = NO;
    goto cleanup;
  }
  
  chandef_1st_level = &list_element_logical_ch->data;

#if DBG_MENU_WAN_CHANNEL_CFG
  chandef = &list_element_logical_ch->data;
  Debug(DBG_MENU_WAN_CHANNEL_CFG, ("1-st level: chan_def->usedby: %d\n", chandef->usedby));
  Debug(DBG_MENU_WAN_CHANNEL_CFG, ("1-st level: chandef->chanconf->config_id: %d\n",
			 chandef->chanconf->config_id));
#endif

  next_level_obj_list = (objects_list*)list_element_logical_ch->next_objects_list;
  
  if(next_level_obj_list != NULL){

    Debug(DBG_MENU_WAN_CHANNEL_CFG, ("next_level_obj_list != NULL\n"));
			    
    next_level_list_el_chan_def = (list_element_chan_def*)next_level_obj_list->get_first();
    chandef = &next_level_list_el_chan_def->data;
  }else{
    Debug(DBG_MENU_WAN_CHANNEL_CFG, ("next_level_obj_list == NULL\n"));
    
    chandef = &list_element_logical_ch->data;
  }

  /////////////////////////////////////////////////////////////////////////////////////
  Debug(DBG_MENU_WAN_CHANNEL_CFG, ("chandef->chanconf->config_id: %d\n",
			chandef->chanconf->config_id));
  Debug(DBG_MENU_WAN_CHANNEL_CFG, ("chan_def->usedby: %d\n",
			chandef->usedby));

  if(chandef->chanconf->config_id != PROTOCOL_NOT_SET ){
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CURRENT_PROTOCOL);
    menu_str += tmp_buff;

    if(chandef->chanconf->config_id == WANCONFIG_AFT ||
       chandef->chanconf->config_id == WANCONFIG_AFT_TE3){

      //if(chandef->chanconf->hdlc_streaming == WANOPT_NO){
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"[Selected Protocol : %s]\" ",
        "HDLC Streaming");
      //}

    }else{
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"[Selected Protocol : %s]\" ",
        get_protocol_string(chandef->chanconf->config_id));
    }
    menu_str += tmp_buff;
	    
  }else{
    //go directly to 'change protocol'
    goto select_new_protocol;
  }
 
  /////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
  menu_str += tmp_buff;
     
  /////////////////////////////////////////////////////////////////////////////////
  num_of_visible_items = 4;

  switch(chandef->chanconf->config_id)
  {
  case WANCONFIG_ADSL:
    if(link_def->sub_config_id == WANCONFIG_MPPP){
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CONFIGURE_CURRENT_PROTOCOL);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Configure Selected Protocol\" ");
      menu_str += tmp_buff;
    }else{
      //do nothing - there is nothing to configure.
    }
    break;
    
  case WANCONFIG_EDUKIT:
  case WANCONFIG_TTY:
  case WANCONFIG_HDLC:
  case PROTOCOL_TDM_VOICE:
  case PROTOCOL_TDM_VOICE_API:
  case WANCONFIG_AFT:
    //do nothing - there is nothing to configure.
    break;
	  
  default:  
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CONFIGURE_CURRENT_PROTOCOL);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Configure Selected Protocol\" ");
    menu_str += tmp_buff;
  }

  /////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", SELECT_NEW_PROTOCOL);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Select New Protocol\" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////////////////////////////////////
  //NOTE!! display 'interface configuration' option only if
  //number of channels in the 'cfr->main_obj_list' is greater than one.
  //because otherwize it is repetition of what can be done in 'menu_new_device_configuration'
  if(obj_list->get_size() > 1){
    switch(chandef->chanconf->config_id)
    {
    //case WANCONFIG_EDUKIT:
    //case WANCONFIG_HDLC:
    //case PROTOCOL_TDM_VOICE:
    //case PROTOCOL_TDM_VOICE_API:
      //no interface setup needed
      //break;
	  
    default:  
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", NET_IF_SETUP);
      menu_str += tmp_buff;

      if(next_level_obj_list != NULL){
	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface Setup------------> %d Defined\" ",
	  next_level_obj_list->get_size());

	num_of_visible_items += next_level_obj_list->get_size();
      }else{
	//if no list above this interface, 
	//only one is defined - the current one (list_element_logical_ch)
	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface Setup------------> %d Defined\" ",
	  1);

	num_of_visible_items += 1;
      }
      menu_str += tmp_buff;
    }
  } 

  /////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\
\nSet cofiguration of %s.", list_element_logical_ch->data.name);
//\nSet cofiguration of %s %s.", TIME_SLOT_GROUP_STR, list_element_logical_ch->data.addr);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "WAN CHANNEL CONFIGURATION",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          num_of_visible_items,
                          (char*)menu_str.c_str()
                          ) == NO){
    rc = NO;
    goto cleanup;
  }

  if(show(selection_index) == NO){
    rc = NO;
    goto cleanup;
  }

  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_WAN_CHANNEL_CFG, ("option selected for editing: %s\n",
      get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case CURRENT_PROTOCOL:
    case EMPTY_LINE:
      //do nothing
      break;
      
    case CONFIGURE_CURRENT_PROTOCOL:
      //display configuration of the protocol.
      if(display_protocol_cfg(list_element_logical_ch) == NO){
          rc = NO;
      }else{
	  //pointers may change, go re-initialize them
          goto again;
      }
      break;
        
    case SELECT_NEW_PROTOCOL:
select_new_protocol:
      {
        /////////////////////////////////////////////////////////////////////////////////
        if(chandef->chanconf->config_id != PROTOCOL_NOT_SET){
          //ask if user *really* want to change the protocol.
          if(yes_no_question(   selection_index,
                                lxdialog_path,
                                NO_PROTOCOL_NEEDED,
                                "Are you sure you want to change the Protocol?") == NO){
            //error displaying dialog
            return NO;
          }
          switch(*selection_index)
          {
          case YES_NO_TEXT_BOX_BUTTON_YES:
	    //go ahead from the user
            break;
          case YES_NO_TEXT_BOX_BUTTON_NO:
            //user does not want to change the protocol.
            goto again;
          }
        }
        //////////////////////////////////////////////////////////////////////////////////
        old_protocol = chandef->chanconf->config_id;
      
      	menu_select_protocol select_protocol(lxdialog_path, cfr);
	select_protocol.run(&new_protocol);
	
	Debug(DBG_MENU_WAN_CHANNEL_CFG, ("2. old_protocol: %d, new_protocol: %d\n",
							  old_protocol, new_protocol));

	if(new_protocol == 0){
	  //no protocol was selected, just redisplay
  	  goto again;
	}
	
	if(old_protocol == new_protocol){
	  //no change, just redisplay
	  goto again;
	}

	//set 'sub_config_id' before calling handle_protocol_change() !!!
	link_def->sub_config_id = new_protocol;
	chandef_1st_level->chanconf->protocol = PROTOCOL_NOT_SET;// WANCONFIG_LIP_ATM

	if(handle_protocol_change(new_protocol, list_element_logical_ch) == NO){
	  goto cleanup;
	}

	//after protocol change special updates at the 'link' level:
	switch(linkconf->card_type)
	{
	case WANOPT_S50X:
	case WANOPT_S51X:
	  switch(new_protocol)
	  {  
	  //these are in LIP layer
	  case WANCONFIG_MFR:
	  case WANCONFIG_MPPP:
	  case WANCONFIG_MPCHDLC:
	  case WANCONFIG_TTY:
	  case WANCONFIG_LAPB:
	    linkconf->config_id = WANCONFIG_MPROT;
	    break;
	    
	  case WANCONFIG_LIP_ATM:
	    linkconf->config_id = WANCONFIG_LIP_ATM;
	    chandef_1st_level->chanconf->protocol = WANCONFIG_LIP_ATM;
            chandef_1st_level->chanconf->u.aft.data_mux = WANOPT_YES;
            chandef_1st_level->chanconf->hdlc_streaming = WANOPT_NO;
	    is_there_a_lip_atm_if = YES;
	    break;

	  //these are in firmware
	  case WANCONFIG_EDUKIT:
	    linkconf->config_id = WANCONFIG_EDUKIT;
	    break;
	    
	  case WANCONFIG_HDLC:
	    linkconf->config_id = WANCONFIG_HDLC;
	    break;
	    
	  //PROTOCOL_TDM_VOICE - not supported on S-cards, only AFT
	  default:
	    ERR_DBG_OUT(("Unsupprted new protocol (%d)!\n", new_protocol))
	    return NO;
	  }
	  break;				
				  
	case WANOPT_AFT:
	  switch(new_protocol)
	  {  
	  //these are in LIP layer
	  case WANCONFIG_MFR:
	  case WANCONFIG_MPPP:
	  case WANCONFIG_MPCHDLC:
	  case WANCONFIG_TTY:
	  case WANCONFIG_LAPB:
	  case WANCONFIG_HDLC:	    //exception
	  case PROTOCOL_TDM_VOICE:  //exception
	  case PROTOCOL_TDM_VOICE_API:  //exception
	    linkconf->config_id = WANCONFIG_AFT;
	    break;

          case WANCONFIG_LIP_ATM:
	    linkconf->config_id = WANCONFIG_AFT;
	    chandef_1st_level->chanconf->protocol = WANCONFIG_LIP_ATM;
            chandef_1st_level->chanconf->u.aft.data_mux = WANOPT_YES;
            chandef_1st_level->chanconf->hdlc_streaming = WANOPT_NO;
	    is_there_a_lip_atm_if = YES;
            break;

	  default:
	    ERR_DBG_OUT(("Unsupprted new protocol (%d)!\n", new_protocol))
	    return NO;
	  }
	  break;
	  
	case WANOPT_ADSL:
	  //the same logic as for S514
	  switch(new_protocol)
	  {  
	  case WANCONFIG_TTY:
	  case WANCONFIG_ADSL:
	  case WANCONFIG_MPPP:
          case PROTOCOL_IP:
          case PROTOCOL_ETHERNET:
 
	    linkconf->config_id = WANCONFIG_ADSL;
	    break;

	  default:
	    ERR_DBG_OUT(("Unsupprted new protocol (%d)!\n", new_protocol))
	    return NO;
	  }
	  break;
	  
	default:
	  ERR_DBG_OUT(("Invalid card type 0x%X!!\n", linkconf->card_type));
	  goto cleanup;
	}
      }
      break;
    
    case NET_IF_SETUP:
      {	//display list of network interfaces.
        menu_net_interfaces_list net_interfaces_list(lxdialog_path, list_element_logical_ch);
        if(net_interfaces_list.run(selection_index) == NO){
          rc = NO;
        }
      }
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    {
      Debug(DBG_MENU_WAN_CHANNEL_CFG,
        ("HELP option selected: %s\n", get_lxdialog_output_string()));

      switch(atoi(get_lxdialog_output_string()))
      {
      case CURRENT_PROTOCOL:
      case EMPTY_LINE:
        //do nothing
        break;

      case CONFIGURE_CURRENT_PROTOCOL:
        tb.show_help_message(lxdialog_path, WANCONFIG_AFT,
          "Set Configuration of Currently selected protocol.");
        break;

      case SELECT_NEW_PROTOCOL:
        tb.show_help_message(lxdialog_path, WANCONFIG_AFT,
          aft_logical_channel_protocol_select_help_str);
        break;

      default:
        tb.show_help_message(lxdialog_path, WANCONFIG_AFT, option_not_implemented_yet_help_str);
        break;
      }
      goto again;
    }
    break;

  case MENU_BOX_BUTTON_EXIT:
    //no check is done here because each menu item checks
    //the input separatly.
    rc = YES;
    exit_dialog = YES;
    break;
  }//switch(*selection_index)

  if(exit_dialog == NO){
    goto again;
  }
  
cleanup:
  Debug(DBG_MENU_WAN_CHANNEL_CFG, ("menu_wan_channel_cfg::run(): rc: %d\n", rc));
  return rc;
}

int menu_wan_channel_cfg::display_protocol_cfg(IN list_element_chan_def* list_element_logical_ch)
{
  int rc = YES;
  int selection_index;
  list_element_chan_def* next_level_list_el_chan_def;
  objects_list* next_level_obj_list;
  chan_def_t* chandef;
  text_box tb;

  link_def_t* link_defs = cfr->link_defs;
  wandev_conf_t* linkconf = link_defs->linkconf;
  
  Debug(DBG_MENU_WAN_CHANNEL_CFG, ("display_protocol_cfg()\n"));

  if(linkconf->card_type == WANOPT_ADSL && link_defs->sub_config_id == WANCONFIG_MPPP){
    goto ppp_cfg;
  }

  next_level_obj_list = (objects_list*)list_element_logical_ch->next_objects_list;
  if(next_level_obj_list != NULL){

    Debug(DBG_MENU_WAN_CHANNEL_CFG, ("next_level_obj_list != NULL\n"));
			    
    next_level_list_el_chan_def = (list_element_chan_def*)next_level_obj_list->get_first();
    chandef = &next_level_list_el_chan_def->data;
  }else{
    //there must be a LIP layer above for this function to work
    ERR_DBG_OUT(("Invalid 'next_level_obj_list' pointer!\n"));
    return NO;    
  }

  Debug(DBG_MENU_WAN_CHANNEL_CFG, ("chandef->chanconf->config_id: %d\n",
			 chandef->chanconf->config_id));
  Debug(DBG_MENU_WAN_CHANNEL_CFG, ("chan_def->usedby: %d\n",
			  chandef->usedby));

  switch(chandef->chanconf->config_id)
  {
  case WANCONFIG_MFR:
    {
      menu_frame_relay_basic_cfg fr_basic_cfg(lxdialog_path, list_element_logical_ch);
      //pass parent device, so paren device name will be accessible for clarity of display
      fr_basic_cfg.run(&selection_index);
    }
    break;

  case WANCONFIG_MPPP://and WANCONFIG_MPROT too
ppp_cfg:
    {
      menu_ppp_basic_cfg ppp_basic_cfg(lxdialog_path, list_element_logical_ch);
      ppp_basic_cfg.run(&selection_index);
    }
    break;

  case WANCONFIG_MPCHDLC:
    {
      menu_chdlc_basic_cfg chdlc_basic_cfg(lxdialog_path, list_element_logical_ch);
      chdlc_basic_cfg.run(&selection_index);
    }
    break;

  case WANCONFIG_TTY:
  case WANCONFIG_ADSL:
    tb.show_help_message( lxdialog_path, WANCONFIG_AFT, 
			  no_configuration_onptions_available_help_str);
    break;
    
  case WANCONFIG_LAPB:
    {
      menu_lapb_basic_cfg lapb_basic_cfg(lxdialog_path, list_element_logical_ch);
      lapb_basic_cfg.run(&selection_index);
    }
    break;
 
#if defined(CONFIG_PRODUCT_WANPIPE_LIP_ATM)
  case WANCONFIG_LIP_ATM:
    {
      menu_atm_basic_cfg atm_basic_cfg(lxdialog_path, list_element_logical_ch);
      atm_basic_cfg.run(&selection_index);
    }
    break;
#endif
   
  default:
    rc = NO;
    break;
  }

  return rc;
}

int menu_wan_channel_cfg::handle_protocol_change( 
		IN unsigned int new_protocol,
		IN list_element_chan_def* list_element_logical_ch)
{
  int rc = YES;
  link_def_t* link_defs = cfr->link_defs;
  wandev_conf_t* linkconf = link_defs->linkconf;

  fr_config_info_t fr_config_info;

  chan_def_t* chandef = &list_element_logical_ch->data;
  wanif_conf_t* chanconf = chandef->chanconf;

  Debug(DBG_MENU_WAN_CHANNEL_CFG, ("handle_protocol_change()\n"));
     
  //for protocols which need the LIP layer
  objects_list* next_level_obj_list;
  
  //////////////////////////////////////////////////////////////////////////////////////
  next_level_obj_list = (objects_list*)list_element_logical_ch->next_objects_list;
  if(next_level_obj_list != NULL){

    Debug(DBG_MENU_WAN_CHANNEL_CFG, ("deleting next_level_obj_list!\n"));
    delete next_level_obj_list;
    list_element_logical_ch->next_objects_list = NULL;
  }else{
    Debug(DBG_MENU_WAN_CHANNEL_CFG, ("next_level_obj_list == NULL - there is nothing to delete\n"));
  }
  //////////////////////////////////////////////////////////////////////////////////////
	
  //for each card type the Hardware Level interface should have
  //a config_id.
  switch(linkconf->card_type)
  {
  case WANOPT_ADSL:
    chanconf->config_id = WANCONFIG_ADSL;
    break;
  case WANOPT_AFT:
  case WANOPT_AFT104:
  case WANOPT_AFT300:
  case WANOPT_S50X:
  case WANOPT_S51X:
    chanconf->config_id = WANCONFIG_HDLC;
    break;
  }
  
  //initialize firmware to default
  snprintf(link_defs->firmware_file_path, MAX_PATH_LENGTH, "%sfirmware/%s", 
		 wanpipe_cfg_dir, "cdual514.sfm");

  switch(new_protocol)
  {
  case WANCONFIG_EDUKIT:
    //no LIP layer	
    chanconf->config_id = new_protocol;
    
    chandef->usedby = WANPIPE;
    chanconf->mc = WANOPT_NO;
    chanconf->true_if_encoding = WANOPT_NO;
    chanconf->if_down = WANOPT_NO;

    //change firmware from default
    snprintf(link_defs->firmware_file_path, MAX_PATH_LENGTH, "%sfirmware/%s", 
		 wanpipe_cfg_dir, "edu_kit.sfm");
    break;
	  
  case WANCONFIG_HDLC:
    //no LIP layer	
    if(global_card_type == WANOPT_AFT  &&  global_card_version == A200_ADPTR_ANALOG){
      chandef->usedby = API;
      chanconf->hdlc_streaming = WANOPT_NO;
    }else{
      chandef->usedby = WANPIPE;
    }
    //chanconf->hdlc_streaming = WANOPT_YES;//???? - don't do it!! or it may overwrite
    					    //user setting for BitStreaming mode!
    chanconf->config_id = WANCONFIG_HDLC;
    chanconf->mtu = 1500;
    chanconf->mc = WANOPT_NO;
    chanconf->true_if_encoding = WANOPT_NO;
    chanconf->if_down = WANOPT_NO;
    /*
    //this is done in 'list_element_chan_def' constructor, so
    //here is the place to overwrite the defaults. do nothing for WANCONFIG_HDLC.
    aft_ptr->idle_flag = 0x7F;
    aft_ptr->mtu = 1500;
    aft_ptr->mru = 1500; 
    */
    break;

  case PROTOCOL_TDM_VOICE:
    //no LIP layer	
    chandef->usedby = TDM_VOICE;
    chanconf->hdlc_streaming = WANOPT_YES;
    
    chanconf->config_id = PROTOCOL_TDM_VOICE;//WANCONFIG_HDLC;

    chanconf->mtu = 1500;
    chanconf->mc = WANOPT_NO;
    chanconf->true_if_encoding = WANOPT_NO;
    chanconf->if_down = WANOPT_NO;
    break; 

  case PROTOCOL_TDM_VOICE_API:
    //no LIP layer	
    chandef->usedby = TDM_VOICE_API;
    chanconf->hdlc_streaming = WANOPT_YES;
    
    chanconf->config_id = PROTOCOL_TDM_VOICE_API;//WANCONFIG_HDLC;

    chanconf->mtu = 1500;
    chanconf->mc = WANOPT_NO;
    chanconf->true_if_encoding = WANOPT_NO;
    chanconf->if_down = WANOPT_NO;
    break;

  case WANCONFIG_MPPP:

    //special code for ADSL:
    //1. PPP without the LIP layer!!
    //2. only PAP/CHAP cfg, nothing else.
    if(linkconf->card_type == WANOPT_ADSL && new_protocol == WANCONFIG_MPPP){

      Debug(DBG_MENU_WAN_CHANNEL_CFG, ("sub_config_id: %d\n", link_defs->sub_config_id));
      chandef->usedby = WANPIPE;
      return 0;
    }
    
    //needs LIP layer
    chandef->usedby = STACK;
    
    list_element_logical_ch->next_objects_list = new objects_list();
    
    //create the one and only PPP interface insert it into the list
    adjust_number_of_logical_channels_in_list(WANCONFIG_MPPP,
					      list_element_logical_ch->next_objects_list,
					      list_element_logical_ch->data.name,
					      1);
    break;
    
  case WANCONFIG_MFR:
    //needs LIP layer
    chandef->usedby = STACK;
    
    list_element_logical_ch->next_objects_list = new objects_list();
    
    //create auto dlci and insert it into the list
    fr_config_info.name_of_parent_layer = list_element_logical_ch->data.name;
    fr_config_info.auto_dlci = YES;
    adjust_number_of_logical_channels_in_list(WANCONFIG_MFR,
					      list_element_logical_ch->next_objects_list,
					      &fr_config_info,
					      1);
    break;

  case WANCONFIG_LIP_ATM:
    //needs LIP layer
    chandef->usedby = STACK;
    
    list_element_logical_ch->next_objects_list = new objects_list();
    
    //create one ATM interface and insert it into the list
    adjust_number_of_logical_channels_in_list(	WANCONFIG_LIP_ATM, 
						list_element_logical_ch->next_objects_list,
					       	list_element_logical_ch->data.name,
					       	1);
    break;

  case WANCONFIG_MPCHDLC:
    //needs LIP layer
    chandef->usedby = STACK;

    list_element_logical_ch->next_objects_list = new objects_list();
    
    //create the interface and insert it into the list
    adjust_number_of_logical_channels_in_list(WANCONFIG_MPCHDLC,
					      list_element_logical_ch->next_objects_list,
					      list_element_logical_ch->data.name,
					      1);
    break;

  case WANCONFIG_LAPB:
    //needs LIP layer
    chandef->usedby = STACK;

    list_element_logical_ch->next_objects_list = new objects_list();
    
    //create the interface and insert it into the list
    adjust_number_of_logical_channels_in_list(WANCONFIG_LAPB,
					      list_element_logical_ch->next_objects_list,
					      list_element_logical_ch->data.name,
					      1);
	  
    break;
    
  case WANCONFIG_TTY:
    Debug(DBG_MENU_WAN_CHANNEL_CFG, ("WANCONFIG_TTY is the new protocol\n"));
    
    //needs LIP layer
    chandef->usedby = STACK;

    list_element_logical_ch->next_objects_list = new objects_list();
    
    //create the interface and insert it into the list
    adjust_number_of_logical_channels_in_list(WANCONFIG_TTY,
					      list_element_logical_ch->next_objects_list,
					      list_element_logical_ch->data.name,
					      1); 
    break;

  case WANCONFIG_ADSL:
  case PROTOCOL_ETHERNET:
  case PROTOCOL_IP:

    //no LIP layer	
    chandef->usedby = WANPIPE;
    chanconf->hdlc_streaming = WANOPT_YES;
    chanconf->mtu = 1500;
    chanconf->mc = WANOPT_NO;
    chanconf->true_if_encoding = WANOPT_NO;
    chanconf->if_down = WANOPT_NO;
    break;

  default:
    ERR_DBG_OUT(("Invalid new protocol: %d!\n", new_protocol));
    return NO;
  }
  
  return rc;
}

