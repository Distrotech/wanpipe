#ifndef __SMG_CAPABILITIES_H__
#define __SMG_CAPABILITIES_H__



#define BC_IE_CAP_SPEECH 			0x00
#define BC_IE_CAP_SPARE				0x01
#define BC_IE_CAP_64K_UNRESTRICTED 		0x02
#define BC_IE_CAP_3_1KHZ_AUDIO			0x03
#define BC_IE_CAP_ALT_SPEECH			0x04
#define BC_IE_CAP_ALT_64K			0x05
#define BC_IE_CAP_64K_PREFERRED			0x06
#define BC_IE_CAP_2_64K_PREFERRED		0x07
#define BC_IE_CAP_384K_UNRESTRICTED		0x08
#define BC_IE_CAP_1536K_UNRESTRICTED		0x09
#define BC_IE_CAP_1920K_UNRESTRICTED		0x10


#define BC_IE_UIL1P_V110			0x01
#define BC_IE_UIL1P_G711_ULAW			0x02
#define BC_IE_UIL1P_G711_ALAW			0x03
#define BC_IE_UIL1P_G721			0x04
#define BC_IE_UIL1P_H221			0x05
#define BC_IE_UIL1P_H223			0x06
#define BC_IE_UIL1P_NON_ITU			0x07
#define BC_IE_UIL1P_V120			0x08
#define BC_IE_UIL1P_X31				0x09


#define BC_IE_STRING_SZ 100
typedef struct bearer_cap_to_str
{
	int bearer_cap;
	char sbearer_cap[BC_IE_STRING_SZ];	
}bearer_cap_to_str_t;

typedef struct bearer_cap_to_str_array
{
	char sbearer_cap[BC_IE_STRING_SZ];	
}bearer_cap_to_str_array_t;


typedef struct uil1p_to_str
{
	int uil1p;
	char suil1p[BC_IE_STRING_SZ];
}uil1p_to_str_t;

typedef struct uil1p_to_str_array
{
	char suil1p[BC_IE_STRING_SZ];	
}uil1p_to_str_array_t;

bearer_cap_to_str_t bearer_cap2str[] = {
{BC_IE_CAP_SPEECH,"SPEECH"},
{BC_IE_CAP_SPARE,"SPARE"},
{BC_IE_CAP_64K_UNRESTRICTED,"64K_UNRESTRICTED"},
{BC_IE_CAP_3_1KHZ_AUDIO,"3_1KHZ_AUDIO"},
{BC_IE_CAP_ALT_SPEECH,"ALTERNATE_SPEECH"},
{BC_IE_CAP_ALT_64K,"ALTERNATE_64K"},
{BC_IE_CAP_64K_PREFERRED,"64K_PREFERRED"},
{BC_IE_CAP_2_64K_PREFERRED,"2x64K_PREFERRED"},
{BC_IE_CAP_384K_UNRESTRICTED,"384K_UNRESTRICTED"},
{BC_IE_CAP_1536K_UNRESTRICTED,"1536K_UNRESTRICTED"},
{BC_IE_CAP_1920K_UNRESTRICTED,"1920K_UNRESTRICTED"},
{255,"Unknown"},
};

uil1p_to_str_t uil1p2str[] = {
{BC_IE_UIL1P_V110,"V_110"},
{BC_IE_UIL1P_G711_ULAW,"G_711_ULAW"},
{BC_IE_UIL1P_G711_ALAW,"G_711_ALAW"},
{BC_IE_UIL1P_G721,"G_721"},
{BC_IE_UIL1P_H221,"H_221"},
{BC_IE_UIL1P_H223,"H_223"},
{BC_IE_UIL1P_NON_ITU,"Non_ITU_Standard"},
{BC_IE_UIL1P_V120,"V_120"},
{BC_IE_UIL1P_X31,"X_31"},
{255,"Unknown"},
};

extern bearer_cap_to_str_array_t bearer_cap_to_str_array[];
extern uil1p_to_str_array_t uil1p_to_str_array[];

static inline void bearer_cap_setup(void)
{
	int i=0;
	
	for (i=0;i<255;i++) {
		strncpy(bearer_cap_to_str_array[i].sbearer_cap, "Unknown", BC_IE_STRING_SZ-1);
	}
	
	for (i=0;i<255;i++) {
		if (bearer_cap2str[i].bearer_cap == 255) {
			break;
		}
		
		strncpy(bearer_cap_to_str_array[bearer_cap2str[i].bearer_cap].sbearer_cap,
		        bearer_cap2str[i].sbearer_cap,
			BC_IE_STRING_SZ-1);	
	}
#if 0
	for (i=0; i< 255;i++) {
		log_printf(SMG_LOG_ALL,server.log,"CAUSE=%i, SCAUSE=%s\n",
			i, bearer_cap_to_str_array[i].sbearer_cap);
	}
#endif
}

static inline char *bearer_cap_to_str(int bearer_cap)
{
	if (bearer_cap < 0 || bearer_cap >= 255) {
		return "Unknown";
	}

#if 0
	log_printf(SMG_LOG_ALL,server.log, "%s:%d: CAUSE = %i SCAUSE=%s\n",
			__FUNCTION__,__LINE__,cause,q931_cause_to_str_array[cause].scause);
#endif
	return bearer_cap_to_str_array[bearer_cap].sbearer_cap;
}

static inline int bearer_cap_to_code(char* sbearer_cap)
{
	int size;
	int i;
	size = sizeof(bearer_cap2str)/sizeof(bearer_cap_to_str_t);

	for (i=0; i<size; i++) {
		if (!strcasecmp(bearer_cap2str[i].sbearer_cap, sbearer_cap)) {
			return bearer_cap2str[i].bearer_cap;
		}
	}
	return 0;
}

static inline void uil1p_to_str_setup(void)
{
	int i=0;
	
	for (i=0;i<255;i++) {
		strncpy(uil1p_to_str_array[i].suil1p, "Unknown", BC_IE_STRING_SZ-1);
	}
	
	for (i=0;i<255;i++) {
		if (uil1p2str[i].uil1p == 255) {
			break;
		}
		
		strncpy(uil1p_to_str_array[uil1p2str[i].uil1p].suil1p,
		        uil1p2str[i].suil1p,
			BC_IE_STRING_SZ-1);	
	}
#if 0
	for (i=0; i< 255;i++) {
		log_printf(SMG_LOG_ALL,server.log,"CAUSE=%i, SCAUSE=%s\n",
			i, uil1p_to_str_array[i].suil1p);
	}
#endif
}

static inline int bearer_cap_is_audio(int cap)
{
        switch (cap) {
        case BC_IE_CAP_SPEECH:
        case BC_IE_CAP_3_1KHZ_AUDIO:
        case BC_IE_CAP_ALT_SPEECH:
                return 1;

        default:
                return 0;
        }

        return 0;
}


static inline char *uil1p_to_str(int uil1p)
{
	if (uil1p < 0 || uil1p >= 255) {
		return "Unknown";
	}

#if 0
	log_printf(SMG_LOG_ALL,server.log, "%s:%d: CAUSE = %i SCAUSE=%s\n",
			__FUNCTION__,__LINE__,cause,q931_cause_to_str_array[cause].scause);
#endif
	return uil1p_to_str_array[uil1p].suil1p;
}

static int uil1p_to_code(char* suil1p)
{
	int size;
	int i;
	size = sizeof(uil1p2str)/sizeof(uil1p_to_str_t);

	for (i=0; i<size; i++) {
		if (!strcasecmp(uil1p2str[i].suil1p, suil1p)) {
			return uil1p2str[i].uil1p;
		}
	}
	return 0;
}

#endif /* __SMG_CAPABILITIES_H__ */
