/***************************************************************************
                          text_box.cpp  -  description
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

#include "text_box.h"

#define DBG_TEXT_BOX 1
/*
wanpipe_lxdialog --backtitle "the backtitle" --textbox "path to text file" 20 60
*/

text_box::text_box()
{
}

text_box::~text_box()
{
}

int text_box::set_configuration(IN char * lxdialog_path,
                                IN char * backtitle,
                                IN char * path_to_text_file,
                                IN int hight,
                                IN int width)
{
  char tmp_buff[MAX_PATH_LENGTH];

  Debug(DBG_TEXT_BOX, ("text_box::set_configuration() - V1\n"));

  snprintf(tmp_buff, MAX_PATH_LENGTH,
    "%s --backtitle \"%s\" --textbox \"%s\" %d %d\n",
    lxdialog_path,
    backtitle,
    path_to_text_file,
    hight,
    width
    );

  return set_lxdialog_path((char*)tmp_buff);
}

int text_box::set_configuration(IN char * lxdialog_path,
                                IN int protocol,
                                IN char * path_to_text_file)
{
  char backtitle[MAX_PATH_LENGTH];

  Debug(DBG_TEXT_BOX, ("text_box::set_configuration() - V2\n"));

  snprintf( backtitle, MAX_PATH_LENGTH, "%s: %s",
            WANCFG_PROGRAM_NAME, get_protocol_string(protocol));

  return set_configuration( lxdialog_path,
                            backtitle,
                            path_to_text_file,
                            TEXT_BOX_HEIGTH,
                            TEXT_BOX_WIDTH);
}

int text_box::show()
{
  int selection_index;

  Debug(DBG_TEXT_BOX, ("text_box::show()\n"));

  return execute_command_line(&selection_index);
}

void text_box::show_error_message(IN char * lxdialog_path,
                                  IN int protocol,
                                  IN char* format, ...)
{
  FILE * err_msg_file;
  char dbg_tmp_buff[LEN_OF_DBG_BUFF];
	va_list ap;
  char* err_msg_file_name = "./wancfg_err_msg_file_name";
  char* remove_err_msg_file = "rm -rf wancfg_err_msg_file_name";

	va_start(ap, format);
  vsnprintf(dbg_tmp_buff, LEN_OF_DBG_BUFF, format, ap);
	va_end(ap);

  //open temporary file
  err_msg_file = fopen(err_msg_file_name, "w");
  if(err_msg_file == NULL){
    ERR_DBG_OUT(("Failed to open 'err_msg_file' %s file for writing!\n", err_msg_file_name));
    return;
  }

  fputs(dbg_tmp_buff, err_msg_file);
  fclose(err_msg_file);

  set_configuration(lxdialog_path,
                    protocol,
                    err_msg_file_name);

  show();

  //remove the temporary file
  system(remove_err_msg_file);
}

void text_box::show_help_message(IN char * lxdialog_path,
                                  IN int protocol,
                                  IN char* format, ...)
{
  FILE * help_msg_file;
  char dbg_tmp_buff[LEN_OF_DBG_BUFF];
	va_list ap;
  char* help_msg_file_name = "./wancfg_help_msg_file_name";
  char* remove_help_msg_file = "rm -rf wancfg_help_msg_file_name";

	va_start(ap, format);
  vsnprintf(dbg_tmp_buff, LEN_OF_DBG_BUFF, format, ap);
	va_end(ap);

  //open temporary file
  help_msg_file = fopen(help_msg_file_name, "w");
  if(help_msg_file == NULL){
    ERR_DBG_OUT(("Failed to open 'help_msg_file' %s file for writing!\n", help_msg_file_name));
    return;
  }

  fputs(dbg_tmp_buff, help_msg_file);
  fclose(help_msg_file);

  set_configuration(lxdialog_path,
                    protocol,
                    help_msg_file_name);

  show();

  //remove the temporary file
  system(remove_help_msg_file);
}
