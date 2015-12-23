/***************************************************************************
                          list_element_frame_relay_dlci.h  -  description
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

#ifndef LIST_ELEMENT_FRAME_RELAY_DLCI_H
#define LIST_ELEMENT_FRAME_RELAY_DLCI_H

#include "list_element_chan_def.h"

#define DBG_LIST_ELEMENT_FRAME_RELAY_DLCI 1

/**
  *@author David Rokhvarg
  */

class list_element_frame_relay_dlci : public list_element_chan_def  {

  wan_fr_conf_t* fr;

public:

  list_element_frame_relay_dlci()
  {
    Debug(DBG_LIST_ELEMENT_FRAME_RELAY_DLCI,
      ("list_element_frame_relay_dlci::list_element_frame_relay_dlci()\n"));

    //set default configuration
    data.chanconf->cir = 0;
    data.chanconf->bc = 0;
    data.chanconf->be = 0;
    data.chanconf->config_id = WANCONFIG_MFR;

    fr = &data.chanconf->u.fr;
    //set the defalut frame relay configuration:
    fr->issue_fs_on_startup = WANOPT_YES;
    fr->station = WANOPT_CPE;
    fr->dlci_num = 1;
    fr->signalling = WANOPT_FR_AUTO_SIG;//WANOPT_FR_ANSI;
    fr->t391 = 10;
    fr->t392 = 16;
    fr->n391 = 6;
    fr->n392 = 6;
    fr->n393 = 4;
  }

  ~list_element_frame_relay_dlci()
  {
    Debug(DBG_LIST_ELEMENT_FRAME_RELAY_DLCI,
      ("list_element_frame_relay_dlci::~list_element_frame_relay_dlci()\n"));
  }

};

#endif
