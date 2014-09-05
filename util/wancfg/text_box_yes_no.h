/***************************************************************************
                          text_box_yes_no.h  -  description
                             -------------------
    begin                : Tue Mar 2 2004
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

#ifndef YES_NO_TEXT_BOX_H
#define YES_NO_TEXT_BOX_H

#include <dialog_base.h>

#define YES_NO_TEXT_BOX_HEIGTH 20
#define YES_NO_TEXT_BOX_WIDTH  60

#define YES_NO_TEXT_BOX_BUTTON_YES SELECTION_0
#define YES_NO_TEXT_BOX_BUTTON_NO  SELECTION_1

/**Displays question, allows selection of 'Yes' or 'No'.
  *@author David Rokhvarg
  */

class text_box_yes_no : public dialog_base  {
  char path_to_text_file[MAX_PATH_LENGTH];

public: 
	text_box_yes_no();
	~text_box_yes_no();

	int set_configuration(IN char * lxdialog_path,
                        IN char * backtitle,
                        IN char * path_to_text_file,
                        IN int hight,
                        IN int width);

  //shorter version for convinience
  int set_configuration(IN char * lxdialog_path,
                        IN int protocol,
                        IN char * path_to_text_file);

  int show(OUT int * selection_index);
};

#endif
