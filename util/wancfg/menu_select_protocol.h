/***************************************************************************
                          menu_aft_logical_channel_cfg.h  -  description
                             -------------------
    begin                : Thu Apr 8 2004
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

#ifndef MENU_SELECT_PROTOCOL_H
#define MENU_SELECT_PROTOCOL_H

#include "menu_base.h"
#include "conf_file_reader.h"

/**
  *@author David Rokhvarg
  */

class menu_select_protocol : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];

  conf_file_reader* cfr;

  int form_protocol_list_valid_for_lip_layer(string& str);

  int form_protocol_list_valid_for_hardware(  string& str,
					      int card_type,
					      int card_version,
					      int comm_port);

  int form_protocol_list_valid_for_ADSL(string& menu_str);
  
public: 
  menu_select_protocol(IN char * lxdialog_path,
                       IN conf_file_reader* cfr);

  ~menu_select_protocol();

  int run(OUT int * selected_protocol);
};

#endif
