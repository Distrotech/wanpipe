/***************************************************************************
                          menu_net_interface_ip_configuration.cpp  -  description
                             -------------------
    begin                : Mon Mar 29 2004
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

#include "menu_net_interface_ip_configuration.h"
#include "text_box_help.h"
#include "text_box_yes_no.h"
#include "input_box.h"

char* net_if_local_ip_addr_help_str =
"LOCAL IP ADDRESS\n"
"\n"
"WARNING: This program does no checking of any of the IP\n"
"entries. Correct configuration is up to you ;)\n"
"Additional routes may have to be added manually,\n"
"to resolve routing problems.\n"
"\n"
"Define IP address for this interface.  IP addresses\n"
"are written as four dot-separated decimal numbers from 0 to 255\n"
"(e.g. 192.131.56.1).  Usually this address is assigned to you by\n"
"your network administrator or by the Internet service provider.\n"
"\n"
"Default: Consult your ISP\n";

char* net_if_point_to_point_ip_addr_help_str =
"POINTOPOINT IP ADDRESS\n"
"\n"
"Most WAN links are of point-to-point type, which means\n"
"that there is only one machine connected to the other end of the\n"
"link and its address is known in advance.  This option is the\n"
"address of the 'other end' of the link and is usually assigned\n"
"by the network administrator or Internet service provider.\n"
"\n"
"Default: Consult your ISP\n";

char* net_if_netmask_help_str =
"NETMASK IP ADDRESS\n"
"\n"
"Defines network address mask used to separate host\n"
"portion of the IP address from the network portions.\n"
"\n"
"The default of 255.255.255.255 specifies a\n"
"point-to-point connection which is almost always OK.\n"
"\n"
"You may want to override this  when you create\n"
"sub-nets within your particular address class.  In this case\n"
"you have to supply a netmask.  Like the IP address, the\n"
"netmask is written as four dot-separated decimal numbers from\n"
"0 to 255 (e.g. 255.255.255.0).\n"
"\n"
"Default: Consult your ISP\n";

char* net_if_dynamic_if_config_help_str =
"DYNAMIC INTERFACE CONFIGURATION\n"
"\n"
"WANPIPE\n"
"\n"
"The driver will dynamically bring up/down the\n"
"interface to reflect the state of the physical\n"
"link (connected/disconnected).\n"
"\n"
"If you are using SNMP, enable this option.\n"
"The SNMP uses the state of the interface to\n"
"determine the state of the connection.\n"
"\n"
"\n"
"ADSL PPPoA and MULTIPORT PPP\n"
"\n"
"Used by PPP protocol to indicate dynamic IP\n"
"negotiation.\n"
"\n"
"\n"
"If unsure select NO.\n"
"\n"
"Options:\n"
"--------\n"
"YES:  Enable  dynamic interface configuration\n"
"NO :  Disable dynamic interface configuration\n";

char* net_if_default_route_help_str =
"SET DEFAULT ROUTE\n"
"\n"
"This option will create a default route, meaning\n"
"that all network traffic with no specific route found\n"
"in the routing table will be forwarded to this interface.\n"
"\n"
"Default route is useful for connections to the\n"
"'outside world', such as Internet service provider.\n"
"\n"
"If unsure select NO.\n"
"\n"
"Options:\n"
"--------\n"
"YES : Set default route\n"
"NO  : No default route\n";

#define DBG_MENU_NET_INTERFACE_IP_CONFIGURATION 1

menu_net_interface_ip_configuration::menu_net_interface_ip_configuration(
                                        IN char * lxdialog_path,
                                        IN list_element_chan_def* list_element_chan)
{
  Debug(DBG_MENU_NET_INTERFACE_IP_CONFIGURATION,
    ("menu_net_interface_ip_configuration::menu_net_interface_ip_configuration()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->list_element_chan = list_element_chan;
}

menu_net_interface_ip_configuration::~menu_net_interface_ip_configuration()
{
  Debug(DBG_MENU_NET_INTERFACE_IP_CONFIGURATION,
    ("menu_net_interface_ip_configuration::~menu_net_interface_ip_configuration()\n"));
}

int menu_net_interface_ip_configuration::run(
                                  OUT int * selection_index,
                                  IN OUT net_interface_file_reader& interface_file_reader)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int option_selected;
  char exit_dialog;
  char* ip_validation_result;
  chan_def_t* chandef;

  //help text box
  text_box tb;

  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s", get_protocol_string(NO_PROTOCOL_NEEDED));

  Debug(DBG_MENU_NET_INTERFACE_IP_CONFIGURATION, ("menu_net_interface_setup::run()\n"));


again:

  rc = YES;
  option_selected = 0;
  exit_dialog = NO;
  chandef = &list_element_chan->data;

  /////////////////////////////////////////////////
  menu_str = " \"1\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Local IP Addr.-----------> %s\" ",
    replace_new_line_with_zero_term(interface_file_reader.if_config.ipaddr));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  menu_str += " \"2\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Point-to-Point IP Addr.--> %s\" ",
    replace_new_line_with_zero_term(interface_file_reader.if_config.point_to_point_ipaddr));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  menu_str += " \"3\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Net Mask-----------------> %s\" ",
    replace_new_line_with_zero_term(interface_file_reader.if_config.netmask));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  menu_str += " \"4\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Dynamic Interface Config-> %s\" ",
    (chandef->chanconf->if_down == 1 ? "Enabled" : "Disabled"));
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  menu_str += " \"5\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Default Gateway----------> %s\" ",
    (chandef->chanconf->gateway == WANOPT_YES ? "YES" : "NO"));
  menu_str += tmp_buff;

  if(chandef->chanconf->gateway == WANOPT_YES){
    menu_str += " \"6\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"            Gateway IP---> %s\" ",
      replace_new_line_with_zero_term(interface_file_reader.if_config.gateway));
    menu_str += tmp_buff;
  }
  /*
  menu_str += " \"5\" ";
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Default Gateway----------> %s\" ",
    (interface_file_reader.if_config.gateway[0] != '\0' ? "YES" : "NO"));
  menu_str += tmp_buff;

  if(interface_file_reader.if_config.gateway[0] != '\0'){
    menu_str += " \"6\" ";
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"            Gateway IP---> %s\" ",
      replace_new_line_with_zero_term(interface_file_reader.if_config.gateway));
    menu_str += tmp_buff;
  }
  */
  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nIP parameters for interface: %s", chandef->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "IP CONFIGURATION",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          6,//number of items in the menu, including the empty lines
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

  exit_dialog = NO;
  switch(*selection_index)
  {
  case MENU_BOX_BUTTON_SELECT:
    Debug(DBG_MENU_NET_INTERFACE_IP_CONFIGURATION,
      ("menu_net_interface_ip_configuration: option selected for editing: %s\n",
      get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1://local ip
show_local_ip_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify the Local IP Address");
      snprintf(initial_text, MAX_PATH_LENGTH, "%s",
        replace_new_line_with_zero_term(interface_file_reader.if_config.ipaddr));

      inb.set_configuration(  lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

      inb.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_NET_INTERFACE_IP_CONFIGURATION,
          ("Local IP on return: %s\n", inb.get_lxdialog_output_string()));

        snprintf(tmp_buff, MAX_PATH_LENGTH, inb.get_lxdialog_output_string());
        ip_validation_result = validate_ipv4_address_string(tmp_buff);
        if(ip_validation_result != NULL){
          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED, ip_validation_result);
          goto show_local_ip_input_box;
        }

        snprintf(interface_file_reader.if_config.ipaddr, IF_CONFIG_BUF_LEN,
          inb.get_lxdialog_output_string());
        break;

      case INPUT_BOX_BUTTON_HELP:

        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_local_ip_addr_help_str);
        goto show_local_ip_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case 2://point-to-point ip
show_point_to_point_ip_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify the Point-to-Point IP Address");
      snprintf(initial_text, MAX_PATH_LENGTH, "%s",
        replace_new_line_with_zero_term(interface_file_reader.if_config.point_to_point_ipaddr));

      inb.set_configuration(  lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

      inb.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_NET_INTERFACE_IP_CONFIGURATION,
          ("Point-to-point IP on return: %s\n", inb.get_lxdialog_output_string()));

        snprintf(tmp_buff, MAX_PATH_LENGTH, inb.get_lxdialog_output_string());
        ip_validation_result = validate_ipv4_address_string(tmp_buff);
        if(ip_validation_result != NULL){
          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED, ip_validation_result);
          goto show_point_to_point_ip_input_box;
        }

        snprintf(interface_file_reader.if_config.point_to_point_ipaddr, IF_CONFIG_BUF_LEN,
          inb.get_lxdialog_output_string());
        break;

      case INPUT_BOX_BUTTON_HELP:

        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          net_if_point_to_point_ip_addr_help_str);
        goto show_point_to_point_ip_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case 3://netmask
show_netmask_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify the Net Mask");
      snprintf(initial_text, MAX_PATH_LENGTH, "%s",
        replace_new_line_with_zero_term(interface_file_reader.if_config.netmask));

      inb.set_configuration(  lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

      inb.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_NET_INTERFACE_IP_CONFIGURATION,
          ("Netmask on return: %s\n", inb.get_lxdialog_output_string()));

        snprintf(tmp_buff, MAX_PATH_LENGTH, inb.get_lxdialog_output_string());
        ip_validation_result = validate_ipv4_address_string(tmp_buff);
        if(ip_validation_result != NULL){
          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED, ip_validation_result);
          goto show_netmask_input_box;
        }

        snprintf(interface_file_reader.if_config.netmask, IF_CONFIG_BUF_LEN,
          inb.get_lxdialog_output_string());
        break;

      case INPUT_BOX_BUTTON_HELP:

        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_netmask_help_str);
        goto show_netmask_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case 4:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s Dynamic Interface Configuration?",
        (chandef->chanconf->if_down == 0 ? "Enable" : "Disable"));

      if(yes_no_question(   selection_index,
                            lxdialog_path,
                            NO_PROTOCOL_NEEDED,
                            tmp_buff) == NO){
        return NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        if(chandef->chanconf->if_down == 0){
          //was disabled - enable
          chandef->chanconf->if_down = 1;
        }else{
          //was enabled - disable
          chandef->chanconf->if_down = 0;
        }
        break;
      }
      break;

    case 5:
      snprintf(tmp_buff, MAX_PATH_LENGTH,
"Would you like to set up this interface\n\
as a default route?");

      if(yes_no_question(   selection_index,
                            lxdialog_path,
                            NO_PROTOCOL_NEEDED,
                            tmp_buff) == NO){
        return NO;
      }

      switch(*selection_index)
      {
      case YES_NO_TEXT_BOX_BUTTON_YES:
        chandef->chanconf->gateway = WANOPT_YES;

	//if p-to-p is know use it for gateway, if not put '0.0.0.0'
	if(interface_file_reader.if_config.point_to_point_ipaddr[0] != '\0'){
		strlcpy(interface_file_reader.if_config.gateway,
		      interface_file_reader.if_config.point_to_point_ipaddr,
		      IF_CONFIG_BUF_LEN);
	}else{
		strlcpy(interface_file_reader.if_config.gateway, "0.0.0.0",IF_CONFIG_BUF_LEN);
	}
		
        break;

      case YES_NO_TEXT_BOX_BUTTON_NO:
        //was enabled - disable
        chandef->chanconf->gateway = WANOPT_NO;
        memset(interface_file_reader.if_config.gateway, 0x00, IF_CONFIG_BUF_LEN);
        break;
      }
      break;

    case 6:
show_gateway_ip_input_box:
      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify the Gateway IP Address");
      snprintf(initial_text, MAX_PATH_LENGTH, "%s",
        replace_new_line_with_zero_term(interface_file_reader.if_config.gateway));

      inb.set_configuration(  lxdialog_path,
                              backtitle,
                              explanation_text,
                              INPUT_BOX_HIGTH,
                              INPUT_BOX_WIDTH,
                              initial_text);

      inb.show(selection_index);

      switch(*selection_index)
      {
      case INPUT_BOX_BUTTON_OK:
        Debug(DBG_MENU_NET_INTERFACE_IP_CONFIGURATION,
          ("Gateway IP on return: %s\n", inb.get_lxdialog_output_string()));

        snprintf(tmp_buff, MAX_PATH_LENGTH, inb.get_lxdialog_output_string());
        ip_validation_result = validate_ipv4_address_string(tmp_buff);
        if(ip_validation_result != NULL){
          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED, ip_validation_result);
          goto show_gateway_ip_input_box;
        }

        snprintf(interface_file_reader.if_config.gateway, IF_CONFIG_BUF_LEN,
          inb.get_lxdialog_output_string());
        break;

      case INPUT_BOX_BUTTON_HELP:

        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_default_route_help_str);
        goto show_gateway_ip_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    Debug(DBG_MENU_NET_INTERFACE_IP_CONFIGURATION,
      ("MENU_BOX_BUTTON_HELP: menu_net_interface_ip_configuration: option selected for editing: %s\n",
      get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case 1:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_local_ip_addr_help_str);
      break;

    case 2:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
        net_if_point_to_point_ip_addr_help_str);
      break;

    case 3:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_netmask_help_str);
      break;

    case 4:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_dynamic_if_config_help_str);
      break;

    case 5:
    case 6:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, net_if_default_route_help_str);
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

