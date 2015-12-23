/***************************************************************************
                          menu_aft_logical_channels_list.cpp  -  description
                             -------------------
    begin                : Wed Apr 7 2004
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

#include "menu_aft_logical_channels_list.h"
#include "wancfg.h"
#include "text_box.h"

#include "list_element_chan_def.h"
#include "input_box_number_of_logical_channels.h"
#include "menu_aft_logical_channel_cfg.h"


char* number_of_timeslot_groups =
"Number of Timeslot Groups on a T1 or E1 line.\n"
"Valid numbers:\n"
"for T1 - Minimum 1, Maximum 24\n"
"for E1 - Minimum 1, Maximum 31\n"
"Each group must use unique timeslots (DS0),\n"
"no overlapping allowed.\n"
"For example: 2 groups on T1 line may use timeslots\n"
"as follows: Group 1: 1-12, Group 2: 13-24\n";

char* per_group_configuration_options_help_str =
"Each Timeslot Group has following configuration options:\n"
"--------------------------------------------------------\n"
" -Configure Selected Protocol\n"
" -Change Protocol\n"
" -Set Timeslots for the Group\n";


enum AFT_LOGICAL_CHANNELS_SETUP{
  NUMBER_OF_LOGICAL_CHANNELS=0,//important to be 0!! because 'aft_channel_counter'
                               //in 'conf_file_reader.cpp' is initialized to 1.
  EMPTY_LINE=1024
};

#define DBG_MENU_AFT_LOGICAL_CHANNELS_LIST 1

menu_aft_logical_channels_list::
  menu_aft_logical_channels_list(IN char * lxdialog_path, IN conf_file_reader* cfr)
{
  Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST,
    ("menu_aft_logical_channels_list::menu_aft_logical_channels_list()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);

  this->cfr = cfr;
}

menu_aft_logical_channels_list::~menu_aft_logical_channels_list()
{
  Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST,
    ("menu_aft_logical_channels_list::~menu_aft_logical_channels_list()\n"));
}

int menu_aft_logical_channels_list::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  unsigned int new_number_of_groups_of_channels;
  int num_of_visible_items;
  list_element_chan_def* list_el_chan_def = NULL;
  sdla_fe_cfg_t* fe_cfg = &cfr->link_defs->linkconf->fe_cfg;

  link_defs = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  //help text box
  text_box tb;

  objects_list * obj_list = cfr->main_obj_list;

  Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, ("menu_new_device_configuration::run()\n"));

again:
  rc = YES;
  exit_dialog = NO;
  menu_str = " ";

  //adjust number of items in the menu depending on number of dlcis
  num_of_visible_items = cfr->main_obj_list->get_size() + 2;
  //8 items looks best
  if(num_of_visible_items > 8){
    num_of_visible_items = 8;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", NUMBER_OF_LOGICAL_CHANNELS);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Number of %s------> %d\" ",
    TIME_SLOT_GROUPS_STR,
    cfr->main_obj_list->get_size());
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  if(create_logical_channels_list_str(menu_str) == NO){
    return NO;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\n%s configuration for Wan Device: %s",TIME_SLOT_GROUPS_STR, cfr->link_defs->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "AFT SETUP",
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

  //////////////////////////////////////////////////////////////////////////////////////

  exit_dialog = NO;
  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case EMPTY_LINE:
      goto again;
      
    case NUMBER_OF_LOGICAL_CHANNELS:
      {
        input_box_number_of_logical_channels
          number_of_logical_channels( (int*)&new_number_of_groups_of_channels,
                                      fe_cfg->media);

        //convert int to string
        if(obj_list->get_size() == 0){
          //suggest 1 logical channel
          snprintf(tmp_buff, MAX_PATH_LENGTH, "%d", 1);
        }else{
          snprintf(tmp_buff, MAX_PATH_LENGTH, "%d", obj_list->get_size());
        }

        if(number_of_logical_channels.show( lxdialog_path,
                                            WANCONFIG_AFT,
                                            tmp_buff,//IN char * initial_text,
                                            selection_index) == NO){
          rc = NO;
          goto cleanup;
        }

        if(new_number_of_groups_of_channels != obj_list->get_size()){
		
          Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST,
            ("new_number_of_groups_of_channels: %d, old number: %d\n",
            new_number_of_groups_of_channels,
            obj_list->get_size()
            ));

          adjust_number_of_groups_of_channels(obj_list, new_number_of_groups_of_channels);
        }
        goto again;
      }
      break;

    default:
      //one of the Timeslot Groups was selected for configuration
      Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, ("default case selection: %d\n", *selection_index));

      list_el_chan_def = (list_element_chan_def*)
                            obj_list->find_element(
                              remove_spaces_in_int_string(get_lxdialog_output_string()));

      if(list_el_chan_def != NULL){

	menu_aft_logical_channel_cfg per_aft_logical_channel_cfg( lxdialog_path,
                                                                  cfr,
                                                                  list_el_chan_def);
        //list_el_chan_def = NULL;
        //do NOT use list_el_chan_def on return - it may be invalid!
        rc = per_aft_logical_channel_cfg.run(cfr->wanpipe_number, selection_index,
                                            obj_list->get_size());
        if(rc == YES){
          goto again;
        }
	
      }else{
        //failed to find the element
        ERR_DBG_OUT(("Failed to find the element!! selection: %s\n",
          get_lxdialog_output_string()));
        rc = NO;
        exit_dialog = YES;
      }
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    switch(atoi(get_lxdialog_output_string()))
    {
    case NUMBER_OF_LOGICAL_CHANNELS:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, number_of_timeslot_groups);
      break;

    default:
      tb.show_help_message( lxdialog_path, NO_PROTOCOL_NEEDED,
                            per_group_configuration_options_help_str);
      break;
    }
    break;

  case MENU_BOX_BUTTON_BACK:
    
    if(check_bound_channels_not_overlapping() == NO){
      exit_dialog = NO;
      break;
    }

    exit_dialog = YES;
    break;
  }//switch(*selection_index)

  if(exit_dialog == NO){
    goto again;
  }
cleanup:
  return rc;
}

int menu_aft_logical_channels_list::create_logical_channels_list_str(string& menu_str)
{
  char tmp_buff[MAX_PATH_LENGTH];
  sdla_fe_cfg_t* fe_cfg = &cfr->link_defs->linkconf->fe_cfg;

  if(link_defs->card_version == A200_ADPTR_ANALOG){
      max_valid_number_of_logical_channels = MAX_FXOFXS_CHANNELS;
  }else if(link_defs->card_version == AFT_ADPTR_ISDN){
      max_valid_number_of_logical_channels = MAX_BRI_TIMESLOTS;
  }else{
    if(fe_cfg->media == WAN_MEDIA_T1){

      max_valid_number_of_logical_channels = NUM_OF_T1_CHANNELS;
    }else if(fe_cfg->media == WAN_MEDIA_E1){

      max_valid_number_of_logical_channels = NUM_OF_E1_CHANNELS - 1;
    }else if(fe_cfg->media == WAN_MEDIA_NONE || fe_cfg->media == WAN_MEDIA_56K){
      //serial or 56k
      max_valid_number_of_logical_channels = 1;
    }else{
      ERR_DBG_OUT(("Invalid Media Type!! (0x%X)\n", fe_cfg->media));
      return NO;
    }
  }

  objects_list * obj_list = cfr->main_obj_list;
  list_element_chan_def* list_el_chan_def = NULL;

  if(obj_list->get_size() > 0 && obj_list->get_size() <= max_valid_number_of_logical_channels){

    list_el_chan_def = (list_element_chan_def*)obj_list->get_first();
    
    while(list_el_chan_def != NULL){

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", list_el_chan_def->data.addr);
      menu_str += tmp_buff;

      Debug(DBG_MENU_AFT_LOGICAL_CHANNELS_LIST, 
		      ("list_el_chan_def->data.name: %s\n", list_el_chan_def->data.name));

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s %s configuration-> %s\" ",
            TIME_SLOT_GROUP_STR, list_el_chan_def->data.addr,
            list_el_chan_def->data.active_channels_string);
      menu_str += tmp_buff;

      list_el_chan_def = (list_element_chan_def*)obj_list->get_next_element(list_el_chan_def);
    }
  }else{
    ERR_DBG_OUT(("Invalid number of %s: %d!!\n", TIME_SLOT_GROUPS_STR, obj_list->get_size()));
    return NO;
  }

  return YES;
}


int menu_aft_logical_channels_list::
  adjust_number_of_groups_of_channels(IN objects_list * obj_list,
                                      IN unsigned int new_number_of_groups_of_channels)
{
  sdla_fe_cfg_t* fe_cfg = &cfr->link_defs->linkconf->fe_cfg;
  text_box tb;
  
  if(link_defs->card_version == A200_ADPTR_ANALOG){
      max_valid_number_of_logical_channels = MAX_FXOFXS_CHANNELS;
  }else if(link_defs->card_version == AFT_ADPTR_ISDN){
      max_valid_number_of_logical_channels = MAX_BRI_TIMESLOTS;
  }else{
    if(fe_cfg->media == WAN_MEDIA_T1){

      max_valid_number_of_logical_channels = NUM_OF_T1_CHANNELS;
    }else if(fe_cfg->media == WAN_MEDIA_E1){

      max_valid_number_of_logical_channels = NUM_OF_E1_CHANNELS - 1;
    }else if(fe_cfg->media == WAN_MEDIA_NONE || fe_cfg->media == WAN_MEDIA_56K){
      //serial or 56k
      max_valid_number_of_logical_channels = 1;
    }else{
      ERR_DBG_OUT(("Invalid Media Type!! (0x%X)\n", fe_cfg->media));
      return NO;
    }
  }

  if(new_number_of_groups_of_channels > max_valid_number_of_logical_channels){
    tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED, 
	"Invalid number of groups of channels: %d! Maximum is: %d.\n",
		  new_number_of_groups_of_channels,
		  max_valid_number_of_logical_channels);
    return NO;
  }

  return adjust_number_of_logical_channels_in_list(WANCONFIG_AFT, obj_list,
		 cfr, new_number_of_groups_of_channels);
}

int menu_aft_logical_channels_list::check_bound_channels_not_overlapping()
{
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  //help text box
  text_box tb;
  unsigned long accumulated_channels_bits = 0;
  conf_file_reader* local_cfr = (conf_file_reader*)global_conf_file_reader_ptr;

  //for each logical channel:
  //1.  check that bit which is set in it's 'logical_ch_cfg->data.chanconf->active_ch' variable
  //    is NOT set in the 'accumulated_channels_bits'.
  //    If bit is set it is an error - overlapping channels.
  //    If bit is not set, set it to indicate that it is used.

  objects_list * obj_list = cfr->main_obj_list;
  list_element_chan_def* list_el_chan_def = (list_element_chan_def*)obj_list->get_first();

  while(list_el_chan_def != NULL){

    //convert channels string to number and check for overlapping timeslots
    list_el_chan_def->data.chanconf->active_ch = 
      parse_active_channel(list_el_chan_def->data.active_channels_string,
		     local_cfr->link_defs->linkconf->fe_cfg.media);	
    
    if(accumulated_channels_bits & list_el_chan_def->data.chanconf->active_ch){

      snprintf(tmp_buff, MAX_PATH_LENGTH, "Error: Overlapping Time Slots!");
      tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED, tmp_buff);
      rc = NO;
      break;
    }else{
      accumulated_channels_bits |= list_el_chan_def->data.chanconf->active_ch;
    }

    list_el_chan_def = (list_element_chan_def*)obj_list->get_next_element(list_el_chan_def);
  }

  return rc;
}
