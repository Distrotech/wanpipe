/***************************************************************************
                          menu_ppp_select_authentication_protocol.h  -  description
                             -------------------
    begin                : Fri Apr 30 2004
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

#ifndef MENU_PPP_SELECT_AUTHENTICATION_PROTOCOL_H
#define MENU_PPP_SELECT_AUTHENTICATION_PROTOCOL_H

#include "menu_base.h"
#include "conf_file_reader.h"

/**
  *@author David Rokhvarg
  */

class menu_ppp_select_authentication_protocol : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  list_element_chan_def* list_el_chan_def;

public:
	
  menu_ppp_select_authentication_protocol(  IN char * lxdialog_path,
                                            IN list_element_chan_def* list_el_chan_def);

  ~menu_ppp_select_authentication_protocol();

  int run(OUT int * selection_index);
};

#endif
