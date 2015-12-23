
/* wanec_api.h - declarations for wanec_api */

#if !defined(__WANEC_API_H__)
# define __WANEC_API_H__

#ifdef __WINDOWS__
# ifdef __cplusplus
   extern "C" {	/* for C++ users */
# endif

# ifdef _WANEC_API_EXPORTS_
#  define _WANEC_API_CALL __declspec(dllexport) __cdecl
# else
#  define _WANEC_API_CALL __declspec(dllimport) __cdecl
# endif

#else
# define _WANEC_API_CALL
#endif

#define WANEC_API_FEATURE_AUDIO_MEM_BUFFER_LOAD 1


/* Echo Cancellatio OP MOde defines */
#define WANEC_API_OPMODE_NORMAL			0
#define WANEC_API_OPMODE_HT_FREEZE		1
#define WANEC_API_OPMODE_HT_RESET		2
#define WANEC_API_OPMODE_POWER_DOWN		3
#define WANEC_API_OPMODE_NO_ECHO		4
#define WANEC_API_OPMODE_SPEECH_RECOGNITION	5

#pragma pack(1)

typedef struct {
	wan_custom_conf_t	conf;
} wanec_api_config_t;
typedef struct {
	int			notused;
} wanec_api_release_t;
typedef struct {
	int			enable;
	unsigned long		fe_chan_map;
#if defined(__GNUC__) && !defined(__x86_64__)
    unsigned int reserved;
#endif
} wanec_api_mode_t;
typedef struct {
	int			enable;
	unsigned long		fe_chan_map;
#if defined(__GNUC__) && !defined(__x86_64__)
    unsigned int reserved;
#endif
} wanec_api_bypass_t;
typedef struct {
	int			mode;
	unsigned long		fe_chan_map;
#if defined(__GNUC__) && !defined(__x86_64__)
    unsigned int reserved;
#endif
} wanec_api_opmode_t;
typedef struct {
	unsigned long		fe_chan_map;
#if defined(__GNUC__) && !defined(__x86_64__)
    unsigned int reserved;
#endif
	wan_custom_conf_t	conf;
} wanec_api_modify_t;
typedef struct {
	int			mode;
	unsigned long		fe_chan_map;
#if defined(__GNUC__) && !defined(__x86_64__)
    unsigned int reserved;
#endif
	unsigned char		port_map;
} wanec_api_mute_t;
typedef struct {
	int			id;
	int			enable;
	unsigned long		fe_chan_map;
#if defined(__GNUC__) && !defined(__x86_64__)
    unsigned int reserved;
#endif
	unsigned char		port_map;
	unsigned char		type_map;
} wanec_api_tone_t;
typedef struct {
	int			full;
	int			fe_chan;
	int			reset;
	int			silent;
} wanec_api_stats_t;
typedef struct {
	int			notused;
} wanec_api_image_t;
typedef struct {
	char			*buffer;
#if defined(__GNUC__) && !defined(__x86_64__)
	unsigned int reserved;
#endif
	unsigned char		pcmlaw;
	unsigned int		buffer_id;
} wanec_api_bufferload_t;
typedef struct {
	unsigned int		buffer_id;
} wanec_api_bufferunload_t;
typedef struct {
	int			start;
	unsigned int		buffer_id;
	int			fe_chan;
	unsigned char		port;
	unsigned int		repeat_cnt;
	unsigned int		duration;
	int			notifyonstop;
	unsigned int		user_event_id;
} wanec_api_playout_t;
typedef struct {
	int			fe_chan;
	int			data_mode;
} wanec_api_monitor_t;

typedef struct {
	char			*buffer;
#if defined(__GNUC__) && !defined(__x86_64__)
	unsigned int reserved;
#endif
	unsigned int	size;
	unsigned char	pcmlaw;
	unsigned int	buffer_id;
} wanec_api_membufferload_t;

#pragma pack()


extern int _WANEC_API_CALL wanec_api_init(void);
extern int _WANEC_API_CALL wanec_api_param_name(char *key);
extern int _WANEC_API_CALL wanec_api_param_value(char *value);
extern int _WANEC_API_CALL wanec_api_config(char*,int,wanec_api_config_t*);
extern int _WANEC_API_CALL wanec_api_release(char*,int,wanec_api_release_t*);
extern int _WANEC_API_CALL wanec_api_mode(char*,int,wanec_api_mode_t*);
extern int _WANEC_API_CALL wanec_api_bypass(char*,int,wanec_api_bypass_t*);
extern int _WANEC_API_CALL wanec_api_opmode(char*,int,wanec_api_opmode_t*);
extern int _WANEC_API_CALL wanec_api_modify(char*,int,wanec_api_modify_t*);
extern int _WANEC_API_CALL wanec_api_mute(char*,int,wanec_api_mute_t*);
extern int _WANEC_API_CALL wanec_api_tone(char*,int,wanec_api_tone_t*);
extern int _WANEC_API_CALL wanec_api_stats(char*,int,wanec_api_stats_t*);
extern int _WANEC_API_CALL wanec_api_hwimage(char*,int,wanec_api_image_t*);
/* */
extern int _WANEC_API_CALL wanec_api_buffer_load(char*,int,wanec_api_bufferload_t*);
/* */
extern int _WANEC_API_CALL wanec_api_mem_buffer_load(char*,int,wanec_api_membufferload_t*);
extern int _WANEC_API_CALL wanec_api_buffer_unload(char*,int,wanec_api_bufferunload_t*);
extern int _WANEC_API_CALL wanec_api_playout(char*,int,wanec_api_playout_t*);
extern int _WANEC_API_CALL wanec_api_monitor(char*,int,wanec_api_monitor_t*);
extern void _WANEC_API_CALL wanec_api_set_lib_verbosity(int verbosity_level);



#ifdef __WINDOWS__
# ifdef __cplusplus
}	/* for C++ users */
# endif
#endif

/* Backward compatible */
#define wanec_api_dtmf_t wanec_api_tone_t
#define wanec_api_dtmf wanec_api_tone

/* Forward compatible with 'trunk' version */
#define wanec_api_hwec_t wanec_api_bypass_t
#define wanec_api_hwec(devname,	verbose, bypass) wanec_api_bypass(devname, verbose, bypass)

#define WAN_EC_API_CMD_DTMF_ENABLE  WAN_EC_API_CMD_TONE_ENABLE
#define WAN_EC_API_CMD_DTMF_DISABLE WAN_EC_API_CMD_TONE_DISABLE


#endif /* __WANEC_API_H__ */
