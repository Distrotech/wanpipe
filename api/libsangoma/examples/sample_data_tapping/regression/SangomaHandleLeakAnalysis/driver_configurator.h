//////////////////////////////////////////////////////////////////////
// driver_configurator.h: interface for the driver_configurator class.
//
// Author	:	David Rokhvarg	<davidr@sangoma.com>
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DRIVER_CONFIGURATOR_H__115A6EC1_518C_45E9_AA55_148A8D999B61__INCLUDED_)
#define AFX_DRIVER_CONFIGURATOR_H__115A6EC1_518C_45E9_AA55_148A8D999B61__INCLUDED_

#if defined(__WINDOWS__)
# include <windows.h>
# include <conio.h>
#elif defined(__LINUX__)
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
# include <curses.h>
#else
# error Unsupported OS
#endif

#include <stdio.h>
#include <stddef.h>		//for offsetof()

#include "libsangoma.h"
#include "wanpipe_api.h"

#define MAX_CARDS			16
#define MAX_PORTS_PER_CARD	16

class driver_configurator  
{
public:
	driver_configurator();
	virtual ~driver_configurator();

	int get_hardware_info(hardware_info_t *hardware_info);

	int get_driver_version(wan_driver_version_t *version);

	int set_t1_e1_configuration(sdla_fe_cfg_t *sdla_fe_cfg, buffer_settings_t *buffer_settings);

	int init(unsigned int wanpipe_number, unsigned int port_number);

	//GET current configuraion of the API driver
	int get_configuration(port_cfg_t *port_cfg);

	//SET new configuration of the API driver
	int set_configuration(port_cfg_t *port_cfg);

	//Function to print contents of 'port_cfg_t' structure.
	int print_port_cfg_structure(port_cfg_t *port_cfg);

	int print_sdla_fe_cfg_t_structure(sdla_fe_cfg_t *sdla_fe_cfg);
	int print_wanif_conf_t_structure(wanif_conf_t *wanif_conf);

	int scan_for_sangoma_cards(wanpipe_instance_info_t *wanpipe_info_array, int card_model);

private:

	port_cfg_t	port_cfg;
	unsigned int wp_number;
	char wanpipe_name_str[WAN_DRVNAME_SZ*2];
	sng_fd_t wp_handle;

	//reverse actions of init()
	void cleanup();
	
	int push_a_card_into_wanpipe_info_array(wanpipe_instance_info_t *wanpipe_info_array, hardware_info_t *new_hw_info);
};

#endif // !defined(AFX_DRIVER_CONFIGURATOR_H__115A6EC1_518C_45E9_AA55_148A8D999B61__INCLUDED_)

