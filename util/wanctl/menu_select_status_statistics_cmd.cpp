/***************************************************************************
                          menu_select_status_statistics_cmd.cpp  -  description
                             -------------------
    begin                : Tue Mar 2 2004
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

#include "menu_select_status_statistics_cmd.h"
#include "text_box.h"

extern char* tmp_hwprobe_file_full_path;

#define DBG_SELECT_STATUS_STATS_CMD 1

enum OPERATIONAL_CMD{
  EMPTY_LINE,
  CHDLC_LINK_STATUS,
  CHDLC_OPERATIONAL_STATS,
  CHDLC_SLARP_AND_CDP_STATS,
  CHDLC_READ_CONFIGURATION,

  FR_LINK_STATUS,
  FR_SIGNALLING_STATS,
  FR_DLCI_STATS,//hardcoded for DLCI 16
  
  PPP_LINK_STATUS,
  PPP_GLOBAL_STATS,
  PPP_PACKET_STATS,
  PPP_LCP_STATS,

  ATM_LINK_STATUS,
  ATM_GLOBAL_STATS,
  ATM_OPERATIONAL_STATS
  
};

menu_select_status_statistics_cmd::menu_select_status_statistics_cmd( IN char * lxdialog_path,
                                                    IN conf_file_reader* cfr)
{
  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = cfr;
}

menu_select_status_statistics_cmd::~menu_select_status_statistics_cmd()
{
}

int menu_select_status_statistics_cmd::run(OUT int * selection_index)
{
  string menu_str;
  int rc, number_of_items;
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int option_selected;
  char exit_dialog;
  //help text box
  text_box tb;
  char is_valid_command;
  char backtitle[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "%s: ", WANCFG_PROGRAM_NAME);
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s", get_protocol_string(NO_PROTOCOL_NEEDED));

  Debug(DBG_SELECT_STATUS_STATS_CMD, ("menu_select_status_statistics_cmd::run()\n"));

again:

  rc = YES;
  option_selected = 0;
  exit_dialog = NO;
  menu_str = " ";
  is_valid_command = 1;
  number_of_items = 0;

  //////////////////////////////////////////////////////////////////////////////////////
  
  switch(cfr->protocol)
  {
  case WANCONFIG_CHDLC:	

  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CHDLC_LINK_STATUS);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Link Status\" ");
  	menu_str += tmp_buff;

  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CHDLC_OPERATIONAL_STATS);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Operational Statistics\" ");
  	menu_str += tmp_buff;

	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CHDLC_SLARP_AND_CDP_STATS);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"SLARP and CDP Statistics\" ");
  	menu_str += tmp_buff;
		
	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CHDLC_READ_CONFIGURATION);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Read CHDLC Configuration\" ");
  	menu_str += tmp_buff;

	number_of_items += 4;		
	break;

  case WANCONFIG_FR:
 
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", FR_LINK_STATUS);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Read Link Status\" ");
  	menu_str += tmp_buff;

  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", FR_SIGNALLING_STATS);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Read Signalling Statistics\" ");
  	menu_str += tmp_buff;

	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", FR_DLCI_STATS);//hardcoded for dlci 16
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Read Statistics for DLCI 16\" ");
  	menu_str += tmp_buff;

	number_of_items += 3;
	break;

  case WANCONFIG_PPP:
 	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ",  PPP_LINK_STATUS);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Read Link Status\" ");
  	menu_str += tmp_buff;

  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", PPP_GLOBAL_STATS);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Read Global Statistics\" ");
  	menu_str += tmp_buff;

	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", PPP_PACKET_STATS);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Read Packet Statistics\" ");
  	menu_str += tmp_buff;

	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", PPP_LCP_STATS);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Read LCP Statistics\" ");
  	menu_str += tmp_buff;
	
	number_of_items += 4;
	break;	

  case WANCONFIG_ATM:
	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ",  ATM_LINK_STATUS);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Read ATM PHY Status\" ");
  	menu_str += tmp_buff;
/*
 * useless for education
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_GLOBAL_STATS);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Read Global Statistics\" ");
  	menu_str += tmp_buff;
*/
	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_OPERATIONAL_STATS);
  	menu_str += tmp_buff;
  	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Read ATM PHY Operational Statistics\" ");
  	menu_str += tmp_buff;

	number_of_items += 2;
	break;
  }


  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nNote: to exit a command press 'Enter'.");

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "STATUS AND STATISTICS COMMANDS",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          number_of_items,//number of items in the menu, including the empty lines
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
    Debug(DBG_SELECT_STATUS_STATS_CMD,
      ("menu_net_interface_ip_configuration: option selected for editing: %s\n",
      get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case EMPTY_LINE:
      is_valid_command = 0;//do nothing
      break;

    case CHDLC_LINK_STATUS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1chdlc -c xl");
      break;

    case CHDLC_OPERATIONAL_STATS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1chdlc -c so");
      break;

    case CHDLC_SLARP_AND_CDP_STATS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1chdlc -c ss");
      break;

    case CHDLC_READ_CONFIGURATION:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1chdlc -c crc");
      break;
	/*
        snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1svc1 -c xl");
        snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1svc1 -c sx");
	*/
    
    case FR_LINK_STATUS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1fr -c xl");
      break;

    case FR_SIGNALLING_STATS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1fr -c sg");
      break;
	      
    case FR_DLCI_STATS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1fr -c sd -d 16");
      break;
	
    case PPP_LINK_STATUS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1ppp -c xs");
      break;

    case PPP_GLOBAL_STATS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1ppp -c sg");
      break;

    case PPP_PACKET_STATS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1ppp -c sp");
      break;

    case PPP_LCP_STATS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1ppp -c slpc");
      break;
  
    case ATM_LINK_STATUS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1atm1 -c xl");
      break;

    case ATM_GLOBAL_STATS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1atm1 -c sg");
      break;
      
    case ATM_OPERATIONAL_STATS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1atm1 -c so");
      break;
      
    default:
      ERR_DBG_OUT(("Command invalid for protocol %d !\n",
        cfr->protocol));

      rc = NO;
      exit_dialog = YES;
      is_valid_command = 0;	
    }

    if(is_valid_command == 0){
	break;
    }
repeat_cmd:

    run_system_command("clear");
    if(run_system_command(tmp_buff)){
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, "Failed to run command.");
    }
    
    if(repeat_command() == YES){
      goto repeat_cmd;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    Debug(DBG_SELECT_STATUS_STATS_CMD, ("MENU_BOX_BUTTON_HELP: option selected for editing: %s\n",
      get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case EMPTY_LINE:
      break;
/*
    default:
      
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
*/
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

