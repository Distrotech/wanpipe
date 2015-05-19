/***************************************************************************
                          menu_wan_channel_cfg_v1.cpp  -  description
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

#include "menu_wan_channel_cfg_v1.h"
#include "menu_select_protocol.h"

#include "menu_frame_relay_basic_cfg.h"
#include "menu_ppp_basic_cfg.h"
#include "menu_chdlc_basic_cfg.h"

#include "menu_net_interfaces_list.h"

#define DBG_MENU_WAN_CHANNEL_CFG_V1 1

extern char* aft_logical_channel_protocol_select_help_str;

enum AFT_LOGICAL_CHANNEL_CONFIGURATION{

  CURRENT_PROTOCOL,
  CONFIGURE_CURRENT_PROTOCOL,
  SELECT_NEW_PROTOCOL,

  NET_IF_SETUP,
  EMPTY_LINE
};


menu_wan_channel_cfg_v1::menu_wan_channel_cfg_v1( IN char * lxdialog_path,
						  IN list_element_chan_def* parent_list_element_logical_ch)
{
  Debug(DBG_MENU_WAN_CHANNEL_CFG_V1,
    ("menu_wan_channel_cfg_v1::menu_wan_channel_cfg_v1()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);

  this->parent_list_element_logical_ch = parent_list_element_logical_ch;
}

menu_wan_channel_cfg_v1::~menu_wan_channel_cfg_v1()
{
  Debug(DBG_MENU_WAN_CHANNEL_CFG_V1,
    ("menu_wan_channel_cfg_v1::~menu_wan_channel_cfg_v1()\n"));
}

int menu_wan_channel_cfg_v1::run(IN int wanpipe_number,
			         OUT int * selection_index,
                                 IN int number_of_timeslot_groups)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  int num_of_visible_items = 1;
  //help text box
  text_box tb;
  input_box_active_channels act_channels_ip;
  
  int old_protocol, new_protocol;
  int exit_dialog;
  
  list_element_chan_def* next_level_list_el_chan_def;
  objects_list* next_level_obj_list;
  chan_def_t* parent_chandef;
  chan_def_t* child_chandef=NULL;
  
  input_box input_bx;
  char backtitle[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility");

  Debug(DBG_MENU_WAN_CHANNEL_CFG_V1,("menu_frame_relay_DLCI_configuration::run()\n"));

again:
  rc = YES;
  menu_str = " ";
  exit_dialog = NO;
  
  if(parent_list_element_logical_ch == NULL){
    ERR_DBG_OUT(("The 'parent_list_element_logical_ch' is NULL!\n"));
    rc = NO;
    goto cleanup;
  }
  
  parent_chandef = &parent_list_element_logical_ch->data;
  Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("%s:(parent): parent_chandef->usedby: %d\n", 
			parent_chandef->name, parent_chandef->usedby));
  Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("%s:(parent): parent_chandef->chanconf->config_id: %d\n",
			parent_chandef->name, parent_chandef->chanconf->config_id));

  next_level_obj_list = (objects_list*)parent_list_element_logical_ch->next_objects_list;
  if(next_level_obj_list != NULL){

    Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("next_level_obj_list != NULL\n"));
			    
    next_level_list_el_chan_def = (list_element_chan_def*)next_level_obj_list->get_first();
    child_chandef = &next_level_list_el_chan_def->data;

    Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("%s:(child): child_chandef->usedby: %d\n",
		child_chandef->name, child_chandef->usedby));
    Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("%s:(child): child_chandef->chanconf->config_id: %d\n",
		child_chandef->name, child_chandef->chanconf->config_id));

    /////////////////////////////////////////////////////////////////////////////////
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CURRENT_PROTOCOL);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"[Selected Protocol : %s]\" ",
      get_protocol_string(child_chandef->chanconf->config_id));
    menu_str += tmp_buff;
    /////////////////////////////////////////////////////////////////////////////////
  }else{
    //there is no LIP protocol above right now.
    Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("next_level_obj_list == NULL\n"));
    //the only option is to 'select new protocol'
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", SELECT_NEW_PROTOCOL);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Protocol--------> Not Configured\" ");
    menu_str += tmp_buff;

    num_of_visible_items = 1;
    goto display_this_dialog;
  }
  
  /////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
  menu_str += tmp_buff;
     
  /////////////////////////////////////////////////////////////////////////////////
  num_of_visible_items = 4;

  switch(child_chandef->chanconf->config_id)
  {
  case WANCONFIG_MFR:
  case WANCONFIG_MPPP:
  case WANCONFIG_MPCHDLC:
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CONFIGURE_CURRENT_PROTOCOL);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Configure Selected Protocol\" ");
    menu_str += tmp_buff;
    break;
	  
  case WANCONFIG_TTY:
    //do nothing - there is nothing to configure.
    break;

  default:  
    ERR_DBG_OUT(("Invalid protocol: %d(%s)!!\n", child_chandef->chanconf->config_id,
					get_protocol_string(child_chandef->chanconf->config_id)));
    rc = NO;
    goto cleanup;
  }

  /////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", SELECT_NEW_PROTOCOL);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Select New Protocol\" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", NET_IF_SETUP);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface Setup------------> %d Defined\" ",
    next_level_obj_list->get_size());
  menu_str += tmp_buff;
   
  num_of_visible_items += next_level_obj_list->get_size();

  /////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
display_this_dialog:
  if(child_chandef != NULL){
   snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\
\nSet cofiguration of %s.", child_chandef->name);
  }else{
   snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\
\nSet Protocol above STACK interface %s.", parent_chandef->name);
  }
  
  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "WAN CHANNEL CONFIGURATION",// (V1)",
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
    Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("option selected for editing: %s\n",
      get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case CURRENT_PROTOCOL:
    case EMPTY_LINE:
      //do nothing
      break;
      
    case CONFIGURE_CURRENT_PROTOCOL:
      if(display_protocol_cfg(parent_list_element_logical_ch) == NO){
          rc = NO;
      }else{
	  //pointers may change, go re-initialize them
          goto again;
      }
      break;
        
    case SELECT_NEW_PROTOCOL:
      {
        /////////////////////////////////////////////////////////////////////////////////
        if(child_chandef != NULL && child_chandef->chanconf->config_id != PROTOCOL_NOT_SET){
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
	if(child_chandef != NULL){
          old_protocol = child_chandef->chanconf->config_id;
	}else{
	  old_protocol = PROTOCOL_NOT_SET;
	}
      
      	menu_select_protocol select_protocol(lxdialog_path, NULL);
	select_protocol.run(&new_protocol);
	
	Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("old_protocol: %d, new_protocol: %d\n",
		  old_protocol, new_protocol));

// ADBG
exit(0);
	if(new_protocol == 0){
	  //no protocol was selected, just redisplay
  	  goto again;
	}
	if(old_protocol == new_protocol){
	  //no change, just redisplay
	  goto again;
	}

	//set the new LIP protocol above 'parent' list element
	if(handle_protocol_change(new_protocol, parent_list_element_logical_ch) == NO){
	  goto cleanup;
	}
      }
      break;
    
    case NET_IF_SETUP:
      {	//display list of network interfaces.
        menu_net_interfaces_list net_interfaces_list(lxdialog_path,
		      				     parent_list_element_logical_ch);
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
      Debug(DBG_MENU_WAN_CHANNEL_CFG_V1,
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
  Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("menu_wan_channel_cfg_v1::run(): rc: %d\n", rc));
  return rc;
}

int menu_wan_channel_cfg_v1::display_protocol_cfg(IN list_element_chan_def* parent_list_element_logical_ch)
{
  int rc = YES;
  int selection_index;
  list_element_chan_def* next_level_list_el_chan_def;
  objects_list* next_level_obj_list;
  chan_def_t* chandef;

  Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("display_protocol_cfg()\n"));

  next_level_obj_list = (objects_list*)parent_list_element_logical_ch->next_objects_list;
  if(next_level_obj_list != NULL){

    Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("next_level_obj_list != NULL\n"));
			    
    next_level_list_el_chan_def = (list_element_chan_def*)next_level_obj_list->get_first();
    chandef = &next_level_list_el_chan_def->data;
  }else{
    //there must be a LIP layer above for this function to work
    ERR_DBG_OUT(("Invalid 'next_level_obj_list' pointer!\n"));
    return NO;    
  }

  Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("chandef->chanconf->config_id: %d\n",
			 chandef->chanconf->config_id));
  Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("chan_def->usedby: %d\n",
			  chandef->usedby));

  switch(chandef->chanconf->config_id)
  {
  case WANCONFIG_MFR:
    {
      menu_frame_relay_basic_cfg fr_basic_cfg(lxdialog_path, parent_list_element_logical_ch);
      fr_basic_cfg.run(&selection_index);
    }
    break;

  case WANCONFIG_MPPP://and WANCONFIG_MPROT too
    {
      menu_ppp_basic_cfg ppp_basic_cfg(lxdialog_path, parent_list_element_logical_ch);
      ppp_basic_cfg.run(&selection_index);
    }
    break;

  case WANCONFIG_MPCHDLC:
    {
      menu_chdlc_basic_cfg chdlc_basic_cfg(lxdialog_path, parent_list_element_logical_ch);
      chdlc_basic_cfg.run(&selection_index);
    }
    break;
  
  default:
    rc = NO;
    break;
  }

  return rc;
}

int menu_wan_channel_cfg_v1::handle_protocol_change( 
					IN unsigned int new_protocol,
					IN list_element_chan_def* list_element_logical_ch)
{
  int rc = YES;
  fr_config_info_t fr_config_info;
  //list of interface in the new LIP layer
  objects_list* next_level_obj_list;
  
  Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("handle_protocol_change()\n"));
  
  //////////////////////////////////////////////////////////////////////////////////////
  next_level_obj_list = (objects_list*)list_element_logical_ch->next_objects_list;
  if(next_level_obj_list != NULL){

    Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("deleting old next_level_obj_list!\n"));
    delete next_level_obj_list;
    list_element_logical_ch->next_objects_list = NULL;
  }else{
    Debug(DBG_MENU_WAN_CHANNEL_CFG_V1, ("old next_level_obj_list == NULL - there is nothing to delete\n"));
  }

  //all of these protocols in LIP layer, so create the 'objects list'
  list_element_logical_ch->next_objects_list = new objects_list();
  
  //////////////////////////////////////////////////////////////////////////////////////
  
  switch(new_protocol)
  {
  case WANCONFIG_MPPP:
    //create the one and only PPP interface insert it into the list
    adjust_number_of_logical_channels_in_list(WANCONFIG_MPPP,
					      list_element_logical_ch->next_objects_list,
					      parent_list_element_logical_ch->data.name,
					      1);
    break;
    
  case WANCONFIG_MFR:
    //create auto dlci and insert it into the list
    fr_config_info.name_of_parent_layer = list_element_logical_ch->data.name;
    fr_config_info.auto_dlci = YES;
    adjust_number_of_logical_channels_in_list(WANCONFIG_MFR,
					      list_element_logical_ch->next_objects_list,
					      &fr_config_info,
					      1);
    break;

  case WANCONFIG_MPCHDLC:
    //create the interface and insert it into the list
    adjust_number_of_logical_channels_in_list(WANCONFIG_MPCHDLC,
					      list_element_logical_ch->next_objects_list,
					      list_element_logical_ch->data.name,
					      1);
    break;

  case WANCONFIG_TTY:
    //create the interface and insert it into the list
    adjust_number_of_logical_channels_in_list(WANCONFIG_TTY,
					      list_element_logical_ch->next_objects_list,
					      list_element_logical_ch->data.name,
					      1); 
    break;
    
  default:
    ERR_DBG_OUT(("Invalid new protocol: %d!\n", new_protocol));
    return NO;
  }
  
  return rc;
}

