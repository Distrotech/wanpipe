/***************************************************************************
                          net_interface_file_reader.h  -  description
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

#ifndef NET_INTERFACE_FILE_READER_H
#define NET_INTERFACE_FILE_READER_H

#include "wanctl.h"

/**
  *@author David Rokhvarg
  */

//minimal ip addr string : ?.?.?.? - 7 chars
#define MIN_IP_ADDR_STR_LEN 7

class net_interface_file_reader {

  string full_file_path;
  char* interface_name;


public:
  if_config_t if_config;

	net_interface_file_reader(char* interface_name);
	~net_interface_file_reader();

  int parse_net_interface_file();//to read file
  int create_net_interface_file(if_config_t* if_config);//to save updated file
};

#endif
