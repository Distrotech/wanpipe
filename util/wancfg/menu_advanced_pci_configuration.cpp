/***************************************************************************
                          menu_advanced_pci_configuration.cpp  -  description
                             -------------------
    begin                : Wed Jun 2 2004
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

#include "menu_advanced_pci_configuration.h"

#include "text_box_help.h"
#include "input_box.h"
#include "text_box_yes_no.h"
#include "menu_hardware_cpu_number.h"
#include "menu_hardware_select_comms_port.h"

extern char* cpu_number_help_str;
extern char* comms_port_help_str;


char* pci_slot_auto_help_str =
"PCI SLOT AUTO-DETECTION\n"
"\n"
"Select this option to enable PCI SLOT autodetection.\n"
"Otherwise, you must enter a valid PCI SLOT location\n"
"of your S514 or AFT card.\n"
"\n"
"NOTE: This option will only work for single S514 or AFT\n"
"card installations.\n"
"\n"
"In multi-adaptor situations a correct PCI\n"
"slot must be supplied for each card.\n";

char* pci_bus_slot_help_str =
"PCI BUS and SLOT\n"
"\n"
"If PCI slot is set for autodetection,\n"
"PCI bus will also be autodetected.\n"
"\n"
"If more than one Sangoma card is installed,\n"
"the 'PCI SLOT AUTO-DETECTION' should NOT be used.\n"
"In multi-adaptor situations a correct PCI\n"
"bus and slot must be supplied for each card.\n";

#define MIN_PCI_BUS_SLOT 0
#define MAX_PCI_BUS_SLOT 255

#define DBG_MENU_ADVANCED_PCI_CONFIGURATION 1

enum ADVANCED_PCI_CFG_OPTIONS {
  EMPTY_LINE=1,
  S514CPU,
  AUTO_PCISLOT,
  PCISLOT,
  PCIBUS,
  COMMS_PORT,
  FE_LINE_NUMBER
};

menu_advanced_pci_configuration::menu_advanced_pci_configuration(IN char * lxdialog_path,
                                                          IN conf_file_reader* cfr)
{
  Debug(DBG_MENU_ADVANCED_PCI_CONFIGURATION,
    ("menu_advanced_pci_configuration::menu_advanced_pci_configuration()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);

  this->cfr = cfr;
}

menu_advanced_pci_configuration::~menu_advanced_pci_configuration()
{
  Debug(DBG_MENU_ADVANCED_PCI_CONFIGURATION,
    ("menu_advanced_pci_configuration::~menu_advanced_pci_configuration()\n"));
}

int menu_advanced_pci_configuration::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int option_selected;
  char exit_dialog;
  int number_of_items;

  //help text box
  text_box tb;

  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility");

  link_def_t * link_def;
  wandev_conf_t *linkconf;
  sdla_fe_cfg_t*  fe_cfg;
  
  Debug(DBG_MENU_ADVANCED_PCI_CONFIGURATION, ("menu_advanced_pci_configuration::run()\n"));

again:
  number_of_items = 0;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;
  fe_cfg   = &linkconf->fe_cfg;
  Debug(DBG_MENU_ADVANCED_PCI_CONFIGURATION, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  option_selected = 0;
  exit_dialog = NO;

  menu_str = " ";

  form_pci_card_locations_options_menu(menu_str, number_of_items);

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nHardware for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "ADVANCED PCI CONFIGURATION",
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
    Debug(DBG_MENU_ADVANCED_PCI_CONFIGURATION,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case EMPTY_LINE:
      //do nothing
      break;

    case S514CPU://A or B
      {
        menu_hardware_cpu_number cpu_number(lxdialog_path, cfr);
        if(cpu_number.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case AUTO_PCISLOT://YES/NO
      //dlci_cfg->data.chanconf->inarp_rx == 0 ? "NO" : "YES"));
      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s AUTO PCISLOT option?",
        (linkconf->auto_hw_detect == 0 ? "Enable" : "Disable"));

      if(yes_no_question(   selection_index,
                            lxdialog_path,
                            NO_PROTOCOL_NEEDED,
                            tmp_buff) == NO){
        return NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(linkconf->auto_hw_detect == 0){
          //was disabled - enable
          linkconf->auto_hw_detect = 1;
        }else{
          //was enabled - disable
          linkconf->auto_hw_detect = 0;
        }
        break;
      }
      break;

    case PCISLOT:
      int pcislot;
      snprintf(explanation_text, MAX_PATH_LENGTH, "PCI Slot Number (between 0 and 255)");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", linkconf->PCI_slot_no);

show_pcislot_input_box:
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
        Debug(DBG_MENU_ADVANCED_PCI_CONFIGURATION,
          ("PCISLOT on return: %s\n", inb.get_lxdialog_output_string()));

        pcislot = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(pcislot < MIN_PCI_BUS_SLOT || pcislot > MAX_PCI_BUS_SLOT){
          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
                                "Invalid PCI Slot. Min: %d, Max: %d.",
                                MIN_PCI_BUS_SLOT, MAX_PCI_BUS_SLOT);
          goto show_pcislot_input_box;
        }else{
          linkconf->PCI_slot_no = pcislot;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:

        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          "Enter PCI Slot Number (between 0 and 255)");
        goto show_pcislot_input_box;
        break;

      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case PCIBUS:
      int pcibus;
      snprintf(explanation_text, MAX_PATH_LENGTH, "PCI Bus Number (between 0 and 255)");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", linkconf->pci_bus_no);

show_pcibus_input_box:
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
        Debug(DBG_MENU_ADVANCED_PCI_CONFIGURATION,
          ("PCIbus on return: %s\n", inb.get_lxdialog_output_string()));

        pcibus = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(pcibus < MIN_PCI_BUS_SLOT || pcibus > MAX_PCI_BUS_SLOT){

          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
                                "Invalid PCI Bus. Min: %d, Max: %d.",
                                MIN_PCI_BUS_SLOT, MAX_PCI_BUS_SLOT);
          goto show_pcibus_input_box;
        }else{
          linkconf->pci_bus_no = pcibus;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:

        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          "Enter PCI Bus Number (between 0 and 255)");
        goto show_pcibus_input_box;
        break;

      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case COMMS_PORT://Pri/Sec
      {
        menu_hardware_select_comms_port select_comms_port(lxdialog_path, cfr);
        if(select_comms_port.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;


    case FE_LINE_NUMBER:
      int line_no;
      snprintf(explanation_text, MAX_PATH_LENGTH, "Line Number (between 1 and 8)");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", fe_cfg->line_no);

show_line_no_input_box:
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
        Debug(DBG_MENU_ADVANCED_PCI_CONFIGURATION,
          ("line_no on return: %s\n", inb.get_lxdialog_output_string()));

        line_no = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

        if(line_no < 1 || line_no > 8){

          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
                                "Invalid Line Number. Min: %d, Max: %d.",
                                1, 8);
          goto show_line_no_input_box;
        }else{
          fe_cfg->line_no = line_no;
        }
        break;

      case INPUT_BOX_BUTTON_HELP:

        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          "Enter Line Number (between 1 and 8)");
        goto show_line_no_input_box;
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

    switch(atoi(get_lxdialog_output_string()))
    {

    case S514CPU:
    case FE_LINE_NUMBER:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, cpu_number_help_str);
      break;

    case AUTO_PCISLOT:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, pci_slot_auto_help_str);
      break;

    case PCISLOT:
    case PCIBUS:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, pci_bus_slot_help_str);
      break;

    case COMMS_PORT:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, comms_port_help_str);
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
    }
    break;

  case MENU_BOX_BUTTON_EXIT:
    rc = YES;
    exit_dialog = YES;
    break;
  }//switch(*selection_index)

  if(exit_dialog == NO){
    goto again;
  }

cleanup:
  return rc;
}

void menu_advanced_pci_configuration::
  form_pci_card_locations_options_menu(string& str, int& number_of_items)
{
  char tmp_buff[MAX_PATH_LENGTH];
  link_def_t * link_def;
  wandev_conf_t *linkconf;
  sdla_fe_cfg_t*  fe_cfg;
  
  Debug(DBG_MENU_ADVANCED_PCI_CONFIGURATION,
    ("menu_net_interface_setup::form_pci_card_locations_options_menu()\n"));

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;
  fe_cfg   = &linkconf->fe_cfg;
 
  switch(linkconf->card_type)
  {
  case WANOPT_S51X:
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S514CPU);
    		str += tmp_buff;

    	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"CPU-------------> %s\" ",
      		linkconf->S514_CPU_no);
    	str += tmp_buff;

    	number_of_items++;
	break;

  case WANOPT_AFT:
  	switch(link_def->card_version)
    	{
    	case A101_ADPTR_1TE1://A101/2
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S514CPU);
    		str += tmp_buff;

    		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"CPU-------------> %s\" ",
      			linkconf->S514_CPU_no);
    		str += tmp_buff;

    		number_of_items++;
		break;

    	case A104_ADPTR_4TE1://A104
		//line number, not cpu
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", FE_LINE_NUMBER);
    		str += tmp_buff;

    		//snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Line Number-----> %d\" ",
    		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Port Number-----> %d\" ",
      			fe_cfg->line_no);
    		str += tmp_buff;

    		number_of_items++;
		break;
	
    	case A300_ADPTR_U_1TE3://WAN_MEDIA_DS3:
    		//only one port, there is nothing to select
		break;
    	}
   }

  if(linkconf->card_type == WANOPT_S51X){

    switch(link_def->card_version)
    {
    case S5141_ADPTR_1_CPU_SERIAL:
    case S5143_ADPTR_1_CPU_FT1:

      number_of_items++;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", COMMS_PORT);
      str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Comms Port------> %s\" ",
        (linkconf->comm_port == 0 ? "Primary" : "Secondary"));
      str += tmp_buff;
      break;
    }
  }

  if(linkconf->auto_hw_detect == 1){

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AUTO_PCISLOT);
    str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AUTO_PCISLOT----> YES\" ");
    str += tmp_buff;

    number_of_items++;
  }else{
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AUTO_PCISLOT);
    str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AUTO_PCISLOT----> NO\" ");
    str += tmp_buff;

    /////////////////////////////////////////////////////////////////
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", PCISLOT);
    str += tmp_buff;

    if(linkconf->PCI_slot_no == NOT_SET){
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"PCISLOT---------> %s\" ", NOT_SET_STR);
    }else{
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"PCISLOT---------> %d\" ",
        linkconf->PCI_slot_no);
    }
    str += tmp_buff;

    /////////////////////////////////////////////////////////////////
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", PCIBUS);
    str += tmp_buff;

    if(linkconf->pci_bus_no == NOT_SET){
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"PCIBUS----------> %s\" ", NOT_SET_STR);
    }else{
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"PCIBUS----------> %d\" ",
        linkconf->pci_bus_no);
    }
    str += tmp_buff;
    /////////////////////////////////////////////////////////////////
    number_of_items += 3;
  }
}
