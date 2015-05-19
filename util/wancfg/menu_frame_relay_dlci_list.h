/***************************************************************************
                          menu_frame_relay_dlci_list.h  -  description
                             -------------------
    begin                : Mon Mar 15 2004
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

#ifndef MENU_FRAME_RELAY_DLCI_LIST_H
#define MENU_FRAME_RELAY_DLCI_LIST_H

#include "menu_base.h"
#include "conf_file_reader.h"

/**
  *@author David Rokhvarg
  */

class menu_frame_relay_dlci_list : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  
  list_element_chan_def* parent_list_element_logical_ch;
  
  char* name_of_parent_layer;
  
  wan_fr_conf_t* fr;	// frame relay configuration - in 'profile' section. has to be THE SAME for all
  			// dlcis belonging to this frame relay device
  
  int create_logical_channels_list_str(string& menu_str);

public:

  menu_frame_relay_dlci_list( IN char * lxdialog_path,
			      IN list_element_chan_def* parent_element_logical_ch);
  
  ~menu_frame_relay_dlci_list();

  int run(OUT int * selection_index);

};

#endif
