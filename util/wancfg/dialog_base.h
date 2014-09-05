/***************************************************************************
                          dialog_base.h  -  description
                             -------------------
    begin                : Thu Feb 26 2004
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

#ifndef DIALOG_BASE_H
#define DIALOG_BASE_H

#include "wancfg.h"

#include <sys/wait.h> //for WIFEXITED() and WEXITSTATUS()

/**Class implementing common behaivior for all types of dialogs.
  *@author David Rokhvarg
  */

class dialog_base {
  //this buffer must contain full path to lxdialog and the command line for
  //lxdialog.
  char lxdialog_path[MAX_PATH_LENGTH];

  FILE * lxdialog_output_file;
  char lxdialog_output_string[MAX_PATH_LENGTH];

protected:
  int open_lxdialog_output_file(char * file_name);
  int read_lxdialog_output_file();

  //return status of command execution
  //YES - command executed successfully
  //NO  - command failed
  int execute_command_line(OUT int * selection_index);

  int set_lxdialog_path(char * lx_dialog_path);

public:
  dialog_base();
  ~dialog_base();

  //return pointer to the output string
  char * get_lxdialog_output_string()
  {
    return (char*)lxdialog_output_string;
  }

};

#endif
