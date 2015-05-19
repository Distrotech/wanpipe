/***************************************************************************
                          menu_advanced_pci_configuration.h  -  description
                             -------------------
    begin                : Wed Jun 2 2004
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

#ifndef MENU_ADVANCED_PCI_CONFIGURATION_H
#define MENU_ADVANCED_PCI_CONFIGURATION_H

#include "menu_base.h"
#include "conf_file_reader.h"

/**
  *@author David Rokhvarg
  */

class menu_advanced_pci_configuration : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;

  void form_pci_card_locations_options_menu(string& str, int& number_of_items);

public:
  menu_advanced_pci_configuration(  IN char * lxdialog_path,
                                    IN conf_file_reader* cfr);

 ~menu_advanced_pci_configuration();

  int run(OUT int * selection_index);
};

#endif
