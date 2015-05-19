/***************************************************************************
                          message_box.h  -  description
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

#ifndef MESSAGE_BOX_H
#define MESSAGE_BOX_H

#include "text_box.h"

/**Class used to display "Help" and other text messages.
  *Will have button "OK" as opposed to "Exit".
  *@author David Rokhvarg
  */

class message_box : public text_box  {
public: 
  message_box();
  ~message_box();
};

#endif
