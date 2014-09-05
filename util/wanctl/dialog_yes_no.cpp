/***************************************************************************
                          dialog_yes_no.cpp  -  description
                             -------------------
    begin                : Mon Mar 1 2004
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

#include "dialog_yes_no.h"

#define DBG_YES_NO_BOX 1

/*
wanpipe_lxdialog --title "the title" --backtitle "the backtitle" --yesno "yes no text" 20 50
*/

dialog_yes_no::dialog_yes_no()
{
}

dialog_yes_no::~dialog_yes_no()
{
}

int dialog_yes_no::set_configuration( IN char * lxdialog_path,
                                      IN char * title,
                                      IN char * backtitle,
                                      IN char * explanation_text,
                                      IN int hight,
                                      IN int width)
{
  char tmp_buff[MAX_PATH_LENGTH];

  Debug(DBG_YES_NO_BOX, ("dialog_yes_no::set_configuration() - V1\n"));

  snprintf(tmp_buff, MAX_PATH_LENGTH,
    "%s --title \"%s\" --backtitle \"%s\" --yesno \"%s\" %d %d\n",
    lxdialog_path,
    title,
    backtitle,
    explanation_text,
    hight,
    width
    );

  return set_lxdialog_path((char*)tmp_buff);
}

int dialog_yes_no::set_configuration( IN char * lxdialog_path,
                                      IN char * title,
                                      IN char * backtitle,
                                      IN char * explanation_text)
{
  Debug(DBG_YES_NO_BOX, ("dialog_yes_no::set_configuration() - V2\n"));

  return set_configuration( lxdialog_path,
                            title,
                            backtitle,
                            explanation_text,
                            YES_NO_BOX_HEIGTH, YES_NO_BOX_WIDTH);
}

int dialog_yes_no::show(OUT int * selection_index)
{
  int rc;

  Debug(DBG_YES_NO_BOX, ("dialog_yes_no::show()\n"));

  rc = execute_command_line(selection_index);

  if(rc == NO){
    return rc;
  }

  switch(*selection_index)
  {
  case YES_NO_BOX_BUTTON_YES:
    Debug(DBG_YES_NO_BOX, ("YES_NO_BOX_BUTTON_YES\n"));
    break;

  case YES_NO_BOX_BUTTON_NO:
    Debug(DBG_YES_NO_BOX, ("YES_NO_BOX_BUTTON_NO\n"));
    break;

  default:
    printf("Invalid selection for 'Yes/No Box'!!\n");
    rc = NO;
  }

  return rc;
}
