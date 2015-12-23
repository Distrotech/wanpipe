/***************************************************************************
                          menu_frame_relay_manual_or_auto_dlci_cfg.h  -  description
                             -------------------
    begin                : Wed Mar 17 2004
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

#ifndef MENU_FRAME_RELAY_MANUAL_OR_AUTO_DLCI_CFG_H
#define MENU_FRAME_RELAY_MANUAL_OR_AUTO_DLCI_CFG_H

#include "menu_base.h"

/**
  *@author David Rokhvarg
  */

class menu_frame_relay_manual_or_auto_dlci_cfg : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];

  wanif_conf_t* wanif_conf;
  wan_fr_conf_t* fr;	/* frame relay configuration */

public: 

  menu_frame_relay_manual_or_auto_dlci_cfg( IN char * lxdialog_path,
                                            IN wanif_conf_t* wanif_conf);

  ~menu_frame_relay_manual_or_auto_dlci_cfg();

  int run(OUT int * selection_index,
	  IN  int* old_dlci_numbering_mode,
	  OUT int* new_dlci_numbering_mode);
};

#endif
