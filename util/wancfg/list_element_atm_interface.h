/***************************************************************************
                          list_element_atm_interface.h  -  description
                             -------------------
    begin                : Tue Oct 11 2005
    copyright            : (C) 2005 by David Rokhvarg
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

#ifndef LIST_ELEMENT_ATM_INTERFACE_H
#define LIST_ELEMENT_ATM_INTERFACE_H

#include "list_element_chan_def.h"

#define DBG_LIST_ELEMENT_ATM_INTERFACE 1

/**
  *@author David Rokhvarg
  */

class list_element_atm_interface : public list_element_chan_def  {
  
  wan_atm_conf_if_t* atm_if_cfg;
public:

  list_element_atm_interface()
  {
    Debug(DBG_LIST_ELEMENT_ATM_INTERFACE,
      ("list_element_atm_interface::list_element_atm_interface()\n"));

    //it's a union, so it does not really matter to which one
    //of the members this pointer is initialized to.
    atm_if_cfg = (wan_atm_conf_if_t*)&data.chanconf->u;

    atm_if_cfg->encap_mode = RFC_MODE_ROUTED_IP_LLC;
    atm_if_cfg->vpi = 0;
    atm_if_cfg->vci = 35;

    atm_if_cfg->atm_oam_loopback  = WANOPT_NO;
    atm_if_cfg->atm_oam_loopback_intr = 5;
    atm_if_cfg->atm_oam_continuity = WANOPT_NO;
    atm_if_cfg->atm_oam_continuity_intr = 2;
    atm_if_cfg->atm_arp = WANOPT_NO;
    atm_if_cfg->atm_arp_intr = 15;

    atm_if_cfg->mtu = 1500;

    //set default configuration
    data.chanconf->config_id = WANCONFIG_LIP_ATM;
  }

  ~list_element_atm_interface()
  {
    Debug(DBG_LIST_ELEMENT_ATM_INTERFACE,
      ("list_element_atm_interface::~list_element_atm_interface()\n"));
  }

};

#endif
