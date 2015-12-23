/***************************************************************************
                          dialog_yes_no.h  -  description
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

#ifndef YES_NO_DIALOG_H
#define YES_NO_DIALOG_H

#include "dialog_base.h"

#define YES_NO_BOX_BUTTON_YES SELECTION_0
#define YES_NO_BOX_BUTTON_NO  SELECTION_1

#define YES_NO_BOX_HEIGTH 20
#define YES_NO_BOX_WIDTH  50

/**Dialog for geting Yes or No answer from the user.
  *@author David Rokhvarg
  */

class dialog_yes_no : public dialog_base  {
public: 
	dialog_yes_no();
	~dialog_yes_no();

	int set_configuration(IN char * lxdialog_path,
                        IN char * title,
                        IN char * backtitle,
                        IN char * explanation_text,
                        IN int hight,
                        IN int width);

  //shorter version, for convinience
	int set_configuration(IN char * lxdialog_path,
                        IN char * title,
                        IN char * backtitle,
                        IN char * explanation_text);

  int show(OUT int * selection_index);

};

#endif
