/***************************************************************************
                          list_element_chan_def.h  -  description
                             -------------------
    begin                : Tue Mar 16 2004
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

#ifndef LIST_ELEMENT_CHAN_DEF_H
#define LIST_ELEMENT_CHAN_DEF_H

#include "list_element.h"

#define DBG_LIST_ELEMENT_CHAN_DEF  0

#include "wancfg.h"

//must be shorter than 'WAN_IFNAME_SZ+1'
#define CHAN_NOT_CONFIGURED "chan_not_cfgd"

#define AUTO_CHAN_CFG "auto"

/**
  *@author David Rokhvarg
  */

class list_element_chan_def : public list_element{

public:

  chan_def_t data;

  ///////////////////////////////
  //each logical channel (on channelized card) can be the "base" of a protocol.
  //e.g.: MP_FR. This will hold configuration of this protocol.
  //void* conf_file_reader;
  
  void* next_objects_list;

  ///////////////////////////////
  list_element_chan_def()
  {
    Debug(DBG_LIST_ELEMENT_CHAN_DEF, ("list_element_chan_def::list_element_chan_def()\n"));

    memset(&data, 0x00, sizeof(data));
    strlcpy(data.name, CHAN_NOT_CONFIGURED, WAN_IFNAME_SZ);

    list_element_type = CHAN_DEF_LIST_ELEMENT;

    data.chanconf = (wanif_conf_t*)malloc(sizeof(wanif_conf_t));
    if(data.chanconf == NULL){
      ERR_DBG_OUT(("Memory allocation failed while creating new\
'list_element_chan_def' object!!\n"));
      exit(1);
    }

    memset(data.chanconf, 0x00, sizeof(wanif_conf_t));

    //set default values for this new Channel
    //MULTICAST
    data.chanconf->mc = 0;
    //inarp;		/* Send Inverse ARP requests Y/N */
    data.chanconf->inarp = 0;
    //inarp_interval;	/* sec, between InARP requests */
    data.chanconf->inarp_interval = 0;
    //unsigned char inarp_rx;		/* Receive Inverse ARP requests Y/N */
    data.chanconf->inarp_rx = 0;
    //unsigned char enable_IPX;	/* Enable or Disable IPX */
    data.chanconf->enable_IPX = 0;
    //IPX Network Addr
    data.chanconf->network_number = 0xABCDEFAB;
    data.chanconf->true_if_encoding = 0;

    data.usedby = WANPIPE;
   
    data.addr = (char*)malloc(MAX_ADDR_STR_LEN);
    if(data.addr == NULL){
      ERR_DBG_OUT(("Memory allocation failed for new channel!!\n"));
      exit(1);
    }else{
      //memset(data.addr, 0x00, MAX_USEDBY_STR_LEN);
      snprintf(data.addr, WAN_ADDRESS_SZ, "1");
    }

    //FIXME: David to set active_ch to ALL for VOICE and 1 for API
    //       For now 99% is VOICE which should have ALL set
    if(	global_card_type == WANOPT_AFT){
			if(	global_card_version == A200_ADPTR_ANALOG ||
					global_card_version == A400_ADPTR_ANALOG ||
					global_card_version == AFT_ADPTR_ISDN){

				snprintf(data.active_channels_string, MAX_LEN_OF_ACTIVE_CHANNELS_STRING, "ALL");
				data.chanconf->active_ch = ENABLE_ALL_CHANNELS;
			}else{
				snprintf(data.active_channels_string, MAX_LEN_OF_ACTIVE_CHANNELS_STRING, "1");
				data.chanconf->active_ch = 1;
			}
    }else{
      snprintf(data.active_channels_string, MAX_LEN_OF_ACTIVE_CHANNELS_STRING, "ALL");
      data.chanconf->active_ch = ENABLE_ALL_CHANNELS;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //init to "ALL" but set to NO
    snprintf(data.active_hwec_channels_string, MAX_LEN_OF_ACTIVE_CHANNELS_STRING, "ALL");
    //data.hwec_flag = WANOPT_NO;
    data.chanconf->xoff_char = WANOPT_NO;//used instead of hwec_flag
    /////////////////////////////////////////////////////////////////////////////////////

    data.chanconf->config_id = PROTOCOL_NOT_SET;

    data.chanconf->hdlc_streaming = WANOPT_YES;
    data.chanconf->mtu = 1500;
    data.chanconf->mc = WANOPT_NO;
    data.chanconf->true_if_encoding = WANOPT_NO;
    data.chanconf->if_down = WANOPT_NO;

    //for AFT?? - it may corrupt default values for other protocols?
    /*
    data.chanconf->u.aft.mtu = 1500;
    data.chanconf->u.aft.mru = 1500;
    data.chanconf->u.aft.idle_flag = 0x7E;
    */
    //data.chanconf->u.aft.tdmv_span_no = 1;//moved to wandev initialization
    
    //for S514, when running HDLC streaming
    data.chanconf->keepalive_tx_tmr = 10000;
    data.chanconf->keepalive_rx_tmr = 11000;
    data.chanconf->keepalive_err_margin = 5;
    data.chanconf->slarp_timer = 0;
    data.chanconf->ignore_dcd = WANOPT_NO;	  //valid only for CHDLC in firmware
    data.chanconf->ignore_cts = WANOPT_NO;	  //valid only for CHDLC in firmware
    data.chanconf->ignore_keepalive = WANOPT_YES; //valid only for CHDLC in firmware
    
    next_objects_list = NULL;
  }

  virtual ~list_element_chan_def()
  {
    Debug(DBG_LIST_ELEMENT_CHAN_DEF, ("list_element_chan_def::~list_element_chan_def()\n"));

    chan_def_t* chandef = &data;

    Debug(1, ("deleting channel: %s\n", chandef->name));	
		
    if (chandef->conf) free(chandef->conf);
    if (chandef->conf_profile) free(chandef->conf_profile);
    if (chandef->addr) free(chandef->addr);
    //if (chandef->usedby) free(chandef->usedby);
    if (chandef->label) free(chandef->label);
    if (chandef->virtual_addr) free(chandef->virtual_addr);
    if (chandef->real_addr) free(chandef->real_addr);

    if (chandef->chanconf){
      free(chandef->chanconf);
    }
    //free(chandef);
  }

  int compare(list_element* element)
  {
    Debug(DBG_LIST_ELEMENT_CHAN_DEF,
      ("list_element_chan_def::compare(list_element_chan_def* element)\n"));
/*
    Debug(DBG_LIST_ELEMENT_CHAN_DEF,
      ("element existing: %s, element for comparison: %s\n",
      data.addr, ((list_element_chan_def*)element)->data.addr));
*/
    //convert to decimal values. string comparisons do not work sometimes
    //because of invisible characters in strings.
    int existing = atoi(remove_spaces_in_int_string(data.addr));
    int new_el = atoi(remove_spaces_in_int_string(((list_element_chan_def*)element)->data.addr));
    int rc;

    Debug(DBG_LIST_ELEMENT_CHAN_DEF,("DEC element existing: %d, DEC element for comparison: %d\n",
      existing, new_el));

    if(existing  < new_el){
      rc = -1;
    }else if(existing  > new_el){
      rc = 1;
    }else{
      rc = 0;
    }

    Debug(DBG_LIST_ELEMENT_CHAN_DEF, ("comparison result %d\n", rc));

    return rc;
  }

  int compare(char* comparator)
  {
    Debug(DBG_LIST_ELEMENT_CHAN_DEF, ("list_element_chan_def::compare(char* comparator)\n"));

    Debug(DBG_LIST_ELEMENT_CHAN_DEF, ("data.addr: %s, comparator: %s\n", data.addr, comparator));
    return strcmp(data.addr, comparator);
  }

  //the comparator is char string in this case
  char* get_comparator()
  {
    Debug(DBG_LIST_ELEMENT_CHAN_DEF, ("char* list_element_chan_def::get_comparator()\n"));

    return data.addr;
  }

  void set_comparator(char* comparator)
  {
    Debug(DBG_LIST_ELEMENT_CHAN_DEF, ("void list_element_chan_def::set_comparator()\n"));

    data.addr = strdup(comparator);
  }

/*
  int compare(int comparator)
  {
    Debug(DBG_LIST_ELEMENT_CHAN_DEF,
      ("list_element_chan_def::compare(int comparator)\n"));

    Debug(DBG_LIST_ELEMENT_CHAN_DEF, ("data.addr: %s, comparator: %s", data.addr, comparator));

    Debug(DBG_LIST_ELEMENT_CHAN_DEF,
      ("data.addr: %d, comparator: %d", atoi(data.addr), comparator));

    return (atoi(data.addr) == comparator ? 0 : (atoi(data.addr) < comparator ? -1 : 1));
  }
*/
/*
  //the comparator is char string in this case
  int get_comparator()
  {
    Debug(DBG_LIST_ELEMENT_CHAN_DEF, ("int list_element_chan_def::get_comparator()\n"));

    return atoi(data.addr);
  }
*/
};

#endif

