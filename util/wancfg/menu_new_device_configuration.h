/***************************************************************************
                          menu_new_device_configuration.h  -  description
                             -------------------
    begin                : Tue Mar 9 2004
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

#ifndef MENU_NEW_DEVICE_CONFIGURATION_H
#define MENU_NEW_DEVICE_CONFIGURATION_H

#include "menu_base.h"
#include "conf_file_reader.h"
#include "wanrouter_rc_file_reader.h"

//#include "conf_file_writer.h"

/**Presents selections: 1. Protocol/Operation Mode 2. Hardware Setup
                        3. Network Interface Setup 4. Device Start/Stop scripts
   Depending on Protocol/Mode some of the selections can be hidden.

  *@author David Rokhvarg
  */

class menu_new_device_configuration : public menu_base  {
  char lxdialog_path[MAX_PATH_LENGTH];
  conf_file_reader* cfr;

  //if different protocol is selected by user from one passed on
  //creation of this object, old configuration will be deleted,
  //and new one creted.
  //The original "conf_file_reader" object will be deleted,
  //new one created using template "protocol_template.conf" file
  //in "etc/wanpipe/wancfg" directory.
  //On return the pointer will be set to the new configuration.
  conf_file_reader** ptr_cfr;

  //Temporary storage for info that should be preserved, *before*
  //the original "conf_file_reader" object will is deleted.
  //For example: 1. Protocol change does not mean the card location
  //and type should change.
  //wandev_conf_t linkconf;

  char* ip_address_configuration_complete(objects_list * obj_list);
  
  int create_logical_channels_list_str(
		string& menu_str,
		unsigned int max_valid_number_of_logical_channels,
		objects_list* obj_list);

  int check_aft_timeslot_groups_cfg();

  char err_buf[MAX_PATH_LENGTH];
  char* check_configuration_of_interfaces(objects_list * obj_list);

  wanrouter_rc_file_reader*  wanrouter_rc_fr;
public: 
  menu_new_device_configuration(IN char * lxdialog_path, IN conf_file_reader** cfr);
  ~menu_new_device_configuration();
  int run(OUT int * selection_index);

};

#endif
