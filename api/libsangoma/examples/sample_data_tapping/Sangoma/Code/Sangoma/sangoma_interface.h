//////////////////////////////////////////////////////////////////////
// sangoma_interface.h: interface for Sangoma API driver.
//
// Author	:	David Rokhvarg	<davidr@sangoma.com>
//////////////////////////////////////////////////////////////////////

#ifndef SANGOMA_INTERFACE_H
#define SANGOMA_INTERFACE_H

#if defined(__WINDOWS__)
# include <windows.h>
# include <conio.h>
#elif defined(__LINUX__)
/* Include headers */
# include <stddef.h>
# include <errno.h>
# include <fcntl.h>
# include <string.h>
# include <ctype.h>
# include <sys/stat.h>
# include <sys/ioctl.h>
# include <sys/types.h>
# include <dirent.h>
# include <unistd.h>
# include <sys/socket.h>
# include <netdb.h>
# include <sys/un.h>
# include <sys/wait.h>
# include <unistd.h>
# include <signal.h>
# include <time.h>
#else
# error Unsupported OS
#endif

#include <string>
#include <stdio.h>
#include <stddef.h>		//for offsetof()

#include "libsangoma.h"
#include "wanpipe_api.h"

/**
  *@author David Rokhvarg
  */
struct func_protocol {
	int		protocol_id;
	unsigned char   prot_name[10];
	unsigned char	mbox_offset;
};

#define DECODE_PROT(prot)((prot==WANCONFIG_FR)?"Frame Relay":	\
			  (prot==WANCONFIG_MFR)?"Frame Relay":				\
	          (prot==WANCONFIG_PPP)?"PPP Point-to-Point":		\
	          (prot==WANCONFIG_CHDLC)?"Cisco HDLC":				\
	          (prot==WANCONFIG_X25)?"X25":						\
	          (prot==WANCONFIG_ADSL)?"ADSL: Eth,IP,PPP/ATM":	\
			  (prot==WANCONFIG_ATM)?"ATM":						\
			  (prot==WANCONFIG_AFT)?"AFT T1/E1":				\
			  (prot==WANCONFIG_AFT_TE1)?"AFT T1/E1":			\
			  (prot==WANCONFIG_AFT_ANALOG)?"AFT Analog":		\
							"Unknown")

#define DEV_NAME_LEN			100

#define ON_STR "ON"
#define OFF_STR "OFF"

#define MAX_INTERFACES_PER_PORT 32

class sangoma_interface {

	//////////////////////////////////////////////////////////////////
	/*! Sangoma IO device descriptor */
	sng_fd_t	sangoma_dev;

	/*! wait object for an IO device */
	sangoma_wait_obj_t *sng_wait_obj;

	//////////////////////////////////////////////////////////////////
	//receive stuff
	ULONG		rx_frames_count;
	ULONG		rx_bytes_count;
	//for counting frames with CRC/Abort errors
	ULONG		bad_rx_frames_count;

	//////////////////////////////////////////////////////////////////
	//transmit stuff
	ULONG		tx_bytes_count;
	ULONG		tx_frames_count;
	UCHAR		tx_test_byte;

	//////////////////////////////////////////////////////////////////
	//IOCTL management structures and variables
	wan_udp_hdr_t	wan_udp;
	int				protocol_cb_size;
	int 			wan_protocol;
	unsigned char	adapter_type;
	unsigned int	card_model;

	int GetWANConfig( void );
	int get_interface_configuration(if_cfg_t *wanif_conf_ptr);

	//return DATA at offset 'off'
	unsigned char get_wan_udphdr_data_byte(unsigned char off);
	//return POINTER to data at offset 'off'
	unsigned char *get_wan_udphdr_data_ptr(unsigned char off);
	unsigned char set_wan_udphdr_data_byte(unsigned char off, unsigned char data);

	int get_fe_type(unsigned char* adapter_type);
	int check_api_driver_version();

	int wanpipe_api_ioctl(wan_cmd_api_t *api_cmd);


	UCHAR	DoWriteCommand(sng_fd_t drv, TX_DATA_STRUCT * pTx, UINT buffer_length);
	UCHAR	DoSetIdleTxBufferCommand(sng_fd_t drv, TX_DATA_STRUCT *pTx);
	int		DoManagementCommand(sng_fd_t fd, wan_udp_hdr_t* wan_udp);

	//////////////////////////////////////////////////////////////////
	wan_driver_version_t	drv_version;

	char		interface_name[DEV_NAME_LEN];	//used only for debugging
	if_cfg_t	wanif_conf_struct;				//used only for debugging

	int wanpipe_number, interface_number;

	//In Legacy Mode  1-st interface is zero-based (i.g. wanpipe1_if0).
	//In Regular Mode 1-st interface is one-based  (i.g. wanpipe1_if1).
	//The Setup installs in Regular Mode.
	HANDLE open_first_interface_of_a_mapped_port();

public:

	sangoma_interface(int wanpipe_number, int interface_number);
	~sangoma_interface();

	int init();
	int cleanup();

	int read_te1_56k_stat(unsigned long *p_alarms);
	char alarms_present();
	void set_lb_modes(unsigned char type, unsigned char mode);
	int loopback_command(u_int8_t type, u_int8_t mode, u_int32_t chan_map);

	void operational_stats (wanpipe_chan_stats_t *stats);
	void flush_operational_stats (void);
	void flush_buffers (void);

	sangoma_wait_obj_t* get_wait_object_reference();
	unsigned char get_adapter_type();

	//Returns Port Number mapped specifically for G3 use.
	unsigned int get_mapped_port_number();

	unsigned int get_mapped_span_number();

	int get_api_driver_version (PDRIVER_VERSION p_drv_version);

	int set_buffer_multiplier(wanpipe_api_t* wp_api, int buffer_multiplier);

	int transmit (wp_api_hdr_t* hdr, void *pTx, UINT buffer_length);
	int read_data(wp_api_hdr_t* hdr, void *pRx, UINT buffer_length);
	int read_event(wanpipe_api_t* wp_api);

	bool GetReceiverLevelRange(std::string& receiver_level_range);
};

#endif//#define SANGOMA_INTERFACE_H

