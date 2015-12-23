/***************************************************************************
                          menu_ppp_basic_cfg.h  -  description
                             -------------------
    begin                : Tue Apr 6 2004
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

#ifndef MENU_PPP_BASIC_CFG_H
#define MENU_PPP_BASIC_CFG_H

#include "menu_base.h"
#include "wancfg.h"
#include "list_element_chan_def.h"

/**
  *@author David Rokhvarg
  */

class menu_ppp_basic_cfg : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  list_element_chan_def* parent_list_element_logical_ch;
  char* name_of_parent_layer;
  
public: 
  menu_ppp_basic_cfg(	IN char * lxdialog_path,
			IN list_element_chan_def* parent_element_logical_ch);
  
  ~menu_ppp_basic_cfg();

  int run(OUT int * selection_index);
};

#endif
