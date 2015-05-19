/***************************************************************************
                          text_box_yes_no.cpp  -  description
                             -------------------
    begin                : Tue Mar 2 2004
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

#include "text_box_yes_no.h"

#define DBG_YES_NO_TEXT_BOX  1

text_box_yes_no::text_box_yes_no()
{
  snprintf(path_to_text_file, MAX_PATH_LENGTH, "Not initialized!!");
}

text_box_yes_no::~text_box_yes_no()
{
}

int text_box_yes_no::set_configuration(IN char * lxdialog_path,
                                IN char * backtitle,
                                IN char * text_file_name,
                                IN int hight,
                                IN int width)
{
  char tmp_buff[MAX_PATH_LENGTH];

  Debug(DBG_YES_NO_TEXT_BOX, ("text_box_yes_no::set_configuration() - V1\n"));

  snprintf(path_to_text_file, MAX_PATH_LENGTH, "%s%s", wancfg_cfg_dir, text_file_name);

  snprintf(tmp_buff, MAX_PATH_LENGTH,
    "%s --backtitle \"%s\" --clear --textyesno \"%s\" %d %d\n",
    lxdialog_path,
    backtitle,
    path_to_text_file,
    hight,
    width
    );

  return set_lxdialog_path((char*)tmp_buff);
}

int text_box_yes_no::set_configuration(IN char * lxdialog_path,
                                IN int protocol,
                                IN char * text_file_name)
{
  char backtitle[MAX_PATH_LENGTH];

  Debug(DBG_YES_NO_TEXT_BOX, ("text_box_yes_no::set_configuration() - V2\n"));

  snprintf( backtitle, MAX_PATH_LENGTH, "%s: %s",
            WANCFG_PROGRAM_NAME, get_protocol_string(protocol));

  return set_configuration( lxdialog_path,
                            backtitle,
                            text_file_name,
                            YES_NO_TEXT_BOX_HEIGTH,
                            YES_NO_TEXT_BOX_WIDTH);
}

int text_box_yes_no::show(OUT int * selection_index)
{
  int rc;

  Debug(DBG_YES_NO_TEXT_BOX, ("text_box_yes_no::show()\n"));

  rc = execute_command_line(selection_index);

  if(rc == NO){
    return rc;
  }

  switch(*selection_index)
  {
  case YES_NO_TEXT_BOX_BUTTON_YES:
    Debug(DBG_YES_NO_TEXT_BOX, ("YES_NO_TEXT_BOX_BUTTON_YES\n"));
    break;

  case YES_NO_TEXT_BOX_BUTTON_NO:
    Debug(DBG_YES_NO_TEXT_BOX, ("YES_NO_TEXT_BOX_BUTTON_NO\n"));
    break;

  default:
    ERR_DBG_OUT(("Invalid selection for 'Yes/No Text Box'!!\n"));
    rc = NO;
  }

  return rc;
}
