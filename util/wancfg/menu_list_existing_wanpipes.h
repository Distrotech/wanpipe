/***************************************************************************
                          menu_list_existing_wanpipes.h  -  description
                             -------------------
    begin                : Wed Mar 3 2004
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

#ifndef MENU_LIST_EXISTING_WANPIPES_H
#define MENU_LIST_EXISTING_WANPIPES_H

#define DBG_MENU_LIST_EXISTING_WANPIPES 1
#define DBG_AFT_READ  1

#include <menu_base.h>

#include "list_element_chan_def.h"
#include "objects_list.h"
#include "conf_file_reader.h"


/**List existing wanpipe#.conf files.
  *@author David Rokhvarg
  */

class menu_list_existing_wanpipes : public menu_base  {
  char lxdialog_path[MAX_PATH_LENGTH];
  char conf_file_path[MAX_PATH_LENGTH];

public:
  
  menu_list_existing_wanpipes(IN char * lxdialog_path);
  ~menu_list_existing_wanpipes();
  int run(OUT int * selection_index);
};

#endif
