%{

/* Any C code */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(__WINDOWS__)
#include "wanpipe_includes.h"
#include "unistd.h"
#else
#include <fcntl.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/ioctl.h>
#endif

#include "wan_ec_argy.h"
#include "wan_ecmain.h"
#include "wanec_api.h"

#define WAN_EC_VERSION	"1.3"

#if !defined(YYLMAX)
#define YYLMAX		512
#endif
#define token(x)	x
#define END(v)		(v-1+sizeof(v)/sizeof(v[0]))
#define YY_NO_UNPUT
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size)			\
	{						\
		char c;					\
		if (targv < arglim){			\
			c = targv[0][offset++];		\
			if (c == '\n'){			\
				result = 0;		\
			}else if (c==' ' || c=='\0'){	\
				targv++;		\
				offset = 0;		\
				buf[0] = '\n';		\
				result = 1;		\
			}else{				\
				buf[0] = c;		\
				result = 1;		\
			}				\
		}else{					\
			result = 0;			\
		}					\
	}

char		**targv;
static char	**arglim;
unsigned	offset = 0;

extern wanec_client_t	ec_client;
int			action = WAN_EC_ACT_CMD;
char			idstr[YYLMAX];
int			gl_err = 0;

extern long convert_str(char* str, int type);
extern void yyerror(char*);
extern int yyparse();
void yy_reset(void);

static void s_lookup(int);
static int lookup_name(void);
int help(int);
static int help1(int);


%}

sign			[-]
letter			[a-zA-Z_]
exp_letter		[a-zA-Z_\x80-\xFF]
digit			[0-9]
hex			[a-fA-F0-9]
power			[Ee][+-]?{digit}+
float_suffix		(f|F|l|L)
integer_suffix		("u"|"U"|"l"|"L"|"ul"|"lu"|"Ul"|"lU"|"uL"|"Lu"|"UL"|"LU")
letter_or_digit		[a-zA-Z_0-9]
exp_letter_or_digit	[a-zA-Z_0-9\x80-\xFF]
blank			[ \t]

%%

"-v"				{ ec_client.verbose = 1; }
"-vv"				{ ec_client.verbose = 2; }
"--h"				{ help(1); exit(0); }
"?"					{ help(1); exit(0); }
"/?"				{ help(1); exit(0); }
"help"				{ help(1); exit(0); }
"--h1"				{ help(1); exit(0); }
"--help"			{ help(1); exit(0); }
"-help"				{ help(1); exit(0); }

{digit}{digit}*{integer_suffix}?	{ /* decimal */ 
				s_lookup(token(DEC_CONSTANT));
				return token(DEC_CONSTANT);
				}

"t"{digit}{digit}*	{ /* decimal */ 
			s_lookup(token(DIAL_STRING));
			return token(DIAL_STRING);
			}

{letter}{letter_or_digit}*	{ return lookup_name(); }

"-"				{ return token('-'); }
"."				{ return token('.'); }
"="				{ return token('='); }

"--"				{ return token(CUSTOM_PARAM_TOKEN); }

.				{ 
					//return token(yytext[0]);
					s_lookup(token(CHAR_STRING));
					return token(CHAR_STRING);
				}

%%

static struct table_args_t {
	char*	name;
	int 	value;
	char*	descr;
} table_args[] = {
	{"alaw",		token(PCM_ALAW_TOKEN),			"Description" },
	{"all",			token(ALL_TOKEN),				"Description" },
	{"all_ports",	token(ALL_PORT_TOKEN),			"Description" },
	{"bd",			token(BYPASS_DISABLE_TOKEN),	"Description" },
	{"be",			token(BYPASS_ENABLE_TOKEN),		"Description" },
	{"buffer_load",	token(BUFFER_LOAD_TOKEN),		"Description" },
	{"buffer_unload",	token(BUFFER_UNLOAD_TOKEN),	"Description" },
	{"config",		token(CONFIG_TOKEN),			"Description" },
	{"dd",			token(DTMF_DISABLE_TOKEN),		"Description" },
	{"de",			token(DTMF_ENABLE_TOKEN),		"Description" },
	{"disable",		token(DISABLE_TOKEN),			"Description" },
	{"duration",	token(DURATION_TOKEN),			"Description" },
	{"enable",		token(ENABLE_TOKEN),			"Description" },
	{"fd",			token(FAX_DISABLE_TOKEN),				"Description" },
	{"fe",			token(FAX_ENABLE_TOKEN),				"Description" },
	{"help",		token(HELP_TOKEN),				"Description" },
	{"help1",		token(HELP1_TOKEN),				"Description" },
	{"hwimage",		token(HWIMAGE_TOKEN),			"Description" },
	{"kill",		token(KILL_TOKEN),				"Description" },
	{"load",		token(LOAD_TOKEN),				"Description" },
	{"mhtf",		token(MODE_HT_FREEZE_TOKEN),	"Description" },
	{"mhtr",		token(MODE_HT_RESET_TOKEN),		"Description" },
	{"mn",			token(MODE_NORMAL_TOKEN),		"Description" },
	{"mne",			token(MODE_NO_ECHO_TOKEN),		"Description" },
	{"modify",		token(MODIFY_TOKEN),			"Description" },
	{"monitor",		token(MONITOR_TOKEN),			"Description" },
	{"monitor120",	token(MONITOR120_TOKEN),		"Description" },
	{"mpd",			token(MODE_POWERDOWN_TOKEN),	"Description" },
	{"msr",			token(MODE_SPEECH_RECOGNITION_TOKEN),	"Description" },
	{"mute",		token(MUTE_TOKEN),				"Description" },
	{"playout_start",	token(PLAYOUT_START_TOKEN),	"Description" },
	{"playout_stop",	token(PLAYOUT_STOP_TOKEN),	"Description" },
	{"release",		token(RELEASE_TOKEN),			"Description" },
	{"repeat",		token(REPEAT_TOKEN),			"Description" },
	{"rin",			token(RIN_PORT_TOKEN),			"Description" },
	{"rout",		token(ROUT_PORT_TOKEN),			"Description" },
	{"sin",			token(SIN_PORT_TOKEN),			"Description" },
	{"sout",		token(SOUT_PORT_TOKEN),			"Description" },
	{"stats",		token(STATS_TOKEN),				"Description" },
	{"stats_full",	token(STATS_FULL_TOKEN),		"Description" },
	{"test",		token(TEST_TOKEN),				"Description" },
	{"ulaw",		token(PCM_ULAW_TOKEN),			"Description" },
	{"unload",		token(UNLOAD_TOKEN),			"Description" },
	{"unmute",		token(UNMUTE_TOKEN),			"Description" }
};

static int lookup_name()
{
	struct table_args_t	*first_arg = table_args,
				*last_arg = END(table_args),
				*mid_arg;
	int			res = 0;

	while(first_arg <= last_arg){
		mid_arg = first_arg + (last_arg - first_arg) / 2;
		res = strcmp(mid_arg->name, yytext);
		if (res == 0){
			res = strlen(mid_arg->name) - strlen(yytext);			
			if (res == 0) return mid_arg->value;
		}
		if (res < 0){
			first_arg = mid_arg + 1;
		}else{
			last_arg = mid_arg - 1;
		}
	}

	s_lookup(token(CHAR_STRING));
	return token(CHAR_STRING);
}

#ifndef YYSTYPE
static char* strsave(char* str)
{
	long	len = strlen(str);
	char*	new_str = NULL;
	
	if (len > YYLMAX){
		yyerror("Line is to long!");
		return NULL;
	}
	new_str = calloc(len, sizeof(char));
	strlcpy(new_str, str, len);
	return new_str;
}
#endif

static void s_lookup(int yylex)
{
#ifdef YYSTYPE
	switch(yylex){
	case DEC_CONSTANT:
	case HEX_CONSTANT:
		memset(idstr, 0, YYLMAX*sizeof(char));
		strlcpy(idstr, yytext, YYLMAX);
		yylval.val = convert_str(idstr, yylex);
		//yylval.str = idstr;
		break;
	case CHAR_STRING:
		memset(idstr, 0, YYLMAX*sizeof(char));
		strlcpy(idstr, yytext, YYLMAX);
		yylval.str = idstr;
		break;
	case DIAL_STRING:
		memset(idstr, 0, YYLMAX*sizeof(char));
		strlcpy(idstr, yytext, YYLMAX);
		yylval.str = idstr;
		break;
	}	
#else
	yylval = strsave(yytext);
#endif
}

void yy_reset()
{
	yy_start = 0;
	yyleng = 0;
}

int yywrap(void) { return 1; }

static void help_extra(int verbose)
{
	printf(" where:\n");
	printf("    fe_chan values:\n");
	printf("        1,2,..,24       - for T1 line\n");
	printf("        1,2,..,31       - for E1 line\n");
	printf("        all             - all channels\n");
	printf("\n");
	printf("    port values:\n");
	printf("      sin|sout|rin|rout - EC port\n");
	printf("      all_ports         - all EC ports\n");
	printf("\n");
	printf("Examples\n");
	printf("wan_ec_client wanpipe1 enable  all         #Enable ec on all channels\n");
	printf("wan_ec_client wanpipe1 enable  1-15.17-31  #Enable ec on 1 to 15 and 17 to 31 channels\n");
	printf("wan_ec_client wanpipe1 disable all         #Disable ec on all channels\n");
	printf("wan_ec_client wanpipe1 disable 1-15.17-31  #Disable ec on 1 to 15 and 17 to 31 channels\n");
	printf("wanpipemon -i w1g1 -c ehw                  #List currently active ec on span 1\n");
	printf("wan_ec_client stats_full 1                 #Read full config and stats of channel 1\n");
	printf("wan_ec_client stats 1                      #Read abbreviated config and stats of channel 1\n");
	printf("wan_ec_client stats                        #Read global config and stats\n");
	printf("wan_ec_client wanpipe1 modify all --WANEC_DtmfToneRemoval=TRUE\n");
	printf("wan_ec_client wanpipe1 modify all --WANEC_DtmfToneRemoval=FALSE\n");
	printf("\nFor More info and detailed list of modify options\nDownload ftp://ftp.sangoma.com/linux/current_wanpipe/doc/WanpipeEC_API.pdf\n");

	return;
}

int help(int verbose)
{
	printf("\nWanpipe Echo Canceller setup utility (ver. %s)\n\n",
					WAN_EC_VERSION);
	printf("Usage:\n");
	printf(" wan_ec_client <devname> <commands>\n");
	printf("\n");
	printf(" Commands are:\n");
	printf("  config              - Configure all Echo Canceller channels\n");
	printf("                        in Normal mode!\n");
	printf("  release             - Close/Release all Echo Canceller channels\n");
	printf("  enable <fe_chan>    - Enable Echo canceller on specified channel(s)\n");
	printf("  disable <fe_chan>   - Disable Echo canceller on specified channel(s)\n");
	printf("  de <fe_chan> [sout|rout] - Enable DTMF detection on specified channel(s)\n");
	printf("  dd <fe_chan> [sout|rout] - Disable DTMF detection on specified channel(s)\n");
	printf("  stats                - Read Echo Canceller Chip/Image statistis\n");
	printf("  stats <fe_chan>      - Read Echo Canceller channel statistis\n");
	printf("  hwimage              - Read Echo Canceller image statistis\n");
	printf("  monitor <fe_chan>    - Enable recording for 15sec on specified channel\n");
	printf("  monitor120 <fe_chan> - Enable recording for 120sec on specified channel\n");
	printf("  monitor              - Read last recording for previously specified channel\n");
	printf("\n");
	help1(0);
	help_extra(verbose);
	return 0;
}	

static int help1(int verbose)
{
	printf(" Special commands are:\n");
	printf("  be   <fe_chan>               - Enable Echo canceller bypass mode\n");
	printf("  bd   <fe_chan>               - Disable Echo canceller bypass mode\n");
	printf("  mn   <fe_chan>               - Set Echo canceller in Normal mode\n");
	printf("  mpd  <fe_chan>               - set Echo canceller in Power-Down mode\n");
	printf("  mhtf <fe_chan>               - set Echo canceller in HT Freeze mode\n");
	printf("  mhtr <fe_chan>               - set Echo canceller in HT Reset mode\n");
	printf("  mne  <fe_chan>               - set Echo canceller in No Echo mode\n");
	printf("  msr  <fe_chan>               - set Echo canceller in Speech Recognition mode\n");
	printf("  buffer_load <name>           - Load specific buffer (name.pcm)\n");
	printf("  buffer_unload <index>        - Unload specific buffer (index \n");
	printf("                                 returns by buffer_load command)\n");
	printf("  playout_start <fe_chan> <id> [duration <num> | repeat <cnt> <port>]\n");
	printf("                               - Start playout specific bufer\n");
	printf("  playout_stop <fe_chan> <id> [port] - Stop playout specific buffer\n");
	printf("  stats_full <fe_chan>         - Read full Echo Canceller channel statistis\n");
	printf("  modify <fe_chan>             - Modify channel configuration parameters\n");
	printf("  mute <channel> [ports]       - Mute specified channels on ports\n");
	printf("  unmute <channel> [ports]     - Un-Mute specified channels on ports\n");
	printf("\n");
	if (verbose) help_extra(verbose);
	return 0;
}

int wan_ec_args_parse_and_run(int argc, char* argv[])
{
	if (argc == 1){
		help(1);
		return 1;
	}
	targv = argv+1;
	arglim = argv+argc;
	yyparse();
	return gl_err;
}
