/***************************************************************************
                          input_box_number_of_dlcis.h  -  description
                             -------------------
    begin                : Wed Mar 17 2004
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

#ifndef INPUT_BOX_NUMBER_OF_DLCIS_H
#define INPUT_BOX_NUMBER_OF_DLCIS_H

#include <input_box.h>

/**
  *@author David Rokhvarg
  */

#define NUMBER_OF_DLCIs_INPUT_BOX_HIGHT 10
#define NUMBER_OF_DLCIs_INPUT_BOX_WIDTH 50

class input_box_number_of_dlcis : public input_box  {
  int* number_of_dlcis;

public: 
	input_box_number_of_dlcis(IN OUT int* number_of_dlcis);
	~input_box_number_of_dlcis();

	int show( IN char * lxdialog_path,
            IN int protocol,
            IN char * initial_text,
            OUT int * selection_index);
};

#endif
