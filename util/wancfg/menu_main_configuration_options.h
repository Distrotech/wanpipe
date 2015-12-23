/***************************************************************************
                          menu_main_configuration_options.h  -  description
                             -------------------
    begin                : Tue Mar 2 2004
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

#ifndef MAIN_MENU_CONFIGURATION_OPTIONS_H
#define MAIN_MENU_CONFIGURATION_OPTIONS_H

#include "menu_base.h"

/**Presents with options: 1. Create New Configration file  2. Edit existing configuration file.
  *@author David Rokhvarg
  */

class menu_main_configuration_options : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];

public: 
	menu_main_configuration_options(IN char * lxdialog_path);
	~menu_main_configuration_options();
  /** No descriptions */
  int run(OUT int * selection_index);
};

#endif
