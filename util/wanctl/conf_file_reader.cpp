/***************************************************************************
                          conf_file_reader.cpp  -  description
                             -------------------
    begin                : Thu Mar 4 2004
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

#include "conf_file_reader.h"

#define DBG_CONF_FILE_READER  1

//////////////////////////////////////////////////////////////////////////////////////////

/****** Global Data *********************************************************/


//////////////////////////////////////////////////////////////////////////////////////////

/****** Global Function Prototypes *************************************************/

//////////////////////////////////////////////////////////////////////////////////////////

conf_file_reader::conf_file_reader()
{
  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::conf_file_reader()\n"));

  snprintf(conf_file, MAX_PATH_LENGTH, "%s%s",
    wanpipe_cfg_dir, "wanpipe1.conf");

  protocol = PROTOCOL_NOT_SET;
  station_type = STATION_TYPE_NOT_KNOWN;
  current_wanrouter_state = WANPIPE_NOT_RUNNING;
  ip_cfg_complete = NO;

  snprintf(if_name, WAN_IFNAME_SZ, IF_NAME_NOT_INIT);
}

conf_file_reader::~conf_file_reader()
{
  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::~conf_file_reader()\n"));
}


int conf_file_reader::read_conf_file()
{
	int err = 0;
  char conf_file_line[MAX_PATH_LENGTH];
  char copy_of_conf_file_line[MAX_PATH_LENGTH];
  FILE* conf_file_ptr;
  char is_found_valid_protocol = 0;
  char is_found_station_type = 0;

  Debug(DBG_CONF_FILE_READER, ("conf_file_reader::read_conf_file()\n"));

  conf_file_ptr = fopen(conf_file, "r+");
  if(conf_file_ptr == NULL){
    //not an error - may be a new installation
    //ERR_DBG_OUT(("Failed to open the file '%s' for reading!\n",
    //  conf_file));
    return FILE_DOES_NOT_EXIST;
  }

  //read each line in the file and try to get PROTOCOL and 'Station Type'
  //or whatever it is called, depending on the Protocol.
  //If failed to find the above info or if Protocol is unsupported
  //return failure.

  //search the Protocol
  while(!feof(conf_file_ptr)){

    fgets(conf_file_line, MAX_PATH_LENGTH, conf_file_ptr);

    //Debug(DBG_CONF_FILE_READER, ("conf_file_line: %s\n", conf_file_line));

    if(get_protocol_from_string(conf_file_line)){
      is_found_valid_protocol = 1;
      break;
    }
  }//while()

  if(is_found_valid_protocol == 0){
    ERR_DBG_OUT(("Failed to find valid 'Protocol' in configuration file!\n"));
    return UNKNOWN_PROTOCOL;
  }

  rewind(conf_file_ptr);
  while(!feof(conf_file_ptr)){

    fgets(conf_file_line, MAX_PATH_LENGTH, conf_file_ptr);
    //work with a copy of the line. because strstr() is changing the buffer.
    memcpy(copy_of_conf_file_line, conf_file_line, MAX_PATH_LENGTH);

    //Debug(DBG_CONF_FILE_READER, ("conf_file_line: %s\n", conf_file_line));

    if(card_type == S5141_ADPTR_1_CPU_SERIAL){
      //search for 'Clocking' to find is it CPE or NODE
      if(strstr(copy_of_conf_file_line, "Clocking") != NULL){

        //restore the search string
        memcpy(copy_of_conf_file_line, conf_file_line, MAX_PATH_LENGTH);

        if(get_serial_clocking_type_from_string(copy_of_conf_file_line)){
          is_found_station_type = 1;
        }
        break;
      }
    }else{
      //search for 'TE_CLOCK' to find is it CPE or NODE
      if(strstr(copy_of_conf_file_line, "TE_CLOCK") != NULL){
        //restore the search string
        memcpy(copy_of_conf_file_line, conf_file_line, MAX_PATH_LENGTH);

        if(get_te1_clocking_type_from_string(copy_of_conf_file_line)){
          is_found_station_type = 1;
        }
        break;
      }
    }
  }//while()

  if(is_found_station_type == 0){
    ERR_DBG_OUT(("Failed to find valid 'Router Location' in configuration file!\n"));
    return UNKNOWN_STATION;
  }

	return err;
}

int conf_file_reader::get_protocol_from_string(char* conf_file_line)
{
  if(strstr(conf_file_line, "WAN_ATM") != NULL){
    protocol = WANCONFIG_ATM;
    return 1;
  }

  if(strstr(conf_file_line, "WAN_CHDLC") != NULL){
    protocol = WANCONFIG_CHDLC;
    return 1;
  }

  if(strstr(conf_file_line, "WAN_FR") != NULL){
    protocol = WANCONFIG_FR;
    return 1;
  }

  if(strstr(conf_file_line, "WAN_PPP") != NULL){
    protocol = WANCONFIG_PPP;
    return 1;
  }

  if(strstr(conf_file_line, "WAN_X25") != NULL){
    protocol = WANCONFIG_X25;
    return 1;
  }

  //none of the valid protocols found in this string
  return 0;
}

/*
//precondition for this function: the protocol should be
//known
int conf_file_reader::get_station_type_from_string(char* conf_file_line)
{
  switch(protocol)
  {
  case WANCONFIG_ATM:
  case WANCONFIG_CHDLC:
  case WANCONFIG_PPP:
    return 1;

  case WANCONFIG_FR:
    if(strstr(conf_file_line, "CPE") != NULL){
      station_type = WANOPT_CPE;
      return 1;
    }

    if(strstr(conf_file_line, "NODE") != NULL){
      station_type = WANOPT_NODE;
      return 1;
    }
    break;

  case WANCONFIG_X25:
    if(strstr(conf_file_line, "DTE") != NULL){
      //station_type = WANOPT_DTE;
      station_type = WANOPT_CPE;
      return 1;
    }

    if(strstr(conf_file_line, "DCE") != NULL){
      //station_type = WANOPT_DCE;
      station_type = WANOPT_NODE;
      return 1;
    }
    break;

  default:
    ERR_DBG_OUT(("Invalid input!\n"));
    return 0;
  }

  //none of the valid Station types found in this string
  return 0;
}
*/

//based on Protocol form the name of the net interface
int conf_file_reader::form_interface_name(unsigned char protocol)
{
  int rc = YES;

  switch(protocol)
  {
  case WANCONFIG_ATM:
    snprintf(if_name, WAN_IFNAME_SZ, "wp1atm1");
    break;

  case WANCONFIG_CHDLC:
    snprintf(if_name, WAN_IFNAME_SZ, "wp1chdlc");
    break;

  case WANCONFIG_PPP:
    snprintf(if_name, WAN_IFNAME_SZ, "wp1ppp");
    break;

  case WANCONFIG_FR:
    snprintf(if_name, WAN_IFNAME_SZ, "wp1fr");
    break;

  case WANCONFIG_X25:
    snprintf(if_name, WAN_IFNAME_SZ, "wp1svc1");
    break;

  default:
    rc = NO;
  }
  return rc;
}

int conf_file_reader::get_serial_clocking_type_from_string(char* line)
{
  str_tolower(line);

  Debug(DBG_CONF_FILE_READER, ("get_serial_clocking_type_from_string(): line: %s\n", line));

  if(strstr(line, "internal")){
    station_type = WANOPT_NODE;
    return 1;
  }

  if(strstr(line, "external")){
    station_type = WANOPT_CPE;
    return 1;
  }

  //if got here it is a failure
  Debug(DBG_CONF_FILE_READER, ("get_serial_clocking_type_from_string() failed!!\n"));
  return 0;
}

int conf_file_reader::get_te1_clocking_type_from_string(char* line)
{
  str_tolower(line);

  Debug(DBG_CONF_FILE_READER, ("get_te1_clocking_type_from_string(): line: %s\n", line));

  if(strstr(line, "master")){
    station_type = WANOPT_NODE;
    return 1;
  }

  if(strstr(line, "normal")){
    station_type = WANOPT_CPE;
    return 1;
  }

  //if got here it is a failure
  Debug(DBG_CONF_FILE_READER, ("get_te1_clocking_type_from_string() failed!!\n"));
  return 0;
}

//****** End *****************************************************************/
