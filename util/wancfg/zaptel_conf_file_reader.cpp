/***************************************************************************
                          zaptel_conf_file_reader.cpp  -  description
                             -------------------
    begin                : Wed Nov 23 2005
    author            	 : David Rokhvarg <davidr@sangoma.com>
			   This class will parse the 'zaptel.conf' file.
			   Code based on on ztcfg.c from Zaptel package.
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "wancfg.h"
#include "zaptel_conf_file_reader.h"
#include "conf_file_writer.h"

#if defined(__LINUX__)
# include <zapcompat_user.h>
#else
#endif


#define DBG_ZAP_CONF_FILE_READER	1

#define DEBUG_PARSER 0x0001
#define DEBUG_READER 0x0002

/****** Global Data *********************************************************/
int debug = 0;
int errcnt = 0;
int spans = 0;
int numdynamic = 0;
//it is assumed that Analog devices use the same law.
int global_law = ZT_LAW_MULAW;

int first_spano = -1;

struct zt_lineconfig lc[ZT_MAX_SPANS];
#define NUM_DYNAMIC	1024
struct zt_dynamic_span zds[NUM_DYNAMIC];

int slineno[ZT_MAX_CHANNELS];	/* Line number where signalling specified */
const char *sig[ZT_MAX_CHANNELS];
struct zt_chanconfig cc[ZT_MAX_CHANNELS];

struct handler {
	char *keyword;
	int (zaptel_conf_file_reader::*func)(char *keyword, char *args);
};

struct handler handlers[] =
{
{ "span",	&zaptel_conf_file_reader::spanconfig 	},
{ "e&m",	&zaptel_conf_file_reader::chanconfig	},
{ "e&me1",	&zaptel_conf_file_reader::chanconfig	},
{ "fxsls",	&zaptel_conf_file_reader::chanconfig	},
{ "fxsgs",	&zaptel_conf_file_reader::chanconfig	},
{ "fxsks",	&zaptel_conf_file_reader::chanconfig 	},
{ "fxols",	&zaptel_conf_file_reader::chanconfig	},
{ "fxogs",	&zaptel_conf_file_reader::chanconfig 	},
{ "fxoks",	&zaptel_conf_file_reader::chanconfig 	},
{ "rawhdlc",	&zaptel_conf_file_reader::chanconfig 	},
{ "nethdlc",	&zaptel_conf_file_reader::chanconfig 	},
{ "fcshdlc",	&zaptel_conf_file_reader::chanconfig 	},
{ "dchan",	&zaptel_conf_file_reader::chanconfig 	},
{ "bchan",	&zaptel_conf_file_reader::chanconfig 	},
{ "indclear",	&zaptel_conf_file_reader::chanconfig 	},
{ "clear",	&zaptel_conf_file_reader::chanconfig 	},
{ "unused",	&zaptel_conf_file_reader::chanconfig 	},
{ "cas",	&zaptel_conf_file_reader::chanconfig 	},
{ "dacs",	&zaptel_conf_file_reader::chanconfig 	},
{ "dacsrbs",	&zaptel_conf_file_reader::chanconfig 	},
{ "user",	&zaptel_conf_file_reader::chanconfig 	},
{ "alaw", 	&zaptel_conf_file_reader::setlaw 	},
{ "mulaw", 	&zaptel_conf_file_reader::setlaw 	},
{ "deflaw", 	&zaptel_conf_file_reader::setlaw 	},

{ "dynamic", 	&zaptel_conf_file_reader::discard 	},
{ "loadzone", 	&zaptel_conf_file_reader::discard 	},
{ "defaultzone",&zaptel_conf_file_reader::discard 	},
{ "ctcss", 	&zaptel_conf_file_reader::discard	},
{ "rxdcs", 	&zaptel_conf_file_reader::discard	},
{ "tx", 	&zaptel_conf_file_reader::discard	},
{ "debouncetime",&zaptel_conf_file_reader::discard	},
{ "bursttime", 	&zaptel_conf_file_reader::discard	},
{ "exttone", 	&zaptel_conf_file_reader::discard	},
{ "invertcor", 	&zaptel_conf_file_reader::discard	},
{ "corthresh",	&zaptel_conf_file_reader::discard	},
{ "channel",	&zaptel_conf_file_reader::discard	},
{ "channels",	&zaptel_conf_file_reader::discard	}
};

const char *lbostr[] = {
"0 db (CSU)/0-133 feet (DSX-1)",
"133-266 feet (DSX-1)",
"266-399 feet (DSX-1)",
"399-533 feet (DSX-1)",
"533-655 feet (DSX-1)",
"-7.5db (CSU)",
"-15db (CSU)",
"-22.5db (CSU)"
};

const char *laws[] = {
	"Default",
	"Mu-law",
	"A-law"
};

/****** End of Global Data *********************************************************/
/***********************************************************************************/

zaptel_conf_file_reader::zaptel_conf_file_reader(char* file_path)
{
  Debug(DBG_ZAP_CONF_FILE_READER, ("zaptel_conf_file_reader::zaptel_conf_file_reader()\n"));
  zaptel_conf_path = file_path;
}

zaptel_conf_file_reader::~zaptel_conf_file_reader()
{
  Debug(DBG_ZAP_CONF_FILE_READER, ("zaptel_conf_file_reader::~zaptel_conf_file_reader()\n"));
}

void zaptel_conf_file_reader::trim(char *buf)
{
	/* Trim off trailing spaces, tabs, etc */
	while(strlen(buf) && (buf[strlen(buf) -1] < 33)){
		buf[strlen(buf) -1] = '\0';
	}
}

int zaptel_conf_file_reader::error(char *fmt, ...)
{
	int res;
	static int shown=0;
	va_list ap;

	if (!shown) {
		//printf("Notice: Configuration file is %s\n", zaptel_conf_path);
		shown++;
	}
	res = printf("line %d: ", lineno);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	errcnt++;
	return res;
}

char* zaptel_conf_file_reader::readline()
{
	static char buf[256];
	char *c;
	do {
		if (!fgets(buf, sizeof(buf), cf)){
			return NULL;
		}
		/* Strip comments */
		c = strchr(buf, '#');
		if (c){
			*c = '\0';
		}
		trim(buf);
		lineno++;
	} while (!strlen(buf));
	return buf;
}

int zaptel_conf_file_reader::parseargs(char *input, char *output[], int maxargs, char sep)
{
	char *c;
	int pos=0;
	c = input;
	output[pos++] = c;
	while(*c) {
		while(*c && (*c != sep)){
			c++;
		}
		if (*c) {
			*c = '\0';
			c++;
			while(*c && (*c < 33)){
				c++;
			}
			if (*c)  {
				if (pos >= maxargs){
					return -1;
				}
				output[pos] = c;
				trim(output[pos]);
				pos++;
				output[pos] = NULL;
				/* Return error if we have too many */
			} else {
				return pos;
			}
		}
	}
	return pos;
}

//This is used to dicard data we are not interested in. 
int zaptel_conf_file_reader::discard(char *keyword, char *args)
{
	return 0;
}

const char *zaptel_conf_file_reader::sigtype_to_str(const int sig)
{
	switch (sig) {
	case 0:
		return "Unused";
	case ZT_SIG_EM:
		return "E & M";
	case ZT_SIG_EM_E1:
		return "E & M E1";
	case ZT_SIG_FXSLS:
		return "FXS Loopstart";
	case ZT_SIG_FXSGS:
		return "FXS Groundstart";
	case ZT_SIG_FXSKS:
		return "FXS Kewlstart";
	case ZT_SIG_FXOLS:
		return "FXO Loopstart";
	case ZT_SIG_FXOGS:
		return "FXO Groundstart";
	case ZT_SIG_FXOKS:
		return "FXO Kewlstart";
	case ZT_SIG_CAS:
		return "CAS / User";
	case ZT_SIG_DACS:
		return "DACS";
	case ZT_SIG_DACS_RBS:
		return "DACS w/RBS";
	case ZT_SIG_CLEAR:
		return "Clear channel";
	case ZT_SIG_SLAVE:
		return "Slave channel";
	case ZT_SIG_HDLCRAW:
		return "Raw HDLC";
	case ZT_SIG_HDLCNET:
		return "Network HDLC";
	case ZT_SIG_HDLCFCS:
		return "HDLC with FCS check";
	default:
		return "Unknown";
	}
}

int zaptel_conf_file_reader::setlaw(char *keyword, char *args)
{
	int res;
	int law;
	int x;
	int chans[ZT_MAX_CHANNELS];

	memset(&chans[0], 0x00, sizeof(int)*ZT_MAX_CHANNELS);
	res = apply_channels(chans, args);
	if (res <= 0){
		return -1;
	}
	if (!strcasecmp(keyword, "alaw")) {
		law = ZT_LAW_ALAW;
		global_law = ZT_LAW_ALAW;
	} else if (!strcasecmp(keyword, "mulaw")) {
		law = ZT_LAW_MULAW;
		global_law = ZT_LAW_MULAW;
	} else if (!strcasecmp(keyword, "deflaw")) {
		law = ZT_LAW_DEFAULT;
	} else {
		printf("Invalid '%s' law\n", keyword);
		return -1;
	}
	for (x=0;x<ZT_MAX_CHANNELS;x++) {
		if (chans[x]){
			cc[x].deflaw = law;
		}
	}
	return 0;
}

int zaptel_conf_file_reader::parse_channel(char *channel, int *startchan)
{
	if (!channel || (sscanf(channel, "%i", startchan) != 1) || 
		(*startchan < 1)) {
		error("DACS requires a starting channel in the form ':x' where x is the channel\n");
		return -1;
	}
	return 0;
}

int zaptel_conf_file_reader::apply_channels(int chans[], char *argstr)
{
	char *args[ZT_MAX_CHANNELS+1];
	char *range[3];
	int res,x, res2,y;
	int chan;
	int start, finish;
	char argcopy[256];
	res = parseargs(argstr, args, ZT_MAX_CHANNELS, ',');
	if (res < 0){
		error("Too many channels...  Max is %d\n", ZT_MAX_CHANNELS);
		return -1;		
	}
	for (x=0;x<res;x++) {
		if (strchr(args[x], '-')) {
			/* It's a range */
			strncpy(argcopy, args[x], sizeof(argcopy));
			res2 = parseargs(argcopy, range, 2, '-');
			if (res2 != 2) {
				error("Syntax error in range '%s'.  Should be <val1>-<val2>.\n", args[x]);
				return -1;
			}
			res2 =sscanf(range[0], "%i", &start);
			if (res2 != 1) {
				error("Syntax error.  Start of range '%s' should be a number from 1 to %d\n", 
					args[x], ZT_MAX_CHANNELS - 1);
				return -1;
			} else if ((start < 1) || (start >= ZT_MAX_CHANNELS)) {
				error("Start of range '%s' must be between 1 and %d (not '%d')\n",
					args[x], ZT_MAX_CHANNELS - 1, start);
				return -1;
			}
			res2 =sscanf(range[1], "%i", &finish);
			if (res2 != 1) {
				error("Syntax error.  End of range '%s' should be a number from 1 to %d\n",
					args[x], ZT_MAX_CHANNELS - 1);
				return -1;
			} else if ((finish < 1) || (finish >= ZT_MAX_CHANNELS)) {
				error("end of range '%s' must be between 1 and %d (not '%d')\n",
					args[x], ZT_MAX_CHANNELS - 1, finish);
				return -1;
			}
			if (start > finish) {
				error("Range '%s' should start before it ends\n", args[x]);
				return -1;
			}
			for (y=start;y<=finish;y++)
				chans[y]=1;
		} else {
			/* It's a single channel */
			res2 =sscanf(args[x], "%i", &chan);
			if (res2 != 1) {
				error("Syntax error.  Channel should be a number from 1 to %d, not '%s'\n",
					ZT_MAX_CHANNELS - 1, args[x]);
				return -1;
			} else if ((chan < 1) || (chan >= ZT_MAX_CHANNELS)) {
				error("Channel must be between 1 and %d (not '%d')\n",
					ZT_MAX_CHANNELS - 1, chan);
				return -1;
			}
			chans[chan]=1;
		}		
	}
	return res;
}

int zaptel_conf_file_reader::parse_idle(int *i, char *s)
{
	char a,b,c,d;
	if (s) {
		if (sscanf(s, "%c%c%c%c", &a,&b,&c,&d) == 4) {
			if (((a == '0') || (a == '1')) && ((b == '0') ||
				(b == '1')) && ((c == '0') || (c == '1')) &&
					((d == '0') || (d == '1'))) {
				*i = 0;
				if (a == '1') 
					*i |= ZT_ABIT;
				if (b == '1')
					*i |= ZT_BBIT;
				if (c == '1')
					*i |= ZT_CBIT;
				if (d == '1')
					*i |= ZT_DBIT;
				return 0;
			}
		}
	}
	error("CAS Signalling requires idle definition in the form ':xxxx' at the end of the channel definition, where xxxx represent the a, b, c, and d bits\n");
	return -1;
}

int zaptel_conf_file_reader::chanconfig(char *keyword, char *args)
{
	int chans[ZT_MAX_CHANNELS];
	int res = 0;
	int x;
	int master=0;
	int dacschan = 0;
	char *idle;
	bzero(chans, sizeof(chans));
	strtok(args, ":");
	idle = strtok(NULL, ":");
	if (!strcasecmp(keyword, "dacs") || !strcasecmp(keyword, "dacsrbs")) {
		res = parse_channel(idle, &dacschan);
	}
	if (!res){
		res = apply_channels(chans, args);
		if (debug & DEBUG_READER){
			printf("res: %d\n", res);
		}
	}
	if (res <= 0){
		return -1;
	}
	for (x=1;x<ZT_MAX_CHANNELS;x++)
	{
		if (chans[x]) {
			if (slineno[x]) {
				error("Channel %d already configured as '%s' at line %d\n",
					x, sig[x], slineno[x]);
				continue;
			}
			if ((!strcasecmp(keyword, "dacs") || !strcasecmp(keyword, "dacsrbs")) &&
											slineno[dacschan]) {
				error("DACS Destination channel %d already configured as '%s' at line %d\n",
					dacschan, sig[dacschan], slineno[dacschan]);
				continue;
			} else {
				cc[dacschan].chan = dacschan;
				cc[dacschan].master = dacschan;
				slineno[dacschan] = lineno;
			}
			cc[x].chan = x;
			cc[x].master = x;
			slineno[x] = lineno;
			if (!strcasecmp(keyword, "e&m")) {
				cc[x].sigtype = ZT_SIG_EM;
				sig[x] = sigtype_to_str(cc[x].sigtype);
			} else if (!strcasecmp(keyword, "e&me1")) {
				cc[x].sigtype = ZT_SIG_EM_E1;
				sig[x] = sigtype_to_str(cc[x].sigtype);
			} else if (!strcasecmp(keyword, "fxsls")) {
				cc[x].sigtype = ZT_SIG_FXSLS;
				sig[x] = sigtype_to_str(cc[x].sigtype);
			} else if (!strcasecmp(keyword, "fxsgs")) {
				cc[x].sigtype = ZT_SIG_FXSGS;
				sig[x] = sigtype_to_str(cc[x].sigtype);
			} else if (!strcasecmp(keyword, "fxsks")) {
				cc[x].sigtype = ZT_SIG_FXSKS;
				sig[x] = sigtype_to_str(cc[x].sigtype);
			} else if (!strcasecmp(keyword, "fxols")) {
				cc[x].sigtype = ZT_SIG_FXOLS;
				sig[x] = sigtype_to_str(cc[x].sigtype);
			} else if (!strcasecmp(keyword, "fxogs")) {
				cc[x].sigtype = ZT_SIG_FXOGS;
				sig[x] = sigtype_to_str(cc[x].sigtype);
			} else if (!strcasecmp(keyword, "fxoks")) {
				cc[x].sigtype = ZT_SIG_FXOKS;
				sig[x] = sigtype_to_str(cc[x].sigtype);
			} else if (!strcasecmp(keyword, "cas") || !strcasecmp(keyword, "user")) {
				if (parse_idle(&cc[x].idlebits, idle))
					return -1;
				cc[x].sigtype = ZT_SIG_CAS;
				sig[x] = sigtype_to_str(cc[x].sigtype);
			} else if (!strcasecmp(keyword, "dacs")) {
				/* Setup channel for monitor */
				cc[x].idlebits = dacschan;
				cc[x].sigtype = ZT_SIG_DACS;
				sig[x] = sigtype_to_str(cc[x].sigtype);
				/* Setup inverse */
				cc[dacschan].idlebits = x;
				cc[dacschan].sigtype = ZT_SIG_DACS;
				sig[x] = sigtype_to_str(cc[dacschan].sigtype);
				dacschan++;
			} else if (!strcasecmp(keyword, "dacsrbs")) {
				/* Setup channel for monitor */
				cc[x].idlebits = dacschan;
				cc[x].sigtype = ZT_SIG_DACS_RBS;
				sig[x] = sigtype_to_str(cc[x].sigtype);
				/* Setup inverse */
				cc[dacschan].idlebits = x;
				cc[dacschan].sigtype = ZT_SIG_DACS_RBS;
				sig[x] = sigtype_to_str(cc[dacschan].sigtype);
				dacschan++;
			} else if (!strcasecmp(keyword, "unused")) {
				cc[x].sigtype = 0;
				sig[x] = sigtype_to_str(cc[x].sigtype);
			} else if (!strcasecmp(keyword, "indclear") || !strcasecmp(keyword, "bchan")) {
				cc[x].sigtype = ZT_SIG_CLEAR;
				sig[x] = sigtype_to_str(cc[x].sigtype);
			} else if (!strcasecmp(keyword, "clear")) {
				sig[x] = sigtype_to_str(ZT_SIG_CLEAR);
				if (master) {
					cc[x].sigtype = ZT_SIG_SLAVE;
					cc[x].master = master;
				} else {
					cc[x].sigtype = ZT_SIG_CLEAR;
					master = x;
				}
			} else if (!strcasecmp(keyword, "rawhdlc")) {
				sig[x] = sigtype_to_str(ZT_SIG_HDLCRAW);
				if (master) {
					cc[x].sigtype = ZT_SIG_SLAVE;
					cc[x].master = master;
				} else {
					cc[x].sigtype = ZT_SIG_HDLCRAW;
					master = x;
				}
			} else if (!strcasecmp(keyword, "nethdlc")) {
				sig[x] = sigtype_to_str(ZT_SIG_HDLCNET);
				if (master) {
					cc[x].sigtype = ZT_SIG_SLAVE;
					cc[x].master = master;
				} else {
					cc[x].sigtype = ZT_SIG_HDLCNET;
					master = x;
				}
			} else if (!strcasecmp(keyword, "fcshdlc")) {
				sig[x] = sigtype_to_str(ZT_SIG_HDLCFCS);
				if (master) {
					cc[x].sigtype = ZT_SIG_SLAVE;
					cc[x].master = master;
				} else {
					cc[x].sigtype = ZT_SIG_HDLCFCS;
					master = x;
				}
			} else if (!strcasecmp(keyword, "dchan")) {
				sig[x] = "D-channel";
				cc[x].sigtype = ZT_SIG_HDLCFCS;
			} else {
				printf("Invalid keyword: '%s'\n", keyword);
			}
		}//if()
	}//for()
	return 0;
}

int zaptel_conf_file_reader::spanconfig(char *keyword, char *args)
{
	static char *realargs[10];
	int res;
	int argc;
	int span;
	int timing;

	argc = res = parseargs(args, realargs, 7, ',');
	if ((res < 5) || (res > 7)) {
		error("Incorrect number of arguments to 'span' (should be <spanno>,<timing>,<lbo>,<framing>,<coding>[, crc4 | yellow [, yellow]])\n");
		return -1;
	}
	res = sscanf(realargs[0], "%i", &span);
	if (res != 1) {
		error("Span number should be a valid span number, not '%s'\n", realargs[0]);
		return -1;
	}
	res = sscanf(realargs[1], "%i", &timing);
	if ((res != 1) || (timing < 0) || (timing > 15)) {
		error("Timing should be a number from 0 to 15, not '%s'\n", realargs[1]);
		return -1;
	}
	res = sscanf(realargs[2], "%i", &lc[spans].lbo);
	if (res != 1) {
		error("Line build-out (LBO) should be a number from 0 to 7 (usually 0) not '%s'\n",
			realargs[2]);
		return -1;
	}
	if ((lc[spans].lbo < 0) || (lc[spans].lbo > 7)) {
		error("Line build-out should be in the range 0 to 7, not %d\n", lc[spans].lbo);
		return -1;
	}
	if (!strcasecmp(realargs[3], "d4")) {
		lc[spans].lineconfig |= ZT_CONFIG_D4;
		lc[spans].lineconfig &= ~ZT_CONFIG_ESF;
		lc[spans].lineconfig &= ~ZT_CONFIG_CCS;
	} else if (!strcasecmp(realargs[3], "esf")) {
		lc[spans].lineconfig |= ZT_CONFIG_ESF;
		lc[spans].lineconfig &= ~ZT_CONFIG_D4;
		lc[spans].lineconfig &= ~ZT_CONFIG_CCS;
	} else if (!strcasecmp(realargs[3], "ccs")) {
		lc[spans].lineconfig |= ZT_CONFIG_CCS;
		lc[spans].lineconfig &= ~(ZT_CONFIG_ESF | ZT_CONFIG_D4);
	} else if (!strcasecmp(realargs[3], "cas")) {
		lc[spans].lineconfig &= ~ZT_CONFIG_CCS;
		lc[spans].lineconfig &= ~(ZT_CONFIG_ESF | ZT_CONFIG_D4);
	} else {
		error("Framing(T1)/Signalling(E1) must be one of 'd4', 'esf', 'cas' or 'ccs', not '%s'\n",
			realargs[3]);
		return -1;
	}
	if (!strcasecmp(realargs[4], "ami")) {
		lc[spans].lineconfig &= ~(ZT_CONFIG_B8ZS | ZT_CONFIG_HDB3);
		lc[spans].lineconfig |= ZT_CONFIG_AMI;
	} else if (!strcasecmp(realargs[4], "b8zs")) {
		lc[spans].lineconfig |= ZT_CONFIG_B8ZS;
		lc[spans].lineconfig &= ~(ZT_CONFIG_AMI | ZT_CONFIG_HDB3);
	} else if (!strcasecmp(realargs[4], "hdb3")) {
		lc[spans].lineconfig |= ZT_CONFIG_HDB3;
		lc[spans].lineconfig &= ~(ZT_CONFIG_AMI | ZT_CONFIG_B8ZS);
	} else {
		error("Coding must be one of 'ami', 'b8zs' or 'hdb3', not '%s'\n", realargs[4]);
		return -1;
	}
	if (argc > 5) {
		if (!strcasecmp(realargs[5], "yellow"))
			lc[spans].lineconfig |= ZT_CONFIG_NOTOPEN;
		else if (!strcasecmp(realargs[5], "crc4")) {
			lc[spans].lineconfig |= ZT_CONFIG_CRC4;
		} else {
			error("Only valid fifth arguments are 'yellow' or 'crc4', not '%s'\n",
				realargs[5]);
			return -1;
		}
	}
	if (argc > 6) {
		if (!strcasecmp(realargs[6], "yellow")){
			lc[spans].lineconfig |= ZT_CONFIG_NOTOPEN;
		}else{
			error("Only valid sixth argument is 'yellow', not '%s'\n", realargs[6]);
			return -1;
		}
	}

	lc[spans].span = span;
	if(first_spano == -1){
		first_spano = lc[spans].span;
		Debug(DBG_ZAP_CONF_FILE_READER, ("FIRST T1/E1 SPAN NUMBER: %d\n", first_spano));
	}
	lc[spans].sync = timing;
	/* Valid span */

	spans++;
	return 0;
}

int zaptel_conf_file_reader::create_wanpipe_config()
{
  sdla_fe_cfg_t	fe_cfg;
  list_element_sangoma_card *sangoma_card_ptr;
  int wanpipe_counter = 1;//start from wanpipe1.conf
  
  Debug(DBG_ZAP_CONF_FILE_READER, ("zaptel_conf_file_reader::create_wanpipe_config()\n"));

  sangoma_card_list sang_te1_card_list(WANOPT_AFT);
  sangoma_card_list sang_analog_card_list(WANOPT_AFT_ANALOG);

  menu_hardware_probe hw_probe_te1(&sang_te1_card_list);
  menu_hardware_probe hw_probe_analog(&sang_analog_card_list);

  hw_probe_te1.hardware_probe();
  hw_probe_analog.hardware_probe();

  if(sang_te1_card_list.get_size() == 0 && sang_analog_card_list.get_size() == 0){
    printf("\nError: No Sangoma cards detected on the system.\n");
    return NO_SANGOMA_CARDS_DETECTED;
  }

  if(spans != 0 && sang_te1_card_list.get_size() == 0) {
    printf("\nError: There are TE1 spans defined in zaptel.conf, but no Sangoma TE1 cards on the system.\n");
    return NO_SANGOMA_TE1_CARDS_DETECTED;
  }

/*
	here is the logic for further action:
	-------------------------------------
1. Number of TE1 spans is known from parsing of zaptel.conf.
2. The Span Number is known for each span and is assumed to be valid.
3. All "missing" spans starting from "1" up to 1-st TE1 span, assumed
   to be Analog. Therefore: go through the list of Detected Sangoma Analog
   cards and configure them (maximum is: "1-st TE1 span - 1", OR untill
   list of Analog cards is used up).
*/
  /////////////////////////////////////////////////////////////////////////////////////////
  //configure all Aanalog cards
  if(sang_analog_card_list.get_size() > 0){
    sangoma_card_ptr = (list_element_sangoma_card *)sang_analog_card_list.get_first();
    int max_number_of_analog_spans = 0;

    if(first_spano != -1){
      max_number_of_analog_spans = first_spano - 1;
    }else{
      max_number_of_analog_spans = sang_analog_card_list.get_size();
    }

    Debug(DBG_ZAP_CONF_FILE_READER, ("max_number_of_analog_spans: %d\n", max_number_of_analog_spans));

    for(int i = 0; i < max_number_of_analog_spans && sangoma_card_ptr != NULL; i++){
	//set the spanno following "Aanalog card" logic
	sangoma_card_ptr->set_spanno(i + 1);

	switch(sangoma_card_ptr->card_version)
	{
	case A200_ADPTR_ANALOG:
  		memset(&fe_cfg, 0x00, sizeof(sdla_fe_cfg_t));
		fe_cfg.tdmv_law = global_law;
		//set law for Analog card - note, using the same call as for TE1 card
		sangoma_card_ptr->set_fe_parameters(&fe_cfg);
		break;
	default:
		//all other cards are invalid
        	ERR_DBG_OUT(("Invalid AFT card version: 0x%x!\n", sangoma_card_ptr->card_version));
		return INVALID_CARD_VERSION;
	}

	conf_file_writer *conf_file_writer_ptr = new conf_file_writer(sangoma_card_ptr);
	if(conf_file_writer_ptr->write_wanpipe_zap_file(wanpipe_counter) != 0){
		delete conf_file_writer_ptr;
		return FAILED_TO_WRITE_CONF_FILE;
	}
	wanpipe_counter++;
	delete conf_file_writer_ptr;

      sangoma_card_ptr = 
		(list_element_sangoma_card *)sang_analog_card_list.get_next_element(sangoma_card_ptr);
    }//for()
  }//if(sang_analog_card_list.get_size() > 0)

  /////////////////////////////////////////////////////////////////////////////////////////
  //configure all TE1 cards
  if(sang_te1_card_list.get_size() > 0){
    sangoma_card_ptr = (list_element_sangoma_card *)sang_te1_card_list.get_first();
    
    if(first_spano == -1){
      //If no TE1 spans defined in zaptel.conf, there is no way to guess 
      //settings for Sangoma TE1 cards on the system.
      //It is NOT necessary an error - it is possible that Sangoma TE1 card(s)
      //not used for TDM Voice, but used for Data (Frame Relay).
      printf("Warning: Found Sangoma TE1 card(s) but no T1/E1 spans defined in '%s'.\n",
		zaptel_conf_path);
      return NO_TE1_SPAN_FOUND_IN_ZAPTEL_CONF;
    }
    
    Debug(DBG_ZAP_CONF_FILE_READER, ("number of te1 spans: %d\n", spans));

    for(int i = 0; i < spans && sangoma_card_ptr != NULL; i++){
	//set the spanno following "TE1 card" logic
	sangoma_card_ptr->set_spanno(lc[i].span);

	switch(sangoma_card_ptr->card_version)
	{
	case A101_ADPTR_1TE1:
	case A104_ADPTR_4TE1:
  		memset(&fe_cfg, 0x00, sizeof(sdla_fe_cfg_t));

		if(lc[i].lineconfig & ZT_CONFIG_D4 || lc[i].lineconfig & ZT_CONFIG_ESF){
			fe_cfg.media = WAN_MEDIA_T1;
			if(lc[i].lineconfig & ZT_CONFIG_D4){
				fe_cfg.frame = WAN_FR_D4;
			}else if(lc[i].lineconfig & ZT_CONFIG_ESF){
				fe_cfg.frame = WAN_FR_ESF;
			}

		}else{
			fe_cfg.media = WAN_MEDIA_E1;
			if(lc[i].lineconfig & ZT_CONFIG_CRC4){
				fe_cfg.frame = WAN_FR_CRC4;
			}else{
				fe_cfg.frame = WAN_FR_NCRC4;
			}
		}

		if(lc[i].lineconfig & ZT_CONFIG_AMI){
			fe_cfg.lcode = WAN_LCODE_AMI;
		}else if(lc[i].lineconfig & ZT_CONFIG_B8ZS){
			fe_cfg.lcode = WAN_LCODE_B8ZS;
		}else{
			fe_cfg.lcode = WAN_LCODE_HDB3;
		}
		
		fe_cfg.tx_tristate_mode = WANOPT_NO;

		if(sangoma_card_ptr->card_version == A101_ADPTR_1TE1){
			fe_cfg.line_no = 1;
		}else{
			fe_cfg.line_no = sangoma_card_ptr->line_no;
		}

#if 0
		if(lc[i].sync == 1){
			/*This span is "clock reference" span, not much we can do,
			because we do NOT know if spans following this one, is on
			the same card.
			//fe_cfg.cfg.te_cfg.te_ref_clock = fe_cfg.line_no;//A104
			//fe_cfg.cfg.te_cfg.te_ref_clock = fe_cfg.line_no;//A102 - depends on CPU?
			OR:
			store PCI BUS/SLOT of this span, set "clock reference"
			on all the following on	the same bus/slog.
			*/
		}
#endif
		fe_cfg.cfg.te_cfg.active_ch 	= ENABLE_ALL_CHANNELS;
		fe_cfg.cfg.te_cfg.te_clock 	= WAN_NORMAL_CLK;
		fe_cfg.cfg.te_cfg.lbo 		= WAN_T1_LBO_0_DB;

		//Has to be done AFTER spanno is set and Line type (T1 or E1) is known too!!
		if(check_span_has_a_dchan(lc[i].span, fe_cfg.media) == YES){
			if(fe_cfg.media == WAN_MEDIA_T1){
				sangoma_card_ptr->set_dchan(24);
			}else{
				sangoma_card_ptr->set_dchan(16);
			}
		}

		//set fe cfg
		sangoma_card_ptr->set_fe_parameters(&fe_cfg);
		break;
	default:
		//all other cards are invalid
        	ERR_DBG_OUT(("Invalid AFT card version: 0x%x!\n", sangoma_card_ptr->card_version));
		return INVALID_CARD_VERSION;
	}

	conf_file_writer *conf_file_writer_ptr = new conf_file_writer(sangoma_card_ptr);
	if(conf_file_writer_ptr->write_wanpipe_zap_file(wanpipe_counter) != 0){
		delete conf_file_writer_ptr;
		return FAILED_TO_WRITE_CONF_FILE;
	}
	wanpipe_counter++;
	delete conf_file_writer_ptr;

      sangoma_card_ptr = 
		(list_element_sangoma_card *)sang_te1_card_list.get_next_element(sangoma_card_ptr);
    }//for()
  }//if(sang_analog_card_list.get_size() > 0)

  return 0;
}

int zaptel_conf_file_reader::check_span_has_a_dchan(int span, unsigned char media)
{
	int search_start_offset;
	int search_length;

	if(media == WAN_MEDIA_T1){
		search_length = 24;
	}else{
		search_length = 31;
	}
	search_start_offset = (span - 1) * search_length + 1;//note, starting minimum is 1.
			
	Debug(DBG_ZAP_CONF_FILE_READER, ("search_start_offset: %d, search_length: %d\n",
		search_start_offset, search_length));

	for(int i = search_start_offset; 
	        (i < search_start_offset + search_length) && (i < ZT_MAX_CHANNELS); i++){

		if(cc[i].sigtype == ZT_SIG_HDLCFCS){
			Debug(DBG_ZAP_CONF_FILE_READER, ("----------------> found Dchan: i=%d\n", i));
			return YES;
		}
	} 
	return NO;
}

/*
	This function used for debugging only. Do NOT place any real code in it!!
*/
void zaptel_conf_file_reader::printconfig()
{
	int x,y;
	int ps;
	int configs=0;
	printf("\nZaptel Configuration\n"
	       "======================\n\n");
	for (x=0;x<spans;x++) {
		printf("SPAN %d(real spanno %d), (sync clock %d): %3s/%4s Build-out: %s\n",
		       x+1, lc[x].span, lc[x].sync,
				( lc[x].lineconfig & ZT_CONFIG_D4 ? "D4" :
			      	lc[x].lineconfig & ZT_CONFIG_ESF ? "ESF" :
			      	lc[x].lineconfig & ZT_CONFIG_CCS ? "CCS" : "CAS" ),

				( lc[x].lineconfig & ZT_CONFIG_AMI ? "AMI" :
			  	lc[x].lineconfig & ZT_CONFIG_B8ZS ? "B8ZS" :
			  	lc[x].lineconfig & ZT_CONFIG_HDB3 ? "HDB3" : "???" ),
				lbostr[lc[x].lbo]);
	}//for()

	//not interested in dynamic channels, but print anyway for clarity.
	for (x=0;x<numdynamic;x++) {
		printf("Dynamic span %d: driver %s, addr %s, channels %d, timing %d\n",
		       x +1, zds[x].driver, zds[x].addr, zds[x].numchans, zds[x].timing);
	}//for()

	printf("\nChannel map:\n\n");
	for (x=1;x<ZT_MAX_CHANNELS;x++) {
		if ((cc[x].sigtype != ZT_SIG_SLAVE) && (cc[x].sigtype)) {
			configs++;
			ps = 0;
			if ((cc[x].sigtype & __ZT_SIG_DACS) == __ZT_SIG_DACS){
				printf("Channel %02d %s to %02d", x, sig[x], cc[x].idlebits);
			}else{
				printf("Channel %02d: %s (%s)", x, sig[x], laws[cc[x].deflaw]);
				for (y=1;y<ZT_MAX_CHANNELS;y++) {
					if (cc[y].master == x)  {
						printf("%s%02d", ps++ ? " " : " (Slaves: ", y);
					}
				}
			}
			if (ps) printf(")\n"); else printf("\n");
		} else {
			if (cc[x].sigtype) configs++;
		}
	}//for()
/*
	//this is the laconic version
	for (x=1;x<ZT_MAX_CHANNELS;x++) 
		if (cc[x].sigtype)
			configs++;
*/
	printf("\n%d channels configured.\n\n", configs);
}

int zaptel_conf_file_reader::read_file()
{
	char *buf;
	char *key, *value;
	int x,found;

	lineno = 0;
	errcnt = 0;
	spans = 0;
	numdynamic = 0;

  	//debug |= DEBUG_PARSER;
  	//debug |= DEBUG_READER;

	memset(&lc[0], 0x00, sizeof(struct zt_lineconfig)*ZT_MAX_SPANS);
	memset(&zds[0], 0x00, sizeof(struct zt_dynamic_span)*NUM_DYNAMIC);

	cf = fopen(zaptel_conf_path, "r");
	if (cf == NULL){
		error("Unable to open configuration file '%s'\n", zaptel_conf_path);
		return 1;
	}

	printf("\nReading '%s'...\n", zaptel_conf_path);

	while((buf = readline())) {
		if (debug & DEBUG_READER){
			printf("Line %d: %s\n", lineno, buf);
		}

		key = value = buf;
		while(value && *value && (*value != '=')){
			value++;
		}

		if (value){
			*value='\0';
		}

		if (value){
			value++;
		}

		while(value && *value && (*value < 33)){
			value++;
		}

		if (*value) {
			trim(key);
			if (debug & DEBUG_PARSER){
				printf("Keyword: [%s], Value: [%s]\n", key, value);
			}
		}else{
			error("Syntax error.  Should be <keyword>=<value>\n");
			return 1;
		}
		found=0;

		for (x=0; x < (int)(sizeof(handlers) / sizeof(handlers[0]));x++) {
			if (!strcasecmp(key, handlers[x].keyword)) {
				found++;
				if((this->*handlers[x].func)(key, value) != 0){
					return 1;
				}
				break;
			}
		}
		if (!found) {
			error("Unknown keyword '%s'\n", key);
			return 1;
		}
	}//while()

	if (debug & DEBUG_READER){
		printf("<End of File>\n");
	}
	fclose(cf);
	
	printf("Done.\n\n");

	return 0;
}

