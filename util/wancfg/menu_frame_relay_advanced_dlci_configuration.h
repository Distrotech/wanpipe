/***************************************************************************
                          menu_frame_relay_advanced_dlci_configuration.h  -  description
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

#ifndef MENU_FRAME_RELAY_ADVANCED_DLCI_CONFIGURATION_H
#define MENU_FRAME_RELAY_ADVANCED_DLCI_CONFIGURATION_H

#include <menu_base.h>
#include "list_element_frame_relay_dlci.h"


/**
  *@author David Rokhvarg
  */

class menu_frame_relay_advanced_dlci_configuration : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  list_element_frame_relay_dlci* dlci_cfg;

public: 
	menu_frame_relay_advanced_dlci_configuration(
                IN char * lxdialog_path,
                IN list_element_frame_relay_dlci* dlci_cfg);
	~menu_frame_relay_advanced_dlci_configuration();

  int run(OUT int * selection_index);

};

#endif
