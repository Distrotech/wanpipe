/***************************************************************************
                          input_box_frame_relay_dlci_number.h  -  description
                             -------------------
    begin                : Fri Mar 19 2004
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

#ifndef INPUT_BOX_FRAME_RELAY_DLCI_NUMBER_H
#define INPUT_BOX_FRAME_RELAY_DLCI_NUMBER_H

#include <input_box.h>

/**
  *@author David Rokhvarg
  */

#define FRAME_RELAY_DLCI_NUMBER_INPUT_BOX_HIGHT 10
#define FRAME_RELAY_DLCI_NUMBER_INPUT_BOX_WIDTH 50

class input_box_frame_relay_DLCI_number : public input_box  {

  int* dlci_number;

public: 
	input_box_frame_relay_DLCI_number(IN OUT int* dlci_number);
	~input_box_frame_relay_DLCI_number();

	int show( IN char * lxdialog_path,
            IN int protocol,
            IN char * initial_text,
            OUT int * selection_index);
};

#endif
