/***************************************************************************
                          menu_atm_basic_cfg.cpp  -  description
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

#include "wancfg.h"
#include "text_box.h"
#include "input_box.h"

#include "menu_atm_basic_cfg.h"
#include "list_element_atm_interface.h"
#include "menu_atm_interface_configuration.h"

#define DBG_MENU_ATM_BASIC_CFG  1

typedef struct atm_check_struct{
  int vpi;
  int vci;
}atm_check_struct_t;

static atm_check_struct_t vpi_vci_check_table[MAX_NUMBER_OF_NET_INTERFACES_PER_WANPIPE];

enum ATM_BASIC_CFG_OPTIONS{
  //number large enough not to interfere with interface number
  SET_NUMBER_OF_INTERFACES=MAX_NUMBER_OF_NET_INTERFACES_PER_WANPIPE, 
  EMPTY_LINE
};

//it is just a reasonably big number, which should be enough
#define MAX_NUMBER_OF_INTERFACES SET_NUMBER_OF_INTERFACES

char* atm_number_of_interfaces_help_str =
"Number of ATM interfaces\n"
"------------------------\n"
"\n"
"Number of DLCIs supported by this ATM\n"
"connection.\n"
"\n"
"For each ATM interface, a network\n"
"interface will be created.\n"
"Minimum number is 1, maximum is 100.\n"
"\n"
"Each network interface must be configured with\n"
"an IP address.\n";


menu_atm_basic_cfg::menu_atm_basic_cfg(IN char *lxdialog_path,
				       IN list_element_chan_def* parent_element_logical_ch)
{
  Debug(DBG_MENU_ATM_BASIC_CFG, ("menu_atm_basic_cfg::menu_atm_basic_cfg()\n"));

  Debug(DBG_MENU_ATM_BASIC_CFG, ("parent_element_logical_ch->data.name: %s\n",
	parent_element_logical_ch->data.name));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);

  this->parent_list_element_logical_ch = parent_element_logical_ch;

  name_of_parent_layer = parent_element_logical_ch->data.name;

  init_vpi_vci_check_table();
}

menu_atm_basic_cfg::~menu_atm_basic_cfg()
{
  Debug(DBG_MENU_ATM_BASIC_CFG,("menu_atm_basic_cfg::~menu_atm_basic_cfg()\n"));
}

void menu_atm_basic_cfg::init_vpi_vci_check_table()
{
  for(int i = 0; i < MAX_NUMBER_OF_NET_INTERFACES_PER_WANPIPE; i++){
    vpi_vci_check_table[i].vpi = -1;
    vpi_vci_check_table[i].vci = -1;
  }
}

int menu_atm_basic_cfg::run(OUT int * selection_index)
{
  string menu_str;
  int rc, old_number_of_atm_interfaces, new_number_of_atm_interfaces, num_of_visible_items;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  list_element_atm_interface *tmp_atm_if_ptr = NULL;
  objects_list* obj_list;
  //help text box
  text_box tb;

  Debug(DBG_MENU_ATM_BASIC_CFG, ("menu_atm_basic_cfg::run()\n"));

again:
  rc = YES;
  exit_dialog = NO;
  menu_str = " ";

  obj_list = (objects_list*)parent_list_element_logical_ch->next_objects_list;
  if(obj_list == NULL){
    ERR_DBG_OUT(("Invalid 'obj_list' pointer!!\n"));
    return NO;
  }

  //number of interface, plus one menu item for "number of interfaces", plus
  //two to make it more readable
  num_of_visible_items = obj_list->get_size() + 1 + 2;
  if(num_of_visible_items > 8){
	num_of_visible_items = 8;
  }else{
	;//do nothing
  }
 
  //////////////////////////////////////////////////////////////////////////////////////
  create_logical_channels_list_str(menu_str);

  //////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", EMPTY_LINE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \" \" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\n      ATM configuration options for\
\nWan Device: %s", name_of_parent_layer);
//list_element_logical_ch->data.name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "ATM (BASIC) CONFIGURATION",
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
    Debug(DBG_MENU_ATM_BASIC_CFG, ("option selected for editing: %s\n",
		get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case SET_NUMBER_OF_INTERFACES:
      //intialize to be the same  
      old_number_of_atm_interfaces = new_number_of_atm_interfaces = obj_list->get_size();

      get_new_number_of_interfaces(&old_number_of_atm_interfaces, &new_number_of_atm_interfaces);

      if(old_number_of_atm_interfaces != new_number_of_atm_interfaces){
        if(adjust_number_of_logical_channels_in_list(WANCONFIG_LIP_ATM, obj_list,
					       	     parent_list_element_logical_ch->data.name,
					       	     new_number_of_atm_interfaces) == NO){
          rc = NO;
          goto cleanup;
        }
      }
      break;
    
    case EMPTY_LINE:
      //do nothing
      break;

    default:
      //one of the DLCIs was selected for configuration
      Debug(DBG_MENU_ATM_BASIC_CFG, ("default case selection: %d\n", *selection_index));

      tmp_atm_if_ptr = (list_element_atm_interface*)obj_list->find_element(
                              remove_spaces_in_int_string(get_lxdialog_output_string()));

      if(tmp_atm_if_ptr != NULL){
	menu_atm_interface_configuration per_atm_if_cfg(  lxdialog_path,
                                                          obj_list,
                                                          tmp_atm_if_ptr);
        rc = per_atm_if_cfg.run(selection_index, name_of_parent_layer);
        if(rc == YES){
          goto again;
        }

      }else{
	ERR_DBG_OUT(("Failed to find ATM inteface in the list! (%s)\n", get_lxdialog_output_string()));
	rc = NO;
      }
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    switch(atoi(get_lxdialog_output_string()))
    {
    default:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
      break;
    }
    break;

  case MENU_BOX_BUTTON_EXIT:
    //check there is NO duplicate VPI/VCI in any of the interfaces
    init_vpi_vci_check_table();
    if(check_duplicate_vpi_vci_combination(obj_list) == NO){
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

int menu_atm_basic_cfg::check_duplicate_vpi_vci_combination(objects_list* obj_list)
{
  text_box tb;
  list_element_atm_interface *atm_if = NULL;
  wan_atm_conf_if_t *atm_if_cfg;

  atm_if = (list_element_atm_interface*)obj_list->get_first();

  while(atm_if != NULL){

    atm_if_cfg = (wan_atm_conf_if_t*)&atm_if->data.chanconf->u;

    if(find_vpi_vci_in_check_table(atm_if_cfg->vpi, atm_if_cfg->vci) == YES){
      tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED, 
	"Error: Found duplicate VPI/VCI (%d/%d) pair!", atm_if_cfg->vpi, atm_if_cfg->vci);
      return NO;
    }

    //pair was not found, add it to the table
    if(add_vpi_vci_to_check_table(atm_if_cfg->vpi, atm_if_cfg->vci) == NO){
      tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
        "Failed to check for duplicate VPI/VCI pairs! (%d/%d)", atm_if_cfg->vpi, atm_if_cfg->vci);
      return YES;
    }

    //go to next
    atm_if = (list_element_atm_interface*)obj_list->get_next_element(atm_if);

  }//while()

  return YES;
}

int menu_atm_basic_cfg::find_vpi_vci_in_check_table(int vpi, int vci)
{
  for(int i = 0; i < MAX_NUMBER_OF_NET_INTERFACES_PER_WANPIPE; i++){
    if(vpi_vci_check_table[i].vpi == vpi && vpi_vci_check_table[i].vci == vci ){
      return YES;
    }
  }
  return NO;
}

int menu_atm_basic_cfg::add_vpi_vci_to_check_table(int vpi, int vci)
{
  for(int i = 0; i < MAX_NUMBER_OF_NET_INTERFACES_PER_WANPIPE; i++){

    if(vpi_vci_check_table[i].vpi == -1 && vpi_vci_check_table[i].vci == -1 ){
      vpi_vci_check_table[i].vpi = vpi;
      vpi_vci_check_table[i].vci = vci;
      return YES;
    }

  }
  return NO;
}

int menu_atm_basic_cfg::create_logical_channels_list_str(string& menu_str)
{
  char		tmp_buff[MAX_PATH_LENGTH];
  objects_list	*obj_list = (objects_list *)parent_list_element_logical_ch->next_objects_list;
  list_element_atm_interface *list_el_chan_def = NULL;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", SET_NUMBER_OF_INTERFACES);
  menu_str += tmp_buff;

  if(obj_list->get_size() > 0 && obj_list->get_size() <= MAX_NUMBER_OF_INTERFACES){

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Number of interfaces ---> %u\" ", obj_list->get_size());
    menu_str += tmp_buff;

    list_el_chan_def = (list_element_atm_interface*)obj_list->get_first();

    while(list_el_chan_def != NULL){
      //set the tag
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", list_el_chan_def->data.addr);
      menu_str += tmp_buff;

      Debug(DBG_MENU_ATM_BASIC_CFG,
	     ("list_el_chan_def->data.name: %s\n", list_el_chan_def->data.name));
      //create the displayed part
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interface number %s\" ", list_el_chan_def->data.addr);
      menu_str += tmp_buff;

      //go to next
      list_el_chan_def = (list_element_atm_interface*)obj_list->get_next_element(list_el_chan_def);
    }//while()

  }else if(obj_list->get_size() == 0){
      snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Number of interfaces ---> Undefined\" ");
      menu_str += tmp_buff;
  }else{
    ERR_DBG_OUT(("Invalid number of ATM interfaces: %d!!\n", obj_list->get_size()));
    return NO;
  }

  return YES;
}

int menu_atm_basic_cfg::get_new_number_of_interfaces(	
				int *current_number_of_interfaces,
				int *new_number_of_interfaces)
{
  int		tmp, selection_index;
  input_box	inb;
  char		backtitle[MAX_PATH_LENGTH];
  char		explanation_text[MAX_PATH_LENGTH];
  char		initial_text[MAX_PATH_LENGTH];
  text_box	tb;
 
  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s Setup", get_protocol_string(WANCONFIG_LIP_ATM));

again:
  snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify number of ATM interfaces");
  snprintf(initial_text, MAX_PATH_LENGTH, "%d", *current_number_of_interfaces);

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
    Debug(DBG_MENU_ATM_BASIC_CFG, ("num of atm interfaces str: %s\n",
		inb.get_lxdialog_output_string()));

    tmp = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

    if(tmp < 0 || tmp > SET_NUMBER_OF_INTERFACES){
      tb.show_error_message(lxdialog_path, WANCONFIG_LIP_ATM, 
	"Invalid number of ATM interfaces!\n\n%s\n", atm_number_of_interfaces_help_str);
      goto again;
    }else{
      *new_number_of_interfaces = tmp;
    }
    break;

  case INPUT_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, WANCONFIG_LIP_ATM, atm_number_of_interfaces_help_str);
    goto again;

  }//switch(selection_index)
  return 0;   
}

