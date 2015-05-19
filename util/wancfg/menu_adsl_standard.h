/***************************************************************************
                          menu_adsl_standard.h  -  description
                             -------------------
    begin                : Mon Mar 15 2004
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

#ifndef MENU_ADSL_STANDARD_H
#define MENU_ADSL_STANDARD_H

#include "menu_base.h"

/**
  *@author David Rokhvarg
  */

class menu_adsl_standard : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  wan_adsl_conf_t* adsl_cfg;

public: 
  
  menu_adsl_standard(IN char * lxdialog_path, IN wan_adsl_conf_t* adsl_cfg);
  ~menu_adsl_standard();
  int run(OUT int * selection_index);
};

#endif
