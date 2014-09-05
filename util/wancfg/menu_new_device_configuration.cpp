/***************************************************************************
                          menu_new_device_configuration.cpp  -  description
                             -------------------
    begin                : Tue Mar 9 2004
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

#include "text_box_help.h"
#include "text_box_yes_no.h"
#include "menu_new_device_configuration.h"

#include "menu_device_miscellaneous_options.h"

#include "menu_net_interfaces_list.h"
#include "menu_hardware_setup.h"
#include "net_interface_file_reader.h"
#include "menu_wan_channel_cfg.h"

#include "conf_file_writer.h"

char* new_dev_cfg_help_str =
"Hardware Setup\n"
"--------------\n"
"Select type of Sangoma card and\n"
"resources used by it.\n"
"\n"
"Protocol\n"
"--------\n"
"Select Communications Protocol.\n"
"Following protocols available for\n"
"S514 and S508 cards:\n"
"  Frame Relay, PPP, CHDLC, HDLC Streaming\n"
"A101/102 cards:\n"
"  MP Frame Relay, MP PPP, MP CHDLC, MP HDLC Streaming\n"
"\n"
"Interface Setup\n"
"---------------\n"
"  -set Interface Name\n"
"  -select Interface Operation Mode: WANPIPE, API, BRIDGE...\n"
"  -set Interface IP information.\n"
"  -configure per-Interface start/stop scripts.\n"
"\n"
"Advanced WANPIPE options\n"
"-----------------------------\n"
"  -configure WANPIPE UDP debugging.\n"
"  -configure per-WANPIPE start/stop scripts.\n";

char* logical_channel_setup_help_str =
"Timeslot Groups Configuration\n"
"-----------------------------\n"
" -Set Number of Timeslot Groups\n"
" -Set Configuration for each Timeslot Group\n";

enum NEW_DEV_CFG_OPTIONS{
  HW_SETUP=100,
  PROTOCOL_SETUP,
  NET_IF_SETUP,
  MISCELLANEOUS_OPTIONS,
  EMPTY_LINE
};

#define DBG_MENU_NEW_DEVICE_CONFIG  1

menu_new_device_configuration::menu_new_device_configuration( IN char * lxdialog_path,
                                                              IN conf_file_reader** ptr_cfr)
{
  Debug(DBG_MENU_NEW_DEVICE_CONFIG,
    ("menu_new_device_configuration::menu_new_device_configuration()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->ptr_cfr = ptr_cfr;
  cfr = *ptr_cfr;

  wanrouter_rc_fr = NULL;
}

menu_new_device_configuration::~menu_new_device_configuration()
{
  Debug(DBG_MENU_NEW_DEVICE_CONFIG,
    ("menu_new_device_configuration::~menu_new_device_configuration()\n"));

  if(wanrouter_rc_fr != NULL){
    delete wanrouter_rc_fr;
  }
}

int menu_new_device_configuration::run(OUT int * selection_index)
{
  string menu_str;
  int rc, number_of_items;
  int exit_dialog;
  char tmp_buff[MAX_PATH_LENGTH];
  char* interface_setup_completion_check;
  unsigned int max_valid_number_of_logical_channels;

  //help text box
  text_box tb;

  link_def_t*		  link_def;
  wandev_conf_t*	  linkconf;
  objects_list*		  obj_list = NULL;
  list_element_chan_def*  list_el_chan_def;
  sdla_fe_cfg_t*	  fe_cfg;
  
again:
  rc = YES;
  exit_dialog = NO;
  number_of_items = 0;

  obj_list = cfr->main_obj_list;
  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;
  fe_cfg = &link_def->linkconf->fe_cfg;

  wanrouter_rc_fr = new wanrouter_rc_file_reader(cfr->wanpipe_number);

  Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("menu_new_device_configuration::run()\n"));
  Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("cfr->link_defs->name: %s\n", link_def->name));
  Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("linkconf->card_type: %d\n", linkconf->card_type));
  Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("link_def->card_version: %d\n", link_def->card_version));
  Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("linkconf->config_id: %d\n", linkconf->config_id));
  Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("fe_cfg->media: 0x%X\n", fe_cfg->media));

  if(linkconf->card_type == NOT_SET){
    //card type not known yet, go directly to hardware setup
    goto display_hw_setup_dialog_label;
  }

  switch(fe_cfg->media)
  {
  case WAN_MEDIA_T1:
    max_valid_number_of_logical_channels = NUM_OF_T1_CHANNELS;
    break;

  case WAN_MEDIA_E1:
    max_valid_number_of_logical_channels = NUM_OF_E1_CHANNELS - 1; //32-1=31
    break;

  case WAN_MEDIA_NONE:
  case WAN_MEDIA_56K:
  case WAN_MEDIA_DS3:
  case WAN_MEDIA_E3:
	case WAN_MEDIA_SERIAL:
    //for now only un-channelized TE3 supported
    max_valid_number_of_logical_channels = 1;
    break;

  case WAN_MEDIA_FXOFXS:
    max_valid_number_of_logical_channels = MAX_FXOFXS_CHANNELS;
    break;

  case WAN_MEDIA_BRI:
    max_valid_number_of_logical_channels = MAX_BRI_TIMESLOTS;
    break;

  default:
    ERR_DBG_OUT(("Invalid Media Type!! (0x%X)\n", fe_cfg->media));
    return NO;
  }

  ////////////////////////////////////////////////////////////////////
  menu_str = " ";
  number_of_items = 5;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", HW_SETUP);
  menu_str += tmp_buff;
  //create the menu item
  switch(linkconf->card_type)
  {
  case WANOPT_AFT:
    switch(link_def->card_version)
    {
    case A101_ADPTR_1TE1:
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Hardware Setup--> %s (CPU: %s)\" ",
        get_card_type_string(linkconf->card_type, link_def->card_version), 
	linkconf->S514_CPU_no);
      break;

    case A104_ADPTR_4TE1:
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Hardware Setup--> %s (Line No: %d)\" ",
        get_card_type_string(linkconf->card_type, link_def->card_version), fe_cfg->line_no);
      break;

    case A200_ADPTR_ANALOG:
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Hardware Setup--> %s\" ",
        get_card_type_string(linkconf->card_type, link_def->card_version));
      break;

    case AFT_ADPTR_56K:
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Hardware Setup--> %s\" ",
        get_card_type_string(linkconf->card_type, link_def->card_version));
      break;

    case AFT_ADPTR_ISDN:
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Hardware Setup--> %s (Line No: %d)\" ",
        get_card_type_string(linkconf->card_type, link_def->card_version), fe_cfg->line_no);
      break;

    default:
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Hardware Setup--> %s\" ",
        get_card_type_string(linkconf->card_type, link_def->card_version));
    }
    break;

  case WANOPT_S51X:
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Hardware Setup--> %s (CPU:%s, Port:%s)\" ",
        get_card_type_string(linkconf->card_type, link_def->card_version), 
	linkconf->S514_CPU_no,
	(linkconf->comm_port == WANOPT_PRI ? "Pri" : "Sec"));
    break;

  default:
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Hardware Setup--> %s\" ",
        get_card_type_string(linkconf->card_type, link_def->card_version));
  }
  menu_str += tmp_buff;

  ////////////////////////////////////////////////////////////////////
  Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("number of objects in the list: %d\n", obj_list->get_size()));
  number_of_items += obj_list->get_size();

  create_logical_channels_list_str( menu_str,
				    max_valid_number_of_logical_channels,
				    obj_list);
  
  Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("create_logical_channels_list_str() - returned\n"));

  ////////////////////////////////////////////////////////////////////
  if(linkconf->config_id != PROTOCOL_NOT_SET ){

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
    menu_str += tmp_buff;
  
    ////////////////////////////////////////////////////////////////////
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", MISCELLANEOUS_OPTIONS);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Advanced WANPIPE options\" " );
    menu_str += tmp_buff;
  }else{
    ;//exit(0);
  }
  

  Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("\nmenu_str:%s\n", menu_str.c_str()));

  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
    "Configuration for Wan Device: %s\n\n", cfr->link_defs->name);

  snprintf(&tmp_buff[strlen(tmp_buff)], MAX_PATH_LENGTH, "%s%s",
    "------------------------------------------\n",			  
    "Press ESC to Cancel/Exit Config");

  //snprintf(&tmp_buff[strlen(tmp_buff)], MAX_PATH_LENGTH - strlen(tmp_buff), "%s", MENUINSTR_EXIT);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          //MENU_BOX_BACK,
                          MENU_BOX_SELECT,//to see "Exit" button
                          lxdialog_path,
                          "WANPIPE CONFIGURATION",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          (number_of_items > 8 ? 8 : number_of_items),
                          (char*)menu_str.c_str()
                          ) == NO){
    rc = NO;
    goto cleanup;
  }

  if(show(selection_index) == NO){
    Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("menu_new_device_configuration.show() - failed!!\n"));
    rc = NO;
    goto cleanup;
  }

  exit_dialog = NO;
  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_NEW_DEVICE_CONFIG,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case EMPTY_LINE:
      ;//do nothing
      break;

    case HW_SETUP:
display_hw_setup_dialog_label:
      {
        menu_hardware_setup hardware_setup(lxdialog_path, cfr);
        if(hardware_setup.run(selection_index) == YES){
		
	   if(linkconf->card_type == NOT_SET){
             Debug(DBG_MENU_NEW_DEVICE_CONFIG, 
		("on return from 'menu_hardware_setup' linkconf->card_type == NOT_SET!!\n"));
	     //not an error - user just want to exit - exit the dialog
	     rc = YES;
             exit_dialog = YES;
	   }else{
	      //do nothing
	   }

        }else{
	  rc = NO;
          exit_dialog = YES;
	}
      }
      break;
      
    case NET_IF_SETUP:
      {	//display list of network interfaces.
        menu_net_interfaces_list 
	    net_interfaces_list(lxdialog_path, (list_element_chan_def*)obj_list->get_first());

        if(net_interfaces_list.run(selection_index) == NO){
          rc = NO;
        }
      }
      break;

    case PROTOCOL_SETUP:
      {	//for single 'channels group' case
	menu_wan_channel_cfg per_wan_channel_cfg( lxdialog_path,
						  cfr,
                                                  (list_element_chan_def*)obj_list->get_first());

        rc = per_wan_channel_cfg.run( cfr->wanpipe_number, selection_index, obj_list->get_size());
        if(rc == YES){
          goto again;
        }
      }	
      break;

    case MISCELLANEOUS_OPTIONS:
      {
        menu_device_miscellaneous_options device_miscellaneous_options(lxdialog_path, cfr);
        if(device_miscellaneous_options.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    default:
      
      Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("default case selection: %d\n", *selection_index));
      
      ////////////////////////////////////////////////////////////
      //general case for AFT: more than one Timeslot Group exist.
      //one of the Timeslot Groups was selected for configuration
      list_el_chan_def =
	(list_element_chan_def*)obj_list->find_element(
				    remove_spaces_in_int_string(get_lxdialog_output_string()));

      if(list_el_chan_def != NULL){

	menu_wan_channel_cfg per_wan_channel_cfg( lxdialog_path,
                                                   cfr,
                                                   list_el_chan_def);

        rc = per_wan_channel_cfg.run( cfr->wanpipe_number, selection_index, obj_list->get_size());
        if(rc == YES){
          goto again;
        }
      }else{
	//error - failed to find the element
        ERR_DBG_OUT(("Failed to find the element!! selection: %s\n",
            get_lxdialog_output_string()));
        rc = NO;
        exit_dialog = YES;
      }//if(list_el_chan_def != NULL)
    }//witch(atoi(get_lxdialog_output_string()))

    break;
    
  case MENU_BOX_BUTTON_HELP:
    
    switch(atoi(get_lxdialog_output_string()))
    {
    case HW_SETUP:
    case PROTOCOL_SETUP:
    case NET_IF_SETUP:
    case MISCELLANEOUS_OPTIONS:
    default:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, new_dev_cfg_help_str);
      break;
/*
    case LOGICAL_CHANNELS_SETUP:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, logical_channel_setup_help_str);
      break;
*/
    }
    break;

  case MENU_BOX_BUTTON_EXIT:

    //run checks on the configuration here. notify user about errors.
    if(linkconf->config_id == PROTOCOL_NOT_SET){
      tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
        "Protocol is NOT selected!!");
      rc = YES;
      exit_dialog = YES;
      break;
    }

    if(check_aft_timeslot_groups_cfg() == NO){
	    
      //tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
      //  "Error(s) found in configuration.");
      break;
    }

    /////////////////////////////////////////////////////////////
    //check configuration of each interface
    interface_setup_completion_check = check_configuration_of_interfaces(cfr->main_obj_list);
    if(interface_setup_completion_check != NULL){

      replace_new_line_with_space(interface_setup_completion_check);

      tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
        interface_setup_completion_check);
      break;
    }
     
    ///////////////////////////////////////////////////////////
    //if no errors found ask confirmation to save to file.
    ///////////////////////////////////////////////////////////
    snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to save '%s.conf' configuration file?",
      cfr->link_defs->name);

    if(yes_no_question(   selection_index,
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
      {
        conf_file_writer cf_writer(cfr);
        cf_writer.write();
      }
      break;

    case YES_NO_TEXT_BOX_BUTTON_NO:
      break;
    }
    ///////////////////////////////////////////////////////////
    //check if this wanpipe is in wanrouter.rc->WAN_DEVICES list
    if(wanrouter_rc_fr->search_and_update_boot_start_device_setting() == NO){
    
      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to add  '%s' to\n 'wanrouter start' sequence?",
        cfr->link_defs->name);

      if(yes_no_question(  selection_index,
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
        wanrouter_rc_fr->update_wanrouter_rc_file();
        break;
      case YES_NO_TEXT_BOX_BUTTON_NO:
        break;
      }
      ///////////////////////////////////////////////////////////
    }

    rc = YES;
    exit_dialog = YES;
    break;
    
  }//switch(*selection_index)

  
  if(exit_dialog == NO){
    goto again;
  }
  
cleanup:
  return rc;
  ////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////////////////
//recursevly check configuration of all interface belonging to a list
char* menu_new_device_configuration::check_configuration_of_interfaces(objects_list * obj_list)
{
  char* return_str;
  chan_def_t* chandef;

  Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("check_configuration_of_interfaces()\n"));
  
  if(obj_list == NULL){
    ERR_DBG_OUT(("ip_address_configuration_complete(): obj_list == NULL!!\n"));
    snprintf(err_buf, MAX_PATH_LENGTH,
      "ip_address_configuration_complete(): obj_list == NULL!!\n");
    return err_buf;
  }

  return_str = ip_address_configuration_complete(obj_list);
  if(return_str != NULL){
    //a  check failed
    return return_str;
  }
	  
  list_element_chan_def* list_el_chan_def = (list_element_chan_def*)obj_list->get_first();

  while(list_el_chan_def != NULL){
    chandef = &list_el_chan_def->data;

    Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("device name->: %s\n",chandef->name ));
	
    if(list_el_chan_def->next_objects_list != NULL){
	    
      Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("-------- next level -------\n" ));
      
      //recursive call
      return_str = check_configuration_of_interfaces((objects_list*)list_el_chan_def->next_objects_list);
      if(return_str != NULL){
	//a  check failed
	return return_str;
      }

      Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("-------- end of next level -------\n"));
    }

    list_el_chan_def = (list_element_chan_def*)obj_list->get_next_element(list_el_chan_def);
  }//while()

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////////////////

char* menu_new_device_configuration::ip_address_configuration_complete(objects_list * obj_list)
{
#if 1
  Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("ip_address_configuration_complete()\n"));

  if(obj_list == NULL){
    ERR_DBG_OUT(("ip_address_configuration_complete(): obj_list == NULL!!\n"));
    snprintf(err_buf, MAX_PATH_LENGTH,
      "ip_address_configuration_complete(): obj_list == NULL!!\n");
    return err_buf;
  }

  list_element_chan_def* list_el_chan_def = (list_element_chan_def*)obj_list->get_first();

  while(list_el_chan_def != NULL){

    Debug(DBG_MENU_NEW_DEVICE_CONFIG,
      ("list_el_chan_def->data.addr: %s\n", list_el_chan_def->data.addr));

    switch(list_el_chan_def->data.usedby)
    {
    case WANPIPE:
    case BRIDGE_NODE:
      {
        Debug(DBG_MENU_NEW_DEVICE_CONFIG,
          ("attempting to read 'net interface file' for %s\n", list_el_chan_def->data.addr));

        net_interface_file_reader interface_file_reader(list_el_chan_def->data.name);

        if(interface_file_reader.parse_net_interface_file() == NO){
          Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("parse_net_interface_file() FAILED!!\n"));

          snprintf(err_buf, MAX_PATH_LENGTH,
            "Failed to parse 'Net Interface' file for %s!!\n", list_el_chan_def->data.name);
          return err_buf;
        }
        /////////////////////////////////////////////////////////////////
        //file was parsed, check the contents is complete
        if( strlen(interface_file_reader.if_config.ipaddr) <  MIN_IP_ADDR_STR_LEN ||
          strlen(interface_file_reader.if_config.point_to_point_ipaddr) < MIN_IP_ADDR_STR_LEN){
                                                         //  v NO SPACE HERE!!
          snprintf(err_buf, MAX_PATH_LENGTH, "IP Setup for %s is Incomplete!\n\n",
            interface_file_reader.if_config.device);

          return err_buf;

        }else{
          //ip cfg is complete
          //snprintf(tmp_buff, MAX_PATH_LENGTH, " \"IP Address Setup---> Complete\" ");
        }
      }
      break;

    default:
      Debug(DBG_MENU_NEW_DEVICE_CONFIG,
        ("NO NEED to read 'net interface file' for %s\n", list_el_chan_def->data.addr));
      break;
    }

    list_el_chan_def =
      (list_element_chan_def*)obj_list->get_next_element(list_el_chan_def);

  }//while()
#endif
  return NULL;
}

int menu_new_device_configuration::create_logical_channels_list_str(
		string& menu_str,
		unsigned int max_valid_number_of_logical_channels,
		objects_list* obj_list)
{
  char tmp_buff[MAX_PATH_LENGTH];
  list_element_chan_def* list_el_chan_def;
  chan_def_t* chandef;
  chan_def_t* this_level_chandef;

  if(obj_list->get_size() > 0 && obj_list->get_size() <= max_valid_number_of_logical_channels){

    list_el_chan_def = (list_element_chan_def*)obj_list->get_first();
    
    while(list_el_chan_def != NULL){

      this_level_chandef = &list_el_chan_def->data;

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", this_level_chandef->addr);
      menu_str += tmp_buff;

      Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("this_level_chandef->name: %s\n",
			     this_level_chandef->name));

      Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("this_level_chandef->addr: %s\n",
			     this_level_chandef->addr));

      Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("config_id: %d\n", this_level_chandef->chanconf->config_id));
      
      if(this_level_chandef->chanconf->config_id != PROTOCOL_NOT_SET){
  
	objects_list* next_level_obj_list = (objects_list*)list_el_chan_def->next_objects_list;
	list_element_chan_def* next_level_list_el_chan_def = NULL;
     
        if(next_level_obj_list != NULL){

          if(next_level_obj_list->get_size() == 0){
	    //empty!!
#if defined(__LINUX__) && !BSD_DEBG
	    ERR_DBG_OUT(("The Protocol layer interface list is empty!!\n\
Usually means the name of an interface does NOT start with name of it's 'parent' STACK device.\n\
For example: if name of 'parent' STACK device is 'w1g1', name of PPP interface running above it\n\
should be 'w1g1ppp' or 'w1g1?' where '?' is alphanumerical, up to 8 characters.\n"));
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
	    ERR_DBG_OUT(("The Protocol layer interface list is empty!!\n\
Usually means the name of an interface does NOT start with name of it's 'parent' STACK device.\n\
For example: if name of 'parent' STACK device is 'waga', name of PPP interface running above it\n\
should be 'wagappp0' or 'waga?#' where '?' is alphabetical and '#' is numerical.\n"));
#endif
	    return NO;
	  }
	  
	  next_level_list_el_chan_def = (list_element_chan_def*)next_level_obj_list->get_first();
	  chandef = &next_level_list_el_chan_def->data;
	
	}else{
          Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("-- 1.2\n"));
	  chandef = &list_el_chan_def->data;
	}

	if(chandef == NULL){
	  ERR_DBG_OUT(("Invalid 'chandef' pointer!!\n"));
	  return NO;
	}
	
	if(obj_list->get_size() > 1){
	  
          Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("-- 3\n"));

	  //General case - more than one logical channel.
	  //Usually AFT groups of channels because protocols are in the LIP layer.
	  
          if(chandef->chanconf->config_id == WANCONFIG_AFT ||
             chandef->chanconf->config_id == WANCONFIG_AFT_TE3){

	    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s %s: %s\" ",
	      TIME_SLOT_GROUP_STR, list_el_chan_def->data.addr,
	      "HDLC Streaming");
	  }else{
	    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s %s: %s\" ",
	      TIME_SLOT_GROUP_STR, list_el_chan_def->data.addr,
	      get_protocol_string(chandef->chanconf->config_id));
	  }
	  menu_str += tmp_buff;
	  
	}else{
		
          Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("-- 4\n"));

	  //Special case - exactly one logical channel.
          if(chandef->chanconf->config_id == WANCONFIG_AFT ||
             chandef->chanconf->config_id == WANCONFIG_AFT_TE3){
		  
	    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s %s\" ",
	      "Protocol-------->",
	      "HDLC Streaming");
	  }else{

	    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s %s\" ",
	      "Protocol-------->",
	      get_protocol_string(chandef->chanconf->config_id));
	  }
	  menu_str += tmp_buff;

	  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", NET_IF_SETUP);
	  menu_str += tmp_buff;

	  if(next_level_obj_list != NULL){
		 
	    //create the menu item
	    if(next_level_obj_list->get_size()){
	      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface Setup-> %d Defined\" ",
		next_level_obj_list->get_size());
	    }else{
	      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface Setup->  Undefined\" ");
	    }
	    menu_str += tmp_buff;
	  }else{
	    
	    //create the menu item
	    if(obj_list->get_size()){
	      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface Setup-> %d Defined\" ",
		obj_list->get_size());
	    }else{
	      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface Setup->  Undefined\" ");
	    }
	    menu_str += tmp_buff;
	    
	  }//if(next_level_obj_list != NULL)
	  
	}//if(obj_list->get_size() > 1)
      }else{
	      
	//new configuration - NO protocol yet
	if(obj_list->get_size() > 1){
          FUNC_DBG();

	  //general case - more than one	
	  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s %s: %s\" ",
	    TIME_SLOT_GROUP_STR,
	    this_level_chandef->addr,
	    get_protocol_string(this_level_chandef->chanconf->config_id));
	  menu_str += tmp_buff;
	  
	}else{

	  //special case - exactly one
	  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Protocol--------> Not Configured\" ");
	  menu_str += tmp_buff;
	}
      }//if(this_level_chandef->chanconf->config_id != PROTOCOL_NOT_SET)

      list_el_chan_def =
	    (list_element_chan_def*)obj_list->get_next_element(list_el_chan_def);
    }//while()
    
  }else{
    ERR_DBG_OUT(("Invalid number of %s: %d!!\n", TIME_SLOT_GROUPS_STR, obj_list->get_size()));
    return NO;
  }

  return YES;
}

//all Timeslot Groups check
int menu_new_device_configuration::check_aft_timeslot_groups_cfg()
{
  objects_list * obj_list = cfr->main_obj_list;
  list_element_chan_def* list_el_chan_def = NULL;
  list_element_chan_def* first_list_el_chan_def = NULL;
  chan_def_t* chandef;
  text_box tb;
  char tmp_buff[MAX_PATH_LENGTH];
  wan_tdmv_conf_t* tdmv_conf=&cfr->link_defs->linkconf->tdmv_conf;
  char local_is_there_a_voice_if = NO;
  
  Debug(DBG_MENU_NEW_DEVICE_CONFIG,
      ("int menu_new_device_configuration::check_aft_timeslot_groups_cfg()\n"));

  first_list_el_chan_def = list_el_chan_def = (list_element_chan_def*)obj_list->get_first();

  if(list_el_chan_def != NULL){
    chandef = &list_el_chan_def->data;
  }
	  
  //int i = 0;
  while(list_el_chan_def != NULL){

    Debug(DBG_MENU_NEW_DEVICE_CONFIG,
      ("list_el_chan_def->data.addr: %s\n", list_el_chan_def->data.addr));

    chandef = &list_el_chan_def->data;

    Debug(DBG_MENU_NEW_DEVICE_CONFIG, ("chandef->usedby: %d, tdmv_span_no: %d\n",
      chandef->usedby,tdmv_conf->span_no));
  
    //some special checks may be needed, depending how 'group' is actually used.
    switch(chandef->usedby)
    {
    case TDM_VOICE:
    case TDM_VOICE_API:
      local_is_there_a_voice_if = YES;
      
      if(tdmv_conf->span_no == 0){
	//user must initialize the span_no!
        snprintf(tmp_buff, MAX_PATH_LENGTH, "Error: Span Number not set!\n\
Must be a non-zero number.\n\
Can be set under \"Interface Setup\" \n\
for any of TDM Voice interfaces.");
      	tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED, tmp_buff);
        return NO;
      }
      break;
    }//switch()

    list_el_chan_def =
      (list_element_chan_def*)obj_list->get_next_element(list_el_chan_def);
  }
  
  is_there_a_voice_if = local_is_there_a_voice_if;
  
  return YES;
}

//FIXME: make sure it is done when user trying to exit this menu
/*
int menu_new_device_configuration::check_all_groups_of_channels_configured()
{
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  //help text box
  text_box tb;

  objects_list * obj_list = cfr->main_obj_list;
  list_element_chan_def* list_el_chan_def = (list_element_chan_def*)obj_list->get_first();

  while(list_el_chan_def != NULL){

    if(list_el_chan_def->data.chanconf->protocol == PROTOCOL_NOT_SET){
      snprintf(tmp_buff, MAX_PATH_LENGTH,
        "Error:\nConfiguration of \"%s %s\" is incomplete!\nProtocol is not set.",
        TIME_SLOT_GROUP_STR, list_el_chan_def->data.addr);
      tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED, tmp_buff);

      rc = NO;
      break;
    }
    list_el_chan_def = (list_element_chan_def*)obj_list->get_next_element(list_el_chan_def);
  }

  return rc;
}
*/
