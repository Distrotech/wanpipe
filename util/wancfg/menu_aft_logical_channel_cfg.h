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

#ifndef MENU_AFT_LOGICAL_CHANNEL_CFG_H
#define MENU_AFT_LOGICAL_CHANNEL_CFG_H

#include "menu_base.h"
#include "conf_file_reader.h"
#include "objects_list.h"
#include "list_element_chan_def.h"

/**
  *@author David Rokhvarg
  */

class menu_aft_logical_channel_cfg : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];

  conf_file_reader* cfr;
  objects_list* obj_list;
  list_element_chan_def* logical_ch_cfg;

  link_def_t    *link_defs;
  wandev_conf_t *linkconf;

  int display_protocol_cfg(IN int protocol, IN conf_file_reader* local_cfr);
  int handle_protocol_change(unsigned int protocol_selected);

  int aft_update_interface_name( list_element_chan_def* list_el_chan_def,
                                 int config_id,
                                 IN char* timeslot_group_name);

public: 
  menu_aft_logical_channel_cfg(IN char * lxdialog_path,
                               IN conf_file_reader* cfr,
                               IN list_element_chan_def* logical_ch_cfg);

  ~menu_aft_logical_channel_cfg();

  int run(IN int wanpipe_number, OUT int * selection_index, IN int number_of_timeslot_groups);
};

#endif
