/***************************************************************************
                          list_element_lapb_interface.h  -  description
                             -------------------
    begin                : Mon May 2 2005
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

#ifndef LIST_ELEMENT_LAPB_IF_H
#define LIST_ELEMENT_LAPB_IF_H

#include "list_element_chan_def.h"

#define DBG_LIST_ELEMENT_LAPB_INTERFACE 1

/**
  *@author David Rokhvarg
  */

class list_element_lapb_interface : public list_element_chan_def  {

  wan_lapb_if_conf_t 	*lapb;
  
public:

  list_element_lapb_interface()
  {
    Debug(DBG_LIST_ELEMENT_LAPB_INTERFACE,
      ("list_element_lapb_interface::list_element_lapb_interface()\n"));

    data.chanconf->config_id = WANCONFIG_LAPB;
    data.usedby = API;
    
    //set default configuration
    lapb = &data.chanconf->u.lapb;
    /*
    STATION	= DTE
    T1	= 2
    T2	= 0
    T3	= 6
    T4	= 3
    N2	= 5
    MODE = 8
    WINDOW  = 7
    MAX_PKT_SIZE = 1024
    */
    
    lapb->station = WANOPT_DTE;
    lapb->t1 = 3;
    //unsigned int t1timer;
    lapb->t2 = 0;
    //unsigned int t2timer;
    lapb->n2 = 10;
    //unsigned int n2count;
    lapb->t3 = 6;
    lapb->t4 = 0;
    //unsigned int state;
    lapb->mode = 8;
    lapb->window = 7;
    lapb->mtu = 1024;
    //unsigned char label[WAN_IF_LABEL_SZ+1];
    //unsigned char virtual_addr[WAN_ADDRESS_SZ+1];
    //unsigned char real_addr[WAN_ADDRESS_SZ+1];
    
    snprintf(data.chanconf->addr, WAN_ADDRESS_SZ, "0");
  }

  ~list_element_lapb_interface()
  {
    Debug(DBG_LIST_ELEMENT_LAPB_INTERFACE,
      ("list_element_lapb_interface::~list_element_lapb_interface()\n"));
  }

};

#endif
