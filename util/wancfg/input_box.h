/***************************************************************************
                          input_box.h  -  description
                             -------------------
    begin                : Fri Feb 27 2004
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

#ifndef INPUT_BOX_H
#define INPUT_BOX_H

#include "dialog_base.h"

//input box definitions
#define INPUT_BOX_BUTTON_OK   SELECTION_0
#define INPUT_BOX_BUTTON_HELP SELECTION_1

#define INPUT_BOX_HIGTH 10
#define INPUT_BOX_WIDTH 50

/**Extends the dialog_base class. Provides functionality of an input box with two buttons "OK" and "Help".
  *@author David Rokhvarg
  */

class input_box : public dialog_base  {

public:
	input_box();
	~input_box();

	int set_configuration(IN char * lxdialog_path,
                        IN char * backtitle,
                        IN char * explanation_text,
                        IN int hight,
                        IN int width,
                        IN char * initial_text);

  int show(OUT int * selection_index);

};

#endif
