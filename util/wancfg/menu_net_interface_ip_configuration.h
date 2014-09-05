/***************************************************************************
                          menu_net_interface_ip_configuration.h  -  description
                             -------------------
    begin                : Mon Mar 29 2004
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

#ifndef MENU_NET_INTERFACE_IP_CONFIGURATION_H
#define MENU_NET_INTERFACE_IP_CONFIGURATION_H

#include "menu_base.h"
#include "net_interface_file_reader.h"
#include "list_element_chan_def.h"

/**
  *@author David Rokhvarg
  */

class menu_net_interface_ip_configuration : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  list_element_chan_def* list_element_chan;

public: 
	
  menu_net_interface_ip_configuration(  IN char * lxdialog_path,
                                        IN list_element_chan_def* chan_def
                                        );

  ~menu_net_interface_ip_configuration();

  int run(OUT int * selection_index, IN OUT net_interface_file_reader& net_if_file_reader);
};

#endif
