/***************************************************************************
                          input_box_number_of_logical_channels.cpp  -  description
                             -------------------
    begin                : Wed Apr 7 2004
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

#include "input_box_number_of_logical_channels.h"
#include "text_box.h"
#include "conf_file_reader.h"

#define DBG_INPUT_BOX_NUMBER_OF_LOGICAL_CHANNELS 1

input_box_number_of_logical_channels::
  input_box_number_of_logical_channels( IN OUT int* number_of_channels,
                                        IN int media)
{
  Debug(DBG_INPUT_BOX_NUMBER_OF_LOGICAL_CHANNELS,
    ("input_box_number_of_logical_channels::input_box_number_of_logical_channels()\n"));

  this->number_of_channels = number_of_channels;
  this->media = media;
}

input_box_number_of_logical_channels::~input_box_number_of_logical_channels()
{
  Debug(DBG_INPUT_BOX_NUMBER_OF_LOGICAL_CHANNELS,
    ("input_box_number_of_logical_channels::~input_box_number_of_logical_channels()\n"));
}

int input_box_number_of_logical_channels::show( IN char * lxdialog_path,
                                                IN int protocol,
                                                IN char * initial_text,
                                                OUT int * selection_index)
{
  Debug(DBG_INPUT_BOX_NUMBER_OF_LOGICAL_CHANNELS, ("input_box_number_of_dlcis::show()\n"));

  char backtitle[MAX_PATH_LENGTH];
  int rc;
  int number_of_channels;
  int max_valid_number_of_channels =
    (media == WAN_MEDIA_T1 ? NUM_OF_T1_CHANNELS : (NUM_OF_E1_CHANNELS - 1));
  //help text box
  text_box tb;
  conf_file_reader  *local_cfr = (conf_file_reader*)global_conf_file_reader_ptr;
  link_def_t        *link_defs = local_cfr->link_defs;

  if(link_defs->card_version == A200_ADPTR_ANALOG){
    max_valid_number_of_channels = MAX_FXOFXS_CHANNELS;
  }

  if(link_defs->card_version == AFT_ADPTR_ISDN){
    max_valid_number_of_channels = MAX_BRI_TIMESLOTS;
  }

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility ");
//  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
//          "%s Setup", get_protocol_string(protocol));

  if(set_configuration( lxdialog_path,
                        backtitle,
                        "Please specify the number of Timeslot Groups.",//explanation_text,
                        NUMBER_OF_LOGICAL_CHANNELS_INPUT_BOX_HIGHT,
                        NUMBER_OF_LOGICAL_CHANNELS_INPUT_BOX_WIDTH,
                        initial_text
                        ) == NO){
    //failed some of the checks.
    return NO;
  }

again:
  rc = input_box::show(selection_index);

  if(rc == NO){
    //error
    goto cleanup;
  }

  switch(*selection_index)
  {
  case INPUT_BOX_BUTTON_OK:
    Debug(DBG_INPUT_BOX_NUMBER_OF_LOGICAL_CHANNELS,
      ("number of logical channels str: %s\n", get_lxdialog_output_string()));

    number_of_channels = atoi(get_lxdialog_output_string());

    if(number_of_channels < 1 || number_of_channels > max_valid_number_of_channels){

      text_box tb;

      if(link_defs->card_version == A200_ADPTR_ANALOG){

        tb.show_error_message(lxdialog_path, WANCONFIG_AFT,
"Invalid number of Channels. Minimum: 1, Max: %d.", MAX_FXOFXS_CHANNELS);
        goto again;

      }else if(link_defs->card_version == AFT_ADPTR_ISDN){

        tb.show_error_message(lxdialog_path, WANCONFIG_AFT,
"Invalid number of Channels. Minimum: 1, Max: %d.", MAX_BRI_TIMESLOTS);
        goto again;

      }else{
        tb.show_error_message(lxdialog_path, WANCONFIG_AFT,
"Invalid number of %s.\n\
For T1 line Min: %d, Max: %d.\n\
For E1 line Min: %d, Max: %d.",
         TIME_SLOT_GROUPS_STR,
         1, NUM_OF_T1_CHANNELS,
         1, NUM_OF_E1_CHANNELS - 1);
        goto again;
      }
    }else{
      *this->number_of_channels = number_of_channels;
    }
    goto cleanup;

  case INPUT_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
    goto again;

  }//switch(*selection_index)

cleanup:
  return rc;
}
