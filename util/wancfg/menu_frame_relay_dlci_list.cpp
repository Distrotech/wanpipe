/***************************************************************************
                          menu_frame_relay_dlci_list.cpp  -  description
                             -------------------
    begin                : Mon Mar 15 2004
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

#include "menu_frame_relay_dlci_list.h"
#include "input_box_number_of_dlcis.h"
#include "list_element_frame_relay_dlci.h"
#include "menu_frame_relay_dlci_configuration.h"


enum DLCI_LIST_OPTIONS{
  NUMBER_OF_DLCIS=2000	//number large enough not to interfier with dlci number
};

char* fr_number_of_dlcis_help_str =
"Number of DLCIs supported\n"
"-------------------------\n"
"\n"
"Number of DLCIs supported by this Frame\n"
"Relay connection.\n"
"\n"
"For each DLCI, a network interface will be\n"
"created.\n"
"\n"
"Each network interface must be configured with\n"
"an IP address.\n";

char* fr_per_dlci_cfg_options_help_str =
"Per-DLCI configuration options\n"
"------------------------------\n"
" -DLCI Number\n"
" -Advanced options: CIR, BC, BE, TX Inv. ARP,\n"
"  RX Inv. ARP\n";


#define DBG_MENU_FRAME_RELAY_DLCI_LIST  1

#define DLCI_NUM_NOT_SET 1

int check_global_frame_relay_configuration(wan_fr_conf_t* fr_global_cfg);

menu_frame_relay_dlci_list::
      menu_frame_relay_dlci_list( IN char * lxdialog_path,
				  IN list_element_chan_def* parent_element_logical_ch)
{
  Debug(DBG_MENU_FRAME_RELAY_DLCI_LIST,
    ("menu_frame_relay_dlci_list::menu_frame_relay_dlci_list()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);

  this->parent_list_element_logical_ch = parent_element_logical_ch;

  name_of_parent_layer = parent_list_element_logical_ch->data.name;
}

menu_frame_relay_dlci_list::~menu_frame_relay_dlci_list()
{
  Debug(DBG_MENU_FRAME_RELAY_DLCI_LIST,
    ("menu_frame_relay_dlci_list::~menu_frame_relay_dlci_list()\n"));
}

int menu_frame_relay_dlci_list::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  int num_of_visible_items;

  //help text box
  text_box tb;

  list_element_chan_def*	  tmp_dlci_ptr = NULL;
  list_element_frame_relay_dlci*  local_list_el_chan_def = NULL;
  objects_list*			  obj_list = NULL;
  int is_auto_dlci = NO;
  fr_config_info_t fr_config_info;

  Debug(DBG_MENU_FRAME_RELAY_DLCI_LIST, ("menu_frame_relay_dlci_list::run()\n"));

again:
  menu_str = " ";
  rc = YES;
  exit_dialog = NO;
  
  obj_list = (objects_list *)parent_list_element_logical_ch->next_objects_list;

  if(obj_list ==  NULL){
    ERR_DBG_OUT(("Channels list pointer is NULL (obj_list) !!!\n"));
    rc = NO;
    goto cleanup;
  }

  tmp_dlci_ptr = (list_element_chan_def*)obj_list->get_first();
  if(tmp_dlci_ptr == NULL){
    ERR_DBG_OUT(("Invalid 'dlci' pointer!!\n"));
    return NO;
  }
  
  fr = &tmp_dlci_ptr->data.chanconf->u.fr;
  
  if(fr->dlci_num == 1){
    //only one DLCI - there is possibility of 'auto dlci'
    Debug(DBG_MENU_FRAME_RELAY_DLCI_LIST, ("tmp_dlci_ptr->data.name: %s\n",
		  tmp_dlci_ptr->data.name));

    Debug(DBG_MENU_FRAME_RELAY_DLCI_LIST, ("tmp_dlci_ptr->data.addr: %s.\n",
			     tmp_dlci_ptr->data.addr));
    
    //check 'addr' is '1' - a must for 'auto'
    if(strcmp(tmp_dlci_ptr->data.addr, "1") != 0){
      Debug(DBG_MENU_FRAME_RELAY_DLCI_LIST, ("address: %s - NOT 'auto dlci'.\n",
			     tmp_dlci_ptr->data.addr));
      //not an auto dlci
      is_auto_dlci = NO;
    }else{
      //auto dlci
      is_auto_dlci = YES;
    }
  }else{
    //more than one dlci
    is_auto_dlci = NO;
  }

  fr_config_info.name_of_parent_layer = parent_list_element_logical_ch->data.name;
  fr_config_info.auto_dlci = is_auto_dlci;

  //always run this. if number of dlcis was not changed this call
  //does not do anything.
  if(adjust_number_of_logical_channels_in_list(	WANCONFIG_MFR,
						obj_list,
					       	&fr_config_info,
					       	fr->dlci_num) == NO){
    rc = NO;
    goto cleanup;
  }

  menu_str = " ";
  if(is_auto_dlci == YES){
    //for the 'auto' dlci go directly to the 'dlci configuration page'.
    menu_frame_relay_DLCI_configuration per_dlci_cfg(
                          lxdialog_path,
                          obj_list,
                          (list_element_frame_relay_dlci*)tmp_dlci_ptr);
      
    rc = per_dlci_cfg.run(selection_index, name_of_parent_layer);
    if(rc == YES){
      return YES;
    }else{
      return NO;
    }
  }else{
    //non-auto case
    if(create_logical_channels_list_str(menu_str) == NO){
      return NO;
    }
  }//if()

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\
\nSelect DLCI you want to configure.");

  //adjust number of items in the menu depending on number of dlcis
  num_of_visible_items = fr->dlci_num + 1;
  //8 items looks best
  if(num_of_visible_items > 8){
    num_of_visible_items = 8;
  }

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "FRAME RELAY DLCI LIST",
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
    Debug(DBG_MENU_FRAME_RELAY_DLCI_LIST,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case NUMBER_OF_DLCIS: //number of DLCIs if manual config.
                          //ask for number of DLCIs
      {
        input_box_number_of_dlcis ib_num_of_dlcis((int*)&fr->dlci_num);

        //convert int to string
        snprintf(tmp_buff, MAX_PATH_LENGTH, "%d", fr->dlci_num);
        
        ib_num_of_dlcis.show(  lxdialog_path,
                                WANCONFIG_FR,
                                tmp_buff,//IN char * initial_text,
                                selection_index);
        goto again;
      }
      break;

    default:
      //one of the DLCIs was selected for configuration
      Debug(DBG_MENU_FRAME_RELAY_DLCI_LIST, ("default case selection: %d\n", *selection_index));

      local_list_el_chan_def =
	(list_element_frame_relay_dlci*)obj_list->find_element(
                              remove_spaces_in_int_string(get_lxdialog_output_string()));

      if(local_list_el_chan_def != NULL){

        menu_frame_relay_DLCI_configuration per_dlci_cfg( lxdialog_path,
                                                          obj_list,
                                                          local_list_el_chan_def);
        //list_el_chan_def = NULL;
        //do NOT use list_el_chan_def on return - it may be invalid!
        rc = per_dlci_cfg.run(selection_index, name_of_parent_layer);
        if(rc == YES){
          goto again;
        }
      }else{
	ERR_DBG_OUT(("Failed to find dlci in the list! (%s)\n", get_lxdialog_output_string()));
	rc = NO;
      }
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    switch(atoi(get_lxdialog_output_string()))
    {
    case NUMBER_OF_DLCIS:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, fr_number_of_dlcis_help_str);
      break;

    default:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, fr_per_dlci_cfg_options_help_str);
    }
    goto again;
    break;

  case MENU_BOX_BUTTON_EXIT:

    fr->dlci_num = obj_list->get_size();

    //check configuration in the temporary structure.
    //if passed the check save it in the real structure.
    if(check_global_frame_relay_configuration(fr) != 0){

      //if failed, prompt user that "Configuration is invalid or incomplete..." and
      //redisplay the dialog.
      text_box tb;
      tb.show_error_message(lxdialog_path, WANCONFIG_FR,
                            "Configuration is invalid or incomplete..");
      goto again;
    }

    exit_dialog = YES;
    break;
  }//switch(*selection_index)

cleanup:
  return rc;
}

int check_global_frame_relay_configuration(wan_fr_conf_t* fr_global_cfg)
{
  if(fr_global_cfg->dlci_num < 1 || fr_global_cfg->dlci_num > MAX_DLCI_NUMBER){
    Debug(DBG_MENU_FRAME_RELAY_DLCI_LIST, ("invalid number of DLCIs: %d\n", fr_global_cfg->dlci_num));
    return 1;
  }

  return 0;
}

int menu_frame_relay_dlci_list::create_logical_channels_list_str(string& menu_str)
{
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int max_valid_number_of_logical_channels;

  max_valid_number_of_logical_channels = MAX_DLCI_NUMBER;

  objects_list * obj_list = (objects_list *)parent_list_element_logical_ch->next_objects_list;
  list_element_frame_relay_dlci* list_el_chan_def = NULL;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", NUMBER_OF_DLCIS);
  menu_str += tmp_buff;
  if(obj_list->get_size() > 0 && obj_list->get_size() <= max_valid_number_of_logical_channels){

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Number of DLCI(s) ---> %u\" ", obj_list->get_size());
    menu_str += tmp_buff;

    list_el_chan_def = (list_element_frame_relay_dlci*)obj_list->get_first();
    //int i = 0;
    while(list_el_chan_def != NULL){

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", list_el_chan_def->data.addr);
      menu_str += tmp_buff;

      Debug(DBG_MENU_FRAME_RELAY_DLCI_LIST,
		     ("list_el_chan_def->data.name: %s\n", list_el_chan_def->data.name));

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"DLCI %s\" ", list_el_chan_def->data.addr);
      menu_str += tmp_buff;

      list_el_chan_def =
        (list_element_frame_relay_dlci*)obj_list->get_next_element(list_el_chan_def);
    }

  }else if(obj_list->get_size() == 0){

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Number of DLCI(s) ---> Undefined\" ");
      menu_str += tmp_buff;

  }else{

    ERR_DBG_OUT(("Invalid number of DLCIs: %d!!\n", obj_list->get_size()));
    return NO;
  }

  return YES;
}


