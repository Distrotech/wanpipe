/***************************************************************************
                          text_box_help.h  -  description
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

#ifndef TEXT_BOX_HELP_H
#define TEXT_BOX_HELP_H

#include <text_box.h>

#define DBG_TEXT_BOX_HELP  0

/**This class used for displaying Help, Warning and Iformation messages.
  *@author David Rokhvarg
  */

class text_box_help : public text_box  {
  char lxdialog_path[MAX_PATH_LENGTH];
  char path_to_text_file[MAX_PATH_LENGTH];
  int help_text_type;

  char * get_path_to_help_text_file(IN int help_text_type)
  {
    char * result;

    return result;
  }

public: 
	text_box_help(IN char * lxdialog_path, IN int help_text_type)
  {
    Debug(DBG_TEXT_BOX_HELP, ("text_box_help::text_box_help()\n"));

    snprintf(this->lxdialog_path, MAX_PATH_LENGTH, lxdialog_path);
    this->help_text_type = help_text_type;
  }

	~text_box_help()
  {
    Debug(DBG_TEXT_BOX_HELP, ("text_box_help::~text_box_help()\n"));
  }


  int run()
  {
    Debug(DBG_TEXT_BOX_HELP, ("text_box_help::run()\n"));

    snprintf(path_to_text_file, MAX_PATH_LENGTH, "%s%s",
             wanpipe_cfg_dir, get_path_to_help_text_file(help_text_type));

    Debug(DBG_TEXT_BOX_HELP, ("text_box_help: path_to_text_file: %s\n",
                                                    path_to_text_file));

    if(set_configuration(lxdialog_path,
                         WANCFG_PROGRAM_NAME,
                         path_to_text_file,
                         TEXT_BOX_HEIGTH,
                         TEXT_BOX_WIDTH) == NO){
      return NO;
    }

    return show();
  }

};

#endif
