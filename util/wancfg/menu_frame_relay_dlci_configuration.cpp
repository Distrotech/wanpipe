/***************************************************************************
                          menu_frame_relay_dlci_configuration.cpp  -  description
                             -------------------
    begin                : Fri Mar 19 2004
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

#include "menu_frame_relay_dlci_configuration.h"
#include "text_box.h"
#include "input_box_frame_relay_dlci_number.h"
#include "menu_frame_relay_advanced_dlci_configuration.h"

#define DBG_MENU_FRAME_RELAY_DLCI_CONFIGURATION 0

menu_frame_relay_DLCI_configuration::
	menu_frame_relay_DLCI_configuration(  IN char * lxdialog_path,
                                        IN objects_list* obj_list,
                                        IN list_element_frame_relay_dlci* dlci_cfg)
{
  Debug(DBG_MENU_FRAME_RELAY_DLCI_CONFIGURATION,
    ("menu_frame_relay_DLCI_configuration::menu_frame_relay_DLCI_configuration()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);

  this->obj_list = obj_list;
  this->dlci_cfg = dlci_cfg;
}

menu_frame_relay_DLCI_configuration::~menu_frame_relay_DLCI_configuration()
{
  Debug(DBG_MENU_FRAME_RELAY_DLCI_CONFIGURATION,
    ("menu_frame_relay_DLCI_configuration::~menu_frame_relay_DLCI_configuration()\n"));
}

int menu_frame_relay_DLCI_configuration::run( OUT int * selection_index,
                                              IN char* name_of_underlying_layer)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  int num_of_visible_items;

  Debug(DBG_MENU_FRAME_RELAY_DLCI_CONFIGURATION,
    ("menu_frame_relay_DLCI_configuration::run()\n"));

again:
  rc = YES;

  if(dlci_cfg ==  NULL){
    ERR_DBG_OUT(("DLCI configuration pointer is NULL (dlci_cfg) !!!\n"));
    rc = NO;
    goto cleanup;
  }

  menu_str = " ";
  if(strcmp(dlci_cfg->data.addr, "1") != 0){
    //if 'auto' - cannot set the number
    menu_str = " \"1\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"DLCI Number--> %s\" ", dlci_cfg->data.addr);
    menu_str += tmp_buff;
  }

  menu_str += " \"2\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Advanced DLCI configuration\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\
\nSet per-DLCI cofiguration.");

  num_of_visible_items = 2;

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "FRAME RELAY DLCI CONFIGURATION",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          num_of_visible_items,//number of visible items in the menu
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
    Debug(DBG_MENU_FRAME_RELAY_DLCI_CONFIGURATION,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1://set DLCI number

      if(strcmp(dlci_cfg->data.addr, "1") == 0){
        text_box tb;
        tb.show_error_message(lxdialog_path, WANCONFIG_FR,
                            "The DLCI is in 'Auto' mode. DLCI number can not be set.");
        goto again;
      }

      if(set_dlci_number(selection_index, name_of_underlying_layer) == NO){
        goto cleanup;
      }else{
        goto again;
      }
      break;

    case 2://advanced dlci configuration.
      {
        menu_frame_relay_advanced_dlci_configuration
                                  advanved_dlci_cfg(lxdialog_path, dlci_cfg);
        if(advanved_dlci_cfg.run(selection_index) == NO){
          rc = NO;
          goto cleanup;
        }else{
          goto again;
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
      text_box tb;

      Debug(DBG_MENU_FRAME_RELAY_DLCI_CONFIGURATION,
        ("HELP option selected: %s\n", get_lxdialog_output_string()));

      switch(atoi(get_lxdialog_output_string()))
      {
      case 1://set DLCI number
{
char* help_str =
"DLCI Number\n"
"-----------\n"
"\n"
"  DLCI number specifies a frame relay\n"
"  logical channel.  Frame Relay protocol multiplexes the\n"
"  physical line into number of channels, each one of\n"
"  these channels has a DLCI number associated with it.\n"
"\n"
"  Please enter the DLCI number as specified by your\n"
"  ISP.\n";
        tb.show_help_message(lxdialog_path, WANCONFIG_FR, help_str);
}
        break;

      case 2://advanced dlci configuration.
{
char* help_str =
"Advanced DLCI configuration\n"
"---------------------------\n"
"\n"
"  Help is available once you switch to the\n"
"  'Advanced DLCI configuration' dialog.\n";
        tb.show_help_message(lxdialog_path, WANCONFIG_FR, help_str);
}
        break;

      default:
{
char* help_str =
"Invalid Selection!!!\n"
"--------------------\n";
        tb.show_help_message(lxdialog_path, WANCONFIG_FR, help_str);
}

        break;
      }
      goto again;
    }
    break;

  case MENU_BOX_BUTTON_EXIT:
    //no check is done here because each menu item checks
    //the input separatly.
    rc = YES;
    break;
  }//switch(*selection_index)

cleanup:
  Debug(DBG_MENU_FRAME_RELAY_DLCI_CONFIGURATION,
    ("menu_frame_relay_DLCI_configuration::run(): rc: %d\n", rc));

  return rc;
}

int menu_frame_relay_DLCI_configuration::set_dlci_number( OUT int * selection_index,
                                                          IN char* name_of_underlying_layer)
{
  int new_dlci;
  int old_dlci;
  char tmp_addr[MAX_ADDR_STR_LEN];
  list_element_chan_def* dlci_next = NULL;
  list_element_chan_def* dlci_first = NULL;
  wan_fr_conf_t* fr_first;
  
  old_dlci = atoi(remove_spaces_in_int_string(dlci_cfg->data.addr));

  Debug(DBG_MENU_FRAME_RELAY_DLCI_CONFIGURATION, ("old_dlci: %d\n", old_dlci));

redisplay:

  //////////////////////////////////////////////////////////////////////
  //suggest DLCI number.
  snprintf(tmp_addr, MAX_ADDR_STR_LEN, "%s", dlci_cfg->data.addr);
  input_box_frame_relay_DLCI_number dlci_num(&new_dlci);
  if(dlci_num.show( lxdialog_path,
                    WANCONFIG_FR,
                    tmp_addr,
                    selection_index) == NO){
    goto cleanup;
  }

  if(old_dlci == new_dlci){
    //dlci number was not chaned, do nothing
    return YES;  
  }
  
  //////////////////////////////////////////////////////////////////////
  //In order to change DLCI number:
  //1. remove element from the list.
  //2. update the dlci number (and name) and insert it back into the list.
  //The list will take care of the order.

  //it is the same poiner as it was, but have to do it in order to remove.
  Debug(DBG_MENU_FRAME_RELAY_DLCI_CONFIGURATION, ("old dlci_cfg ptr: 0x%p\n",
    dlci_cfg));
  
  ////////////////////////////////////////////////////////////////////// 
  //on return we have the 'new_dlci' set to user provided value
  snprintf(tmp_addr, MAX_ADDR_STR_LEN, "%d", new_dlci);

  Debug(DBG_MENU_FRAME_RELAY_DLCI_CONFIGURATION,
    ("on return from 'dlci number input box': %s\n", tmp_addr));

  //make sure new dlci number is not already in the list
  if(obj_list->find_element(remove_spaces_in_int_string(tmp_addr)) != NULL){
    //it's already in dlci list. display error.
    Debug(DBG_MENU_FRAME_RELAY_DLCI_CONFIGURATION, ("new_dlci %d already in list!!\n",
      new_dlci));

    text_box tb;
    tb.show_error_message(lxdialog_path, WANCONFIG_FR,
                          "DLCI number %d is already in the list.\nNo duplicates allowed!",
                          new_dlci);
    goto redisplay;
  }
  
  /////////////////////////////////////////////////////////////////////
  //It is possible, the list will be reodered. The global FR configuration
  //is stored in the 1-st object in the list. So, propagade global FR cfg
  //from 1-st to all dlcis in the list.
  //
  dlci_first = (list_element_frame_relay_dlci*)obj_list->get_first();
  dlci_next = (list_element_frame_relay_dlci*)obj_list->get_next_element(dlci_first);
  
  fr_first = &dlci_first->data.chanconf->u.fr;
  
  while(dlci_next != NULL){
    //for new dlci use settings of the first one in the list.
    //this nesesary for 'menu_frame_relay_dlci_list' !!
    //it is using the 1-st dlci in the list!
    wan_fr_conf_t* fr_next;
 
    fr_next = &dlci_next->data.chanconf->u.fr;

    memcpy(fr_next, fr_first, sizeof(wan_fr_conf_t));
    
    dlci_next = (list_element_frame_relay_dlci*)obj_list->get_next_element(dlci_next);
  }

  /////////////////////////////////////////////////////////////////////
  dlci_cfg = (list_element_frame_relay_dlci*)
                //obj_list->remove_element(remove_spaces_in_int_string(dlci_cfg->data.addr));
                obj_list->remove_element(dlci_cfg->data.addr);

  Debug(DBG_MENU_FRAME_RELAY_DLCI_CONFIGURATION, ("new dlci_cfg ptr: 0x%08X\n",
    (unsigned int)dlci_cfg));

  if(dlci_cfg == NULL){
    ERR_DBG_OUT(("Failed to remove existing element from list!!\n"));
    goto cleanup;
  }
 
  //create name for the new interface. can be changed later by the user in Net Interface cfg.
#if defined(__LINUX__) && !BSD_DEBG
  snprintf(dlci_cfg->data.name, WAN_IFNAME_SZ, "%sf%d", name_of_underlying_layer, new_dlci);
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
  snprintf( dlci_cfg->data.name, WAN_IFNAME_SZ, "%sf%d",
	    //underlying layer name ends with digit - change it
	    replace_numeric_with_char(name_of_underlying_layer),
	    new_dlci);
#endif

  snprintf(dlci_cfg->data.addr, MAX_ADDR_STR_LEN, "%d", new_dlci);
  obj_list->insert(dlci_cfg);
  //Done

  return YES;
cleanup:
  return NO;
}
