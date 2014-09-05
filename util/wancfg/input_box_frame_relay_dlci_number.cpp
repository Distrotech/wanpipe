/***************************************************************************
                          input_box_frame_relay_dlci_number.cpp  -  description
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

#include "input_box_frame_relay_dlci_number.h"
#include "text_box_help.h"

#define DBG_INPUT_BOX_FRAME_RELAY_DLCI_NUMBER 1

#define LOWEST_VALID_DLCI 16

input_box_frame_relay_DLCI_number::input_box_frame_relay_DLCI_number(IN OUT int* dlci_number)
{
  Debug(DBG_INPUT_BOX_FRAME_RELAY_DLCI_NUMBER,
    ("input_box_frame_relay_DLCI_number::input_box_frame_relay_DLCI_number()\n"));

  this->dlci_number = dlci_number;
}

input_box_frame_relay_DLCI_number::~input_box_frame_relay_DLCI_number()
{
  Debug(DBG_INPUT_BOX_FRAME_RELAY_DLCI_NUMBER,
    ("input_box_frame_relay_DLCI_number::~input_box_frame_relay_DLCI_number()\n"));
}

int input_box_frame_relay_DLCI_number::show(IN char * lxdialog_path,
                                            IN int protocol,
                                            IN char * initial_text,
                                            OUT int * selection_index)
{
  Debug(DBG_INPUT_BOX_FRAME_RELAY_DLCI_NUMBER, ("input_box_frame_relay_DLCI_number::show()\n"));

  char backtitle[MAX_PATH_LENGTH];
  int rc;
  int dlci;

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s Setup", get_protocol_string(protocol));

  if(set_configuration( lxdialog_path,
                        backtitle,
                        "Please specify a DLCI number.",//explanation_text,
                        FRAME_RELAY_DLCI_NUMBER_INPUT_BOX_HIGHT,
                        FRAME_RELAY_DLCI_NUMBER_INPUT_BOX_WIDTH,
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
    Debug(DBG_INPUT_BOX_FRAME_RELAY_DLCI_NUMBER,
      ("DLCI number: %s\n", get_lxdialog_output_string()));

    dlci = atoi(remove_spaces_in_int_string(get_lxdialog_output_string()));

    if(dlci < LOWEST_VALID_DLCI || dlci > HIGHEST_VALID_DLCI){

      text_box tb;
      tb.show_error_message(lxdialog_path, WANCONFIG_FR,
                            "Invalid DLCI number. Min: %d, Max: %d.",
                            LOWEST_VALID_DLCI, HIGHEST_VALID_DLCI);
      goto again;
    }else{
      *dlci_number = dlci;
    }
    goto cleanup;
    break;

  case INPUT_BOX_BUTTON_HELP:
    {
      text_box tb;
      tb.show_error_message(lxdialog_path, WANCONFIG_FR,
                            "Valid DLCI numbers: Min: %d, Max: %d.",
                            LOWEST_VALID_DLCI, HIGHEST_VALID_DLCI);
      goto again;
    }
    break;
  }//switch(*selection_index)

cleanup:
  return rc;
}
