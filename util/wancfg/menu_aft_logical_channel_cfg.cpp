/***************************************************************************
                          menu_aft_logical_channel_cfg.cpp  -  description
                             -------------------
    begin                : Thu Apr 8 2004
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

#include "menu_aft_logical_channel_cfg.h"
#include "text_box.h"
#include "text_box_yes_no.h"
#include "input_box.h"
#include "input_box_active_channels.h"

char* bound_te1_channels_help_str =
"\n"
"BOUND T1/E1 ACTIVE CHANNELS\n"
"\n"
"The Channels Group can be bound into any single\n"
"or conbination of T1/E1 Active channels.\n"
"\n"
"Using socket interface API user can send and\n"
"receive data on specified T1/E1 channels.\n"
"\n"
"Please specify T1/E1 channles to bind to a\n"
"Channels Group.\n"
"\n"
"You can use the follow format:\n"
"o ALL     - for full T1/E1 links\n"
"o x.a-b.w - where the letters are numbers; to specify\n"
"            the channels x and w and the range of\n"
"            channels a-b, inclusive for fractional\n"
"            links.\n"
" Example:\n"
"o ALL     - All channels will be selcted\n"
"o 1       - A first channel will be selected\n"
"o 1.3-8.12 - Channels 1,3,4,5,6,7,8,12 will be selected.\n"
"\n"
"If more than one Channels Group is configured, make sure\n"
"that \"Bound Channels\" do not overlap between different\n"
"Channel Groups\n";

enum AFT_LOGICAL_CHANNEL_CONFIGURATION{

  //Bound T1/E1 Channels  --> 1-5
  BOUND_TE1_CHANNELS,
/*
  //Ignore DCD Status ------> NO
  IGNORE_DCD,
  //Ignore CTS Status ------> NO
  IGNORE_CTS
*/
  IDLE_FLAG_CHAR,
  CHAN_MTU,
  CHAN_MRU,
  CORE_MODE,
  BOUND_ANALOG_CHANNEL,
  EMPTY_LINE
};


#define DBG_MENU_AFT_LOGICAL_CHANNEL_CFG 1

menu_aft_logical_channel_cfg::
	menu_aft_logical_channel_cfg(IN char * lxdialog_path,
                               IN conf_file_reader* cfr,
                               IN list_element_chan_def* logical_ch_cfg)
{
  Debug(DBG_MENU_AFT_LOGICAL_CHANNEL_CFG,
    ("menu_aft_logical_channel_cfg::menu_aft_logical_channel_cfg()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);

  this->cfr = cfr;
  this->obj_list = cfr->main_obj_list;
  this->logical_ch_cfg = logical_ch_cfg;
}

menu_aft_logical_channel_cfg::~menu_aft_logical_channel_cfg()
{
  Debug(DBG_MENU_AFT_LOGICAL_CHANNEL_CFG,
    ("menu_aft_logical_channel_cfg::~menu_aft_logical_channel_cfg()\n"));
}

int menu_aft_logical_channel_cfg::run(IN int wanpipe_number, OUT int * selection_index,
                                      IN int number_of_timeslot_groups)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  int num_of_visible_items = 1;
  //help text box
  text_box tb;
  input_box input_bx;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];


  link_defs = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility");

  Debug(DBG_MENU_AFT_LOGICAL_CHANNEL_CFG,
    ("menu_frame_relay_DLCI_configuration::run()\n"));

  input_box_active_channels act_channels_ip;

again:
  rc = YES;
  menu_str = " ";

  if(logical_ch_cfg ==  NULL){
    ERR_DBG_OUT(("%s configuration pointer is NULL (logical_ch_cfg) !!!\n",
      TIME_SLOT_GROUP_STR));
    rc = NO;
    goto cleanup;
  }

  num_of_visible_items = 5;

  /////////////////////////////////////////////////////////////////////////////////////
  if(link_defs->card_version != A200_ADPTR_ANALOG){
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", BOUND_TE1_CHANNELS);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Timeslots in Group-> %s\" ",
      logical_ch_cfg->data.active_channels_string);
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CORE_MODE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"HDLC engine--------> %s\" ",
      logical_ch_cfg->data.chanconf->hdlc_streaming == WANOPT_YES ? "Enabled" : "Disabled");
      //logical_ch_cfg->data.active_channels_string);
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", IDLE_FLAG_CHAR);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Idle char ---------> 0x%02X\" ",
      logical_ch_cfg->data.chanconf->u.aft.idle_flag);
      //logical_ch_cfg->data.active_channels_string);
    menu_str += tmp_buff;
  }else{
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", BOUND_ANALOG_CHANNEL);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Channel in Group---> %s\" ",
      logical_ch_cfg->data.active_channels_string);
    menu_str += tmp_buff;
  }
  /////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CHAN_MTU);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"MTU ---------------> %u\" ",
    logical_ch_cfg->data.chanconf->u.aft.mtu);
  menu_str += tmp_buff;

  /////////////////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CHAN_MRU);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"MRU ---------------> %u\" ",
    logical_ch_cfg->data.chanconf->u.aft.mru);
  menu_str += tmp_buff;
  //goto label_show_bound_timeslots;

  /////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\
\nSet cofiguration of %s %s.", TIME_SLOT_GROUP_STR, logical_ch_cfg->data.addr);


  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
			  (char*)(link_defs->card_version != A200_ADPTR_ANALOG ?
                          "AFT DS0 CHANNEL CONFIGURATION" :
							(link_defs->card_version == A200_ADPTR_ANALOG ? 
								"AFT ANALOG CHANNEL CONFIGURATION": "ISDN BRI CHANNEL CONFIGURATION")),
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          num_of_visible_items,
                          (char*)menu_str.c_str()
                          ) == NO){
    rc = NO;
    goto cleanup;
  }

  if(show(selection_index) == NO){
    rc = NO;
    goto cleanup;
  }

  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_AFT_LOGICAL_CHANNEL_CFG,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case EMPTY_LINE:
      //do nothing
      goto again;

    case BOUND_ANALOG_CHANNEL:
show_analog_chan_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Enter Channel Number. From 1 to 16 or ALL.");
      snprintf(initial_text, MAX_PATH_LENGTH, "%s", logical_ch_cfg->data.active_channels_string);

      input_bx.set_configuration(lxdialog_path,
                              	backtitle,
                                explanation_text,
                                INPUT_BOX_HIGTH,
                                INPUT_BOX_WIDTH,
                                initial_text);

      input_bx.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        snprintf(tmp_buff, MAX_PATH_LENGTH, input_bx.get_lxdialog_output_string());

        str_tolower(tmp_buff);
      
	if(strcmp(tmp_buff, "all") == 0){
		snprintf(logical_ch_cfg->data.active_channels_string, 
					MAX_LEN_OF_ACTIVE_CHANNELS_STRING, "ALL");
        }else{
		int chan = atoi(remove_spaces_in_int_string(tmp_buff));
      
		if(chan >= 1 && chan <= MAX_FXOFXS_CHANNELS){
          		logical_ch_cfg->data.chanconf->active_ch = chan;
			snprintf(logical_ch_cfg->data.active_channels_string,
					MAX_LEN_OF_ACTIVE_CHANNELS_STRING, tmp_buff);
		}else{
			tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED, "Invalid input: %s!",
				input_bx.get_lxdialog_output_string());
		}
	}
        break;

      case INPUT_BOX_BUTTON_HELP:
	tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, "Channel number 1-16 or ALL.");
        goto show_analog_chan_input_box;
      }//switch(*selection_index)
      break;

    case BOUND_TE1_CHANNELS:
//label_show_bound_timeslots:
      if(act_channels_ip.show(  lxdialog_path,
                                logical_ch_cfg->data.chanconf->config_id,
                                logical_ch_cfg->data.active_channels_string,//initial text
                                selection_index,
                                cfr->link_defs->linkconf->fe_cfg.media) == NO){
        rc = NO;
      }else{

        switch(*selection_index)
        {
        case INPUT_BOX_BUTTON_OK:
          snprintf(logical_ch_cfg->data.active_channels_string, MAX_LEN_OF_ACTIVE_CHANNELS_STRING,
                act_channels_ip.get_lxdialog_output_string());

          logical_ch_cfg->data.chanconf->active_ch = act_channels_ip.active_ch;
          break;

        case INPUT_BOX_BUTTON_HELP:
          tb.show_help_message(lxdialog_path, WANCONFIG_AFT, bound_te1_channels_help_str);
          break;
        }

        //goto again;
      }
      break;

    case IDLE_FLAG_CHAR:
      unsigned int idle_char;

      snprintf(explanation_text, MAX_PATH_LENGTH,
        "Idle Char for Transparent mode (HEX number, i.g 0x7E)");
      snprintf(initial_text, MAX_PATH_LENGTH, "0x%02X",
        logical_ch_cfg->data.chanconf->u.aft.idle_flag);

show_idle_char_input_box:
      input_bx.set_configuration(  lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

      input_bx.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_AFT_LOGICAL_CHANNEL_CFG,
          ("idle char on return: %s\n", input_bx.get_lxdialog_output_string()));

        sscanf(input_bx.get_lxdialog_output_string(), "%X", &idle_char);
        logical_ch_cfg->data.chanconf->u.aft.idle_flag = idle_char;

        Debug(DBG_MENU_AFT_LOGICAL_CHANNEL_CFG,
          ("sscanf() returned : 0x%02X\n", idle_char));

        break;

      case INPUT_BOX_BUTTON_HELP:

        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          "Idle Char for Transparent mode (HEX number, i.g. 0x7E)");
        goto show_idle_char_input_box;
        break;

      }//switch(*selection_index)
      goto again;
      break;

    case CHAN_MTU:
      unsigned int chan_mtu;

      snprintf(explanation_text, MAX_PATH_LENGTH,
        "MTU for this Group of channels");
      snprintf(initial_text, MAX_PATH_LENGTH, "%u",
        logical_ch_cfg->data.chanconf->u.aft.mtu);

show_chan_mtu_input_box:
      input_bx.set_configuration(  lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

      input_bx.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_AFT_LOGICAL_CHANNEL_CFG, ("mtu on return: %s\n",
			       input_bx.get_lxdialog_output_string()));

        sscanf(input_bx.get_lxdialog_output_string(), "%u", &chan_mtu);
        logical_ch_cfg->data.chanconf->u.aft.mtu = chan_mtu;

        Debug(DBG_MENU_AFT_LOGICAL_CHANNEL_CFG, ("sscanf() returned : %u\n", chan_mtu));
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          "MTU for this Group of channels");
        goto show_chan_mtu_input_box;
	
      }//switch(*selection_index)
      goto again;

    case CHAN_MRU:
      unsigned int chan_mru;

      snprintf(explanation_text, MAX_PATH_LENGTH,
        "MRU for this Group of channels");
      snprintf(initial_text, MAX_PATH_LENGTH, "%u",
        logical_ch_cfg->data.chanconf->u.aft.mru);

show_chan_mru_input_box:
      input_bx.set_configuration(  lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

      input_bx.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_AFT_LOGICAL_CHANNEL_CFG,("mru on return: %s\n",
			       input_bx.get_lxdialog_output_string()));

        sscanf(input_bx.get_lxdialog_output_string(), "%u", &chan_mru);
        logical_ch_cfg->data.chanconf->u.aft.mru = chan_mru;

        Debug(DBG_MENU_AFT_LOGICAL_CHANNEL_CFG, ("sscanf() returned : %u\n", chan_mru));
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          "MRU for this Group of channels");
        goto show_chan_mru_input_box;
        
      }//switch(*selection_index)
      goto again;
      
#if 0
    case IGNORE_DCD:
      break;

    case IGNORE_CTS:
      break;
#endif

    case CORE_MODE:

      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s HDLC engine?",
        (logical_ch_cfg->data.chanconf->hdlc_streaming == WANOPT_YES ? "disable" : "enable"));

      if(yes_no_question(   selection_index,
                            lxdialog_path,
                            NO_PROTOCOL_NEEDED,
                            tmp_buff) == NO){
        return NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(logical_ch_cfg->data.chanconf->hdlc_streaming == WANOPT_NO){
          //was disabled - enable
          logical_ch_cfg->data.chanconf->hdlc_streaming = WANOPT_YES;
        }else{
          //was enabled - disable
          logical_ch_cfg->data.chanconf->hdlc_streaming = WANOPT_NO;
        }
        break;
      }

      goto again;
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    {
      Debug(DBG_MENU_AFT_LOGICAL_CHANNEL_CFG,
        ("HELP option selected: %s\n", get_lxdialog_output_string()));

      switch(atoi(get_lxdialog_output_string()))
      {
      case CORE_MODE:
        //do nothing
        break;

      case BOUND_TE1_CHANNELS:
        tb.show_help_message(lxdialog_path, WANCONFIG_AFT, bound_te1_channels_help_str);
        break;

      default:
        tb.show_help_message(lxdialog_path, WANCONFIG_AFT, option_not_implemented_yet_help_str);
        break;
      }
      goto again;
    }
    break;

  case MENU_BOX_BUTTON_EXIT:
    //no check is done here because each menu item checks
    //the input separatly.
    rc = YES;
    break;
  }//switch(*selection_index)

cleanup:
  Debug(DBG_MENU_AFT_LOGICAL_CHANNEL_CFG,
    ("menu_frame_relay_DLCI_configuration::run(): rc: %d\n", rc));

  return rc;
}

