/***************************************************************************
                          menu_hardware_setup.h  -  description
                             -------------------
    begin                : Tue Mar 30 2004
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

#ifndef MENU_HARDWARE_SETUP_H
#define MENU_HARDWARE_SETUP_H

#include "menu_base.h"
#include "conf_file_reader.h"

/**
  *@author David Rokhvarg
  */

class menu_hardware_setup : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;
  
  void form_pci_card_locations_options_menu(string& str, int& number_of_items);
  void form_s514_serial_options_menu(string& str, int& number_of_items);
  void form_s514_TE1_options_menu(string& str, int& number_of_items);
  void form_TE3_options_menu(string& str, int& number_of_items);
	void form_AFT_Serial_options_menu(string& str, int& number_of_items);

public:
	
  menu_hardware_setup(  IN char * lxdialog_path, IN conf_file_reader* cfr);

  ~menu_hardware_setup();

  int run(OUT int * selection_index);
};


class menu_hardware_serial_connection_type : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;

public: 
	menu_hardware_serial_connection_type( IN char * lxdialog_path,
                                        IN conf_file_reader* ptr_cfr);

	~menu_hardware_serial_connection_type();

  int run(OUT int * selection_index);
};

class menu_hardware_serial_line_coding : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;

public: 
	menu_hardware_serial_line_coding( IN char * lxdialog_path,
                                    IN conf_file_reader* ptr_cfr);

	~menu_hardware_serial_line_coding();

  int run(OUT int * selection_index);
};

class menu_hardware_serial_line_idle : public menu_base  {

  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;

public: 
	menu_hardware_serial_line_idle( IN char * lxdialog_path,
                                  IN conf_file_reader* ptr_cfr);

	~menu_hardware_serial_line_idle();

  int run(OUT int * selection_index);
};

#endif

