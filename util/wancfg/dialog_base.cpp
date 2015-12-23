/***************************************************************************
                          dialog_base.cpp  -  description
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

#include "dialog_base.h"

#define DBG_DIALOG_BASE 1

dialog_base::dialog_base()
{
  Debug(DBG_DIALOG_BASE, ("dialog_base::dialog_base()\n"));
  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "NOT INITIALIZED");
}

dialog_base::~dialog_base()
{
  Debug(DBG_DIALOG_BASE, ("dialog_base::~dialog_base()\n"));
  //do nothing
}

int dialog_base::set_lxdialog_path(char * lxdialog_path)
{
  Debug(DBG_DIALOG_BASE, ("dialog_base::set_lx_dialog_path()\n"));
  Debug(DBG_DIALOG_BASE, ("lxdialog_path and command line: %s\n", lxdialog_path));

  if(strlen(lxdialog_path) > MAX_PATH_LENGTH){
    printf("dialog_base: lxdialog_path is longer than maximum\n");
    return NO;
  }

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  return YES;
}

//return status of command execution
//YES - command executed successfully
//NO  - command failed
int dialog_base::execute_command_line(OUT int * selection_index)
{
  int sytem_call_status;
  int lxdialog_exit_status=0;
  int rc;

  sytem_call_status = system(lxdialog_path);

  if(WIFEXITED(sytem_call_status)){

    lxdialog_exit_status = WEXITSTATUS(sytem_call_status);

    //Debug(DBG_DIALOG_BASE, ("lxdialog_exit_status: %d\n", lxdialog_exit_status));

    switch(lxdialog_exit_status)
    {
    case SELECTION_0:
    case SELECTION_1:
    case SELECTION_2:
    //case 255://if lxdialog returns -1
      rc = YES;
      break;

    default:
      Debug(DBG_DIALOG_BASE, ("Invalid lxdialog_exit_status: %d\n", lxdialog_exit_status));
      Debug(DBG_DIALOG_BASE, ("lx_dialog_path and command line: %s\n", lxdialog_path));
      rc = NO;
    }
  }else{
    //lxdialog crashed or killed
    printf("lxdialog exited abnormally (sytem_call_status: %d).\n",
            sytem_call_status);
    rc = NO;
  }

  *selection_index = lxdialog_exit_status;
  return rc;
}

int dialog_base::open_lxdialog_output_file(char * file_name)
{
  lxdialog_output_file = fopen(file_name, "r+");

  if(lxdialog_output_file == NULL){
    Debug(DBG_DIALOG_BASE, ("Failed to open file '%s'. File does not exist.\n", file_name));
    return NO;
  }
  return YES;
}

//this version reads file line by line
int dialog_base::read_lxdialog_output_file()
{
  int rc = NO;
  int lines_counter;

  lines_counter = 0;
  do{
    fgets(lxdialog_output_string, MAX_PATH_LENGTH, lxdialog_output_file);
    if(!feof(lxdialog_output_file)){
      Debug(DBG_DIALOG_BASE, (lxdialog_output_string));
    }

    //there must be at least 1 line. but not more than 1 line.
    rc = YES;

  }while(!feof(lxdialog_output_file) && lines_counter++ < 2);

  if(lines_counter >= 2){
    printf("Invalid number of lines in lxdialog output file!!\n");
    rc = NO;
  }

  fclose(lxdialog_output_file);

  if(rc == NO){
    Debug(DBG_DIALOG_BASE, ("failed to read_lxdialog_output_file!\n"));
  }

  Debug(DBG_DIALOG_BASE, ("\n"));
  return rc;
}
