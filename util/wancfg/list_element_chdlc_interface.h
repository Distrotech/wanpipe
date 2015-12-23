/***************************************************************************
                          list_element_chdlc_interface.h  -  description
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

#ifndef LIST_ELEMENT_CHDLC_IF_H
#define LIST_ELEMENT_CHDLC_IF_H

#include "list_element_chan_def.h"

#define DBG_LIST_ELEMENT_CHDLC_INTERFACE 1

/**
  *@author David Rokhvarg
  */

class list_element_chdlc_interface : public list_element_chan_def  {

  wan_sppp_if_conf_t* chdlc; //the same for ppp and chdlc
  
public:

  list_element_chdlc_interface()
  {
    Debug(DBG_LIST_ELEMENT_PPP_INTERFACE,
      ("list_element_chdlc_interface::list_element_chdlc_interface()\n"));

    //set default configuration
    data.chanconf->config_id = WANCONFIG_MPCHDLC;

    //the configuration is in 'wanif_conf_t'
//    data.chanconf->keepalive_tx_tmr = 10000;
//    data.chanconf->keepalive_rx_tmr = 11000;
//    data.chanconf->keepalive_err_margin = 5;	//valid only for CHDLC in firmware
//    data.chanconf->slarp_timer = 0;
//    data.chanconf->ignore_dcd = WANOPT_NO;    //valid only for CHDLC in firmware
//    data.chanconf->ignore_cts = WANOPT_NO;    //valid only for CHDLC in firmware
    data.chanconf->ignore_keepalive = WANOPT_NO;//if yes, user will be able to set sppp_keepalive_timer
						//to some value
		
    //the above initialization should be in 'wanif_conf_t.u.chdlc',
    //but there is not such thing, so i just follow the
    //legacy structure.
    chdlc = &data.chanconf->u.ppp;
    //chdlc = &data.chanconf->u.chdlc;
    //nothing to do for now
    //unsigned int  pp_auth_timer;
    chdlc->sppp_keepalive_timer = DEFAULT_Tx_KPALV_TIMER;//the same value used for both TX and RX
    //unsigned int  pp_timer;
    chdlc->keepalive_err_margin = DEFAULT_KPALV_ERR_TOL;
  }

  ~list_element_chdlc_interface()
  {
    Debug(DBG_LIST_ELEMENT_PPP_INTERFACE,
      ("list_element_chdlc_interface::~list_element_chdlc_interface()\n"));
  }

};

#endif
