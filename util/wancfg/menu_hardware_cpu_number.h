/***************************************************************************
                          menu_hardware_cpu_number.h  -  description
                             -------------------
    begin                : Mon Apr 26 2004
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

#ifndef MENU_HARDWARE_CPU_NUMBER_H
#define MENU_HARDWARE_CPU_NUMBER_H

#include <menu_base.h>
#include "conf_file_reader.h"

/**
  *@author David Rokhvarg
  */

class menu_hardware_cpu_number : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;

public: 
	menu_hardware_cpu_number( IN char * lxdialog_path,
                            IN conf_file_reader* ptr_cfr);
  ~menu_hardware_cpu_number();

  int run(OUT int * selection_index);
};

#endif
