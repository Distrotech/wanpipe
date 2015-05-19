/***************************************************************************
                          sangoma_card_list.cpp  -  description
                             -------------------
    begin                : Fri November 25 2005
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

#include "sangoma_card_list.h"


#define DBG_SANGOMA_CARD_LIST	1

sangoma_card_list::sangoma_card_list()
{
  Debug(DBG_SANGOMA_CARD_LIST, ("sangoma_card_list::sangoma_card_list()\n"));
  card_type = CARD_TYPE_ANY;
}

sangoma_card_list::sangoma_card_list(int card_type)
{
  Debug(DBG_SANGOMA_CARD_LIST, ("sangoma_card_list::sangoma_card_list()\n"));
  this->card_type = card_type;
}

sangoma_card_list::~sangoma_card_list()
{
  Debug(DBG_SANGOMA_CARD_LIST, ("sangoma_card_list::~sangoma_card_list()\n"));
}

