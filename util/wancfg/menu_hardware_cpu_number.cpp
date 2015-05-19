/***************************************************************************
                          menu_hardware_cpu_number.cpp  -  description
                             -------------------
    begin                : Mon Apr 26 2004
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

#include "menu_hardware_cpu_number.h"
#include "text_box_help.h"

char* cpu_number_help_str =
"S514 or AFT card CPU\n"
"\n"
"The S514 and AFT PCI cards may contain up to two CPUs.\n"
"Each cpu can run a different protocol. Thus its like\n"
"having two cards on one piece of plastic.\n";

enum CPU_NUMBER{
  CPU_A,
  CPU_B
};

#define DBG_MENU_HARDWARE_CPU_NUMBER 1

menu_hardware_cpu_number::menu_hardware_cpu_number( IN char * lxdialog_path,
                                                    IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_HARDWARE_CPU_NUMBER, ("menu_hardware_cpu_number::menu_hardware_cpu_number()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_hardware_cpu_number::~menu_hardware_cpu_number()
{
  Debug(DBG_MENU_HARDWARE_CPU_NUMBER, ("menu_hardware_cpu_number::~menu_hardware_cpu_number()\n"));
}

int menu_hardware_cpu_number::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  int number_of_items;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;

  Debug(DBG_MENU_HARDWARE_CPU_NUMBER, ("menu_net_interface_setup::run()\n"));

again:
  number_of_items = 2;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_HARDWARE_CPU_NUMBER, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;

  menu_str = " ";

  Debug(DBG_MENU_HARDWARE_CPU_NUMBER, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CPU_A);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"CPU A\" ");
  menu_str += tmp_buff;

  /////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", CPU_B);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"CPU B\" ");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect CPU for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT CPU ON THE CARD",
                          WANCFG_PROGRAM_NAME,
                          tmp_buff,
                          MENU_HEIGTH, MENU_WIDTH,
                          number_of_items,
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
    Debug(DBG_MENU_HARDWARE_CPU_NUMBER,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case CPU_A:
      linkconf->S514_CPU_no[0] = 'A';
      exit_dialog = YES;
      break;

    case CPU_B:
      linkconf->S514_CPU_no[0] = 'B';
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
    case CPU_A:
    case CPU_B:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, cpu_number_help_str);
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
