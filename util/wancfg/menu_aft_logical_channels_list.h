/***************************************************************************
                          menu_aft_logical_channels_list.h  -  description
                             -------------------
    begin                : Wed Apr 7 2004
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

#ifndef MENU_AFT_LOGICAL_CHANNELS_LIST_H
#define MENU_AFT_LOGICAL_CHANNELS_LIST_H

#include "menu_base.h"
#include "conf_file_reader.h"

/**
  *@author David Rokhvarg
  */

class menu_aft_logical_channels_list : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;
  
  link_def_t    *link_defs;
  wandev_conf_t *linkconf;

  unsigned int max_valid_number_of_logical_channels;
  int create_logical_channels_list_str(string& menu_str);

  int adjust_number_of_groups_of_channels(IN objects_list * obj_list,
                                          IN unsigned int new_number_of_groups_of_channels);
  
  int check_bound_channels_not_overlapping();

public:

  menu_aft_logical_channels_list(IN char * lxdialog_path, IN conf_file_reader* cfr);
  ~menu_aft_logical_channels_list();
  int run(OUT int * selection_index);
};

#endif
