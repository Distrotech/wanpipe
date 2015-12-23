/***************************************************************************
                          menu_frame_relay_advanced_global_configuration.cpp  -  description
                             -------------------
    begin                : Tue Mar 23 2004
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

#include "menu_frame_relay_advanced_global_configuration.h"
#include "wancfg.h"
#include "text_box_help.h"
#include "text_box_yes_no.h"
#include "input_box.h"

#define DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION  1

char* fr_station_type =
"Station Type\n"
"------------\n"
"This parameter specifies whether the  adapter\n"
"should operate as a Customer  Premises Equipment (CPE)\n"
"or emulate a  frame relay switch (Access Node).\n"
"\n"
"Available options are:\n"
"CPE  mode (default)\n"
"Node (switch emulation mode)\n"
"\n"
"Default: CPE\n";

char* fr_t391_help_str =
"T391 TIMER\n"
"----------\n"
"This is the Link Integrity Verification Timer value in\n"
"seconds. It should be within a range from 5 to 30 and\n"
"is relevant only if adapter is configured as CPE.\n";

char* fr_t392_help_str =
"T392 TIMER\n"
"----------\n"
"This is the Polling Verification Timer value in seconds.\n"
"It should be within a range from 5 to 30 and is relevant\n"
"only if adapter is configured as Access Node.\n";

char* fr_n391_help_str =
"N391 Counter\n"
"------------\n"
"This is the Full Status Polling Cycle Counter. Its\n"
"value should be within a range from 1 to 255 and is\n"
"relevant only if adapter is configured as CPE.\n";

char* fr_n392_help_str =
"N392 Counter\n"
"------------\n"
"This is the Error Threshold Counter. Its value should\n"
"be within a range from 1 to 10 and is relevant for both\n"
"CPE and Access Node configurations.\n";

char* fr_n393_help_str =
"N393 Counter\n"
"------------\n"
"This is the Monitored Events Counter. Its value should\n"
"be within a range from 1 to 10 and is relevant for both\n"
"CPE and Access Node configurations.\n";

char* fr_fast_connect_help_str =
"Fast Connect\n"
"------------\n"
"ISSUE A FULL STATUS REQUEST ON STARTUP\n"
"\n"
"This option is used to speed up frame\n"
"relay connection.  In theory, frame relay takes\n"
"up to a minute to connect, because a full status\n"
"request timer takes that long to timeout.\n"
"\n"
"This option, forces a full status request as soon\n"
"as the protocol is started, thus a line would come\n"
"up faster.\n"
"\n"
"In some VERY rare cases, this causes problems with\n"
"some frame relay switches.  In such situations this\n"
"option should be disabled.\n"
"\n"
"Available options are:\n"
"\n"
"YES : Enable Full Status request on startup\n"
"NO  : Disable Full Status request on startup\n"
"\n"
"Default: YES";

menu_frame_relay_advanced_global_configuration::
  menu_frame_relay_advanced_global_configuration( IN char * lxdialog_path,
                                                  IN list_element_chan_def* parent_list_element_logical_ch)
{
  Debug(DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION,
    ("menu_frame_relay_advanced_global_configuration::\
menu_frame_relay_advanced_global_configuration()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);

  this->parent_list_element_logical_ch = parent_list_element_logical_ch;
}

menu_frame_relay_advanced_global_configuration::
  ~menu_frame_relay_advanced_global_configuration()
{
  Debug(DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION,
    ("menu_frame_relay_advanced_global_configuration::\
~menu_frame_relay_advanced_global_configuration()\n"));
}

int menu_frame_relay_advanced_global_configuration::run(OUT int * selection_index)
{
  string menu_str;
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  //unsigned int option_selected = 0;
  char exit_dialog = NO;
  text_box tb;
  list_element_chan_def* tmp_dlci_ptr = NULL;
  objects_list* obj_list;

  int t391, t392, n391, n392, n393;

  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s Setup", get_protocol_string(WANCONFIG_FR));

  Debug(DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION,
    ("menu_new_device_configuration::run()\n"));
  
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

  Debug(DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION, ("number_of_dlcis: %d\n", obj_list->get_size()));
  Debug(DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION, ("fr->dlci_num: %d\n", fr->dlci_num));

again:
  //Advanced fr cfg options:

  /////////////////////////////////////////////////
  //insert "1" tag for "station" into the menu string
  menu_str = " \"1\" ";
  //create the menu item
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Station------> %s\" ",
    (fr->station == WANOPT_NODE ? "Node (switch emulation)" : "CPE"));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  menu_str += "\"2\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"T391 timer---> %u\" ", fr->t391);
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  menu_str += "\"3\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"T392 timer---> %u\" ", fr->t392);
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  menu_str += "\"4\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"N391 counter-> %u\" ", fr->n391);
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  menu_str += "\"5\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"N392 counter-> %u\" ", fr->n392);
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  menu_str += "\"6\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"N393 counter-> %u\" ", fr->n393);
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  menu_str += "\"7\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Fast Connect-> %s\" ",
    (fr->issue_fs_on_startup == 1 ? "YES" : "NO"));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\n\nFrame Relay Global configuration options.\
\nFor device %s.", parent_list_element_logical_ch->data.name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "FRAME RELAY GLOBAL CONFIGURATION",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          7,//number of items in the menu, including the empty lines
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
    Debug(DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1:
      if(fr->station == WANOPT_NODE){
        //current setting is NODE.
        snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to change Station type to 'CPE'? ");

        if(yes_no_question(   selection_index,
                              lxdialog_path,
                              WANCONFIG_FR,
                              tmp_buff) == NO){
          return NO;
        }

        switch(*selection_index)
        {
        case YES_NO_TEXT_BOX_BUTTON_YES:
          fr->station = WANOPT_CPE;
          break;
        }
        goto again;

      }else{
        //current setting is CPE.
        snprintf(tmp_buff, MAX_PATH_LENGTH,
          "Do you want to change Station type to\n'Node (switch emulation)'? ");

        if(strcmp((char*)fr->dlci, AUTO_CHAN_CFG) == 0){
          tb.show_error_message(lxdialog_path, WANCONFIG_FR,
"DLCI List configuration is set to 'Auto'.\n"
"Can not change Station type to 'Node (switch emulation)'.\n"
);
          goto again;
        }

        if(yes_no_question(   selection_index,
                              lxdialog_path,
                              WANCONFIG_FR,
                              tmp_buff) == NO){
          return NO;
        }

        switch(*selection_index)
        {
        case YES_NO_TEXT_BOX_BUTTON_YES:
          fr->station = WANOPT_NODE;
	  fr->signalling = WANOPT_FR_ANSI;
          break;
        }
        goto again;
      }
      break;

    case 2:
      //fr->t391
      /////////////////////////////////////////////////////////////
show_t391_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify the T391 timer (default 10)");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", fr->t391);

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
        Debug(DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION,
          ("T391 on return: %s\n", inb.get_lxdialog_output_string()));

        t391 = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(t391 < 5 || t391 > 30){

          tb.show_error_message(lxdialog_path, WANCONFIG_FR, "Invalid T391!\n\n%s\n", fr_t391_help_str);
          goto show_t391_input_box;
        }else{
          fr->t391 = t391;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_t391_help_str);
        goto show_t391_input_box;
        break;

      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case 3:
      //fr->t392
      /////////////////////////////////////////////////////////////
show_t392_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, " Please specify the T392 timer (default 16)");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", fr->t392);

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
        Debug(DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION,
          ("T392 on return: %s\n", inb.get_lxdialog_output_string()));

        t392 = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(t392 < 5 || t392 > 30){

          tb.show_error_message(lxdialog_path, WANCONFIG_FR, "Invalid T392!\n\n%s\n",
            fr_t392_help_str);
          goto show_t392_input_box;
        }else{
          fr->t392 = t392;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_t392_help_str);
        goto show_t392_input_box;
        break;

      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case 4:
      //fr->n391
      /////////////////////////////////////////////////////////////
show_n391_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify the N391 timer (default 2)");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", fr->n391);

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
        Debug(DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION,
          ("N391 on return: %s\n", inb.get_lxdialog_output_string()));

        n391 = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(n391 < 1 || n391 > 255){

          tb.show_error_message(lxdialog_path, WANCONFIG_FR, "Invalid N391!\n\n%s\n",
            fr_n391_help_str);
          goto show_n391_input_box;
        }else{
          fr->n391 = n391;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_n391_help_str);
        goto show_n391_input_box;
        break;

      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case 5:
      //fr->n392
      /////////////////////////////////////////////////////////////
show_n392_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify the N392 timer (default 3)");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", fr->n392);

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
        Debug(DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION,
          ("N392 on return: %s\n", inb.get_lxdialog_output_string()));

        n392 = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(n392 < 1 || n392 > 10){

          tb.show_error_message(lxdialog_path, WANCONFIG_FR, "Invalid N392!\n\n%s\n",
            fr_n392_help_str);
          goto show_n392_input_box;
        }else{
          fr->n392 = n392;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_n392_help_str);
        goto show_n392_input_box;
        break;

      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case 6:
      //fr->n393
      /////////////////////////////////////////////////////////////
show_n393_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, " Please specify the N393 timer (default 4)");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", fr->n393);

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
        Debug(DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION,
          ("N393 on return: %s\n", inb.get_lxdialog_output_string()));

        n393 = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(n393 < 1 || n393 > 10){

          tb.show_error_message(lxdialog_path, WANCONFIG_FR, "Invalid N393!\n\n%s\n",
            fr_n393_help_str);
          goto show_n393_input_box;
        }else{
          fr->n393 = n393;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_n393_help_str);
        goto show_n393_input_box;
        break;

      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case 7:
      //fr->issue_fs_on_startup
      if(fr->issue_fs_on_startup == 0){
        //current setting is NO.
        snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to enable 'Fast Connect'? ");

        if(yes_no_question(   selection_index,
                              lxdialog_path,
                              WANCONFIG_FR,
                              tmp_buff) == NO){
          return NO;
        }

        switch(*selection_index)
        {
        case YES_NO_TEXT_BOX_BUTTON_YES:
          fr->issue_fs_on_startup = 1;
          break;
        }
        goto again;

      }else{
        //current setting is YES.
        snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to disable 'Fast Connect'? ");

        if(yes_no_question(   selection_index,
                              lxdialog_path,
                              WANCONFIG_FR,
                              tmp_buff) == NO){
          return NO;
        }

        switch(*selection_index)
        {
        case YES_NO_TEXT_BOX_BUTTON_YES:
          fr->issue_fs_on_startup = 0;
          break;
        }
        goto again;
      }
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
    }

    if(rc == NO){
      goto cleanup;
    }else{
      goto again;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    Debug(DBG_MENU_FRAME_RELAY_ADVANCED_GLOBAL_CONFIGURATION,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1:
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_station_type);
      break;
    case 2:
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_t391_help_str);
      break;
    case 3:
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_t392_help_str);
      break;
    case 4:
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_n391_help_str);
      break;
    case 5:
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_n392_help_str);
      break;
    case 6:
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_n393_help_str);
      break;
    case 7:
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_fast_connect_help_str);
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

