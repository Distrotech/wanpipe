/***************************************************************************
                          menu_hardware_setup.cpp  -  description
                             -------------------
    begin                : Tue Mar 30 2004
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

#include "menu_hardware_setup.h"
#include "text_box_help.h"
#include "input_box.h"
#include "text_box_yes_no.h"
#include "menu_hardware_card_type.h"
#include "menu_hardware_serial_card_advanced_options.h"
#include "menu_hardware_te1_card_advanced_options.h"
#include "menu_hardware_te3_card_advanced_options.h"

#include "menu_hardware_select_comms_port.h"
#include "menu_te1_select_media.h"
#include "menu_te3_select_media.h"

#include "menu_hardware_serial_select_medium.h"
#include "menu_s508_io_port_select.h"
#include "menu_s508_irq_select.h"
#include "menu_s508_memory_addr.h"

#include "menu_list_existing_wanpipes.h"
#include "menu_aft_logical_channels_list.h"

#include "menu_advanced_pci_configuration.h"

#include "menu_adsl_encapsulation.h"
#include "menu_adsl_advanced_cfg.h"

#include "menu_tdmv_law.h"

#define DBG_MENU_HARDWARE_SETUP 1

extern char* adapter_type_help_str;
extern char* cpu_number_help_str;
extern char* media_type_help_str;

#define MIN_PCI_BUS_SLOT 0
#define MAX_PCI_BUS_SLOT 255

///////////////////////////////////////////////////////////
char* te1_advanced_cfg_help_str =
"This option allows to set following T1/E1 parameters:\n"
"  -Line decoding\n"
"  -Framing\n"
"  -TE clock mode\n"
"  -Act. channels\n"
"  -LBO (T1 only)\n";


char* comms_port_help_str =
"COMMUNICATION PORT\n"
"\n"
"Currently only CHDLC and HDLC Streaming protocols can use\n"
"both ports on a S508 or S514 PCI card at the same time.\n"
"Thus, two physical links can be hooked up to\n"
"a single card using the above protocols.\n"
"\n"
"Availabe options:\n"
"\n"
"PRI: Primary High speed port\n"
"  S508: up to 2Mbps\n"
"  S514: up to 4Mbps\n"
"\n"
"SEC: Secondary Low speed port\n"
"  S508: up to 256Kbps\n"
"  S514: up to 512Kbps\n"
"\n"
"Default: PRI\n";

char* s508_io_port_help_str =
"S508 IO PORT\n"
"\n"
"Adapter's I/O port address.  Make sure there is no\n"
"conflict with other devices on the system in\n"
"/proc/ioports.\n"
"IO port address values are set by\n"
"the jumpers on the S508 board.\n"
"Refer to the user manual for the jumper setup.\n";

char* s508_irq_help_str =
"S508 IRQ\n"
"\n"
"Adapters interrupt request level. Make sure there is no\n"
"conflict with other devices on the system in\n"
"/proc/interrupts.\n"
"IRQ used by S508 is software set and\n"
"NO jumper reconfiguration is needed.\n";

char* s508_shared_memory_addr =
"MEMORY ADDRESS\n"
"\n"
"Address of the adapter shared memory window.\n"
"If set to Auto the memory address is determined\n"
"automatically.\n"
"\n"
"Default: Auto\n";

char* serial_physical_interface_help_str =
"Serial Line Physical Interface\n"
"\n"
"Physical interface type. Available options are:\n"
"  RS232   RS-232C interface (V.10)\n"
"  V35     V.35 interface (X.21/EIA530/RS-422/V.11)\n";

char* serial_advanced_configuration_help_str =
"Advanced Serial Line Configuration\n"
"----------------------------------\n"
"CLOCKING\n"
"\n"
"Source of the adapter transmit and receive clock\n"
"signals.  Available options are:\n"
"\n"
"External:  Clock is provided externally (e.g. by\n"
"the modem or CSU/DSU). Use this for the\n"
"FT1 (S5143) boards too.\n"
"\n"
"Internal:  Clock is generated on the adapter.\n"
"When there are two Sangoma Cards back\n"
"to back set clocking to Internal on one\n"
"of the cards.\n"
"Note: Jumpers must be set for internal or external\n"
"clocking for RS232 communication on:\n"
"s508 board: RS232 SEC port\n"
"s514 board: RS232 PRI and SEC port\n"
"\n"
"BAUDRATE\n"
"\n"
"Data transfer rate in bits per second.  These values\n"
"are meaningful if internal clocking is selected\n"
"(like in a back-to-back testing configuration).\n";


///////////////////////////////////////////////////////////

enum HW_SETUP_OPTIONS {
  EMPTY_LINE=1,
  CARD_TYPE,
  S514_SERIAL_INTERFACE,
  S514_TE1_INTERFACE,
  AFT_TE3_INTERFACE,
  COMMS_PORT,
  S514_SERIAL_ADVANCED,
  S514_TE1_ADVANCED,
  AFT_TE1_ADVANCED,
  AFT_TE3_ADVANCED,
  ADSL_ENCAPSULATION_MODE,
  ADSL_ATM_AUTOCFG,
  ADSL_VCI,
  ADSL_VPI,
  ADSL_WATCHDOG,
  ADSL_ADVANCED,
  S508_IO_PORT,
  S508_MEMORY,
  S508_IRQ,
  TIMESLOT_GROUP_CFG,
  ADVANCED_PCI_CFG,
  TDMV_LAW_SELECT,
  TDMV_OPERMODE,
  AFT_ANALOG_ADVANCED,
  AFT_SERIAL_CONNECTION_TYPE,
  AFT_SERIAL_LINE_CODING,
  AFT_SERIAL_LINE_IDLE
};

menu_hardware_setup::menu_hardware_setup(   IN char * lxdialog_path,
                                            IN conf_file_reader* cfr)
{
  Debug(DBG_MENU_HARDWARE_SETUP, ("menu_hardware_setup::menu_hardware_setup()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);

  this->cfr = cfr;
}

menu_hardware_setup::~menu_hardware_setup()
{
  Debug(DBG_MENU_HARDWARE_SETUP, ("menu_hardware_setup::~menu_hardware_setup()\n"));
}

int menu_hardware_setup::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int option_selected;
  char exit_dialog;
  int number_of_items;

  int old_card_type;
  int new_card_type;

  //help text box
  text_box tb;

  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility");

  link_def_t * link_def;
  wandev_conf_t *linkconf;
  wan_adsl_conf_t* adsl_cfg;
  sdla_fe_cfg_t*  fe_cfg;

again:
  number_of_items = 0;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;
  adsl_cfg = &linkconf->u.adsl;
  fe_cfg = &linkconf->fe_cfg;

  Debug(DBG_MENU_HARDWARE_SETUP, ("menu_hardware_setup::run(): cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  option_selected = 0;
  exit_dialog = NO;

  menu_str = " ";
  ////////////////////////////////////////////////
  //depending on 'current hardware' display it's settings
  //e.g.: CHANNELS is irrelevant for S5141
  //      V.35 is irrelevant for S5147
  switch(linkconf->card_type)
  {
  case WANOPT_S51X:

    if(get_S514_card_version_string(link_def->card_version) == NULL){
      ERR_DBG_OUT(("Invalid S514 Card Version!!"));
      rc = NO;
      goto cleanup;
    }
    /////////////////////////////////////////////////
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CARD_TYPE);
    menu_str = tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Card Type-------> %s\" ",
      get_S514_card_version_string(link_def->card_version));
    menu_str += tmp_buff;

    switch(link_def->card_version)
    {
    case S5141_ADPTR_1_CPU_SERIAL:
      number_of_items = 2;

      form_s514_serial_options_menu(menu_str, number_of_items);

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
      menu_str += tmp_buff;
      number_of_items++;

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S514_SERIAL_ADVANCED);
      menu_str += tmp_buff;
      menu_str += " \"Advanced Physical Medium Configuration\" ";

      form_pci_card_locations_options_menu(menu_str, number_of_items);
      break;

    case S5143_ADPTR_1_CPU_FT1:
      //no options
      number_of_items = 1;

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
      menu_str += tmp_buff;
      number_of_items++;

      form_pci_card_locations_options_menu(menu_str, number_of_items);
      break;

    case S5144_ADPTR_1_CPU_T1E1:
      number_of_items = 2;

      form_s514_TE1_options_menu(menu_str, number_of_items);

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
      menu_str += tmp_buff;
      number_of_items++;

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S514_TE1_ADVANCED);
      menu_str += tmp_buff;
      menu_str += " \"Advanced Physical Medium Configuration\" ";

      form_pci_card_locations_options_menu(menu_str, number_of_items);
      break;

    case S5145_ADPTR_1_CPU_56K:
      //no options
      number_of_items = 1;

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
      menu_str += tmp_buff;
      number_of_items++;

      form_pci_card_locations_options_menu(menu_str, number_of_items);
      break;

    }//switch(link_def->card_version)
    break;

  case WANOPT_AFT:

    number_of_items = 2;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CARD_TYPE);
    menu_str = tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Card Type-------> %s\" ",
      get_card_type_string(linkconf->card_type, link_def->card_version));
    menu_str += tmp_buff;

    switch(link_def->card_version)
    {
    case A101_ADPTR_1TE1://WAN_MEDIA_T1:
    case A104_ADPTR_4TE1://WAN_MEDIA_T1:
      //AFT using the same code for TE1 configuration
      form_s514_TE1_options_menu(menu_str, number_of_items);

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
      menu_str += tmp_buff;
      number_of_items++;

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_TE1_ADVANCED);
      menu_str += tmp_buff;
      menu_str += " \"Advanced Physical Medium Configuration\" ";

      //timeslot group configuration
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TIMESLOT_GROUP_CFG);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Advanced %s Configuration\" ", TIME_SLOT_GROUPS_STR);
      menu_str += tmp_buff;
      number_of_items++;
      break;

    case A200_ADPTR_ANALOG:
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TDMV_LAW_SELECT);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"TDMV LAW--------> %s\" ", 
	(fe_cfg->tdmv_law == WAN_TDMV_MULAW ? "MuLaw" : "ALaw"));
      menu_str += tmp_buff;
      number_of_items++;

      //for backwards compatibility with older files, check TDMV_OPERMODE was read
      //or initiaized when card type was selected
      if(fe_cfg->cfg.remora.opermode_name[0] != 0){
        snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TDMV_OPERMODE);
        menu_str += tmp_buff;
        snprintf(tmp_buff, MAX_PATH_LENGTH, " \"TDMV Operational Mode-> %s\" ", 
	  fe_cfg->cfg.remora.opermode_name);
        menu_str += tmp_buff;
        number_of_items++;
      }

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
      menu_str += tmp_buff;
      number_of_items++;

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_ANALOG_ADVANCED);
      menu_str += tmp_buff;
      menu_str += " \"Advanced Physical Medium Configuration\" ";
      number_of_items++;

      //timeslot group configuration
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TIMESLOT_GROUP_CFG);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Advanced %s Configuration\" ", TIME_SLOT_GROUPS_STR);
      menu_str += tmp_buff;
      number_of_items++;
      break;
      
    case A300_ADPTR_U_1TE3://WAN_MEDIA_DS3:
      form_TE3_options_menu(menu_str, number_of_items);
      
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
      menu_str += tmp_buff;
      number_of_items++;

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_TE3_ADVANCED);
      menu_str += tmp_buff;
      menu_str += " \"Advanced Physical Medium Configuration\" ";
      break;
    case AFT_ADPTR_ISDN://WAN_MEDIA_BRI:
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
      menu_str += tmp_buff;
      number_of_items++;

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_TE1_ADVANCED);
      menu_str += tmp_buff;
      menu_str += " \"Advanced Physical Medium Configuration\" ";
      break;
    case AFT_ADPTR_2SERIAL_V35X21:
      form_AFT_Serial_options_menu(menu_str, number_of_items);

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
      menu_str += tmp_buff;
      number_of_items++;

      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S514_SERIAL_ADVANCED);
      menu_str += tmp_buff;
      menu_str += " \"Advanced Physical Medium Configuration\" ";
			break;
    }

    form_pci_card_locations_options_menu(menu_str, number_of_items);
    break;

  case WANOPT_ADSL:

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CARD_TYPE);
    menu_str = tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Card Type-------> %s\" ",
      get_card_type_string(linkconf->card_type, link_def->card_version));
    menu_str += tmp_buff;
    number_of_items++;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
    menu_str += tmp_buff;
    number_of_items++;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ADSL_ENCAPSULATION_MODE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"EncapMode-> %s\" ",
	ADSL_ENCAPSULATION_DECODE(adsl_cfg->EncapMode));
    menu_str += tmp_buff;
    number_of_items++;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ADSL_ATM_AUTOCFG);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"ATM_AUTOCFG-> %s\" ",
	(adsl_cfg->atm_autocfg == WANOPT_NO ? "NO" : "YES"));
    menu_str += tmp_buff;
    number_of_items++;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ADSL_VCI);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"VCI---------> %u\" ",
		    adsl_cfg->Vci);
    menu_str += tmp_buff;
    number_of_items++;
    
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ADSL_VPI);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"VPI---------> %u\" ",
		    adsl_cfg->Vpi);
    menu_str += tmp_buff;
    number_of_items++;
 
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ADSL_WATCHDOG);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"ADSL_WATCHDOG-> %s\" ",
	(adsl_cfg->atm_watchdog == WANOPT_NO ? "NO" : "YES"));
    menu_str += tmp_buff;
    number_of_items++;
    
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
    menu_str += tmp_buff;
    number_of_items++;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ADSL_ADVANCED);
    menu_str += tmp_buff;
    menu_str += " \"Advanced ADSL Configuration\" ";
    number_of_items++;

    form_pci_card_locations_options_menu(menu_str, number_of_items);
    number_of_items++;

    break;

  case WANOPT_S50X:
    number_of_items = 5;

    menu_str.sprintf(" \"%d\" ", CARD_TYPE);
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Card Type-------> %s\" ",
      get_card_type_string(linkconf->card_type, link_def->card_version));
    menu_str += tmp_buff;

    /////////////////////
    //initialize for new configuration
    if(linkconf->ioport == 0){
      linkconf->ioport = 0x360;
    }
    if(linkconf->irq == 0){
      linkconf->irq = 7;
    }
    /////////////////////

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", COMMS_PORT);
    menu_str+= tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Comms Port------> %s\" ",
      (linkconf->comm_port == 0 ? "Primary" : "Secondary"));
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S508_IO_PORT);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"IOPORT----------> 0x%X\" ", linkconf->ioport);
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S508_IRQ);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"IRQ-------------> %d\" ", linkconf->irq);
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S508_MEMORY);
    menu_str += tmp_buff;
    if(linkconf->maddr == 0x00){
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Memory Addr-----> Auto\" ");
    }else{
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Memory Addr-----> 0x%lX\" ", linkconf->maddr);
    }
    menu_str += tmp_buff;

    form_s514_serial_options_menu(menu_str, number_of_items);

    number_of_items++;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S514_SERIAL_ADVANCED);
    menu_str += tmp_buff;
    menu_str += " \"Advanced Physical Medium Configuration\" ";
    break;

  case NOT_SET:
    //It is a new configuration.
    //Do not display "Undefined" twice - go directly to hardware selection.
    goto display_hardware_selection_label;

  }//switch(linkconf->card_type)

  Debug(DBG_MENU_HARDWARE_SETUP, ("number_of_items: %d\n", number_of_items));
  
  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nHardware for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "HARDWARE CONFIGURATION",
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
    Debug(DBG_MENU_HARDWARE_SETUP,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case EMPTY_LINE:
      //do nothing
      break;

    case CARD_TYPE:
    case NOT_SET:
display_hardware_selection_label:
      {
        old_card_type = new_card_type = linkconf->card_type;

        menu_hardware_card_type hardware_card_type(lxdialog_path, cfr);
        if(hardware_card_type.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }else{
          new_card_type = linkconf->card_type;

          Debug(DBG_MENU_HARDWARE_SETUP,
            ("hardware_setup: old_card_type: %d, new_card_type: %d\n", old_card_type, new_card_type));

          if(old_card_type != new_card_type){
	    //if card type change reset everything to defaults
	    //delete list of logical channels
	    if(cfr->main_obj_list != NULL){
	      delete cfr->main_obj_list;
	      cfr->main_obj_list = NULL;
	    }

	    //create the new list
	    cfr->main_obj_list = new objects_list();
	    
	    //insert one logical channel
	    switch(new_card_type)
	    {
	    case WANOPT_S50X:
	    case WANOPT_S51X:
	      adjust_number_of_logical_channels_in_list(WANCONFIG_GENERIC, cfr->main_obj_list,
		cfr, 1);
	      break;
	      
	    case WANOPT_AFT:
	      adjust_number_of_logical_channels_in_list(WANCONFIG_AFT, cfr->main_obj_list,
		cfr, 1);
 	      break;

	    case WANOPT_ADSL:
	      adjust_number_of_logical_channels_in_list(WANCONFIG_ADSL, cfr->main_obj_list,
		cfr, 1);
	      break;

	    default:
	      ERR_DBG_OUT(("Invalid card type 0x%X!!\n", linkconf->card_type));
	    }
	  }else{
            Debug(DBG_MENU_HARDWARE_SETUP, ("card type unchanged\n"));

	    if(linkconf->card_type == NOT_SET){
		//user did NOT select any card type, but wants to exit. exit the dialog.
		rc = YES;
		exit_dialog = YES;
	    }
	  }
        }
      }
      break;

    case S514_TE1_INTERFACE:
      {
        menu_te1_select_media te1_select_media(lxdialog_path, cfr);
        if(te1_select_media.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case AFT_TE3_INTERFACE:
      {
        menu_te3_select_media te3_select_media(lxdialog_path, cfr);
        if(te3_select_media.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
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

    case S514_SERIAL_INTERFACE://V.35/RS232
      {
        menu_hardware_serial_select_medium serial_select_medium(lxdialog_path, cfr);
        if(serial_select_medium.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case S514_SERIAL_ADVANCED://Clock + bit rate
      {
        menu_hardware_serial_card_advanced_options hardware_serial_card_advanced_options(
                                                                          lxdialog_path, cfr);
        if(hardware_serial_card_advanced_options.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case S514_TE1_ADVANCED:
    case AFT_TE1_ADVANCED:
      {
        menu_hardware_te1_card_advanced_options hardware_te1_card_advanced_options(
                                                                  lxdialog_path, cfr);
        if(hardware_te1_card_advanced_options.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case AFT_ANALOG_ADVANCED:
      {
        menu_hardware_analog_card_advanced_options hardware_analog_card_advanced_options(
                                                                  lxdialog_path, cfr);
        if(hardware_analog_card_advanced_options.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case AFT_TE3_ADVANCED:
      {
        menu_hardware_te3_card_advanced_options hardware_te3_card_advanced_options(
                                                                  lxdialog_path, cfr);
        if(hardware_te3_card_advanced_options.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case S508_IO_PORT:
      {
        menu_s508_io_port_select s508_io_port_select(lxdialog_path, cfr);
        if(s508_io_port_select.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case S508_IRQ:
      {
        menu_s508_irq_select s508_irq_select(lxdialog_path, cfr);
        if(s508_irq_select.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case S508_MEMORY:
      {
        menu_s508_memory_addr s508_memory_addr(lxdialog_path, cfr);
        if(s508_memory_addr.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case TIMESLOT_GROUP_CFG:
      //for the AFT card:
      //1. get number of logical channels
      //2. for each log. cha-l get the timeslots it will use
      //3. for each log. cha-l get the protocol
      {
        menu_aft_logical_channels_list aft_logical_channels_list(lxdialog_path, cfr);
        if(aft_logical_channels_list.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case ADVANCED_PCI_CFG:
      {
        menu_advanced_pci_configuration advanced_pci_configuration(lxdialog_path, cfr);
        if(advanced_pci_configuration.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case ADSL_ENCAPSULATION_MODE:
      {
				int old_encapsulation = adsl_cfg->EncapMode;
	
				menu_adsl_encapsulation adsl_encapsulation(lxdialog_path, adsl_cfg);
				if(adsl_encapsulation.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }

				if(old_encapsulation != adsl_cfg->EncapMode){
					//change the 'sub_config_id' accordingly
          switch(adsl_cfg->EncapMode)
          {
          case RFC_MODE_BRIDGED_ETH_LLC:
          case RFC_MODE_BRIDGED_ETH_VC:
						link_def->sub_config_id = PROTOCOL_ETHERNET;
            break;
    
          case RFC_MODE_ROUTED_IP_LLC:
          case RFC_MODE_ROUTED_IP_VC:
          case RFC_MODE_RFC1577_ENCAP:
            link_def->sub_config_id = PROTOCOL_IP;
            break;
    
          case RFC_MODE_PPP_LLC:
          case RFC_MODE_PPP_VC:
            link_def->sub_config_id = WANCONFIG_MPPP;
            break;
          }//switch(adsl_cfg->EncapMode)
				}
      }
      break;
      
    case ADSL_ATM_AUTOCFG:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s ADSL autoconfiguration?",
				(adsl_cfg->atm_autocfg == WANOPT_NO ? "Enable" : "Disable"));

      if(yes_no_question( selection_index,
                          lxdialog_path,
                          NO_PROTOCOL_NEEDED,
                          tmp_buff) == NO){
				//error displaying dialog
				rc = NO;
				goto cleanup;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
				if(adsl_cfg->atm_autocfg == WANOPT_NO){
					//disabled, user wants to enable
				  adsl_cfg->atm_autocfg = WANOPT_YES;
				}else{
				  adsl_cfg->atm_autocfg = WANOPT_NO;
				}
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
				//don't do anything
				break;
      }
      break;

    case ADSL_VCI:
show_vci_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Enter Vci number");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", adsl_cfg->Vci);

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
        adsl_cfg->Vci = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));
        break;

      case INPUT_BOX_BUTTON_HELP:
				tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
        goto show_vci_input_box;
      }//switch(*selection_index)
      break;
      
    case ADSL_VPI:
show_vpi_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Enter Vpi number");
      snprintf(initial_text, MAX_PATH_LENGTH, "%d", adsl_cfg->Vpi);

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
        adsl_cfg->Vpi = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));
        break;

      case INPUT_BOX_BUTTON_HELP:
	tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
        goto show_vpi_input_box;
      }//switch(*selection_index)
      break;
      
    case ADSL_WATCHDOG:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s ADSL watchdog?",
	(adsl_cfg->atm_watchdog == WANOPT_NO ? "Enable" : "Disable"));

      if(yes_no_question( selection_index,
                          lxdialog_path,
                          NO_PROTOCOL_NEEDED,
                          tmp_buff) == NO){
	//error displaying dialog
	rc = NO;
	goto cleanup;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
	if(adsl_cfg->atm_watchdog == WANOPT_NO){
	  //disabled, user wants to enable
	  adsl_cfg->atm_watchdog = WANOPT_YES;
	}else{
	  adsl_cfg->atm_watchdog = WANOPT_NO;
	}
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
	//don't do anything
	break;
      }
      break;
      
    case ADSL_ADVANCED:
      {
	menu_adsl_advanced_cfg adsl_advanced_cfg(lxdialog_path, adsl_cfg);
	if(adsl_advanced_cfg.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case TDMV_LAW_SELECT:
      {
        menu_tdmv_law tdmv_law(lxdialog_path, cfr);
        if(tdmv_law.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case TDMV_OPERMODE:
      {
        menu_tdmv_opermode tdmv_opermode(lxdialog_path, cfr);
        if(tdmv_opermode.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;
		case AFT_SERIAL_CONNECTION_TYPE:
      {
        menu_hardware_serial_connection_type hardware_serial_connection_type(
                                                                     lxdialog_path, cfr);
        if(hardware_serial_connection_type.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
			break;

		case AFT_SERIAL_LINE_CODING:
      {
        menu_hardware_serial_line_coding hardware_serial_line_coding(
                                                                     lxdialog_path, cfr);
        if(hardware_serial_line_coding.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
			break;

		case AFT_SERIAL_LINE_IDLE:
      {
        menu_hardware_serial_line_idle hardware_serial_line_idle(
                                                                     lxdialog_path, cfr);
        if(hardware_serial_line_idle.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
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
    case CARD_TYPE:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, adapter_type_help_str);
      break;

    case S514_TE1_ADVANCED:
    case AFT_TE1_ADVANCED:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, te1_advanced_cfg_help_str);
      break;

    case S514_TE1_INTERFACE:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, media_type_help_str);
      break;

    case COMMS_PORT:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, comms_port_help_str);
      break;

    case S508_IO_PORT:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, s508_io_port_help_str);
      break;

    case S508_IRQ:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, s508_irq_help_str);
      break;

    case S508_MEMORY:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, s508_shared_memory_addr);
      break;

    case S514_SERIAL_INTERFACE:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, serial_physical_interface_help_str);
      break;

    case S514_SERIAL_ADVANCED:
      tb.show_help_message( lxdialog_path, NO_PROTOCOL_NEEDED,
                            serial_advanced_configuration_help_str);
      break;

    case ADSL_ADVANCED:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
      break;
 
    default:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
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

void menu_hardware_setup::form_s514_serial_options_menu(string& str, int& number_of_items)
{
  char tmp_buff[MAX_PATH_LENGTH];
  link_def_t * link_def;
  wandev_conf_t *linkconf;

  Debug(DBG_MENU_HARDWARE_SETUP,
    ("menu_net_interface_setup::form_s514_serial_options_menu()\n"));

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S514_SERIAL_INTERFACE);
  str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Physical Medium-> %s\" ",
  //snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface------> %s\" ",
    (linkconf->electrical_interface == WANOPT_V35 ? "V.35" : "RS232"));
  str += tmp_buff;

  number_of_items++;
}

void menu_hardware_setup::form_AFT_Serial_options_menu(string& str, int& number_of_items)
{
  char tmp_buff[MAX_PATH_LENGTH];
  link_def_t * link_def;
  wandev_conf_t *linkconf;

  Debug(DBG_MENU_HARDWARE_SETUP, ("menu_net_interface_setup::%s()\n", __FUNCTION__));

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

	//////////////////////////////////////////////////////////////////////////////////////////
	//Serial connection type
	//Permanent - CTS/RTS are always up
	//Switched  - CTS/RTS are only brought up tx
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_SERIAL_CONNECTION_TYPE);
  str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Connection Type-> %s\" ",
    (linkconf->connection == WANOPT_PERMANENT ? "Permanent" : (linkconf->connection == WANOPT_SWITCHED?"Switched":"Unknown!")));
  str += tmp_buff;
  number_of_items++;
	//////////////////////////////////////////////////////////////////////////////////////////
	//Serial LineCoding: NRZ or NRZI
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_SERIAL_LINE_CODING);
  str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Line Coding-----> %s\" ",
    (linkconf->line_coding == WANOPT_NRZ ? "NRZ" : (linkconf->line_coding == WANOPT_NRZI ? "NRZI":"Unknown!")));
  str += tmp_buff;
  number_of_items++;
	//////////////////////////////////////////////////////////////////////////////////////////
	//Serial LineIdle:
	//Flag - use flags to idle on the line	
	//Mark - use mark to idle on the line
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_SERIAL_LINE_IDLE);
  str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Line Idle-------> %s\" ",
    (linkconf->line_idle == WANOPT_IDLE_FLAG ? "Flag" : (linkconf->line_idle == WANOPT_IDLE_MARK ? "Mark":"Unknown!")));
  str += tmp_buff;
  number_of_items++;
	//////////////////////////////////////////////////////////////////////////////////////////

}

void menu_hardware_setup::form_s514_TE1_options_menu(string& str, int& number_of_items)
{
  char tmp_buff[MAX_PATH_LENGTH];
  link_def_t * link_def;
  wandev_conf_t *linkconf;

  Debug(DBG_MENU_HARDWARE_SETUP,
    ("menu_net_interface_setup::form_s514_TE1_options_menu()\n"));

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S514_TE1_INTERFACE);
  str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Physical Medium-> %s\" ",
  //snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Media-----------> %s\" ",
    		MEDIA_DECODE(&linkconf->fe_cfg));
  str += tmp_buff;

  number_of_items++;
}

void menu_hardware_setup::form_TE3_options_menu(string& str, int& number_of_items)
{
  char tmp_buff[MAX_PATH_LENGTH];
  link_def_t * link_def;
  wandev_conf_t *linkconf;

  Debug(DBG_MENU_HARDWARE_SETUP,
    ("menu_net_interface_setup::form_TE3_options_menu()\n"));
  
  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_TE3_INTERFACE);
  str += tmp_buff;

  Debug(DBG_MENU_HARDWARE_SETUP, ("linkconf->fe_cfg.media: 0x%X\n",
    linkconf->fe_cfg.media));
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Physical Medium-> %s\" ",
    (linkconf->fe_cfg.media == WAN_MEDIA_DS3 ? "T3" : "E3"));
  str += tmp_buff;

  number_of_items++;
}

void menu_hardware_setup::form_pci_card_locations_options_menu(string& str, int& number_of_items)
{
  char tmp_buff[MAX_PATH_LENGTH];

  Debug(DBG_MENU_HARDWARE_SETUP,
    ("menu_net_interface_setup::form_pci_card_locations_options_menu()\n"));

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ADVANCED_PCI_CFG);
  str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Advanced PCI Configuration\" ");
  str += tmp_buff;

  number_of_items++;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum SERIAL_CONNECTION_TYPE{
	PERMANENT,
	SWITCHED
};

#define DBG_MENU_HARDWARE_SERIAL_CONNECTION_TYPE 1

menu_hardware_serial_connection_type::
  menu_hardware_serial_connection_type(IN char * lxdialog_path,
                                       IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_HARDWARE_SERIAL_CONNECTION_TYPE,
    ("menu_hardware_serial_connection_type::%s()\n", __FUNCTION__));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_hardware_serial_connection_type::~menu_hardware_serial_connection_type()
{
  Debug(DBG_MENU_HARDWARE_SERIAL_CONNECTION_TYPE,
    ("menu_hardware_serial_connection_type::%s()\n", __FUNCTION__));
}

int menu_hardware_serial_connection_type::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  int number_of_items;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;

  Debug(DBG_MENU_HARDWARE_SERIAL_CONNECTION_TYPE,
    ("menu_hardware_serial_connection_type::%s()\n", __FUNCTION__));

again:
  number_of_items = 2;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_HARDWARE_SERIAL_CONNECTION_TYPE, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;
  menu_str = " ";

  Debug(DBG_MENU_HARDWARE_SERIAL_CONNECTION_TYPE, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_PERMANENT);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Permanent\" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_SWITCHED);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Switched\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect Serial Connection type for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT CONNECTION TYPE",
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
    Debug(DBG_MENU_HARDWARE_SERIAL_CONNECTION_TYPE,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    Debug(DBG_MENU_HARDWARE_SERIAL_CONNECTION_TYPE,
      ("serial select clock: atoi(get_lxdialog_output_string(): %d\n",
      atoi(get_lxdialog_output_string())));

    switch(atoi(get_lxdialog_output_string()))
    {
    case WANOPT_PERMANENT:
      linkconf->connection = WANOPT_PERMANENT;
      exit_dialog = YES;
      break;

    case WANOPT_SWITCHED:
      linkconf->connection = WANOPT_SWITCHED;
      exit_dialog = YES;
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
    case WANOPT_PERMANENT:
    case WANOPT_SWITCHED:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
        "Please select Serial Connection type.");
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum SERIAL_LINE_CODING{
	NRZ,
	NRZI
};

#define DBG_MENU_HARDWARE_SERIAL_LINE_CODING 1

menu_hardware_serial_line_coding::
  menu_hardware_serial_line_coding(IN char * lxdialog_path,
                                   IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_HARDWARE_SERIAL_LINE_CODING,
    ("menu_hardware_serial_line_coding::%s()\n", __FUNCTION__));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_hardware_serial_line_coding::~menu_hardware_serial_line_coding()
{
  Debug(DBG_MENU_HARDWARE_SERIAL_LINE_CODING,
    ("menu_hardware_serial_line_coding::%s()\n", __FUNCTION__));
}

int menu_hardware_serial_line_coding::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  int number_of_items;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;

  Debug(DBG_MENU_HARDWARE_SERIAL_LINE_CODING,
    ("menu_hardware_serial_line_coding::%s()\n", __FUNCTION__));

again:
  number_of_items = 2;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_HARDWARE_SERIAL_LINE_CODING, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;
  menu_str = " ";

  Debug(DBG_MENU_HARDWARE_SERIAL_LINE_CODING, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_NRZ);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"NRZ\" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_NRZI);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"NRZI\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect Serial Line Coding type for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT LINE CODING",
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
    Debug(DBG_MENU_HARDWARE_SERIAL_LINE_CODING,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    Debug(DBG_MENU_HARDWARE_SERIAL_LINE_CODING,
      ("serial select clock: atoi(get_lxdialog_output_string(): %d\n",
      atoi(get_lxdialog_output_string())));

    switch(atoi(get_lxdialog_output_string()))
    {
    case WANOPT_NRZ:
      linkconf->line_coding = WANOPT_NRZ;
      exit_dialog = YES;
      break;

    case WANOPT_NRZI:
      linkconf->line_coding = WANOPT_NRZI;
      exit_dialog = YES;
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
    case WANOPT_NRZ:
    case WANOPT_NRZI:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
        "Please select Serial Line coding.");
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum SERIAL_LINE_IDLE{
	FLAG,
	MARK
};

#define DBG_MENU_HARDWARE_SERIAL_LINE_IDLE 1

menu_hardware_serial_line_idle::
  menu_hardware_serial_line_idle(IN char * lxdialog_path,
                                   IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_HARDWARE_SERIAL_LINE_IDLE,
    ("menu_hardware_serial_line_idle::%s()\n", __FUNCTION__));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_hardware_serial_line_idle::~menu_hardware_serial_line_idle()
{
  Debug(DBG_MENU_HARDWARE_SERIAL_LINE_IDLE,
    ("menu_hardware_serial_line_idle::%s()\n", __FUNCTION__));
}

int menu_hardware_serial_line_idle::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  int number_of_items;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;

  Debug(DBG_MENU_HARDWARE_SERIAL_LINE_IDLE,
    ("menu_hardware_serial_line_idle::%s()\n", __FUNCTION__));

again:
  number_of_items = 2;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_HARDWARE_SERIAL_LINE_IDLE, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;
  menu_str = " ";

  Debug(DBG_MENU_HARDWARE_SERIAL_LINE_IDLE, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_IDLE_FLAG);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Flag\" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WANOPT_IDLE_MARK);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Mark\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect Serial Line Idle for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT LINE IDLE",
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
    Debug(DBG_MENU_HARDWARE_SERIAL_LINE_IDLE,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    Debug(DBG_MENU_HARDWARE_SERIAL_LINE_IDLE,
      ("serial select clock: atoi(get_lxdialog_output_string(): %d\n",
      atoi(get_lxdialog_output_string())));

    switch(atoi(get_lxdialog_output_string()))
    {
    case WANOPT_IDLE_FLAG:
      linkconf->line_idle = WANOPT_IDLE_FLAG;
      exit_dialog = YES;
      break;

    case WANOPT_IDLE_MARK:
      linkconf->line_idle = WANOPT_IDLE_MARK;
      exit_dialog = YES;
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
    case WANOPT_IDLE_FLAG:
    case WANOPT_IDLE_MARK:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
        "Please select Serial Line Idle.");
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////
