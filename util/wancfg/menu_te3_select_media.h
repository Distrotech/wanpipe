/***************************************************************************
                          menu_te1_select_media.h  -  description
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

#ifndef MENU_TE3_SELECT_MEDIA_H
#define MENU_TE3_SELECT_MEDIA_H

#include "menu_base.h"
#include "conf_file_reader.h"

/**
  *@author David Rokhvarg
  */

class menu_te3_select_media : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;

public: 
	menu_te3_select_media(  IN char * lxdialog_path,
                          IN conf_file_reader* ptr_cfr);
  ~menu_te3_select_media();

  int run(OUT int * selection_index);
};

#endif
