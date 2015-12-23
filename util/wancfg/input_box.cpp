/***************************************************************************
                          input_box.cpp  -  description
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

#include "input_box.h"
#include "wancfg.h"

#define DBG_INPUT_BOX 1

/*
/usr/sbin/wanpipe_lxdialog --backtitle "WANPIPE Configuration Utility: Bit Stream Setup" --clear
                   --inputbox "Please specify the active channels" 10 50 ALLLLL 2> lx_dialog_output
*/

input_box::input_box()
{
  Debug(DBG_INPUT_BOX, ("input_box::input_box()\n"));
  //do nothing
}

input_box::~input_box()
{
  Debug(DBG_INPUT_BOX, ("input_box::~input_box()\n"));
  //do nothing
}

int input_box::set_configuration( IN char * lxdialog_path,
                                  IN char * backtitle,
                                  IN char * explanation_text,
                                  IN int hight,
                                  IN int width,
                                  IN char * initial_text)
{
  char tmp_buff[MAX_PATH_LENGTH];

  Debug(DBG_INPUT_BOX, ("input_box::set_configuration(char * input_box_text)\n"));

  snprintf(tmp_buff, MAX_PATH_LENGTH,
    "%s --backtitle \"%s\" --clear --inputbox \"%s\" %d %d \"%s\" 2> %s\n",
    lxdialog_path,
    backtitle,
    explanation_text,
    hight,
    width,
    initial_text,
    LXDIALOG_OUTPUT_FILE_NAME);

  return set_lxdialog_path((char*)tmp_buff);
}

int input_box::show(OUT int * selection_index)
{
  int rc;

  rc = execute_command_line(selection_index);

  if(rc == NO){
    return rc;
  }

  switch(*selection_index)
  {
  case INPUT_BOX_BUTTON_OK:
    Debug(DBG_INPUT_BOX, ("INPUT_BOX_BUTTON_OK\n"));

    if(open_lxdialog_output_file(LXDIALOG_OUTPUT_FILE_NAME) == NO){
      rc = NO;
      break;
    }

    if(read_lxdialog_output_file() == NO){
      rc = NO;
      break;
    }
    break;

  case INPUT_BOX_BUTTON_HELP:
    //do nothing. only return 'YES' and indicate that 'Help' was selected.
    Debug(DBG_INPUT_BOX, ("INPUT_BOX_BUTTON_HELP\n"));
    break;

  default:
    ERR_DBG_OUT(("Invalid selection for 'Input Box'!!\n"));
    rc = NO;
  }

  return rc;
}

