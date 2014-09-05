/***************************************************************************
                          menu_frame_relay_advanced_dlci_configuration.cpp  -  description
                             -------------------
    begin                : Mon Mar 22 2004
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

#include "menu_frame_relay_advanced_dlci_configuration.h"
#include "text_box.h"
#include "text_box_yes_no.h"
#include "input_box.h"

#include "menu_frame_relay_cir_configuration.h"
#include "menu_frame_relay_arp.h"

#define DBG_FRAME_RELAY_ADVANCED_DLCI_CONFIGURATION 1

char* bc_help_str =
"Frame Relay: Committed Burst Size (BC)\n"
"--------------------------------------\n"
"\n"
"This parameter states the Committed Burst Size.\n"
"\n"
"Valid values are 1 - 512 kbits.\n"
"\n"
"NOTE: For wanpipe drivers 2.1.X and greater:\n"
"If CIR is enabled, the driver will set BC to the\n"
"CIR value.\n";

char* be_help_str =
"Frame Relay: Excess Burst Size (BE)\n"
"-----------------------------------\n"
"This parameter states the Excess Burst Size.\n"
"\n"
"Options are:  0 - 512 kbits.\n"
"\n"
"Default: 0";

extern char* fr_tx_inverse_arp_help;

char* fr_icoming_inv_arp_help =
"DISABLE or ENABLE INCOMING INVERSE ARP SUPPORT\n"
"\n"
"This parameter enables or disables support\n"
"for incoming Inverse Arp Request packets. If the\n"
"Inverse ARP transmission is enabled this option will be\n"
"enabled by default.  However, if Inverse Arp transmission\n"
"is disabled, this option can be used to ignore\n"
"any incoming in-arp requests.\n"
"\n"
"The arp requests put considerable load on the\n"
"protocol, furthermore, illegal incoming arp requests\n"
"can potentially cause denial of service, since each\n"
"pakcet must be handled by the driver.\n"
"\n"
"Default: NO\n";

menu_frame_relay_advanced_dlci_configuration::
  menu_frame_relay_advanced_dlci_configuration( IN char * lxdialog_path,
                                                IN list_element_frame_relay_dlci* dlci_cfg)
{
  Debug(DBG_FRAME_RELAY_ADVANCED_DLCI_CONFIGURATION,
    ("menu_frame_relay_advanced_dlci_configuration::menu_frame_relay_advanced_dlci_configuration()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->dlci_cfg = dlci_cfg;
}

menu_frame_relay_advanced_dlci_configuration::~menu_frame_relay_advanced_dlci_configuration()
{
  Debug(DBG_FRAME_RELAY_ADVANCED_DLCI_CONFIGURATION,
    ("menu_frame_relay_advanced_dlci_configuration::~menu_frame_relay_advanced_dlci_configuration()\n"));
}

int menu_frame_relay_advanced_dlci_configuration::run(OUT int * selection_index)
{
  string menu_str;
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  int num_of_visible_items;
  int bc=0, be=0;

  //help text box
  text_box tb;

  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s Setup", get_protocol_string(WANCONFIG_FR));

  Debug(DBG_FRAME_RELAY_ADVANCED_DLCI_CONFIGURATION,
    ("menu_frame_relay_advanced_dlci_configuratio::run()\n"));

again:

  if(dlci_cfg ==  NULL){
    ERR_DBG_OUT(("DLCI configuration pointer is NULL (dlci_cfg) !!!\n"));
    rc = NO;
    goto cleanup;
  }

  //initialize the menu_str - very important!
  menu_str  = "";

  if(dlci_cfg->data.chanconf->cir == 0){

    menu_str  = " \"1\" ";
    //snprintf(tmp_buff, MAX_PATH_LENGTH, " \"CIR Disabled.\" ");
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"CIR-----------------------------> Disabled\" ");
    menu_str += tmp_buff;
  }else{
    menu_str  = " \"1\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"CIR Enabled. Value--------------> %d\" ",
      dlci_cfg->data.chanconf->cir);
    menu_str += tmp_buff;

    ////////////////////////////////////////////////////////////////////
    if(dlci_cfg->data.chanconf->bc == 0){
      //set default value
      dlci_cfg->data.chanconf->bc = 64;
    }
    menu_str += " \"2\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"BC------------------------------> %d\" ",
      dlci_cfg->data.chanconf->bc);
    menu_str += tmp_buff;

    ////////////////////////////////////////////////////////////////////
    menu_str += " \"3\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"BE------------------------------> %d\" ",
      dlci_cfg->data.chanconf->be);
    menu_str += tmp_buff;

  }//if(dlci_cfg->data.chanconf->cir > 0)

  if(dlci_cfg->data.chanconf->inarp == 0){

    menu_str += " \"4\" ";
    //Send Inverse ARP requests Y/N
    //snprintf(tmp_buff, MAX_PATH_LENGTH, " \"TX Inverse ARP Disabled\" ");
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"TX Inv. ARP---------------------> Disabled\" ");
    menu_str += tmp_buff;

  }else{

    menu_str += " \"4\" ";
    //sec, between InARP requests
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"TX Inv. ARP Enabled. Iterval----> %d\" ",
      dlci_cfg->data.chanconf->inarp_interval);
    menu_str += tmp_buff;
  }

  menu_str += " \"5\" ";
  //Receive Inverse ARP requests Y/N
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"RX Inv. ARP---------------------> %s\" ",
    (dlci_cfg->data.chanconf->inarp_rx == 0 ? "NO" : "YES"));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\
\nAdvanced per-DLCI cofiguration options.");

  num_of_visible_items = 5;

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "FRAME RELAY ADVANCED DLCI CONFIGURATION",
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
    Debug(DBG_FRAME_RELAY_ADVANCED_DLCI_CONFIGURATION,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1:
      {
        menu_frame_relay_cir_configuration cir_cfg(lxdialog_path, dlci_cfg);
        cir_cfg.run(selection_index);
      }
      break;

    case 2://bc
      /////////////////////////////////////////////////////////////
show_bc_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify BC in Kpbs");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", dlci_cfg->data.chanconf->bc);

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
        Debug(DBG_FRAME_RELAY_ADVANCED_DLCI_CONFIGURATION,
          ("BC on return: %s\n", inb.get_lxdialog_output_string()));

        bc = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(bc < MIN_BC || bc > MAX_BC){

          tb.show_error_message(lxdialog_path, WANCONFIG_FR, "Invalid BC. Min: %d, Max: %d.",
                                MIN_BC, MAX_BC);
          goto show_bc_input_box;
        }else{
          dlci_cfg->data.chanconf->bc = bc;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:

        tb.show_help_message(lxdialog_path, WANCONFIG_FR, bc_help_str);
        goto show_bc_input_box;
        break;

      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case 3://be
      /////////////////////////////////////////////////////////////
show_be_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify BE in Kpbs");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", dlci_cfg->data.chanconf->be);

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
        Debug(DBG_FRAME_RELAY_ADVANCED_DLCI_CONFIGURATION,
          ("BE on return: %s\n", inb.get_lxdialog_output_string()));

        be = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(bc < MIN_BE || bc > MAX_BE){

          tb.show_error_message(lxdialog_path, WANCONFIG_FR, "Invalid BE. Min: %d, Max: %d.",
                                MIN_BE, MAX_BE);
          goto show_bc_input_box;
        }else{
          dlci_cfg->data.chanconf->be = be;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:

        tb.show_help_message(lxdialog_path, WANCONFIG_FR, be_help_str);
        goto show_be_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case 4:
      //dlci_cfg->data.chanconf->inarp == 0 ? "NO" : "YES"));
      //dlci_cfg->data.chanconf->inarp_interval);
      {
        menu_frame_relay_arp frame_relay_arp(lxdialog_path, dlci_cfg);
        frame_relay_arp.run(selection_index);
      }
      break;

    case 5:
      //dlci_cfg->data.chanconf->inarp_rx == 0 ? "NO" : "YES"));
      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s Icoming Inverse ARP support?",
        (dlci_cfg->data.chanconf->inarp_rx == 0 ? "Enable" : "Disable"));

      if(yes_no_question(   selection_index,
                            lxdialog_path,
                            WANCONFIG_FR,
                            tmp_buff) == NO){
        return NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(dlci_cfg->data.chanconf->inarp_rx == 0){
          //was disabled - enable
          dlci_cfg->data.chanconf->inarp_rx = 1;
        }else{
          //was enabled - disable
          dlci_cfg->data.chanconf->inarp_rx = 0;
        }
        break;
      }
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

    Debug(DBG_FRAME_RELAY_ADVANCED_DLCI_CONFIGURATION,
      ("HELP option selected: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1://cir
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_cir_help_str);
      break;

    case 2://bc
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, bc_help_str);
      break;

    case 3://be
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, be_help_str);
      break;

    case 4://tx arp
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_tx_inverse_arp_help);
      break;

    case 5://rx arp
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_icoming_inv_arp_help);
      break;

    default:
      tb.show_help_message(lxdialog_path, WANCONFIG_FR, invalid_help_str);
      break;
    }
    goto again;
    break;

  case MENU_BOX_BUTTON_EXIT:
    //checks if any, done here
    break;
  }//switch(*selection_index)

cleanup:
  return rc;
}
