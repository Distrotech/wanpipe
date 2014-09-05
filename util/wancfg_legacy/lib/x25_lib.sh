#!/bin/sh

function x25_interface_setup () {

	local rc
	local num="$1"
	local choice
	local menu_options


	IDLE_TIMEOUT[$num]=${IDLE_TIMEOUT[$num]:-90}
	HOLD_TIMEOUT[$num]=${HOLD_TIMEOUT[$num]:-10}
	SRC_ADDR[$num]=${SRC_ADDR[$num]}
		

	menu_options="'get_idle_timeout'   	'Idle Timeout      -------> ${IDLE_TIMEOUT[$num]}' \
		      'get_hold_timeout'   	'Hold Timeout      -------> ${HOLD_TIMEOUT[$num]}' 2> $MENU_TMP"

	menu_size=2

	menu_instr="Please configure ${IF_NAME[$num]}: X25 parameters below"

	menu_name "${IF_NAME[$num]}: X25 PROTOCOL SETUP" "$menu_options" "$menu_size" "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	if [ $choice = get_idle_timeout ]; then
				get_integer "Please specify x25 idle timeout" "90" "1" "500"
				IDLE_TIMEOUT[$num]=$($GET_RC)
			fi
			
			if [ $choice = get_hold_timeout ]; then
				get_integer "Please specify x25 hold timeout" "10" "1" "500"
				HOLD_TIMEOUT[$num]=$($GET_RC) 
			fi

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

function get_hdlc_station () {
 
	local menu_options
	local rc
	local choice


	menu_options="'DTE'    'DTE' \
		      'DCE'    'DCE' 2> $MENU_TMP"

	menu_instr="Please specify the X25 Station Type \
				"

	menu_name "X25 STATION SETUP" "$menu_options" 2 "$menu_instr" "$BACK"	
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	STATION=$choice     #Choice is a command 
			return 0
			;;
		2)	
			device_setup_help "get_hdlc_station"	
			get_hdlc_station
			;;
		*) 	return 1
			;;
	esac
}

function get_ccitt () {
 
	local menu_options
	local rc
	local choice


	menu_options="'1988'   '1988 (Default)' \
		      '1984'   '1984' \
		      '1980'   '1980' 2> $MENU_TMP"

	menu_instr="Please specify the X25 CCITT Compatibility \
				"

	menu_name "X25 CCITT SETUP" "$menu_options" 3 "$menu_instr" "$BACK"	
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	CCITT=$choice     #Choice is a command 
			return 0
			;;
		2)	
			device_setup_help "get_ccitt"	
			get_ccitt
			;;
		*) 	return 1
			;;
	esac
}

function get_hdlc_only () {
 
	local menu_options
	local rc
	local choice


	menu_options="'NO'     'Full X25/HDLC Support (Default)' \
		      'YES'    'LAPB HDLC Support Only' 2> $MENU_TMP"

	menu_instr="Please select the desired protocol level\
				"

	menu_name "X25 PROTOCOL SELECTION" "$menu_options" 2 "$menu_instr" "$BACK"	
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	LAPB_HDLC_ONLY=$choice     #Choice is a command 
			return 0
			;;
		2)	
			device_setup_help "get_hdlc_only"	
			get_hdlc_only
			;;
		*) 	return 1
			;;
	esac
}


function get_logging () {
 
	local menu_options
	local rc
	local choice


	menu_options="'YES'     'Enable Channel Setup Logging (Default)' \
		      'NO'      'Disable Logging' 2> $MENU_TMP"

	menu_instr="Please enable or disable channel setup logging \
				"

	menu_name "X25 CHANNEL SETUP LOGGING" "$menu_options" 2 "$menu_instr" "$BACK"	
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	CALL_LOGGING=$choice     #Choice is a command 
			return 0
			;;
		2)	
			device_setup_help "get_logging"	
			get_logging
			;;
		*) 	return 1
			;;
	esac
}


function get_oob_modem () {
 
	local menu_options
	local rc
	local choice


	menu_options="'NO'     'Disable modem status OOB messages (Default)' \
		      'YES'    'Enable modem status OOB messages' 2> $MENU_TMP"

	menu_instr="Please enable or disable modem status messaging \
				"

	menu_name "X25 OOB MODEM MSG SETUP" "$menu_options" 2 "$menu_instr" "$BACK"	
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	OOB_ON_MODEM=$choice     #Choice is a command 
			return 0
			;;
		2)	
			device_setup_help "get_oob_modem"	
			get_oob_modem
			;;
		*) 	return 1
			;;
	esac
}


function get_channel_type () {
 
	local num=$1
	local menu_options
	local rc
	local choice


	menu_options="'SVC'     'SVC Link (Default)' \
		      'PVC'     'PVC Link' 2> $MENU_TMP"

	menu_instr="Please specify the channel type\
				"

	menu_name "X25 CHANNEL TYPE" "$menu_options" 2 "$menu_instr" "$BACK"	
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	CH_TYPE[$num]=$choice     #Choice is a command 
			return 0
			;;
		2)	
			interface_menu_help "get_channel_type"	
		 	get_channel_type $num	
			;;
		*) 	return 1
			;;
	esac
}

