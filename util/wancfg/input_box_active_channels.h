/***************************************************************************
                          input_box_active_channels.h  -  description
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

#ifndef ACTIVE_CHANNELS_INPUT_BOX_H
#define ACTIVE_CHANNELS_INPUT_BOX_H

#include "input_box.h"

#define ACTIVE_CHANNELS_INPUT_BOX_HIGHT 10
#define ACTIVE_CHANNELS_INPUT_BOX_WIDTH 50

/**Creates input box cpecificly for entering/viewing active T1/E1 channels.
  *@author David Rokhvarg
  */

class input_box_active_channels : public input_box {
public:
  //holds parsed active channels
	unsigned long active_ch;

	input_box_active_channels();
	~input_box_active_channels();

	int show( IN char * lxdialog_path,
            IN int protocol,
            IN char * initial_text,
            OUT int * selection_index,
            IN unsigned char media_type);
};

#endif
