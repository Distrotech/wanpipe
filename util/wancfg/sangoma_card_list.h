/***************************************************************************
                          menu_aft_logical_channel_cfg.h  -  description
                             -------------------
    begin                : Thu Apr 8 2004
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

#ifndef SANGOMA_CARD_LIST_H
#define SANGOMA_CARD_LIST_H

#include "objects_list.h"
#include "list_element_sangoma_card.h"

/**
  *@author David Rokhvarg
  */

#define CARD_TYPE_ANY	-1

class sangoma_card_list : public objects_list {

  //this list may contain ANY or SPECIFIC card types
  //
  int card_type;

public: 
  sangoma_card_list();
  sangoma_card_list(int card_type);
  ~sangoma_card_list();

  int get_list_card_type()
  {
    return card_type;
  }
};

#endif
