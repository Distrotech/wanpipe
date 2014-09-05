#!/bin/sh

function fr_interface_setup () {

	local rc
	local num="$1"
	local choice
	local menu_options


	if [ -z "${CIR[$num]}" ]; then
		CIR[$num]=NO
	fi
	if [ -z "${BC[$num]}" ]; then
		BC[$num]=NO
	fi
	if [ -z "${BE[$num]}" ]; then
		BE[$num]=NO
	fi
	if [ -z "${MULTICAST[$num]}" ]; then
		MULTICAST[$num]=NO	
	fi
	if [ -z "${INARP[$num]}" ]; then
		INARP[$num]=NO
	fi
	if [ -z "${INARP_INT[$num]}" ]; then
		INARP_INT[$num]=0
	fi
	if [ -z "${TRUE_ENCODING[$num]}" ]; then
		TRUE_ENCODING[$num]=NO
	fi

	INARP_RX[$num]=${INARP_RX[$num]:-NO}
	IPX[$num]=${IPX[$num]:-NO}
	NETWORK[$num]=${NETWORK[$num]:-0xDEADBEEF}
	
	if [ ${CIR[$num]} = NO ]; then
	
	menu_options="'get_cir'       'CIR       -----------> ${CIR[$num]}' \
		      'get_multicast' 'MULTICAST -----------> ${MULTICAST[$num]}' \
		      'get_inarp'     'TX INARP  -----------> ${INARP[$num]}' \
		      'get_inarp_int' 'INARP_INT  ----------> ${INARP_INT[$num]}' \
		      'get_inarp_rx'  'RX INARP ------------> ${INARP_RX[$num]}' \
		      'get_ipx'       'IPX Support ---------> ${IPX[$num]}' \
		      'get_network'   'IPX Network Addr ----> ${NETWORK[$num]}' \
		      'get_encoding'  'True_Encoding_Type --> ${TRUE_ENCODING[$num]}' 2> $MENU_TMP"
	menu_size=8
	else
	menu_options="'get_cir'       'CIR        ----------> ${CIR[$num]}' \
		      'get_bc'        'BC         ----------> ${BC[$num]}' \
		      'get_be'        'BE         ----------> ${BE[$num]}' \
		      'get_multicast' 'MULTICAST  ----------> ${MULTICAST[$num]}' \
		      'get_inarp'     'TX INARP   ----------> ${INARP[$num]}' \
		      'get_inarp_int' 'INARP_INT  ----------> ${INARP_INT[$num]}' \
		      'get_inarp_rx'  'RX INARP ------------> ${INARP_RX[$num]}' \
		      'get_ipx'       'IPX Support ---------> ${IPX[$num]}' \
		      'get_network'   'IPX Network Addr ----> ${NETWORK[$num]}' \
		      'get_encoding'  'True_Encoding_Type --> ${TRUE_ENCODING[$num]}' 2> $MENU_TMP"
	menu_size=10
	fi

	menu_instr="Please configure ${IF_NAME[$num]}: Frame Relay parameters below  		
						"

	menu_name "${IF_NAME[$num]}: FRAME RELAY PROTOCOL SETUP" "$menu_options" "$menu_size" "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	
			case $choice in

			get_network*) 
				get_string "Please specify you IPX Network Number" "0xABCDEFAB" 
				NETWORK[$num]=$($GET_RC)				
				;;
			get_inarp_int*)
				if [ ${INARP[$num]} = YES ]; then
					arp_dflt=10
				else
					arp_dflt=0
				fi
				get_integer "Please specify InARP interval" "$arp_dflt" "0" "100" "interface_setup_help get_inarp_int" 
				INARP_INT[$num]=$($GET_RC)
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

		*)	if [ ${CIR[$num]} != NO ]; then	
				if [ ${BC[$num]} = NO ]; then
					error "CIR_BC" $num
					return 0
				fi
				if [ ${BE[$num]} = NO ]; then
					error "CIR_BE" $num
					return 0
				fi
			fi
			return 1
			;;
	esac

}

function get_fs_issue () {

	local menu_options
	local rc
	local choice
	local num="$1"

	menu_options="'YES' 'Issue Full Status on startup (default)' \
		      'NO'  'Disable Full Status on startup' 2> $MENU_TMP"

	menu_instr="Please Enable or Disable:  Frame Relay full status issue on startup \
				"

	menu_name "${IF_NAME[$num]}: FULL STATUS ON STARTUP" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	FR_ISSUE_FS=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "get_fs_issue"
			get_fs_issue "$1"
			;;
		*) 	return 1
			;;
	esac

}


function get_inarp () {

	local menu_options
	local rc
	local choice
	local num="$1"

	menu_options="'NO'  'Disable Inverse Arp (default)' \
		      'YES' 'Enable Inverse Arp' 2> $MENU_TMP"

	menu_instr="Please Enable or Disable Inverse ARP Option \
				"

	menu_name "${IF_NAME[$num]}: INVERSE ARP SETUP" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	INARP[$num]=$choice     #Choice is a command 
			if [ "$choice" = YES ]; then
				INARP_RX[$num]=YES
				INARP_INT[$num]=10
			else
				INARP_RX[$num]=NO
			fi
			return 0
			;;
		2)	interface_setup_help "get_inarp"
			get_inarp "$1"
			;;
		*) 	return 1
			;;
	esac

}

function get_inarp_rx () {

	local menu_options
	local rc
	local choice
	local num="$1"

	menu_options="'NO'  'Ignore Incoming Inverse Arps (default)' \
		      'YES' 'Handle Incoming Inverse Arps' 2> $MENU_TMP"

	menu_instr="Please Enable or Disable Incoming Inverse ARP Support \
				"

	menu_name "${IF_NAME[$num]}: INVERSE ARP RECEPTION SETUP" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	if [ ${INARP[$num]} != YES ]; then	
				INARP_RX[$num]=$choice     #Choice is a command 
			else
				INARP_RX[$num]=YES
			fi
			return 0
			;;
		2)	interface_setup_help "get_inarp_rx"
			get_inarp "$1"
			;;
		*) 	return 1
			;;
	esac

}


function get_be () {
 
	local menu_options
	local rc
	local choice
	local num="$1"

	menu_options="'NO' 'Disable BE (Default)' \
		      'custom' 'Custom Config' 2> $MENU_TMP"

	menu_instr="Please Enable or Disable BE Option\
						"

	menu_name "${IF_NAME[$num]}: BE CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	case $choice in 
				custom)
					get_integer "Please specify BE in Kpbs" "0" "0" "512" "interface_setup_help get_be"
					choice=$($GET_RC) 	
				;;
			esac
			BE[$num]=$choice     #Choice is a command 
			return 0
			;;
		2)	interface_setup_help "get_be"
			get_be "$1"
			;;
		*) 	return 1
			;;
	esac
}

function get_bc () {
 
	local menu_options
	local rc
	local choice
	local num="$1"

	menu_options="'NO' 'Disable BC (Default)' \
		      'custom' 'Custom Config' 2> $MENU_TMP"


	menu_instr="Please Enable or Disable BC Option \
						"

	menu_name "${IF_NAME[$num]}: BC CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 
			case $choice in 
				custom)
					get_integer "Please specify BC in Kpbs" "64" "16" "512" "interface_setup_help get_bc"
					choice=$($GET_RC) 	
				;;
			esac
			BC[$num]=$choice     #Choice is a command 
			return 0
			;;
		2)	interface_setup_help "get_bc"
			get_bc "$1"
			;;
		*) 	return 1
			;;
	esac
}

function get_cir () {
 
	local menu_options
	local rc
	local choice
	local num=$1;

	menu_options="'NO' 'Disable CIR (Default)' \
		      'custom' 'Custom Configure' 2> $MENU_TMP"

	menu_instr="Please Enable or Disable CIR Option \
						"

	menu_name "${IF_NAME[$num]}: CIR CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	case $choice in 
				custom)
					get_integer "Please specify CIR in Kpbs" "64" "16" "512" "interface_setup_help get_cir"
 
					choice=$($GET_RC) 	
				;;
			esac

			CIR[$num]=$choice     #Choice is a command 
			if [ $choice = NO ]; then
				BC[$num]=$choice;
				BE[$num]=$choice;
			fi
			return 0
			;;
		2)	interface_setup_help "get_cir"
			get_cir "$1"
			;;
		*) 	return 1
			;;
	esac
}


function get_station () {
 
	local menu_options
	local rc
	local choice
	local num=$1

	menu_options="'CPE'     'CPE' \
		      'NODE'    'Node (switch emulation)' 2> $MENU_TMP"

	menu_instr="Please specify the Frame Relay Station Type \
				"

	menu_name "FRAME RELAY STATION SETUP" "$menu_options" 2 "$menu_instr" "$BACK"	
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	if [ -z $num ]; then
				STATION=$choice     #Choice is a command 
			else
				STATION_IF[$num]=$choice
			fi
			return 0
			;;
		2)	
			device_setup_help "get_station"	
			get_station
			;;
		*) 	return 1
			;;
	esac
}

function get_signal () {
 
	local menu_options
	local rc
	local choice
	local num=$1
	local node=NO

	if [ -z $num ]; then
		if [ $STATION = NODE ]; then
			node=YES
		fi
	else
		if [ ${STATION_IF[$num]} = NODE ]; then
			node=YES
		fi
	fi


	if [ $node = YES ]; then

		menu_options="'ANSI' 'ANSI' \
                	      'LMI'  'LMI' \
		      	      'Q933' 'Q933' \
			      'NO'   'No signalling' 2> $MENU_TMP"
	else
		
		if [ -z $num ]; then

		menu_options="'ANSI' 'ANSI' \
                	      'LMI'  'LMI' \
		      	      'Q933' 'Q933' 2> $MENU_TMP"
		else

		menu_options="'AUTO' 'Auto Detect' \
			      'ANSI' 'ANSI' \
                	      'LMI'  'LMI' \
		      	      'Q933' 'Q933' 2> $MENU_TMP"
		fi

	fi

	menu_instr="Please specify Frame Relay Signalling \
						"

	menu_name "FRAME RELAY SIGNALLING CONFIGURATION" "$menu_options" 4 "$menu_instr" "$BACK"	
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	if [ -z $num ]; then
				SIGNAL=$choice     #Choice is a command 
			else
				SIGNAL_IF[$num]=$choice
			fi
			return 0
			;;
		2)
			device_setup_help "get_signal"
			get_signal
			;;
		*) 	return 1
			;;
	esac
}

