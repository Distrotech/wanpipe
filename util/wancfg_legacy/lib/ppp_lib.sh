#!/bin/sh

function ppp_interface_setup () {

	local rc
	local num="$1"
	local choice
	local menu_options
	local menu_size

	if [ -z "${MULTICAST[$num]}" ]; then
		MULTICAST[$num]=NO	
	fi
	if [ -z "${IPX[$num]}" ]; then
		IPX[$num]=NO
	fi
	if [ -z "${PAP[$num]}" ]; then
		PAP[$num]=NO
	fi
	if [ -z "${CHAP[$num]}" ]; then
		CHAP[$num]=NO
	fi

	echo "'get_multicast' 'MULTICAST---> ${MULTICAST[$num]}' \\" > ppp_opt_temp.$$
	echo "'get_pap'       'PAP --------> ${PAP[$num]}' \\" >> ppp_opt_temp.$$
 	echo "'get_chap'      'CHAP--------> ${CHAP[$num]}' \\" >> ppp_opt_temp.$$
	echo "'get_ipx'       'IPX  -------> ${IPX[$num]}' \\" >> ppp_opt_temp.$$
	menu_size=4

	if [ ${IPX[$num]} = YES ]; then
		   echo "'get_network'   'NETWORK ----> ${NETWORK[$num]}' \\" >> ppp_opt_temp.$$
		   menu_size=$((menu_size+1))
	fi

	if [ ${PAP[$num]} = YES -o ${CHAP[$num]} = YES ]; then
		   echo "'get_userid'    'USERID -----> ${USERID[$num]}' \\" >> ppp_opt_temp.$$  
                   echo "'get_passwd'    'PASSWD -----> ${PASSWD[$num]}' \\" >> ppp_opt_temp.$$
		   echo "'get_sysname'   'SYSNAME ----> ${SYSNAME[$num]}'\\" >> ppp_opt_temp.$$
		   menu_size=$((menu_size+3))
	fi

	echo " 2> $MENU_TMP" >> ppp_opt_temp.$$
	menu_options=`cat ppp_opt_temp.$$`
	rm -f ppp_opt_temp.$$


	menu_instr="Please configure ${IF_NAME[$num]} Interface \
						"

	menu_name "PPP ${IF_NAME[$num]} Setup" "$menu_options" "$menu_size" "$menu_instr" "$BACK" 20
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	case $choice in 
				get_network) 
					get_string "Please specify you IPX Network Number" "0xABCDEFAB" 
					NETWORK[$num]=$($GET_RC)				
					;;
				get_userid)
					get_string "Please specify you User Id" 
					USERID[$num]=$($GET_RC)
					;;
				get_passwd)
					get_string "Please specify you Password"
					PASSWD[$num]=$($GET_RC)
					;;
				get_sysname)
					get_string "Please specify you System Name"
					SYSNAME[$num]=$($GET_RC)
					;;
				*)	
					$choice "$num"    #Choice is a command 
					;;
			esac
			return 0 
			;;
		2)	choice=${choice%\"*\"}
			choice=${choice// /}
			interface_setup_help $choice
			return 0
			;;
		*) 	return 1
			;;
	esac

}


function get_ipmode () {

	local menu_options
	local rc
	local choice

	menu_options="'STATIC'  'Static IP addressing,  STATIC(default)' \
		      'HOST'    'Host IP addressing,    HOST' \
		      'PEER'    'Dynamic IP addressing, PEER' 2> $MENU_TMP"


	menu_instr="Please select PPP IP Addressing Mode \
					"

	menu_name "${IF_NAME[$num]} IP Mode Configuration" "$menu_options" 3 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	IPMODE=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "get_ipmode"
			get_ipmode
			;;	
		*) 	return 1
			;;
	esac

}

function get_ipx () {
 
	local menu_options
	local rc
	local choice
	local num=$1;

	menu_options="'NO'  'Disable IPX (Default)' \
		      'YES' 'Enable IPX' 2> $MENU_TMP"

	menu_instr="Please Enable or Disable IPX Routing \
					"

	menu_name "${IF_NAME[$num]} IPX Configuration" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	IPX[$num]=$choice     #Choice is a command 
			return 0
			;;
		2)	interface_setup_help "get_ipx"
			get_ipx "$1"
			;;
		*) 	return 1
			;;
	esac
}

function get_pap () {
 
	local menu_options
	local rc
	local choice
	local num=$1;

	menu_options="'NO'  'Disable PAP (Default)' \
		      'YES' 'Enable PAP' 2> $MENU_TMP"

	menu_instr="Please Enable or Disable PAP Auth. Protocol \
					"

	menu_name "PAP Configuration" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	
			PAP[$num]=$choice     #Choice is a command 
			return 0
			;;
		2)	interface_setup_help "get_pap"
			get_pap $num
			;;
		*) 	return 1
			;;
	esac
}


function get_chap () {
 
	local menu_options
	local rc
	local choice
	local num=$1;

	menu_options="'NO'  'Disable CHAP (Default)' \
		      'YES' 'Enable CHAP' 2> $MENU_TMP"

	menu_instr="Please Enable or Disable CHAP Auth. Protocol \
				"

	menu_name "CHAP Configuration" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	
			CHAP[$num]=$choice     #Choice is a command 
			return 0
			;;
		2)	interface_setup_help "get_chap"
			get_chap $num 
			;;
		*) 	return 1
			;;
	esac
}

function get_ppp_chdlc_mode ()
{
	local menu_options
	local rc
	local choice
	local num=$1;
	local menu_size=2

	if [ $PROTOCOL = WAN_AFT ]; then
		if [ $OS_SYSTEM = "Linux" ]; then
	menu_options="'MP_FR' 'Frame Relay Protocol (Default)' \
		      'MP_PPP' 'PPP Protocol' \
		      'MP_CHDLC' 'Cisco HDLC Protocol' \
		      'HDLC' 'RAW HDLC Protocol' 2> $MENU_TMP"
		      menu_size=4
		else
	menu_options="'MP_CHDLC' 'Cisco HDLC Protocol (Default)' \
		      'MP_PPP' 'PPP Protocol' 2> $MENU_TMP"
		      menu_size=2

		fi
	elif [ $PROTOCOL = WAN_BITSTRM ]; then
	menu_options="'HDLC' 'RAW HDLC Protocol (Default)' \
		      'MP_PPP' 'PPP Protocol' \
		      'MP_CHDLC' 'Cisco HDLC Protocol' 2> $MENU_TMP"
		      menu_size=3
	else
	menu_options="'MP_PPP' 'PPP Protocol (Default)' \
		      'MP_CHDLC' 'Cisco HDLC Protocol' 2> $MENU_TMP"
	fi

	menu_instr="Please Select MultiPort Protocol \
				"

	menu_name "${IF_NAME[$num]} MultiPort Protocol Configuration" "$menu_options" $menu_size "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	MPPP_PROT[$num]=$choice     #Choice is a command 
			return 0
			;;
		2)	interface_setup_help "get_ppp_chdlc_mode"
			get_ppp_chdlc_mode "$1"
			;;
		*) 	return 1
			;;
	esac
}
