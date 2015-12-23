//////////////////////////////////////////////////////////////////////
// sangoma_port_configurator.h: interface for the sangoma_port_configurator class.
//
// Author	:	David Rokhvarg
//////////////////////////////////////////////////////////////////////

#if !defined(_SANGOMA_PORT_CONFIGURATOR_H)
#define _SANGOMA_PORT_CONFIGURATOR_H

#include "sangoma_port.h"

#define MAX_COMP_INSTID 2096
#define MAX_COMP_DESC   2096
#define MAX_FRIENDLY    2096	
#define TMP_BUFFER_LEN	200

#define T1_ALL_CHANNELS_BITMAP 0xFFFFFF /* channels 1-24 */

/*!
  \brief 
 *
 */

class sangoma_port_configurator : public sangoma_port
{

	TCHAR	name[MAX_FRIENDLY];
	char    szCompInstanceId[MAX_COMP_INSTID];
	TCHAR   szCompDescription[MAX_COMP_DESC];
	TCHAR   szFriendlyName[MAX_FRIENDLY];
	DWORD   dwRegType;

#ifdef __WINDOWS__
	HKEY	hPortRegistryKey;
#endif

	port_cfg_t	port_cfg;

	//SET new configuration of the API driver
	int SetPortVolatileConfigurationCommand(port_cfg_t *port_cfg);


public:
	sangoma_port_configurator();
	virtual ~sangoma_port_configurator();
	
	int get_configration(port_cfg_t *port_cfg);

	int set_default_configuration(port_cfg_t *port_cfg);

	//Function to set Loop Current Measure (LCM) for analog FXO 
	//control_val 1: enable,0: disable
	int control_analog_rm_lcm(port_cfg_t *port_cfg, int control_val);

	//Function to set Operation mode for analog FXO 
	//opermode valid country name supported by Analog FXO
	int set_analog_opermode(port_cfg_t *port_cfg, char *opermode);


	/**************************************************************/
	/*********** Analog Global Gain Setting Functions**************/
	/**************************************************************/

	//Note: following gain settings are global. It requires drivers to
	//restart the card.
	//It is recommended to use dynamic analog txgain/rxgain functions
	//from libsangoma library:sangoma_set_rm_tx_gain and sangoma_set_rm_rx_gain

	
	//Function to set FXO  txgain
	//FXO - txgain value ranges from -150 to 120 
	int set_analog_rm_fxo_txgain(port_cfg_t *port_cfg, int txgain);

	//Function to set FXO  rxgain
	//FXO - rxgain value ranges from -150 to 120 
	int set_analog_rm_fxo_rxgain(port_cfg_t *port_cfg, int rxgain);

	//Function to set FXS  txgain
	//FXO - txgain value can be -35 or 35 
	int set_analog_rm_fxs_txgain(port_cfg_t *port_cfg, int txgain);

	//Function to set FXS  rxgain
	//FXO - rxgain value can be -35 or 35 
	int set_analog_rm_fxs_rxgain(port_cfg_t *port_cfg, int rxgain);

	/**************************************************************/
	/*********** End of Analog Global Gain Setting Functions ******/
	/**************************************************************/


	//Function to check correctness of 'port_cfg_t' structure.
	int check_port_cfg_structure(port_cfg_t *port_cfg);

	//Function to print contents of 'port_cfg_t' structure.
	int print_port_cfg_structure(port_cfg_t *port_cfg);

	//Function to print contents of 'sdla_fe_cfg_t' structure.
	int print_sdla_fe_cfg_t_structure(sdla_fe_cfg_t *sdla_fe_cfg);

	//Function to print contents of 'wanif_conf_t' structure.
	int print_wanif_conf_t_structure(wanif_conf_t *wanif_conf);

	int set_volatile_configration(port_cfg_t *port_cfg);

	int initialize_t1_tdm_span_voice_api_configration_structure(port_cfg_t *port_cfg, hardware_info_t *hardware_info, int span);
	int initialize_e1_tdm_span_voice_api_configration_structure(port_cfg_t *port_cfg, hardware_info_t *hardware_info, int span);
	int initialize_bri_tdm_span_voice_api_configration_structure(port_cfg_t *port_cfg, hardware_info_t *hardware_info, int span);
	int initialize_gsm_tdm_span_voice_api_configration_structure(port_cfg_t *port_cfg, hardware_info_t *hardware_info, int span);
	int initialize_serial_api_configration_structure(port_cfg_t *port_cfg, hardware_info_t *hardware_info, int span);

	int write_configration_on_persistent_storage(port_cfg_t *port_cfg, hardware_info_t *hardware_info, int span);

	void initialize_interface_mtu_mru(port_cfg_t *port_cfg, unsigned int mtu, unsigned int mru);
};

#endif // !defined(_SANGOMA_PORT_CONFIGURATOR_H)
