/***************************************************************************
                          zaptel_conf_file_reader.h  -  description
                             -------------------
    begin                : Wed Nov 23 2005
    author            	 : David Rokhvarg <davidr@sangoma.com>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ZAPTEL_CONF_FILE_READER_H
#define ZAPTEL_CONF_FILE_READER_H

#include "wancfg.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/un.h>
#include <unistd.h>
#include <time.h>

#include "menu_hardware_probe.h"
#include "sangoma_card_list.h"

/****** Defines *************************************************************/

enum {
  NO_SANGOMA_CARDS_DETECTED=1,//must be non-zero!!!
  NO_SANGOMA_TE1_CARDS_DETECTED,
  NO_SANGOMA_ANALOG_CARDS_DETECTED,
  INVALID_CARD_VERSION,
  FAILED_TO_WRITE_CONF_FILE,
  NO_TE1_SPAN_FOUND_IN_ZAPTEL_CONF
};

class zaptel_conf_file_reader 
{
  char *zaptel_conf_path;
  FILE *cf;
  int lineno;

  void trim(char *buf);
  int error(char *fmt, ...);
  char* readline();
  int parseargs(char *input, char *output[], int maxargs, char sep);
  int check_span_has_a_dchan(int span, unsigned char media);

public:
  zaptel_conf_file_reader(char* file_path);
  ~zaptel_conf_file_reader();

  int read_file();
  int spanconfig(char *keyword, char *args);
  int discard(char *keyword, char *args);
  int chanconfig(char *keyword, char *args);
  int parse_channel(char *channel, int *startchan);
  int parse_idle(int *i, char *s);
  int apply_channels(int chans[], char *argstr);
  const char* sigtype_to_str(const int sig);
  int setlaw(char *keyword, char *args);
  void printconfig();
  int create_wanpipe_config();

};

#endif

