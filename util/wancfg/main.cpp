/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin									: Wed Feb 25 16:17:53 EST 2004
    author								: David Rokhvarg
    email									: davidr@sangoma.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DBG_WANCFG_MAIN 1

#include "wancfg.h"

#include "text_box.h"
#include "text_box_yes_no.h"
#include "message_box.h"

#include "conf_file_reader.h"
#include "list_element_chan_def.h"
#include "list_element_frame_relay_dlci.h"
#include "list_element_ppp_interface.h"
#include "list_element_chdlc_interface.h"
#include "list_element_tty_interface.h"
#include "list_element_lapb_interface.h"

#if defined(CONFIG_PRODUCT_WANPIPE_LIP_ATM)
#include "list_element_atm_interface.h"
#endif

#include "objects_list.h"
#include "menu_main_configuration_options.h"
#include "wanrouter_rc_file_reader.h"

#if defined(ZAPTEL_PARSER)
#include "zaptel_conf_file_reader.h"
#include "menu_hardware_probe.h"
#include "sangoma_card_list.h"
int zaptel_to_wanpipe();
#endif

//globals
char lxdialog_path[MAX_PATH_LENGTH];// usually /usr/sbin/wanpipe_lxdialog
char wan_bin_dir[MAX_PATH_LENGTH];

#if defined(__LINUX__)
const char * wanpipe_cfg_dir = "/etc/wanpipe/";
char * interfaces_cfg_dir = "/etc/wanpipe/interfaces/";
#else
const char * wanpipe_cfg_dir = "/usr/local/etc/wanpipe/";
char * interfaces_cfg_dir = "/usr/local/etc/wanpipe/interfaces/";
#endif
char * start_stop_scripts_dir = "scripts";

char * date_and_time_file_name = "/tmp/date_and_time_file.tmp";

int global_card_type = 0;
int global_card_version = 0;

//It is assumed the card HAS HWEC, the only time when
//it is know for sure, is when "hwprobe" is used to detect
//the card. If no HWEC detected, 'global_hw_ec_max_num' will
//be set to zero.
//The reson for this is: there is no way to know if card has
//HWEC when the 'conf' file is edited.
int global_hw_ec_max_num = 1;

char zaptel_conf_path[MAX_PATH_LENGTH];// usually /etc/
#if defined(__LINUX__)
char *zaptel_default_conf_path = "/usr/local/etc/";
#else
char *zaptel_default_conf_path = "/etc/";
#endif
////////////////////////////////////////////////////////////
//help messages
//indicates error.
char* invalid_help_str =
"Invalid Selection!!!\n"
"--------------------\n";

char* option_not_implemented_yet_help_str =
"Selected option not implemented yet.\n";

char* no_configuration_onptions_available_help_str =
"No configuration options available.\n";

//Frame Relay
char* fr_cir_help_str =
"  Frame Relay: Committed Information Rate (CIR)\n"
"  ---------------------------------------------\n"
"  \n"
"  Enable or Disable Committed Information Rate\n"
"  on the board.\n"
"  Valid values: 1 - 512 kbps.\n"
"  \n"
"  Default: Disabled";

////////////////////////////////////////////////
//globals from wanconfig.c
char prognamed[20] =	"wancfg";//"wanconfig";
char progname_sp[] = 	"         ";
#if defined(__LINUX__)
char def_conf_file[] =	"/etc/wanpipe/wanpipe1.conf";	/* default name */
char def_adsl_file[] =	"/etc/wanpipe/wan_adsl.list";	/* default name */
char tmp_adsl_file[] =	"/etc/wanpipe/wan_adsl.tmp";	/* default name */
char router_dir[] =	"/proc/net/wanrouter";	/* location of WAN devices */
#else
char def_conf_file[] =	"/usr/local/etc/wanpipe/wanpipe1.conf";	/* default name */
char def_adsl_file[] =	"/usr/local/etc/wanpipe/wan_adsl.list";	/* default name */
char tmp_adsl_file[] =	"/usr/local/etc/wanpipe/wan_adsl.tmp";	/* default name */
char router_dir[] =	"/var/lock/wanrouter";	/* location of WAN devices */
#endif

char banner[] =		"WAN Router Configurator"
			"(c) 1995-2003 Sangoma Technologies Inc.";

char *krnl_log_file = "/var/log/messages";
char *verbose_log = "/var/log/wanrouter";

char is_there_a_voice_if=NO;
char is_there_a_lip_atm_if=NO;

///////////////////////////////////////////////

int active_channels_str_invalid_characters_check(char* active_ch_str);
int read_wanrouter_rc_file();
int check_directory();
int main_loop();
void cleanup();

///////////////////////////////////////////////

//convert int definition of a protocol to string
char * get_protocol_string(int protocol)
{
  volatile static char protocol_name[MAX_PATH_LENGTH];
  conf_file_reader* local_cfr = (conf_file_reader*)global_conf_file_reader_ptr;

  switch(protocol)
  {
  case PROTOCOL_NOT_SET:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Not Configured");
    break;

  case NO_PROTOCOL_NEEDED:
    //special case, when no protocol name actually needed
    protocol_name[0] = '\0';
    break;

  case WANCONFIG_X25:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "X25");
    break;

  case WANCONFIG_FR:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Frame Relay");
    break;

  case WANCONFIG_PPP:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "PPP");
    break;

  case WANCONFIG_CHDLC:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "CHDLC");
    break;

  case WANCONFIG_BSC:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "BiSync Streaming");
    break;

  case WANCONFIG_HDLC:
    //used with CHDLC firmware
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "HDLC Streaming");
    break;

  case WANCONFIG_MPPP://and WANCONFIG_MPROT too
    //snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Multi Port PPP");
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "PPP");
    break;

  case WANCONFIG_BITSTRM:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Bit Stream");
    break;

  case WANCONFIG_EDUKIT:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "WAN EduKit");
    break;

  case WANCONFIG_SS7:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "SS7");
    break;

  case WANCONFIG_BSCSTRM:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Bisync Streaming Nasdaq");
    break;

  case WANCONFIG_MFR:
    //snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Multi-Port Frame Relay");
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Frame Relay");
    break;

  case WANCONFIG_ADSL:

    if(local_cfr != NULL){
      switch(local_cfr->link_defs->sub_config_id)
      {
      case WANCONFIG_MPPP:
        snprintf((char*)protocol_name, MAX_PATH_LENGTH, "PPP (on ADSL)");
	break;
      
      case PROTOCOL_IP:
        snprintf((char*)protocol_name, MAX_PATH_LENGTH, ADSL_IP_STR);
        break;

      case PROTOCOL_ETHERNET:
        snprintf((char*)protocol_name, MAX_PATH_LENGTH, ADSL_ETHERNET_STR);
        break;
      
      default:
        snprintf((char*)protocol_name, MAX_PATH_LENGTH, "ADSL");
      }
      
    }else{
      snprintf((char*)protocol_name, MAX_PATH_LENGTH, "ADSL");
    }
    break;

  case WANCONFIG_SDLC:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "SDLC");
    break;

  case WANCONFIG_ATM:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "ATM");
    break;

  case WANCONFIG_POS:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Point-of-Sale");
    break;

  case WANCONFIG_AFT:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "AFT(TE1)");
    break;

  case WANCONFIG_AFT_TE3:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "AFT(TE3)");
    break;
    
  case WANCONFIG_DEBUG:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Real Time Debugging");
    break;

  case WANCONFIG_ADCCP:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Special HDLC LAPB");
    break;

  case WANCONFIG_MLINK_PPP:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Multi-Link PPP");
    break;

  case WANCONFIG_GENERIC:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "WANPIPE Generic driver");
    break;

  case WANCONFIG_MPCHDLC:
    //snprintf((char*)protocol_name, MAX_PATH_LENGTH, "Multi-Port CHDLC");
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "CHDLC");
    break;

  case WANCONFIG_TTY:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "TTY");
    break;
  
  case PROTOCOL_TDM_VOICE:
   snprintf((char*)protocol_name, MAX_PATH_LENGTH, "TDM Voice");
   break;

  case PROTOCOL_TDM_VOICE_API:
   snprintf((char*)protocol_name, MAX_PATH_LENGTH, "TDM Voice API");
   break;
   
  case WANCONFIG_LAPB:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "HDLC LAPB");
    break;

  case WANCONFIG_LIP_ATM:
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, "ATM (LIP)");
    break;
       
  default:
    ERR_DBG_OUT(("Invalid protocol: %d\n", protocol));
    snprintf((char*)protocol_name, MAX_PATH_LENGTH, INVALID_PROTOCOL);
    break;
  }

  return (char*)protocol_name;
}

int adjust_number_of_logical_channels_in_list(	int config_id,
						IN void* list,
						IN void* info,
						IN unsigned int new_number_of_logical_channels)
{
  int rc = YES, insert_rc;
  list_element_chan_def* chan_def_tmp = NULL;
  list_element_chan_def* list_el_chan_def_last = NULL;
  list_element_chan_def* list_el_chan_def_first = NULL;
  objects_list* obj_list = (objects_list*)list;
  char* name_of_parent_layer = NULL;
  conf_file_reader* cfr = NULL;

  Debug(DBG_WANCFG_MAIN, ("adjust_number_of_logical_channels_in_list(): config_id: %d.\n",
      config_id));
     
  Debug(DBG_WANCFG_MAIN, ("old number: %d, new: %d.\n",
      obj_list->get_size(), new_number_of_logical_channels));

  fr_config_info_t* fr_config_info = (fr_config_info_t*)info;

  switch(config_id)
  {
  case WANCONFIG_MPPP:
  case WANCONFIG_MPCHDLC:
  case WANCONFIG_TTY:
  case WANCONFIG_LAPB:
  case WANCONFIG_LIP_ATM:
    name_of_parent_layer = (char*)info;
    break;
	  
  case WANCONFIG_MFR:
    name_of_parent_layer = (char*)fr_config_info->name_of_parent_layer;
    break;

  case WANCONFIG_AFT:
  case WANCONFIG_GENERIC:
  case WANCONFIG_ADSL:
    cfr = (conf_file_reader*)info;
    break;

  default:
    ERR_DBG_OUT(("Invalid 'config_id': %d!\n", config_id));
    return NO;
  }

  //Adjust number of elements in the list to the new number.
  if(new_number_of_logical_channels > obj_list->get_size()){
    //
    //more interfaces DLCIs should be added.
    //
    while(obj_list->get_size() != new_number_of_logical_channels)
    {
      Debug(DBG_WANCFG_MAIN, ("obj_list->get_size(): %d (configid: %d)\n", obj_list->get_size(),
      		config_id));

      switch(config_id)
      {
      case WANCONFIG_MFR:
	chan_def_tmp = (list_element_chan_def*)(new list_element_frame_relay_dlci());
        list_el_chan_def_first = (list_element_chan_def*)obj_list->get_first();
	if(list_el_chan_def_first != NULL){
	  //for new dlci use settings of the first one in the list.
	  //this nesesary for 'menu_frame_relay_dlci_list' !!
	  //it is using the 1-st dlci in the list!
          wan_fr_conf_t* fr_first;
          wan_fr_conf_t* fr_new;
 
          fr_first = &list_el_chan_def_first->data.chanconf->u.fr;
          fr_new = &chan_def_tmp->data.chanconf->u.fr;

	  memcpy(fr_new, fr_first, sizeof(wan_fr_conf_t));
	}
	break;

      case WANCONFIG_MPPP:
	chan_def_tmp = (list_element_chan_def*)(new list_element_ppp_interface());
	break;
	
#if defined(CONFIG_PRODUCT_WANPIPE_LIP_ATM)
      case WANCONFIG_LIP_ATM:
	chan_def_tmp = (list_element_chan_def*)(new list_element_atm_interface());
	break;
#endif

      case WANCONFIG_MPCHDLC:
	chan_def_tmp = (list_element_chan_def*)(new list_element_chdlc_interface());
	break;
   
      case WANCONFIG_TTY:
	chan_def_tmp = (list_element_chan_def*)(new list_element_tty_interface());
	break;
	
      case WANCONFIG_LAPB:
	chan_def_tmp = (list_element_chan_def*)(new list_element_lapb_interface());
	break;
	
      default:
	chan_def_tmp = new list_element_chan_def();
      }

      list_el_chan_def_last = (list_element_chan_def*)obj_list->get_last();
      
      if(list_el_chan_def_last != NULL){
        //list is NOT empty.
        //create the temporary addr str. list does not accept the same addr twice so increment
        //the number for each next one.
	switch(config_id)
	{
	case WANCONFIG_MFR:
	  snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d",
                                          atoi(list_el_chan_def_last->data.addr) + 1);
	  chan_def_tmp->data.chanconf->config_id = WANCONFIG_MFR;
	  break;
	
	case WANCONFIG_LIP_ATM:
	  snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d",
                                          atoi(list_el_chan_def_last->data.addr) + 1);
	  chan_def_tmp->data.chanconf->config_id = WANCONFIG_LIP_ATM;
	  break;
  
	case WANCONFIG_MPPP:
	case WANCONFIG_TTY:
	case WANCONFIG_MPCHDLC:
	case WANCONFIG_LAPB:
	  //list has to be empty before adding an interface.
	  ERR_DBG_OUT(("Can NOT add more than one interface for protocol: %d !\n", config_id));
	  /*
	  snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d",
                                          atoi(list_el_chan_def_last->data.addr) + 1);
	  chan_def_tmp->data.chanconf->config_id = WANCONFIG_MPPP;
	  */
	  break;

	case WANCONFIG_AFT:
	case WANCONFIG_GENERIC:
	  snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d",
                                          atoi(list_el_chan_def_last->data.addr) + 1);
	  //chan_def_tmp->data.chanconf->config_id = PROTOCOL_NOT_SET;
	  chan_def_tmp->data.chanconf->config_id = PROTOCOL_NOT_SET;
	  chan_def_tmp->data.chanconf->u.aft.mtu = 1500;
          chan_def_tmp->data.chanconf->u.aft.mru = 1500;
          chan_def_tmp->data.chanconf->u.aft.idle_flag = 0x7E;
	  break;
	/*
	case WANCONFIG_ADSL:
	  snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d",
                                          atoi(list_el_chan_def_last->data.addr) + 1);
	  //chan_def_tmp->data.chanconf->config_id = WANCONFIG_ADSL;
	  //chan_def_tmp->data.chanconf->config_id = WANCONFIG_ADSL;
	  chan_def_tmp->data.chanconf->config_id = PROTOCOL_NOT_SET;
	  break;
	*/
	}
        
      }else{
	
        //list is empty. set to default.
	//default is different for each 'config_id'
	switch(config_id)
	{
	case WANCONFIG_MFR:
	  if(fr_config_info->auto_dlci == YES){
	    snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d", 1);
	  }else{
	    snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d", 16);
	  }
	  chan_def_tmp->data.chanconf->config_id = WANCONFIG_MFR;
	  break;

	case WANCONFIG_MPCHDLC:
	  snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d", 1);
	  chan_def_tmp->data.chanconf->config_id = WANCONFIG_MPCHDLC;
	  break;
	  
	case WANCONFIG_LIP_ATM:
	  snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d", 1);
	  chan_def_tmp->data.chanconf->config_id = WANCONFIG_LIP_ATM;
	  break;

	case WANCONFIG_MPPP:
	  snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d", 1);
	  chan_def_tmp->data.chanconf->config_id = WANCONFIG_MPPP;
	  break;

	case WANCONFIG_TTY:
	  snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d", 1);
	  chan_def_tmp->data.chanconf->config_id = WANCONFIG_TTY;
	  break;

	case WANCONFIG_LAPB:
	  snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d", 1);
	  chan_def_tmp->data.chanconf->config_id = WANCONFIG_LAPB;
	  break;
	  
	case WANCONFIG_AFT:
	case WANCONFIG_GENERIC:
	case WANCONFIG_ADSL:
	  snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d", 1);
	  chan_def_tmp->data.chanconf->config_id = PROTOCOL_NOT_SET;
	  chan_def_tmp->data.chanconf->u.aft.mtu = 1500;
          chan_def_tmp->data.chanconf->u.aft.mru = 1500;
          chan_def_tmp->data.chanconf->u.aft.idle_flag = 0x7E;
	  break;
	/*  
	case WANCONFIG_ADSL:
	  //snprintf(chan_def_tmp->data.addr, MAX_ADDR_STR_LEN, "%d", 1);
	  //chan_def_tmp->data.chanconf->config_id = WANCONFIG_ADSL;
	  //chan_def_tmp->data.chanconf->config_id = WANCONFIG_ADSL;
	  break;
	*/
	}//switch()
      }//if()

      ///////////////////////////////////////////////////////////////////////////////////////////////
      //create name for the new interface (dlci). can be changed later by the user in Net Interface cfg.
      switch(config_id)
      {
      case WANCONFIG_MFR:
      	if(fr_config_info->auto_dlci == YES){
		
#if defined(__LINUX__) && !BSD_DEBG
	  snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%sfr",
	    name_of_parent_layer);
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
	  snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%sf0",
	    //underlying layer name ends with digit - change it
	    replace_numeric_with_char(name_of_parent_layer));
#endif
	  
	}else{

#if defined(__LINUX__) && !BSD_DEBG
	  snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%sf%s",
	    name_of_parent_layer,
	    chan_def_tmp->data.addr);
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
	  snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%sf%s",
	    //underlying layer name ends with digit - change it
	    replace_numeric_with_char(name_of_parent_layer),
	    chan_def_tmp->data.addr);
#endif
	}
	break;

      case WANCONFIG_MPPP:
#if defined(__LINUX__) && !BSD_DEBG
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%sppp",
	    name_of_parent_layer);
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%sp0",
	    //underlying layer name ends with digit - change it
	    replace_numeric_with_char(name_of_parent_layer));
#endif
	break;
	
      case WANCONFIG_LIP_ATM:
#if defined(__LINUX__) && !BSD_DEBG
        snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%sa%s",
	    name_of_parent_layer,
	    chan_def_tmp->data.addr);
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%sa%s",
	    //underlying layer name ends with digit - change it
	    replace_numeric_with_char(name_of_parent_layer),
	    chan_def_tmp->data.addr);
#endif
	break;

      case WANCONFIG_MPCHDLC:
#if defined(__LINUX__) && !BSD_DEBG
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%schdl",
	    name_of_parent_layer);
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%sc0",
	    //underlying layer name ends with digit - change it
	    replace_numeric_with_char(name_of_parent_layer));
#endif
	break;
	
      case WANCONFIG_TTY:
#if defined(__LINUX__) && !BSD_DEBG
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%stty",
	    name_of_parent_layer);
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%stty0",
	    //underlying layer name ends with digit - change it
	    replace_numeric_with_char(name_of_parent_layer));
#endif
	break;
	
      case WANCONFIG_LAPB:
#if defined(__LINUX__) && !BSD_DEBG
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%slapb",
	    name_of_parent_layer);
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%slapb0",
	    //underlying layer name ends with digit - change it
	    replace_numeric_with_char(name_of_parent_layer));
#endif
	break;

      case WANCONFIG_AFT:
	//create name for the new Channel Group
#if defined(__LINUX__) && !BSD_DEBG
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "w%dg%s",
	  cfr->wanpipe_number, chan_def_tmp->data.addr);
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
	//this the AFT level interface - name must end with a digit
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "w%dg",
	  cfr->wanpipe_number);

	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%s",
	  replace_numeric_with_char(chan_def_tmp->data.name));
	      
	snprintf(&chan_def_tmp->data.name[strlen(chan_def_tmp->data.name)],
	  WAN_IFNAME_SZ, "%s", chan_def_tmp->data.addr);
#endif
	break;

      case WANCONFIG_GENERIC:
        //used for new S508/S514 configurations
#if defined(__LINUX__) && !BSD_DEBG
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "wp%d",
	  cfr->wanpipe_number);
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
	//name must end with a digit and nothing needs to be done here
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "wp%d",
	  cfr->wanpipe_number);
#endif
	break;

      case WANCONFIG_ADSL:
#if defined(__LINUX__) && !BSD_DEBG
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "w%dad",
	  cfr->wanpipe_number);
#elif (defined __FreeBSD__) || (defined __OpenBSD__) || defined(__NetBSD__) || BSD_DEBG
	//name must end with a digit: i.g. : wbad0 == w2ad0
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "w%dad",
	  cfr->wanpipe_number);
	
	snprintf(chan_def_tmp->data.name, WAN_IFNAME_SZ, "%s",
	  replace_numeric_with_char(chan_def_tmp->data.name));

	snprintf(&chan_def_tmp->data.name[strlen(chan_def_tmp->data.name)],
	  WAN_IFNAME_SZ, "%d", 0);
#endif
	break;
      }//switch()
      
      //very important check! otherwise may get into infinite loop!!
      if((insert_rc = obj_list->insert(chan_def_tmp)) != LIST_ACTION_OK){
        ERR_DBG_OUT(("Failed to add new interface: name: %s, address: %s, return code: %s!\n",
		chan_def_tmp->data.name, chan_def_tmp->data.addr, DECODE_LIST_ACTION_RC(insert_rc)));
        return NO;
      }

    }//while()

  }else if(new_number_of_logical_channels < obj_list->get_size()){

    //remove unneeded interfaces (DLCIs or ATM ifs) - from the end of the list!
    while(obj_list->get_size() != new_number_of_logical_channels &&
          (chan_def_tmp = (list_element_chan_def*)obj_list->remove_from_head()) != NULL){
      delete chan_def_tmp;
    }
  }else{
    //there is nothing to adjust.
    Debug(DBG_WANCFG_MAIN,
      ("adjust_number_of_logical_channels_in_list(): old == new  == %d.\n",
      obj_list->get_size()));
  }

  return rc;
}

int get_config_id_from_profile(char* config_id)
{
  if (!strcmp(config_id, "fr")){
    return WANCONFIG_MFR;
  }else if(!strcmp(config_id, "ppp")){
    return WANCONFIG_MPPP;
  }else if(!strcmp(config_id, "chdlc")){
    return WANCONFIG_MPCHDLC;
  }else if(!strcmp(config_id, "tty")){
    return WANCONFIG_TTY;
  }else if(!strcmp(config_id, "lip_lapb")){
    return WANCONFIG_LAPB;
  }else if(!strcmp(config_id, "lip_atm")){
    return WANCONFIG_LIP_ATM;
  }else{
    return PROTOCOL_NOT_SET;
  }
}

int check_file_exist(char * file_name, FILE ** file)
{
  int rc;

  *file = fopen(file_name, "r+");

  if(*file == NULL){
    Debug(DBG_WANCFG_MAIN, ("File '%s' does not exist.\n", file_name));
    rc = NO;
  }else{
    Debug(DBG_WANCFG_MAIN, ("File '%s' exist.\n", file_name));
    rc = YES;
    fclose(*file);
  }
  return rc;
}

char * get_card_type_string(int card_type, int card_version)
{
  static char card_type_name[MAX_PATH_LENGTH];

  snprintf(card_type_name, MAX_PATH_LENGTH, "???");

  switch(card_type)
  {
  case NOT_SET:
    snprintf(card_type_name, MAX_PATH_LENGTH, "Undefined");
    break;
  case WANOPT_S50X:
    snprintf(card_type_name, MAX_PATH_LENGTH, "S508");
    break;
  case WANOPT_S51X:
    snprintf(card_type_name, MAX_PATH_LENGTH, "S514X");
    break;
  case WANOPT_ADSL:
    snprintf(card_type_name, MAX_PATH_LENGTH, "S518-ADSL");
    break;
  case WANOPT_AFT:
    switch(card_version)
    {
    case A101_ADPTR_1TE1://WAN_MEDIA_T1:
    	snprintf(card_type_name, MAX_PATH_LENGTH, "A101/2");
		break;
    case A104_ADPTR_4TE1://including A101-SH and A102-SH
        snprintf(card_type_name, MAX_PATH_LENGTH, "A101/2/4/8");
		break;
    case A300_ADPTR_U_1TE3://WAN_MEDIA_DS3:
    	snprintf(card_type_name, MAX_PATH_LENGTH, "A301-T3/E3");
		break;
    case A200_ADPTR_ANALOG:
    	snprintf(card_type_name, MAX_PATH_LENGTH, "A200/A400-Analog");
		break;
    case AFT_ADPTR_56K:
    	snprintf(card_type_name, MAX_PATH_LENGTH, "A056-56k DDS");
		break;
	case AFT_ADPTR_ISDN:
    	snprintf(card_type_name, MAX_PATH_LENGTH, "A500-ISDN BRI");
		break;
	case AFT_ADPTR_2SERIAL_V35X21:
    	snprintf(card_type_name, MAX_PATH_LENGTH, "A14X-Serial");
		break;
    default:
        ERR_DBG_OUT(("Invalid AFT card version: 0x%x!\n", card_version));
    }
    break;
  default:
    snprintf(card_type_name, MAX_PATH_LENGTH, "Invalid Card Type!");
  }
  return card_type_name;
}

//////////////////////////////////////////////////////////
//all types of S514 can be devided on 4
//real types
char* get_S514_card_version_string(char card_version)
{
  switch(card_version)
  {
    case S5141_ADPTR_1_CPU_SERIAL:
      return "S514 - Serial";

    case S5143_ADPTR_1_CPU_FT1:
      return "S514 - FT1";

    case S5144_ADPTR_1_CPU_T1E1:
      return "S514 - T1/E1";

    case S5145_ADPTR_1_CPU_56K:
      return "S514 - 56K DDS";

    default:
      return NULL;
  }
}

//////////////////////////////////////////////////////////
#define MIN_LINES   24
#define MIN_COLUMNS 80
#define WIN_SIZE_ERR_MESSAGE "Your display is too small to run WANPIPE Config!\n\
It must be at least 24 lines by 80 columns.\n"

int is_console_size_valid()
{
  struct winsize win;
  int err = ioctl ( 0, TIOCGWINSZ, (char *)&win);
  Debug(DBG_WANCFG_MAIN, ("err: 0x%X\n", err));

  if(err == 0){
    Debug(DBG_WANCFG_MAIN, ("win.ws_row: %d\n", win.ws_row));
    Debug(DBG_WANCFG_MAIN, ("win.ws_col: %d\n", win.ws_col));

    if(win.ws_row < MIN_LINES){
      ERR_DBG_OUT((WIN_SIZE_ERR_MESSAGE));
      return NO;	
    }
    if(win.ws_col < MIN_COLUMNS){
      ERR_DBG_OUT((WIN_SIZE_ERR_MESSAGE));
      return NO;	
    }
  }else{
    //window size unknown. put a warning but continue.
    err_printf("\nFailed to get console window size row=%i col=%i!\n",
		    	win.ws_row,win.ws_col);
  }
  return YES;
}

//////////////////////////////////////////////////////////

char check_wanpipe_start_script_exist(char* wanpipe_name)
{
  char file_path[MAX_PATH_LENGTH];
  FILE* f;

  snprintf(file_path, MAX_PATH_LENGTH, "%s%s/%s-start",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name);

  return check_file_exist(file_path, &f);
}

char check_wanpipe_stop_script_exist(char* wanpipe_name)
{
  char file_path[MAX_PATH_LENGTH];
  FILE* f;

  snprintf(file_path, MAX_PATH_LENGTH, "%s%s/%s-stop",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name);

  return check_file_exist(file_path, &f);
}

void remove_wanpipe_start_script(char* wanpipe_name)
{
  char command_line[MAX_PATH_LENGTH];

  snprintf(command_line, MAX_PATH_LENGTH, "rm -rf ");

  snprintf(&command_line[strlen(command_line)], MAX_PATH_LENGTH, "%s%s/%s-start",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name);

  Debug(DBG_WANCFG_MAIN, ("remove_wanpipe_start_script(): command line: %s\n",
    command_line));

  system(command_line);
}

void remove_wanpipe_stop_script(char* wanpipe_name)
{
  char command_line[MAX_PATH_LENGTH];

  snprintf(command_line, MAX_PATH_LENGTH, "rm -rf ");

  snprintf(&command_line[strlen(command_line)], MAX_PATH_LENGTH, "%s%s/%s-stop",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name);

  Debug(DBG_WANCFG_MAIN, ("remove_wanpipe_stop_script(): command line: %s\n",
    command_line));

  system(command_line);
}

int create_wanpipe_start_script(char* wanpipe_name)
{
  char temp_str[MAX_PATH_LENGTH];
  string wanpipe_script_str;

char wanpipe_start_script_part1[] = {
"#!/bin/sh\n"
"#\n"
};

char wanpipe_start_script_part2[] = {
"#\n"
"# Description:\n"
"# 	This script is called by /usr/sbin/wanrouter\n"
"#       after the device has been started.\n"
"#\n"
"#       Use this script to add extra routes or start\n"
"#       services that relate directly to this\n"
"#       particular device.\n"
"#\n"
"# Arguments:\n"
};

  ////////////////////////////////////////////////////////////////////////////
  wanpipe_script_str = wanpipe_start_script_part1;
  ////////////////////////////////////////////////////////////////////////////

  snprintf(temp_str, MAX_PATH_LENGTH, "# WANPIPE Device (%s) Start Script\n",
    wanpipe_name);
  wanpipe_script_str += temp_str;
  ////////////////////////////////////////////////////////////////////////////

  wanpipe_script_str += wanpipe_start_script_part2;
  ////////////////////////////////////////////////////////////////////////////

  snprintf(temp_str, MAX_PATH_LENGTH, "#\t%s = wanpipe device name\n",
    wanpipe_name);
  wanpipe_script_str += temp_str;
  ////////////////////////////////////////////////////////////////////////////

  //create full file path
  snprintf(temp_str, MAX_PATH_LENGTH, "%s%s/%s-start",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name);

  return write_string_to_file(temp_str, (char*)wanpipe_script_str.c_str());
}

/////////////////////////////////////////////////////////////////////////////

int create_wanpipe_stop_script(char* wanpipe_name)
{
  char temp_str[MAX_PATH_LENGTH];
  string wanpipe_script_str;

char wanpipe_stop_script_part1[] = {
"#!/bin/sh\n"
"#\n"
};

char wanpipe_stop_script_part2[] = {
"#\n"
"# Description:\n"
"# 	This script is called by /usr/sbin/wanrouter\n"
"#       after the device has been stopped.\n"
"#\n"
"#       Use this script to remove extra routes or stop\n"
"#       services that relate directly to this\n"
"#       particular device.\n"
"#\n"
"# Arguments:\n"
};

  ////////////////////////////////////////////////////////////////////////////
  wanpipe_script_str = wanpipe_stop_script_part1;
  ////////////////////////////////////////////////////////////////////////////

  snprintf(temp_str, MAX_PATH_LENGTH, "# WANPIPE Device (%s) Stop Script\n",
    wanpipe_name);
  wanpipe_script_str += temp_str;
  ////////////////////////////////////////////////////////////////////////////

  wanpipe_script_str += wanpipe_stop_script_part2;
  ////////////////////////////////////////////////////////////////////////////

  snprintf(temp_str, MAX_PATH_LENGTH, "#\t%s = wanpipe device name\n",
    wanpipe_name);
  wanpipe_script_str += temp_str;
  ////////////////////////////////////////////////////////////////////////////

  //create full file path
  snprintf(temp_str, MAX_PATH_LENGTH, "%s%s/%s-stop",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name);

  return write_string_to_file(temp_str, (char*)wanpipe_script_str.c_str());
}

/////////////////////////////////////////////////////////////////////////////

char check_net_interface_start_script_exist(char* wanpipe_name, char* if_name)
{
  char file_path[MAX_PATH_LENGTH];
  FILE* f;

  snprintf(file_path, MAX_PATH_LENGTH, "%s%s/%s-%s-start",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name,
    if_name);

  return check_file_exist(file_path, &f);
}

char check_net_interface_stop_script_exist(char* wanpipe_name, char* if_name)
{
  char file_path[MAX_PATH_LENGTH];
  FILE* f;

  snprintf(file_path, MAX_PATH_LENGTH, "%s%s/%s-%s-stop",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name,
    if_name);

  return check_file_exist(file_path, &f);
}

//////////////////////////////////////////////////////////////////////////////

void remove_net_interface_start_script(char* wanpipe_name, char* if_name)
{
  char command_line[MAX_PATH_LENGTH];

  snprintf(command_line, MAX_PATH_LENGTH, "rm -rf ");

  snprintf(&command_line[strlen(command_line)], MAX_PATH_LENGTH, "%s%s/%s-%s-start",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name,
    if_name);

  Debug(DBG_WANCFG_MAIN, ("remove file command line: %s\n", command_line));

  system(command_line);
}

//////////////////////////////////////////////////////////////////////////////

int edit_wanpipe_start_script(char* wanpipe_name)
{
  int sytem_call_status;
  int vi_exit_status;
  int rc = YES;
  char command_line[MAX_PATH_LENGTH];

  snprintf(command_line, MAX_PATH_LENGTH, "vi ");

  snprintf(&command_line[strlen(command_line)], MAX_PATH_LENGTH, "%s%s/%s-start",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name);

  sytem_call_status = system(command_line);

  if(WIFEXITED(sytem_call_status)){

    vi_exit_status = WEXITSTATUS(sytem_call_status);

    Debug(DBG_WANCFG_MAIN, ("vi_exit_status: %d\n", vi_exit_status));

    if(vi_exit_status != 0){
      rc = NO;
      ERR_DBG_OUT(("Failed to start Text Editor (vi)!!\n"));
    }

  }else{
    //Text Editor (vi) crashed or killed
    ERR_DBG_OUT(("Text Editor (vi) exited abnormally (sytem_call_status: %d).\n",
            sytem_call_status));
    rc = NO;
  }

  return rc;
}

//////////////////////////////////////////////////////////////////////////////

int edit_wanpipe_stop_script(char* wanpipe_name)
{
  int sytem_call_status;
  int vi_exit_status;
  int rc = YES;
  char command_line[MAX_PATH_LENGTH];

  snprintf(command_line, MAX_PATH_LENGTH, "vi ");

  snprintf(&command_line[strlen(command_line)], MAX_PATH_LENGTH, "%s%s/%s-stop",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name);

  sytem_call_status = system(command_line);

  if(WIFEXITED(sytem_call_status)){

    vi_exit_status = WEXITSTATUS(sytem_call_status);

    Debug(DBG_WANCFG_MAIN, ("vi_exit_status: %d\n", vi_exit_status));

    if(vi_exit_status != 0){
      rc = NO;
      ERR_DBG_OUT(("Failed to start Text Editor (vi)!!\n"));
    }

  }else{
    //Text Editor (vi) crashed or killed
    ERR_DBG_OUT(("Text Editor (vi) exited abnormally (sytem_call_status: %d).\n",
            sytem_call_status));
    rc = NO;
  }

  return rc;
}

//////////////////////////////////////////////////////////////////////////////

void remove_net_interface_stop_script(char* wanpipe_name, char* if_name)
{
  char command_line[MAX_PATH_LENGTH];

  snprintf(command_line, MAX_PATH_LENGTH, "rm -rf ");

  snprintf(&command_line[strlen(command_line)], MAX_PATH_LENGTH, "%s%s/%s-%s-stop",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name,
    if_name);

  Debug(DBG_WANCFG_MAIN, ("remove file command line: %s\n", command_line));

  system(command_line);
}

//////////////////////////////////////////////////////////

int edit_net_interface_start_script(char* wanpipe_name, char* if_name)
{
  int sytem_call_status;
  int vi_exit_status;
  int rc = YES;
  char command_line[MAX_PATH_LENGTH];

  snprintf(command_line, MAX_PATH_LENGTH, "vi ");

  snprintf(&command_line[strlen(command_line)], MAX_PATH_LENGTH, "%s%s/%s-%s-start",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name,
    if_name);

  sytem_call_status = system(command_line);

  if(WIFEXITED(sytem_call_status)){

    vi_exit_status = WEXITSTATUS(sytem_call_status);

    Debug(DBG_WANCFG_MAIN, ("vi_exit_status: %d\n", vi_exit_status));

    if(vi_exit_status != 0){
      rc = NO;
      ERR_DBG_OUT(("Failed to start Text Editor (vi)!!\n"));
    }

  }else{
    //Text Editor (vi) crashed or killed
    ERR_DBG_OUT(("Text Editor (vi) exited abnormally (sytem_call_status: %d).\n",
            sytem_call_status));
    rc = NO;
  }

  return rc;
}

//////////////////////////////////////////////////////////

int edit_net_interface_stop_script(char* wanpipe_name, char* if_name)
{
  int sytem_call_status;
  int vi_exit_status;
  int rc = YES;
  char command_line[MAX_PATH_LENGTH];

  snprintf(command_line, MAX_PATH_LENGTH, "vi ");

  snprintf(&command_line[strlen(command_line)], MAX_PATH_LENGTH, "%s%s/%s-%s-stop",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name,
    if_name);

  sytem_call_status = system(command_line);

  if(WIFEXITED(sytem_call_status)){

    vi_exit_status = WEXITSTATUS(sytem_call_status);

    Debug(DBG_WANCFG_MAIN, ("vi_exit_status: %d\n", vi_exit_status));

    if(vi_exit_status != 0){
      rc = NO;
      ERR_DBG_OUT(("Failed to start Text Editor (vi)!!\n"));
    }

  }else{
    //Text Editor (vi) crashed or killed
    ERR_DBG_OUT(("Text Editor (vi) exited abnormally (sytem_call_status: %d).\n",
            sytem_call_status));
    rc = NO;
  }

  return rc;
}

//////////////////////////////////////////////////////////

int create_new_net_interface_start_script(char* wanpipe_name, char* if_name)
{
  char temp_str[MAX_PATH_LENGTH];
  string net_start_script_str;

char net_if_start_script_part1[] = {
"#!/bin/sh\n"
"#\n"
};

//"#WANPIPE Interface (wp2fr16) Start Script\n"
char net_if_start_script_part2[] = {
"#\n"
"# Description:\n"
"# 	This script is called by /usr/sbin/wanrouter\n"
"#       after the interface has been started using\n"
"#       ifconfig.\n"
"#\n"
"#       Use this script to add extra routes or start\n"
"#       services that relate directly to this\n"
"#       particular interafce.\n"
"#\n"
"# Arguments:\n"
};

//#       wanpipe2 = wanpipe device name    (wanpipe1)
//#       wp2fr16 = wanpipe interface name (wan0)
char net_if_start_script_part3[] = {
"#\n"
"# Dynamic Mode:\n"
"#       This script will be called by the\n"
"#       wanpipe kernel driver any time the\n"
"#       state of the interface changes.\n"
"#\n"
"#       Eg: If interface goes up and down,\n"
"#           or changes IP addresses.\n"
"#\n"
"#       Wanpipe Syncppp Driver will call this\n"
"#       script when IPCP negotiates new IP\n"
"#       addresses\n"
"#\n"
"# Dynamic Environment Variables:\n"
"#       These variables are available only when\n"
"#       the script is called via wanpipe kernel\n"
"#       driver.\n"
"#\n"
"#\n"
};

  ////////////////////////////////////////////////////////////////////////////
  net_start_script_str = net_if_start_script_part1;
  ////////////////////////////////////////////////////////////////////////////

  snprintf(temp_str, MAX_PATH_LENGTH, "#WANPIPE Interface (%s) Start Script\n",
    if_name);
  net_start_script_str += temp_str;
  ////////////////////////////////////////////////////////////////////////////

  net_start_script_str += net_if_start_script_part2;
  ////////////////////////////////////////////////////////////////////////////

  snprintf(temp_str, MAX_PATH_LENGTH, "#\t%s = wanpipe device name\n",
    wanpipe_name);
  net_start_script_str += temp_str;
  ////////////////////////////////////////////////////////////////////////////

  snprintf(temp_str, MAX_PATH_LENGTH, "#\t%s = wanpipe interface name\n",
    if_name);
  net_start_script_str += temp_str;
  ////////////////////////////////////////////////////////////////////////////
  net_start_script_str += net_if_start_script_part3;

  //create full file path
  snprintf(temp_str, MAX_PATH_LENGTH, "%s%s/%s-%s-start",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name,
    if_name);

  return write_string_to_file(temp_str, (char*)net_start_script_str.c_str());
}

//////////////////////////////////////////////////////////

int create_new_net_interface_stop_script(char* wanpipe_name, char* if_name)
{
  char temp_str[MAX_PATH_LENGTH];
  string net_stop_script_str;

char net_if_stop_script_part1[] = {
"#!/bin/sh\n"
"#\n"
};

//# WANPIPE Interface (wp2fr16) Stop Script
char net_if_stop_script_part2[] = {
"#\n"
"# Description:\n"
"# 	This script is called by /usr/sbin/wanrouter\n"
"#       after the interface has been stopped using\n"
"#       ifconfig.\n"
"#\n"
"#       Use this script to remove routes or stop\n"
"#       services that relate directly to this\n"
"#       particular interafce.\n"
"#\n"
"# Arguments:\n"
};

  ////////////////////////////////////////////////////////////////////////////
  net_stop_script_str = net_if_stop_script_part1;
  ////////////////////////////////////////////////////////////////////////////

  snprintf(temp_str, MAX_PATH_LENGTH, "#WANPIPE Interface (%s) Stop Script\n",
    if_name);
  net_stop_script_str += temp_str;
  ////////////////////////////////////////////////////////////////////////////

  net_stop_script_str += net_if_stop_script_part2;
  ////////////////////////////////////////////////////////////////////////////

  snprintf(temp_str, MAX_PATH_LENGTH, "#\t%s = wanpipe device name\n",
    wanpipe_name);
  net_stop_script_str += temp_str;
  ////////////////////////////////////////////////////////////////////////////

  snprintf(temp_str, MAX_PATH_LENGTH, "#\t%s = wanpipe interface name\n",
    if_name);
  net_stop_script_str += temp_str;
  ////////////////////////////////////////////////////////////////////////////

  //create full file path
  snprintf(temp_str, MAX_PATH_LENGTH, "%s%s/%s-%s-stop",
    wanpipe_cfg_dir,
    start_stop_scripts_dir,
    wanpipe_name,
    if_name);

  return write_string_to_file(temp_str, (char*)net_stop_script_str.c_str());
}

//////////////////////////////////////////////////////////

int write_string_to_file(char * full_file_path, char* string)
{
  FILE * file;

  Debug(DBG_WANCFG_MAIN, ("write_string_to_file():\n path: %s\n str: %s\n",
    full_file_path, string));

 // return YES;

  file = fopen(full_file_path, "w");
  if(file == NULL){
    ERR_DBG_OUT(("Failed to open %s file for writing!\n", full_file_path));
    return NO;
  }

  fputs(string, file);
  fclose(file);
  return YES;
}

//////////////////////////////////////////////////////////

int append_string_to_file(char * full_file_path, char* string)
{
  FILE * file;

  Debug(DBG_WANCFG_MAIN, ("append_string_to_file():\n path: %s\n str: %s\n",
    full_file_path, string));

  //return YES;

  file = fopen(full_file_path, "a");
  if(file == NULL){
    ERR_DBG_OUT(("Failed to open %s file for writing!\n", full_file_path));
    return NO;
  }

  fputs(string, file);
  fclose(file);
  return YES;
}

//////////////////////////////////////////////////////////

//function used for printing errors in the release version.
void err_printf(char* format, ...)
{
  char dbg_tmp_buff[LEN_OF_DBG_BUFF];
  va_list ap;

  va_start(ap, format);
  vsnprintf(dbg_tmp_buff, LEN_OF_DBG_BUFF, format, ap);
  va_end(ap);

  printf(dbg_tmp_buff);
  printf("\n");
}

/*
//caller must be sure 'input' string is actually an integer.
char* remove_spaces_in_int_string(char* input)
{
  static char output [LEN_OF_DBG_BUFF];

  //remove spaces and zero terminate.
  snprintf(output, LEN_OF_DBG_BUFF, "%d", atoi(input));
  return output;
}
*/

//caller must be sure 'input' string is actually an integer.
char* remove_spaces_in_int_string(char* input)
{
  char output [LEN_OF_DBG_BUFF];

  //remove spaces and zero terminate.
  snprintf(output, LEN_OF_DBG_BUFF, "%d", atoi(input));
  //copy back to caller's buffer
  strlcpy(input, output, MAX_PATH_LENGTH);

  return input;
}

int yes_no_question(  OUT int* selection_index,
                      IN char * lxdialog_path,
                      IN int protocol,
                      IN char* format, ...)
{
  FILE * yes_no_question_file;
  char yes_no_question_buff[LEN_OF_DBG_BUFF];
  va_list ap;
  char* yes_no_question_file_name = "wancfg_yes_no_question_file_name";
  //char* remove_yes_no_question_file = "rm -rf wancfg_yes_no_question_file_name";
  int rc = YES;
  char file_path[MAX_PATH_LENGTH];
  char remove_tmp_file[MAX_PATH_LENGTH];

  text_box_yes_no yn_tb;

  snprintf(file_path, MAX_PATH_LENGTH, "%s%s", wanpipe_cfg_dir,
                                               yes_no_question_file_name);

  snprintf(remove_tmp_file, MAX_PATH_LENGTH, "rm -rf %s", file_path);

	va_start(ap, format);
  vsnprintf(yes_no_question_buff, LEN_OF_DBG_BUFF, format, ap);
	va_end(ap);

  //open temporary file
  yes_no_question_file = fopen(file_path, "w");
  if(yes_no_question_file == NULL){
    ERR_DBG_OUT(("Failed to open 'yes_no_question_file' %s file for writing!\n",
      file_path));
    return NO;
  }

  fputs(yes_no_question_buff, yes_no_question_file);
  fclose(yes_no_question_file);

  if(yn_tb.set_configuration( lxdialog_path,
                              protocol,
                              yes_no_question_file_name) == NO){
    rc = NO;
    goto cleanup;
  }

  if(yn_tb.show(selection_index) == NO){
    rc = NO;
    goto cleanup;
  }

cleanup:
  //remove the temporary file
  system(remove_tmp_file);

  return rc;
}

//
//input : Configuration file name of type: wanpipe??.conf
//output: 0 - if error occured. (0 is invalid Wanpipe number too)
//        positive value (wanpipe number) if successful.
//
int get_wanpipe_number_from_conf_file_name(char* conf_file_name)
{
  char* digit_ptr = NULL;
  int wanpipe_number = 0;

  if(strlen(WANPIPE_TOKEN) >= strlen(conf_file_name)){
    ERR_DBG_OUT(("Invalid 'conf' file name: %s\n", conf_file_name));
    return 0;
  }

  digit_ptr = conf_file_name + strlen(WANPIPE_TOKEN);

  //atoi() - returns 0 if failes.
  wanpipe_number = atoi(digit_ptr);

  if(wanpipe_number > MAX_NUMBER_OF_WANPIPES){
    ERR_DBG_OUT(("Invalid wanpipe number: %d\n", wanpipe_number));
    return 0;
  }
  Debug(DBG_WANCFG_MAIN, ("wanpipe number: %d\n", wanpipe_number));

  return wanpipe_number;
}

//converts all characters in the string to lower case.
void str_tolower(char * str)
{
  unsigned int i;

  //Debug(DBG_WANCFG_MAIN, "str before str_tolower: %s\n", str);

  for(i = 0; i < strlen(str); i++){
    str[i] = (char)tolower(str[i]);
  }

  //Debug(DBG_WANCFG_MAIN, "str after str_tolower: %s\n", str);
}

//converts all characters in the string to upper case.
void str_toupper(char * str)
{
  unsigned int i;

  //Debug(DBG_WANCFG_MAIN, "str before str_toupper: %s\n", str);

  for(i = 0; i < strlen(str); i++){
    str[i] = (char)toupper(str[i]);
  }

  //Debug(DBG_WANCFG_MAIN, "str after str_toupper: %s\n", str);
}

int get_used_by_integer_value(char* used_by_str)
{
  if(strcmp(used_by_str, "WANPIPE") == 0){
    return WANPIPE;
  }else if(strcmp(used_by_str, "API") == 0){
    return API;
  }else if(strcmp(used_by_str, "TDM_API") == 0){
    return TDM_API;
  }else if(strcmp(used_by_str, "BRIDGE") == 0){
    return BRIDGE;
  }else if(strcmp(used_by_str, "BRIDGE_NODE") == 0){
    return BRIDGE_NODE;
  }else if(strcmp(used_by_str, "SWITCH") == 0){
    return WP_SWITCH;
  }else if(strcmp(used_by_str, "STACK") == 0){
    return STACK;
  }else if(strcmp(used_by_str, "ANNEXG") == 0){
    return ANNEXG;
  }else if(strcmp(used_by_str, "TDM_VOICE") == 0){
    return TDM_VOICE;
  }else if(strcmp(used_by_str, "TTY") == 0){
    return TTY;
  }else if(strcmp(used_by_str, "PPPoE") == 0){
    return PPPoE;
  }else if(strcmp(used_by_str, "NETGRAPH") == 0){
    return WP_NETGRAPH;
  }else if(strcmp(used_by_str, "TDM_VOICE_API") == 0){
    return TDM_VOICE_API;
  }else{
    return -1;
  }
}

char* get_used_by_string(int used_by)
{
  switch(used_by)
  {
  case WANPIPE:
    return "WANPIPE";

  case API:
    return "API";

  case TDM_API:
    return "TDM_API";

  case BRIDGE:
    return "BRIDGE";

  case BRIDGE_NODE:
    return "BRIDGE_NODE";

  case WP_SWITCH:
    return "SWITCH";

  case STACK:
    return "STACK";

  case ANNEXG:
    return "ANNEXG";

  case TDM_VOICE:
    return "TDM_VOICE";

  case PPPoE:
    return "PPPoE";
  
  case TTY:
    return "TTY";
    
  case WP_NETGRAPH:
    return "NETGRAPH";

  case TDM_VOICE_API:
    return "TDM_VOICE_API";
    
  default:
    return "Unknown Operation Mode";
  }
}

char* replace_new_line_with_zero_term(char* str)
{
  unsigned int i;

  for(i = 0; i < strlen(str); i++){
    if(str[i] == '\n'){
      str[i] = '\0';
    }
  }
  return str;
}

char* replace_new_line_with_space(char* str)
{
  unsigned int i;

  for(i = 0; i < strlen(str); i++){
    if(str[i] == '\n'){
      str[i] = ' ';
    }
  }
  return str;
}

void tokenize_string(char* input_buff, char* delimeter_str, char* output_buff, int buff_length)
{
  char* p = strtok(input_buff, delimeter_str);

  output_buff[0] = '\0';

  while(p != NULL){
    //printf(p);
    snprintf(&output_buff[strlen(output_buff)], buff_length - strlen(output_buff), p);
    p = strtok(NULL, delimeter_str);
  }
  Debug(DBG_WANCFG_MAIN, ("\ntokenized str: %s\n", output_buff));
}

char* replace_char_with_other_char_in_str(char* str, char old_char, char new_char)
{
  unsigned int i;

  for(i = 0; i < strlen(str); i++){
    if(str[i] == old_char){
      str[i] = new_char;
    }
  }
  return str;
}

//////////////////////////////////////////////////////////
char* get_date_and_time()
{
  int system_exit_status;
  static char date_and_time_str[MAX_PATH_LENGTH];
  char command_line[MAX_PATH_LENGTH];
  FILE* date_and_time_file;

  snprintf(command_line, MAX_PATH_LENGTH, "date > %s", date_and_time_file_name);
  Debug(DBG_WANCFG_MAIN, ("get_date_and_time() command line: %s\n", command_line));

  system_exit_status = system(command_line);
  system_exit_status = WEXITSTATUS(system_exit_status);

  if(system_exit_status != 0){
    ERR_DBG_OUT(("The following command failed: %s\n", command_line));
    return "Unknown";
  }

  date_and_time_file = fopen(date_and_time_file_name, "r+");
  if(date_and_time_file == NULL){
    ERR_DBG_OUT(("Failed to open the file '%s' for reading!\n",
      date_and_time_file_name));
    return "Unknown";
  }

  do{
    fgets(date_and_time_str, MAX_PATH_LENGTH, date_and_time_file);

    if(!feof(date_and_time_file)){
      Debug(DBG_WANCFG_MAIN, ("date_and_time_str: %s\n", date_and_time_str));
    }//if()
  }while(!feof(date_and_time_file));

  return date_and_time_str;
}


/*============================================================================
 * TE1
 * Parse active channel string.
 *
 * Return ULONG value, that include 1 in position `i` if channels i is active.
 */
unsigned int parse_active_channel(char* val, unsigned char media_type)
{
#define SINGLE_CHANNEL	0x2
#define RANGE_CHANNEL	  0x1
	int channel_flag = 0;
	char* ptr = val;
	int channel = 0, start_channel = 0;
	unsigned int tmp = 0;

  Debug(DBG_WANCFG_MAIN, ("parse_active_channel(): input: %s, media_type: %d\n", val, media_type));

	if (strcmp(val,"ALL") == 0)
		return ENABLE_ALL_CHANNELS;

  if(active_channels_str_invalid_characters_check(val) == NO){
    return 0;
  }

	while(*ptr != '\0') {
		if (isdigit(*ptr)) {
			channel = strtoul(ptr, &ptr, 10);
			channel_flag |= SINGLE_CHANNEL;
		} else {
			if (*ptr == '-') {
				channel_flag |= RANGE_CHANNEL;
				start_channel = channel;
			} else {
				tmp |= get_active_channels(channel_flag, start_channel, channel, media_type);
				channel_flag = 0;
			}
			ptr++;
		}
	}
	if (channel_flag){
		tmp |= get_active_channels(channel_flag, start_channel, channel, media_type);
	}

  if(check_channels(channel_flag, start_channel, channel, media_type) == NO){
    tmp = 0;
  }
	return tmp;
}

int active_channels_str_invalid_characters_check(char* active_ch_str)
{
	char* ptr = active_ch_str;

	while(*ptr != '\0') {
		if (isdigit(*ptr)) {
      			;//ok
		} else {
			if(*ptr == '-' || *ptr == '.') {
        			;//ok
			}else{
			        //invalid char
			        printf("Found invalid character '%c' in active channels string!!", *ptr);
			        return NO;
			}
		}
		ptr++;
	}//while()
  return YES;
}

/*============================================================================
 * TE1
 */
unsigned int get_active_channels(int channel_flag, int start_channel,
                                  int stop_channel,  unsigned char media_type)
{
	int i = 0;
	unsigned int tmp = 0, mask = 0;

	if ((channel_flag & (SINGLE_CHANNEL | RANGE_CHANNEL)) == 0)
		return tmp;
	if (channel_flag & RANGE_CHANNEL) { /* Range of channels */
		for(i = start_channel; i <= stop_channel; i++) {
			mask = 1 << (i - 1);
			tmp |=mask;
		}
	} else { /* Single channel */
		mask = 1 << (stop_channel - 1);
		tmp |= mask;
	}
	return tmp;
}

int check_channels(int channel_flag, unsigned int start_channel,
                   unsigned int stop_channel, unsigned char media_type)
{
  Debug(DBG_WANCFG_MAIN, ("check_channels(): media_type: %d, start_channel: %d, stop_channel: %d\n",
    media_type, start_channel, stop_channel));

	if ((channel_flag & 0x03) == 0)
		return NO;
	if ((channel_flag & RANGE_CHANNEL) && (start_channel > stop_channel))
		return NO;
	if (!(channel_flag & RANGE_CHANNEL))
		start_channel = stop_channel;
	if ( media_type == WAN_MEDIA_T1){

		if( (start_channel < 1) || (stop_channel > NUM_OF_T1_CHANNELS) )
			return NO;
	}else if(media_type == WAN_MEDIA_E1){

		if( (start_channel < 1) || (stop_channel > NUM_OF_E1_TIMESLOTS) )
			return NO;
	}
	return YES;
}

//returns:  NULL - if string is valid
//          pointer to err explanation if string is invalid
char* validate_ipv4_address_string(char* str)
{
  char* token;
  char seps[] = ".";
  int field_counter = 0;
  static char err_buf[MAX_PATH_LENGTH];

#define MAX_IPV4_LEN_STR  3*4+4
#define MIN_IPV4_LEN_STR  3*1+4

  if(strlen(str) > MAX_IPV4_LEN_STR){
    snprintf(err_buf, MAX_PATH_LENGTH,
      "Number of characters in IP string is greater than %d!\n", MAX_IPV4_LEN_STR);
    return err_buf;
  }
  if(strlen(str) < MIN_IPV4_LEN_STR){
    snprintf(err_buf, MAX_PATH_LENGTH,
      "Number of characters in IP string is less than %d!\n", MIN_IPV4_LEN_STR);
    return err_buf;
  }

  token = strtok((char*)str, seps);
  while(token != NULL){
    printf("token: %s\n", token);

    if(strlen(token) > 3){
      snprintf(err_buf, MAX_PATH_LENGTH,
        "Invalid number of characters in IP string field (%s).\n", token);
      return err_buf;
    }

    for(unsigned int i = 0; i < strlen(token); i++){
      if(isdigit(token[i]) == 0){
        snprintf(err_buf, MAX_PATH_LENGTH,
          "Invalid characters in IP string field (%s).\n", token);
        return err_buf;
      }
    }

    field_counter++;
    token = strtok(NULL, seps);
  }

  if(field_counter != 4){
    snprintf(err_buf, MAX_PATH_LENGTH,
      "Invalid number of \".\" separated fields in IP string.\n");
    return err_buf;
  }
  return NULL;
}

char* validate_authentication_string(char* str, int max_length)
{
  static char err_buf[MAX_PATH_LENGTH];

  Debug(DBG_WANCFG_MAIN, ("validate_authentication_string(): str: %s\n", str));

  if(strlen(str) > (unsigned)max_length){
    snprintf(err_buf, MAX_PATH_LENGTH,
      "Number of characters in authentication\nstring is greater than %d!\n",
      max_length);
    return err_buf;
  }

  if(strlen(str) < 1){
    snprintf(err_buf, MAX_PATH_LENGTH,
      "Number of characters in authentication\nstring is less than 1!\n");
    return err_buf;
  }
/*
  for(unsigned int i = 0; i < strlen(str); i++){

    Debug(DBG_WANCFG_MAIN, ("validating char: %c\n", str[i]));
    if(isalnum(str[i]) == 0){
      snprintf(err_buf, MAX_PATH_LENGTH,
        "Invalid character (%c) found in authentication string!\n", str[i]);
      return err_buf;
    }
  }
*/
  for(unsigned int i = 0; i < strlen(str); i++){

    Debug(DBG_WANCFG_MAIN, ("validating char: %c\n", str[i]));
    if(isspace(str[i]) != 0){
      snprintf(err_buf, MAX_PATH_LENGTH,
        "Invalid character 'blank space' found\nin authentication string!\n");
      return err_buf;
    }
  }

  //validation passed
  return NULL;
}

/*
FE_MEDIA 	= E3		# DS3/E3
FE_LCODE 	= HDB3		# B3ZS for DS3 or HDB3 for E3 or AMI for both
FE_FRAME 	= G.751		# E3 - G.751/G.832 or DS3 - C-BIT/M13
TE3_FRACTIONAL	= NO		# NO/YES+RDEVICE
TE3_RDEVICE	= KENTROX	# ADTRAN/DIGITAL-LINK/KENTROX/LARSCOM/VERILINK
TE3_FCS		= 16		# 16(default) / 32
TE3_RXEQ	= NO		# NO (default) / YES	
TE3_TAOS	= NO		# NO (default) / YES	
TE3_LBMODE	= NO		# NO (default) / YES	
TE3_TXLBO	= NO		# NO (default) / YES
*/

void set_default_e3_configuration(sdla_fe_cfg_t* fe_cfg)
{
  sdla_te3_cfg_t* te3_cfg = &fe_cfg->cfg.te3_cfg;
  sdla_te3_liu_cfg_t*	liu_cfg = &te3_cfg->liu_cfg;

  fe_cfg->media = WAN_MEDIA_E3;
  fe_cfg->lcode = WAN_LCODE_HDB3;
  fe_cfg->frame = WAN_FR_E3_G751;
  fe_cfg->line_no = 1;
  fe_cfg->tx_tristate_mode = WANOPT_NO;
  
  te3_cfg->fractional = WANOPT_NO;
  te3_cfg->rdevice_type = WAN_TE3_RDEVICE_KENTROX;
  te3_cfg->fcs = 16;
  te3_cfg->clock = WAN_NORMAL_CLK;
  liu_cfg->rx_equal = WANOPT_NO; 
  liu_cfg->taos = WANOPT_NO; 
  liu_cfg->lb_mode = WANOPT_NO;
  liu_cfg->tx_lbo = WANOPT_NO;
}

void set_default_t3_configuration(sdla_fe_cfg_t* fe_cfg)
{
  sdla_te3_cfg_t* te3_cfg = &fe_cfg->cfg.te3_cfg;
  sdla_te3_liu_cfg_t*	liu_cfg = &te3_cfg->liu_cfg;

  fe_cfg->media = WAN_MEDIA_DS3;
  fe_cfg->lcode = WAN_LCODE_B3ZS;
  fe_cfg->frame = WAN_FR_DS3_Cbit;
  fe_cfg->line_no = 1;
  fe_cfg->tx_tristate_mode = WANOPT_NO;
  
  te3_cfg->fractional = WANOPT_NO;
  te3_cfg->rdevice_type = WAN_TE3_RDEVICE_KENTROX;
  te3_cfg->fcs = 16;
  te3_cfg->clock = WAN_NORMAL_CLK;
  liu_cfg->rx_equal = WANOPT_NO; 
  liu_cfg->taos = WANOPT_NO; 
  liu_cfg->lb_mode = WANOPT_NO;
  liu_cfg->tx_lbo = WANOPT_NO; 
}

void set_default_t1_configuration(sdla_fe_cfg_t* fe_cfg)
{
  sdla_te_cfg_t* te1_cfg = &fe_cfg->cfg.te_cfg;

  fe_cfg->media = WAN_MEDIA_T1;
  fe_cfg->lcode = WAN_LCODE_B8ZS;
  fe_cfg->frame = WAN_FR_ESF;
  fe_cfg->tx_tristate_mode = WANOPT_NO;

  if(fe_cfg->line_no == 0){
    //set to default '1' only if it is NOT set to a valid value
    fe_cfg->line_no = 1;
  }
	
  te1_cfg->lbo = WAN_T1_LBO_0_DB;
  te1_cfg->te_clock = WAN_NORMAL_CLK;
  te1_cfg->active_ch = ENABLE_ALL_CHANNELS;
  te1_cfg->high_impedance_mode = WANOPT_NO;
  te1_cfg->rx_slevel = 360;
}
/*
EncapMode	= ETH_LLC_OA
ATM_AUTOCFG	= NO
Vci		= 35
Vpi		= 0
ADSL_WATCHDOG	= YES
 
Verbose			= 1
RxBufferCount		= 50
TxBufferCount		= 50
AdslStandard		= ADSL_MULTIMODE
AdslTrellis		= ADSL_TRELLIS_ENABLE
AdslTxPowerAtten		= 0
AdslCodingGain		= ADSL_AUTO_CODING_GAIN
AdslMaxBitsPerBin		= 0x0E
AdslTxStartBin		= 0x06
AdslTxEndBin		= 0x1F
AdslRxStartBin		= 0x20
AdslRxEndBin		= 0xFF
AdslRxBinAdjust		= ADSL_RX_BIN_DISABLE
AdslFramingStruct		= ADSL_FRAMING_TYPE_3
AdslExpandedExchange	= ADSL_EXPANDED_EXCHANGE
AdslClockType		= ADSL_CLOCK_CRYSTAL
AdslMaxDownRate		= 8192
 
TTL 		= 255
IGNORE_FRONT_END  = NO

*/
void set_default_adsl_configuration(wan_adsl_conf_t* adsl_cfg)
{

  Debug(DBG_WANCFG_MAIN, ("set_default_adsl_configuration() ----------------------------\n"));
  
  memset(adsl_cfg, 0x00, sizeof(wan_adsl_conf_t));
  
  adsl_cfg->EncapMode = RFC_MODE_BRIDGED_ETH_LLC;
  adsl_cfg->Vci = 35;
  adsl_cfg->Vpi = 0;
  adsl_cfg->Verbose = 1; 
  adsl_cfg->RxBufferCount = 50;
  adsl_cfg->TxBufferCount = 50;

  adsl_cfg->Standard = WANOPT_ADSL_MULTIMODE;
  adsl_cfg->Trellis = WANOPT_ADSL_TRELLIS_ENABLE;
  adsl_cfg->TxPowerAtten = 0;
  adsl_cfg->CodingGain = WANOPT_ADSL_AUTO_CODING_GAIN;
  adsl_cfg->MaxBitsPerBin = 0x0E;
  adsl_cfg->TxStartBin = 0x06;
  adsl_cfg->TxEndBin = 0x1F;
  adsl_cfg->RxStartBin = 0x20;
  adsl_cfg->RxEndBin = 0xFF;
  adsl_cfg->RxBinAdjust = WANOPT_ADSL_RX_BIN_DISABLE;
  adsl_cfg->FramingStruct = WANOPT_ADSL_FRAMING_TYPE_3;
  adsl_cfg->ExpandedExchange = WANOPT_ADSL_EXPANDED_EXCHANGE;
  adsl_cfg->ClockType = WANOPT_ADSL_CLOCK_CRYSTAL;
  adsl_cfg->MaxDownRate = 8192;

  adsl_cfg->atm_autocfg = WANOPT_NO;
  /*
  unsigned short	  vcivpi_num;	
  wan_adsl_vcivpi_t vcivpi_list[100];	
  unsigned char	  tty_minor;
  unsigned short	  mtu;
  */
  adsl_cfg->atm_watchdog = WANOPT_YES;
}

//creates a string where all digits replaced with alphabetical
//characters.
// i.g.  1->a, 2->b, 3->c ...
char* replace_numeric_with_char(char* str)
{
	unsigned int i, is_num = 0;
	unsigned int original_len = strlen(str);
	static char new_str[1024];
		
	snprintf(new_str, 1024, str);
		
	for(i=0; i < original_len; i++){

		if(isdigit(new_str[i])){
			Debug(DBG_WANCFG_MAIN, ("new_str[%d]: 0x%X (+0x30: 0x%X)\n",
				i, new_str[i], new_str[i]+0x30));
			
			if (!is_num){
				new_str[i] = 'a' + (new_str[i] - '1');
			}else{
				new_str[i] = 'a' + (new_str[i] - '0');
			}
			is_num = 1;
		}else{
			is_num = 0;
		}
	}
	return new_str;
}

// Verify and create directories
int check_directory()
{
  char command_line[MAX_PATH_LENGTH];

  // Verify/Create etc/wanpipe/interfaces directory 
  snprintf(command_line, MAX_PATH_LENGTH, "mkdir -p %s", interfaces_cfg_dir);
  Debug(DBG_WANCFG_MAIN, ("check_directory(): command line: %s\n",
    command_line));
  system(command_line);

  return 0;
}

//Read location of "lxdialog" and other configrable path settings.
int read_wanrouter_rc_file()
{
  wanrouter_rc_file_reader wanrouter_rc_fr(0);

  if(wanrouter_rc_fr.get_setting_value("WAN_BIN_DIR=", wan_bin_dir, MAX_PATH_LENGTH) == NO){
    return NO;
  }

  Debug(DBG_WANCFG_MAIN, ("wan_bin_dir: %s\n", wan_bin_dir));

  replace_new_line_with_zero_term(wan_bin_dir);

  snprintf(lxdialog_path, MAX_PATH_LENGTH, "%s/wanpipe_lxdialog", wan_bin_dir);

  Debug(DBG_WANCFG_MAIN, ("lxdialog_path: %s\n", lxdialog_path));

  return YES;
}

void cleanup()
{
  char command_line[MAX_PATH_LENGTH];

  //remove LXDIALOG_OUTPUT_FILE_NAME file
  snprintf(command_line, MAX_PATH_LENGTH, "rm -rf ");
  snprintf(&command_line[strlen(command_line)], MAX_PATH_LENGTH - strlen(command_line),
		 LXDIALOG_OUTPUT_FILE_NAME);
  system(command_line);

  //remove 'date_and_time_file_name' file
  snprintf(command_line, MAX_PATH_LENGTH, "rm -rf ");
  snprintf(&command_line[strlen(command_line)], MAX_PATH_LENGTH - strlen(command_line),
		 date_and_time_file_name);
  system(command_line);
}


int main(int argc, char *argv[])
{
  int rc = EXIT_SUCCESS;

  Debug(DBG_WANCFG_MAIN, ("%s: main()\n", WANCFG_PROGRAM_NAME));

  if(argc == 2 && !strcmp(argv[1], "version")){
    printf("\nwancfg version: 1.35 (April 28, 2008)\n");
    return EXIT_SUCCESS;
  }

#if defined(ZAPTEL_PARSER)
  //user needs to convert zaptel.conf to wanpipe#.conf file(s)
  if(argc == 2 && !strcmp(argv[1], "zaptel")){
    zaptel_to_wanpipe();
    return EXIT_SUCCESS;
  }
#endif

  //this call must be done BEFORE any dialog created!!
  if(read_wanrouter_rc_file() == NO){
    //intitialize with most common default path
    snprintf(wan_bin_dir, MAX_PATH_LENGTH, "/usr/sbin");
    snprintf(lxdialog_path, MAX_PATH_LENGTH, "%s/wanpipe_lxdialog", wan_bin_dir);

    err_printf("Failed to read 'WAN_BIN_DIR' in 'wanrouter.rc'! Using default: '%s'.",
	wan_bin_dir);
  }
 
  check_directory();

  if(is_console_size_valid() == NO){
    rc = EXIT_FAILURE;
    goto cleanup;
  }

  rc = main_loop();
cleanup:

  if(rc == EXIT_SUCCESS){
#if !(DBG_FLAG)
    //if no errors, clear the screen
    system("clear");
#endif
  }
 
  cleanup();
  
  return rc;
}

int main_loop()
{
  int		selection_index;
  int 		rc = EXIT_SUCCESS;
  char 		exit_main_loop = NO;
  message_box	note_text;
  menu_main_configuration_options main_menu_box(lxdialog_path);

  Debug(DBG_WANCFG_MAIN, ("%s: main_loop()\n", WANCFG_PROGRAM_NAME));

  note_text.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
"Please note:\n\
 1.This program can be used to configure\n\
   Frame Relay, PPP, CISCO HDLC, HDLC Streaming,\n\
   TDM Voice (AFT), TTY and ATM (AFT)\n\
   to run on Sangoma S508/S514/S518(ADSL)/AFT cards.\n\n\
   If you need to configure any other protocols,\n\
   use \"/usr/sbin/wancfg_legacy\" utility.\n\n\
 2.You can exit any of application's dialogs\n\
   by pressing \"Esc\" key. No data will be\n\
   saved to file in this case.");

  do{
    if(main_menu_box.run(&selection_index) == NO){
      exit_main_loop = YES;
      rc = EXIT_FAILURE;
      break;
    }

    switch(selection_index)
    {
    case MENU_BOX_BUTTON_SELECT:
    case MENU_BOX_BUTTON_HELP:
      break;

    case MENU_BOX_BUTTON_EXIT:
      exit_main_loop = YES;
      break;

    default:
      ERR_DBG_OUT(("Unknown selection_index: %d\n", selection_index));
      exit_main_loop = YES;
      rc = EXIT_FAILURE;
    }

  }while(exit_main_loop == NO);

  return rc;
}

#if 0
int main(int argc, char *argv[])
{
  Debug(DBG_WANCFG_MAIN, ("%s: main()\n", WANCFG_PROGRAM_NAME));

  conf_file_reader* cfr;

  //cfr = new conf_file_reader(3);
  //cfr = new conf_file_reader(6);
  //cfr = new conf_file_reader(1);
  //cfr = new conf_file_reader(atoi(argv[1]));
  if(cfr->init() == 0){
    cfr->read_conf_file();
  }

  objects_list::print_obj_list_contents(cfr->main_obj_list);

  delete cfr;

  Debug(DBG_WANCFG_MAIN, ("%s: returning from main()\n", WANCFG_PROGRAM_NAME));
  
  return 0;
}
#endif

#if defined(ZAPTEL_PARSER)
int zaptel_to_wanpipe()
{
  Debug(DBG_WANCFG_MAIN, ("%s: main()\n", WANCFG_PROGRAM_NAME));

  snprintf(zaptel_conf_path, MAX_PATH_LENGTH, "%szaptel.conf", zaptel_default_conf_path);
  zaptel_conf_file_reader  zaptel_cfr(&zaptel_conf_path[0]);

  if(zaptel_cfr.read_file() == 0){
    //zaptel_cfr.printconfig();
    if(zaptel_cfr.create_wanpipe_config() != 0){
      return 1;
    }
  }else{
    printf("Error found in '%s'!\n", zaptel_conf_path);
  }
  
  return 0;
}
#endif

#if 0
int main(int argc, char *argv[])
{
  Debug(DBG_WANCFG_MAIN, ("%s: main()\n", WANCFG_PROGRAM_NAME));
  //sangoma_card_list sang_te1_card_list;
  sangoma_card_list sang_te1_card_list(WANOPT_AFT);
  sangoma_card_list sang_analog_card_list(WANOPT_AFT_ANALOG);

  menu_hardware_probe hw_probe_te1(&sang_te1_card_list);
  hw_probe_te1.hardware_probe();

  menu_hardware_probe hw_probe_analog(&sang_analog_card_list);
  hw_probe_analog.hardware_probe();

  return 0;
}
#endif

