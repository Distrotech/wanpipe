/***************************************************************************
                          cpp_string.h  -  description
                             -------------------
    begin                : Thu Apr 29 2004
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

#ifndef CPP_STRING_H
#define CPP_STRING_H

/**
  *@author David Rokhvarg
  */

#define MAX_STR 512000//4096

class cpp_string {

  char name[50];
  char cstr[MAX_STR];

public:
  cpp_string();
  cpp_string(char* param);
  ~cpp_string();
  void operator = (char*);
  void operator = (const char*);
  char* operator + (char*);
  char* operator += (char*);
  char* operator += (const char*);
  char* operator += (cpp_string& param);
  char* c_str();
  void sprintf(char* format, ...);
};

#endif
