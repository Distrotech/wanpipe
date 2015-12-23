/***************************************************************************
                          menu_net_interface_miscellaneous_options.h  -  description
                             -------------------
    begin                : Thu Mar 25 2004
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

#ifndef MENU_NET_INTERFACE_MISCELLANEOUS_OPTIONS_H
#define MENU_NET_INTERFACE_MISCELLANEOUS_OPTIONS_H

#include "menu_base.h"
#include "list_element_chan_def.h"

/**Per Network Interface options wich are rarely changed.
  *@author David Rokhvarg
  */

class menu_net_interface_miscellaneous_options : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  list_element_chan_def* list_element_chan;
  char tmp_buff[MAX_PATH_LENGTH];

  int handle_start_script_selection(chan_def_t* chandef);
  int handle_stop_script_selection(chan_def_t* chandef);

public: 

  menu_net_interface_miscellaneous_options( IN char * lxdialog_path,
                                            IN list_element_chan_def* list_element_chan);

  ~menu_net_interface_miscellaneous_options();

  int run(OUT int * selection_index);
};

#endif
