/***************************************************************************
                          input_box_active_channels.cpp  -  description
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

#include "input_box_active_channels.h"
#include "text_box.h"

#define DBG_ACTIVE_CHANNELS_INPUT_BOX 1

input_box_active_channels::input_box_active_channels()
{
  Debug(DBG_ACTIVE_CHANNELS_INPUT_BOX,
    ("input_box_active_channels::input_box_active_channels()\n"));
}

input_box_active_channels::~input_box_active_channels()
{
  Debug(DBG_ACTIVE_CHANNELS_INPUT_BOX,
    ("input_box_active_channels::~input_box_active_channels()\n"));
}

int input_box_active_channels::show(IN char * lxdialog_path,
                                    IN int protocol,
                                    IN char * initial_text,
                                    OUT int * selection_index,
                                    IN unsigned char media_type)
{
  Debug(DBG_ACTIVE_CHANNELS_INPUT_BOX, ("input_box_active_channels::show()\n"));

  int rc;
  char backtitle[MAX_PATH_LENGTH];
  text_box tb;

again:

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s Setup", get_protocol_string(protocol));

  if(set_configuration( lxdialog_path,
                        backtitle,
                        "Please specify the timeslots",//explanation_text,
                        ACTIVE_CHANNELS_INPUT_BOX_HIGHT,
                        ACTIVE_CHANNELS_INPUT_BOX_WIDTH,
                        initial_text
                        ) == NO){
    //failed some of the checks.
    return NO;
  }

  rc = input_box::show(selection_index);

  Debug(DBG_ACTIVE_CHANNELS_INPUT_BOX, ("input_box_active_channels: selection_index: %d\n",
    *selection_index));

  switch(*selection_index)
  {
  case INPUT_BOX_BUTTON_OK:

    //all that should be in "input_box_active_channels" object
    active_ch = parse_active_channel(get_lxdialog_output_string(), media_type);	
    if(active_ch == 0){
      tb.show_error_message(lxdialog_path, protocol,
"Failed to parse active channels.\n\
Possible causes:\n\
1. Channels out of valid range for current\n\
Physical Medium.\n\
2. Invalid characters found in the user input.\n\
3. Input is invalid in general.");
		  goto again;
    }
    //passed check, return to user
    break;

  case INPUT_BOX_BUTTON_HELP:
    //each caller should implement it's own help for this iput box.
    //the reason is : active channels may be needed on per 'wanpipe'
    //OR on per 'logical channel' basis.
    break;

  }//switch()

  return rc;
}
