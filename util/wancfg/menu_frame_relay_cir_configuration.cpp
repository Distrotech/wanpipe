/***************************************************************************
                          menu_frame_relay_cir_configuration.cpp  -  description
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

#include "menu_frame_relay_cir_configuration.h"
#include "text_box.h"
#include "input_box.h"

#define DBG_MENU_FRAME_RELAY_CIR_CONFIGURATION 1

menu_frame_relay_cir_configuration::menu_frame_relay_cir_configuration(
                                      IN char * lxdialog_path,
                                      IN list_element_frame_relay_dlci* dlci_cfg)
{
  Debug(DBG_MENU_FRAME_RELAY_CIR_CONFIGURATION,
    ("menu_frame_relay_cir_configuration::menu_frame_relay_cir_configuration()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->dlci_cfg = dlci_cfg;
}

menu_frame_relay_cir_configuration::~menu_frame_relay_cir_configuration()
{
  Debug(DBG_MENU_FRAME_RELAY_CIR_CONFIGURATION,
    ("menu_frame_relay_cir_configuration::~menu_frame_relay_cir_configuration()\n"));
}

int menu_frame_relay_cir_configuration::run(OUT int * selection_index)
{
  string menu_str;
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  int num_of_visible_items;
  int cir;

  //help text box
  text_box tb;

  //input box for CIR input
  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s Setup", get_protocol_string(WANCONFIG_FR));

  Debug(DBG_MENU_FRAME_RELAY_CIR_CONFIGURATION,
    ("menu_frame_relay_cir_configuration::run()\n"));

  if(dlci_cfg ==  NULL){
    ERR_DBG_OUT(("DLCI configuration pointer is NULL (dlci_cfg) !!!\n"));
    rc = NO;
    goto cleanup;
  }

again:
  snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify CIR in Kpbs");
  snprintf(initial_text, MAX_PATH_LENGTH, "%d", dlci_cfg->data.chanconf->cir);

  inb.set_configuration(  lxdialog_path,
                          backtitle,
                          explanation_text,
                          INPUT_BOX_HIGTH,
                          INPUT_BOX_WIDTH,
                          initial_text);

  menu_str = "";
  if(dlci_cfg->data.chanconf->cir){
    menu_str += " \"1\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Disable CIR\" ");
    menu_str += tmp_buff;

    menu_str += " \"2\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Set CIR value\" ");
    menu_str += tmp_buff;

  }else{
    menu_str += " \"3\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Enable CIR\" ");
    menu_str += tmp_buff;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\n"
"Disable/Enable CIR. If enabled, set CIR\n"
"value.\n"
"\n"
"Use <Back> when configuration is complete.\n");

  num_of_visible_items = 2;

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "DISABLE/ENABLE CIR. SET CIR VALUE.",
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
    Debug(DBG_MENU_FRAME_RELAY_CIR_CONFIGURATION,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1://Disable CIR
      dlci_cfg->data.chanconf->cir = 0;
      break;

    case 2://Set CIR value
show_input_box:
      inb.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_FRAME_RELAY_CIR_CONFIGURATION,
          ("CIR on return: %s\n", inb.get_lxdialog_output_string()));

        cir = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(cir < MIN_CIR || cir > MAX_CIR){

          tb.show_error_message(lxdialog_path, WANCONFIG_FR, "Invalid CIR. Min: %d, Max: %d.",
                                MIN_CIR, MAX_CIR);
          goto show_input_box;
        }else{
          dlci_cfg->data.chanconf->cir = cir;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, WANCONFIG_FR, "Enter CIR. Min: %d, Max: %d.",
                             MIN_CIR, MAX_CIR);
        goto show_input_box;
        break;

      }//switch(*selection_index)
      break;

    case 3://Enable CIR
      dlci_cfg->data.chanconf->cir = 64;//kbps - default
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
    Debug(DBG_MENU_FRAME_RELAY_CIR_CONFIGURATION,
      ("HELP option selected: %s\n", get_lxdialog_output_string()));

    tb.show_help_message(lxdialog_path, WANCONFIG_FR, fr_cir_help_str);

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
