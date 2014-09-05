/***************************************************************************
                          list_element.h  -  description
                             -------------------
    begin                : Mon Mar 8 2004
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

#ifndef LIST_ELEMENT_H
#define LIST_ELEMENT_H

enum LIST_ELEMENT_TYPE{
  BASE_LIST_ELEMENT = 1,
  CHAN_DEF_LIST_ELEMENT
};

#define DBG_LIST_ELEMENT  0

#include "wanctl.h"

/**Class used as an element of a linked list.
  *@author David Rokhvarg
  */

//it is an abstract class - user *must* write a sublclass to use it.
class list_element {

protected:
  char list_element_type;
public:
  void * next;
  void * previous;

	list_element()
  {
    Debug(DBG_LIST_ELEMENT, ("list_element::list_element()\n"));
    next = previous = NULL;
    //must be set to real value in the subclasses
    list_element_type = BASE_LIST_ELEMENT;
  }
/*
  //no real work is done here. comment it out to reduce number of warnings
  //" list_element.h:204: warning: `class list_element_chan_def' has
  //  virtual functions but non-virtual destructor".

	~list_element()
  {
    Debug(DBG_LIST_ELEMENT, ("list_element::~list_element()\n"));
  }
*/

  void reset()
  {
    next = previous = NULL;
  }

  char get_element_type()
  {
    return list_element_type;
  }

  //set these functions to 0 to make the class abstract
	virtual int compare(list_element* element) = 0;
	virtual int compare(char* comparator) = 0;
  //virtual int compare(int comparator) = 0;
  //the comparator is char string in this case
  virtual char* get_comparator() = 0;

};

#endif
