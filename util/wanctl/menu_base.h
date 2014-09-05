/***************************************************************************
                          menu_base.h  -  description
                             -------------------
    begin                : Mon Mar 1 2004
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

#ifndef MENU_BASE_H
#define MENU_BASE_H

#include "dialog_base.h"

//menu buttons
#define MENU_BOX_BUTTON_SELECT  SELECTION_0
#define MENU_BOX_BUTTON_EXIT    SELECTION_1
#define MENU_BOX_BUTTON_BACK    SELECTION_1
#define MENU_BOX_BUTTON_HELP    SELECTION_2

//menu types
#define MENU_BOX_SELECT 1
#define MENU_BOX_BACK   2

//dimensions of a menu
#define MENU_HEIGTH 20
#define MENU_WIDTH  50

//this is the max number of visible items, all the rest will be
//hidden. just scroll to see them.
#define MAX_NUM_OF_VISIBLE_ITEMS_IN_MENU  12

/**Base class for regular menus.
  *@author David Rokhvarg
  */

class menu_base : public dialog_base {

  int menu_type;
  int handle_selection();

public: 
	menu_base();
	~menu_base();

	int set_configuration(IN char menu_type,
                        IN char * lxdialog_path,
                        IN char * title,
                        IN char * backtitle,
                        IN char * explanation_text,
                        IN int hight,
                        IN int width,
                        IN int number_of_items,
                        //number of items may be different
                        ...);

  int set_configuration(IN int reserved,
                        IN char menu_type,
                        IN char * lxdialog_path,
                        IN char * title,
                        IN char * backtitle,
                        IN char * explanation_text,
                        IN int hight,
                        IN int width,
                        IN int number_of_items,
                        IN char * tags_and_items_string);

  int show(OUT int * selection_index);
};

#endif
