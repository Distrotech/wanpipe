/***************************************************************************
                          conf_file_reader.h  -  description
                             -------------------
    begin                : Thu Mar 4 2004
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

#ifndef CONF_FILE_READER_H
#define CONF_FILE_READER_H

#include "wanctl.h"


/****** Defines *************************************************************/

/* Configuration stuff */
#define	MAX_CFGLINE	256
#define	MAX_CMDLINE	256
#define	MAX_TOKENS	32

/*
 * Data types for configuration structure description tables.
 */
#define	DTYPE_INT	1
#define	DTYPE_UINT	2
#define	DTYPE_LONG	3
#define	DTYPE_ULONG	4
#define	DTYPE_SHORT	5
#define	DTYPE_USHORT	6
#define	DTYPE_CHAR	7
#define	DTYPE_UCHAR	8
#define	DTYPE_PTR	9
#define	DTYPE_STR	10
#define	DTYPE_FILENAME	11

#define NO_ANNEXG	0x00
#define ANNEXG_LAPB	0x01
#define ANNEXG_X25	0x02
#define ANNEXG_DSP	0x03


#define show_error(x) { printf("Error Line %i\n",__LINE__); show_error_dbg(x);}


/**Reads wanpipe#.conf file. Allocates configuration structures and lists.
  *@author David Rokhvarg
  */

class conf_file_reader {

  int verbose;			/* verbosity level */
  char conf_file[MAX_PATH_LENGTH];

  int get_protocol_from_string(char* conf_file_line);
//  int get_station_type_from_string(char* conf_file_line);
  int get_serial_clocking_type_from_string(char* line);
  int get_te1_clocking_type_from_string(char* line);

public:
  link_def_t * link_defs;
  int wanpipe_number;

  unsigned char protocol;
  unsigned char station_type;
  char if_name[WAN_IFNAME_SZ+1];
  char ip_cfg_complete;

  char card_type;

  char current_wanrouter_state;

	conf_file_reader();
	~conf_file_reader();

  int read_conf_file();//reads the wanpipe#.conf file, full path should be at 'conf_file'
  int form_interface_name(unsigned char protocol);
};

#endif
