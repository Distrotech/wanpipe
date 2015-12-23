function ss7_interface_init () {
	
	LINE_CONFIG_OPTIONS=${LINE_CONFIG_OPTIONS:-0x0001}
	MODEM_CONFIG_OPTIONS=${MODEM_CONFIG_OPTIONS:-0x0000}
	MODEM_STATUS_TIMER=${MODEM_STATUS_TIMER:-500}
	API_OPTIONS=${API_OPTIONS:-0x0000} 
	PROTOCOL_OPTIONS=${PROTOCOL_OPTIONS:-0x0000}
	PROTOCOL_SPECIFICATION=${PROTOCOL_SPECIFICATION:-ANSI} 
	STATS_HISTORY_OPTIONS=${STATS_HISTORY_OPTIONS:-0x001F}
	MAX_LENGTH_MSU_SIF=${MAX_LENGTH_MSU_SIF:-272}
	MAX_UNACKED_TX_MSUS=${MAX_UNACKED_TX_MSUS:-10}
	LINK_INACTIVITY_TIMER=${LINK_INACTIVITY_TIMER:-5}
	T1_TIMER=${T1_TIMER:-1300}
	T2_TIMER=${T2_TIMER:-1150}
	T3_TIMER=${T3_TIMER:-1150}
	T4_TIMER_EMERGENCY=${T4_TIMER_EMERGENCY:-60}
	T4_TIMER_NORMAL=${T4_TIMER_NORMAL:-230}
	T5_TIMER=${T5_TIMER:-10}
	T6_TIMER=${T6_TIMER:-400}
	T7_TIMER=${T7_TIMER:-100}
	T8_TIMER=${T8_TIMER:-100}
	N1=${N1:-1}
	N2=${N2:-2}
	TIN=${TIN:-10}
	TIE=${TIE:-11}
	SUERM_ERROR_THRESHOLD=${SUERM_ERROR_THRESHOLD:-20}
	SUERM_NUMBER_OCTETS=${SUERM_NUMBER_OCTETS:-21}
	SUERM_NUMBER_SUS=${SUERM_NUMBER_SUS:-1024}
	SIE_INTERVAL_TIMER=${SIE_INTERVAL_TIMER:-30}
	SIO_INTERVAL_TIMER=${SIO_INTERVAL_TIMER:-31}
	SIOS_INTERVAL_TIMER=${SIOS_INTERVAL_TIMER:-32}
	FISU_INTERVAL_TIMER=${FISU_INTERVAL_TIMER:-33} 
}



function ss7_interface_setup () {

	local rc
	local num="$1"
	local choice
	local menu_options

	ss7_interface_init $num

	menu_options="'get_LINE_CONFIG_OPTIONS' 'Line Config --------> $LINE_CONFIG_OPTIONS' \
		      'get_MODEM_CONFIG_OPTION' 'Modem Config -------> $MODEM_CONFIG_OPTIONS' \
		      'get_MODEM_STATUS_TIMER'  'Modem Status Timer -> $MODEM_STATUS_TIMER' \
		      'get_API_OPTIONS'         'Api Options --------> $API_OPTIONS' \
		      'get_PROTOCOL_OPTIONS'    'Protocol Options ---> $PROTOCOL_OPTIONS' \
		      'get_ss7_signal'          'Protocol Spec. -----> $PROTOCOL_SPECIFICATION' \
		      'get_STATS_HISTORY'       'Status History -----> $STATS_HISTORY_OPTIONS' \
		      'get_MAX_LENGTH_MSU_SIF'  'Max MSU SIF Length -> $MAX_LENGTH_MSU_SIF' \
		      'get_MAX_UNACKED_TX_MSUS' 'Max Unacked Tx MSU -> $MAX_UNACKED_TX_MSUS' \
		      'get_LINK_INACT_TIMER'    'Link Inact. Timer --> $LINK_INACTIVITY_TIMER' \
		      'get_T1_TIMER'            'T1 Timer -----------> $T1_TIMER' \
		      'get_T2_TIMER'            'T2 Timer -----------> $T2_TIMER' \
		      'get_T3_TIMER'            'T3 Timer -----------> $T3_TIMER' \
		      'get_T4_TIMER_EMERG'      'T4 Timer Emergency -> $T4_TIMER_EMERGENCY' \
		      'get_T4_TIMER_NORMAL'     'T4 Timer Normal ----> $T4_TIMER_NORMAL' \
		      'get_T5_TIMER'            'T5 Timer -----------> $T5_TIMER' \
		      'get_T6_TIMER'            'T6 Timer -----------> $T6_TIMER' \
		      'get_T7_TIMER'            'T7 Timer -----------> $T7_TIMER' \
		      'get_T8_TIMER'            'T8 Timer -----------> $T8_TIMER' \
		      'get_N1'                  'N1 -----------------> $N1' \
		      'get_N2'                  'N2 -----------------> $N2' \
		      'get_TIN'                 'TIN ----------------> $TIN' \
		      'get_TIE'                 'TIE ----------------> $TIE' \
		      'get_SUERM_ERROR_THR'     'Suerm Err Thresh. --> $SUERM_ERROR_THRESHOLD' \
		      'get_SUERM_NUMBER_OCTETS' 'Suerm Num Octects --> $SUERM_NUMBER_OCTETS' \
		      'get_SUERM_NUMBER_SUS'    'Suerm Num Sus ------> $SUERM_NUMBER_SUS' \
		      'get_SIE_INTERVAL_TIMER'  'Sie Interval Timer -> $SIE_INTERVAL_TIMER' \
		      'get_SIO_INTERVAL_TIMER'  'Sio Interval Timer -> $SIO_INTERVAL_TIMER' \
		      'get_SIOS_INTERVAL_TIMER' 'Sios Interv. Timer -> $SIOS_INTERVAL_TIMER' \
		      'get_FISU_INTERVAL_TIMER' 'Fisu Interv. Timer -> $FISU_INTERVAL_TIMER' 2> $MENU_TMP"

	menu_instr="Please specify the ${IF_NAME[$num]}: SS7 protocol parameters below\
				"

	menu_name "SS7 ${IF_NAME[$num]} PROTOCOL SETUP" "$menu_options" 12 "$menu_instr" "$BACK" 
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	case $choice in 

			get_LINE_CONFIG_OPTIONS)
				get_string "Please specify Line Config Options" "$LINE_CONFIG_OPTIONS"
				LINE_CONFIG_OPTIONS=$($GET_RC)
				;;
				
			get_MODEM_CONFIG_OPTION)
				get_string "Please specify MODEM_CONFIG_OPTIONS" "$MODEM_CONFIG_OPTIONS"
				MODEM_CONFIG_OPTIONS=$($GET_RC)
				;;
		        
			get_MODEM_STATUS_TIMER)
				get_string "Please specify MODEM_STATUS_TIMER" "$MODEM_STATUS_TIMER"
				MODEM_STATUS_TIMER=$($GET_RC)
				;;
		        get_API_OPTIONS)
				get_string "Please specify API_OPTIONS" "$API_OPTIONS"
				API_OPTIONS=$($GET_RC)
				;;
		      	get_PROTOCOL_OPTIONS)
				get_string "Please specify PROTOCOL_OPTIONS" "$PROTOCOL_OPTIONS"
				PROTOCOL_OPTIONS=$($GET_RC)
				;;
		      	get_STATS_HISTORY)
				get_string "Please specify STATS_HISTORY_OPTIONS" "$STATS_HISTORY_OPTIONS"
				STATS_HISTORY_OPTIONS=$($GET_RC)	
				;;
		        get_MAX_LENGTH_MSU_SIF)
				get_string "Please specify MAX_LENGTH_MSU_SIF" "$MAX_LENGTH_MSU_SIF"
				MAX_LENGTH_MSU_SIF=$($GET_RC)	
				;;
		      	get_MAX_UNACKED_TX_MSUS)
				get_string "Please specify MAX_UNACKED_TX_MSUS" "$MAX_UNACKED_TX_MSUS"
				MAX_UNACKED_TX_MSUS=$($GET_RC)	
				;;
		      	get_LINK_INACT_TIMER)
				get_string "Please specify LINK_INACTIVITY_TIMER" "$LINK_INACTIVITY_TIMER"
				LINK_INACTIVITY_TIMER=$($GET_RC)	
				;;
		      	get_T1_TIMER)
				get_string "Please specify T1_TIMER" "$T1_TIMER"
				T1_TIMER=$($GET_RC)
				;;
		      	get_T2_TIMER)
				get_string "Please specify T2_TIMER" "$T2_TIMER"
				T2_TIMER=$($GET_RC)
				;;
		      	get_T3_TIMER)
				get_string "Please specify T3_TIMER" "$T3_TIMER"
				T3_TIMER=$($GET_RC)
				;;
		      	get_T4_TIMER_EMERG)
				get_string "Please specify T4_TIMER_EMERGENCY" "$T4_TIMER_EMERGENCY"
				T4_TIMER_EMERGENCY=$($GET_RC)
				;;
		      	get_T4_TIMER_NORMAL)
				get_string "Please specify T4_TIMER_NORMAL" "$T4_TIMER_NORMAL"
				T4_TIMER_NORMAL=$($GET_RC)
				;;
		      	get_T5_TIMER)
				get_string "Please specify T5_TIMER" "$T5_TIMER"
				T5_TIMER=$($GET_RC)
				;;
		      	get_T6_TIMER)
				get_string "Please specify T6_TIMER" "$T6_TIMER"
				T6_TIMER=$($GET_RC)
				;;
		      	get_T7_TIMER)
				get_string "Please specify T7_TIMER" "$T7_TIMER"
				T7_TIMER=$($GET_RC)
				;;
		      	get_T8_TIMER)
				get_string "Please specify T8_TIMER" "$T8_TIMER"
				T8_TIMER=$($GET_RC)
				;;
		      	get_N1)
				get_string "Please specify N1" "$N1"
				N1=$($GET_RC)
				;;
		      	get_N2)
				get_string "Please specify N2" "$N2"
				N2=$($GET_RC)
				;;
		      	get_TIN)
				get_string "Please specify TIN" "$TIN"
				TIN=$($GET_RC)
				;;
		      	get_TIE)
				get_string "Please specify TIE" "$TIE"
				TIE=$($GET_RC)
				;;
				
		      	get_SUERM_ERROR_THR)
				get_string "Please specify SUERM_ERROR_THRESHOLD" "$SUERM_ERROR_THRESHOLD"	
				SUERM_ERROR_THRESHOLD=$($GET_RC)	
				;;
		      	get_SUERM_NUMBER_OCTETS)
				get_string "Please specify SUERM_NUMBER_OCTETS" "$SUERM_NUMBER_OCTETS"
				SUERM_NUMBER_OCTETS=$($GET_RC)	
				;;
		      	get_SUERM_NUMBER_SUS)
				get_string "Please specify SUERM_NUMBER_SUS" "$SUERM_NUMBER_SUS"
				SUERM_NUMBER_SUS=$($GET_RC)	
				;;
		      	get_SIE_INTERVAL_TIMER)
				get_string "Please specify SIE_INTERVAL_TIMER" "$SIE_INTERVAL_TIMER"
				SIE_INTERVAL_TIMER=$($GET_RC)	
				;;
		      	get_SIO_INTERVAL_TIMER)
				get_string "Please specify SIO_INTERVAL_TIMER" "$SIO_INTERVAL_TIMER"
				SIO_INTERVAL_TIMER=$($GET_RC)	
				;;
		      	get_SIOS_INTERVAL_TIMER)
				get_string "Please specify SIOS_INTERVAL_TIMER" "$SIOS_INTERVAL_TIMER"
				SIOS_INTERVAL_TIMER=$($GET_RC)	
				;;
		      	get_FISU_INTERVAL_TIMER)
				get_string "Please specify FISU_INTERVAL_TIMER" "$FISU_INTERVAL_TIMER"	
				FISU_INTERVAL_TIMER=$($GET_RC)
				;;
			*)
				$choice    #Choice is a command 
				;;	
			esac
			return 0	
			;;
			
		2) 	choice=${choice%\"*\"}
			choice=${choice// /}
			interface_setup_help $choice	
			return 0
			;;
		*)
			return 1
			;;
	esac
}

function get_ss7_signal () {
 
	local menu_options
	local rc
	local choice


	menu_options="'ANSI' 'ANSI' \
               	      'ITU'  'ITU' \
	      	      'NTT'  'NTT' 2> $MENU_TMP"

	menu_instr="Please specify SS7 Signalling \
						"

	menu_name "SS7 SIGNALLING CONFIGURATION" "$menu_options" 4 "$menu_instr" "$BACK"	
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	PROTOCOL_SPECIFICATION=$choice     #Choice is a command 
			return 0
			;;
		2)
			device_setup_help "get_ss7_signal"
			get_ss7_signal
			;;
		*) 	return 1
			;;
	esac
}

