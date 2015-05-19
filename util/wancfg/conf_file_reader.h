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

#include "wancfg.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__)
# include <wanpipe_version.h>
# include <wanproc.h>
# include <wanpipe.h>
#else
# include <linux/wanpipe_version.h>
# include <linux/wanpipe.h>
#endif


/****** Defines *************************************************************/

#ifndef	min
#define	min(a,b)	(((a)<(b))?(a):(b))
#endif

#define	is_digit(ch) (((ch)>=(unsigned)'0'&&(ch)<=(unsigned)'9')?1:0)

enum ErrCodes			/* Error codes */
{
	ERR_SYSTEM = 1,		/* system error */
	ERR_SYNTAX,		/* command line syntax error */
	ERR_CONFIG,		/* configuration file syntax error */
	ERR_MODULE,		/* module not loaded */
	ERR_LIMIT
};

/* Configuration stuff */
#define	MAX_CFGLINE	256
#define	MAX_CMDLINE	256
#define	MAX_TOKENS	32

/****** Data Types **********************************************************/

typedef struct look_up		/* Look-up table entry */
{
	uint	val;		/* look-up value */
	void*	ptr;		/* pointer */
} look_up_t;

typedef struct key_word		/* Keyword table entry */
{
	char*	keyword;	/* -> keyword */
	uint	offset;		/* offset of the related parameter */
	uint	size;		/* offset of the related parameter */
	int	dtype;		/* data type */
} key_word_t;

typedef struct data_buf		/* General data buffer */
{
	unsigned size;
	void* data;
} data_buf_t;

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

#include "wancfg.h"

#include "objects_list.h"

/** Reads wanpipe#.conf file.
 *  Allocates configuration structures and linked lists.
 */
class conf_file_reader {

  int	verbose;			/* verbosity level */
  char	conf_file_full_path[MAX_PATH_LENGTH];
  FILE* conf_file_ptr;
  
  int parse_conf_file(char* fname);
  
  int read_conf_record (FILE* file, char* key, int max_len);
  char* read_conf_section (FILE* file, char* section);
 
  int set_conf_param (char* key, char* val, key_word_t* dtab, void* conf, int size, link_def_t* lnk_def,
		      chan_def_t* chan_def);
      	
  int read_devices_section(FILE* file);
  int read_wanpipe_section(FILE* file);
  int read_interfaces_section(
		FILE* file,
	       	objects_list* parent_objects_list,
		char* parent_dev_name,
	       	int* current_line_number
		);

  int configure_link (link_def_t* def, char init);

  int configure_chan (chan_def_t* chandef, int id);
                
  void free_linkdefs(void);

public:
  //this member initialized as follows:
  //1. If "New" wanpipe - user selects wanpipe number.
  //2. If "Edit existing" - wanpipe number is given (read from the conf file).
  int wanpipe_number;

  //per card (CPU or Line) configuration
  link_def_t* link_defs;    //link
  //list of logical channels
  objects_list* main_obj_list;   //channels for the link

  void print_obj_list_contents(objects_list* the_list);

  //wanpipe number passed as parameter
  conf_file_reader(int file_number);
  
  ~conf_file_reader();
  //call this just after creating the object.
  //if non zero return code, the object can *not* be used!
  int init();
  
  //read the wanpipe#.conf file, full path should be at 'conf_file_full_path'
  //before calling this function
  int read_conf_file();

  ///////////////////
  int get_pci_adapter_type_from_wanpipe_conf_file(char * conf_file_name, unsigned int* card_version);
  int get_card_type_from_str(char * line);
  int get_media_type_from_str(char * line, unsigned int* card_version);

};

#endif
