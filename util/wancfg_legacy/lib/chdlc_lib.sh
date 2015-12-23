#!/bin/sh

function chdlc_interface_init () {
	
	local num=$1


	if [ -z "${MULTICAST[$num]}" ]; then
		MULTICAST[$num]=NO	
	fi
	if [ -z "${DCD[$num]}" ]; then
		DCD[$num]=NO	
	fi
	if [ -z "${CTS[$num]}" ]; then
		CTS[$num]=NO	
	fi
	if [ -z "${KEEP[$num]}" ]; then
		KEEP[$num]=NO	
	fi
	if [ -z "${STREAM[$num]}" ]; then
		STREAM[$num]=NO	
	fi
	if [ -z "${TXTIME[$num]}" ]; then
		TXTIME[$num]=10000	
	fi
	if [ -z "${RXTIME[$num]}" ]; then
		RXTIME[$num]=11000	
	fi
	if [ -z "${ERR[$num]}" ]; then
		ERR[$num]=5	
	fi
	if [ -z "${SLARP[$num]}" ]; then
		SLARP[$num]=0	
	fi
	if [ -z "${TRUE_ENCODING[$num]}" ]; then
		TRUE_ENCODING[$num]=NO
	fi


	if [ $PROTOCOL = WAN_MULTPPP ] || [ $PROTOCOL = WAN_MULTPROT ]; then
		STREAM[$num]=YES
		DCD[$num]=NO
		CTS[$num]=NO
		KEEP[$num]=YES
		SLARP[$num]=0	
	fi
}


function chdlc_interface_setup () {

	local rc
	local num="$1"
	local choice
	local menu_options

	chdlc_interface_init $num

	if [ -z "${CHDLC_IPMODE[$num]}" ]; then
		CHDLC_IPMODE[$num]=NO;
		SLARP[$num]=0;
	fi

	if [ $PROTOCOL = WAN_CHDLC ]; then
	menu_options="'get_slarp'     'Dynamic IP Addressing ----> ${CHDLC_IPMODE[$num]}' \
		      'get_multicast' 'Multicast  ---------------> ${MULTICAST[$num]}' \
		      'get_ig_dcd'    'Ignore DCD  --------------> ${DCD[$num]}' \
	              'get_ig_cts'    'Ignore CTS  --------------> ${CTS[$num]}' \
		      'get_ig_keep'   'Ignore Keepalive ---------> ${KEEP[$num]}' \
		      'get_stream'    'HDLC Streaming -----------> ${STREAM[$num]}' \
		      'get_tx_time'   'Keep Alive Tx Timer ------> ${TXTIME[$num]}' \
		      'get_rx_time'   'Keep Alive Rx Timer ------> ${RXTIME[$num]}' \
		      'get_err_mar'   'Keep Alive Error Margin --> ${ERR[$num]}' \
		      'get_encoding'  'True_Encoding_Type -------> ${TRUE_ENCODING[$num]}' 2> $MENU_TMP"

	else
	menu_options="'no_opt'   'NO OPTIONS' 2> $MENU_TMP"
	fi
	
	menu_instr="Please specify the ${IF_NAME[$num]}: CHDLC protocol parameters below\
				"

	menu_name "CHDLC ${IF_NAME[$num]} PROTOCOL SETUP" "$menu_options" 9 "$menu_instr" "$BACK" 20 
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	case $choice in 
				get_tx_time)
					get_integer "Please specify the Keepalive Tx Timer" "10000" "0"  "60000" "interface_setup_help get_tx_time" 
					TXTIME[$num]=$($GET_RC)
					;;
				get_rx_time)
					get_integer "Please specify the Keepalive Rx Timer" "11000" "10"  "60000" "interface_setup_help get_rx_time"
					RXTIME[$num]=$($GET_RC)
					;;
				get_err_mar)
					get_integer "Please specify the Keepalive Error Margin" "5"    "1"   "20" "interface_setup_help get_err_mar"
					ERR[$num]=$($GET_RC)
					;;
				get_slarp)
					warning get_slarp
					if [ $? -eq 0 ]; then	
						CHDLC_IPMODE[$num]=YES
						SLARP[$num]=1000
					else	
						CHDLC_IPMODE[$num]=NO
						SLARP[$num]=0
					fi
					;;
				*)	
					$choice "$num"    #Choice is a command 
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


function get_commport () {

	local menu_options
	local rc
	local choice


	if [ $PROTOCOL = WAN_TTYPPP -a $TTY_MODE = ASYNC ]; then
	menu_options="'SEC'  'Secondary' 2> $MENU_TMP"

	else
	menu_options="'PRI'  'Primary (default)' \
		      'SEC'  'Secondary' 2> $MENU_TMP"

	fi

	menu_instr="Please select the CHDLC Communication Port \
						"

	menu_name "${IF_NAME[$num]} COMM PORT CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	COMMPORT=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "get_commport"
			get_commport
			;;
		*) 	return 1
			;;
	esac

}

function get_connection () 
{

	local menu_options
	local rc
	local choice


	menu_instr="Please select the CHDLC Connection Type \
						"
	menu_options="'Permanent' 'Permanent (default)' \
		      'Switched'  'Switched' 2> $MENU_TMP"

	menu_name "${IF_NAME[$num]} CONNECTION TYPE " "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	CONNECTION=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "get_connection"
			get_connection
			;;
		*) 	return 1
			;;
	esac

}

function get_linecode () {

	local menu_options
	local rc
	local choice


	menu_instr="Please select the CHDLC Line Coding Type \
						"
	menu_options="'NRZ'   'NRZ (default)' \
		      'NRZI'  'NRZI' 2> $MENU_TMP"

	menu_name "${IF_NAME[$num]} LINE CODING TYPE " "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	LINECODING=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "get_linecode"
			get_linecode
			;;
		*) 	return 1
			;;
	esac

}

function get_lineidle () {

	local menu_options
	local rc
	local choice


	menu_instr="Please select the CHDLC Line Idle Type \
						"
	menu_options="'FLAG'  'Idle Flag (default)' \
		      'MARK'  'Idle Mark' 2> $MENU_TMP"

	menu_name "${IF_NAME[$num]} LINE IDLE CONFIG " "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	LINEIDLE=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "get_lineidle"
			get_lineidle
			;;
		*) 	return 1
			;;
	esac

}



function get_rec_only () {

	local menu_options
	local rc
	local choice

	menu_options="'NO'   'Disable Receive Only Comm. (default)' \
		      'YES'  'Enable Receive Only Comm.' 2> $MENU_TMP"


	menu_instr="Enable or Disable Receive Only Communications \
						"

	menu_name "${IF_NAME[$num]} RECEIVE ONLY CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	REC_ONLY=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "get_rec_only"
			get_commport
			;;
		*) 	return 1
			;;
	esac

}


function get_ig_dcd () {
 
	local menu_options
	local rc
	local choice
	local num=$1;

	menu_options="'NO'   'Monitor Modem DCD (Default)' \
		      'YES'  'Ignore Modem DCD' 2> $MENU_TMP"

	menu_instr="Enable or Disable DCD Monitoring \
						"

	menu_name "${IF_NAME[$num]}: DCD MONITORING SETUP" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	DCD[$num]=$choice     #Choice is a command 
			return 0
			;;
		2)	interface_setup_help "get_ig_dcd"
			get_ig_dcd "$1"
			;;
		*) 	return 1
			;;
	esac
}

function get_ig_cts () {
 
	local menu_options
	local rc
	local choice
	local num=$1;

	menu_options="'NO'   'Monitor Modem CTS (Default)' \
		      'YES'  'Ignore Modem CTS' 2> $MENU_TMP"

	menu_instr="Enable or Disable CTS Monitoring \
						"

	menu_name "${IF_NAME[$num]} CTS MONITORING SETUP" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	CTS[$num]=$choice     #Choice is a command 
			return 0
			;;
		2)	interface_setup_help "get_ig_cts"
			get_ig_cts "$1"
			;;
		*) 	return 1
			;;
	esac
}

function get_modem_ignore () {
 
	local menu_options
	local rc
	local choice
	local num=$1;

	menu_options="'YES'  'Ignore Modem Status (Default)' \
		      'NO'   'Reset Protocol on Modem change' 2> $MENU_TMP"

	menu_instr="Enable or Disable Modem Monitoring \
						"

	menu_name "${IF_NAME[$num]}: MODEM MONITORING SETUP" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	MPPP_MODEM_IGNORE[$num]=$choice     #Choice is a command 
			return 0
			;;
		2)	interface_setup_help "get_modem_ignore"
			get_modem_ignore "$1"
			;;
		*) 	return 1
			;;
	esac
}



function get_ig_keep () {
 
	local menu_options
	local rc
	local choice
	local num=$1;

	menu_options="'NO'   'Activate Keep Alive (Default)' \
		      'YES'  'Ignore Keep Alive' 2> $MENU_TMP"

	menu_instr="Enable or Disable Keep Alive Monitoring \
					"

	menu_name "${IF_NAME[$num]} KEEP ALIVE SETUP" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	KEEP[$num]=$choice     #Choice is a command 
			return 0
			;;
		2)	interface_setup_help "get_ig_keep"
			get_ig_keep "$1"
			;;	
		*) 	return 1
			;;
	esac
}

function get_stream () {
 
	local menu_options
	local rc
	local choice
	local num=$1;

	menu_options="'NO'   'Disable HDLC Streaming (Default)' \
		      'YES'  'Enable HDLC Streaming' 2> $MENU_TMP"

	menu_instr="Enable or Disable HDLC Streaming \
						"

	menu_name "${IF_NAME[$num]}: HDLC STREAMING CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	STREAM[$num]=$choice     #Choice is a command 
			if [ $choice = YES ]; then
				DCD[$num]=YES
				CTS[$num]=YES
				KEEP[$num]=YES
			else
				DCD[$num]=NO
				CTS[$num]=NO
				KEEP[$num]=NO
			fi
			return 0
			;;
		2)	interface_setup_help "get_stream"
			get_stream "$1"
			;;
		*) 	return 1
			;;
	esac
}
