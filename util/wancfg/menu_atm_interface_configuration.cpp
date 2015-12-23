/***************************************************************************
                          menu_atm_interface_configuration.cpp  -  description
                             -------------------
    begin                : Tue Oct 11 2005
    copyright            : (C) 2005 by David Rokhvarg
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

#include "menu_atm_interface_configuration.h"
#include "text_box.h"
#include "input_box.h"
#include "menu_adsl_encapsulation.h"//reusing for AFT ATM
#include "text_box_yes_no.h"

#define DBG_MENU_ATM_INTERFACE_CONFIGURATION 1

#define MAX_VPI_VCI_NUMBER	60000
#define MAX_UCHAR_VALUE		256

enum {
	SET_VPI_NUMBER=1,
	SET_VCI_NUMBER,
	ENCAPSULATION_MODE,

	ATM_OAM_LOOPBACK,		//atm_oam_loopback
	ATM_OAM_LOOPBACK_INTERVAL,	//atm_oam_loopback_intr

	ATM_OAM_CONTINUITY_CHECK,	//atm_oam_continuity
	ATM_OAM_CONTINUITY_CHECK_INTERVAL,//atm_oam_continuity_intr

	ATM_ARP,			//atm_arp
	ATM_ARP_INTERVAL,		//atm_arp_intr

	MTU,
	EMPTY_LINE
};

menu_atm_interface_configuration::
	menu_atm_interface_configuration(IN char * lxdialog_path,
                                         IN objects_list* obj_list,
                                         IN list_element_atm_interface *atm_if)
{
  Debug(DBG_MENU_ATM_INTERFACE_CONFIGURATION,
    ("menu_atm_interface_configuration::menu_atm_interface_configuration()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);

  this->obj_list = obj_list;
  this->atm_if  = atm_if;
}

menu_atm_interface_configuration::~menu_atm_interface_configuration()
{
  Debug(DBG_MENU_ATM_INTERFACE_CONFIGURATION,
    ("menu_atm_interface_configuration::~menu_atm_interface_configuration()\n"));
}

int menu_atm_interface_configuration::run( OUT int *selection_index,
                                           IN char *name_of_underlying_layer)
{
  string	menu_str;
  int		rc, exit_dialog = NO, tmp;
  char		tmp_buff[MAX_PATH_LENGTH];
  int		num_of_visible_items;
  wan_atm_conf_if_t *atm_if_cfg;

  Debug(DBG_MENU_ATM_INTERFACE_CONFIGURATION,
    ("menu_atm_interface_configuration::run()\n"));

again:
  rc = YES;

  if(atm_if ==  NULL){
    ERR_DBG_OUT(("ATM interface configuration pointer is NULL (atm_if) !!!\n"));
    rc = NO;
    goto cleanup;
  }

  menu_str = " ";
  atm_if_cfg = (wan_atm_conf_if_t*)&atm_if->data.chanconf->u;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ENCAPSULATION_MODE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"EncapMode-> %s\" ",
	ADSL_ENCAPSULATION_DECODE(atm_if_cfg->encap_mode));
  menu_str += tmp_buff;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", SET_VPI_NUMBER);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"VPI number--> %d\" ", atm_if_cfg->vpi);
  menu_str += tmp_buff;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", SET_VCI_NUMBER);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"VCI number--> %d\" ", atm_if_cfg->vci);
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_OAM_LOOPBACK);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"OAM Loopback--> %s\" ", 
	(atm_if_cfg->atm_oam_loopback == WANOPT_YES ? "Yes" : "No"));
  menu_str += tmp_buff;

  if(atm_if_cfg->atm_oam_loopback == WANOPT_YES){
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_OAM_LOOPBACK_INTERVAL);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"OAM Loopback Interval--> %d\" ", 
	atm_if_cfg->atm_oam_loopback_intr);
    menu_str += tmp_buff;
  }
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_OAM_CONTINUITY_CHECK);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"OAM Continuity Check--> %s\" ", 
	(atm_if_cfg->atm_oam_continuity == WANOPT_YES ? "Yes" : "No"));
  menu_str += tmp_buff;

  if(atm_if_cfg->atm_oam_continuity == WANOPT_YES){
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_OAM_CONTINUITY_CHECK_INTERVAL);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"OAM Continuity Check Interval--> %d\" ", 
	atm_if_cfg->atm_oam_continuity_intr);
    menu_str += tmp_buff;
  }
  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
  menu_str += tmp_buff;
  //////////////////////////////////////////////////////////////////////////////////////
  if(atm_if_cfg->encap_mode == RFC_MODE_ROUTED_IP_LLC){
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_ARP);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"ATM ARP--> %s\" ", 
	(atm_if_cfg->atm_arp == WANOPT_YES ? "Yes" : "No"));
    menu_str += tmp_buff;

    if(atm_if_cfg->atm_arp == WANOPT_YES){
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_ARP_INTERVAL);
      menu_str += tmp_buff;
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"ATM ARP Interval--> %d\" ", 
  	atm_if_cfg->atm_arp_intr);
      menu_str += tmp_buff;
    }
  }//if(atm_if_cfg->encap_mode == RFC_MODE_ROUTED_IP_LLC)

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\
\nSet per - ATM interface cofiguration.");

  num_of_visible_items = 8;

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "ATM INTERFACE CONFIGURATION",
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
    Debug(DBG_MENU_ATM_INTERFACE_CONFIGURATION,("option selected for editing: %s\n",
		get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case SET_VPI_NUMBER:
      set_vpi_number(atm_if_cfg);
      break;

    case SET_VCI_NUMBER:
      set_vci_number(atm_if_cfg);
      break;

    case ENCAPSULATION_MODE:
      { 
        wan_adsl_conf_t adsl_cfg;
        menu_adsl_encapsulation adsl_encapsulation(lxdialog_path, &adsl_cfg);
        if(adsl_encapsulation.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }else{
          atm_if_cfg->encap_mode = adsl_cfg.EncapMode;
        }
      }
      break;

    case ATM_OAM_LOOPBACK:		//atm_oam_loopback
      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s ATM OAM Loopback?",
        (atm_if_cfg->atm_oam_loopback == WANOPT_NO ? "Enable" : "Disable"));

      if(yes_no_question(   selection_index, lxdialog_path,
                            NO_PROTOCOL_NEEDED, tmp_buff) == NO){
        return NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(atm_if_cfg->atm_oam_loopback == WANOPT_NO){
          //was disabled - enable
          atm_if_cfg->atm_oam_loopback = WANOPT_YES;
        }else{
          //was enabled - disable
          atm_if_cfg->atm_oam_loopback = WANOPT_NO;
        }
        break;
      }
      break;

    case ATM_OAM_LOOPBACK_INTERVAL:	//atm_oam_loopback_intr
      if(get_user_numeric_input(
		&atm_if_cfg->atm_oam_loopback_intr,
		(char*)"OAM Loopback Interval",	
		MAX_UCHAR_VALUE,				
		1,				
		&tmp,			
		sizeof(atm_if_cfg->atm_oam_loopback_intr)
		) == 0){
        atm_if_cfg->atm_oam_loopback_intr = tmp;
      }
      break;

    case ATM_OAM_CONTINUITY_CHECK:	//atm_oam_continuity
       snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s ATM OAM Loopback?",
        (atm_if_cfg->atm_oam_loopback == WANOPT_NO ? "Enable" : "Disable"));

      if(yes_no_question(   selection_index, lxdialog_path,
                            NO_PROTOCOL_NEEDED, tmp_buff) == NO){
        return NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(atm_if_cfg->atm_oam_loopback == WANOPT_NO){
          //was disabled - enable
          atm_if_cfg->atm_oam_loopback = WANOPT_YES;
        }else{
          //was enabled - disable
          atm_if_cfg->atm_oam_loopback = WANOPT_NO;
        }
        break;
      }
      break;

    case ATM_OAM_CONTINUITY_CHECK_INTERVAL://atm_oam_continuity_intr
      if(get_user_numeric_input(
		&atm_if_cfg->atm_oam_continuity_intr,
		(char*)"OAM Continuity Check Interval",	
		MAX_UCHAR_VALUE,				
		1,				
		&tmp,			
		sizeof(atm_if_cfg->atm_oam_continuity_intr)
		) == 0){
        atm_if_cfg->atm_oam_continuity_intr = tmp;
      }
      break;

    case ATM_ARP:			//atm_arp
      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s ATM ARP?",
        (atm_if_cfg->atm_arp == WANOPT_NO ? "Enable" : "Disable"));

      if(yes_no_question(   selection_index, lxdialog_path,
                            NO_PROTOCOL_NEEDED, tmp_buff) == NO){
        return NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(atm_if_cfg->atm_arp == WANOPT_NO){
          //was disabled - enable
          atm_if_cfg->atm_arp = WANOPT_YES;
        }else{
          //was enabled - disable
          atm_if_cfg->atm_arp = WANOPT_NO;
        }
        break;
      }
      break;

    case ATM_ARP_INTERVAL:
      if(get_user_numeric_input(
		&atm_if_cfg->atm_arp_intr,
		(char*)"ATM ARP Interval",	
		MAX_UCHAR_VALUE,				
		1,				
		&tmp,			
		sizeof(atm_if_cfg->atm_arp_intr)
		) == 0){
        atm_if_cfg->atm_arp_intr = tmp;
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

      Debug(DBG_MENU_ATM_INTERFACE_CONFIGURATION,("HELP option selected: %s\n",
		get_lxdialog_output_string()));

      switch(atoi(get_lxdialog_output_string()))
      {
      default:
        tb.show_help_message(lxdialog_path, WANCONFIG_LIP_ATM,
		option_not_implemented_yet_help_str);
        break;
      }
      goto again;
    }
    break;

  case MENU_BOX_BUTTON_EXIT:
    //no check is done here because each menu item checks
    //the input separatly.
    rc = YES;
    exit_dialog = YES;
    break;
  }//switch(*selection_index)

  if(exit_dialog == NO){
    goto again;
  }

cleanup:
  Debug(DBG_MENU_ATM_INTERFACE_CONFIGURATION,
    ("menu_atm_interface_configuration::run(): rc: %d\n", rc));

  return rc;
}

int menu_atm_interface_configuration::set_vpi_number(wan_atm_conf_if_t *atm_if_cfg)
{
  int tmp;
  if(get_user_numeric_input(
		&atm_if_cfg->vpi,	//IN int *initial_value,
		(char*)"VPI number",	//IN char *explanation_text_ptr,
		MAX_VPI_VCI_NUMBER,	//IN int max_value,
		0,			//IN int min_value,
		&tmp,			//OUT int *new_value
		sizeof(atm_if_cfg->vpi)	//IN int size_of_numeric_type	//char, short, int
		) == 0){
    atm_if_cfg->vpi = tmp;
  }
  return 0;
}

int menu_atm_interface_configuration::set_vci_number(wan_atm_conf_if_t *atm_if_cfg)
{
  int tmp;
  if(get_user_numeric_input(
		&atm_if_cfg->vci,	//IN int *initial_value,
		(char*)"VCI number",	//IN char *explanation_text_ptr,
		MAX_VPI_VCI_NUMBER,	//IN int max_value,
		0,			//IN int min_value,
		&tmp,			//OUT int *new_value
		sizeof(atm_if_cfg->vci)	//IN int size_of_numeric_type	//char, short, int
		) == 0){
    atm_if_cfg->vci = tmp;
  }
  return 0;
}

int menu_atm_interface_configuration::get_user_numeric_input(	
				IN void *initial_value_ptr,
				IN char *explanation_text_ptr,
				IN int max_value,
				IN int min_value,
				OUT void *new_value_ptr,
				IN int size_of_numeric_type	//char, short, int
				)
{
  int		tmp, selection_index, rc=0, initial_value;
  input_box	inb;
  char		backtitle[MAX_PATH_LENGTH];
  char		explanation_text[MAX_PATH_LENGTH];
  char		initial_text[MAX_PATH_LENGTH];
  text_box	tb;

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s Setup", get_protocol_string(WANCONFIG_LIP_ATM));

again:
  switch(size_of_numeric_type)
  {
  case sizeof(char):
    initial_value = *(unsigned char*)initial_value_ptr;
    break;

  case sizeof(short):
    initial_value = *(unsigned short*)initial_value_ptr;
    break;

  case sizeof(int):
    initial_value = *(unsigned int*)initial_value_ptr;
    break;

  default:
    initial_value = *(unsigned char*)initial_value_ptr;
  }

  snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify %s", explanation_text_ptr);
  snprintf(initial_text, MAX_PATH_LENGTH, "%d", initial_value);

  inb.set_configuration(  lxdialog_path,
                          backtitle,
                          explanation_text,
                          INPUT_BOX_HIGTH,
                          INPUT_BOX_WIDTH,
                          initial_text);
  inb.show(&selection_index);

  switch(selection_index)
  {
  case INPUT_BOX_BUTTON_OK:
    Debug(DBG_MENU_ATM_INTERFACE_CONFIGURATION, ("num of atm interfaces str: %s\n",
		inb.get_lxdialog_output_string()));

    tmp = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

    if(tmp < min_value || tmp > max_value){
      tb.show_error_message(lxdialog_path, WANCONFIG_LIP_ATM, 
	"Invalid input for\n\n%s\n", explanation_text_ptr);
      rc = 1;
      goto done;
    }else{
      switch(size_of_numeric_type)
      {
      case sizeof(char):
        *(unsigned char*)new_value_ptr = tmp;
        break;

      case sizeof(short):
        *(unsigned short*)new_value_ptr = tmp;
        break;

      case sizeof(int):
        *(unsigned int*)new_value_ptr = tmp;
        break;

      default:
        *(unsigned char*)new_value_ptr = tmp;
      }
      rc = 0;
      goto done;
    }
    break;

  case INPUT_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, WANCONFIG_LIP_ATM, option_not_implemented_yet_help_str);
    goto again;
  }//switch(selection_index)

done:
  return rc;   
}

