/***************************************************************************
                          net_interface_file_reader.cpp  -  description
                             -------------------
    begin                : Wed Mar 24 2004
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

#include "net_interface_file_reader.h"
#include "string.h" //for memset()

#define DBG_NET_INTERFACE_FILE_READER 1

net_interface_file_reader::net_interface_file_reader(char* interface_name)
{
  Debug(DBG_NET_INTERFACE_FILE_READER,
    ("net_interface_file_reader::net_interface_file_reader()\n"));

  this->interface_name = interface_name;

  memset(&if_config, 0x00, sizeof(if_config_t));
  if_config.ipaddr[0] = '\0';
  if_config.netmask[0] = '\0';
  if_config.point_to_point_ipaddr[0] = '\0';
  if_config.on_boot = 1;
  if_config.gateway[0] = '\0';

  //initialize
  full_file_path = interfaces_cfg_dir;
  full_file_path += interface_name;

  Debug(DBG_NET_INTERFACE_FILE_READER, ("full_file_path: %s\n", (char*)full_file_path.c_str()));
}

net_interface_file_reader::~net_interface_file_reader()
{
  Debug(DBG_NET_INTERFACE_FILE_READER,
    ("net_interface_file_reader::~net_interface_file_reader()\n"));
}

//returns:  YES - file found, and parsed successfully
//          NO  - file not found or failed to parse the file
int net_interface_file_reader::parse_net_interface_file()
{
  int rc = YES;
  FILE* net_interface_file = NULL;
  char tmp_read_buffer[IF_CONFIG_BUF_LEN];
  char* tmp;

  if(check_file_exist((char*)full_file_path.c_str(), &net_interface_file) == NO){
    Debug(DBG_NET_INTERFACE_FILE_READER, ("The 'Net interface' file '%s' does not exist!\n",
      (char*)full_file_path.c_str()));

    //file was not found - most likely it is a new interface, so create the file.
    if(create_net_interface_file(&if_config) == NO){
      return NO;
    }
  }

  net_interface_file = fopen((char*)full_file_path.c_str(), "r+");
  if(net_interface_file == NULL){
    ERR_DBG_OUT(("Failed to open the 'Net interface' file '%s' for reading!\n",
      (char*)full_file_path.c_str()));
    return NO;
  }

  do{
    fgets(tmp_read_buffer, IF_CONFIG_BUF_LEN, net_interface_file);

    if(!feof(net_interface_file)){
      //Debug(DBG_NET_INTERFACE_FILE_READER, (tmp_read_buffer));

      tmp = strstr(tmp_read_buffer, "DEVICE=");
      if(tmp != NULL){
        snprintf(if_config.device, WAN_IFNAME_SZ, tmp += strlen("DEVICE="));
        Debug(DBG_NET_INTERFACE_FILE_READER, ("if_config.device: %s\n", if_config.device));
      }

      tmp = strstr(tmp_read_buffer, "IPADDR=");
      if(tmp != NULL){
        snprintf(if_config.ipaddr, IF_CONFIG_BUF_LEN, tmp += strlen("IPADDR="));
        Debug(DBG_NET_INTERFACE_FILE_READER, ("if_config.ipaddr: %s\n", if_config.ipaddr));
      }

      tmp = strstr(tmp_read_buffer, "NETMASK=");
      if(tmp != NULL){
        snprintf(if_config.netmask, IF_CONFIG_BUF_LEN, tmp += strlen("NETMASK="));
        Debug(DBG_NET_INTERFACE_FILE_READER, ("if_config.netmask: %s\n", if_config.netmask));
      }

      tmp = strstr(tmp_read_buffer, "POINTOPOINT=");
      if(tmp != NULL){
        snprintf(if_config.point_to_point_ipaddr, IF_CONFIG_BUF_LEN, tmp += strlen("POINTOPOINT="));
        Debug(DBG_NET_INTERFACE_FILE_READER, ("if_config.point_to_point_ipaddr: %s\n",
          if_config.point_to_point_ipaddr));
      }

      tmp = strstr(tmp_read_buffer, "ONBOOT=");
      if(tmp != NULL){

        tmp += strlen("ONBOOT=");
        str_tolower(tmp);

        if(strstr(tmp, "yes") != NULL){
          if_config.on_boot = 1;
        }else{
          if_config.on_boot = 0;
        }
        Debug(DBG_NET_INTERFACE_FILE_READER, ("if_config.on_boot: %s\n",
          (if_config.on_boot == 1 ? "YES" : "NO")));
      }

      tmp = strstr(tmp_read_buffer, "GATEWAY=");
      if(tmp != NULL){
        snprintf(if_config.gateway, IF_CONFIG_BUF_LEN, tmp += strlen("GATEWAY="));
        Debug(DBG_NET_INTERFACE_FILE_READER, ("if_config.gateway: %s\n",
          if_config.gateway));
      }

    }//if()
  }while(!feof(net_interface_file));

  //Debug(DBG_NET_INTERFACE_FILE_READER, ("closing 'net_interface_file'\n"));

  fclose(net_interface_file);

  //Debug(DBG_NET_INTERFACE_FILE_READER, ("closed 'net_interface_file'. rc: %d\n", rc));

  return rc;
}


int net_interface_file_reader::create_net_interface_file(if_config_t* if_config)
{
  char temp_str[MAX_PATH_LENGTH];
  string net_interface_file_str;

  ///////////////////////////////////////////////////
  snprintf(temp_str, MAX_PATH_LENGTH, "\
# Wanrouter interface configuration file\n\
# name:	%s\n\
# date:	%s\
#\n", interface_name, get_date_and_time());

  net_interface_file_str = temp_str;

  ///////////////////////////////////////////////////
  snprintf(temp_str, MAX_PATH_LENGTH, "DEVICE=%s\n", interface_name);
  net_interface_file_str += temp_str;

  //////////////////////////////////////////////////
  net_interface_file_str += "IPADDR=";
  net_interface_file_str += if_config->ipaddr;
  net_interface_file_str += "\n";

  //////////////////////////////////////////////////
  net_interface_file_str += "NETMASK=";
  net_interface_file_str +=
    (if_config->netmask[0] == '\0' ? "255.255.255.255" :  if_config->netmask);
  net_interface_file_str += "\n";

  //////////////////////////////////////////////////
  net_interface_file_str += "POINTOPOINT=";
  net_interface_file_str += if_config->point_to_point_ipaddr;
  net_interface_file_str += "\n";

  //////////////////////////////////////////////////
  net_interface_file_str += "ONBOOT=";
  net_interface_file_str += (if_config->on_boot == 1 ? "yes" : "no");
  net_interface_file_str += "\n";

  /////////////////////////////////////////////////
  if(if_config->gateway[0] != '\0'){
  	net_interface_file_str += "GATEWAY=";
  	net_interface_file_str += if_config->gateway;
  	net_interface_file_str += "\n";
  }
  return write_string_to_file((char*)full_file_path.c_str(),
                              (char*)net_interface_file_str.c_str());
}
