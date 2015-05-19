/***************************************************************************
                          menu_select_card_type_manualy.cpp  -  description
                             -------------------
    begin                : Wed Mar 31 2004
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

#include "menu_select_card_type_manualy.h"
#include "text_box_help.h"

char* select_card_type_manualy_help_str =
"ADAPTER TYPE\n"
"\n"
"Adapter type for Sangoma wanpipe.\n"
"\n"
"Please make sure selected card type corresponds\n"
"to the actual card installed on your system.\n";

#define DBG_MENU_SELECT_CARD_TYPE_MANUALY 1

menu_select_card_type_manualy::
	menu_select_card_type_manualy(  IN char * lxdialog_path,
                                  IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_SELECT_CARD_TYPE_MANUALY,
    ("menu_select_card_type_manualy::menu_select_card_type_manualy()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_select_card_type_manualy::~menu_select_card_type_manualy()
{
  Debug(DBG_MENU_SELECT_CARD_TYPE_MANUALY,
    ("menu_select_card_type_manualy::~menu_select_card_type_manualy()\n"));
}

int menu_select_card_type_manualy::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int option_selected;
  char exit_dialog;
  int number_of_items;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;

  Debug(DBG_MENU_SELECT_CARD_TYPE_MANUALY, ("menu_select_card_type_manualy::run()\n"));

again:
  rc = YES;
  option_selected = 0;
  exit_dialog = NO;
  menu_str = " ";
  number_of_items = 9;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_SELECT_CARD_TYPE_MANUALY, ("cfr->link_defs->name: %s\n", link_def->name));

  //////////////////////////////////////////////////////////////////////////
  //
  // S514 Cards
  //
  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S5141_ADPTR_1_CPU_SERIAL);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ",
    get_S514_card_version_string(S5141_ADPTR_1_CPU_SERIAL));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S5143_ADPTR_1_CPU_FT1);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ",
    get_S514_card_version_string(S5143_ADPTR_1_CPU_FT1));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S5144_ADPTR_1_CPU_T1E1);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ",
    get_S514_card_version_string(S5144_ADPTR_1_CPU_T1E1));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S5145_ADPTR_1_CPU_56K);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ",
    get_S514_card_version_string(S5145_ADPTR_1_CPU_56K));
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////
  //
  // ADSL Card
  //
  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", S518_ADPTR_1_CPU_ADSL);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", "S518 - ADSL");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////
  //
  // A101 Card
  //
  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", A101_ADPTR_1TE1);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", "A101/2 - T1/E1");
  menu_str += tmp_buff;
  
  //////////////////////////////////////////////////////////////////////////
  //
  // A104 Card - different from A101/2!!!
  //
  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", A104_ADPTR_4TE1);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", "A104 - Quad T1/E1");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////
  //
  // A108 Card - the same as A104
  //
  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", A104_ADPTR_4TE1);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", "A108 - 8 T1/E1 Ports");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////
  //
  // A200 - Analog Card
  //
  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", A200_ADPTR_ANALOG);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", "A200 - Analog");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////
  //
  // A056 - 56k DDS Card
  //
  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_ADPTR_56K);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", "A056 - 56k DDS");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////
  //
  // A500 - ISDN BRI Card
  //
  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_ADPTR_ISDN);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", "A500 - ISDN BRI");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////
  //
  // A14X - AFT Serial Card
  //
  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_ADPTR_2SERIAL_V35X21);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", "A14X - Serial");
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////
  //
  // A300 Card
  //
  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", A300_ADPTR_U_1TE3);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", "A301 - T3/E3");
  menu_str += tmp_buff;

/*
  ////////////////////////////
  //FIXIT:for debugging only
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", NOT_SET);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", "Card Type NOT_SET");
  menu_str += tmp_buff;
*/
  //////////////////////////////////////////////////////////////////////////
  //
  // S508 ISA Card
  //
  //////////////////////////////////////////////////////////////////////////
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", SDLA_S508);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", "S508 (ISA) Card");
  menu_str += tmp_buff;
	  
  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect card type for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "MANUALY SELECT CARD TYPE",
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
    Debug(DBG_MENU_SELECT_CARD_TYPE_MANUALY,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case S5141_ADPTR_1_CPU_SERIAL:
      linkconf->card_type = WANOPT_S51X;
      link_def->card_version = S5141_ADPTR_1_CPU_SERIAL;
      break;

    case S5143_ADPTR_1_CPU_FT1:
      linkconf->card_type = WANOPT_S51X;
      link_def->card_version = S5143_ADPTR_1_CPU_FT1;
      break;

    case S5144_ADPTR_1_CPU_T1E1:
      linkconf->card_type = WANOPT_S51X;
      link_def->card_version = S5144_ADPTR_1_CPU_T1E1;
      break;

    case S5145_ADPTR_1_CPU_56K:
      linkconf->card_type = WANOPT_S51X;
      link_def->card_version = S5145_ADPTR_1_CPU_56K;
      break;

    case S518_ADPTR_1_CPU_ADSL:
      linkconf->card_type = WANOPT_ADSL;
      link_def->card_version = NOT_SET;
      break;

    case A101_ADPTR_1TE1:
      linkconf->card_type = WANOPT_AFT;
      link_def->card_version = A101_ADPTR_1TE1;
      break;
      
    case A104_ADPTR_4TE1:
      linkconf->card_type = WANOPT_AFT;
      link_def->card_version = A104_ADPTR_4TE1;
      break;

    case AFT_ADPTR_56K:
      linkconf->card_type = WANOPT_AFT;
      link_def->card_version = AFT_ADPTR_56K;
      break;

    case A200_ADPTR_ANALOG:
      linkconf->card_type = WANOPT_AFT;
      link_def->card_version = A200_ADPTR_ANALOG;
      global_card_type = WANOPT_AFT;
      global_card_version = A200_ADPTR_ANALOG;
      break;

    case AFT_ADPTR_ISDN:
      linkconf->card_type = WANOPT_AFT;
      link_def->card_version = AFT_ADPTR_ISDN;
      global_card_type = WANOPT_AFT;
      global_card_version = AFT_ADPTR_ISDN;
      break;
		
		case AFT_ADPTR_2SERIAL_V35X21:
      linkconf->card_type = WANOPT_AFT;
      link_def->card_version = AFT_ADPTR_2SERIAL_V35X21;
      global_card_type = WANOPT_AFT;
      global_card_version = AFT_ADPTR_2SERIAL_V35X21;
			break;

    case A300_ADPTR_U_1TE3:
      linkconf->card_type = WANOPT_AFT;
      link_def->card_version = A300_ADPTR_U_1TE3;
      break;
/*
    //FIXME: work in progress on channelized T3
    case A102_ADPTR_1_CHN_T3E3:
      linkconf->card_type = WANOPT_AFT;
      break;
*/
    case SDLA_S508:
      linkconf->card_type = WANOPT_S50X;
      link_def->card_version = NOT_SET;
      break;

    //FIXIT:for debugging only
    case NOT_SET:
      linkconf->card_type = NOT_SET;
      link_def->card_version = NOT_SET;
      break;

    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
      exit_dialog = YES;
      goto cleanup;
    }
    rc = YES;
    exit_dialog = YES;
    break;

  case MENU_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, select_card_type_manualy_help_str);
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
