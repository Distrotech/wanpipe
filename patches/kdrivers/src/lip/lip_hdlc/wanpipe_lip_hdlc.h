
#ifndef _LIP_HDLC_H_
#define _LIP_HDLC_H_ 1 

#include "stddef.h"
#include "linux/types.h"
#include "wanpipe_abstr.h"
#include "wanpipe_cfg_def.h"
#include "wanpipe_cfg_hdlc.h"
#include "wanpipe_cfg_lip.h"


/* This is the shared header between the LIP
 * and this PROTOCOL */
#include "wanpipe_lip_hdlc_iface.h"

#ifndef WAN_IFNAME_SZ
#define WAN_IFNAME_SZ 20
#endif

typedef struct wp_lip_hdlc_header
{
	/* IMPLEMENT PROTOCOL HEADER HERE */

}wp_lip_hdlc_header_t;


/* PRIVATE Function Prototypes Go Here */

#define lip_container_of(ptr, type, member) ({  const typeof( ((type *)0)->member ) *__mptr = (ptr); \
						(type *)( (char *)__mptr - offsetof(type,member) );})

/*===================================================================
 * 
 * DEFINES
 * 
 *==================================================================*/

#define MAX_SOCK_CRC_QUEUE 3
#define MAX_SOCK_HDLC_BUF 10000
#define HDLC_ENG_BUF_LEN 10000

 
#define INC_CRC_CNT(a)   if (++a >= MAX_SOCK_CRC_QUEUE) a=0;
#define GET_FIN_CRC_CNT(a)  { if (--a < 0) a=MAX_SOCK_CRC_QUEUE-1; \
		              if (--a < 0) a=MAX_SOCK_CRC_QUEUE-1; }

#define FLIP_CRC(a,b)  { b=0; \
			 b |= (a&0x000F)<<12 ; \
			 b |= (a&0x00F0) << 4; \
			 b |= (a&0x0F00) >> 4; \
			 b |= (a&0xF000) >> 12; }

#define DECODE_CRC(a) { a=( (((~a)&0x000F)<<4) | \
		            (((~a)&0x00F0)>>4) | \
			    (((~a)&0x0F00)<<4) | \
			    (((~a)&0xF000)>>4) ); }
#define BITSINBYTE 8

#define NO_FLAG 	0
#define OPEN_FLAG 	1
#define CLOSING_FLAG 	2       


typedef struct wanpipe_hdlc_stats
{
	int packets;
	int errors;

	int crc;
	int abort;
	int frame_overflow;

}wanpipe_hdlc_stats_t;


typedef	struct wanpipe_hdlc_decoder{
	unsigned char 	rx_decode_buf[MAX_SOCK_HDLC_BUF];
	unsigned int  	rx_decode_len;
	unsigned char 	rx_decode_bit_cnt;
	unsigned char 	rx_decode_onecnt;
	
	unsigned long	hdlc_flag;
	unsigned short 	rx_orig_crc;
	unsigned short 	rx_crc[MAX_SOCK_CRC_QUEUE];
	unsigned short 	crc_fin;

	unsigned short 	rx_crc_tmp;
	int 		crc_cur;
	int 		crc_prv;
	wanpipe_hdlc_stats_t stats;
}wanpipe_hdlc_decoder_t;


typedef struct wanpipe_hdlc_encoder_bpk {

	unsigned char tx_flag_idle;
	unsigned char tx_flag_offset;
	unsigned char tx_decode_bit_cnt;
	unsigned char tx_flag_offset_data;

}wanpipe_hdlc_encoder_bpk_t;

typedef	struct wanpipe_hdlc_encoder{
	
	unsigned char tx_decode_buf[HDLC_ENG_BUF_LEN];
	unsigned int  tx_decode_len;
	unsigned char tx_decode_bit_cnt;
	unsigned char tx_decode_onecnt;

	unsigned short tx_crc;
	unsigned char tx_flag;
	unsigned char tx_flag_offset;
	unsigned char tx_flag_offset_data;
	unsigned char tx_flag_idle;  
	
	unsigned short tx_crc_fin;
	unsigned short tx_crc_tmp;   
	//unsigned char  tx_idle_flag;
	unsigned char  bits_in_byte;
	  
	wanpipe_hdlc_encoder_bpk_t backup;

	wanpipe_hdlc_stats_t stats;
}wanpipe_hdlc_encoder_t;

typedef struct wanpipe_hdlc_engine
{

	wanpipe_hdlc_decoder_t decoder;
	wanpipe_hdlc_encoder_t encoder;

	unsigned char	raw_rx[MAX_SOCK_HDLC_BUF];
	unsigned char	raw_tx[MAX_SOCK_HDLC_BUF];

	int 		refcnt;

	unsigned char	bound;

	unsigned long	active_ch;
	unsigned short  timeslots;
	struct wanpipe_hdlc_engine *next;

	int 		skb_decode_size;
	unsigned char	seven_bit_hdlc;
	unsigned char 	bits_in_byte;

	void *prot_ptr;
	int (*hdlc_data) (struct wanpipe_hdlc_engine *hdlc_eng, void *data, int len);

}wanpipe_hdlc_engine_t;

typedef struct hdlc_list
{
	wanpipe_hdlc_engine_t *hdlc;
	struct hdlc_list *next;
}wanpipe_hdlc_list_t; 



typedef struct _lip_hdlc_ 
{
	unsigned long		critical;
	void 			*link_dev;
	void 			*dev;
	unsigned char		type;
	wplip_prot_reg_t	reg;
	wan_lip_hdlc_if_conf_t	cfg;
	unsigned char 		name[WAN_IFNAME_SZ+1];
	unsigned char 		hwdevname[WAN_IFNAME_SZ+1];
	unsigned int 		refcnt;

	/* INSERT PROTOCOL SPECIFIC VARIABLES HERE */

	int 			facility_index;	
	unsigned char		next_idle;
	wanpipe_hdlc_engine_t *	hdlc_eng;
	void*			tx_q;

}wp_lip_hdlc_t;



#define set_bit(bit_no,ptr) ((*ptr)|=(1<<bit_no)) 
#define clear_bit(bit_no,ptr) ((*ptr)&=~(1<<bit_no))
#define test_bit(bit_no,ptr)  ((*ptr)&(1<<bit_no))

#if 0
#define DEBUG_TX	printf
#define DEBUG_EVENT	printf	
#else
#define DEBUG_EVENT	
#define DEBUG_TX	
#endif

static __inline 
void init_hdlc_decoder(wanpipe_hdlc_decoder_t *hdlc_decoder)
{
	hdlc_decoder->hdlc_flag=0;
	set_bit(NO_FLAG,&hdlc_decoder->hdlc_flag);
	
	hdlc_decoder->rx_decode_len=0;
	hdlc_decoder->rx_decode_buf[hdlc_decoder->rx_decode_len]=0;
	hdlc_decoder->rx_decode_bit_cnt=0;
	hdlc_decoder->rx_decode_onecnt=0;
	hdlc_decoder->rx_crc[0]=-1;
	hdlc_decoder->rx_crc[1]=-1;
	hdlc_decoder->rx_crc[2]=-1;
	hdlc_decoder->crc_cur=0; 
	hdlc_decoder->crc_prv=0;
}                   

static __inline
void init_hdlc_encoder(wanpipe_hdlc_encoder_t *chan)
{
	chan->tx_crc=-1;
	chan->bits_in_byte=8;
	chan->tx_flag= 0x7E; 
	chan->tx_flag_idle= 0x7E;
}

/* External Functions */

extern wanpipe_hdlc_engine_t *wanpipe_reg_hdlc_engine (wan_lip_hdlc_if_conf_t *);
extern void wanpipe_unreg_hdlc_engine(wanpipe_hdlc_engine_t *hdlc_eng);
extern int wanpipe_hdlc_decode (wanpipe_hdlc_engine_t *hdlc_eng, 
			 	unsigned char *buf, int len);
extern int wanpipe_hdlc_encode(wanpipe_hdlc_engine_t *hdlc_eng, 
		       unsigned char *usr_data, int usr_len,
		       unsigned char *hdlc_data, int *hdlc_len,
		       unsigned char *next_idle);
extern int wanpipe_get_rx_hdlc_errors (wanpipe_hdlc_engine_t *hdlc_eng);
extern int wanpipe_get_tx_hdlc_errors (wanpipe_hdlc_engine_t *hdlc_eng);
extern int wanpipe_get_rx_hdlc_packets (wanpipe_hdlc_engine_t *hdlc_eng);
extern int wanpipe_get_tx_hdlc_packets (wanpipe_hdlc_engine_t *hdlc_eng);

extern int wanpipe_hdlc_bkp(wanpipe_hdlc_engine_t *hdlc_eng);
extern int wanpipe_hdlc_restore(wanpipe_hdlc_engine_t *hdlc_eng);

extern int wanpipe_hdlc_encode_idle(wanpipe_hdlc_engine_t *hdlc_eng,unsigned char *usr_data, int usr_len);


/* PRIVATE Debug Functions */

#define wp_dev_hold(dev)    do{\
				wpabs_debug_test("%s:%d (dev_hold) Refnt %i!\n",\
							__FUNCTION__,__LINE__,dev->refcnt );\
				dev->refcnt++; \
			    }while(0)

#define wp_dev_put(dev)	    do{\
				wpabs_debug_test("%s:%d (wp_dev_put): Refnt %i!\n",\
							__FUNCTION__,__LINE__,dev->refcnt );\
				if (--dev->refcnt == 0){\
					wpabs_debug_test("%s:%d (wp_dev_put): Deallocating!\n",\
							__FUNCTION__,__LINE__);\
					wpabs_free(dev);\
					dev=NULL;\
				}\
		            }while(0)

#define FUNC_BEGIN() wpabs_debug_test("%s:%d ---Begin---\n",__FUNCTION__,__LINE__); 
#define FUNC_END() wpabs_debug_test("%s:%d ---End---\n\n",__FUNCTION__,__LINE__); 

#define FUNC_BEGIN1() wpabs_debug_event("%s:%d ---Begin---\n",__FUNCTION__,__LINE__); 
#define FUNC_END1() wpabs_debug_event("%s:%d ---End---\n\n",__FUNCTION__,__LINE__); 


#endif


