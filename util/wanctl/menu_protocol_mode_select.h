/***************************************************************************
                          menu_protocol_mode_select.h  -  description
                             -------------------
    begin                : Fri Mar 12 2004
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

#ifndef MENU_PROTOCOL_MODE_SELECT_H
#define MENU_PROTOCOL_MODE_SELECT_H

#include <menu_base.h>
#include "conf_file_reader.h"

#define DBG_MENU_PROTOCOL_MODE_SELECT  1

/**Presents list of valid Protocols and Modes. Returns user selection to the caller.
  *@author David Rokhvarg
  */

class menu_protocol_mode_select : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;

public:
	menu_protocol_mode_select(IN char * lxdialog_path, IN conf_file_reader* cfr);
	~menu_protocol_mode_select();

  int run(OUT int * selected_protocol);
};

#endif
