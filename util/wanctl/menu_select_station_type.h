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

#ifndef MENU_SELECT_STATION_TYPE_H
#define MENU_SELECT_STATION_TYPE_H

#include <menu_base.h>
#include "conf_file_reader.h"

class menu_select_station_type : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];

  conf_file_reader* cfr;

public: 
	menu_select_station_type(IN char * lxdialog_path, IN conf_file_reader* cfr);
	~menu_select_station_type();
  /** No descriptions */
  int run(OUT int * selection_index);
};

#endif
