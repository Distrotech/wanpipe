/***************************************************************************
                          menu_list_all_wanpipes.h  -  description
                             -------------------
    begin                : Tue Apr 13 2004
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

#ifndef MENU_LIST_ALL_WANPIPES_H
#define MENU_LIST_ALL_WANPIPES_H

#include "menu_base.h"
#include "conf_file_reader.h"

/**List all possible '.conf' files - both existing (marked as such) and non-existing.
  *@author David Rokhvarg
  */

class menu_list_all_wanpipes : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  char conf_file_path[MAX_PATH_LENGTH];

  conf_file_reader* cfr_tmp;

  int create_new_configuration_file(int wanpipe_number);

public: 
  menu_list_all_wanpipes(IN char * lxdialog_path);
  ~menu_list_all_wanpipes();
  int run(OUT int * selection_index);
};

#endif

