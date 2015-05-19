/***************************************************************************
                          menu_ppp_ip_mode.h  -  description
                             -------------------
    begin                : Fri Apr 30 2004
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

#ifndef MENU_PPP_IP_MODE_H
#define MENU_PPP_IP_MODE_H

#include "menu_base.h"
#include "wancfg.h"

/**
  *@author David Rokhvarg
  */

class menu_ppp_ip_mode : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];

public:
	
  menu_ppp_ip_mode( IN char * lxdialog_path);

  ~menu_ppp_ip_mode();

  int run(OUT unsigned char * new_ip_mode);
};

#endif
