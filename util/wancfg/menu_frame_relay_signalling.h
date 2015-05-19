/***************************************************************************
                          menu_frame_relay_signalling.h  -  description
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

#ifndef MENU_FRAME_RELAY_SIGNALLING_H
#define MENU_FRAME_RELAY_SIGNALLING_H

#include <menu_base.h>

/**
  *@author David Rokhvarg
  */

class menu_frame_relay_signalling : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];

  wan_fr_conf_t* fr;	/* frame relay configuration */

public: 
	menu_frame_relay_signalling(IN char * lxdialog_path, IN wan_fr_conf_t* fr);
	~menu_frame_relay_signalling();
  int run(OUT int * selection_index);
};

#endif
