/***************************************************************************
                          list_element_tty_interface.h  -  description
                             -------------------
    begin                : Thu Mar 18 2004
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

#ifndef LIST_ELEMENT_TTY_IF_H
#define LIST_ELEMENT_TTY_IF_H

#include "list_element_chan_def.h"

#define DBG_LIST_ELEMENT_CHDLC_INTERFACE 1

/**
  *@author David Rokhvarg
  */

class list_element_tty_interface : public list_element_chan_def  {

  wan_sppp_if_conf_t* chdlc; //the same for ppp and chdlc
  
public:

  list_element_tty_interface()
  {
    Debug(DBG_LIST_ELEMENT_PPP_INTERFACE,
      ("list_element_tty_interface::list_element_tty_interface()\n"));

    //set default configuration
    data.chanconf->config_id = WANCONFIG_TTY;
    data.usedby = TTY;

    snprintf(data.chanconf->addr, WAN_ADDRESS_SZ, "0");
  }

  ~list_element_tty_interface()
  {
    Debug(DBG_LIST_ELEMENT_PPP_INTERFACE,
      ("list_element_tty_interface::~list_element_tty_interface()\n"));
  }

};

#endif
