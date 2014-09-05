/***************************************************************************
                          menu_frame_relay_dlci_configuration.h  -  description
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

#ifndef MENU_ATM_INTERFACE_CONFIGURATION_H
#define MENU_ATM_INTERFACE_CONFIGURATION_H

#include "menu_base.h"
#include "list_element_atm_interface.h"
#include "objects_list.h"

/**
  *@author David Rokhvarg
  */

class menu_atm_interface_configuration : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  objects_list 			*obj_list;
  list_element_atm_interface 	*atm_if;

public:

  menu_atm_interface_configuration(  IN char 		*lxdialog_path,
                                     IN objects_list	*obj_list,
                                     IN list_element_atm_interface *atm_cfg);

  ~menu_atm_interface_configuration();

  int run(OUT int * selection_index, IN char* name_of_underlying_layer);

  int set_vpi_number(wan_atm_conf_if_t *atm_if_cfg);

  int set_vci_number(wan_atm_conf_if_t *atm_if_cfg);

  int get_user_numeric_input(	IN void *initial_value,
				IN char *explanation_text_ptr,
				IN int max_value,
				IN int min_value,
				OUT void *new_value,
				IN int size_of_numeric_type	//char, short, int
				);
};

#endif
