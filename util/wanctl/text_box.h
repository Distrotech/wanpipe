/***************************************************************************
                          text_box.h  -  description
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

#ifndef TEXT_BOX_H
#define TEXT_BOX_H

#include <dialog_base.h>

#define TEXT_BOX_HEIGTH 20
#define TEXT_BOX_WIDTH  60

/**Class used to display "Help" and other text messages.
  *@author David Rokhvarg
  */

class text_box : public dialog_base  {
public: 
	text_box();
	~text_box();

	int set_configuration(IN char * lxdialog_path,
                        IN char * backtitle,
                        IN char * path_to_text_file,
                        IN int hight,
                        IN int width);

  //shorter version for convinience
  int set_configuration(IN char * lxdialog_path,
                        IN int protocol,
                        IN char * path_to_text_file);

  int show();

  void show_error_message(IN char * lxdialog_path,
                          IN int protocol,
                          IN char* format, ...);

  void show_help_message(IN char * lxdialog_path,
                         IN int protocol,
                         IN char* format, ...);

};

#endif
