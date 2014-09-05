/***************************************************************************
                          menu_net_interface_operation_mode.cpp  -  description
                             -------------------
    begin                : Fri Mar 26 2004
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

#include "menu_net_interface_operation_mode.h"
#include "text_box.h"
#include "conf_file_reader.h"
#include "wancfg.h"

char* net_if_operational_mode_help_str =
"Operation Mode\n"
"==============\n"
"\n"
"Wanpipe drivers can work in four modes:\n"
"\n"
"WANPIPE Mode\n"
"------------\n"
"Driver links to the IP stack, and behaves like\n"
"an Ethernet device. The linux routing stack\n"
"determines how to send and receive packets.\n"
"\n"
"API Mode:\n"
"---------\n"
"Wanpipe driver uses a special RAW socket to\n"
"directly couple the user application to the driver.\n"
"Thus, user application can send and receive raw data\n"
"to and from the wanpipe driver.\n"
"\n"
"Note: API option is currently only available on\n"
"CHDLC and Frame Relay protocols.\n"
"\n"
"BRIDGE Mode:\n"
"------------\n"
"For each frame relay network interface\n"
"a virtual ethernet network interface is configured.\n"
"\n"
"Since the virtual network interface is on a BRIDGE\n"
"device it is linked into the kernel bridging code.\n"
"\n"
"Once linked to the kenrel bridge, ethernet\n"
"frames can be transmitted over a WANPIPE frame\n"
"relay connections.\n"
"\n"
"BRIDGED NODE Mode:\n"
"------------------\n"
"\n"
"For each frame relay network interface\n"
"a virtual ethernet netowrk interface is configured.\n"
"\n"
"However, this interface is not on the bridge itself,\n"
"it is connected remotely via frame relay link to the\n"
"bridge, thus it is  a node.\n"
"Since the remote end of the frame link is connected\n"
"to the bridge, this interface will be able to send\n"
"and receive ethernet frames over the frame relay\n"
"line.\n"
"\n"
"Note: Bridging is only supprted under Frame Relay\n"
"\n"
"\n"
"PPPoE Mode:\n"
"-----------\n"
"\n"
"The ADSL card configured for Bridged LLC Ethernet\n"
"over ATM encapsulation, usually uses PPPoE protocol\n"
"to connect to the ISP.\n"
"\n"
"With this option enabled, NO IP setup is necessary\n"
"because PPPoE protocol will obtain IP info from\n"
"the ISP.\n"
"\n"
"\n"
"ANNEXG Mode:\n"
"------------\n"
"\n"
"The ANNEXG mode enables wanpipe kernel\n"
"protocol layers that will run on top of the\n"
"current interface.\n"
"\n"
"Supporter MPAPI protocol are: X25/LAPB\n";

#define DBG_MENU_NET_INTERFACE_OPERATION_MODE 1

menu_net_interface_operation_mode::
  menu_net_interface_operation_mode( IN char * lxdialog_path,
                                     IN list_element_chan_def* list_element_chan)
{
  Debug(DBG_MENU_NET_INTERFACE_OPERATION_MODE,
    ("menu_net_interface_operation_mode::menu_net_interface_operation_mode()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  
  this->list_element_chan = list_element_chan;
}

menu_net_interface_operation_mode::~menu_net_interface_operation_mode()
{
  Debug(DBG_MENU_NET_INTERFACE_OPERATION_MODE,
    ("menu_net_interface_operation_mode::~menu_net_interface_operation_mode()\n"));
}

int menu_net_interface_operation_mode::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  int number_of_items_in_menu;
  chan_def_t* chandef;
  int protocol;

  //help text box
  text_box tb;

  Debug(DBG_MENU_NET_INTERFACE_OPERATION_MODE, ("menu_net_interface_operation_mode::run()\n"));

again:

  rc = YES;
  exit_dialog = NO;
  chandef = &list_element_chan->data;
  
  protocol = chandef->chanconf->config_id;

  /////////////////////////////////////////////////
  //passing arguments by reference.
  form_operation_modes_menu_for_protocol(protocol, menu_str, number_of_items_in_menu);

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nPlease select Interface Operation Mode.");

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "INTERFACE OPERATION MODE",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          number_of_items_in_menu,//number of items in the menu, including the empty lines
                          (char*)menu_str.c_str()
                          ) == NO){
    rc = NO;
    goto cleanup;
  }

  if(show(selection_index) == NO){
    rc = NO;
    goto cleanup;
	  }
  //////////////////////////////////////////////////////////////////////////////////////

  Debug(DBG_MENU_NET_INTERFACE_OPERATION_MODE,
    ("net_if_setup: *selection_index: %d\n", *selection_index));

  exit_dialog = NO;
  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_NET_INTERFACE_OPERATION_MODE,
      ("net_if_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1:
      chandef->usedby = WANPIPE;
      exit_dialog = YES;
      break;

    case 2:
      chandef->usedby = API;
      exit_dialog = YES;
      break;

    case 3:
      chandef->usedby = BRIDGE;
      exit_dialog = YES;
      break;

    case 4:
      chandef->usedby = BRIDGE_NODE;
      exit_dialog = YES;
      break;

    case 5:
      chandef->usedby = WP_SWITCH;
      exit_dialog = YES;
      break;

    case 6:
      chandef->usedby = STACK;
      exit_dialog = YES;
      break;

    case 7:
      chandef->usedby = ANNEXG;
      exit_dialog = YES;
      break;

    case 8:
      chandef->usedby = TDM_VOICE;
      exit_dialog = YES;
      break;

    case 9:
      chandef->usedby = PPPoE;
      exit_dialog = YES;
      break;

    case 10:
      chandef->usedby = TTY;
      exit_dialog = YES;
      break;

    case 11:
      chandef->usedby = TDM_API;
      exit_dialog = YES;
      break;

    case 12:
      chandef->usedby = WP_NETGRAPH;
      exit_dialog = YES;
      break;

    case 13:
      chandef->usedby = TDM_VOICE_API;
      exit_dialog = YES;
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
    }
    break;

  case MENU_BOX_BUTTON_HELP:

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_operational_mode_help_str);
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
    }
    break;

  case MENU_BOX_BUTTON_EXIT:
    exit_dialog = YES;
    break;
  }//switch(*selection_index)

  if(exit_dialog == NO){
    goto again;
  }

cleanup:
  return rc;
}

int menu_net_interface_operation_mode::form_operation_modes_menu_for_protocol(
                                                                  int protocol,
                                                                  string& operation_modes_str,
                                                                  int& number_of_items_in_menu)
{
  int rc = YES;
  operation_modes_str = "";

  conf_file_reader* cfr = (conf_file_reader*)global_conf_file_reader_ptr;
  link_def_t * link_defs =  cfr->link_defs;
  wandev_conf_t *linkconf = cfr->link_defs->linkconf;
  wan_adsl_conf_t* adsl_cfg = &linkconf->u.adsl;
  
  Debug(DBG_MENU_NET_INTERFACE_OPERATION_MODE, ("protocol: %d\n", protocol));
 
  number_of_items_in_menu = 0;

  switch(protocol)
  {
  case PROTOCOL_TDM_VOICE:
    operation_modes_str = QUOTED_8;
    operation_modes_str += OP_MODE_VoIP;
    
    number_of_items_in_menu = 1;
    break;

  case PROTOCOL_TDM_VOICE_API:
    operation_modes_str = QUOTED_13;
    operation_modes_str += OP_MODE_TDMV_API;
    
    number_of_items_in_menu = 1;
    break;
    
  case WANCONFIG_X25:
    operation_modes_str = QUOTED_1;
    operation_modes_str += OP_MODE_WANPIPE;

    operation_modes_str += QUOTED_2;
    operation_modes_str += OP_MODE_API;

    number_of_items_in_menu = 2;
    break;

  //case WANCONFIG_FR:
  case WANCONFIG_MFR:
    operation_modes_str = QUOTED_1;
    operation_modes_str += OP_MODE_WANPIPE;
    number_of_items_in_menu++;

    operation_modes_str += QUOTED_2;
    operation_modes_str += OP_MODE_API;
    number_of_items_in_menu++;

#if defined(__LINUX__)
    operation_modes_str += QUOTED_3;
    operation_modes_str += OP_MODE_BRIDGE;
    number_of_items_in_menu++;

    operation_modes_str += QUOTED_4;
    operation_modes_str += OP_MODE_BRIDGE_NODE;
    number_of_items_in_menu++;
#endif
    operation_modes_str += QUOTED_6;
    operation_modes_str += OP_MODE_STACK;
    number_of_items_in_menu++;

#if defined(__FreeBSD__)
    operation_modes_str += QUOTED_12;
    operation_modes_str += OP_MODE_NETGRAPH;
    number_of_items_in_menu ++;
#endif

    break;

  //case WANCONFIG_PPP:
  case WANCONFIG_MPPP://and WANCONFIG_MPROT too
    operation_modes_str = QUOTED_1;
    operation_modes_str += OP_MODE_WANPIPE;
    number_of_items_in_menu++;

#if defined(__LINUX__)
    operation_modes_str += QUOTED_3;
    operation_modes_str += OP_MODE_BRIDGE;
    number_of_items_in_menu++;

    operation_modes_str += QUOTED_4;
    operation_modes_str += OP_MODE_BRIDGE_NODE;
    number_of_items_in_menu++;
#endif
    operation_modes_str += QUOTED_6;
    operation_modes_str += OP_MODE_STACK;
    number_of_items_in_menu++;

#if defined(__FreeBSD__)
    operation_modes_str += QUOTED_12;
    operation_modes_str += OP_MODE_NETGRAPH;
    number_of_items_in_menu ++;
#endif

    break;

  case WANCONFIG_HDLC:
  
    Debug(DBG_MENU_NET_INTERFACE_OPERATION_MODE, ("WANCONFIG_HDLC\n"));

    operation_modes_str = QUOTED_1;
    operation_modes_str += OP_MODE_WANPIPE;
    number_of_items_in_menu ++;

    operation_modes_str += QUOTED_2;
    operation_modes_str += OP_MODE_API;
    number_of_items_in_menu ++;
  
    if(linkconf->card_type == WANOPT_AFT || linkconf->card_type == WANOPT_AFT104){
      operation_modes_str += QUOTED_11;
      operation_modes_str += OP_MODE_TDM_API;
      number_of_items_in_menu ++;
    }
    
#if defined(__FreeBSD__)
    operation_modes_str += QUOTED_12;
    operation_modes_str += OP_MODE_NETGRAPH;
    number_of_items_in_menu ++;
#endif

    //treaded as a protocol
    //operation_modes_str += QUOTED_8;
    //operation_modes_str += OP_MODE_VoIP;
 
    //treaded as a protocol
    //operation_modes_str += QUOTED_10;
    //operation_modes_str += OP_MODE_TTY;

    break;
  
  //case WANCONFIG_CHDLC:
  case WANCONFIG_MPCHDLC:

    operation_modes_str = QUOTED_1;
    operation_modes_str += OP_MODE_WANPIPE;
    number_of_items_in_menu ++;

    operation_modes_str += QUOTED_2;
    operation_modes_str += OP_MODE_API;
    number_of_items_in_menu ++;

#if defined(__LINUX__)
    operation_modes_str += QUOTED_3;
    operation_modes_str += OP_MODE_BRIDGE;
    number_of_items_in_menu ++;

    operation_modes_str += QUOTED_4;
    operation_modes_str += OP_MODE_BRIDGE_NODE;
    number_of_items_in_menu ++;
#endif
    operation_modes_str += QUOTED_6;
    operation_modes_str += OP_MODE_STACK;
    number_of_items_in_menu ++;

#if defined(__FreeBSD__)
    operation_modes_str += QUOTED_12;
    operation_modes_str += OP_MODE_NETGRAPH;
    number_of_items_in_menu ++;
#endif
    break;

  case WANCONFIG_LIP_ATM:

    operation_modes_str = QUOTED_1;
    operation_modes_str += OP_MODE_WANPIPE;

    operation_modes_str += QUOTED_6;
    operation_modes_str += OP_MODE_STACK;

    number_of_items_in_menu = 2;
    break;

  case WANCONFIG_BSC:
    operation_modes_str = QUOTED_2;
    operation_modes_str += OP_MODE_API;
    number_of_items_in_menu = 1;
    break;
/*
  case WANCONFIG_MPPP://and WANCONFIG_MPROT too
    operation_modes_str = QUOTED_1;
    operation_modes_str += OP_MODE_WANPIPE;

    //note: Nenad will change the name soon
    //operation_modes_str += QUOTED_5;
    //operation_modes_str += OP_MODE_ANNEXG;

    number_of_items_in_menu = 1;
    break;
*/
  case WANCONFIG_BITSTRM:
    operation_modes_str = QUOTED_1;
    operation_modes_str += OP_MODE_WANPIPE;


    operation_modes_str += QUOTED_2;
    operation_modes_str += OP_MODE_API;

    operation_modes_str += QUOTED_5;
    operation_modes_str += OP_MODE_SWITCH;

    operation_modes_str += QUOTED_8;
    operation_modes_str += OP_MODE_VoIP;

    number_of_items_in_menu = 4;
    break;

  case WANCONFIG_EDUKIT:
    operation_modes_str = QUOTED_1;
    operation_modes_str += OP_MODE_WANPIPE;

    operation_modes_str += QUOTED_2;
    operation_modes_str += OP_MODE_API;

    number_of_items_in_menu = 2;
    break;

  case WANCONFIG_SS7:
    operation_modes_str = QUOTED_2;
    operation_modes_str += OP_MODE_API;
    number_of_items_in_menu = 1;
    break;

  case WANCONFIG_BSCSTRM:
    operation_modes_str = QUOTED_2;
    operation_modes_str += OP_MODE_API;
    number_of_items_in_menu = 1;
    break;

  case WANCONFIG_TTY:
    operation_modes_str = QUOTED_10;
    operation_modes_str += OP_MODE_TTY;

    number_of_items_in_menu = 1;
    break;

  case WANCONFIG_LAPB:

    operation_modes_str += QUOTED_2;
    operation_modes_str += OP_MODE_API;

    operation_modes_str += QUOTED_6;
    operation_modes_str += OP_MODE_STACK;

    number_of_items_in_menu = 2;
    break;

  case WANCONFIG_ADSL:
    //Valid Operation Mode depends on "Encapsulation Type"
 
    switch(adsl_cfg->EncapMode)
    {
    case RFC_MODE_BRIDGED_ETH_LLC:  //"Bridged Eth LLC over ATM (PPPoE)"  both
    case RFC_MODE_BRIDGED_ETH_VC:   //"Bridged Eth VC over ATM"		  both
      operation_modes_str = QUOTED_1;
      operation_modes_str += OP_MODE_WANPIPE;

      operation_modes_str += QUOTED_9;
      operation_modes_str += OP_MODE_PPPoE;
    
      number_of_items_in_menu = 2;
      break;
      
    case RFC_MODE_ROUTED_IP_LLC:    //"Classical IP LLC over ATM"	  wanpipe only
    case RFC_MODE_ROUTED_IP_VC:	    //"Routed IP VC over ATM"		  wanpipe only
    case RFC_MODE_RFC1577_ENCAP:    //"RFC_MODE_RFC1577_ENCAP"		  ??? not defined in 'wancfg_adv'
    case RFC_MODE_PPP_LLC:	    //"PPP (LLC) over ATM"		  wanpipe only
    case RFC_MODE_PPP_VC:	    //"PPP (VC) over ATM (PPPoA)"	  wanpipe only
      operation_modes_str = QUOTED_1;
      operation_modes_str += OP_MODE_WANPIPE;
      
      number_of_items_in_menu = 1;
      break;
    }
    
    break;

  case WANCONFIG_SDLC:
    operation_modes_str = QUOTED_2;
    operation_modes_str += OP_MODE_API;
    number_of_items_in_menu = 1;
    break;

  case WANCONFIG_ATM:
    operation_modes_str = QUOTED_1;
    operation_modes_str += OP_MODE_WANPIPE;

    operation_modes_str += QUOTED_3;
    operation_modes_str += OP_MODE_BRIDGE;

    number_of_items_in_menu = 2;
    break;

  case WANCONFIG_POS:
    //"Point-of-Sale Mode"
    operation_modes_str = QUOTED_2;
    operation_modes_str += OP_MODE_API;

    number_of_items_in_menu = 1;
    break;

  case WANCONFIG_AFT:
    //"AFT Mode"
    if(link_defs->card_version != A200_ADPTR_ANALOG ){

      operation_modes_str = QUOTED_1;
      operation_modes_str += OP_MODE_WANPIPE;
      number_of_items_in_menu ++;

      operation_modes_str += QUOTED_2;
      operation_modes_str += OP_MODE_API;
      number_of_items_in_menu ++;

      operation_modes_str += QUOTED_11;
      operation_modes_str += OP_MODE_TDM_API;
      number_of_items_in_menu ++;

      //operation_modes_str += QUOTED_8;
      //operation_modes_str += OP_MODE_VoIP;

      //FIXIT: for two levels user will not be able to select STACK,
      //because it is set automatically on Protocol selcection
      //in "menu_aft_logical_channel_cfg::run()"
//    operation_modes_str += QUOTED_6;
//    operation_modes_str += OP_MODE_STACK;

#if defined(__FreeBSD__)
      operation_modes_str += QUOTED_12;
      operation_modes_str += OP_MODE_NETGRAPH;
      number_of_items_in_menu ++;
#endif

    }else{
      operation_modes_str += QUOTED_2;
      operation_modes_str += OP_MODE_API;

      operation_modes_str += QUOTED_11;
      operation_modes_str += OP_MODE_TDM_API;

      number_of_items_in_menu = 2;
    }
    break;

  case WANCONFIG_ADCCP:
    //Special HDLC LAPB Protocol
    operation_modes_str = QUOTED_2;
    operation_modes_str += OP_MODE_API;
    number_of_items_in_menu = 1;
    break;

  case WANCONFIG_MLINK_PPP:
    //Sync/Async TTY PPP - ("Multi-Link PPP Protocol")
    //Mode is irrelevant
    rc = NO_OPER_MODE;
    break;
/*
  //internal Sangoma use only
  case WANCONFIG_DEBUG:
    break;

  case WANCONFIG_GENERIC:
    break;
*/

  default:
    ERR_DBG_OUT(("Invalid protocol: %d!! (%s)\n", protocol,
	       get_protocol_string(protocol)));
    rc = NO;
    break;
  }

  return rc;
}
