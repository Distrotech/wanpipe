/***************************************************************************
                          menu_frame_relay_arp.cpp  -  description
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

#include "menu_frame_relay_arp.h"
#include "text_box.h"
#include "input_box.h"

char* fr_tx_inverse_arp_help =
"INVERSE ARP INTERVAL\n"
"\n"
"This parameter sets the time interval in seconds\n"
"between Inverse ARP Request.\n"
"\n"
"Default: 10 sec\n";

#define DBG_MENU_FRAME_RELAY_ARP 1

menu_frame_relay_arp::menu_frame_relay_arp( IN char * lxdialog_path,
                                            IN list_element_frame_relay_dlci* dlci_cfg)
{
  Debug(DBG_MENU_FRAME_RELAY_ARP,
    ("menu_frame_relay_arp::menu_frame_relay_arp()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->dlci_cfg = dlci_cfg;
}

menu_frame_relay_arp::~menu_frame_relay_arp()
{
  Debug(DBG_MENU_FRAME_RELAY_ARP,
    ("menu_frame_relay_arp::~menu_frame_relay_arp()\n"));
}

int menu_frame_relay_arp::run(OUT int * selection_index)
{
  string menu_str;
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  int num_of_visible_items;
  int inarp_interval;

  //help text box
  text_box tb;

  //input box for TX ARP Interval input
  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s Setup", get_protocol_string(WANCONFIG_FR));

  Debug(DBG_MENU_FRAME_RELAY_ARP,
    ("menu_frame_relay_arp::run()\n"));

  if(dlci_cfg ==  NULL){
    ERR_DBG_OUT(("DLCI configuration pointer is NULL (dlci_cfg) !!!\n"));
    rc = NO;
    goto cleanup;
  }

again:
  snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify Inverse ARP interval");
  snprintf(initial_text, MAX_PATH_LENGTH, "%d", dlci_cfg->data.chanconf->inarp_interval);

  inb.set_configuration(  lxdialog_path,
                          backtitle,
                          explanation_text,
                          INPUT_BOX_HIGTH,
                          INPUT_BOX_WIDTH,
                          initial_text);

  menu_str = "";
  if(dlci_cfg->data.chanconf->inarp_interval){
    menu_str += " \"1\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Disable TX Inverse ARP\" ");
    menu_str += tmp_buff;

    menu_str += " \"2\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Set TX Inverse ARP Interval\" ");
    menu_str += tmp_buff;

  }else{
    menu_str += " \"3\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Enable TX Inverse ARP\" ");
    menu_str += tmp_buff;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\n"
"Disable/Enable TX Inverse ARP. If enabled,\n"
"Set TX Inverse ARP Interval.\n"
"\n"
"Use <Back> when configuration is complete.\n");

  num_of_visible_items = 2;

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "DISABLE/ENABLE INVERSE ARP. SET ARP INTERVAL.",
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
    Debug(DBG_MENU_FRAME_RELAY_ARP,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1://Disable Inv ARP
      dlci_cfg->data.chanconf->inarp = 0;
      dlci_cfg->data.chanconf->inarp_interval = 0;
      break;

    case 2://Set ARP Interval
show_input_box:
      inb.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_FRAME_RELAY_ARP,
          ("ARP interval on return: %s\n", inb.get_lxdialog_output_string()));

        inarp_interval = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(inarp_interval == 0){
            tb.show_help_message(lxdialog_path, WANCONFIG_FR, "Invalid Input!\n%s",
              fr_tx_inverse_arp_help);
          goto show_input_box;
        }
        dlci_cfg->data.chanconf->inarp_interval = inarp_interval;
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_tx_inverse_arp_help);
        goto show_input_box;
        break;
      }//switch(*selection_index)
      break;

    case 3://Enable  Inv ARP
      dlci_cfg->data.chanconf->inarp = 1;
      dlci_cfg->data.chanconf->inarp_interval = 10;//seconds - default
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      goto cleanup;
    }
    goto again;
    break;

  case MENU_BOX_BUTTON_HELP:
    Debug(DBG_MENU_FRAME_RELAY_ARP,
      ("HELP option selected: %s\n", get_lxdialog_output_string()));

    tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_tx_inverse_arp_help);

    goto again;
    break;

  case MENU_BOX_BUTTON_EXIT:
    //no check is done here because each menu item checks
    //the input separatly.
    break;
  }//switch(*selection_index)

cleanup:
  return rc;
}
