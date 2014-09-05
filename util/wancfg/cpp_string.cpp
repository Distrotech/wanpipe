/***************************************************************************
                          cpp_string.cpp  -  description
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "cpp_string.h"
#include "wancfg.h"

#define DBG_CPP_STR 0

cpp_string::cpp_string()
{
  Debug(DBG_CPP_STR, ("cpp_string::cpp_string()\n"));

  name[0] = '\0';

  memset(cstr, 0x00, MAX_STR);
}

cpp_string::cpp_string(char* param)
{
  Debug(DBG_CPP_STR, ("cpp_string::cpp_string(char* param)\n"));

  snprintf(cstr, MAX_STR, param);
}


cpp_string::~cpp_string()
{
  Debug(DBG_CPP_STR, ("cpp_string::~cpp_string()\n"));
  Debug(DBG_CPP_STR, ("name: %s\n", name));
}

void cpp_string::operator= (char* param)
{
  Debug(DBG_CPP_STR, ("operator= (char* param)\n"));
  Debug(DBG_CPP_STR, ("strlen(param): %d\n", strlen(param)));

  char tmp[MAX_STR];

  snprintf(tmp, MAX_STR, param);
  strlcpy(cstr, tmp, MAX_STR);

  Debug(DBG_CPP_STR, ("end of 'operator=' cstr: %s\n", cstr));
}

void cpp_string::operator= (const char* param)
{
  Debug(DBG_CPP_STR, ("operator=(const char* param)\n"));
  Debug(DBG_CPP_STR, ("strlen(param): %d\n", strlen(param)));
  Debug(DBG_CPP_STR, ("strlen(cstr): %d\n", strlen(cstr)));

//  *this = (char*)param;
  snprintf(cstr, MAX_STR, (char*)param);

  Debug(DBG_CPP_STR, ("end of 'operator=' cstr: %s\n", cstr));
}

char* cpp_string::operator+ (char* param)
{
//ok
  Debug(DBG_CPP_STR, ("!!operator+(char* param)\n"));
  Debug(DBG_CPP_STR, ("strlen(param): %d\n", strlen(param)));
  Debug(DBG_CPP_STR, ("strlen(cstr): %d\n", strlen(cstr)));

  snprintf(&cstr[strlen(cstr)], MAX_STR - strlen(cstr), param);

  Debug(DBG_CPP_STR, ("end of 'operator+' cstr: %s\n", cstr));

  return cstr;
}

char* cpp_string::operator+= (char* param)
{
//ok
  Debug(DBG_CPP_STR, ("operator+=(char* param)\n"));
  Debug(DBG_CPP_STR, ("strlen(param): %d\n", strlen(param)));
  Debug(DBG_CPP_STR, ("strlen(cstr): %d\n", strlen(cstr)));

  *this = *this + param;

  Debug(DBG_CPP_STR, ("end of 'operator+' cstr: %s\n", cstr));

  return cstr;
}

char* cpp_string::operator+= (const char* param)
{
//ok
  Debug(DBG_CPP_STR, ("operator+=(const char* param)\n"));
  Debug(DBG_CPP_STR, ("strlen(param): %d\n", strlen(param)));
  Debug(DBG_CPP_STR, ("strlen(cstr): %d\n", strlen(cstr)));

  *this = *this + (char*)param;

  Debug(DBG_CPP_STR, ("end of 'operator+' cstr: %s\n", cstr));

  return cstr;
}

char* cpp_string::operator+= (cpp_string& param)
{
  Debug(DBG_CPP_STR, ("operator+= (cpp_string& param)\n"));

  char tmp[MAX_STR];

  snprintf(tmp, MAX_STR, param.c_str());

  *this = *this + tmp;

  Debug(DBG_CPP_STR, ("end of 'operator+= (cpp_string& param)' cstr: %s\n", cstr));

  return cstr;
}

char* cpp_string::c_str()
{
  return cstr;
}

void cpp_string::sprintf(char* format, ...)
{
	va_list ap;

	va_start(ap, format);
  vsnprintf(cstr, MAX_STR, format, ap);
	va_end(ap);

  Debug(DBG_CPP_STR, ("cpp_string::sprintf(): cstr: %s\n", cstr));
}
