/***************************************************************************
                          menu_net_interfaces_list.cpp  -  description
                             -------------------
    begin                : Wed Mar 24 2004
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

#include "menu_net_interfaces_list.h"
#include "text_box_help.h"
#include "objects_list.h"

#include "menu_net_interface_setup.h"

char* net_if_list_no_interface_error_message =
"Network Interfaces List\n"
"-----------------------\n"
"No Network Interfaces defined.\n"
"Each Network Interface correspons to a Logical Channel.\n"
"Add at least one Logical Channel on the Protocol Level.\n";

char* net_if_list_help =
"NETWORK INTERFACE CONFIGURATION\n"
"\n"
"Each network interface has the following\n"
"options: Interface Name, Operation Mode,\n"
"IP Address Setup and Interface Scripts.\n"
"\n"
"Select and configure each interface\n"
"displayed in the menu.  When the configuration of ALL\n"
"interfaces is complete, use the <Back> option to exit\n"
"the dialog\n";


#define DBG_MENU_NET_INTERFACES_LIST 1

menu_net_interfaces_list::menu_net_interfaces_list( IN char * lxdialog_path,
                                                    IN list_element_chan_def* list_element_logical_ch)
{
  Debug(DBG_MENU_NET_INTERFACES_LIST,
    ("menu_net_interfaces_list::menu_net_interfaces_list()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  
  this->list_element_logical_ch = list_element_logical_ch;
}

menu_net_interfaces_list::~menu_net_interfaces_list()
{
  Debug(DBG_MENU_NET_INTERFACES_LIST,
    ("menu_net_interfaces_list::~menu_net_interfaces_list()\n"));
}

int menu_net_interfaces_list::run(OUT int * selection_index)
{
  string menu_str;
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog = NO;
  int number_of_items;
  int protocol;
  text_box tb;

  list_element_chan_def* next_level_list_el_chan_def;
  chan_def_t* chandef;

  Debug(DBG_MENU_NET_INTERFACES_LIST, ("menu_net_interfaces_list::run()\n"));

again:
  
  objects_list * obj_list = (objects_list*)list_element_logical_ch->next_objects_list;
  if(obj_list == NULL){
    //it means there is no LIP layer - protocol is in hardware or firmware
    //ERR_DBG_OUT(("'obj_list' pointer is NULL!!\n"));
    //return NO;
  }
  
  if(obj_list && obj_list->get_size() == 0){
    ERR_DBG_OUT(("No interfaces defined!!\n"));
    return NO;
  } 
  
  chandef = &list_element_logical_ch->data;
  if(obj_list){
    next_level_list_el_chan_def = (list_element_chan_def*)obj_list->get_first();
  }else{
    next_level_list_el_chan_def = list_element_logical_ch;
  }

//again:
  menu_str = "";
  protocol = chandef->chanconf->config_id;
    
  Debug(DBG_MENU_NET_INTERFACES_LIST, ("chandef->chanconf->config_id: %d\n",
      chandef->chanconf->config_id));
    
  Debug(DBG_MENU_NET_INTERFACES_LIST, ("next_level_list_el_chan_def->data.addr: %s\n",
      next_level_list_el_chan_def->data.addr));
  
  int i=0;
  while(next_level_list_el_chan_def != NULL){

    //create tag
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", next_level_list_el_chan_def->data.addr);
    menu_str += tmp_buff;

    i++;
    switch(protocol)
    {
    case WANCONFIG_MFR:
	if(atoi(next_level_list_el_chan_def->data.addr) == 1){
	  //it is an 'auto' dlci
          snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface %d (DLCI:auto)--> %s\" ",
            i,
            next_level_list_el_chan_def->data.name);
	}else{
	  //it is a regular dlci
          snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface %d (DLCI:%s)----> %s\" ",
            i,
            next_level_list_el_chan_def->data.addr,
            next_level_list_el_chan_def->data.name);
	}
      break;
      
    default:
        //snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface %s--> %s\" ",
        //  next_level_list_el_chan_def->data.addr,
        //  next_level_list_el_chan_def->data.name);
        snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface %d--> %s\" ",
          i,
          next_level_list_el_chan_def->data.name);
    }//switch()
    menu_str += tmp_buff;

    if(obj_list){
      next_level_list_el_chan_def = 
	(list_element_chan_def*)obj_list->get_next_element(next_level_list_el_chan_def);
    }else{
      break;
    }
  }//while()

  if(obj_list){
    number_of_items = obj_list->get_size();
  }else{
    number_of_items = 1;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nList of Network Interfaces for Wan Device: %s.\
\n-------------------------------------------\
\nPlease select Network Interface you want to configure.", list_element_logical_ch->data.name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "NETWORK INTERFACE(S) LIST",
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
    rc = NO;
    goto cleanup;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  exit_dialog = NO;
  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_NET_INTERFACES_LIST,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    if(obj_list){
      next_level_list_el_chan_def = 
	      (list_element_chan_def *)obj_list->find_element(
						remove_spaces_in_int_string(get_lxdialog_output_string()));
    }else{
      next_level_list_el_chan_def = list_element_logical_ch;
    }

    if(next_level_list_el_chan_def == NULL){
      ERR_DBG_OUT(("Failed to find selected element '%s' in the list\n",
	get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
    }else{
      
      //found interface in the list.
      Debug(DBG_MENU_NET_INTERFACES_LIST,
          ("Interface selected for configuration %s\n", next_level_list_el_chan_def->data.name));
      
      menu_net_interface_setup net_interface_setup(lxdialog_path, next_level_list_el_chan_def);
      net_interface_setup.run(selection_index);

      rc = YES;
      exit_dialog = NO;
      
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_list_help);
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

