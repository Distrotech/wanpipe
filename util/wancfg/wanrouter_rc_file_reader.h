/***************************************************************************
                          wanrouter_rc_file_reader.h  -  description
                             -------------------
    begin                : Wed Mar 24 2004
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

#ifndef WANROUTER_RC_FILE_READER_H
#define WANROUTER_RC_FILE_READER_H

#include "wancfg.h"

/**
  *@author David Rokhvarg
  */

class wanrouter_rc_file_reader {

  string full_file_path;
  string wanpipe_name;

  string updated_file_content;
  string updated_wandevices_str;

public:
  if_config_t if_config;

  wanrouter_rc_file_reader(int wanpipe_number);
  ~wanrouter_rc_file_reader();

  int search_and_update_boot_start_device_setting();
  int update_wanrouter_rc_file();//to save updated file
  int get_setting_value(IN char* setting_name,
			OUT char* setting_value_buff,
			int setting_value_buff_len);

};

#endif
