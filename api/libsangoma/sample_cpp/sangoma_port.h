//////////////////////////////////////////////////////////////////////
// sangoma_port.h: interface for the sangoma_port class.
//
// Author	:	David Rokhvarg
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DRIVER_CONFIGURATOR_H__115A6EC1_518C_45E9_AA55_148A8D999B61__INCLUDED_)
#define AFX_DRIVER_CONFIGURATOR_H__115A6EC1_518C_45E9_AA55_148A8D999B61__INCLUDED_

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>



#if defined(__WINDOWS__)
# include <windows.h>
# include <winioctl.h>
# include <conio.h>
# include "bit_win.h"

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
# error "sangoma_port.h: undefined OS type"
#endif

#include "libsangoma.h"
#include "wanpipe_api.h"

#define MAX_CARDS			32
#define MAX_PORTS_PER_CARD	48



#if defined (__LINUX__)
static inline void CloseHandle(HANDLE handle)
{
	if (handle >= 0) {
		close(handle);
	}

	return;
}
#endif

/*!
  \brief 
 *
 */
class sangoma_port
{
public:
	int get_hardware_info(hardware_info_t *hardware_info);
	int stop_port();
	int start_port();

	int init(uint16_t wanpipe_number);
	void cleanup();

	sangoma_port();
	virtual ~sangoma_port();

	int scan_for_sangoma_cards(wanpipe_instance_info_t *wanpipe_info_array, int card_model);

protected:
	uint16_t wp_number;
	sng_fd_t wp_handle;

	int push_a_card_into_wanpipe_info_array(wanpipe_instance_info_t *wanpipe_info_array,
										hardware_info_t *new_hw_info);
};

#endif // !defined(AFX_DRIVER_CONFIGURATOR_H__115A6EC1_518C_45E9_AA55_148A8D999B61__INCLUDED_)

