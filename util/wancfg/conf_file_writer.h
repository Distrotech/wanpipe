/***************************************************************************
                          conf_file_writer.h  -  description
                             -------------------
    begin                : Fri Apr 2 2004
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

#ifndef CONF_FILE_WRITER_H
#define CONF_FILE_WRITER_H

#include "conf_file_reader.h"

#if defined(ZAPTEL_PARSER)
#include "list_element_sangoma_card.h"

#if defined(__LINUX__)
# include <zapcompat_user.h>
#else
#endif

#endif

/**
  *@author David Rokhvarg
  */

class conf_file_writer {

  conf_file_reader* cfr;

#if defined(ZAPTEL_PARSER)
  list_element_sangoma_card *list_element_sangoma_card_ptr;
#endif

  string full_file_path;

  int form_top_comment_str(string& top_comment_str);
  int form_devices_section_str(string& devices_section_str);
  int form_interface_str(string& interfaces_section_str,
                         list_element_chan_def* list_el_chan_def,
			 IN list_element_chan_def* parent_list_el_chan_def);
  int form_wanpipe_section_str(string& wanpipe_section_str);
  int form_wanpipe_card_location_str(string& wp_card_location_string);
  int form_fe_card_cfg_str(string& te1_cfg_string, sdla_fe_cfg_t*  fe_cfg);
  int form_wanpipe_card_miscellaneous_options_str(string& misc_opt_string);

  ///////////////////////////////////////////////////////////////////////////////////////////
  int form_frame_relay_global_configuration_string( wan_fr_conf_t* fr_cfg,
                                                    string& fr_global_cfg);

  int form_ppp_global_configuration_string(wan_sppp_if_conf_t* ppp_cfg,
                                           string& global_protocol_cfg);

  int form_chdlc_global_configuration_string(wan_sppp_if_conf_t* chdlc_cfg,
                                           string& global_protocol_cfg);

//  int form_chdlc_global_configuration_string( wan_chdlc_conf_t* chdlc_cfg,
//                                              string& global_protocol_cfg);

  int form_aft_global_configuration_string( wan_xilinx_conf_t* xilinx_conf,
                                            string& global_protocol_cfg);

  int form_adsl_global_cfg_str(string& tmp_string);

  int form_lapb_global_configuration_string(wan_lapb_if_conf_t *lapb_cfg,
                                           string& global_protocol_cfg);
  
  int form_atm_per_interface_str( string& wp_interface,
                                  list_element_chan_def* list_el_chan_def);
  ///////////////////////////////////////////////////////////////////////////////////////////
  int traverse_interfaces(string& interfaces_section_str, objects_list * obj_list, IN int task_type,
	IN list_element_chan_def* parent_list_el_chan_def);

  int form_per_interface_str( string& wp_interface, list_element_chan_def* list_el_chan_def,
                              int lip_layer_flag);

  int form_common_per_interface_str( string& wp_interface,
                                     list_element_chan_def* list_el_chan_def);
  ///////////////////////////////////////////////////////////////////////////////////////////
  int form_frame_relay_per_interface_str( string& wp_interface,
                                          list_element_chan_def* list_el_chan_def);
  int form_ppp_per_interface_str(string& wp_interface,
		  		 wanif_conf_t* chanconf);
                                 
  int form_chdlc_per_interface_str(string& wp_interface,
                                   list_element_chan_def* list_el_chan_def,
		  			wanif_conf_t* chanconf);

  int form_hardware_interface_str( string& wp_interface,
                                  list_element_chan_def* list_el_chan_def);

  char* get_aft_lip_layer_protocol(int protocol);

  int form_profile_str(string& profile_str, list_element_chan_def* list_el_chan_def);

  ///////////////////////////////////////////////////////////////////////////////////////////

public: 
  conf_file_writer(IN conf_file_reader* cfr);
#if defined(ZAPTEL_PARSER)
  conf_file_writer(IN list_element_sangoma_card *list_element_sangoma_card_ptr);
  int write_wanpipe_zap_file(int wanpipe_number);
  int add_to_wanrouter_start_sequence(int wanpipe_number);
#endif
  ~conf_file_writer();
  int write();

  char* get_keyword_from_key_word_t_table(IN key_word_t* table,
                                          IN int offset_in_structure);

  char* get_keyword_from_look_up_t_table(IN look_up_t* table,
                                         IN int search_value);

  char* form_keyword_and_value_str( key_word_t* conftab,
                                    int offset_in_structure,
                                    look_up_t* table,
                                    int search_value);

};

#endif
