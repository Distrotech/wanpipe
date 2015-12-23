/***************************************************************************
                          menu_net_interface_operation_mode.h  -  description
                             -------------------
    begin                : Fri Mar 26 2004
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

#ifndef MENU_NET_INTERFACE_OPERATION_MODE_H
#define MENU_NET_INTERFACE_OPERATION_MODE_H

#include "menu_base.h"
#include "list_element_chan_def.h"

/**
  *@author David Rokhvarg
  */

class menu_net_interface_operation_mode : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  
  list_element_chan_def* list_element_chan;

  int form_operation_modes_menu_for_protocol( int protocol, string& operation_modes_str,
                                              int& number_of_items_in_menu);

public: 
	
  menu_net_interface_operation_mode(  IN char * lxdialog_path,
                                      IN list_element_chan_def* chan_def);

  ~menu_net_interface_operation_mode();

  int run(OUT int * selection_index);
};

#endif
