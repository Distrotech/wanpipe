

#include "sangoma_mgd.h"
#include "sangoma_mgd_memdbg.h"


int wan_debug_mem;
pthread_mutex_t smg_debug_mem_lock;

#if defined(SMG_MEMORY_DEBUG)
#warning "SMG_MEMORY_DEBUG Enabled"
#endif

WAN_LIST_HEAD(SNAME_PLACEHOLDER_MEM, sdla_memdbg_el) sdla_memdbg_head = 
			WAN_LIST_HEAD_INITIALIZER(&sdla_memdbg_head);


int sdla_memdbg_init(void)
{
#if defined(SMG_MEMORY_DEBUG)
	pthread_mutex_init(&smg_debug_mem_lock, NULL);	
	WAN_LIST_INIT(&sdla_memdbg_head);
#endif
	return 0;
}


int sdla_memdbg_push(void *mem, char *func_name, int line, int len)
{
#if defined(SMG_MEMORY_DEBUG)
	sdla_memdbg_el_t *sdla_mem_el;

	sdla_mem_el = malloc(sizeof(sdla_memdbg_el_t));
	if (!sdla_mem_el) {
		log_printf(SMG_LOG_ALL,NULL,"%s:%d Critical failed to allocate memory!\n",
			__FUNCTION__,__LINE__);
		return -1;
	}

	memset(sdla_mem_el,0,sizeof(sdla_memdbg_el_t));
		
	sdla_mem_el->len=len;
	sdla_mem_el->line=line;
	sdla_mem_el->mem=mem;
	strncpy(sdla_mem_el->cmd_func,func_name,sizeof(sdla_mem_el->cmd_func)-1);
	

	pthread_mutex_lock(&smg_debug_mem_lock);
	wan_debug_mem+=sdla_mem_el->len;
	WAN_LIST_INSERT_HEAD(&sdla_memdbg_head, sdla_mem_el, next);
	pthread_mutex_unlock(&smg_debug_mem_lock);

#ifdef SMG_MEMORY_DEBUG_PRINT
	log_printf(SMG_LOG_ALL,NULL,"%s:%d: Alloc %p Len=%i Total=%i\n",
			sdla_mem_el->cmd_func,sdla_mem_el->line,
			 sdla_mem_el->mem, sdla_mem_el->len,wan_debug_mem);
#endif
	
#endif
	return 0;

}

int sdla_memdbg_pull(void *mem, char *func_name, int line)
{
#if defined(SMG_MEMORY_DEBUG)
	sdla_memdbg_el_t *sdla_mem_el;
	int err=-1;

	pthread_mutex_lock(&smg_debug_mem_lock);

	WAN_LIST_FOREACH(sdla_mem_el, &sdla_memdbg_head, next){
		if (sdla_mem_el->mem == mem) {
			break;
		}
	}

	if (sdla_mem_el) {
		WAN_LIST_REMOVE(sdla_mem_el, next);
		wan_debug_mem-=sdla_mem_el->len;
		pthread_mutex_unlock(&smg_debug_mem_lock);

#ifdef SMG_MEMORY_DEBUG_PRINT
		log_printf(SMG_LOG_ALL,NULL,"%s:%d: DeAlloc %p Len=%i Total=%i (From %s:%d)\n",
			func_name,line,
			sdla_mem_el->mem, sdla_mem_el->len, wan_debug_mem,
			sdla_mem_el->cmd_func,sdla_mem_el->line);
#endif

		free(sdla_mem_el);
		err=0;
	} else {
		pthread_mutex_unlock(&smg_debug_mem_lock);
	}

	if (err) {
		log_printf(SMG_LOG_ALL,NULL,"%s:%d: sdla_memdbg_pull - Critical Error: Unknown Memeory %p\n",
			func_name,line,mem);
	}

	return err;
#else
	return 0;
#endif
}

int sdla_memdbg_free(int free_mem)
{
#if defined(SMG_MEMORY_DEBUG)
	sdla_memdbg_el_t *sdla_mem_el;
	int total=0;

	log_printf(SMG_LOG_ALL,NULL,"sdladrv: Memory Still Allocated=%i \n",
			 wan_debug_mem);

	log_printf(SMG_LOG_ALL,NULL,"=====================BEGIN================================\n");

	sdla_mem_el = WAN_LIST_FIRST(&sdla_memdbg_head);
	while(sdla_mem_el){
		sdla_memdbg_el_t *tmp = sdla_mem_el;

		log_printf(SMG_LOG_ALL,NULL,"%s:%d: Mem Leak %p Len=%i \n",
			sdla_mem_el->cmd_func,sdla_mem_el->line,
			sdla_mem_el->mem, sdla_mem_el->len);
		total+=sdla_mem_el->len;

		sdla_mem_el = WAN_LIST_NEXT(sdla_mem_el, next);

		if (free_mem) {
			WAN_LIST_REMOVE(tmp, next);
			free(tmp);
		}
	}

	log_printf(SMG_LOG_ALL,NULL,"=====================END==================================\n");
	log_printf(SMG_LOG_ALL,NULL,"sdladrv: Memory Still Allocated=%i  Leaks Found=%i Missing=%i\n",
			 wan_debug_mem,total,wan_debug_mem-total);

	if (free_mem) {
		pthread_mutex_destroy(&smg_debug_mem_lock);	
	}
#endif
	return 0;
}




