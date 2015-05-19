/* wan_mem_debug.h */
#ifndef __WAN_MEMDEBUG_H_
#define __WAN_MEMDEBUG_H_


/****************************************************************************
**		MEMORY DEBUG F U N C T I O N S				
****************************************************************************/

#if defined(WAN_DEBUG_MEM)

#if defined(__WINDOWS__) 

#if defined(BUSENUM_DRV)
# define WAN_DEBUG_MEM_CALL
#else
# define WAN_DEBUG_MEM_CALL	DECLSPEC_IMPORT
#endif

int __sdla_memdbg_init(void);
int __sdla_memdbg_free(void);
int __sdla_memdbg_push(void *mem, const char *func_name, const int line, int len);
int __sdla_memdbg_pull(void *mem, const char *func_name, const int line);

#else

# define WAN_DEBUG_MEM_CALL

#endif	/* !__WINDOWS__*/

WAN_DEBUG_MEM_CALL int sdla_memdbg_init(void);
WAN_DEBUG_MEM_CALL int sdla_memdbg_free(void);
WAN_DEBUG_MEM_CALL int sdla_memdbg_push(void *mem, const char *func_name, const int line, int len);
WAN_DEBUG_MEM_CALL int sdla_memdbg_pull(void *mem, const char *func_name, const int line);

#endif	/* WAN_DEBUG_MEM */

#endif	/* __WAN_MEMDEBUG_H_ */
