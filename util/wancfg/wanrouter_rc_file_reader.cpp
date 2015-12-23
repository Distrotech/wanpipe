/***************************************************************************
                          wanrouter_rc_file_reader.cpp  -  description
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

#include "wanrouter_rc_file_reader.h"
#include "string.h" //for memset()

#define DBG_WANROUTER_RC_FILE_READER 1

wanrouter_rc_file_reader::wanrouter_rc_file_reader(int wanpipe_number)
{
  Debug(DBG_WANROUTER_RC_FILE_READER,
    ("wanrouter_rc_file_reader::wanrouter_rc_file_reader()\n"));
   
  wanpipe_name.sprintf("wanpipe%d", wanpipe_number);

  full_file_path = wanpipe_cfg_dir;
  full_file_path += "wanrouter.rc";

  Debug(DBG_WANROUTER_RC_FILE_READER, ("full_file_path: %s\n", (char*)full_file_path.c_str()));
}

wanrouter_rc_file_reader::~wanrouter_rc_file_reader()
{
  Debug(DBG_WANROUTER_RC_FILE_READER,
    ("wanrouter_rc_file_reader::~wanrouter_rc_file_reader()\n"));
}

//returns:  YES - device name found
//          NO  - device name not found OR failed to parse the file
int wanrouter_rc_file_reader::search_and_update_boot_start_device_setting()
{
  int rc = YES;
  FILE* wanrouter_rc_file = NULL;
  char tmp_read_buffer[MAX_PATH_LENGTH];
  char tmp_read_buffer1[MAX_PATH_LENGTH];
  char tmp_read_buffer2[MAX_PATH_LENGTH];
  char *tmp, *tmp1=NULL;
  string str_tmp1;

  updated_wandevices_str = "";
  updated_file_content = "";
	  
  if(check_file_exist((char*)full_file_path.c_str(), &wanrouter_rc_file) == NO){
    Debug(DBG_WANROUTER_RC_FILE_READER, ("The 'Net interface' file '%s' does not exist!\n",
      (char*)full_file_path.c_str()));
    return NO;
  }

  wanrouter_rc_file = fopen((char*)full_file_path.c_str(), "r+");
  if(wanrouter_rc_file == NULL){
    ERR_DBG_OUT(("Failed to open the 'Net interface' file '%s' for reading!\n",
      (char*)full_file_path.c_str()));
    return NO;
  }

  do{
    fgets(tmp_read_buffer, MAX_PATH_LENGTH, wanrouter_rc_file);

    if(!feof(wanrouter_rc_file)){
      //must have an original copy, because strstr() changing the original buffer!!!
      memcpy(tmp_read_buffer1, tmp_read_buffer, MAX_PATH_LENGTH);

      if(tmp_read_buffer[0] == '#'){
        updated_file_content += tmp_read_buffer1;
	continue;
      }

      tmp = NULL;
      tmp = strstr(tmp_read_buffer, "WAN_DEVICES=");
      if(tmp == NULL){
        updated_file_content += tmp_read_buffer1;
	continue;
      }
        
      tmp += strlen("WAN_DEVICES=");//skip past WAN_DEVICES=
      tmp1 = strstr(tmp, wanpipe_name.c_str());
      if(tmp1 != NULL){
        //in the list already
        updated_file_content += tmp_read_buffer1;
        continue;
      }

      //not in the list
      //
      //get rid of the closing <"> quotaion mark
      memcpy(tmp_read_buffer2, tmp, strlen(tmp) - 2);
      tmp_read_buffer2[strlen(tmp) - 2] = '\0';//terminate the string
      
      str_tmp1 = tmp_read_buffer2;
      
      //append the new wanpipe name
      str_tmp1 += " ";
      str_tmp1 += wanpipe_name;
      str_tmp1 += "\"";

      //prepend "WAN_DEVICES="
      wanpipe_name = str_tmp1;
      str_tmp1 = "WAN_DEVICES=";

      updated_wandevices_str = (str_tmp1 += wanpipe_name);
      updated_wandevices_str += "\n";
	     
      Debug(DBG_WANROUTER_RC_FILE_READER, ("tmp: %s\n", tmp));
      Debug(DBG_WANROUTER_RC_FILE_READER, ("tmp_read_buffer2: %s\n", tmp_read_buffer2));
      Debug(DBG_WANROUTER_RC_FILE_READER, ("updated_wandevices_str: %s\n", updated_wandevices_str.c_str()));

      updated_file_content += updated_wandevices_str;
      
    }//if()
    
  }while(!feof(wanrouter_rc_file));

  fclose(wanrouter_rc_file);

  Debug(DBG_WANROUTER_RC_FILE_READER, ("updated_file_content: %s\n", updated_file_content.c_str()));

  if(tmp1 != NULL){
    rc = YES;
   }else{
    rc = NO;
  }
  return rc;
}

//write the updated string (containing the whole file) to the file
int wanrouter_rc_file_reader::update_wanrouter_rc_file()
{
  return write_string_to_file((char*)full_file_path.c_str(),
		 (char*)updated_file_content.c_str());
}

//returns:  YES - setting was found and read
//          NO  - setting was not found or failed to read it's value
int wanrouter_rc_file_reader::get_setting_value(IN char* setting_name, 
						OUT char* setting_value_buff,
						int setting_value_buff_len)
{
  int rc = NO;
  FILE* wanrouter_rc_file = NULL;
  char tmp_read_buffer[MAX_PATH_LENGTH];
  char *tmp;
    
  if(check_file_exist((char*)full_file_path.c_str(), &wanrouter_rc_file) == NO){
    ERR_DBG_OUT(("The '%s' file does not exist!\n",
	(char*)full_file_path.c_str()));
    return NO;
  }

  wanrouter_rc_file = fopen((char*)full_file_path.c_str(), "r+");
  if(wanrouter_rc_file == NULL){
    ERR_DBG_OUT(("Failed to open '%s' file for reading!\n",
	(char*)full_file_path.c_str()));
    return NO;
  }

  do{
    fgets(tmp_read_buffer, MAX_PATH_LENGTH, wanrouter_rc_file);

    if(!feof(wanrouter_rc_file)){

      if(tmp_read_buffer[0] == '#'){
	continue;
      }

      ///////////////////////////////////////////////////////////////////////////////
      //find the setting_name
      tmp = strstr(tmp_read_buffer, setting_name); //WAN_BIN_DIR
      if(tmp != NULL){
        Debug(DBG_WANROUTER_RC_FILE_READER, ("1. tmp: %s\n", tmp));
	tmp += strlen(setting_name);
        Debug(DBG_WANROUTER_RC_FILE_READER, ("2. tmp: %s\n", tmp));
        snprintf(setting_value_buff, setting_value_buff_len, "%s", tmp);
	rc = YES;
	break;
      }else{
        continue;
      }

    }//if()
    
  }while(!feof(wanrouter_rc_file));

  fclose(wanrouter_rc_file);

  return rc;
}

