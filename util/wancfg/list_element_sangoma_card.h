/***************************************************************************
                          list_element_sangoma_card.h  -  description
                             -------------------
    begin                : Fri Nov 25 2005
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

#ifndef LIST_ELEMENT_SANGOMA_CARD_H
#define LIST_ELEMENT_SANGOMA_CARD_H

#include "list_element.h"

#define DBG_LIST_ELEMENT_SANGOMA_CARD  1

#include "wancfg.h"

#if defined(__LINUX__)
# include <zapcompat_user.h>
#else
#endif

enum CARD_SUB_VERSION {
	A104=1,	//regular A104
	A104D  	//A104 "Shark" with Echo Canceller
};

/**
  *@author David Rokhvarg
  */
class list_element_sangoma_card : public list_element{

  int rc;
  int key;
  char key_str[MAX_ADDR_STR_LEN];

  int spanno, dchan;

public:

  sdla_fe_cfg_t	fe_cfg;
  sdla_te_cfg_t *sdla_te_cfg_ptr;

  char card_type;
  int card_version, card_sub_version; 
  unsigned int line_no;
  unsigned int PCI_slot_no;
  unsigned int pci_bus_no;
  char S514_CPU_no;

  list_element_sangoma_card(char card_type, 
			    int card_version, 
			    unsigned int line_no,
			    unsigned int PCI_slot_no,
			    unsigned int pci_bus_no,
			    char S514_CPU_no, 
			    int card_sub_version)
  {
    Debug(DBG_LIST_ELEMENT_SANGOMA_CARD, ("list_element_sangoma_card::list_element_sangoma_card()\n"));

    list_element_type = SANGOMA_CARD_LIST_ELEMENT;
    
    this->card_type = card_type;
    this->card_version = card_version; 
    this->card_sub_version = card_sub_version; 
    this->line_no = line_no;
    this->PCI_slot_no = PCI_slot_no;
    this->pci_bus_no = pci_bus_no;
    this->S514_CPU_no = S514_CPU_no;

    memset(&fe_cfg, 0x00, sizeof(sdla_fe_cfg_t));
    sdla_te_cfg_ptr = &fe_cfg.cfg.te_cfg;
    
    spanno = dchan = 0;
  }

  virtual ~list_element_sangoma_card()
  {
    Debug(DBG_LIST_ELEMENT_SANGOMA_CARD, 
	("list_element_sangoma_card::~list_element_sangoma_card(): key:%d\n", key));
    Debug(DBG_LIST_ELEMENT_SANGOMA_CARD, ("card type/version: %s\n",
	get_card_type_string(card_type, card_version)));
  }

  void set_fe_parameters(sdla_fe_cfg_t *new_te1_cfg)
  {
    memcpy(&fe_cfg, new_te1_cfg, sizeof(sdla_fe_cfg_t));
  }

  void set_spanno(int new_spanno)
  {
    spanno = new_spanno;
  }

  int set_dchan(int new_dchan)
  {
    if(spanno == 0){
	//no way to verify this dchan number belongs to this span
    	Debug(DBG_LIST_ELEMENT_SANGOMA_CARD,("%s(): spanno NOT set!!\n", __FUNCTION__));
	return 1;
    }
    dchan = new_dchan;
    return 0;
  }

  int set_key(int new_key)
  {
    key = new_key;
    snprintf(key_str, MAX_ADDR_STR_LEN, "%d", new_key );
    return 0;
  }

  int get_key()
  {
    return key;
  }

  int get_spanno()
  {
    return spanno;
  }

  int get_dchan()
  {
    return dchan;
  }

  void print_card_summary()
  {
    char dchan_str[10];

    switch(card_version)
    {
    case A200_ADPTR_ANALOG:
    case A400_ADPTR_ANALOG:
      	printf("\tSpan: %d, Line: Analog, Law: %s.\n", 
		spanno, (fe_cfg.tdmv_law == ZT_LAW_MULAW ? "MuLaw" : "ALaw"));
      	break;

	case AFT_ADPTR_ISDN:
      	printf("\tSpan: %d, Line: ISDN BRI, Law: %s.\n", 
		spanno, (fe_cfg.tdmv_law == ZT_LAW_MULAW ? "MuLaw" : "ALaw"));
		break;

    case A101_ADPTR_1TE1:
    case A104_ADPTR_4TE1:
	snprintf(dchan_str, 10, "%d", dchan);

	printf("\tSpan: %d, Line: %s, Coding: %-4s, Framing: %-5s, D-channel: %s.\n",
		spanno,

		(fe_cfg.media == WAN_MEDIA_T1 ? "T1" : "E1"),
		(fe_cfg.lcode == WAN_LCODE_AMI ? "AMI" : (fe_cfg.lcode == WAN_LCODE_B8ZS ? "B8ZS" : "HDB3" )),

(fe_cfg.frame == WAN_FR_CRC4  ? "CRC4"  : 
(fe_cfg.frame == WAN_FR_NCRC4 ? "NCRC4" :
(fe_cfg.frame == WAN_FR_ESF   ? "ESF"   : "D4"))),

		(dchan == 0 ? "Not used" : dchan_str )
		);

	printf("\tHardware Echo Cancellation: %-3s\n", (card_sub_version == A104D ? "YES" : "NO"));
      	break;

    default:
	printf("\tInvalid card version: 0x%X\n", card_version);
    }
  }

  int compare(list_element* element)
  {
    Debug(DBG_LIST_ELEMENT_SANGOMA_CARD,
      ("list_element_sangoma_card::compare(list_element_sangoma_card* element)\n"));

    Debug(DBG_LIST_ELEMENT_SANGOMA_CARD, ("this->key: %d, element->key for comparison: %d\n",
      key, ((list_element_sangoma_card*)element)->get_key()));

    if(key < ((list_element_sangoma_card*)element)->get_key()){
      rc = -1;
    }else if(key > ((list_element_sangoma_card*)element)->get_key()){
      rc = 1;
    }else{
      rc = 0;
    }

    Debug(DBG_LIST_ELEMENT_SANGOMA_CARD, ("comparison result %d\n", rc));

    return rc;
  }

  int compare(char* comparator)
  {
    Debug(DBG_LIST_ELEMENT_SANGOMA_CARD, ("list_element_sangoma_card::compare(char* comparator)\n"));
    Debug(DBG_LIST_ELEMENT_SANGOMA_CARD, ("key_str: %s, comparator: %s\n", key_str, comparator));

    return strcmp(key_str, comparator);
  }

  //the comparator is char string in this case
  char* get_comparator()
  {
    Debug(DBG_LIST_ELEMENT_SANGOMA_CARD, ("char* list_element_sangoma_card::get_comparator()\n"));
    return key_str;
  }

  void set_comparator(char* comparator)
  {
    Debug(DBG_LIST_ELEMENT_SANGOMA_CARD, ("void list_element_sangoma_card::set_comparator()\n"));

    snprintf(key_str, MAX_ADDR_STR_LEN, "%s", comparator);
    key = atoi(comparator);
  }
};

#endif

