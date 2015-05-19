/***************************************************************************
                          menu_adsl_advanced_cfg.cpp  -  description
                             -------------------
    begin                : Mon Mar 15 2004
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

#include "wancfg.h"
#include "text_box.h"
#include "input_box.h"
#include "menu_adsl_advanced_cfg.h"
#include "menu_adsl_standard.h"
#include "menu_adsl_trellis.h"
#include "menu_adsl_coding_gain.h"
#include "menu_adsl_rx_bin_adjust.h"
#include "menu_adsl_framing_struct.h"
#include "menu_adsl_exchange_type.h"
#include "menu_adsl_clock_type.h"

#define DBG_ADSL_ADVANCED_CFG 1

char* tx_start_bin_help_str = 
"The lowest bin number allowed for the transmit\n\
signal can be selected. This allows the customer\n\
to limit bins used for special configurations.\n\
\n\
0x06-0x1F       upstream\n\
0x06-0xFF       downstrem (0x20-0xFF for FDM)";

char* tx_end_bin_help_str = 
"The highest bin number allowed for the transmit\n\
signal can be selected. This allows the customer\n\
to limit bins used for special configurations.\n\
\n\
0x06-0x1F       upstream\n\
0x06-0xFF       downstrem (0x20-0xFF for FDM)";


char* rx_start_bin_help_str = 
"The lowest bin number allowed for the receive\n\
signal can be selected. This allows the customer\n\
to limit bins used for special configurations.";
				  
char* rx_end_bin_help_str = 
"The highest bin number allowed for the receive\n\
signal can be selected. This allows the customer\n\
to limit bins used for special configurations.";

char* max_down_rate_help_str =
"Use this value to limit the downstream data rate.\n\
The max downrate is 8192Kbps.  By default this\n\
value is used to achieve the highest baudrate\n\
during negotiation.\n\
\n\
However, user can specify a lower value in order\n\
to force the remote end to lower baud rate.\n\
\n\
Default: 8192Kbps.";
				  
enum {
  Verbose,
  RxBufferCount,
  TxBufferCount,
  AdslStandard,
  AdslTrellis,
  AdslTxPowerAtten,
  AdslCodingGain,
  AdslMaxBitsPerBin,
  AdslTxStartBin,
  AdslTxEndBin,
  AdslRxStartBin,
  AdslRxEndBin,
  AdslRxBinAdjust,
  AdslFramingStruct,
  AdslExpandedExchange,
  AdslClockType,
  AdslMaxDownRate
};

menu_adsl_advanced_cfg::menu_adsl_advanced_cfg(IN char * lxdialog_path, IN wan_adsl_conf_t* adsl_cfg)
{
  Debug(DBG_ADSL_ADVANCED_CFG,
    ("menu_adsl_advanced_cfg::menu_adsl_advanced_cfg()\n"));

  snprintf(this->lxdialog_path, MAX_PATH_LENGTH, "%s", lxdialog_path);
  this->adsl_cfg = adsl_cfg;
}

menu_adsl_advanced_cfg::~menu_adsl_advanced_cfg()
{
  Debug(DBG_ADSL_ADVANCED_CFG,
    ("menu_adsl_advanced_cfg::~menu_adsl_advanced_cfg()\n"));
}

int menu_adsl_advanced_cfg::run(OUT int * selection_index)
{
  string menu_str = "";
  int rc = YES;
  char tmp_buff[MAX_PATH_LENGTH];
  char exit_dialog = NO;
  int number_of_items = 0;
 
  //help text box
  text_box tb;
/*
Verbose			= 1
*/
again:
  Debug(DBG_ADSL_ADVANCED_CFG, ("menu_adsl_advanced_cfg::run()\n"));

  
  menu_str = "";
  rc = YES;
  exit_dialog = NO;
  number_of_items = 0;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", RxBufferCount);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"RxBufferCount--------> %u\" ", adsl_cfg->RxBufferCount);
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", TxBufferCount);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"TxBufferCount--------> %u\" ", adsl_cfg->TxBufferCount);
  menu_str += tmp_buff;
  number_of_items++;

  //= ADSL_MULTIMODE
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslStandard);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslStandard---------> %s\" ",
		  ADSL_DECODE_STANDARD(adsl_cfg->Standard));
  menu_str += tmp_buff;
  number_of_items++;
  
  //= ADSL_TRELLIS_ENABLE
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslTrellis);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslTrellis----------> %s\" ",
		  ADSL_DECODE_TRELLIS(adsl_cfg->Trellis));
  menu_str += tmp_buff;
  number_of_items++;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslTxPowerAtten);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslTxPowerAtten-----> %d\" ", adsl_cfg->TxPowerAtten);
  menu_str += tmp_buff;
  number_of_items++;
  
  //= ADSL_AUTO_CODING_GAIN
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslCodingGain);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslCodingGain-------> %s\" ",
		  ADSL_DECODE_GAIN(adsl_cfg->CodingGain));
  menu_str += tmp_buff;
  number_of_items++;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslMaxBitsPerBin);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslMaxBitsPerBin----> %u\" ", adsl_cfg->MaxBitsPerBin);
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslTxStartBin);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslTxStartBin-------> 0x%02X\" ", adsl_cfg->TxStartBin);
  menu_str += tmp_buff;
  number_of_items++;
  
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslTxEndBin);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslTxEndBin---------> 0x%02X\" ", adsl_cfg->TxEndBin);
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslRxStartBin);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslRxStartBin-------> 0x%02X\" ", adsl_cfg->RxStartBin);
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslRxEndBin);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslRxEndBin---------> 0x%02X\" ", adsl_cfg->RxEndBin);
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslRxBinAdjust);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslRxBinAdjust------> %s\" ",
		  ADSL_DECODE_RX_BIN_ADJUST(adsl_cfg->RxBinAdjust));
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslFramingStruct);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslFramingStruct----> %s\" ",
		  ADSL_DECODE_FRAMING_STRUCT(adsl_cfg->FramingStruct));
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslExpandedExchange);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslExpandedExchange-> %s\" ",
		  ADSL_DECODE_EXPANDED_EXCHANGE(adsl_cfg->ExpandedExchange));
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslClockType);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslClockType--------> %s\" ",
		  ADSL_DECODE_CLOCK_TYPE(adsl_cfg->ClockType));
  menu_str += tmp_buff;
  number_of_items++;

  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"%d\" ", AdslMaxDownRate);
  menu_str += tmp_buff;
  snprintf(tmp_buff, MAX_PATH_LENGTH, " \"AdslMaxDownRate------> %u\" ", adsl_cfg->MaxDownRate);
  menu_str += tmp_buff;
  number_of_items++;

  //////////////////////////////////////////////////////////////////////////////////////
  //create the explanation text for the menu
  snprintf(tmp_buff, MAX_PATH_LENGTH,
"\n------------------------------------------\
\n\nAdvanced ADSL configuration.");

  if(set_configuration(   YES,//indicates to call V2 of the function
                          MENU_BOX_SELECT,
                          lxdialog_path,
                          "ADVANCED ADSL CONFIGURATION",
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
    Debug(DBG_ADSL_ADVANCED_CFG,
      ("option selected for editing: %s\n", get_lxdialog_output_string()));

    switch(atoi(get_lxdialog_output_string()))
    {
    case RxBufferCount:
      get_adsl_numeric_parameter( &adsl_cfg->RxBufferCount, //OUT unsigned short* result,
				  &adsl_cfg->RxBufferCount, //IN  unsigned short* initial_value,
				  "Please specify Rx buffer count",//IN  char* prompt_text
				  option_not_implemented_yet_help_str,
				  NO);
      break;
      
    case TxBufferCount:
      get_adsl_numeric_parameter( &adsl_cfg->TxBufferCount,
				  &adsl_cfg->TxBufferCount,
				  "Please specify Tx buffer count",
				  option_not_implemented_yet_help_str,
				  NO);
      break;
      
    case AdslStandard:
      {
	menu_adsl_standard adsl_standard(lxdialog_path, adsl_cfg);
	if(adsl_standard.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;
      
    case AdslTrellis:
      {
	menu_adsl_trellis adsl_trellis(lxdialog_path, adsl_cfg);
	if(adsl_trellis.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;
      
    case AdslTxPowerAtten:
show_tx_pwr_atten_box:
      get_adsl_numeric_parameter( &adsl_cfg->TxPowerAtten,
				  &adsl_cfg->TxPowerAtten,
				  "Tx Power Attennuation (from 0 to 12 Db)",
				  option_not_implemented_yet_help_str,
				  NO);
      if(adsl_cfg->TxPowerAtten > 12){
	tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, "Invalid Input");
	goto show_tx_pwr_atten_box;
      }
      break;
      
    case AdslCodingGain:
      {
	menu_adsl_coding_gain adsl_coding_gain(lxdialog_path, adsl_cfg);
	if(adsl_coding_gain.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;
      
    case AdslMaxBitsPerBin:
show_max_bits_per_bin_box:
      get_adsl_numeric_parameter( &adsl_cfg->MaxBitsPerBin,
				  &adsl_cfg->MaxBitsPerBin,
				  "Maximum Bits Per Bin (less or equal to 15)",
				  option_not_implemented_yet_help_str,
				  NO);
      if(adsl_cfg->MaxBitsPerBin > 15){
	tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, "Invalid Input");
	goto show_max_bits_per_bin_box;
      }
      break;
      
    case AdslTxStartBin:
      get_adsl_numeric_parameter( &adsl_cfg->TxStartBin,
				  &adsl_cfg->TxStartBin,
				  "TxStartBin",
				  tx_start_bin_help_str,
				  YES);
      break;
      
    case AdslTxEndBin:
     get_adsl_numeric_parameter(  &adsl_cfg->TxEndBin,
				  &adsl_cfg->TxEndBin,
				  "TxEndBin",
				  tx_end_bin_help_str,
				  YES);
      break;
      
    case AdslRxStartBin:
      get_adsl_numeric_parameter( &adsl_cfg->RxStartBin,
				  &adsl_cfg->RxStartBin,
				  "RxStartBin",
				  rx_start_bin_help_str,
				  YES);

      break;
      
    case AdslRxEndBin:
      get_adsl_numeric_parameter( &adsl_cfg->RxEndBin,
				  &adsl_cfg->RxEndBin,
				  "RxEndBin",
				  rx_end_bin_help_str,
				  YES);
      break;
      
    case AdslRxBinAdjust:
      {
	menu_adsl_rx_bin_adjust adsl_rx_bin_adjust(lxdialog_path, adsl_cfg);
	if(adsl_rx_bin_adjust.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;
      
    case AdslFramingStruct:
      {
	menu_adsl_framing_struct adsl_framing_struct(lxdialog_path, adsl_cfg);
	if(adsl_framing_struct.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;
      
    case AdslExpandedExchange:
      {
	menu_adsl_exchange_type adsl_exchange_type(lxdialog_path, adsl_cfg);
	if(adsl_exchange_type.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;
      
    case AdslClockType:
      {
	menu_adsl_clock_type adsl_clock_type(lxdialog_path, adsl_cfg);
	if(adsl_clock_type.run(selection_index) == NO){
          rc = NO;
          exit_dialog = YES;
        }
      }
      break;
      
    case AdslMaxDownRate:
      get_adsl_numeric_parameter( &adsl_cfg->MaxDownRate,
				  &adsl_cfg->MaxDownRate,
				  "MaxDownRate",
				  max_down_rate_help_str,
				  NO);
      break;
      
    default:
      ERR_DBG_OUT(("Invalid option selected for editing: %s\n", get_lxdialog_output_string()))
      exit_dialog = YES;
    }
    break;

  case MENU_BOX_BUTTON_HELP:
    switch(atoi(get_lxdialog_output_string()))
    {
    case AdslTxStartBin:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, tx_start_bin_help_str);
      break;
    case AdslTxEndBin:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, tx_end_bin_help_str);
      break;
    case AdslRxStartBin:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, rx_start_bin_help_str);
      break;
    case AdslRxEndBin:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, rx_end_bin_help_str);
      break;
    case AdslMaxDownRate:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, max_down_rate_help_str);
    default:
      tb.show_help_message(lxdialog_path, NO_PROTOCOL_NEEDED, option_not_implemented_yet_help_str);
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

int menu_adsl_advanced_cfg::
  get_adsl_numeric_parameter(	OUT unsigned short* result,
				IN  unsigned short* initial_value,
				IN  char* prompt_text,
				IN  char* help_str,
				IN  char  is_hexadecimal)
{
  text_box tb;
  input_box inb;
  char backtitle[MAX_PATH_LENGTH];
  char explanation_text[MAX_PATH_LENGTH];
  char initial_text[MAX_PATH_LENGTH];
  int  selection_index;
  unsigned int int_tmp;
  
again:
  snprintf(explanation_text, MAX_PATH_LENGTH, prompt_text);
  if(is_hexadecimal == NO){
    snprintf(initial_text, MAX_PATH_LENGTH, "%u", *initial_value);
  }else{
    snprintf(initial_text, MAX_PATH_LENGTH, "%02X", *initial_value);
  }
  snprintf(backtitle, MAX_PATH_LENGTH, "WANPIPE Configuration Utility");

  inb.set_configuration(  lxdialog_path,
                          backtitle,
                          explanation_text,
                          INPUT_BOX_HIGTH,
                          INPUT_BOX_WIDTH,
                          initial_text);

  inb.show(&selection_index);

  switch(selection_index)
  {
  case INPUT_BOX_BUTTON_OK:
    if(is_hexadecimal == NO){
      *result = atoi(inb.get_lxdialog_output_string());
    }else{
      //Note: format "%x" writes and "unsigned int" which
      //overwrites the next value after "result" in the structure.
      //To fix it, use temporary unsigned int and then  copy to unsigned short.
      sscanf(inb.get_lxdialog_output_string(), "%x", &int_tmp);
      *result = (unsigned short)int_tmp;
    }
    break;

  case INPUT_BOX_BUTTON_HELP:
    tb.show_help_message(lxdialog_path, PROTOCOL_NOT_SET, help_str);
    goto again;
  }//switch(*selection_index)
  
  return YES;
}

