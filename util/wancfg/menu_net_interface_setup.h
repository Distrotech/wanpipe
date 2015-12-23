/***************************************************************************
                          menu_net_interface_setup.h  -  description
                             -------------------
    begin                : Tue Mar 23 2004
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

#ifndef MENU_NET_INTERFACE_SETUP_H
#define MENU_NET_INTERFACE_SETUP_H

#include "menu_base.h"
#include "list_element_chan_def.h"

/**
  *@author David Rokhvarg
  */

class menu_net_interface_setup : public menu_base{

  char lxdialog_path[MAX_PATH_LENGTH];
  list_element_chan_def* list_element_logical_ch;
#if 0
  void add_hw_echo_cancel_items(string& menu_str, link_def_t *link_defs, 
				wan_xilinx_conf_t *wan_xilinx_conf);
#endif

  void add_hw_echo_cancel_items(string& menu_str, chan_def_t* chandef, wan_tdmv_conf_t* tdmv_conf);

public: 
  menu_net_interface_setup( IN char * lxdialog_path,
                            IN list_element_chan_def* chan_def);

  ~menu_net_interface_setup();

  int run(OUT int * selection_index);
};

#endif
