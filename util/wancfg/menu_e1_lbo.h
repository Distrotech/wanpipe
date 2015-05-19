/***************************************************************************
                          menu_t1_lbo.h  -  description
                             -------------------
    begin                : Thu Mar 2 2006
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

#ifndef MENU_E1_LBO_H
#define MENU_E1_LBO_H

#include <menu_base.h>
#include "conf_file_reader.h"

/**
  *@author David Rokhvarg
  */

class menu_e1_lbo : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;

public: 
  menu_e1_lbo(IN char * lxdialog_path, IN conf_file_reader* ptr_cfr);
  ~menu_e1_lbo();

  int run(OUT int * selection_index);
};

class menu_e1_signalling_mode : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;

public: 
  menu_e1_signalling_mode(IN char * lxdialog_path, IN conf_file_reader* ptr_cfr);
  ~menu_e1_signalling_mode();

  int run(OUT int * selection_index);
};

#endif
