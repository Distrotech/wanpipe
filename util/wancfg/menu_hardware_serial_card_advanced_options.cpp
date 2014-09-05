/***************************************************************************
                          menu_hardware_serial_card_advanced_options.cpp  -  description
                             -------------------
    begin                : Thu Apr 1 2004
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

#include "menu_hardware_serial_card_advanced_options.h"
#include "text_box.h"
#include "input_box.h"
#include "menu_hardware_select_serial_clock_source.h"

enum SERIAL_ADVANCED_OPTIONS{
  CLOCK_SOURCE,
  BAUD_RATE
};

#define DBG_MENU_HARDWARE_SERIAL_CARD_ADVANCED_OPTIONS 1

menu_hardware_serial_card_advanced_options::
	menu_hardware_serial_card_advanced_options( IN char * lxdialog_path,
                                              IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_HARDWARE_SERIAL_CARD_ADVANCED_OPTIONS,
    ("menu_hardware_setup::menu_hardware_setup()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_hardware_serial_card_advanced_options::~menu_hardware_serial_card_advanced_options()
{
  Debug(DBG_MENU_HARDWARE_SERIAL_CARD_ADVANCED_OPTIONS,
    ("menu_hardware_setup::~menu_hardware_setup()\n"));
}

int menu_hardware_serial_card_advanced_options::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int option_selected;
  char exit_dialog;
  int number_of_items;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;

  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility");

  Debug(DBG_MENU_HARDWARE_SERIAL_CARD_ADVANCED_OPTIONS, ("menu_net_interface_setup::run()\n"));

again:
  menu_str = "";
  number_of_items = 1;
  rc = YES;
  option_selected = 0;
  exit_dialog = NO;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_HARDWARE_SERIAL_CARD_ADVANCED_OPTIONS,
    ("cfr->link_defs->name: %s\n", link_def->name));

  /////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CLOCK_SOURCE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Clock Source---> %s\" ",
    (linkconf->clocking == 0 ? "External" : "Internal"));
  menu_str += tmp_buff;
  /////////////////////////////////////////////////////////////////
  if(linkconf->clocking == 1){

    number_of_items++;
    //internal clock - display baud rate option
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", BAUD_RATE);
    menu_str += tmp_buff;

    if(linkconf->bps != 0){
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Baud Rate------> %d\" ",
        linkconf->bps);
    }else{
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Baud Rate------> Undefined!\" ");
    }
    menu_str += tmp_buff;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nAdvanced Hardware settings for: %s.", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SERIAL CARD ADVANCED HARDWARE OPTIONS",
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
  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:

    Debug(DBG_MENU_HARDWARE_SERIAL_CARD_ADVANCED_OPTIONS,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    Debug(DBG_MENU_HARDWARE_SERIAL_CARD_ADVANCED_OPTIONS,
      ("comms_port: atoi(get_lxdialog_output_string(): %d\n", atoi(get_lxdialog_output_string())));

    switch(atoi(get_lxdialog_output_string()))
    {
    case CLOCK_SOURCE:
      {
        menu_hardware_select_serial_clock_source select_serial_clock_source(lxdialog_path, cfr);
        if(select_serial_clock_source.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case BAUD_RATE:
      long baudrate;
      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify Baud Rate (non-zero)");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", linkconf->bps);

show_baudrate_input_box:
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
        Debug(DBG_MENU_HARDWARE_SERIAL_CARD_ADVANCED_OPTIONS,
          ("baudrate on return: %s\n", inb.get_lxdialog_output_string()));

        baudrate = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(baudrate < 1){
          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
                                "Invalid Baude Rate. Must be non-zero value.");
          goto show_baudrate_input_box;
        }else{
          linkconf->bps = baudrate;
          exit_dialog = YES;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:

        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          "Enter Baude Rate. Must be non-zero value.");
        goto show_baudrate_input_box;
        break;

      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
    break;

  case MENU_BOX_BUTTON_EXIT:
    if(linkconf->clocking == 1){

      if(linkconf->bps < 1){
        tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          "Clock Source is Internal. Baud Rate must be set\nto non zero value.");
      }else{
        exit_dialog = YES;
      }
    }else{
      exit_dialog = YES;
    }
    break;
  }//switch(*selection_index)

  if(exit_dialog == NO){
    goto again;
  }

cleanup:
  return rc;
}
