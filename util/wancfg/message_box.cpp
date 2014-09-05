/***************************************************************************
                          message_box.cpp  -  description
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

#include "message_box.h"
#include "wancfg.h"

#define DBG_TEXT_BOX 1

message_box::message_box()
{
  Debug(DBG_TEXT_BOX, ("message_box::message_box()\n"));
  box_type = 1;
}

message_box::~message_box()
{
}

