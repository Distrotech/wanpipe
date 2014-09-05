#!/bin/sh

ANNEXG_DIR="$PROD_HOME/annexg"
ANNEXG_INTR_DIR="$ANNEXG_DIR/interface"
LAPB_PROFILE_DIR="$ANNEXG_DIR/lapb_profiles"
X25_PROFILE_DIR="$ANNEXG_DIR/x25_profiles"
MASTER_PROT_NAME="f"

function annexg_setup_menu () {
	
	local ret
	local rc
	local menu_options
	local menu_cnt=2
	local num="$1"
	local master_ifdev="$2"
	local read_only="$3"

	if [ $PROTOCOL = WAN_FR ]; then
		MASTER_PROT_NAME="f"
	else
		MASTER_PROT_NAME="c"
	fi

	LAPB_PROFILE[$num]=${LAPB_PROFILE[$num]:-Unconfigured}
	X25_PROFILE[$num]=${X25_PROFILE[$num]:-Unconfigured}
	LAPB_IFACE=${LAPB_IFACE:-Unconfigured}
	LAPB_IF_FILE=${LAPB_IF_FILE:-$ANNEXG_DIR/$master_ifdev".lapb"}


	if [ -f "$LAPB_PROFILE_DIR/lapb.$master_ifdev" ]; then
		read LAPB_PROFILE[$num] < $LAPB_PROFILE_DIR/"lapb."$master_ifdev
		#echo "LAPB PROFILE IS ${LAPB_PROFILE[$num]}"

		LAPB_PROFILE[$num]=${LAPB_PROFILE[$num]//]/}
		LAPB_PROFILE[$num]=${LAPB_PROFILE[$num]//[/}
		LAPB_PROFILE[$num]=${LAPB_PROFILE[$num]// /}

		edit_lapb_profile "lapb.$master_ifdev" "$num" "$master_ifdev" "readonly"

		if [ -f "$ANNEXG_DIR/$master_ifdev.lapb" ]; then
			read LAPB_IFACE < $ANNEXG_DIR/$master_ifdev".lapb"
			LAPB_IFACE=`echo "$LAPB_IFACE" | cut -d'=' -f1`
			LAPB_IFACE=${LAPB_IFACE// /}
			#echo "LAPB IFACE IS $LAPB_IFACE"
		fi
	fi

	if [ -f "$X25_PROFILE_DIR/x25.$master_ifdev" ]; then
		read X25_PROFILE[$num] < $X25_PROFILE_DIR/"x25."$master_ifdev
		#echo "X25 PROFILE IS ${X25_PROFILE[$num]}"

		X25_PROFILE[$num]=${X25_PROFILE[$num]//]/}
		X25_PROFILE[$num]=${X25_PROFILE[$num]//[/}
		X25_PROFILE[$num]=${X25_PROFILE[$num]// /}

		edit_x25_profile "x25.$master_ifdev" "$num" "$master_ifdev" "readonly"
	fi

	if [ ! -z "$read_only" ]; then
		return 0;
	fi


	if [ ${LAPB_PROFILE[$num]} = "Unconfigured" -a ${X25_PROFILE[$num]} = "Unconfigured" ]; then
	menu_options="'get_lapb_profile' 'Lapb Profile  --------> ${LAPB_PROFILE[$num]}' \
		      'get_x25_profile'  'X25 Profile  ---------> ${X25_PROFILE[$num]}' 2> $MENU_TMP"
	else	
	menu_options="'get_lapb_profile' 'Lapb Profile  --------> ${LAPB_PROFILE[$num]}'"
		
		if [ ${LAPB_PROFILE[$num]} != "Unconfigured" ]; then
	menu_options=$menu_options" 'cfg_lapb_interface'  'Lapb Interface Cfg ---> $LAPB_IFACE'"
		menu_cnt=$((menu_cnt+1))
		fi	
	
	menu_options=$menu_options" 'get_x25_profile'    'X25 Profile  ---------> ${X25_PROFILE[$num]}'"

		if [ ${X25_PROFILE[$num]} != "Unconfigured" ]; then
	menu_options=$menu_options" 'x25_cfg_list'       'X25 Interface Config -> Setup'"
		menu_cnt=$((menu_cnt+1))
		fi

	menu_options=$menu_options" 2> $MENU_TMP"
	fi

	menu_instr="	Configure Lapb/X25 Protocol Profiles \
				"
  	menu_name "${IF_NAME[$num]}: LAPB/X25 CONFIGURATION" "$menu_options" "10" "$menu_instr" "$BACK" 20 50 
	rc=$?
	
	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in

		0) 
			$choice "$num" "$master_ifdev" "YES" 
			annexg_setup_menu  "$num" "$master_ifdev"
			return 0
			;;
		2)	
			return 0 
			;;
			
		*) return 1
			;;

	esac

}

#-------------- LAPB PROFILE SECTION -------------------

function get_lapb_profile()
{

	local profiles
	local profile
	local ret
	local rc
	local menu_options
	local menu_cnt=1
	local num="$1"
	local master_ifdev="$2"

	menu_options="'create_lapb_profile' 'Create New Lapb Profile' "

	if [ -d "$LAPB_PROFILE_DIR" ]; then
		unset profiles
		profiles=`ls $LAPB_PROFILE_DIR`
		#echo "Listing directory $LAPB_PROFILE_DIR: IFS=$IFS :  ++++$profiles++++"
		unset profile
		for profile in $profiles
		do
			#echo "Setting up menu for ---$profile---- "
			menu_options=$menu_options" '$profile'  '$profile' "	
			let "menu_cnt += 1"
		done
	fi
	
	menu_options=$menu_options" 2> $MENU_TMP"


	menu_instr="	Edit/Configure Lapb Protocol Profile \
				"
  	menu_name "LAPB CONFIGURATION" "$menu_options" "10" "$menu_instr" "$BACK" 20 50 
	rc=$?
	
	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in

		0) 
			if [ $choice = create_lapb_profile ]; then
				unset LAPB_FILE
				$choice "$num" "$master_ifdev" "YES"
			else
				#echo "Starting to edit $choice"
				edit_lapb_profile "$choice" "$num" "$master_ifdev"
			fi
			return 0
			;;
		2)	
			return 0 
			;;
			
		*) return 1
			;;

	esac
}


function create_lapb_profile ()
{

	local ret
	local rc
	local menu_options
	local menu_cnt=2
	local num="$1"
	local master_ifdev="$2"
	local initialize=${3:-NO}


	if [ "$initialize" = YES ]; then
		initialize=NO
		LAPB_PROFILE[$num]=Unconfigured
		unset LAPB_T1
		unset LAPB_T2
		unset LAPB_T3
		unset LAPB_T4
		unset LAPB_N2
		unset LAPB_MODE
		unset LAPB_WINDOW
		unset LAPB_PKT_SZ
	fi

	if [ "${LAPB_PROFILE[$num]}" = Unconfigured ]; then
		LAPB_PROFILE[$num]="lapb."$master_ifdev
	fi

	LAPB_STATION=${LAPB_STATION:-DTE}
	LAPB_T1=${LAPB_T1:-10}
	LAPB_T2=${LAPB_T2:-0}
	LAPB_T3=${LAPB_T3:-100}
	LAPB_T4=${LAPB_T4:-20}
	LAPB_N2=${LAPB_N2:-5}
	LAPB_MODE=${LAPB_MODE:-8}
	LAPB_WINDOW=${LAPB_WINDOW:-7}
	LAPB_PKT_SZ=${LAPB_PKT_SZ:-1024}
	
	menu_options="'get_lapb_station' 'Lapb Station ---------> $LAPB_STATION' \
		      'get_lapb_modulus' 'Lapb modulus ---------> $LAPB_MODE' \
		      'get_lapb_window'  'Lapb window size -----> $LAPB_WINDOW' \
		      'get_lapb_pkt_sz'  'Lapb packet size -----> $LAPB_PKT_SZ' \
		      'get_lapb_t1_timer' 'T1 timer -------------> $LAPB_T1' \
		      'get_lapb_t2_timer' 'T2 timer -------------> $LAPB_T2' \
		      'get_lapb_t3_timer' 'T3 timer -------------> $LAPB_T3' \
		      'get_lapb_t4_timer' 'T4 timer -------------> $LAPB_T4' \
		      'get_lapb_n2_timer' 'N2 -------------------> $LAPB_N2' 2> $MENU_TMP"

	menu_instr="	Configure Lapb Protocol Profile \
				"
  	menu_name "LAPB PROFILE CONFIGURATION" "$menu_options" 10 "$menu_instr" "$BACK" 20 50 
	rc=$?
	
	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in

		0) 
			case $choice in
			get_lapb_t1_timer)
				get_integer "Please specify a T1 timer value" "10" "1" "30" "interface_menu_help get_lapb_t1_timer"

				LAPB_T1=$($GET_RC)
				;;

			get_lapb_t1_timer)
				get_integer "Please specify a T2 timer value" "0" "0" "$((LAPB_T1-1))" "interface_menu_help get_lapb_t2_timer"

				LAPB_T2=$($GET_RC)
				;;

			get_lapb_t3_timer)
				get_integer "Please specify a T3 timer value" "0" "$((LAPB_T4))" "100000" "interface_menu_help get_lapb_t3_timer"

				LAPB_T3=$($GET_RC)
				;;

			get_lapb_t4_timer)
				get_integer "Please specify a T4 timer value" "0" "$((LAPB_T1+1))" "$((LAPB_T3))" "interface_menu_help get_lapb_t4_timer"

				LAPB_T4=$($GET_RC)
				;;

			get_lapb_n2_timer)
				get_integer "Please specify a N2 timer value" "5" "1" "30" "interface_menu_help get_lapb_n2_timer"

				LAPB_N2=$($GET_RC)
				;;
			get_lapb_modulus)
				get_integer "Please specify lapb modulus (8 or 128)" "$LAPB_MODE" "8" "128" "interface_menu_help get_lapb_modulus"

				LAPB_MODE=$($GET_RC)
				;;
			
			get_lapb_window)
				get_integer "Please specify lapb window" "$LAPB_WINDOW" "1" "7" "interface_menu_help get_lapb_window"

				LAPB_WINDOW=$($GET_RC)
				;;

			get_lapb_pkt_sz)
				get_integer "Please specify maximum lapb pkt size" "1024" "64" "4096" "interface_menu_help get_lapb_pkt_sz"

				LAPB_PKT_SZ=$($GET_RC)
				;;
			*)
				$choice
				;;
			esac
			create_lapb_profile "$num" "$master_ifdev"
			return 0
			;;
		2)
			interface_menu_help $choice
			create_lapb_profile "$num" "$master_ifdev"
			return 0 
			;;
			
		*) 
			if [ ! -d "$LAPB_PROFILE_DIR" ]; then
				mkdir -p $LAPB_PROFILE_DIR
			fi

			if [ ! -d "$ANNEXG_INTR_DIR" ]; then
				mkdir -p $ANNEXG_INTR_DIR
			fi	

			LAPB_FILE=${LAPB_FILE:-$LAPB_PROFILE_DIR/"lapb."$master_ifdev}

			#echo "Saving to LAPB FILE $LAPB_FILE"

			echo "[${LAPB_PROFILE[$num]}]" > $LAPB_FILE
			echo "STATION	= $LAPB_STATION" >> $LAPB_FILE
			echo "T1	= $LAPB_T1" >> $LAPB_FILE 
			echo "T2	= $LAPB_T2" >> $LAPB_FILE	
			echo "T3	= $LAPB_T3" >> $LAPB_FILE 
			echo "T4	= $LAPB_T4" >> $LAPB_FILE
			echo "N2	= $LAPB_N2" >> $LAPB_FILE
			echo "LAPB_MODE = $LAPB_MODE" >> $LAPB_FILE
			echo "LAPB_WINDOW  = $LAPB_WINDOW" >> $LAPB_FILE
			echo "MAX_PKT_SIZE = $LAPB_PKT_SZ" >> $LAPB_FILE

			LAPB_IF_FILE=$ANNEXG_DIR/$master_ifdev".lapb"

			if [ ! -f $LAPB_IF_FILE ]; then
				LAPB_IFACE="w"$DEVICE_NUM$MASTER_PROT_NAME${DLCI_NUM[$num]}"lapb"
				echo "$LAPB_IFACE = wanpipe$DEVICE_NUM, ,$API, lapb, ${LAPB_PROFILE[$num]}, $X25_VIRTUAL_ADDR, $X25_REAL_ADDR " > $LAPB_IF_FILE
			fi
			return 1
			;;

	esac
}


function edit_lapb_profile ()
{

	local ret
	local rc
	local menu_options
	local menu_cnt=2
	local file="$1"
	local num="$2"
	local master_ifdev="$3"
	local read_only="$4"
	local tmp_ifs

	read LAPB_PROFILE[$num] < $LAPB_PROFILE_DIR/$file
	#echo "LAPB PROFILE IS ${LAPB_PROFILE[$num]}"

	LAPB_PROFILE[$num]=${LAPB_PROFILE[$num]//]/}
	LAPB_PROFILE[$num]=${LAPB_PROFILE[$num]//[/}
	LAPB_PROFILE[$num]=${LAPB_PROFILE[$num]// /}

	tmp_ifs=$IFS
	IFS="="
	while read name value 
	do
		if [ "$value" = "" ]; then
			continue;
		fi

		value=${value// /}
		name=${name// /}

		case $name in
	
		STATION)
			LAPB_STATION=$value;
			;;
		T1)
			LAPB_T1=$value;
			;;
		T2)
			LAPB_T2=$value;
			;;
		T3)
			LAPB_T3=$value;
			;;
		T4)
			LAPB_T4=$value;
			;;
		N2)
			LAPB_N2=$value;
			;;
		LAPB_MODE)
			LAPB_MODE=$value;
			;;
		LAPB_WINDOW)
			LAPB_WINDOW=$value;
			;;
		MAX_PKT_SIZE)
			LAPB_PKT_SZ=$value;
			;;
		esac
		
	done < $LAPB_PROFILE_DIR/$file
	IFS=$tmp_ifs

	LAPB_FILE=$LAPB_PROFILE_DIR/$file

	if [ -z $read_only ]; then
		create_lapb_profile "$num" "$master_ifdev"  
	fi

	return 0;

}

function cfg_lapb_interface ()
{
	
	local ret
	local rc
	local menu_options
	local menu_cnt=1
	local num="$1"
	local master_ifdev="$2"
	local update="$3"

	#echo "Starting LAPB CFG Update = $update FILE $LAPB_IF_FILE"

	ANNEXG_OPMODE=${ANNEXG_OPMODE:-API}

	if [ "$update" = "YES" ]; then
		if [ -f "$LAPB_IF_FILE" ]; then
			read lapb_if_tmp < $LAPB_IF_FILE

			ANNEXG_OPMODE=`echo "$lapb_if_tmp" | cut -d',' -f3`
			ANNEXG_OPMODE=${ANNEXG_OPMODE// /}

			X25_VIRTUAL_ADDR=`echo "$lapb_if_tmp" | cut -d',' -f6`
			X25_VIRTUAL_ADDR=${X25_VIRTUAL_ADDR// /};
	
			X25_REAL_ADDR=`echo "$lapb_if_tmp" | cut -d',' -f7`
			X25_REAL_ADDR=${X25_REAL_ADDR// /};
			
			#echo "Parsed opmode is $ANNEXG_OPMODE $X25_VIRTUAL_ADDR $X25_REAL_ADDR !!!!"
			
			LAPB_IFACE=`echo "$lapb_if_tmp" | cut -d'=' -f1`
			LAPB_IFACE=${LAPB_IFACE// /}

			if [ "$ANNEXG_OPMODE" = "WANPIPE" ];  then
			
				cat $ANNEXG_INTR_DIR/$LAPB_IFACE

				if [ -f $ANNEXG_INTR_DIR/$LAPB_IFACE ]; then

				ifs_tmp=$IFS
				IFS="="
				while read name value 
				do
					value=${value// /}
					name=${name// /}

					#Convert to upper case
					name=`echo $name | tr a-z A-Z`
			
					case $name in
						IPADDR)
							ANNEXG_L_IP=$value
						;;
						NETMASK)
							ANNEXG_N_IP=$value
						;;
						POINTOPOINT)
							ANNEXG_R_IP=$value
						;;
					esac
				
				done < $ANNEXG_INTR_DIR/$LAPB_IFACE
				IFS=$ifs_tmp
				fi
			fi
		fi
	fi

	menu_options="'get_operation_mode' 'Operation Mode ----------> $ANNEXG_OPMODE'"

	if [ $ANNEXG_OPMODE = WANPIPE ]; then
	menu_options=$menu_options" 'get_local_ip' 'Local IP Addr. ----------> $ANNEXG_L_IP' \
		      	  'get_ptp_ip'     'Local P-t-P Addr. -------> $ANNEXG_R_IP' \
			  'get_netmask'    'Local Netmask Addr. -----> $ANNEXG_N_IP'" 
		menu_cnt=$((menu_cnt+3))
	elif [ $ANNEXG_OPMODE = API ]; then
	menu_options=$menu_options" 'get_virtual_addr' 'Switching Virtual Addr. -> $X25_VIRTUAL_ADDR' \
		      	  'get_real_addr' 'Switching Real Addr. ----> $X25_REAL_ADDR' "
		menu_cnt=$((menu_cnt+2))
	fi	

	menu_options=$menu_options" 2> $MENU_TMP"

	menu_instr="	Configure Lapb Protocol Profile \
				"
  	menu_name "LAPB PROFILE CONFIGURATION" "$menu_options" "10" "$menu_instr" "$BACK" 20 50 
	rc=$?
	
	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in

	0)
		case $choice in	

		get_local_ip)
			get_string "Please specify the Local IP Address" "$ANNEXG_L_IP" "IP_ADDR" 
			ANNEXG_L_IP=$($GET_RC)
			;;
		get_ptp_ip)
			get_string "Please specify the Point-to-Point IP Address" "$ANNEXG_R_IP" "IP_ADDR"
			ANNEXG_R_IP=$($GET_RC)
			;;
		get_netmask)
			get_string "Please specify the Net Mask Addr" "$ANNEXG_N_IP" "IP_ADDR"
			ANNEXG_N_IP=$($GET_RC)
			;;
		get_virtual_addr)
			get_string "Please specify the X.25 Switching Virtual Addr" "$X25_VIRTUAL_ADDR" ""
			X25_VIRTUAL_ADDR=$($GET_RC)
			;;
		get_real_addr)
			get_string "Please specify the X.25 Switching Real Addr" "$X25_REAL_ADDR" ""
			X25_REAL_ADDR=$($GET_RC)
			;;
		*)
			$choice "$num" "ANNEXG" 
			;;
		esac

		cfg_lapb_interface "$num" "$master_ifdev"
		;;

	2) 	return 0
		;;

	*)	
	
		if [ ! -d "$ANNEXG_INTR_DIR" ]; then
			mkdir -p $ANNEXG_INTR_DIR
		fi
	
		LAPB_IF_FILE=${LAPB_IF_FILE:-$ANNEXG_DIR/$master_ifdev".lapb"}

		LAPB_IFACE="w"$DEVICE_NUM$MASTER_PROT_NAME${DLCI_NUM[$num]}"lapb"
		echo "$LAPB_IFACE = wanpipe$DEVICE_NUM, ,$ANNEXG_OPMODE, lapb, ${LAPB_PROFILE[$num]}, $X25_VIRTUAL_ADDR, $X25_REAL_ADDR " > $LAPB_IF_FILE
	

		if [ $ANNEXG_OPMODE = WANPIPE ]; then
		date=`date`
		cat <<EOM > $ANNEXG_INTR_DIR/$LAPB_IFACE
# Wanrouter interface configuration file
# name:	$LAPB_IFACE
# date:	$date
#
DEVICE=$LAPB_IFACE
IPADDR=$ANNEXG_L_IP
NETMASK=$ANNEXG_N_IP
POINTOPOINT=$ANNEXG_R_IP
ONBOOT=yes
EOM
		fi

		return 1
		;;
	
	esac
}



#--------------- X25 PROFILE SECTION ----------------------------------------

function get_x25_profile()
{

	local profiles
	local profile
	local ret
	local rc
	local menu_options
	local menu_cnt=1
	local num="$1"
	local master_ifdev="$2"

	menu_options="'create_x25_profile' 'Create New X25 Profile' "

	if [ -d "$X25_PROFILE_DIR" ]; then
		unset profiles
		profiles=`ls $X25_PROFILE_DIR`
		#echo "Listing directory $X25_PROFILE_DIR: IFS=$IFS :  ++++$profiles++++"
		unset profile
		for profile in $profiles
		do
			#echo "Setting up menu for ---$profile---- "
			menu_options=$menu_options" '$profile'  '$profile' "	
			let "menu_cnt += 1"
		done
	fi
	
	menu_options=$menu_options" 2> $MENU_TMP"


	menu_instr="	Edit/Configure X25 Protocol Profile \
				"
  	menu_name "X25 CONFIGURATION" "$menu_options" "$menu_cnt" "$menu_instr" "$BACK" 20 50 
	rc=$?
	
	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in

		0) 
			if [ $choice = create_x25_profile ]; then
				unset X25_FILE
				$choice "$num" "$master_ifdev" "YES"
			else
				#echo "Starting to edit $choice"
				edit_x25_profile "$choice" "$num" "$master_ifdev"
			fi
			return 0
			;;
		2)	
			return 0 
			;;
			
		*) return 1
			;;

	esac
}


function create_x25_profile ()
{

	local ret
	local rc
	local menu_options
	local menu_cnt=2
	local num="$1"
	local master_ifdev="$2"
	local initialize=${3:-NO}

	#echo "Creating X25 Profile Intialize=$initialize"

	if [ "$initialize" = YES ]; then
			
		initialize=NO
		X25_PROFILE[$num]=Unconfigured
		unset X25_STATION
		unset X25_API_OPTIONS
		unset X25_PROTOCOL_OPTIONS
		unset X25_RESPONSE_OPTIONS
		unset X25_STATISTICS_OPTIONS
		unset X25_LOWEST_SVC
		unset X25_HIGHEST_SVC
		unset X25_PACKET_WINDOW
		unset CCITT
		unset T10_T20
		unset T11_T21
		unset T12_T22
		unset T13_T23 
		unset T16_T26
		unset T28
		unset R10_R20
		unset R12_R22
		unset R13_R23

		unset X25_GEN_FACILITY_1
		unset X25_GEN_FACILITY_2
		unset X25_CCITT_FACILITY
		unset NON_X25_FACILITY
		unset DFLT_PKT_SIZE
		unset MAX_PKT_SIZE
		unset X25_MODE
		unset CALL_LOGGING
		unset CALL_BACKOFF
	fi


	if [ "${X25_PROFILE[$num]}" = Unconfigured ]; then
		X25_PROFILE[$num]="x25."$master_ifdev
	fi

	X25_STATION=${X25_STATION:-DTE}
	X25_API_OPTIONS=${X25_API_OPTIONS:-0x00}
	X25_PROTOCOL_OPTIONS=${X25_PROTOCOL_OPTIONS:-0x00D0}
	X25_RESPONSE_OPTIONS=${X25_RESPONSE_OPTIONS:-0x01}
	X25_STATISTICS_OPTIONS=${X25_STATISTICS_OPTIONS:-0x00}
	X25_LOWEST_SVC=${X25_LOWEST_SVC:-0}
	X25_HIGHEST_SVC=${X25_HIGHEST_SVC:-0}
	X25_LOWEST_PVC=${X25_LOWEST_PVC:-0}
	X25_HIGHEST_PVC=${X25_HIGHEST_PVC:-0}
	X25_PACKET_WINDOW=${X25_PACKET_WINDOW:-7}
	CCITT=${CCITT:-1988}
	T10_T20=${T10_T20:-30}
	T11_T21=${T11_T21:-30}
	T12_T22=${T12_T22:-30}
	T13_T23=${T13_T23:-10}
	T16_T26=${T16_T26:-30}
	T28=${T28:-30}
	R10_R20=${R10_R20:-5}
	R12_R22=${R12_R22:-5}
	R13_R23=${R13_R23:-5}
	X25_GEN_FACILITY_1=${X25_GEN_FACILITY_1:-0x00}
	X25_GEN_FACILITY_2=${X25_GEN_FACILITY_2:-0x00}
	X25_CCITT_FACILITY=${X25_CCITT_FACILITY:-0x00}
	NON_X25_FACILITY=${NON_X25_FACILITY:-0x00}
	DFLT_PKT_SIZE=${DFLT_PKT_SIZE:-128}
	MAX_PKT_SIZE=${MAX_PKT_SIZE:-1024}
	X25_MODE=${X25_MODE:-8}
	X25_CALL_BACKOFF=${X25_CALL_BACKOFF:-10}
	X25_CALL_LOGGING=${X25_CALL_LOGGING:-NO}

	
	menu_options="'get_x25mpapi_station' 'X25 station  ---------> $X25_STATION' \
		      'get_x25_mode'     'X25 modulus ----------> $X25_MODE' \
		      'get_low_pvc'      'X25 Lowest PVC -------> $X25_LOWEST_PVC' \
		      'get_high_pvc'     'X25 Highest PVC ------> $X25_HIGHEST_PVC' \
		      'get_low_svc'      'X25 Lowest SVC -------> $X25_LOWEST_SVC' \
		      'get_high_svc'     'X25 Highest SVC ------> $X25_HIGHEST_SVC' \
		      'again'            '---Tx/Rx Configuration--- ' \
		      'get_x25_pkt_win'  'X25 Packet Window ----> $X25_PACKET_WINDOW' \
		      'get_x25_dft_size' 'X25 Default Pkt size -> $DFLT_PKT_SIZE' \
		      'get_x25_max_size' 'X25 Max Pkt size -----> $MAX_PKT_SIZE' \
		      'again'            '---X25 Operation--- ' \
		      'get_x25_api_opt'	 'X25 API options ------> $X25_API_OPTIONS' \
		      'get_x25_prot_opt' 'X25 Protocol options -> $X25_PROTOCOL_OPTIONS' \
		      'get_x25_resp_opt' 'X25 Response options -> $X25_RESPONSE_OPTIONS' \
		      'get_x25_stat_opt' 'X25 Statist options --> $X25_STATISTICS_OPTIONS' \
		      'get_gen_facil_1'  'X25 General facil 1 --> $X25_GEN_FACILITY_1' \
		      'get_gen_facil_2'  'X25 General facil 2 --> $X25_GEN_FACILITY_2' \
		      'get_ccitt_facil'  'X25 CCITT facil ------> $X25_CCITT_FACILITY' \
		      'get_non_x25_fac'  'X25 Misc facil -------> $NON_X25_FACILITY' \
		      'get_ccitt'        'X25 CCITT compat -----> $CCITT' \
		      'again'            '---X25 Timers--- ' \
		      'get_T10_T20'      'T10_T20 --------> $T10_T20' \
		      'get_T11_T21'      'T11_T21 --------> $T11_T21' \
		      'get_T12_T22'   	 'T12_T22 --------> $T12_T22' \
                      'get_T13_T23'   	 'T13_T23 --------> $T13_T23' \
		      'get_T16_T26'   	 'T16_T26 --------> $T16_T26' \
		      'get_T28'   	 'T28 ------------> $T28' \
		      'get_R10_R20'   	 'R10_R20 --------> $R10_R20' \
		      'get_R12_R22'   	 'R12_R22 --------> $R12_R22' \
		      'get_R13_R23'   	 'R13_R23 --------> $R13_R23' \
		      'again'            '---X25 Driver--- ' \
		  'get_x25_call_backoff' 'Call Backoff ---> $X25_CALL_BACKOFF' \
		  'get_x25_call_logging' 'Call Logging ---> $X25_CALL_LOGGING'  2> $MENU_TMP"

	menu_instr="	Configure X25 Protocol Profile \
				"
  	menu_name "X25 PROFILE CONFIGURATION" "$menu_options" 12 "$menu_instr" "$BACK" 20 50 
	rc=$?
	
	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in

		0) 
			case $choice in

			get_T10_T20)
				get_integer "Please specify the X25 T10_T20 Timer value" "30" "1" "255" "device_setup_help get_T10_T20"
				T10_T20=$($GET_RC)
				;;
				
			get_T11_T21)
				get_integer "Please specify the X25 T11_T21 Timer value" "30" "1" "255" "device_setup_help get_T11_T21"
				T11_T21=$($GET_RC)
				;;
			
			get_T12_T22)
				get_integer "Please specify the X25 T12_T22 Timer value" "30" "1" "255" "device_setup_help get_T12_T22"
				T11_T21=$($GET_RC)
				;;
		
			get_T13_T23)
				get_integer "Please specify the X25 T13_T23 Timer value" "10" "1" "255" "device_setup_help get_T13_T23"
				T11_T21=$($GET_RC)
				;;
			
			get_T16_T26)
				get_integer "Please specify the X25 T16_T26 Timer value" "30" "1" "255" "device_setup_help get_T16_T26"
				T11_T21=$($GET_RC)
				;;


			get_T28)
				get_integer "Please specify the X25 T28 Timer value" "30" "1" "255" "device_setup_help get_T28"
				T28=$($GET_RC)
				;;
				
			get_R10_R20)
				get_integer "Please specify the X25 R10_R20 Timer value" "5" "0" "250" "device_setup_help get_R10_R20"
				R10_20=$($GET_RC)
				;;
				
			get_R12_R22)
				get_integer "Please specify the X25 R12_R22 Timer value" "5" "0" "250" "device_setup_help get_R12_R22"
				R12_22=$($GET_RC)
				;;
				
			get_R13_R23)
				get_integer "Please specify the X25 R13_R23 Timer value" "5" "0" "250" "device_setup_help get_R13_R23"
				R13_23=$($GET_RC)
				;;

			get_low_svc)
				get_integer "Please specify Lowest SVC Channel" "0" "0" "100000" "device_setup_help get_low_svc"
				TMP=$($GET_RC)
				if [ $TMP -gt $X25_LOWEST_PVC ]; then 
					X25_LOWEST_SVC=$TMP
				fi
				
				;;

			get_high_svc)
				get_integer "Please specify Highest SVC Channel" "0" "0" "100000" "device_setup_help get_high_svc"
				X25_HIGHEST_SVC=$($GET_RC)
				;;

			get_low_pvc)
				get_integer "Please specify Lowest PVC Channel" "0" "0" "100000" "device_setup_help get_low_pvc"
				X25_LOWEST_PVC=$($GET_RC)
				;;

			get_high_pvc)
				get_integer "Please specify Highest PVC Channel" "0" "0" "100000" "device_setup_help get_high_pvc"
				TMP=$($GET_RC)
				if [ $X25_LOWEST_SVC -eq 0 ] || [ $TMP -lt $X25_LOWEST_SVC ]; then
					X25_HIGHEST_PVC=$TMP
				fi
				;;


			get_x25_pkt_win)
				get_integer "Please specify Packet Window Size" "7" "1" "7" "device_setup_help get_packet_win"
				X25_PACKET_WINDOW=$($GET_RC)				
				;;
		
			get_x25_dft_size)
				get_integer "Please specify Default Packet Size" "128" "64" "4096" "device_setup_help get_packet_win"
				DFLT_PKT_SIZE=$($GET_RC)				
				;;

			get_x25_max_size)
				get_integer "Please specify Default Packet Size" "1024" "64" "4096" "device_setup_help get_packet_win"
				MAX_PKT_SIZE=$($GET_RC)				
				;;

		      	get_x25_mode)
				get_integer "Please specify X25 Modulus (8 or 128)" "8" "8" "128"
				X25_MODE=$($GET_RC)
				;;

			get_x25_call_backoff)
				get_integer "Please specify X25 Call Backoff Time" "10" "1" "100"
				X25_CALL_BACKOFF=$($GET_RC)
				;;

			get_x25_call_logging)
				warning "X25_CALL_LOGGING"	
				if [ $? -eq 0 ]; then
					X25_CALL_LOGGING=YES
				else
					X25_CALL_LOGGING=NO
				fi
				;;

			get_x25_api_opt)
				get_string "Please specify X25 API options" "0x00"
				X25_API_OPTIONS=$($GET_RC)
				;;
				
		        get_x25_prot_opt)
				get_string "Please specify X25 Protocol options" "0x00"
				X25_PROTOCOL_OPTIONS=$($GET_RC)
				;;
			
			get_x25_resp_opt)
				get_string "Please specify X25 Response options" "0x00"
				X25_RESPONSE_OPTIONS=$($GET_RC)
				;;
			
			get_x25_stat_opt)
				get_string "Please specify X25 Statistics options" "0x00"
				X25_STATISTICS_OPTIONS=$($GET_RC)
				;;
			get_gen_facil_1)
				get_string "Please specify X25 General Facilities 1" "0x00"
				X25_GEN_FACILITY_1=$($GET_RC)
				;;
			get_gen_facil_2)
				get_string "Please specify X25 General Facilities 2" "0x00"
				X25_GEN_FACILITY_2=$($GET_RC)
				;;
			get_ccitt_facil)
				get_string "Please specify X25 CCITT Facilities" "0x00"
				X25_CCITT_FACILITY=$($GET_RC)
				;;
			get_non_x25_fac)
				get_string "Please specify Non-X25 Facilities" "0x00"
				NON_X25_FACILITY=$($GET_RC)
				;;
			again)
				;;
				
			*)
				$choice "annexg"
				;;
				
			esac
			create_x25_profile "$num" "$master_ifdev"
			return 0
			;;
		2)	
			device_setup_help $choice	
			create_x25_profile "$num" "$master_ifdev"
			return 0 
			;;
			
		*)
			if [ ! -d "$X25_PROFILE_DIR" ]; then
				mkdir -p $X25_PROFILE_DIR
			fi
	
			X25_FILE=${X25_FILE:-$X25_PROFILE_DIR/"x25."$master_ifdev}

			#echo "Saving to X25 FILE $X25_FILE"
		
			echo "[${X25_PROFILE[$num]}]" 	> $X25_FILE
			echo "STATION	= $X25_STATION" 	>> $X25_FILE
	
			echo "LOWESTPVC		= $X25_LOWEST_PVC" 	>> $X25_FILE
			echo "HIGHESTPVC	= $X25_HIGHEST_PVC" 	>> $X25_FILE
			echo "LOWESTSVC 	= $X25_LOWEST_SVC" 	>> $X25_FILE
			echo "HIGHESTSVC	= $X25_HIGHEST_SVC" 	>> $X25_FILE
			echo "PACKETWINDOW	= $X25_PACKET_WINDOW" 	>> $X25_FILE
			echo "DFLT_PKT_SIZE 	= $DFLT_PKT_SIZE" >> $X25_FILE
			echo "MAX_PKT_SIZE 	= $MAX_PKT_SIZE"  >> $X25_FILE
		
			echo "X25_API_OPTIONS	= $X25_API_OPTIONS" 		   >> $X25_FILE
			echo "X25_PROTOCOL_OPTIONS	= $X25_PROTOCOL_OPTIONS"   >> $X25_FILE
			echo "X25_RESPONSE_OPTIONS	= $X25_RESPONSE_OPTIONS"   >> $X25_FILE
			echo "X25_STATISTICS_OPTIONS	= $X25_STATISTICS_OPTIONS" >> $X25_FILE

			echo "CCITTCOMPAT	= $CCITT" 	>> $X25_FILE
			echo "T10_T20		= $T10_T20" 	>> $X25_FILE
			echo "T11_T21		= $T11_T21" >> $X25_FILE
			echo "T12_T22	 	= $T12_T22" >> $X25_FILE
			echo "T13_T23		= $T13_T23" >> $X25_FILE
			echo "T16_T26		= $T16_T26" >> $X25_FILE
			echo "T28		= $T28"     >> $X25_FILE
			echo "R10_R20		= $R10_R20" >> $X25_FILE
			echo "R12_R22		= $R12_R22" >> $X25_FILE
			echo "R13_R23		= $R13_R23" >> $X25_FILE

			echo "GEN_FACILITY_1 	= $X25_GEN_FACILITY_1"  >> $X25_FILE
			echo "GEN_FACILITY_2 	= $X25_GEN_FACILITY_2"  >> $X25_FILE
			echo "CCITT_FACILITY 	= $X25_CCITT_FACILITY"  >> $X25_FILE
			echo "NON_X25_FACILITY  = $NON_X25_FACILITY" >> $X25_FILE 

			echo "X25_MODE = $X25_MODE"  >> $X25_FILE
			echo "CALL_BACKOFF = $X25_CALL_BACKOFF" >> $X25_FILE
			echo "CALL_LOGGING = $X25_CALL_LOGGING" >> $X25_FILE

			X25_INTR_CFG_FILE=$ANNEXG_DIR/$master_ifdev".x25" 
			if [ ! -f $X25_INTR_CFG_FILE ]; then
				rm -f $X25_INTR_CFG_FILE
			fi

			create_x25_interfaces 

			return 1
			;;

	esac
}


function edit_x25_profile ()
{

	local ret
	local rc
	local menu_options
	local menu_cnt=2
	local file="$1"
	local num="$2"
	local master_ifdev="$3"
	local read_only="$4"
	local tmp_ifs

	read X25_PROFILE[$num] < $X25_PROFILE_DIR/$file
	#echo "Editing X25 PROFILE ${X25_PROFILE[$num]}"

	X25_PROFILE[$num]=${X25_PROFILE[$num]//]/}
	X25_PROFILE[$num]=${X25_PROFILE[$num]//[/}
	X25_PROFILE[$num]=${X25_PROFILE[$num]// /}

	tmp_ifs=$IFS
	IFS="="
	while read name value 
	do
		if [ "$value" = "" ]; then
			continue;
		fi

		value=${value// /}
		name=${name// /}

		#Convert to upper case
		name=`echo $name | tr a-z A-Z`

		#echo "EDIT $name = $value"

		case $name in

		STATION*) 
			X25_STATION=$value
			;;
		LOWESTSVC*)
			#echo "Editing LOWEST SVC $value"
			X25_LOWEST_SVC=$value
			;;
		HIGHESTSVC*)
			#echo "Editing HIGHTE SVC $value"
			X25_HIGHEST_SVC=$value
			;;
		PACKETWINDOW*)
			X25_PACKET_WINDOW=$value
			;;
		DFLT_PKT_SIZE*)
			DFLT_PKT_SIZE=$value
			;;
		MAX_PKT_SIZE*)
			MAX_PKT_SIZE=$value
			;;
		X25_API_OPTIONS*)
			X25_API_OPTIONS=$value
			;;
		X25_PROTOCOL_OPTIONS*)
			X25_PROTOCOL_OPTIONS=$value
			;;
		X25_RESPONSE_OPTIONS*)
			X25_RESPONSE_OPTIONS=$value
			;;
		X25_STATISTICS_OPTIONS*)
			X25_STATISTICS_OPTIONS=$value
			;;
		CCITTCOMPAT*)
			CCITT=$value
			;;
		T10_T20*)
			T10_T20=$value
			;;
		T11_T21*)
			T11_T21=$value
			;;
		T12_T22*)
			T12_T22=$value
			;;
		T13_T23*)
			T13_T23=$value
			;;
		T16_T26*)
			T16_T26=$value
			;;
		T28*)
			T28=$value
			;;
		R10_R20*)
			R10_R20=$value
			;;
		R12_R22*)
			R12_R22=$value
			;;
		R13_R23*)
			R13_R23=$value
			;;
		GEN_FACILITY_1*)
			X25_GEN_FACILITY_1=$value
			;;
		GEN_FACILITY_2*)
			X25_GEN_FACILITY_2=$value
			;;
		CCITT_FACILITY*)
			X25_CCITT_FACILITY=$value
			;;
		NON_X25_FACILITY*)
			NON_X25_FACILITY=$value
			;;
		X25_MODE*)
			X25_MODE=$value
			;;
		CALL_BACKOFF*)
			X25_CALL_BACKOFF=$value
			;;
		CALL_LOGGING*)
			X25_CALL_LOGGING=$value
			;;
		esac
		
	done < $X25_PROFILE_DIR/$file
	IFS=$tmp_ifs

	X25_FILE=$X25_PROFILE_DIR/$file

	if [ -z $read_only ]; then
		create_x25_profile "$num" "$master_ifdev"  
	else
		X25_INTR_CFG_FILE=$ANNEXG_DIR/$master_ifdev".x25" 
		if [ -f $X25_INTR_CFG_FILE ]; then
			rm -f $X25_INTR_CFG_FILE
		fi

		create_x25_interfaces
	fi
	
	return 0;

}


function x25_cfg_list()
{

	local if_names
	local profile
	local ret
	local rc
	local menu_options
	local num="$1"
	local master_ifdev="$2"


	#echo "X25_CFG_LIST: Looking for $ANNEXG_DIR/$master_ifdev.x25"
	
#	echo
#	cat $ANNEXG_DIR/$master_ifdev.x25
#	echo


	if [ -f "$ANNEXG_DIR/$master_ifdev.x25" ]; then
		ifs_tmp=$IFS
		IFS="="
		while read if_name value
		do
			if_name=${if_name// /}
			value=`echo $value | cut -d',' -f3`
			value=${value// /}
			#echo "Value obtained for $if_name = $value"
			value=${value:-API}
			menu_options=$menu_options" '$if_name'  '$if_name ---> $value' "

		done < $ANNEXG_DIR/$master_ifdev".x25"
		
		IFS=$ifs_tmp
	else
		return 0
	fi
	
	menu_options=$menu_options" 2> $MENU_TMP"

	menu_instr="	Edit/Configure X25 Interfaces \
				"
  	menu_name "X25 INTERFACE CONFIGURATION" "$menu_options" "10" "$menu_instr" "$BACK" 20 50 
	rc=$?
	
	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in

		0) 
			#echo "Starting to edit $choice"
			choice=${choice// /}
			X25_IF_FILE=$ANNEXG_DIR/$master_ifdev".x25"
			X25_IFACE=$choice
			cfg_x25_interface "$num" "$master_ifdev" "YES"
			x25_cfg_list "$num" "$master_ifdev"			
			return 0
			;;
		2)	
			return 0 
			;;
			
		*) return 1
			;;

	esac
}



function cfg_x25_interface ()
{
	
	local ret
	local rc
	local menu_options
	local menu_cnt=1
	local num="$1"
	local master_ifdev="$2"
	local update="$3"

	#echo "Starting X25 CFG Update = $update FILE $X25_IF_FILE  IFACE $X25_IFACE"
	
	ANNEXG_OPMODE=${ANNEXG_OPMODE:-API}

	if [ "$update" = "YES" ]; then
		if [ -f "$X25_IF_FILE" ]; then
	
			#echo "File found $X25_IF_FILE"

			#echo
			#cat $X25_IF_FILE
			#echo

			eval "grep \"$X25_IFACE *=.*API\" $X25_IF_FILE > /dev/null 2> /dev/null"
			if [ $? -eq 0 ]; then
				ANNEXG_OPMODE=API
			else
				eval "grep \"$X25_IFACE *=.*WANPIPE\" $X25_IF_FILE > /dev/null 2> /dev/null"
				if [ $? -eq 0 ]; then
					ANNEXG_OPMODE=WANPIPE
				else
					eval "grep \"$X25_IFACE *=.*DSP\" $X25_IF_FILE > /dev/null 2> /dev/null"
					if [ $? -eq 0 ]; then
						ANNEXG_OPMODE=DSP
					else
							
						echo "ERROR ---- ERRROR NOTHING FOUNE FOR API OR WANPIPE"
						return 0;
					fi
				fi
			fi
		
			#echo "AFTER ALL ANNEXG MODE IS for $X25_IFACE = $ANNEXG_OPMODE"
		
			if [ "$ANNEXG_OPMODE" = "WANPIPE" ];  then
			
				cat $ANNEXG_INTR_DIR/$X25_IFACE

				if [ -f $ANNEXG_INTR_DIR/$X25_IFACE ]; then

				ifs_tmp=$IFS
				IFS="="
				while read name value 
				do
					value=${value// /}
					name=${name// /}

					#Convert to upper case
					name=`echo $name | tr a-z A-Z`
			
					case $name in
						IPADDR*)
							ANNEXG_L_IP=$value
						;;
						NETMASK*)
							ANNEXG_N_IP=$value
						;;
						POINTOPOINT*)
							ANNEXG_R_IP=$value
						;;
					esac
				
				done < $ANNEXG_INTR_DIR/$X25_IFACE
				IFS=$ifs_tmp
				fi
			fi
		else
			echo "File not found $X25_IF_FILE"
		fi
	fi

	menu_options="'get_operation_mode' 'Operation Mode --------> $ANNEXG_OPMODE'"

	if [ $ANNEXG_OPMODE = WANPIPE ]; then
	menu_options=$menu_options" 'get_local_ip' 'Local IP Addr. --------> $ANNEXG_L_IP' \
		      	  'get_ptp_ip'     'Local P-t-P Addr. -----> $ANNEXG_R_IP' \
			  'get_netmask'    'Local Netmask Addr. ---> $ANNEXG_N_IP'" 
		menu_cnt=$((menu_cnt+3))
	fi	

	menu_options=$menu_options" 2> $MENU_TMP"

	menu_instr="	Configure X25 IP/Protocol Information \
				"
  	menu_name "X25 INTERFACE CONFIGURATION" "$menu_options" "10" "$menu_instr" "$BACK" 20 50 
	rc=$?
	
	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in

	0)
		case $choice in	

		get_local_ip)
			get_string "Please specify the Local IP Address" "$ANNEXG_L_IP" "IP_ADDR" 
			ANNEXG_L_IP=$($GET_RC)
			;;
		get_ptp_ip)
			get_string "Please specify the Point-to-Point IP Address" "$ANNEXG_R_IP" "IP_ADDR"
			ANNEXG_R_IP=$($GET_RC)
			;;
		get_netmask)
			get_string "Please specify the Net Mask Addr" "$ANNEXG_N_IP" "IP_ADDR"
			ANNEXG_N_IP=$($GET_RC)
			;;
		*)
			$choice "$num" "ANNEXG" 
			;;
		esac

		cfg_x25_interface "$num" "$master_ifdev"
		;;

	2) 	return 0
		;;

	*)	
		if [ ! -d "$ANNEXG_INTR_DIR" ]; then
			mkdir -p $ANNEXG_INTR_DIR
		fi
	

		#echo "Configuring $X25_IFACE for $ANNEXG_OPMODE"

		if [ $ANNEXG_OPMODE != WANPIPE ]; then
			if [ -f $ANNEXG_INTR_DIR/$X25_IFACE ]; then
				rm -f $ANNEXG_INTR_DIR/$X25_IFACE
			fi
		fi
	
		X25_IF_FILE=${X25_IF_FILE:-$ANNEXG_DIR/$master_ifdev".x25"}

		if [ -f "$X25_IF_FILE" ]; then
			sed s/$X25_IFACE\ *=.*/"$X25_IFACE = wanpipe$DEVICE_NUM, ,$ANNEXG_OPMODE, x25, ${X25_PROFILE[$num]}"/ $X25_IF_FILE  > tmp.$$
			mv tmp.$$  $X25_IF_FILE
		else	
			echo "$X25_IFACE = wanpipe$DEVICE_NUM, ,$ANNEXG_OPMODE, x25, ${X25_PROFILE[$num]}" >> $X25_IF_FILE
		fi

		if [ $ANNEXG_OPMODE = WANPIPE ]; then
		date=`date`
		cat <<EOM > $ANNEXG_INTR_DIR/$X25_IFACE
# Wanrouter interface configuration file
# name:	$X25_IFACE
# date:	$date
#
DEVICE=$X25_IFACE
IPADDR=$ANNEXG_L_IP
NETMASK=$ANNEXG_N_IP
POINTOPOINT=$ANNEXG_R_IP
ONBOOT=yes
EOM
		fi

		echo -e "\n\n NENAD !!!!"
		cat $X25_IF_FILE
		echo

		return 1
		;;
	
	esac
}


function get_lapb_station () {
 
	local menu_options
	local rc
	local choice

	menu_options="'DTE'    'DTE' \
		      'DCE'    'DCE' 2> $MENU_TMP"

	menu_instr="Please specify the LAPB Station Type \
				"

	menu_name "LAPB STATION SETUP" "$menu_options" 2 "$menu_instr" "$BACK"	
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	
			LAPB_STATION=$choice     #Choice is a command 
			return 0
			;;
		2)	
			device_setup_help "get_lapb_station"	
			get_lapb_station
			;;
		*) 	return 1
			;;
	esac
}

function get_x25mpapi_station () {
 
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
		0) 	
			X25_STATION=$choice     #Choice is a command 
			return 0
			;;
		2)	
			device_setup_help "get_lapb_station"	
			get_x25mpapi_station
			;;
		*) 	return 1
			;;
	esac
}



function create_x25_interfaces()
{

	minsvc=$X25_LOWEST_PVC
	
	while [ $minsvc -le $X25_HIGHEST_PVC -a $minsvc -gt 0 ]
	do
		if_name_tmp="w"$DEVICE_NUM$MASTER_PROT_NAME${DLCI_NUM[$num]}"p"$minsvc

		if [ -f "$ANNEXG_INTR_DIR/$if_name_tmp" ]; then	
			if_opmode_tmp=WANPIPE
		else
			if_opmode_tmp=API
		fi

		#echo "Creating interface $if_name_tmp"
		echo "$if_name_tmp = wanpipe$DEVICE_NUM, $minsvc, $if_opmode_tmp, x25, ${X25_PROFILE[$num]}" >> $X25_INTR_CFG_FILE 
		let "minsvc += 1"
	done


	if [ $X25_HIGHEST_SVC -eq 0 ]; then
		return
	fi

	minsvc=$X25_LOWEST_SVC
	
	#echo "Creating interface cfg for x25 from $X25_LOWEST_SVC to $X25_HIGHEST_SVC in $X25_INTR_CFG_FILE"

	while [ $minsvc -le $X25_HIGHEST_SVC ]
	do
		if_name_tmp="w"$DEVICE_NUM$MASTER_PROT_NAME${DLCI_NUM[$num]}"s"$minsvc

		if [ -f "$ANNEXG_INTR_DIR/$if_name_tmp" ]; then	
			if_opmode_tmp=WANPIPE
		else
			if_opmode_tmp=API
		fi

		#echo "Creating interface $if_name_tmp"
		echo "$if_name_tmp = wanpipe$DEVICE_NUM, , $if_opmode_tmp, x25, ${X25_PROFILE[$num]}" >> $X25_INTR_CFG_FILE 
		let "minsvc += 1"
	done
	
}
