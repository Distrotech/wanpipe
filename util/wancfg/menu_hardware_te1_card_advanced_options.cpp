/***************************************************************************
		 menu_hardware_te1_card_advanced_options.cpp  -  description
							 -------------------
	begin				 : Thu Apr 1 2004
	copyright			 : (C) 2004 by David Rokhvarg
	email				 : davidr@sangoma.com
 ***************************************************************************/

/***************************************************************************
 *																		   *
 *	 This program is free software; you can redistribute it and/or modify  *
 *	 it under the terms of the GNU General Public License as published by  *
 *	 the Free Software Foundation; either version 2 of the License, or	   *
 *	 (at your option) any later version.								   *
 *																		   *
 ***************************************************************************/

#include "menu_hardware_te1_card_advanced_options.h"
#include "text_box.h"
#include "text_box_yes_no.h"
#include "menu_te1_select_media.h"
#include "menu_te_select_line_decoding.h"
#include "menu_te_select_framing.h"
#include "menu_t1_lbo.h"
#include "menu_e1_lbo.h"
#include "input_box_active_channels.h"
#include "menu_te1_clock_mode.h"

char* active_te1_channels_help_str =
"\n"
"BOUND T1/E1 ACTIVE CHANNELS\n"
"\n"
"Please specify T1/E1 channles to be used.\n"
"\n"
"You can use the follow format:\n"
"o ALL     - for full T1/E1 links\n"
"o x.a-b.w - where the letters are numbers; to specify\n"
"            the channels x and w and the range of\n"
"            channels a-b, inclusive for fractional\n"
"            links.\n"
" Example:\n"
"o ALL     - All channels will be selcted\n"
"o 1       - A first channel will be selected\n"
"o 1.3-8.12 - Channels 1,3,4,5,6,7,8,12 will be selected.\n"
"\n";

char* te1_options_help_str =
"T1/E1 LINE DECODING\n"
"\n"
"for T1:\n"
" AMI or B8ZS\n"
"for E1:\n"
" AMI or HDB3\n"
"\n"
"T1/E1 FRAMIING MODE\n"
"\n"
"for T1:\n"
" D4 or ESF or UNFRAMED\n"
"for E1:\n"
" non-CRC4 or CRC4 or UNFRAMED\n"
"\n"
"T1/E1 CLOCK MODE\n"
"\n"
" Normal - normal clock mode (default)\n"
" Master - master clock mode\n"
"\n"
"T1 LINE BUILD OUT\n"
"\n"
"for Long Haul\n"
" 1.  CSU: 0dB\n"
" 2.  CSU: 7.5dB\n"
" 3.  CSU: 15dB\n"
" 4.  CSU: 22.5dB\n"
"for Short Haul\n"
" 5.  DSX: 0-110ft\n"
" 6.  DSX: 110-220ft\n"
" 7.  DSX: 220-330ft\n"
" 8.  DSX: 330-440ft\n"
" 9.  DSX: 440-550ft\n"
" 10. DSX: 550-660ft\n"
"\n\n"
"Reference Clock Port:\n"
"or \"A104/2 TE1 Clock Synchronization\"\n"
"\n"
"A104 Supported from Release: beta7-2.3.3 or greater\n"
"Firmware Version: V.12\n"
"A102 Supported from Release: beta7-2.3.2 or greater.\n"
"Firmware Version: V.24\n"
"\n"		
"TE1 Clock synchronization is used to propagate\n"
"a single clock source throughout the\n"
"TE1 network.\n"
"\n"
"wanpipe1->Port A-> Normal Mode:\n"
"	Receives T1 Clock from Telco\n"
"wanpipe2->Port B-> Master Mode:\n"
"	Transmits the SAME T1\n" 
"Clock as a master source.\n"
"\n"	
"In order to enable this option on wanpipe2:\n"
"Set the Reference Clock Port to a non zero\n"
"value which is the port number\n"
"that is supplying to clock.\n"
"Range: 0   - dislable\n"
"       1-4 - supported ports\n"
"	For A102: 1=PortA\n"
"                 2=PortB\n"
"\n"
"\n"		
"Note: That TE_CLOCK must be set as MASTER\n"
"      The Clock source is the Port 1 LINE.\n"
"\n"
"IMPORTANT: When start and stopping devices\n"
"START:  Master(wanpipe1) then Slave(wanpipe2)\n"
"STOP:   Slave(wanpipe2) then Master(wanpipe1)\n"
"\n"			  
"This is done automatically by the wanrouter\n"
"startup script.\n" 
"i.e: wanrouter stop: will stop devices in\n" 
"inverse order.\n"
"\n"			 
"IMPORTANT:\n"
"	If the wanpipe1 device is stopped, the\n"
"	wanpipe2 device, that is using the clock\n"
"	from wanpipe1, will loose its clock thus\n"
"	resulting in unpredictable operation.\n";

char* bri_options_help_str =
"BRI Master Clock Port:\n"
"YES -	this port is MASTER clock for all OTHER\n"
"	ports on this A500 card\n"
"NO -	this port is in NORMAL clock mode\n"
"\n"			 
"IMPORTANT:\n"
"	If MASTER port is in disconnected state or\n"
"	stopped, all ports on the A500 card will\n"
"	run on internal card's clock.\n";


enum TE1_ADVANCED_OPTIONS{
	TE1_MEDIA=1,
	TE1_LCODE,
	TE1_FRAME,
	T1_LBO,
	E1_LBO,
	TE1_CLOCK,
	TE1_ACTIVE_CH,
	AFT_FE_TXTRISTATE,
	TE_REF_CLOCK,
	E1_SIG_MODE,
	TE_HIGHIMPEDANCE,
	BRI_CLOCK_MASTER,
	TE_RX_SLEVEL,
	RM_NETWORK_SYNC
};


////////////////////////////////////////////////////////////////////////////////////
/*
TE_RX_SLEVEL:  Receiver Sensitivity (Max Cable Loss Allowed) (dB)
The user will have the following option to choose:
1. Hi-Imedance mode:
30 dB
22.5 dB
17.5 dB
12 dB

Depending on selection, write the following values into configuration files:
300
225
175
120

2. Normal mode:
12 dB
18 dB
30 dB
36 dB (for T1) 
43 dB (for E1)

Depending on selection, write the following values into configuration files:
120
180
300
360
430
*/
typedef struct _te_rx_slevel
{
	const char* name;
	unsigned int value;
}te_rx_slevel_t;

const te_rx_slevel_t high_impedance_rx_slevel[] = {
	{"30 dB",	300},
	{"22.5 dB",	225},
	{"17.5 dB",	175},
	{"12 dB",	120},
	{NULL,		0}
};

const te_rx_slevel_t t1_normal_mode_rx_slevel[] = {
	{"12 dB",	120},
	{"18 dB",	180},
	{"30 dB",	300},
	{"36 dB",	360},
	{NULL,		0}
};

const te_rx_slevel_t e1_normal_mode_rx_slevel[] = {
	{"12 dB",	120},
	{"18 dB",	180},
	{"30 dB",	300},
	{"43 dB",	430},
	{NULL,		0}
};
////////////////////////////////////////////////////////////////////////////////////


#define DBG_MENU_HARDWARE_TE1_CARD_ADVANCED_OPTIONS 1

menu_hardware_te1_card_advanced_options::
	menu_hardware_te1_card_advanced_options(  IN char * lxdialog_path,
											IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_HARDWARE_TE1_CARD_ADVANCED_OPTIONS,
	("menu_hardware_te1_card_advanced_options::menu_hardware_te1_card_advanced_options()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_hardware_te1_card_advanced_options::~menu_hardware_te1_card_advanced_options()
{
  Debug(DBG_MENU_HARDWARE_TE1_CARD_ADVANCED_OPTIONS,
	("menu_hardware_te1_card_advanced_options::~menu_hardware_te1_card_advanced_options()\n"));
}

int menu_hardware_te1_card_advanced_options::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int option_selected;
  char exit_dialog;
  int number_of_items, n, rx_slevel_ind;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;
  sdla_te_cfg_t*  te_cfg;
  const te_rx_slevel_t *te_rx_slevel_ptr;
  sdla_fe_cfg_t		*sdla_fe_cfg;

  input_box_active_channels act_channels_ip;

  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility");

  Debug(DBG_MENU_HARDWARE_TE1_CARD_ADVANCED_OPTIONS, ("menu_net_interface_setup::run()\n"));

again:
  rc = YES;
  option_selected = 0;
  exit_dialog = NO;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;
  te_cfg = &linkconf->fe_cfg.cfg.te_cfg;
  sdla_fe_cfg = &linkconf->fe_cfg;

  Debug(DBG_MENU_HARDWARE_TE1_CARD_ADVANCED_OPTIONS,
	("cfr->link_defs->name: %s\n", link_def->name));

	if(te_cfg->high_impedance_mode){
		te_rx_slevel_ptr = &high_impedance_rx_slevel[0];
	}else{
		if(	linkconf->fe_cfg.media == WAN_MEDIA_T1){
			te_rx_slevel_ptr = &t1_normal_mode_rx_slevel[0];
		}else{
			te_rx_slevel_ptr = &e1_normal_mode_rx_slevel[0];
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////
	//find index of line in menue using current rx_slevel
	n=0;rx_slevel_ind=0;
	while(te_rx_slevel_ptr[n].name != NULL){
		if(te_rx_slevel_ptr[n].value == (unsigned int)te_cfg->rx_slevel){
			rx_slevel_ind = n; 
			break;
		}
		n++;
	}

	/////////////////////////////////////////////////////////////////////////////////////
  if(linkconf->fe_cfg.media == WAN_MEDIA_T1){
		number_of_items = 7;
  }else if(linkconf->fe_cfg.media == WAN_MEDIA_E1){
		number_of_items = 7;
  }else if(linkconf->fe_cfg.media == WAN_MEDIA_BRI){
		number_of_items = 1;
  }else{
		ERR_DBG_OUT(("Unknown Media Type!! te_cfg.media: 0x%X\n", linkconf->fe_cfg.media));
		return NO;
  }

  menu_str = "";

  if(linkconf->fe_cfg.media == WAN_MEDIA_BRI){

		//////////////////////////////////////////////////////////////////////////////////////
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", RM_NETWORK_SYNC);
		menu_str += tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"External Network Sync--> %s\" ",
			(sdla_fe_cfg->network_sync == WANOPT_YES ? "Yes" : "No"));
		menu_str += tmp_buff;
		number_of_items++;

		//////////////////////////////////////////////////////////////////////////////////////
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", BRI_CLOCK_MASTER);
		menu_str += tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"BRI Master Clock Port--> %s\" ",
				(linkconf->fe_cfg.cfg.bri.clock_mode == WANOPT_YES ? "Yes":"No"));
		menu_str += tmp_buff;

  }else{
		//////////////////////////////////////////////////////////////////////////////////////
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE1_MEDIA);
		menu_str += tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Physical Medium--> %s\" ",
					MEDIA_DECODE(&linkconf->fe_cfg));
		menu_str += tmp_buff;

		//////////////////////////////////////////////////////////////////////////////////////

		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE1_LCODE);
		menu_str += tmp_buff;

		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Line decoding----> %s\" ", 
					LCODE_DECODE(&linkconf->fe_cfg));
		menu_str += tmp_buff;

		//////////////////////////////////////////////////////////////////////////////////////
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE1_FRAME);
		menu_str += tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Framing----------> %s\" ", 
					FRAME_DECODE(&linkconf->fe_cfg));
		menu_str += tmp_buff;

		//////////////////////////////////////////////////////////////////////////////////////
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE1_CLOCK);
		menu_str += tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"TE clock mode----> %s\" ",
					TECLK_DECODE(&linkconf->fe_cfg));
#if 0
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"TE clock mode----> %s\" ",
			(linkconf->fe_cfg.cfg.te_cfg.te_clock == WANOPT_NORMAL_CLK ? "Normal" : "Master"));
#endif
		menu_str += tmp_buff;
		//////////////////////////////////////////////////////////////////////////////////////
  }

  //on AFT it must be 'ALL' because channelization is done on per-group-of-channels basis.
  //on S514-cards it is configurable here
  if(linkconf->card_type == WANOPT_S51X){
	
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE1_ACTIVE_CH);
		menu_str += tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Act. channels----> %s\" ",
			link_def->active_channels_string);
		menu_str += tmp_buff;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  if(linkconf->fe_cfg.media == WAN_MEDIA_T1){
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", T1_LBO);
		menu_str += tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"LBO--------------> %s\" ", 
				LBO_DECODE(&linkconf->fe_cfg));
		menu_str += tmp_buff;
		}else if(linkconf->fe_cfg.media == WAN_MEDIA_E1){
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", E1_LBO);
		menu_str += tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"LBO--------------> %s\" ", 
				LBO_DECODE(&linkconf->fe_cfg));
		menu_str += tmp_buff;

		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", E1_SIG_MODE);
		menu_str += tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Signalling Mode--> %s\" ", 
			(FE_SIG_MODE(&linkconf->fe_cfg) == WAN_TE1_SIG_CCS ? "CCS":"CAS"));
		menu_str += tmp_buff;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  if(linkconf->fe_cfg.media != WAN_MEDIA_BRI &&
	  (linkconf->card_type == WANOPT_AFT || linkconf->card_type == WANOPT_AFT104)){

		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AFT_FE_TXTRISTATE);
		menu_str += tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Disable Transmitter---> %s\" ",
			(linkconf->fe_cfg.tx_tristate_mode == WANOPT_YES ? "YES" : "NO"));
		menu_str += tmp_buff;

		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE_REF_CLOCK);
		menu_str += tmp_buff;
		if(te_cfg->te_ref_clock == 0){
		  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Reference Clock Port--> %s\" ",  "Not Used");
		}else{
			snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Reference Clock Port--> %d\" ", 
			te_cfg->te_ref_clock);
		}
		menu_str += tmp_buff;
  }

  if(linkconf->fe_cfg.media != WAN_MEDIA_BRI){
		//////////////////////////////////////////////////////////////////////////////////////
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE_HIGHIMPEDANCE);
		menu_str += tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"High Impedance----------> %s\" ", 
				(te_cfg->high_impedance_mode == WANOPT_YES ? "YES" : "NO"));
		menu_str += tmp_buff;

		//////////////////////////////////////////////////////////////////////////////////////
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TE_RX_SLEVEL);
		menu_str += tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Rx Sensetivity Level----> %s\" ", 
				te_rx_slevel_ptr[rx_slevel_ind].name);
		menu_str += tmp_buff;
  }

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nAdvanced Hardware settings for: %s.", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
						  MENU_BOX_BACK,//MENU_BOX_SELECT,
						  lxdialog_path,
						  "T1/E1 ADVANCED CONFIGURATION OPTIONS",
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
	Debug(DBG_MENU_HARDWARE_TE1_CARD_ADVANCED_OPTIONS,
	  ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

	switch(atoi(get_lxdialog_output_string()))
	{
	case TE1_MEDIA:
	  {
		menu_te1_select_media te1_select_media(lxdialog_path, cfr);
		if(te1_select_media.run(selection_index) == NO){
		  rc = NO;
		  exit_dialog = YES;
		}
	  }
	  break;

	case TE1_LCODE:
	  {
		menu_te_select_line_decoding te1_select_line_decoding(lxdialog_path, cfr);
		if(te1_select_line_decoding.run(selection_index) == NO){
		  rc = NO;
		  exit_dialog = YES;
		}
	  }
	  break;

	case TE1_FRAME:
	  {
		menu_te_select_framing te1_select_framing(lxdialog_path, cfr);
		if(te1_select_framing.run(selection_index) == NO){
		  rc = NO;
		  exit_dialog = YES;
		}
	  }
	  break;

	case T1_LBO:
	  {
		menu_t1_lbo t1_lbo(lxdialog_path, cfr);
		if(t1_lbo.run(selection_index) == NO){
		  rc = NO;
		  exit_dialog = YES;
		}
	  }
	  break;

	case E1_LBO:
	  {
		menu_e1_lbo e1_lbo(lxdialog_path, cfr);
		if(e1_lbo.run(selection_index) == NO){
		  rc = NO;
		  exit_dialog = YES;
		}
	  }
	  break;

	case E1_SIG_MODE:
	  {
		menu_e1_signalling_mode e1_signalling_mode(lxdialog_path, cfr);
		if(e1_signalling_mode.run(selection_index) == NO){
		  rc = NO;
		  exit_dialog = YES;
		}
	  }
	  break;

	case TE1_CLOCK:
	  {
		menu_te1_clock_mode te1_clock_mode(lxdialog_path, cfr);
		if(te1_clock_mode.run(selection_index) == NO){
		  rc = NO;
		  exit_dialog = YES;
		}
	  }
	  break;

	case TE_HIGHIMPEDANCE:
	  snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s High Impedance mode?",
			(te_cfg->high_impedance_mode == WANOPT_NO ? "Enable" : "Disable"));

	  if(yes_no_question(	selection_index,
							lxdialog_path,
							NO_PROTOCOL_NEEDED,
							tmp_buff) == NO){
		return NO;
	  }

	  switch(*selection_index)
	  {
	  case YES_NO_TEXT_BOX_BUTTON_YES:
		if(te_cfg->high_impedance_mode == WANOPT_NO){
		  //was disabled - enable
		  te_cfg->high_impedance_mode = WANOPT_YES;
		}else{
		  //was enabled - disable
		  te_cfg->high_impedance_mode = WANOPT_NO;
		}
		break;
	  }
	  break;
#if 1
	case TE_RX_SLEVEL:
	  {
		menu_rx_slevel rx_slevel(lxdialog_path, cfr);
		if(rx_slevel.run(selection_index) == NO){
		  rc = NO;
		  exit_dialog = YES;
		}
	  }
	  break;
#endif

	case RM_NETWORK_SYNC:
		snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s External Network Sync?",
		(sdla_fe_cfg->network_sync == WANOPT_NO ? "Enable" : "Disable"));

		if(yes_no_question( selection_index,
				lxdialog_path,
				NO_PROTOCOL_NEEDED,
				tmp_buff) == NO){
			return NO;
		}
		
		switch(*selection_index)
		{
		case YES_NO_TEXT_BOX_BUTTON_YES:
			if(sdla_fe_cfg->network_sync == WANOPT_NO){
				//was disabled - enable
				sdla_fe_cfg->network_sync = WANOPT_YES;
			}else{
				//was enabled - disable
				sdla_fe_cfg->network_sync = WANOPT_NO;
			}
			break;
		}
		break;

	case BRI_CLOCK_MASTER:
		if(linkconf->fe_cfg.cfg.bri.clock_mode == WANOPT_YES){
			//It is currently Master, ask if user wants to change Normal mode
			snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to use this line in Normal clock mode?");
		}else{
			//It is currently in Normal mode, ask if user wants to change Master mode
			snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to use this line in Normal clock mode?");
		}

	  if(yes_no_question(	selection_index,
							lxdialog_path,
							NO_PROTOCOL_NEEDED,
							tmp_buff) == NO){
			return NO;
	  }

	  switch(*selection_index)
	  {
	  case YES_NO_TEXT_BOX_BUTTON_YES:
			if(linkconf->fe_cfg.cfg.bri.clock_mode == WANOPT_NO){
				//was Normal - change to Master
				linkconf->fe_cfg.cfg.bri.clock_mode = WANOPT_YES;
			}else{
				//was Master - change to Normal
				linkconf->fe_cfg.cfg.bri.clock_mode = WANOPT_NO;
			}
			break;
	  }
		break;

	case TE_REF_CLOCK:
	  {
	unsigned int ref_clock_port;

show_ref_clock_input_box:
		snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify Reference Clock Port (0 to disable).\
If this option is used, TE1 Clock MUST be set to Master!");
		snprintf(initial_text, MAX_PATH_LENGTH, "%d", te_cfg->te_ref_clock);

		inb.set_configuration(lxdialog_path,
							  backtitle,
							  explanation_text,
							  INPUT_BOX_HIGTH,
							  INPUT_BOX_WIDTH,
							  initial_text);

		inb.show(selection_index);

		switch(*selection_index)
		{
		case INPUT_BOX_BUTTON_OK:
		  ref_clock_port = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

		  if(ref_clock_port < 0 || ref_clock_port > 4){
			tb.show_error_message(lxdialog_path, WANCONFIG_AFT, "Invalid Reference Clock Port!");
			goto show_ref_clock_input_box;
		  }else{
			te_cfg->te_ref_clock = ref_clock_port;
		  }
		  break;

		case INPUT_BOX_BUTTON_HELP:
		  tb.show_help_message(lxdialog_path, WANCONFIG_AFT, te1_options_help_str);
		  goto show_ref_clock_input_box;
		}//switch(*selection_index)
	  }
	  break;

	case TE1_ACTIVE_CH:
	  Debug(DBG_MENU_HARDWARE_TE1_CARD_ADVANCED_OPTIONS,
		("cfr->link_defs->linkconf->config_id: %d\n", cfr->link_defs->linkconf->config_id));
	  if(cfr->link_defs->linkconf->config_id == WANCONFIG_AFT){
		//must be alwayse ALL channels
		break;
	  }

	  if(act_channels_ip.show(	lxdialog_path,
								linkconf->config_id,
								link_def->active_channels_string,//initial text
								selection_index,
								linkconf->fe_cfg.media) == NO){
		rc = NO;
	  }else{

		switch(*selection_index)
		{
		case INPUT_BOX_BUTTON_OK:
		  snprintf(link_def->active_channels_string, MAX_LEN_OF_ACTIVE_CHANNELS_STRING,
				act_channels_ip.get_lxdialog_output_string());

		  break;

		case INPUT_BOX_BUTTON_HELP:
		  tb.show_help_message(lxdialog_path, WANCONFIG_AFT, active_te1_channels_help_str);
		  break;
		}
		goto again;
	  }
	  break;

	case AFT_FE_TXTRISTATE:
	  snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s transmitter?",
	(linkconf->fe_cfg.tx_tristate_mode == WANOPT_NO ? "Disable" : "Enable"));

	  if(yes_no_question( selection_index,
						  lxdialog_path,
						  NO_PROTOCOL_NEEDED,
						  tmp_buff) == NO){
	//error displaying dialog
	rc = NO;
	goto cleanup;
	  }

	  switch(*selection_index)
	  {
	  case YES_NO_TEXT_BOX_BUTTON_YES:
	if(linkconf->fe_cfg.tx_tristate_mode == WANOPT_NO){
	  //transmitter enabled, user wants to disable
	  linkconf->fe_cfg.tx_tristate_mode = WANOPT_YES;
	}else{
	  linkconf->fe_cfg.tx_tristate_mode = WANOPT_NO;
	}
		break;

	  case YES_NO_TEXT_BOX_BUTTON_NO:
	//don't do anything
	break;
	  }
	  break;

	default:
	  ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
		get_lxdialog_output_string()));
	  rc = NO;
	  exit_dialog = YES;
	}
	break;

  case MENU_BOX_BUTTON_HELP:
	if(linkconf->fe_cfg.media == WAN_MEDIA_BRI){
	  tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, bri_options_help_str);
	}else{
	  tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, te1_options_help_str);
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


//////////////////////////////////////////////////////////////////////////////////////////////
//							ANALOG
//////////////////////////////////////////////////////////////////////////////////////////////
#define DBG_MENU_HARDWARE_ANALOG_CARD_ADVANCED_OPTIONS 1

enum {
	RM_BATTTHRESH=1,
	RM_BATTDEBOUNCE,
};

menu_hardware_analog_card_advanced_options::
	menu_hardware_analog_card_advanced_options(  IN char * lxdialog_path,
											IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_HARDWARE_ANALOG_CARD_ADVANCED_OPTIONS,
	("menu_hardware_analog_card_advanced_options::menu_hardware_analog_card_advanced_options()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_hardware_analog_card_advanced_options::~menu_hardware_analog_card_advanced_options()
{
  Debug(DBG_MENU_HARDWARE_ANALOG_CARD_ADVANCED_OPTIONS,
	("menu_hardware_analog_card_advanced_options::~menu_hardware_analog_card_advanced_options()\n"));
}

int menu_hardware_analog_card_advanced_options::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  unsigned int option_selected;
  char exit_dialog;
  int number_of_items=0;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;
  sdla_remora_cfg_t	*remora_cfg;
  sdla_fe_cfg_t		*sdla_fe_cfg;

  input_box_active_channels act_channels_ip;

  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];

  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility");

  Debug(DBG_MENU_HARDWARE_ANALOG_CARD_ADVANCED_OPTIONS, ("menu_net_interface_setup::%s()\n", __FUNCTION__));

again:
  rc = YES;
  option_selected = 0;
  exit_dialog = NO;
  number_of_items=0;

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  sdla_fe_cfg = &linkconf->fe_cfg;
  remora_cfg = &linkconf->fe_cfg.cfg.remora;

  Debug(DBG_MENU_HARDWARE_ANALOG_CARD_ADVANCED_OPTIONS,
	("cfr->link_defs->name: %s\n", link_def->name));

  menu_str = "";

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", RM_BATTTHRESH);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Battery Threshold--> %d\" ", 
    	remora_cfg->battthresh);
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", RM_BATTDEBOUNCE);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"Battery Debounce---> %d\" ", 
    	remora_cfg->battdebounce);
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", RM_NETWORK_SYNC);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"External Network Sync--> %s\" ",
	(sdla_fe_cfg->network_sync == WANOPT_YES ? "Yes" : "No"));
  menu_str += tmp_buff;
  number_of_items++;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nAdvanced Hardware settings for: %s.", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
						  MENU_BOX_BACK,//MENU_BOX_SELECT,
						  lxdialog_path,
						  "ANALOG ADVANCED CONFIGURATION OPTIONS",
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
	Debug(DBG_MENU_HARDWARE_ANALOG_CARD_ADVANCED_OPTIONS,
	  ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

	switch(atoi(get_lxdialog_output_string()))
	{
	case RM_BATTTHRESH:
		unsigned int battery_threshold;

show_RM_BATTTHRESH_input_box:

		snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify Battery Threshold value (minimum is 1).");
		snprintf(initial_text, MAX_PATH_LENGTH, "%d", remora_cfg->battthresh);

		inb.set_configuration(lxdialog_path,
							  backtitle,
							  explanation_text,
							  INPUT_BOX_HIGTH,
							  INPUT_BOX_WIDTH,
							  initial_text);

		inb.show(selection_index);

		switch(*selection_index)
		{
		case INPUT_BOX_BUTTON_OK:
		  battery_threshold = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

		  if(battery_threshold < 1){
			tb.show_error_message(lxdialog_path, WANCONFIG_AFT, "Invalid Battery Threshold!");
			goto show_RM_BATTTHRESH_input_box;
		  }else{
			remora_cfg->battthresh = battery_threshold;
		  }
		  break;

		case INPUT_BOX_BUTTON_HELP:
		  tb.show_help_message(lxdialog_path, WANCONFIG_AFT, option_not_implemented_yet_help_str);
		  goto show_RM_BATTTHRESH_input_box;
		}//switch(*selection_index)
		break;

	case RM_BATTDEBOUNCE:
		unsigned int battdebounce;

show_RM_BATTDEBOUNCE_input_box:

		snprintf(explanation_text, MAX_PATH_LENGTH, "Please specify Battery Debounce value (minimum is 1).");
		snprintf(initial_text, MAX_PATH_LENGTH, "%d", remora_cfg->battdebounce);

		inb.set_configuration(lxdialog_path,
							  backtitle,
							  explanation_text,
							  INPUT_BOX_HIGTH,
							  INPUT_BOX_WIDTH,
							  initial_text);

		inb.show(selection_index);

		switch(*selection_index)
		{
		case INPUT_BOX_BUTTON_OK:
		  battdebounce = atoi(remove_spaces_in_int_string(inb.get_lxdialog_output_string()));

		  if(battdebounce < 1){
			tb.show_error_message(lxdialog_path, WANCONFIG_AFT, "Invalid Battery Debounce!");
			goto show_RM_BATTDEBOUNCE_input_box;
		  }else{
			remora_cfg->battdebounce = battdebounce;
		  }
		  break;

		case INPUT_BOX_BUTTON_HELP:
		  tb.show_help_message(lxdialog_path, WANCONFIG_AFT, option_not_implemented_yet_help_str);
		  goto show_RM_BATTDEBOUNCE_input_box;
		}//switch(*selection_index)
		break;

	case RM_NETWORK_SYNC:
		snprintf(tmp_buff, MAX_PATH_LENGTH, "Do you want to %s External Network Sync?",
		(sdla_fe_cfg->network_sync == WANOPT_NO ? "Enable" : "Disable"));

		if(yes_no_question( selection_index,
				lxdialog_path,
				NO_PROTOCOL_NEEDED,
				tmp_buff) == NO){
			return NO;
		}
		
		switch(*selection_index)
		{
		case YES_NO_TEXT_BOX_BUTTON_YES:
			if(sdla_fe_cfg->network_sync == WANOPT_NO){
				//was disabled - enable
				sdla_fe_cfg->network_sync = WANOPT_YES;
			}else{
				//was enabled - disable
				sdla_fe_cfg->network_sync = WANOPT_NO;
			}
			break;
		}
		break;



	default:
	  ERR_DBG_OUT(("Invalid option selected for editing!! selection: %s\n",
		get_lxdialog_output_string()));
	  rc = NO;
	  exit_dialog = YES;
	}//switch(atoi(get_lxdialog_output_string()))
	break;

  case MENU_BOX_BUTTON_HELP:
	if(linkconf->fe_cfg.media == WAN_MEDIA_BRI){
	  tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, bri_options_help_str);
	}else{
	  tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, te1_options_help_str);
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

#if 1
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//RX SLEVEL
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DBG_MENU_RX_SLEVEL 1

menu_rx_slevel::menu_rx_slevel(IN char * lxdialog_path, IN conf_file_reader* ptr_cfr)
{
  Debug(DBG_MENU_RX_SLEVEL, ("menu_rx_slevel::%s()\n", __FUNCTION__));
  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->cfr = ptr_cfr;
}

menu_rx_slevel::~menu_rx_slevel()
{
  Debug(DBG_MENU_RX_SLEVEL, ("menu_rx_slevel::~%s()\n", __FUNCTION__));
}

int menu_rx_slevel::run(OUT int * selection_index)
{
  string menu_str;
  int rc;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog;
  int number_of_items, n;
  unsigned char media;

  //help text box
  text_box tb;

  link_def_t * link_def;
  wandev_conf_t *linkconf;
	const te_rx_slevel_t *te_rx_slevel_ptr;

  Debug(DBG_MENU_RX_SLEVEL, ("menu_rx_slevel::%s()\n", __FUNCTION__));

again:

  link_def = cfr->link_defs;
  linkconf = cfr->link_defs->linkconf;

  Debug(DBG_MENU_RX_SLEVEL, ("cfr->link_defs->name: %s\n", link_def->name));

  rc = YES;
  exit_dialog = NO;
  media = linkconf->fe_cfg.media;

	if(linkconf->fe_cfg.cfg.te_cfg.high_impedance_mode){
		te_rx_slevel_ptr = &high_impedance_rx_slevel[0];
	}else{
		if(	media == WAN_MEDIA_T1){
			te_rx_slevel_ptr = &t1_normal_mode_rx_slevel[0];
		}else{
			te_rx_slevel_ptr = &e1_normal_mode_rx_slevel[0];
		}
	}


  Debug(DBG_MENU_RX_SLEVEL, ("linkconf->card_type: DEC:%d, HEX: 0x%X\n",
    linkconf->card_type, linkconf->card_type));

  number_of_items = 0;
  menu_str = " ";
/*
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", WAN_E1_75);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"75 OH\" ");
  menu_str += tmp_buff;
*/
	//add the lines to the menue
	n=0;
	while(te_rx_slevel_ptr[n].name != NULL){
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", te_rx_slevel_ptr[n].value);
		menu_str += (char*)tmp_buff;
		snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%s\" ", te_rx_slevel_ptr[n].name);
		menu_str += tmp_buff;
		number_of_items++;
		n++;
	}

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\nSelect Receiver Sensitivity for Wan Device: %s", link_def->name);

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_BACK,//MENU_BOX_SELECT,
                          lxdialog_path,
                          "SELECT RX SENSETIVITY LEVEL",
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
    Debug(DBG_MENU_RX_SLEVEL,
      ("hardware_setup: option selected for editing: %s\n", get_lxdialog_output_string()));

    linkconf->fe_cfg.cfg.te_cfg.rx_slevel = atoi(get_lxdialog_output_string());
    exit_dialog = YES;
    break;

  case MENU_BOX_BUTTON_HELP:

    switch(atoi(get_lxdialog_output_string()))
    {
    default:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED,
        option_not_implemented_yet_help_str);
      break;
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
#endif
