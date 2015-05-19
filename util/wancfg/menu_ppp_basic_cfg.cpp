/***************************************************************************
                          menu_ppp_basic_cfg.cpp  -  description
                             -------------------
    begin                : Tue Apr 6 2004
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

#include "text_box.h"
#include "input_box.h"
#include "objects_list.h"

#include "menu_ppp_basic_cfg.h"
#include "menu_ppp_ip_mode.h"
#include "menu_ppp_select_authentication_protocol.h"

#define MAX_PPP_AUTHENTICATION_STR_LEN  511

char* ppp_ip_mode_help_str =
"PPP IP MODE\n"
"\n"
"Configure PPP IP addressing mode.\n"
"\n"
"Options:\n"
"--------\n"
"\n"
"STATIC:\n"
"No IP addresses are requested or supplied.\n"
"Driver uses IP addresses supplied by the user.\n"
"HOST:\n"
"Provide IP addresses to the remote peer upon\n"
"peer request.\n"
"Note:   This option may cause problems against\n"
"        routers that do not support dynamic ip\n"
"        addressing.\n"
"PEER:\n"
"Request both local and remote IP addresses\n"
"from the remote host. If the remote host does not\n"
"respond, configuration will fail.\n"
"\n"
"Default: STATIC\n";

char* ppp_authentication_protocol_select_help_str =
"This parameter enables or disables the\n"
"use of PAP or CHAP\n";

char* user_id_help_str =
"PAP/CHAP: USERID\n"
"\n"
"This parameter is dependent on the AUTHENTICATOR\n"
"parameter.  If AUTHENTICATOR is set to NO then\n"
"you will simply enter in your login name that the\n"
"other side specified to you.\n"
"\n"
"If AUTHENTICATOR is set to YES then you will have to\n"
"maintain a list of all the users that are valid for\n"
"authentication purposes. If your list contains ONLY\n"
"ONE MEMBER then simply enter in the login name.  If\n"
"the list contains more than one member then follow\n"
"the below format:\n"
"  USERID = LOGIN1 / LOGIN2 / LOGIN3....so on\n"
"\n"
"The '/' separators are VERY IMPORTANT if you have\n"
"more than one member to support.\n";

char* passwd_help_str =
"PAP/CHAP: PASSWD\n"
"\n"
"This parameter is dependent on the AUTHENTICATOR\n"
"parameter.  If AUTHENTICATOR is set to NO then you\n"
"will simply enter in your password for the login\n"
"name that the other side specified to you.\n"
"\n"
"If AUTHENTICATOR is set to YES then you will\n"
"have to maintain a list of all the passwords for all the\n"
"users that are valid for authentication purposes.  If\n"
"your list contains ONLY ONE MEMBER then simply enter\n"
"in the password for the corresponding login name in\n"
"the USERID parameter.  If the list contains more than\n"
"one member then follow the format below:\n"
"\n"
"       PASSWD = PASS1 / PASS2 / PASS3....so on\n"
"\n"
"The '/' separators are VERY IMPORTANT if you have more\n"
"than one member to support.  The ORDER of your passwords\n"
"is very important.  They correspond to the order of\n"
"the userids.\n";

char* sysname_help_str =
"PAP/CHAP: SYSNAME\n"
"\n"
"This parameter is dependent on the AUTHENTICATOR\n"
"parameter.  If AUTHENTICATOR is set to NO then you can\n"
"simply ignore this parameter. (comment it )\n"
"\n"
"If AUTHENTICATOR is set to YES then you have to enter\n"
"Challenge system name which can be no longer than 31\n"
"characters.\n";


enum PPP_BASIC_CFG_OPTION{

  //Static IP addressing,  STATIC(default)
  //Host IP addressing,    HOST
  //Dynamic IP addressing, PEER
  IP_MODE,

  //Authentication Protocol-> Disabled, PAP, CHAP
  AUTHENTICATION_PROTOCOL,
  //if Authentication Enabled:
  USERID,
  PASSWD,
  SYSNAME
};

#define DBG_MENU_PPP_BASIC_CFG  1

menu_ppp_basic_cfg::menu_ppp_basic_cfg(	IN char * lxdialog_path,
					IN list_element_chan_def* parent_element_logical_ch)
{
  Debug(DBG_MENU_PPP_BASIC_CFG, ("menu_ppp_basic_cfg::menu_ppp_basic_cfg()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  
  this->parent_list_element_logical_ch = parent_element_logical_ch;
  name_of_parent_layer = parent_element_logical_ch->data.name;
}

menu_ppp_basic_cfg::~menu_ppp_basic_cfg()
{
  Debug(DBG_MENU_PPP_BASIC_CFG,
    ("menu_ppp_basic_cfg::~menu_ppp_basic_cfg()\n"));
}

int menu_ppp_basic_cfg::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog = NO;
  char authentication;
  int number_of_items;
  char* authentication_string_validation_result;

  //help text box
  text_box tb;

  //input box for : 1. User ID 2. Password 3. System ID
  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility: ");
  snprintf(&backtitle[strlen(backtitle)], MAX_PATH_LENGTH,
          "%s", get_protocol_string(WANCONFIG_PPP));

  list_element_chan_def* list_el_chan_def = NULL;
  objects_list * obj_list = NULL;

  conf_file_reader* local_cfr = (conf_file_reader*)global_conf_file_reader_ptr;
  
  Debug(DBG_MENU_PPP_BASIC_CFG, ("menu_ppp_basic_cfg::run()\n"));

again:
  rc = YES;
  menu_str = " ";
   
  if(local_cfr->link_defs->linkconf->card_type != WANOPT_ADSL){
	  
    obj_list = (objects_list*)parent_list_element_logical_ch->next_objects_list;
    if(obj_list == NULL){
      ERR_DBG_OUT(("Invalid 'obj_list' pointer!!\n"));
      return NO;
    }
  
    list_el_chan_def = (list_element_chan_def*)obj_list->get_first();
    if(list_el_chan_def == NULL){
      ERR_DBG_OUT(("Invalid 'ppp interface' pointer!!\n"));
      return NO;
    }
  }else{
    list_el_chan_def = parent_list_element_logical_ch;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  if(local_cfr->link_defs->linkconf->card_type != WANOPT_ADSL){
	  
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", IP_MODE);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"IP Mode-----------------> %s\" ",
      (list_el_chan_def->data.chanconf->u.ppp.dynamic_ip == WANOPT_PPP_STATIC ?
        "Static (default)" :
        (list_el_chan_def->data.chanconf->u.ppp.dynamic_ip == WANOPT_PPP_HOST ? "Host" : "Peer")));
    menu_str += tmp_buff;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  if(list_el_chan_def->data.chanconf->u.ppp.pap == 1 || list_el_chan_def->data.chanconf->u.ppp.chap == 1){
    authentication = YES;
    number_of_items = 5;
  }else{
    authentication = NO;
    number_of_items = 2;
  }
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AUTHENTICATION_PROTOCOL);
  menu_str += tmp_buff;

  if(list_el_chan_def->data.chanconf->u.ppp.pap == 1){
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Authentication Protocol-> %s\" ", "PAP");
  }else if(list_el_chan_def->data.chanconf->u.ppp.chap == 1){
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Authentication Protocol-> %s\" ", "CHAP");
  }else{
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Authentication Protocol-> %s\" ", "Disabled (default)");
  }
  menu_str += tmp_buff;

  //////////////////////////////////////////////////////////////////////////////////////
  if(authentication == YES){
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", USERID);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"User ID-----------------> %s\" ",
      list_el_chan_def->data.chanconf->u.ppp.userid);
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", PASSWD);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Password----------------> %s\" ",
      list_el_chan_def->data.chanconf->u.ppp.passwd);
    menu_str += tmp_buff;

    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", SYSNAME);
    menu_str += tmp_buff;
    snprintf(tmp_buff, MAX_PATH_LENGTH, " \"System Name-------------> %s\" ",
      list_el_chan_def->data.chanconf->u.ppp.sysname);
    menu_str += tmp_buff;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nPPP configuration options for\
\nWan Device: %s", name_of_parent_layer);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "PPP CONFIGURATION",
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
    Debug(DBG_MENU_PPP_BASIC_CFG,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {

    case IP_MODE:
      {
	menu_ppp_ip_mode ppp_ip_mode(lxdialog_path);
        if(ppp_ip_mode.run(&list_el_chan_def->data.chanconf->u.ppp.dynamic_ip) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case AUTHENTICATION_PROTOCOL:
      {
        menu_ppp_select_authentication_protocol
          ppp_select_authentication_protocol(lxdialog_path, list_el_chan_def);
        if(ppp_select_authentication_protocol.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;

    case USERID:

show_user_id_input_box:

      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify your User Id");
      snprintf(initial_text, MAX_PATH_LENGTH, "%s",
        replace_new_line_with_zero_term((char*)list_el_chan_def->data.chanconf->u.ppp.userid));

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
        Debug(DBG_MENU_PPP_BASIC_CFG,
          ("User id on return: %s\n", inb.get_lxdialog_output_string()));

        authentication_string_validation_result =
          validate_authentication_string( inb.get_lxdialog_output_string(),
                                          MAX_PPP_AUTHENTICATION_STR_LEN);
        if(authentication_string_validation_result != NULL){
          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
                                authentication_string_validation_result);
          goto show_user_id_input_box;
        }
        snprintf( (char*)list_el_chan_def->data.chanconf->u.ppp.userid,
                  MAX_PPP_AUTHENTICATION_STR_LEN,
                  inb.get_lxdialog_output_string());
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          option_not_implemented_yet_help_str);
        goto show_user_id_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case PASSWD:

show_password_input_box:

      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify your Password");
      snprintf(initial_text, MAX_PATH_LENGTH, "%s",
        replace_new_line_with_zero_term((char*)list_el_chan_def->data.chanconf->u.ppp.passwd));

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
        Debug(DBG_MENU_PPP_BASIC_CFG,
          ("password on return: %s\n", inb.get_lxdialog_output_string()));

        authentication_string_validation_result =
          validate_authentication_string( inb.get_lxdialog_output_string(),
                                          MAX_PPP_AUTHENTICATION_STR_LEN);
        if(authentication_string_validation_result != NULL){
          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
                                authentication_string_validation_result);
          goto show_password_input_box;
        }

        snprintf( (char*)list_el_chan_def->data.chanconf->u.ppp.passwd,
                  MAX_PPP_AUTHENTICATION_STR_LEN,
                  inb.get_lxdialog_output_string());
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          option_not_implemented_yet_help_str);
        goto show_password_input_box;
        break;
      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;

    case SYSNAME:
show_sysname_input_box:

      snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify your System Name");
      snprintf(initial_text, MAX_PATH_LENGTH, "%s",
        replace_new_line_with_zero_term((char*)list_el_chan_def->data.chanconf->u.ppp.sysname));

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
        Debug(DBG_MENU_PPP_BASIC_CFG,
          ("sysname on return: %s\n", inb.get_lxdialog_output_string()));

        authentication_string_validation_result =
          validate_authentication_string( inb.get_lxdialog_output_string(),
                                          MAX_PPP_AUTHENTICATION_STR_LEN);
        if(authentication_string_validation_result != NULL){
          tb.show_error_message(lxdialog_path, NO_PROTOCOL_NEEDED,
                                authentication_string_validation_result);
          goto show_sysname_input_box;
        }

        snprintf( (char*)list_el_chan_def->data.chanconf->u.ppp.sysname,
                  MAX_PPP_AUTHENTICATION_STR_LEN,
                  inb.get_lxdialog_output_string());
        break;

      case INPUT_BOX_BUTTON_HELP:
        tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
          option_not_implemented_yet_help_str);
        goto show_sysname_input_box;

      }//switch(*selection_index)
      /////////////////////////////////////////////////////////////
      break;
      
    default:
      ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
        get_lxdialog_output_string()));
      rc = NO;
    }
    break;

  case MENU_BOX_BUTTON_HELP:

    switch(atoi(get_lxdialog_output_string()))
    {
    case IP_MODE:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, ppp_ip_mode_help_str);
      break;

    case AUTHENTICATION_PROTOCOL:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
        ppp_authentication_protocol_select_help_str);
      break;

    case USERID:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, user_id_help_str);
      break;

    case PASSWD:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, passwd_help_str);
      break;

    case SYSNAME:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, sysname_help_str);
      break;
    }//switch()
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

