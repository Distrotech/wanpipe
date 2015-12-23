/*************************************************************************
* wanpipe_cfg_adsl.h							 *
*									 *
*	WANPIPE(tm)	Wanpipe ADSL Interface configuration	 *
*									 *
* Author:	Alex Feldman <al.feldman@sangoma.com>			 *
*========================================================================*
* Aug 27, 2008	Alex Feldman	Initial version				 *
*									 *
*************************************************************************/

#ifndef __WANPIPE_CFG_ADSL_H__
# define __WANPIPE_CFG_ADSL_H__

/* DSL interface types */
#define WAN_INTERFACE 	0
#define LAN_INTERFACE	1

#pragma pack(1)

typedef struct wan_adsl_vcivpi
{
	unsigned short	vci;
	unsigned char	vpi;
} wan_adsl_vcivpi_t;


typedef struct wan_adsl_conf
{
#if 1
	unsigned char     EncapMode;
	unsigned short    Vci;
	unsigned short    Vpi;
#else
	unsigned char     interface;
	unsigned char     Rfc1483Mode;
	unsigned short    Rfc1483Vci;
	unsigned short    Rfc1483Vpi;
	unsigned char     Rfc2364Mode;
	unsigned short    Rfc2364Vci;
	unsigned short    Rfc2364Vpi;
#endif
	unsigned char     Verbose; 
	unsigned short    RxBufferCount;
	unsigned short    TxBufferCount;

	unsigned short    Standard;
	unsigned short    Trellis;
	unsigned short    TxPowerAtten;
	unsigned short    CodingGain;
	unsigned short    MaxBitsPerBin;
	unsigned short    TxStartBin;
	unsigned short    TxEndBin;
	unsigned short    RxStartBin;
	unsigned short    RxEndBin;
	unsigned short    RxBinAdjust;
	unsigned short    FramingStruct;
	unsigned short    ExpandedExchange;
	unsigned short    ClockType;
	unsigned short    MaxDownRate;

	unsigned char	  atm_autocfg;
	unsigned short	  vcivpi_num;	
	wan_adsl_vcivpi_t vcivpi_list[100];	
	unsigned char	  tty_minor;
	unsigned short	  mtu;

	unsigned char	  atm_watchdog;
	/*	Number of cells received on each interrupt. Recommended values: 5 - 40. 
		Higher values for higher line speeds.
		
	*/
	unsigned short    RxCellCount;

}wan_adsl_conf_t;

#pragma pack()

#endif /* __WANPIPE_CFG_ADSL_H__ */
