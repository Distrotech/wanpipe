/***************************************************************************
                          menu_select_trace_cmd.cpp  -  description
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

#include "menu_select_trace_cmd.h"
#include "text_box.h"
#include "unixio.h"

#define DBG_SELECT_TRACE_CMD 1

enum OPERATIONAL_CMD{
  RAW_TRACE,
  INTERPRETED_TRACE,
  TCP_IP_TRACE,	//run 'tcpdump' utility
  ATM_PHY_TRACE_RAW_ALL_CELLS,
  ATM_PHY_TRACE_RAW_NO_IDLE_CELLS,
  ATM_PHY_TRACE_INTERPRETED_ALL_CELLS,
  ATM_PHY_TRACE_INTERPRETED_NO_IDLE_CELLS,
  ATM_DATA_TRACE_RAW,
  ATM_DATA_TRACE_INTERPRETED//IP V4 interpretation
};

menu_select_trace_cmd::menu_select_trace_cmd( IN char * lxdialog_path,
                                              IN conf_file_reader* cfr)
{
  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = cfr;
}

menu_select_trace_cmd::~menu_select_trace_cmd()
{
}

int menu_select_trace_cmd::run(OUT int * selection_index)
{
  string menu_str;
  int rc, number_of_items;
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int option_selected;
  char exit_dialog;
  //help text box
  text_box tb;
  
  Debug(DBG_SELECT_TRACE_CMD, ("menu_select_trace_cmd::run()\n"));

again:

  rc = YES;
  option_selected = 0;
  exit_dialog = NO;
  menu_str = " ";

  /////////////////////////////////////////////////

  switch(cfr->protocol)
  {
  case WANCONFIG_ATM:
	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_PHY_TRACE_RAW_ALL_CELLS);
    	menu_str += tmp_buff;
    	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"PHY Level Trace -Raw\" ");
    	menu_str += tmp_buff;

	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_PHY_TRACE_RAW_NO_IDLE_CELLS);
    	menu_str += tmp_buff;
    	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"PHY Level Trace -Raw (surpress idle)\" ");
    	menu_str += tmp_buff;
	
	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_PHY_TRACE_INTERPRETED_ALL_CELLS);
    	menu_str += tmp_buff;
    	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"PHY Level Trace -Interpreted\" ");
    	menu_str += tmp_buff;

	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_PHY_TRACE_INTERPRETED_NO_IDLE_CELLS);
    	menu_str += tmp_buff;
    	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"PHY Level Trace -Interpreted (surpress idle)\" ");
    	menu_str += tmp_buff;

	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_DATA_TRACE_RAW);
    	menu_str += tmp_buff;
    	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"ATM Level Trace -Raw\" ");
    	menu_str += tmp_buff;

	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", ATM_DATA_TRACE_INTERPRETED);
    	menu_str += tmp_buff;
    	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"ATM Level Trace -Interpreted\" ");
    	menu_str += tmp_buff;

	number_of_items = 7;
	break;

  default:
	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", RAW_TRACE);
    	menu_str += tmp_buff;
    	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Raw (Hex) Trace\" ");
    	menu_str += tmp_buff;

    	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", INTERPRETED_TRACE);
    	menu_str += tmp_buff;
    	snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Interpreted (at Protocol Level) Trace\" ");
    	menu_str += tmp_buff;

	number_of_items = 3;
  }
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TCP_IP_TRACE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"TCP\\IP Trace (using 'tcpdump' utility)\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"------------------------------------------\
\n         Commands used to start Line Trace.\
\n------------------------------------------\
\nNote: to exit 'Raw' and 'Interpreted' trace\
\n      press 'Enter'.                       \
\n      To exit 'TCP\\IP' trace press 'Cntrl+C'.");

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "LINE TRACE COMMANDS",
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
    Debug(DBG_SELECT_TRACE_CMD,
      ("menu_net_interface_ip_configuration: option selected for editing: %s\n",
      get_lxdialog_output_string()));

repeat_trace:
    
    switch(atoi(get_lxdialog_output_string()))
    {
    case ATM_PHY_TRACE_RAW_ALL_CELLS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1atm1 -c trphy");
      break;

    case ATM_PHY_TRACE_RAW_NO_IDLE_CELLS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1atm1 -c trl1");
      break;
      
    case ATM_PHY_TRACE_INTERPRETED_ALL_CELLS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1atm1 -c tiphy");
      break;

    case ATM_PHY_TRACE_INTERPRETED_NO_IDLE_CELLS:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1atm1 -c til1");
      break;

    case ATM_DATA_TRACE_RAW:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1atm1 -c trl2");
      break;
      
    case ATM_DATA_TRACE_INTERPRETED:
      snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1atm1 -c ti4");
      break;
	  
    case RAW_TRACE:
      switch(cfr->protocol)
      {
      case WANCONFIG_CHDLC:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1chdlc -c tr");
        break;

      case WANCONFIG_PPP:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1ppp -c tr");
        break;

      case WANCONFIG_FR:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1fr -c tr");
        break;

      case WANCONFIG_X25:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1svc1 -c tr");
        break;
      }
      break;

    case INTERPRETED_TRACE:

      switch(cfr->protocol)
      {
      case WANCONFIG_CHDLC:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1chdlc -c ti");
        break;

      case WANCONFIG_PPP:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1ppp -c ti");
        break;

      case WANCONFIG_FR:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1fr -c ti4");
        break;

      case WANCONFIG_X25:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "wanpipemon -i wp1svc1 -c ti");
        break;
      }

      break;

    case TCP_IP_TRACE:
      switch(cfr->protocol)
      {
      case WANCONFIG_ATM:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "tcpdump -i wp1atm1");
        break;

      case WANCONFIG_CHDLC:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "tcpdump -i wp1chdlc");
        break;

      case WANCONFIG_PPP:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "tcpdump -i wp1ppp");
        break;

      case WANCONFIG_FR:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "tcpdump -i wp1fr");
        break;

      case WANCONFIG_X25:
        snprintf(tmp_buff, MAX_PATH_LENGTH, "tcpdump -i wp1svc1");
        break;
      }
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
    }

    if(exit_dialog == YES){
      break;
    }
    
    if(run_system_command(tmp_buff)){

      if(atoi(get_lxdialog_output_string()) == TCP_IP_TRACE){
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          "Failed to start TCP\\IP Line Trace.\nPlease make sure 'tcpdump' utility is installed.");
      }else{
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          "Failed to start Line Trace. Please check Message Log.");
      }
    }

    if(repeat_command() == YES){
      goto repeat_trace;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    Debug(DBG_SELECT_TRACE_CMD, ("MENU_BOX_BUTTON_HELP: option selected for editing: %s\n",
      get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    default:
      ;//do nothing
      /*
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

