/***************************************************************************
                          menu_atm_basic_cfg.h  -  description
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

#ifndef MENU_ATM_BASIC_CFG_H
#define MENU_ATM_BASIC_CFG_H

#include "menu_base.h"
#include "conf_file_reader.h"

/**Presents basic ATM configuration options.
  *@author David Rokhvarg
  */

class menu_atm_basic_cfg : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  
  list_element_chan_def* parent_list_element_logical_ch;
  char* name_of_parent_layer;

  int create_logical_channels_list_str(string& menu_str);
  int get_new_number_of_interfaces(int *current_number_of_interfaces,
				   int *new_number_of_interfaces);

  void init_vpi_vci_check_table();
  int check_duplicate_vpi_vci_combination(objects_list* obj_list);
  int find_vpi_vci_in_check_table(int vpi, int vci);
  int add_vpi_vci_to_check_table(int vpi, int vci);

/*	  
  wan_fr_conf_t* fr;	// frame relay configuration - in 'profile' section. has to be THE SAME for all
  			// dlcis belonging to this frame relay device
*/

public: 

  menu_atm_basic_cfg(
		  IN char * lxdialog_path,
		  IN list_element_chan_def* parent_element_logical_ch);
  
  ~menu_atm_basic_cfg();
  
  int run(OUT int * selection_index);
};

#endif
