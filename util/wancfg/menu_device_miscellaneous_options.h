/***************************************************************************
                          menu_device_miscellaneous_options.h  -  description
                             -------------------
    begin                : Fri Apr 2 2004
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

#ifndef MENU_DEVICE_MISCELLANEOUS_OPTIONS_H
#define MENU_DEVICE_MISCELLANEOUS_OPTIONS_H

#include "menu_base.h"
#include "conf_file_reader.h"

/**
  *@author David Rokhvarg
  */

class menu_device_miscellaneous_options : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;

  int handle_start_script_selection();
  int handle_stop_script_selection();

public: 
	
  menu_device_miscellaneous_options(IN char * lxdialog_path, IN conf_file_reader* cfr);
  ~menu_device_miscellaneous_options();
  int run(OUT int * selection_index);
};

#endif
