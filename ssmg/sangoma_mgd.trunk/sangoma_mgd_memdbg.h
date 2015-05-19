# ifndef __SMG_MEM_DEBUG__
# define __SMG_MEM_DEBUG__



# define WAN_LIST_HEAD(name, type)		struct name { struct type * lh_first; }
# define WAN_LIST_HEAD_INITIALIZER(head)	{ NULL }
# define WAN_LIST_ENTRY(type) 			struct { struct type *le_next; struct type **le_prev; }
# define WAN_LIST_FIRST(head)			((head)->lh_first)
# define WAN_LIST_END(head)			NULL
# define WAN_LIST_EMPTY(head)			(WAN_LIST_FIRST(head) == WAN_LIST_END(head))
# define WAN_LIST_NEXT(elm, field)		((elm)->field.le_next)
# define WAN_LIST_FOREACH(var, head, field)	for((var) = WAN_LIST_FIRST(head);	\
							(var);				\
							(var) = WAN_LIST_NEXT(var, field))
# define WAN_LIST_INIT(head)		do { WAN_LIST_FIRST(head) = NULL;}\
		while(0)

#define	WAN_LIST_INSERT_HEAD(head, elm, field) do {				\
	if ((WAN_LIST_NEXT((elm), field) = WAN_LIST_FIRST((head))) != NULL)	\
		WAN_LIST_FIRST((head))->field.le_prev = &WAN_LIST_NEXT((elm), field);\
	WAN_LIST_FIRST((head)) = (elm);					\
	(elm)->field.le_prev = &WAN_LIST_FIRST((head));			\
} while (0)
#define	WAN_LIST_INSERT_AFTER(listelm, elm, field) do {			\
	if ((WAN_LIST_NEXT((elm), field) = WAN_LIST_NEXT((listelm), field)) != NULL)\
		WAN_LIST_NEXT((listelm), field)->field.le_prev =		\
		    &WAN_LIST_NEXT((elm), field);				\
	WAN_LIST_NEXT((listelm), field) = (elm);				\
	(elm)->field.le_prev = &WAN_LIST_NEXT((listelm), field);		\
} while (0)
#define	WAN_LIST_REMOVE(elm, field) do {					\
	if (WAN_LIST_NEXT((elm), field) != NULL)				\
		WAN_LIST_NEXT((elm), field)->field.le_prev = 		\
		    (elm)->field.le_prev;				\
	*(elm)->field.le_prev = WAN_LIST_NEXT((elm), field);		\
} while (0)



typedef struct sdla_memdbg_el
{
	unsigned int len;
	unsigned int line;
	char cmd_func[128];
	void *mem;
	WAN_LIST_ENTRY(sdla_memdbg_el)	next;
}sdla_memdbg_el_t;


int sdla_memdbg_push(void *mem, char *func_name, int line, int len);
int sdla_memdbg_init(void);
int sdla_memdbg_pull(void *mem, char *func_name, int line);
int sdla_memdbg_free(int);

#if defined(SMG_MEMORY_DEBUG)
#define smg_malloc(size) __smg_malloc(size,(char*)__FUNCTION__,(int)__LINE__)
static inline void* __smg_malloc(int size, char *func_name, int line)
#else
static inline void* smg_malloc(int size)
#endif
{
	void*	ptr = NULL;
	ptr = malloc(size);
	if (ptr){
		memset(ptr, 0, size);
#if defined(SMG_MEMORY_DEBUG)
		sdla_memdbg_push(ptr, func_name, line, size);
#endif
	}
	return ptr;
}



#if defined(SMG_MEMORY_DEBUG)
#define smg_strdup(ptr) __smg_strdup(ptr,(char*)__FUNCTION__,(int)__LINE__)
static inline char* __smg_strdup(char *sptr, char *func_name, int line)
#else
static inline char* smg_strdup(char *sptr)
#endif
{
	char* ptr = NULL;
	ptr = strdup(sptr);
	if (ptr){
#if defined(SMG_MEMORY_DEBUG)
		sdla_memdbg_push(ptr, func_name, line, strlen(ptr));
#endif
	}
	return ptr;
}

#if defined(SMG_MEMORY_DEBUG)
#define smg_strndup(ptr,len) __smg_strndup(ptr,len,(char*)__FUNCTION__,(int)__LINE__)
static inline char* __smg_strndup(char *sptr, int len, char *func_name, int line)
#else
static inline char* smg_strndup(char *sptr, int len)
#endif
{
	char* ptr = NULL;
	ptr = strndup(sptr,len);
	if (ptr){
#if defined(SMG_MEMORY_DEBUG)
		sdla_memdbg_push(ptr, func_name, line, strlen(ptr));
#endif
	}
	return ptr;
}


#if defined(SMG_MEMORY_DEBUG)
#define smg_vasprintf(a,b,c) __smg_vasprintf(a,b,c,(char*)__FUNCTION__,(int)__LINE__)
static inline int __smg_vasprintf(char **strp, const char *fmt, va_list ap, char *func_name, int line)
#else
static inline int smg_vasprintf(char **strp, const char *fmt, va_list ap)
#endif
{
	int ret = vasprintf(strp,fmt,ap);
	if (ret){
#if defined(SMG_MEMORY_DEBUG)
		sdla_memdbg_push(*strp, func_name, line, strlen(*strp));
#endif
	}
	return ret;
}



#if defined(SMG_MEMORY_DEBUG)
#define smg_free(ptr) __smg_free(ptr,(char*)__FUNCTION__,(int)__LINE__)
static inline void __smg_free(void* ptr, char *func_name, int line)
#else
static inline void smg_free(void* ptr)
#endif
{
	if (!ptr){
		return;
	}
	
#ifdef SMG_MEMORY_DEBUG
	sdla_memdbg_pull(ptr, func_name, line);
#endif

	free(ptr);
}

#endif


