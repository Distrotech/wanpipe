/***************************************************************************
                          input_box_number_of_dlcis.cpp  -  description
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

#include "input_box_number_of_dlcis.h"
#include "text_box.h"

#define DBG_INPUT_BOX_NUMBER_OF_DLCIS 0

input_box_number_of_dlcis::input_box_number_of_dlcis(IN OUT int* number_of_dlcis)
{
  Debug(DBG_INPUT_BOX_NUMBER_OF_DLCIS,
    ("input_box_number_of_dlcis::input_box_number_of_dlcis()\n"));

  this->number_of_dlcis = number_of_dlcis;
}

input_box_number_of_dlcis::~input_box_number_of_dlcis()
{
  Debug(DBG_INPUT_BOX_NUMBER_OF_DLCIS,
    ("input_box_number_of_dlcis::~input_box_number_of_dlcis()\n"));
}

int input_box_number_of_dlcis::show(IN char * lxdialog_path,
                                    IN int protocol,
                                    IN char * initial_text,
                                    OUT int * selection_index)
{
  Debug(DBG_INPUT_BOX_NUMBER_OF_DLCIS, ("input_box_number_of_dlcis::show()\n"));

  char backtitle[MAX_PATH_LENGTH];
  int rc;
  int dlcis;
  //help text box
  text_box tb;

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s Setup", get_protocol_string(protocol));

  if(set_configuration( lxdialog_path,
                        backtitle,
                        "Please specify the number of DLCIs you need.",//explanation_text,
                        NUMBER_OF_DLCIs_INPUT_BOX_HIGHT,
                        NUMBER_OF_DLCIs_INPUT_BOX_WIDTH,
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
    Debug(DBG_INPUT_BOX_NUMBER_OF_DLCIS,
      ("number of DLCIs: %s\n", get_lxdialog_output_string()));

    dlcis = atoi(get_lxdialog_output_string());

    if(dlcis < 1 || dlcis > MAX_DLCI_NUMBER){

      text_box tb;
      tb.show_error_message(lxdialog_path, WANCONFIG_FR,
                            "Invalid number of DLCIs. Min: %d, Max: %d.",
                            1, MAX_DLCI_NUMBER);
      goto again;
    }else{
      *number_of_dlcis = dlcis;
    }
    goto cleanup;
    break;

  case INPUT_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
    goto again;
    break;

  }//switch(*selection_index)

cleanup:
  return rc;
}
