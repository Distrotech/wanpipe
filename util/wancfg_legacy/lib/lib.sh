#!/bin/sh

function setup_name_prefix()
{
	local devnum=$1

	if [ $OS_SYSTEM = "Linux" ]; then
		name_prefix="wp"$devnum	
		name_postfix=""
	else
		name_prefix="wp"${abc_index[$devnum]}	
		name_postfix="0"
	fi
}

function load_driver () {

	local err

	if [ $OS_SYSTEM = "Linux" ]; then
		err=`$MODULE_LOAD $1  > /dev/null 2> /dev/null` 
	elif [ $OS_SYSTEM = "FreeBSD" ]; then
		$MODULE_STAT | grep -wq $1 && return 0
		[ ! -e "$CDEV_WANROUTER" ] && {
			mknod $CDEV_WANROUTER c $CDEV_MAJOR $CDEV_MINOR
		}
		err=`$MODULE_LOAD $1 >/dev/null 2>/dev/null`
	elif [ $OS_SYSTEM = "OpenBSD" -o $OS_SYSTEM = "NetBSD" ]; then
		$MODULE_STAT | grep -wq $1 && return 0
		err=`$MODULE_LOAD -d -o $MODULE_DIR/$1.out -e$1 -p$MODULE_DIR/$POSTINSTALL $MODULE_DIR/$1.o  > /dev/null >/dev/null 2>/dev/null`
	fi
	return $err
}


function init_bstrm_opt()
{

	if [ "$FRAME" = "UNFRAMED" ]; then
		IGNORE_FRONT_END=YES
		SYNC_OPTIONS=0x0001
		RX_SYNC_CHAR=0x7E
		MSYNC_TX_TIMER=0x7E
		MAX_TX_BLOCK=1200
		RX_COMP_LEN=1200
		RX_COMP_TIMER=1500
		return 0;
	fi
	

	if [ $PROTOCOL = WAN_BITSTRM ]; then

		if [ "$DEVICE_TE1_TYPE" = YES ]; then
			SYNC_OPTIONS=0x0010
			RX_SYNC_CHAR=0x00
			MSYNC_TX_TIMER=0x00
			MAX_TX_BLOCK=720
			RX_COMP_LEN=720
			RX_COMP_TIMER=1500
		else
			SYNC_OPTIONS=0x0001
			RX_SYNC_CHAR=0x7E
			MSYNC_TX_TIMER=0x7E
			MAX_TX_BLOCK=4096
			RX_COMP_LEN=4096
			RX_COMP_TIMER=1500
		fi
	fi

	return 0
}

function interface_menu () {

	local rc
	local num="$1"
	local name
	local choice
	local menu_options
	local menu_size


	# Initialize common variables
        # Operation mode is WANPIPE by default
        # Protocol Status is unconfigured by default
        # IP Stataus is unconfigured by default
	
	AUTO_INTR_CFG=${AUTO_INTR_CFG:-NO}

	if [ $AUTO_INTR_CFG = YES ]; then
		if [ -z "${OP_MODE[$num]}" ]; then
		OP_MODE[$num]="API"
		fi
	else
		if [ -z "${OP_MODE[$num]}" ]; then

			if [ "$PROTOCOL" = WAN_ADSL ] && [ "$ADSL_EncapMode" = ETH_LLC_OA -o "$ADSL_EncapMode" = ETH_VC_OA ]; then
				OP_MODE[$num]="PPPoE"
				
			elif [ "$PROTOCOL" = WAN_BITSTRM ]; then
				OP_MODE[$num]="API"
			else
				OP_MODE[$num]="WANPIPE"
			fi
		fi

		if [ "${OP_MODE[$num]}" = PPPoE ] && [ $PROTOCOL != WAN_ADSL ]; then
			OP_MODE[$num]="WANPIPE"
		fi
	fi
	
	if [ -z "${PROT_STATUS[$num]}" ]; then
		PROT_STATUS[$num]="Unconfigured"
	fi

	if [ -z "${IP_STATUS[$num]}" ]; then
		IP_STATUS[$num]="Unconfigured"
	fi

	if [ -z "${ANNEXG_STATUS[$num]}" ]; then
		ANNEXG_STATUS[$num]="Unconfigured"
	fi

	# For each protocol createa special menu
	case $PROTOCOL in

	WAN_FR) 	
		name=$name_prefix"fr"${DLCI_NUM[$num]}	
		GET_DLCI=YES
		if [ $STATION = CPE ] && [ $NUM_OF_INTER -eq 1 ]; then
			GET_DLCI=NO
		fi

		if [ -z ${DLCI_NUM[$num]} ] && [ "$GET_DLCI" = "YES" ]; then
		
	menu_options="'get_dlci'           'DLCI Number  --------> ${DLCI_NUM[$num]}' \\"
	menu_size=1
			echo "$menu_options" > $DEVOPT
		else
			if [ $STATION = CPE ]; then
				DLCI_NUM[$num]=${DLCI_NUM[$num]:-auto}	
				IF_NAME[$num]=${IF_NAME[$num]:-$name}
			fi
	menu_options="'get_dlci'           'DLCI Number  -------------> ${DLCI_NUM[$num]}' \
		      'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \
		      'get_operation_mode' 'Operation Mode -----------> ${OP_MODE[$num]}' \
		      'prot_inter_setup'   'DLCI Protocol Setup ------> ${PROT_STATUS[$num]}' \\"
			echo "$menu_options" > $DEVOPT

			if [ ${OP_MODE[$num]} = WANPIPE ] || \
			   [ ${OP_MODE[$num]} = BRIDGE_NODE ]; then	
	
	menu_options="'ip_setup'           'DLCI IP Address Setup ----> ${IP_STATUS[$num]}' \\"
				echo "$menu_options" >> $DEVOPT
			fi
		
			menu_size=5
		fi
			;;

	WAN_MFR) 	
		name=$name_prefix"mfr"${DLCI_NUM[$num]}	
		if [ -z ${DLCI_NUM[$num]} ]; then
	menu_options="'get_dlci'           'DLCI Number  --------> ${DLCI_NUM[$num]}' \\"
			echo "$menu_options" > $DEVOPT
	menu_size=1
		else
	menu_options="'get_dlci'           'DLCI Number  -------------> ${DLCI_NUM[$num]}' \
		      'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \
		      'get_operation_mode' 'Operation Mode -----------> ${OP_MODE[$num]}' \
		      'prot_inter_setup'   'DLCI Protocol Setup ------> ${PROT_STATUS[$num]}' \\"
		
			echo "$menu_options" > $DEVOPT

			if [ ${OP_MODE[$num]} = WANPIPE ] || \
			[ ${OP_MODE[$num]} = BRIDGE_NODE ]; then	
	
	menu_options="'ip_setup'           'DLCI IP Address Setup ----> ${IP_STATUS[$num]}' \\"
				echo "$menu_options" >> $DEVOPT
			fi

			if [ ${OP_MODE[$num]} = ANNEXG ]; then
				
	menu_options="'annexg_setup_menu'  'Annexg Link Setup --------> ${ANNEXG_STATUS[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
			fi

			menu_size=5
		fi

		;;

	WAN_CHDLC)
	 	name=$name_prefix"chdlc"$name_postfix
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi
	menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \
		      'get_operation_mode' 'Operation Mode -----------> ${OP_MODE[$num]}' \
		      'prot_inter_setup'   'CHDLC Protocol Setup -----> ${PROT_STATUS[$num]}' \\"
		echo "$menu_options" > $DEVOPT	

		if [ ${OP_MODE[$num]} = WANPIPE ] || \
			[ ${OP_MODE[$num]} = BRIDGE_NODE ]; then	
		menu_options="'ip_setup'           'IP Address Setup ---------> ${IP_STATUS[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
		fi


		menu_size=4
		;;

	WAN_BITSTRM)

		name=$name_prefix"bstrm$num"
		menu_size=2
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi

		OP_MODE[$num]=${OP_MODE[$num]:-API}

		STREAM[$num]=${STREAM[$num]:-NO}
		BSTRM_ACTIVE_CH[$num]=${BSTRM_ACTIVE_CH[$num]:-ALL}
		SEVEN_BIT_HDLC[$num]=${SEVEN_BIT_HDLC[$num]:-NO}
		

	menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \
		      'get_operation_mode' 'Operation Mode -----------> ${OP_MODE[$num]}' \\"

		echo "$menu_options" > $DEVOPT

		if [ ${OP_MODE[$num]} = API ] || [ ${OP_MODE[$num]} = SWITCH ] || [ ${OP_MODE[$num]} = TDM_VOICE ]; then
	menu_options="'get_stream'    'HDLC Streaming -----------> ${STREAM[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
			menu_size=$((menu_size+1))
	
			if [ ${STREAM[$num]} = YES ]; then
			menu_options="'get_seven_bit_hdlc'    'Seven Bit HDLC Engine ----> ${SEVEN_BIT_HDLC[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
			menu_size=$((menu_size+1))
			fi
		fi

		if [ "$DEVICE_TE1_TYPE" = YES ]; then

			if [ -z ${BSTRM_ACTIVE_CH[$num]} ]; then
				BSTRM_ACTIVE_CH[$num]=ALL;
			fi
		
	menu_options="'bstrm_get_active_chan' 'Bound T1/E1 Channels  ----> ${BSTRM_ACTIVE_CH[$num]}' \\"
	
			echo "$menu_options" >> $DEVOPT
			menu_size=$((menu_size+1))
		fi
		
		if [ ${OP_MODE[$num]} = SWITCH ]; then
			if [ -z ${BSTRM_SWITCH_DEV[$num]} ]; then
				BSTRM_SWITCH_DEV[$num]="Undefined";
			fi

		menu_options="'bstrm_get_switch_dev'  'Switch Device Name -------> ${BSTRM_SWITCH_DEV[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
			menu_size=$((menu_size+1))
		fi

		if [ ${OP_MODE[$num]} = WANPIPE ]; then

		MPPP_PROT[$num]=${MPPP_PROT[$num]:-MP_PPP}
		MPPP_MODEM_IGNORE[$num]=${MPPP_MODEM_IGNORE[$num]:-NO}
		
		menu_options="'get_ppp_chdlc_mode' 'Protocol  ----------------> ${MPPP_PROT[$num]}' \
		      	      'get_modem_ignore'   'Ignore Modem Status ------> ${MPPP_MODEM_IGNORE[$num]}' \
		              'ip_setup'           'IP Address Setup ---------> ${IP_STATUS[$num]}' \\"
		      	menu_size=$((menu_size+3))
			echo "$menu_options" >> $DEVOPT
		fi

		;;

	WAN_AFT*)
		
	 	name=$name_prefix"aft"$name_postfix
		menu_size=2
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi

		OP_MODE[$num]=${OP_MODE[$num]:-WANPIPE}

		if [ "$DEVICE_TE1_TYPE" = NO ] && [ "$DEVICE_TE3_TYPE" = NO ]; then
			OP_MODE[$num]=API
		fi

		STREAM[$num]=${STREAM[$num]:-YES}
		BSTRM_ACTIVE_CH[$num]=${BSTRM_ACTIVE_CH[$num]:-ALL}
				

	menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \
		      'get_operation_mode' 'Operation Mode -----------> ${OP_MODE[$num]}' \\"

		echo "$menu_options" > $DEVOPT

		if [ ${OP_MODE[$num]} = API ] || [ ${OP_MODE[$num]} = SWITCH ] || [ ${OP_MODE[$num]} = TDM_VOICE ]; then
	menu_options="'get_stream'    'HDLC Streaming -----------> ${STREAM[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
			menu_size=$((menu_size+1))
	
		fi

		if [ "${STREAM[$num]}" = NO ]; then
#NENAD???????
			IDLE_FLAG[$num]=${IDLE_FLAG[$num]:-0x7E}
			if  [ -z ${IDLE_SIZE[$num]} ]; then
				IDLE_SIZE[$num]=$MTU
			fi
			
	menu_options="'get_idle_flag' 'Idle Flag Character ------> ${IDLE_FLAG[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
			menu_size=$((menu_size+1))


		if [ -z ${IFACE_MTU[$num]} ]; then
			IFACE_MTU[$num]=$MTU;
		fi
		if [ -z ${IFACE_MRU[$num]} ]; then
			IFACE_MRU[$num]=$MTU;
		fi
		

	menu_options="'get_iface_mtu'  'Iface MTU ----------------> ${IFACE_MTU[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
			menu_size=$((menu_size+1))
		
	menu_options="'get_iface_mru'  'Iface MRU ----------------> ${IFACE_MRU[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
			menu_size=$((menu_size+1))

		fi

		if [ "$DEVICE_TE1_TYPE" = YES ]; then

			if [ -z ${BSTRM_ACTIVE_CH[$num]} ]; then
				BSTRM_ACTIVE_CH[$num]=ALL;
			fi
		
	menu_options="'bstrm_get_active_chan' 'Bound T1/E1 Channels  ----> ${BSTRM_ACTIVE_CH[$num]}' \\"
	
			echo "$menu_options" >> $DEVOPT
			menu_size=$((menu_size+1))
		
			if [ ${OP_MODE[$num]} = SWITCH ]; then
				if [ -z ${BSTRM_SWITCH_DEV[$num]} ]; then
					BSTRM_SWITCH_DEV[$num]="Undefined";
				fi

		menu_options="'bstrm_get_switch_dev'  'Switch Device Name -------> ${BSTRM_SWITCH_DEV[$num]}' \\"
				echo "$menu_options" >> $DEVOPT
				menu_size=$((menu_size+1))
			fi
		fi

		if [ ${OP_MODE[$num]} = WANPIPE ]; then

			if [ $OS_SYSTEM = "Linux" ]; then
				MPPP_PROT[$num]=${MPPP_PROT[$num]:-MP_FR}
			else
				MPPP_PROT[$num]=${MPPP_PROT[$num]:-MP_CHDLC}
			fi
			MPPP_MODEM_IGNORE[$num]=${MPPP_MODEM_IGNORE[$num]:-NO}
			STREAM[$num]=YES;

			STATION_IF[$num]=${STATION_IF[$num]:-CPE}
			SIGNAL_IF[$num]=${SIGNAL_IF[$num]:-AUTO}

			DLCI_NUM[$num]=${DLCI_NUM[$num]:-auto}
		
			if [ ${DLCI_NUM[$num]} = auto ] && [ ${STATION_IF[$num]} = NODE ]; then
				DLCI_NUM[$num]=16
			fi

			if [ ${MPPP_PROT[$num]} = MP_FR ]; then
			
			menu_options="'get_ppp_chdlc_mode' 'Protocol  ----------------> ${MPPP_PROT[$num]}' \
				      'get_station'	   '  Station  ---------------> ${STATION_IF[$num]}' \
				      'get_signal'	   '  Signalling -------------> ${SIGNAL_IF[$num]}' \
			              'get_dlci'           '  DLCI Number  -----------> ${DLCI_NUM[$num]}' \
				      'get_modem_ignore'   'Ignore Modem Status ------> ${MPPP_MODEM_IGNORE[$num]}' \
				      'ip_setup'           'IP Address Setup ---------> ${IP_STATUS[$num]}' \\"
				menu_size=$((menu_size+6))
				echo "$menu_options" >> $DEVOPT

			else

			menu_options="'get_ppp_chdlc_mode' 'Protocol  ----------------> ${MPPP_PROT[$num]}' \
				      'get_modem_ignore'   'Ignore Modem Status ------> ${MPPP_MODEM_IGNORE[$num]}' \
				      'ip_setup'           'IP Address Setup ---------> ${IP_STATUS[$num]}' \\"
				menu_size=$((menu_size+3))
				echo "$menu_options" >> $DEVOPT
			fi

		fi

		
		;;


	WAN_ADSL)
		name=$name_prefix"adsl"$name_postfix
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi

		menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \\"
		echo "$menu_options" > $DEVOPT

		menu_options="'get_operation_mode' 'Operation Mode -----------> ${OP_MODE[$num]}' \\"
		echo "$menu_options" >> $DEVOPT
	
		PAP[$num]=${PAP[$num]:-NO}
		CHAP[$num]=${CHAP[$num]:-NO}
		
		if [ "$OS_SYSTEM" = Linux ] && [ "$ADSL_EncapMode" = PPP_LLC_OA -o "$ADSL_EncapMode" = PPP_VC_OA ]; then
			if [ ${PAP[$num]} = YES ] || [ ${CHAP[$num]} = YES ]; then
			menu_options="'get_pap'    	'PAP ----------------------> ${PAP[$num]}' \
				      'get_chap'   	'CHAP----------------------> ${CHAP[$num]}' \
				      'get_userid' 	'USERID -------------------> ${USERID[$num]}' \
				      'get_passwd' 	'PASSWD -------------------> ${PASSWD[$num]}' \\"
		        echo "$menu_options" >> $DEVOPT
			else
			menu_options="'get_pap'      'PAP ----------------------> ${PAP[$num]}' \
				      'get_chap'     'CHAP ---------------------> ${CHAP[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
			fi	
		fi	
		
		if [ ${OP_MODE[$num]} = WANPIPE ]; then
		menu_options="'ip_setup'           'IP Address Setup ---------> ${IP_STATUS[$num]}' \\"
		echo "$menu_options" >> $DEVOPT
		fi
		menu_size=5
		;;

	WAN_ATM)
		name=$name_prefix"atm"$num
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi

		atm_vci[$num]=${atm_vci[$num]:-35}
		atm_vpi[$num]=${atm_vpi[$num]:-0}

		atm_encap_mode[$num]=${atm_encap_mode[$num]:-IP_LLC_OA}

		menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \\"
		echo "$menu_options" > $DEVOPT

		menu_options="'get_operation_mode' 'Operation Mode -----------> ${OP_MODE[$num]}' \\"
		echo "$menu_options" >> $DEVOPT

		menu_options="'g_ADSL_EncapMode'   'ATM Encapsulation Mode ---> ${atm_encap_mode[$num]}' \\" 
		echo "$menu_options" >> $DEVOPT
		
		menu_options="'get_atm_vpi'        'ATM VPI ------------------> ${atm_vpi[$num]}' \\" 
		echo "$menu_options" >> $DEVOPT
		
		menu_options="'get_atm_vci'        'ATM VCI ------------------> ${atm_vci[$num]}' \\" 
		echo "$menu_options" >> $DEVOPT
		
		menu_options="'prot_inter_setup'   'ATM/OAM Protocol Setup ---> ${PROT_STATUS[$num]}' \\"
		echo "$menu_options" >> $DEVOPT	
		
		if [ ${OP_MODE[$num]} = WANPIPE ]; then
		menu_options="'ip_setup'           'IP Address Setup ---------> ${IP_STATUS[$num]}' \\"
		echo "$menu_options" >> $DEVOPT
		fi
		
		menu_size=7
		;;
	

	WAN_BSCSTRM)

		name=$name_prefix"bscstrm"
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi
		OP_MODE[$num]=API;
		
		menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \\"
		echo "$menu_options" > $DEVOPT	

		menu_size=3
		;;

	WAN_BSC)

		name=$name_prefix"bsc"
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi
		OP_MODE[$num]=API;
		
		menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \\"
		echo "$menu_options" > $DEVOPT	

		menu_size=3
		;;

	WAN_POS)

		name=$name_prefix"pos"
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi
		OP_MODE[$num]=API;
		
		menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \\"
		echo "$menu_options" > $DEVOPT	

		menu_size=3
		;;


	WAN_MULTP*)
		name=$name_prefix"mp"$name_postfix
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi
		if [ -z ${MPPP_PROT[$num]} ]; then
			MPPP_PROT[$num]="MP_PPP"
		fi
		if [ -z ${MPPP_MODEM_IGNORE[$num]} ]; then
			MPPP_MODEM_IGNORE[$num]="YES"
		fi

		if [ $DEVICE_TYPE = "AFT" ]; then
			MPPP_PROT[$num]="HDLC";		
			OP_MODE[$num]="ANNEXG";
		fi
		
	menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \
		      'get_operation_mode' 'Operation Mode -----------> ${OP_MODE[$num]}' \\"

		echo "$menu_options" > $DEVOPT

		if [ ${OP_MODE[$num]} = WANPIPE ]; then	

	menu_options="'get_ppp_chdlc_mode' 'Protocol  ----------------> ${MPPP_PROT[$num]}' \
		      'get_modem_ignore'   'Ignore Modem Status ------> ${MPPP_MODEM_IGNORE[$num]}' \\"

		echo "$menu_options" >> $DEVOPT	

		PAP[$num]=${PAP[$num]:-NO}
		CHAP[$num]=${CHAP[$num]:-NO}
		
		if [ "$OS_SYSTEM" = Linux ] && [ "${MPPP_PROT[$num]}" = MP_PPP ]; then
			if [ ${PAP[$num]} = YES ] || [ ${CHAP[$num]} = YES ]; then
			menu_options="'get_pap'    	'PAP ----------------------> ${PAP[$num]}' \
				      'get_chap'   	'CHAP----------------------> ${CHAP[$num]}' \
				      'get_userid' 	'USERID -------------------> ${USERID[$num]}' \
				      'get_passwd' 	'PASSWD -------------------> ${PASSWD[$num]}' \\"
		        echo "$menu_options" >> $DEVOPT
			else
			menu_options="'get_pap'      'PAP ----------------------> ${PAP[$num]}' \
				      'get_chap'     'CHAP ---------------------> ${CHAP[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
			fi	
		fi	

		
	menu_options="'ip_setup'           'IP Address Setup ---------> ${IP_STATUS[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
		fi

		if [ ${OP_MODE[$num]} = ANNEXG ]; then
			MPPP_PROT[$num]="HDLC"		
	menu_options="'annexg_setup_menu'  'X25 Link Setup -----------> ${ANNEXG_STATUS[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
		fi


		menu_size=5
		;;


	WAN_SS7)
		name=$name_prefix"ss7"
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi
		OP_MODE[$num]=API;
		
	menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \
		      'get_operation_mode' 'Operation Mode -----------> ${OP_MODE[$num]}' \
		      'prot_inter_setup'   'SS7 Protocol Setup -------> ${PROT_STATUS[$num]}' \\"
		menu_size=4
			echo "$menu_options" >> $DEVOPT
			;;
		

	WAN_PPP)
	 	name=$name_prefix"ppp"$name_postfix
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi
		
	menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \
		      'get_operation_mode' 'Operation Mode -----------> ${OP_MODE[$num]}' \
		      'prot_inter_setup'   'PPP Protocol Setup -------> ${PROT_STATUS[$num]}' \\"

		      echo "$menu_options" > $DEVOPT	

		if [ ${OP_MODE[$num]} = WANPIPE ] || \
			[ ${OP_MODE[$num]} = BRIDGE_NODE ]; then	
	menu_options="'ip_setup'           'IP Address Setup ---------> ${IP_STATUS[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
		fi

			menu_size=4
			;;
	WAN_ADCCP)
		CH_TYPE[$num]=${CH_TYPE[$num]:-PVC}
		LAPB_HDLC_ONLY=YES
		OP_MODE[$num]=API

		name=$name_prefix"adccp"
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi

		menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \\"
		echo "$menu_options" > $DEVOPT	

		menu_size=8;
		;;

	WAN_X25)
		CH_TYPE[$num]=${CH_TYPE[$num]:-SVC}

		#If X25API is to be used as LAPB HDLC only than,
       	 	#only interface allowed is PVC.
		if [ ! -z "$LAPB_HDLC_ONLY" ]; then
			if [ $LAPB_HDLC_ONLY = YES ]; then
				CH_TYPE[$num]="PVC"
			fi
		else
			LAPB_HDLC_ONLY="NO"	
		fi

		name=$name_prefix"svc"$num
		if [ -z ${IF_NAME[$num]} ]; then
			IF_NAME[$num]=$name	
		fi

	menu_options="'get_interface_name' 'Interface Name -----------> ${IF_NAME[$num]}' \
		      'get_operation_mode' 'Operation Mode -----------> ${OP_MODE[$num]}' \
		      'get_channel_type'   'Channel Type -------------> ${CH_TYPE[$num]}' \\"


		echo "$menu_options" > $DEVOPT

		if [ ${CH_TYPE[$num]} = PVC ]; then
			X25_ADDR[$num]=${X25_ADDR[$num]:-1}	
		menu_options="'get_x25_address'    'LCN Number  --------------> ${X25_ADDR[$num]}' \\"
			echo "$menu_options" >> $DEVOPT	
		else
			if [ ${OP_MODE[$num]} = WANPIPE ]; then
		menu_options="'again' 	        'Place Call Addr' \
			      'get_x25_address' '   Destination (-d) Addr -> ${X25_ADDR[$num]}' \
			      'get_src_addr'   	'   Source      (-s) Addr -> ${SRC_ADDR[$num]}' \
			      'again' 	        'Accept Call Addr' \
		    'get_accept_dest_addr'  	'   Accept Dest (-d) Addr -> ${X25_ACC_DST_ADDR[$num]}' \
		    'get_accept_src_addr'       '   Accept Src  (-s) Addr -> ${X25_ACC_SRC_ADDR[$num]}' \
		    'get_accept_usr_data'	'   Accept User (-u) Data -> ${X25_ACC_USR_DATA[$num]}' \\"
			echo "$menu_options" >> $DEVOPT	
			fi
		fi
		

		if [ ${OP_MODE[$num]} = WANPIPE -a ${CH_TYPE[$num]} = SVC ]; then
	menu_options="'prot_inter_setup'   'X25 Protocol Setup -------> ${PROT_STATUS[$num]}' \\"
			echo "$menu_options" >> $DEVOPT	
		else
			PROT_STATUS[$num]="Setup Done"
			IDLE_TIMEOUT[$num]=90
			HOLD_TIMEOUT[$num]=10
		fi

		
	
		
		if [ ${OP_MODE[$num]} = WANPIPE ]; then	
	menu_options="'ip_setup'           'IP Address Setup ---------> ${IP_STATUS[$num]}' \\"
			echo "$menu_options" >> $DEVOPT
		fi

		menu_size=8;
		;;
	esac


	if [ -f "$WAN_SCRIPTS_DIR/wanpipe$DEVICE_NUM-${IF_NAME[$num]}-start" ] || 
	   [ -f "$WAN_SCRIPTS_DIR/wanpipe$DEVICE_NUM-${IF_NAME[$num]}-stop" ]; then
		SCRIPT_CFG[$num]=Enabled
	else
		SCRIPT_CFG[$num]=Disabled
	fi
	

	if [ $menu_size -gt 1 ]; then
		menu_options="'get_cfg_scripts'  'Interface Scripts --------> ${SCRIPT_CFG[$num]}' \\"
		echo "$menu_options" >> $DEVOPT
		menu_size=$((menu_size+1))
	fi
	menu_options="2> $MENU_TMP" 
	echo "$menu_options" >> $DEVOPT
	menu_options=`cat $DEVOPT`
	rm -f $DEVOPT


	#Now that the menu is created, set the menu instructions
        #and print the menu to the screen. Once the menu is 
        #printed wait for user input.

	menu_instr="	Please specify interface parameters below.	\
									"

	menu_name "INTERFACE $num DEFINITION" "$menu_options" "$menu_size" "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP


	#We received user input, now take appropriate action
	case $rc in
		0)	case $choice in
			get_interface_name)
				get_string "Please specify a Network Interface Name" "$name"
				IF_NAME[$num]=$($GET_RC)
				name=${IF_NAME[$num]}
				;;
			get_atm_vci)
					get_integer "Please specify ATM VCI" "${atm_vci[$num]}" "0" "65535" "interface_menu_help get_atm_vci"
					atm_vci[$num]=$($GET_RC)
					;;
			get_atm_vpi)
					get_integer "Please specify ATM VPI" "${atm_vpi[$num]}" "0" "128" "interface_menu_help get_atm_vpi"
					atm_vpi[$num]=$($GET_RC)
					;;

			get_dlci)
				get_integer "Please specify a DLCI number" "16" "16" "16000" "interface_menu_help get_dlci"

				DLCI_NUM[$num]=$($GET_RC)
				if [ $PROTOCOL = WAN_MFR ]; then
					IF_NAME[$num]=$name_prefix"mfr"${DLCI_NUM[$num]}
				else
					IF_NAME[$num]=$name_prefix"fr"${DLCI_NUM[$num]}
				fi
				name=${IF_NAME[$num]}
				;;
			get_seven_bit_hdlc)
				warning get_seven_bit_hdlc
				if [ $? -eq 0 ]; then
					SEVEN_BIT_HDLC[$num]=YES
				else
					SEVEN_BIT_HDLC[$num]=NO
				fi
				;;

			get_idle_flag)
				get_string "Please specify a Idle Flag Character." "${IDLE_FLAG[$num]}" 
				IDLE_FLAG[$num]=$($GET_RC)
				;;

			get_idle_size)
				get_integer "Please specify Idle Buf Size" "${IDLE_SIZE[$num]}" "8" "8188"
				IDLE_SIZE[$num]=$($GET_RC)
				;;

			get_iface_mtu)
				get_integer "Please specify Iface MTU Size" "${IFACE_MTU[$num]}" "8" "8188"
				IFACE_MTU[$num]=$($GET_RC)
				;;

			get_iface_mru)
				get_integer "Please specify Iface MRU Size" "${IFACE_MRU[$num]}" "8" "8188"
				IFACE_MRU[$num]=$($GET_RC)
				;;

			get_x25_address)
				if [ ${CH_TYPE[$num]} = PVC ]; then
				get_integer "Please specify a PVC LCN number." "1" "1" "4096" "interface_menu_help get_x25_address"
				else
				get_string "Please specify the SVC destination/called (-d) address to be used in placing outgoing calls. (Leave blank to disable outgoing calls)." ""
				fi
				X25_ADDR[$num]=$($GET_RC)
				;;
			
			get_src_addr)
				get_string "Please specify the SVC source/calling (-s) address to be used in placing outgoing calls. If blank: not used." ""
				SRC_ADDR[$num]=$($GET_RC)
				;;

			get_accept_dest_addr)
				get_string "Please specify the incoming, SVC, destination/called (-d) address matching pattern. (eg: * accept all)." ""
				X25_ACC_DST_ADDR[$num]=$($GET_RC)
				;;

			get_accept_src_addr)
				get_string "Please specify the incoming, SVC, source/calling (-s) address matching pattern. (eg: * accept all)." ""
				X25_ACC_SRC_ADDR[$num]=$($GET_RC)
				;;

			get_accept_usr_data)
				get_string "Please specify the incoming, SVC, user data (-u) matching pattern. (eg: * accept all)." ""
				X25_ACC_USR_DATA[$num]=$($GET_RC)
				;;

			annexg_setup_menu)
				annexg_setup_menu "$num" "${IF_NAME[$num]}"
				if [ $? -eq 0 ]; then
					ANNEXG_STATUS[$num]="Configured"
				fi
				;;

			get_userid)
				get_string "Please specify you User Id" 
				USERID[$num]=$($GET_RC)
				;;

			get_passwd)
				get_string "Please specify you Password"
				PASSWD[$num]=$($GET_RC)
				;;
				
			get_cfg_scripts)
				
				configure_scripts "wanpipe$DEVICE_NUM" "${IF_NAME[$num]}"
				;;

			again)
				;;

			*)
				$choice "$num"    #Choice is a command 
				;;
			esac
			return 0 
			;;

		1) 	
			#EXIT to previous screen
			return 1
			;;

		2)	
			#HELP
			choice=${choice%\"*\"}
			choice=${choice// /}
			interface_menu_help $choice
			return 0
			;;
		*)	
			#EXIT to previous screen
			return 1
	esac

}

#============================================================
# get_device_setup
#
# 	General Hardware Setup
#	All [wanpipe#] variables configued in the area
#	
#============================================================
function gen_device_setup () {

	local rc
	local choice
	local menu_options
	local menu_options0
	local menu_options1
	local menu_options2
	local menu_opt_prot
	local frim_name
	local menu_size

	if [ -z "$DEVICE_TYPE" ]; then
		DEVICE_TYPE=S514
		DEVICE_TE1_TYPE=NO;
		DEVICE_TE3_TYPE=NO;
	fi

	if [ "$PROTOCOL" = "WAN_ADSL" ]; then
		DEVICE_TYPE=S518
		DEVICE_TE1_TYPE=NO;
		DEVICE_TE3_TYPE=NO;
	fi
	
	if [ "$PROTOCOL" = "WAN_AFT" ]; then
		DEVICE_TYPE=AFT
		DEVICE_TE1_TYPE=YES;
		DEVICE_TE3_TYPE=NO;
	fi

	if [ "$PROTOCOL" = "WAN_AFT_TE3" ]; then
		DEVICE_TYPE=AFT
		DEVICE_TE3_TYPE=YES;
		DEVICE_TE1_TYPE=NO;
	fi
	
	# TE1 Set default value 
	if [ -z "$DEVICE_TE1_TYPE" ]; then
		DEVICE_TE1_TYPE=NO
	fi
	if [ -z "$DEVICE_56K_TYPE" ]; then
		DEVICE_56K_TYPE=NO
	fi

	AUTO_PCI_CFG=${AUTO_PCI_CFG:-YES}
	IGNORE_FRONT_END=${IGNORE_FRONT_END:-NO}

	#Setup protocol specific menues
	if [ $DEVICE_TE1_TYPE = NO ] && [ $DEVICE_TE3_TYPE = NO ]; then
		if [ $DEVICE_56K_TYPE = NO ]; then
		menu_options0="'probe_wanpipe_hw'      '  Probe Hardware  ' \
			       'again'                 ' ' \
			       'get_device_type'       'Adapter Type ---> $DEVICE_TYPE' \\"
		else
		menu_options0="'probe_wanpipe_hw'      '  Probe Hardware  ' \
			       'again'                 ' ' \
		               'get_device_type'       'Adapter Type ---> S514 56K' \\"
		fi
	else
		
		if [ $DEVICE_TE1_TYPE = YES ]; then

			if [ $DEVICE_TYPE = AFT ]; then
			menu_options0="'probe_wanpipe_hw'      '  Probe Hardware  ' \
			               'again'                 ' ' \
				       'get_device_type'       'Adapter Type ---> AFT T1/E1' \\"
			else
			menu_options0="'probe_wanpipe_hw'      '  Probe Hardware  ' \
			               'again'                 ' ' \
				       'get_device_type'       'Adapter Type ---> S514 T1/E1' \\"
			fi

		else
			menu_options0="'probe_wanpipe_hw'      '  Probe Hardware  ' \
			               'again'                 ' ' \
				       'get_device_type'       'Adapter Type ---> AFT T3/E3' \\"
		fi
	fi

	if [ $DEVICE_TYPE = S514 ] || [ $DEVICE_TYPE = AFT ]; then
		if [ $AUTO_PCI_CFG = NO ]; then
		menu_options1="'get_s514_cpu'  'S514CPU --------> $S514CPU' \
			       'get_s514_auto' 'AUTO_PCISLOT ---> $AUTO_PCI_CFG' \
			       'get_s514_slot' 'PCISLOT --------> $PCISLOT' \
			       'get_s514_bus'  'PCIBUS ---------> $PCIBUS' \\"
		else
		menu_options1="'get_s514_cpu'  'S514CPU --------> $S514CPU' \
			       'get_s514_auto' 'AUTO_PCISLOT ---> $AUTO_PCI_CFG' \\"
		fi
	elif [ $DEVICE_TYPE = S518 ]; then
		if [ $AUTO_PCI_CFG = NO ]; then
		menu_options1="'get_s514_auto' 'AUTO_PCISLOT ---> $AUTO_PCI_CFG' \
			       'get_s514_slot' 'PCISLOT --------> $PCISLOT' \
			       'get_s514_bus'  'PCIBUS ---------> $PCIBUS' \\"
		else
		menu_options1="'get_s514_auto' 'AUTO_PCISLOT ---> $AUTO_PCI_CFG' \\"
		fi
	else
		menu_options1="'get_s508_io'   'IOPORT ---------> $IOPORT' \
			       'get_s508_irq'  'IRQ ------------> $IRQ' \\"
	fi


	if  [ $PROTOCOL = WAN_TTYPPP -a $TTY_MODE = Async -o $PROTOCOL = WAN_MLINK_PPP ]; then
	
	menu_options2="'get_firmware'  'Firmware Module-> $FIRMWARE' \
		       'get_memory'    'Memory Addr ----> $MEMORY' \
		       'get_interface' 'Interface ------> $INTERFACE' \
		       'get_mtu'       'MTU  -----------> $MTU' \
		       'get_udpport'   'UDP Port -------> $UDPPORT' \
		       'get_ttl'       'Time to live ---> $TTL' \\"


	elif [ $PROTOCOL = WAN_ADSL ] || [ $PROTOCOL = WAN_AFT ] || [ $PROTOCOL = WAN_AFT_TE3 ]; then

	menu_options2="'get_mtu'       'MTU  -----------> $MTU' \
		       'get_udpport'   'UDP Port -------> $UDPPORT' \
		       'get_ttl'       'Time to live ---> $TTL' \\"

	elif [ $DEVICE_56K_TYPE = YES ] || [ $DEVICE_TE1_TYPE = YES ]; then
	
	INTERFACE="V35"
	CLOCKING="External"
	BAUDRATE="1540000"

	menu_options2="'get_firmware'  'Firmware Module-> $FIRMWARE' \
		       'get_memory'    'Memory Addr ----> $MEMORY' \
		       'get_mtu'       'MTU  -----------> $MTU' \
		       'get_udpport'   'UDP Port -------> $UDPPORT' \
		       'get_ttl'       'Time to live ---> $TTL' \
		       'get_ignore_fe' 'Ignore Front End> $IGNORE_FRONT_END' \\"
	
	elif [ $CLOCKING = "External" ]; then

	BAUDRATE="1540000"
	menu_options2="'get_firmware'  'Firmware Module-> $FIRMWARE' \
		       'get_memory'    'Memory Addr ----> $MEMORY' \
		       'get_interface' 'Interface ------> $INTERFACE' \
		       'get_clocking'  'Clocking -------> $CLOCKING' \
		       'get_mtu'       'MTU  -----------> $MTU' \
		       'get_udpport'   'UDP Port -------> $UDPPORT' \
		       'get_ttl'       'Time to live ---> $TTL' \
		       'get_ignore_fe' 'Ignore Front End> $IGNORE_FRONT_END' \\"

	else

	menu_options2="'get_firmware'  'Firmware Module-> $FIRMWARE' \
		       'get_memory'    'Memory Addr ----> $MEMORY' \
		       'get_interface' 'Interface ------> $INTERFACE' \
		       'get_clocking'  'Clocking -------> $CLOCKING' \
		       'get_baudrate'  'Baud Rate ------> $BAUDRATE' \
		       'get_mtu'       'MTU  -----------> $MTU' \
		       'get_udpport'   'UDP Port -------> $UDPPORT' \
		       'get_ttl'       'Time to live ---> $TTL' \
		       'get_ignore_fe' 'Ignore Front End> $IGNORE_FRONT_END' \\"
	fi

	case $PROTOCOL in

	WAN_FR)
	menu_opt_prot="'get_station'   'Station --------> $STATION' \
		       'get_signal'    'Signalling -----> $SIGNAL' \
		       'get_T391'      'T391 -----------> $T391' \
		       'get_T392'      'T392 -----------> $T392' \
		       'get_N391'      'N391 -----------> $N391' \
		       'get_N392'      'N392 -----------> $N392' \
		       'get_N393'      'N393 -----------> $N393' \
		       'get_fs_issue'   'Fast Connect ---> $FR_ISSUE_FS' \\"

		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT
		echo "$menu_opt_prot" >> $DEVOPT
		menu_size=13
		firm_name=fr514.sfm
		;;

	WAN_MFR)
		menu_opt_prot="'get_commport'  'Comm Port ------> $COMMPORT' \
		       'get_station'   'Station --------> $STATION' \
		       'get_signal'    'Signalling -----> $SIGNAL' \
		       'get_T391'      'T391 -----------> $T391' \
		       'get_T392'      'T392 -----------> $T392' \
		       'get_N391'      'N391 -----------> $N391' \
		       'get_N392'      'N392 -----------> $N392' \
		       'get_N393'      'N393 -----------> $N393' \
		       'get_fs_issue'   'Fast Connect ---> $FR_ISSUE_FS' \\"

		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT
		echo "$menu_opt_prot" >> $DEVOPT
		menu_size=13
		firm_name=cdual514.sfm
		;;

	WAN_CHDLC)
	
		CONNECTION=${CONNECTION:-Permanent};
		LINECODING=${LINECODING:-NRZ};
		LINEIDLE=${LINEIDLE:-Flag};

		menu_opt_prot="'get_commport'  'Comm Port ------> $COMMPORT' \\";
		menu_opt_prot1="'get_connection' 'Connection -----> $CONNECTION' \
		                'get_linecode'  'Line Coding ----> $LINECODING' \
				'get_lineidle'  'Line Idle ------> $LINEIDLE' \\"

		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_opt_prot" >> $DEVOPT

		if [ $DEVICE_TYPE = S514 ]; then
			menu_opt_prot="'get_rec_only' 'Receive Only ---> $REC_ONLY' \\"
			echo "$menu_opt_prot" >> $DEVOPT
		fi	
		echo "$menu_options2" >> $DEVOPT
		echo "$menu_opt_prot1" >> $DEVOPT
		menu_size=13
		firm_name=cdual514.sfm
		;;

	WAN_BITSTRM)
		COMMPORT=${COMMPORT:-PRI}
	
		menu_opt_prot="'get_commport'  'Comm Port ------> $COMMPORT' \\"

		SYNC_OPTIONS=${SYNC_OPTIONS:-0x0001}
		RX_SYNC_CHAR=${RX_SYNC_CHAR:-0x7E}
		MSYNC_TX_TIMER=${MSYNC_TX_TIMER:-0x7E}
		MAX_TX_BLOCK=${MAX_TX_BLOCK:-1200}
		RX_COMP_LEN=${RX_COMP_LEN:-1200}
		RX_COMP_TIMER=${RX_COMP_TIMER:-1500}
		RBS_CH_MAP=${RBS_CH_MAP:-0}
	
		menu_opt_prot1="'get_sync_opt'     'Sync Options --------> $SYNC_OPTIONS' \
			       'get_rx_sync_ch'    'Rx sync char --------> $RX_SYNC_CHAR' \
			       'get_msync_fill_ch' 'Monosync fill char --> $MSYNC_TX_TIMER' \
			       'get_max_tx_block'  'Max tx block len ----> $MAX_TX_BLOCK' \
			       'get_rx_comp_length' 'Rx complete len -----> $RX_COMP_LEN' \
			       'get_rx_comp_timer' 'Rx complete timer ---> $RX_COMP_TIMER' \
			       'get_rbs_ch_map'    'Robbits ch map ------> $RBS_CH_MAP' \\"
		
		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_opt_prot" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT
		echo "$menu_opt_prot1" >> $DEVOPT


		menu_size=13
		firm_name=bitstrm.sfm
		;;

	WAN_AFT*)	
		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT

		menu_size=12
		firm_name=cdual514.sfm
		;;

	WAN_ADSL)

		ADSL_EncapMode=${ADSL_EncapMode:-ETH_LLC_OA}
		ADSL_ATM_autocfg=${ADSL_ATM_autocfg:-NO}
		ADSL_Vci=${ADSL_Vci:-35}
		ADSL_Vpi=${ADSL_Vpi:-0}

		ADSL_WATCHDOG=${ADSL_WATCHDOG:-YES}

		ADSL_RxBufferCount=${ADSL_RxBufferCount:-50}
		ADSL_TxBufferCount=${ADSL_TxBufferCount:-50}
		ADSL_Standard=${ADSL_Standard:-ADSL_MULTIMODE}
		ADSL_Trellis=${ADSL_Trellis:-ADSL_TRELLIS_ENABLE}
		ADSL_TxPowerAtten=${ADSL_TxPowerAtten:-0}
		ADSL_CodingGain=${ADSL_CodingGain:-ADSL_AUTO_CODING_GAIN}
		ADSL_MaxBitsPerBin=${ADSL_MaxBitsPerBin:-0x0E}
		ADSL_TxStartBin=${ADSL_TxStartBin:-0x06}
		ADSL_TxEndBin=${ADSL_TxEndBin:-0x1F}	
		ADSL_RxStartBin=${ADSL_RxStartBin:-0x20}
		ADSL_RxEndBin=${ADSL_RxEndBin:-0xFF}
		ADSL_RxBinAdjust=${ADSL_RxBinAdjust:-ADSL_RX_BIN_DISABLE}
		ADSL_FramingStruct=${ADSL_FramingStruct:-ADSL_FRAMING_TYPE_3}
		ADSL_ExpandedExchange=${ADSL_ExpandedExchange:-ADSL_EXPANDED_EXCHANGE}
		ADSL_ClockType=${ADSL_ClockType:-ADSL_CLOCK_CRYSTAL}
		ADSL_MaxDownRate=${ADSL_MaxDownRate:-8192}

		if [ $ADSL_ATM_autocfg = "YES" ]; then
		menu_opt_prot1="'again'			' ' \
				'g_ADSL_EncapMode'	'Encapsulation Mode ---> $ADSL_EncapMode' \
				'ADSL_ATM_autocfg'	'ATM Auto Config ------> $ADSL_ATM_autocfg' \
				'adsl_watchdog'         'ADSL Line Watchdog ---> $ADSL_WATCHDOG ' \\"
				
		else
		menu_opt_prot1="'again'			' ' \
				'g_ADSL_EncapMode'	'Encapsulation Mode ---> $ADSL_EncapMode' \
				'ADSL_ATM_autocfg'	'ATM Auto Config ------> $ADSL_ATM_autocfg' \
				'ADSL_Vci'	        'Vci ------------------> $ADSL_Vci' \
				'ADSL_Vpi'	        'Vpi ------------------> $ADSL_Vpi' \
				'adsl_watchdog'         'ADSL Line Watchdog ---> $ADSL_WATCHDOG ' \\"
		fi		

		menu_opt_prot11=""

#NC: Used by PPPD which is not used any more
#		if [ "$OS_SYSTEM" = Linux ] && [ "$PROTOCOL" = WAN_ADSL ] && [ "$ADSL_EncapMode" = PPP_LLC_OA -o "$ADSL_EncapMode" = PPP_VC_OA ]; then
#			TTY_MINOR=${TTY_MINOR:-0}
#			menu_opt_prot11="'get_tty_minor' 'TTY Port/Minor -------> $TTY_MINOR' \\"
#		fi	

		menu_opt_prot2="'again'			' ' \
				'again'			'----- Advanced -------' \
				'again'			' ' \
				'ADSL_RxBufferCount'   	'RxBufferCount --------> $ADSL_RxBufferCount' \
				'ADSL_TxBufferCount'   	'TxBufferCount --------> $ADSL_TxBufferCount' \
				'adsl_get_std_item'	'AdslStandard ---------> $ADSL_Standard' \
				'adsl_get_trellis'   	'AdslTrellis ----------> $ADSL_Trellis' \
				'ADSL_TxPowerAtten'	'AdslTxPowerAtten -----> $ADSL_TxPowerAtten' \
				'adsl_get_codinggain'	'AdslCodingGain -------> $ADSL_CodingGain' \
				'ADSL_MaxBitsPerBin'	'AdslMaxBitsPerBin ----> $ADSL_MaxBitsPerBin' \
				'ADSL_TxStartBin'	'AdslTxStartBin -------> $ADSL_TxStartBin' \
				'ADSL_TxEndBin'		'AdslTxEndBin ---------> $ADSL_TxEndBin' \
				'ADSL_RxStartBin'	'AdslRxStartBin -------> $ADSL_RxStartBin' \
				'ADSL_RxEndBin'		'AdslRxEndBin ---------> $ADSL_RxEndBin' \
				'adsl_get_rxbinadjust'	'AdslRxBinAdjust ------> $ADSL_RxBinAdjust' \
				'adsl_get_framingtype'	'AdslFramingStruct ----> $ADSL_FramingStruct' \
				'adsl_get_expandedexchange'	'AdslExpandedExchange -> $ADSL_ExpandedExchange' \
				'adsl_get_clocktype'	'AdslClockType --------> $ADSL_ClockType' \
				'ADSL_MaxDownRate'	'AdslMaxDownRate ------> $ADSL_MaxDownRate' \\"
		

		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT
		echo "$menu_opt_prot1" >> $DEVOPT
		if [ "$menu_opt_prot11" != "" ]; then
			echo "$menu_opt_prot11" >> $DEVOPT
		fi
		echo "$menu_opt_prot2" >> $DEVOPT

		menu_size=13
		;;


	WAN_BSCSTRM)
		COMMPORT=${COMMPORT:-PRI}

		BSCSTRM_BAUD=${BSCSTRM_BAUD:-0}
	        BSCSTRM_ADAPTER_FR=${BSCSTRM_ADAPTER_FR:-0}
 	        BSCSTRM_MTU=${BSCSTRM_MTU:-1000}
  		BSCSTRM_EBCDIC=${BSCSTRM_EBCDIC:-1}
  		BSCSTRM_RB_BLOCK_TYPE=${BSCSTRM_RB_BLOCK_TYPE:-1}
  		BSCSTRM_NO_CONSEC_PAD_EOB=${BSCSTRM_NO_CONSEC_PAD_EOB:-3}
  		BSCSTRM_NO_ADD_LEAD_TX_SYN_CHARS=${BSCSTRM_NO_ADD_LEAD_TX_SYN_CHARS:-0}
  		BSCSTRM_NO_BITS_PER_CHAR=${BSCSTRM_NO_BITS_PER_CHAR:-8}
  		BSCSTRM_PARITY=${BSCSTRM_PARITY:-0}
  		BSCSTRM_MISC_CONFIG_OPTIONS=${BSCSTRM_MISC_CONFIG_OPTIONS:-0}
  		BSCSTRM_STATISTICS_OPTIONS=${BSCSTRM_STATISTICS_OPTIONS:-0x0103}
  		BSCSTRM_MODEM_CONFIG_OPTIONS=${BSCSTRM_MODEM_CONFIG_OPTIONS:-0}


		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT

		menu_opt_prot2="'again'			' ' \
				'again'			'-------- Advanced -------' \
				'again'			' ' \
				'BSCSTRM_ADAPTER_FR'    'ADAPTER FR ---------> $BSCSTRM_ADAPTER_FR'  \
				'BSCSTRM_MTU'           'BSC MTU ------------> $BSCSTRM_MTU'  \
				'BSCSTRM_EBCDIC'        'EBCDIC -------------> $BSCSTRM_EBCDIC'  \
 			    'BSCSTRM_RB_BLOCK_TYPE'     'RB BLOCK TYPE ------> $BSCSTRM_RB_BLOCK_TYPE'  \
			    'BSCSTRM_NO_CONSEC_PAD_EOB' 'NO CONSEC PAD ------> $BSCSTRM_NO_CONSEC_PAD_EOB'  \
  	             'BSCSTRM_NO_ADD_LEAD_TX_SYN_CHARS' 'NO ADD LEAD TX SYN -> $BSCSTRM_NO_ADD_LEAD_TX_SYN_CHARS'  \
		            'BSCSTRM_NO_BITS_PER_CHAR'  'NO BITS PER CHAR ---> $BSCSTRM_NO_BITS_PER_CHAR'  \
			    'BSCSTRM_PARITY'            'BSC PARITY ---------> $BSCSTRM_PARITY'  \
                          'BSCSTRM_MISC_CONFIG_OPTIONS' 'MISC CONFIG OPT ----> $BSCSTRM_MISC_CONFIG_OPTIONS'  \
		  	   'BSCSTRM_STATISTICS_OPTIONS' 'STATISTICS OPT -----> $BSCSTRM_STATISTICS_OPTIONS'  \
		         'BSCSTRM_MODEM_CONFIG_OPTIONS' 'MODEM CONFIG OPT ---> $BSCSTRM_MODEM_CONFIG_OPTIONS' \\"
				

		echo "$menu_opt_prot2"  >> $DEVOPT


		menu_size=12
		firm_name=bscstrm514.sfm
		;;


	WAN_ATM)
		COMMPORT=${COMMPORT:-PRI}
	
		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT

		ATM_SYNC_MODE=${ATM_SYNC_MODE:-AUTO}
		ATM_SYNC_DATA=${ATM_SYNC_DATA:-0x0152}
		ATM_SYNC_OFFSET=${ATM_SYNC_OFFSET:-3}
		ATM_HUNT_TIMER=${ATM_HUNT_TIMER:-100}
		ATM_CELL_CFG=${ATM_CELL_CFG:-0}
		ATM_CELL_PT=${ATM_CELL_PT:-0}
		ATM_CELL_CLP=${ATM_CELL_CLP:-1}
		ATM_CELL_PAYLOAD=${ATM_CELL_PAYLOAD:-0x6A}

		if [ "$SYNC_OPTIONS" = "AUTO" ]; then
		menu_opt_prot2="'get_atm_sync_mode'   'Atm Sync Mode ------> $SYNC_OPTIONS' \\"
		else
		menu_opt_prot2="'get_atm_sync_mode'   'Atm Sync Mode ------> $ATM_SYNC_MODE'  \
				'get_atm_sync_bytes'  'Atm Sync Bytes -----> $ATM_SYNC_DATA'  \
				'get_atm_sync_offset' 'Atm Sync Offset ----> $ATM_SYNC_OFFSET' \\"
		fi
		menu_opt_prot3="'again'                '------------'                           \
				'get_atm_hunt_timer'   'Atm Hunt Timer -----> $ATM_HUNT_TIMER'  \
				'get_atm_cell_cfg'     'Atm Cell CFG -------> $ATM_CELL_CFG'    \
				'get_atm_cell_pt'      'Atm Cell PT --------> $ATM_CELL_PT'     \
				'get_atm_cell_clp'     'Atm Cell CLP -------> $ATM_CELL_CLP'    \
				'get_atm_cell_payload' 'Atm Cell Payload ---> $ATM_CELL_PAYLOAD' \\"
		                  
		
		echo "$menu_opt_prot2" >> $DEVOPT
		echo "$menu_opt_prot3" >> $DEVOPT
		
		menu_size=14
		firm_name=atm514.sfm
		;;


	WAN_BSC)
		COMMPORT=${COMMPORT:-PRI}
	
		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT

		menu_size=12
		firm_name=bscmp514.sfm
		;;

	WAN_POS)
		COMMPORT=${COMMPORT:-PRI}

		pos_protocol=${pos_protocol:=IBM4680}

		menu_opt_prot1="'get_firmware'  'Firmware Module-> $FIRMWARE' \
			       'get_memory'     'Memory Addr ----> $MEMORY' \
			       'get_pos_prot'	'POS Protocol ---> $pos_protocol' \\"
	
		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_opt_prot1" >> $DEVOPT

		menu_size=12

		
		firm_name=${firm_name:-pos4680.sfm}
		;;


	WAN_SS7)
		COMMPORT=${COMMPORT:-PRI}
	
		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT

		menu_size=12
		firm_name=ss7.sfm
		;;

	WAN_MULTP*)
		menu_opt_prot="'get_commport'  'Comm Port ------> $COMMPORT' \\"
		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_opt_prot" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT
		
		menu_size=13
		firm_name=cdual514.sfm
		;;
	WAN_EDU_KIT)
		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "'get_firmware'  'Firmware Module-> $FIRMWARE' \\" >> $DEVOPT
		menu_size=8
		firm_name=edu_kit.sfm
		;;

	WAN_TTYPPP)
		if [ $TTY_MODE = Async ]; then
		menu_opt_prot="'get_tty_minor' 'TTY Port/Minor -> $TTY_MINOR' \
			       'get_tty_mode'  'TTY Mode -------> $TTY_MODE' \\" 
			       
		else
		menu_opt_prot="'get_commport'  'Comm Port ------> $COMMPORT' \
		               'get_tty_minor' 'TTY Port/Minor -> $TTY_MINOR' \
			       'get_tty_mode'  'TTY Mode -------> $TTY_MODE' \\" 
		fi
		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_opt_prot" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT
		
		menu_size=14
		firm_name=cdual514.sfm
		;;
	
	WAN_PPP)
	menu_opt_prot="'get_ipmode'    'IP Mode --------> $IPMODE' \\"
		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT
		echo "$menu_opt_prot" >> $DEVOPT
		menu_size=12
		firm_name=ppp514.sfm
		;;

	WAN_ADCCP)
	menu_opt_prot="'get_hdlc_station' 'Station --------> $STATION' \
		       'get_hdlc_win'     'HDLC Window ----> $HDLC_WIN' \
		       'get_logging'      'Call Logging ---> $CALL_LOGGING' \
		       'get_oob_modem'	  'OOB Modem Msg --> $OOB_ON_MODEM' \
		       'get_T1'      	  'T1 -------------> $X25_T1' \
		       'get_T2'      	  'T2 -------------> $X25_T2' \
		       'get_T4'      	  'T4 -------------> $X25_T4' \
		       'get_N2'      	  'N2 -------------> $X25_N2' \
		       'get_hdlc_st_addr' 'Station Addr ---> $HDLC_STATION_ADDR' \\"

		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT
		echo "$menu_opt_prot" >> $DEVOPT
		menu_size=13
		firm_name=adccp.sfm
		;;
	WAN_X25)
	menu_opt_prot="'get_hdlc_station' 'Station --------> $STATION' \
		       'get_low_pvc'   	  'Lowest PVC -----> $LOW_PVC' \
		       'get_high_pvc'     'Highest PVC ----> $HIGH_PVC' \
		       'get_low_svc'      'Lowest SVC -----> $LOW_SVC' \
		       'get_high_svc'     'Highest SVC-----> $HIGH_SVC' \
		       'get_hdlc_win'     'HDLC Window ----> $HDLC_WIN' \
		       'get_packet_win'   'Packet Window --> $PACKET_WIN' \
		       'get_def_pkt_size' 'Def Packet Size-> $DEF_PKT_SIZE' \
		       'get_max_pkt_size' 'Max Packet Size-> $MAX_PKT_SIZE' \
		       'get_ccitt'        'CCITT Compat. --> $CCITT' \
		       'get_hdlc_only'    'LAPB HDLC Only -> $LAPB_HDLC_ONLY' \
		       'get_logging'      'Call Logging ---> $CALL_LOGGING' \
		       'get_oob_modem'	  'OOB Modem Msg --> $OOB_ON_MODEM' \
		       'get_T1'      	  'T1 -------------> $X25_T1' \
		       'get_T2'      	  'T2 -------------> $X25_T2' \
		       'get_T4'      	  'T4 -------------> $X25_T4' \
		       'get_N2'      	  'N2 -------------> $X25_N2' \
		       'get_T10_T20'   	  'T10_T20 --------> $X25_T10_T20' \
		       'get_T11_T21'   	  'T11_T21 --------> $X25_T11_T21' \
		       'get_T12_T22'   	  'T12_T22 --------> $X25_T12_T22' \
                       'get_T13_T23'   	  'T13_T23 --------> $X25_T13_T23' \
		       'get_T16_T26'   	  'T16_T26 --------> $X25_T16_T26' \
		       'get_T16_T26'   	  'T16_T26 --------> $X25_T16_T26' \
		       'get_T28'   	  'T28 ------------> $X25_T28' \
		       'get_R10_R20'   	  'R10_R20 --------> $X25_R10_R20' \
		       'get_R12_R22'   	  'R12_R22 --------> $X25_R12_R22' \
		       'get_R13_R23'   	  'R13_R23 --------> $X25_R13_R23' \\"		
		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT
		echo "$menu_opt_prot" >> $DEVOPT
		menu_size=13
		firm_name=x25_514.sfm
		;;
	WAN_MLINK_PPP)
		echo "$menu_options0" > $DEVOPT
		echo "$menu_options1" >> $DEVOPT
		echo "$menu_options2" >> $DEVOPT
		
		menu_size=12
		firm_name=cdual514.sfm
		;;
	*) 
		echo "ERROR, Protocol ($PROTOCOL) not specified"
		return 1
		;;
	esac

	echo " 2> $MENU_TMP" >> $DEVOPT


	#The menues are completed, now print the menu to the user and 
        #wait for result

	menu_options=`cat $DEVOPT` 
	rm -f $DEVOPT

	menu_instr="	Physical Link: wanpipe$DEVICE_NUM Configuration  \
					"

	menu_name "WANPIPE$DEVICE_NUM HARDWARE CONFIGURATION" "$menu_options" "$menu_size" "$menu_instr" "$BACK" 20 50 
	rc=$?
	
	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	#We got the result, take appropriate action 
	case $rc in
		0)	case $choice in

				ADSL_ATM_autocfg)
					warning "ADSL_ATM_autocfg"
					if [ $? -eq 0 ]; then
						ADSL_ATM_autocfg=YES
					else
						ADSL_ATM_autocfg=NO
					fi
					;;
		
				get_s514_auto)
					warning "AUTO_PCI_CFG"
					if [ $? -eq 0 ]; then
						AUTO_PCI_CFG=YES
					else
						AUTO_PCI_CFG=NO
					fi
					;;
				get_s514_slot) 
					get_integer "Please specify a PCI Slot Number (default 0)" "0" "0" "100" "device_setup_help get_s514_slot" 
					PCISLOT=$($GET_RC)      #Choice is a command 
					;;
				get_s514_bus) 
					get_integer "Please specify a PCI Bus Number (default 0)" "0" "0" "100" "device_setup_help get_s514_bus" 
					PCIBUS=$($GET_RC)      #Choice is a command 
					;;
				get_udpport)
					get_integer "Please specify a UDP Port (default 9000, 0 to disable)" "9000" "0" "9999" "device_setup_help get_udpport"
					UDPPORT=$($GET_RC)
					;;
				get_T391)
					get_integer "Please specify the T391 timer (default 10)" "10" "5" "30" "device_setup_help get_T391"
					T391=$($GET_RC)
					;;
				get_T392)
					get_integer "Please specify the T392 timer (default 16)" "16" "5" "30" "device_setup_help get_T392"
					T392=$($GET_RC)
					;;
				get_N391)
					get_integer "Please specify the N391 timer (default 2)" "6" "1" "255" "device_setup_help get_N391"
					N391=$($GET_RC)
					;;	
				get_N392)
					get_integer "Please specify the N392 timer (default 3)" "3" "1" "10" "device_setup_help get_N392"
					N392=$($GET_RC)
					;;
				get_N393)
					get_integer "Please specify the N393 timer (default 4)" "4" "1" "10" "device_setup_help get_N393"
					N393=$($GET_RC)
					;;
				get_ttl)
					get_integer "Please specify the TTL value (default 255)" "255" "1" "255" "device_setup_help get_ttl"
					TTL=$($GET_RC)
					;;
			        get_s508_irq)
					get_integer "Please specify an IRQ number" "7" "1" "15" "device_setup_help get_s508_irq"
					IRQ=$($GET_RC)
					;;

				get_hdlc_st_addr)
					get_integer "Please specify HDLC Station Address" "1" "1" "255" "device_setup_help get_hdlc_st_addr"
					HDLC_STATION_ADDR=$($GET_RC)
					;;

				get_T1)
					get_integer "Please specify the HDLC T1 Timer value" "3" "1" "30" "device_setup_help get_T1"
					X25_T1=$($GET_RC)
					;;
				get_T2)
					if [ $X25_T1 -gt 0 ]; then
						t2_max=$((X25_T1-1))
					else
						t2_max=0
					fi
					get_integer "Please specify the HDLC T2 Timer value. (Must be lower than T1=$X25_T1)." "0" "0" "$t2_max" "device_setup_help get_T2"
					X25_T2=$($GET_RC)
					;;
				get_T4)
					get_integer "Please specify the HDLC T4 Timer value" "1" "0" "240" "device_setup_help get_T4"
					X25_T4=$($GET_RC)
					;;
				
				get_N2)
					get_integer "Please specify the HDLC N2 Timer value" "10" "1" "30" "device_setup_help get_N2"
					X25_N2=$($GET_RC)
					;;

				get_T10_T20)
					get_integer "Please specify the HDLC T10_T20 Timer value" "30" "1" "255" "device_setup_help get_T10_T20"
					X25_T10_T20=$($GET_RC)
					;;
				get_T11_T21)
					get_integer "Please specify the HDLC T11_T21 Timer value" "30" "1" "255" "device_setup_help get_T11_T21"
					X25_T11_T21=$($GET_RC)
					;;
				get_T12_T22)
					get_integer "Please specify the HDLC T12_T22 Timer value" "30" "1" "255" "device_setup_help get_T12_T22"
					X25_T12_T22=$($GET_RC)
					;;
				get_T13_T23)
					get_integer "Please specify the HDLC T13_T23 Timer value" "30" "1" "255" "device_setup_help get_T13_T23"
					X25_T13_T23=$($GET_RC)
					;;
				get_T16_T26)
					get_integer "Please specify the HDLC T16_T26 Timer value" "30" "1" "255" "device_setup_help get_T16_T26"
					X25_T16_T26=$($GET_RC)
					;;
				get_T28)
					get_integer "Please specify the HDLC T28 Timer value" "30" "1" "255" "device_setup_help get_T28"
					X25_T28=$($GET_RC)
					;;
				get_R10_R20)
					get_integer "Please specify the HDLC R10_R20 Timer value" "30" "0" "250" "device_setup_help get_R10_R20"
					X25_R10_20=$($GET_RC)
					;;
				get_R12_R22)
					get_integer "Please specify the HDLC R12_R22 Timer value" "30" "0" "250" "device_setup_help get_R12_R22"
					X25_R12_22=$($GET_RC)
					;;
				get_R13_R23)
					get_integer "Please specify the HDLC R13_R23 Timer value" "30" "0" "250" "device_setup_help get_R13_R23"
					X25_R13_23=$($GET_RC)
					;;
				
				get_tty_minor*)
					get_integer "Please specify the TTY Port/Minor number: /dev/ttyWPX where X={0,1,2,3}" "0" "0" "3" "device_setup_help get_tty_minor"
					TTY_MINOR=$($GET_RC)
					;;
				
				get_firmware)
					while true
					do
						get_string "Specify the location of the Firmware (.sfm) module: Absolute Path" "$FIRMWARE" 
						FIRMWARE=$($GET_RC)
						if [ -d "$FIRMWARE" ]; then
							break
						else
							error "FIRMWARE"
						fi
					done
					;;
				get_baudrate)
					if [ $DEVICE_TYPE = S514 ]; then
					get_integer "Please specify the Baud Rate in bps" "1540000" "64" "4096000" "device_setup_help get_baudrate"

					else
					get_integer "Please specify the Baud Rate in bps" "1540000" "64" "2048000" "device_setup_help get_baudrate" 
					fi
					BAUDRATE=$($GET_RC)
					;;
				get_mtu)
					dflt_mtu=1500;
					get_integer "Please specify IP Maximum Packet Size (MTU)" "$dflt_mtu" "128" "4098" "device_setup_help get_mtu"
					MTU=$($GET_RC)
					;;
				get_low_pvc)
					get_integer "Please specify Lowest PVC Channel" "0" "0" "100000" "device_setup_help get_low_pvc"
					LOW_PVC=$($GET_RC)
					;;
				get_high_pvc)
					get_integer "Please specify Highest PVC Channel" "0" "0" "100000" "device_setup_help get_high_pvc"
					HIGH_PVC=$($GET_RC)
					;;
				get_low_svc)
					get_integer "Please specify Lowest SVC Channel" "0" "0" "100000" "device_setup_help get_low_svc"
					LOW_SVC=$($GET_RC)
					;;
				get_high_svc)
					get_integer "Please specify Highest SVC Channel" "0" "0" "100000" "device_setup_help get_high_svc"
					HIGH_SVC=$($GET_RC)
					;;
				get_hdlc_win) 
					get_integer "Please specify HDLD Window Size" "7" "1" "7" "device_setup_help get_hdlc_win"
					HDLC_WIN=$($GET_RC)
					;;
				get_packet_win)
					get_integer "Please specify Packet Window Size" "7" "1" "7" "device_setup_help get_packet_win"
					PACKET_WIN=$($GET_RC)				
					;;

				get_def_pkt_size)
					get_integer "Please specify X25 Default Packet Size" "1024" "16" "1024" "device_setup_help get_def_pkt_size"
					DEF_PKT_SIZE=$($GET_RC)				
					;;

				get_max_pkt_size)
					get_integer "Please specify X25 Maximum Packet Size" "1024" "16" "1024" "device_setup_help get_max_pkt_size"
					MAX_PKT_SIZE=$($GET_RC)				
					;;

				get_sync_opt)
					get_string "Please specify bitstrm Sync options" "$SYNC_OPTIONS" 
					SYNC_OPTIONS=$($GET_RC)
					;;

 				get_rx_sync_ch)
					get_string "Please specify bitstrm Rx sync char" "$RX_SYNC_CHAR"
					RX_SYNC_CHAR=$($GET_RC)
					;;
					
			 get_msync_fill_ch)
			       		get_string "Please specify bitstrm Monosync fill char" "$MSYNC_TX_TIMER"
					MSYNC_TX_TIMER=$($GET_RC)
					;;
					
			 get_max_tx_block)
			       		get_string "Please specify bitstrm Max tx block len" "$MAX_TX_BLOCK" 
					MAX_TX_BLOCK=$($GET_RC)
					;;
					
			get_rx_comp_length)
			       		get_string "Please specify bitstrm Rx complete len" "$RX_COMP_LEN" 
 					RX_COMP_LEN=$($GET_RC)
					;;
					
			get_rx_comp_timer)
			       		get_string "Please specify bitstrm Rx complete timer" "$RX_COMP_TIMER" 
					RX_COMP_TIMER=$($GET_RC)
					;;

			get_atm_sync_bytes)
					get_string "Please specify ATM Sync Data" "$ATM_SYNC_DATA" 
					ATM_SYNC_DATA=$($GET_RC)
					;;

			get_atm_sync_offset)
					get_string "Please specify ATM Sync Offset" "$ATM_SYNC_OFFSET" 
					ATM_SYNC_OFFSET=$($GET_RC)
					;;

			get_atm_hunt_timer)   
					get_string "Please specify ATM Hunt Timer"  "$ATM_HUNT_TIMER"
					ATM_HUNT_TIMER=$($GET_RC)
					;;
		
			get_atm_cell_cfg)
					get_string "Please specify ATM Cell CFG" "$ATM_CELL_CFG"
					ATM_CELL_CFG=$($GET_RC)
					;;
		
			get_atm_cell_pt)
					get_string "Please specify ATM Cell PT" "$ATM_CELL_PT"
					ATM_CELL_PT=$($GET_RC)
					;;

			get_atm_cell_clp)
					get_string "Please specify ATM Cell CLP" "$ATM_CELL_CLP"
					ATM_CELL_CLP=$($GET_RC)
					;;
					
			get_atm_cell_payload)
					get_string "Please specify ATM Cell Payload" "$ATM_CELL_PAYLOAD"
					ATM_CELL_PAYLOAD=$($GET_RC)
			                ;;  

			ADSL_Vci)
					get_string "Please specify ATM VCI" "$ADSL_Vci"
					ADSL_Vci=$($GET_RC)
					;;
			ADSL_Vpi)
					get_string "Please specify ATM VPI" "$ADSL_Vpi"
					ADSL_Vpi=$($GET_RC)
					;;

			adsl_watchdog)
					warning "ADSL_WATCHDOG"
					if [ $? -eq 0 ]; then
						ADSL_WATCHDOG=YES
					else
						ADSL_WATCHDOG=NO
					fi
					;;

			ADSL_RxBufferCount)
					get_string "Please specify Rx Buffer Count" "$ADSL_RxBufferCount"
					ADSL_RxBufferCount=$($GET_RC)
					;;
					
			ADSL_TxBufferCount)
					get_string "Please specify Rx Buffer Count" "$ADSL_TxBufferCount"
					ADSL_TxBufferCount=$($GET_RC)
					;;
					
			ADSL_TxPowerAtten)
					get_integer "Please specify ADSL Tx Power Atten (from 0dB to 12 dB)" "$ADSL_TxPowerAtten" "0" "12" "device_setup_help ADSL_TxPowerAtten"
					ADSL_TxPowerAtten=$($GET_RC)
					;;

			ADSL_MaxBitsPerBin)
					get_integer "Please specify ADSL Max Bits Per Bin on RX" "$ADSL_MaxBitsPerBin" "0" "15" "device_setup_help ADSL_MaxBitsPerBin"
					ADSL_MaxBitsPerBin=$($GET_RC)
					;;
			ADSL_TxStartBin)
					get_string "Please specify ADSL Tx Start Bin" "$ADSL_TxStartBin"
					ADSL_TxStartBin=$($GET_RC)
					;;
			ADSL_TxEndBin)
					get_string "Please specify ADSL Tx End Bin" "$ADSL_TxEndBin"
					ADSL_TxEndBin=$($GET_RC)
					;;
			ADSL_RxStartBin)
					get_string "Please specify ADSL Rx Start Bin" "$ADSL_RxStartBin"
					ADSL_RxStartBin=$($GET_RC)
					;;
			ADSL_RxEndBin)
					get_string "Please specify ADSL Rx End Bin" "$ADSL_RxEndBin"
					ADSL_RxEndBin=$($GET_RC)
					;;

			ADSL_MaxDownRate)
					get_integer "Please specify ADSL Max Down Rate" "$ADSL_MaxDownRate" "32" "8192" "device_setup_help ADSL_MaxDownRate"
					ADSL_MaxDownRate=$($GET_RC)
					;;

			BSCSTRM_ADAPTER_FR)
	        			get_string "Please specify Bisync Adapter Frequency" "$BSCSTRM_ADAPTER_FR"
					BSCSTRM_ADAPTER_FR=$($GET_RC)
					;;
			BSCSTRM_MTU)
					get_string "Please specify Bisync MTU" "$BSCSTRM_MTU"
					BSCSTRM_MTU=$($GET_RC)
					;;
			BSCSTRM_EBCDIC)
					get_string "Enable Bisync EBCDIC [1=yes:0=no]" "$BSCSTRM_EBCDIC"
					BSCSTRM_EBCDIC=$($GET_RC)
					;;
			BSCSTRM_RB_BLOCK_TYPE)
					get_integer "Specify Block Type [1-3]" "$BSCSTRM_RB_BLOCK_TYPE" "1" "3"
					BSCSTRM_RB_BLOCK_TYPE=$($GET_RC)
					;;
			BSCSTRM_NO_CONSEC_PAD_EOB)
					get_string "Bisync num of consec pad eob" "$BSCSTRM_NO_CONSEC_PAD_EOB"
					BSCSTRM_NO_CONSEC_PAD_EOB=$($GET_RC)
					;;
			BSCSTRM_NO_ADD_LEAD_TX_SYN_CHARS)
					get_string "Bisync num of add lead tx sync chars" "$BSCSTRM_NO_ADD_LEAD_TX_SYN_CHARS"
					BSCSTRM_NO_ADD_LEAD_TX_SYN_CHARS=$($GET_RC)
					;;
			BSCSTRM_NO_BITS_PER_CHAR)
					get_string "Bisync num of bits per char" "$BSCSTRM_NO_BITS_PER_CHAR"
					BSCSTRM_NO_BITS_PER_CHAR=$($GET_RC)
					;;
			BSCSTRM_PARITY)
					get_integer "Select Bisync Parity [0-no,1-odd,2-even]" "$BSCSTRM_PARITY" "0" "2"
					BSCSTRM_PARITY=$($GET_RC)
					;;
			BSCSTRM_MISC_CONFIG_OPTIONS)
  					get_string "Bisync misc config options" "$BSCSTRM_MISC_CONFIG_OPTIONS"
					BSCSTRM_MISC_CONFIG_OPTIONS=$($GET_RC)
					;;
			BSCSTRM_STATISTICS_OPTIONS)
  					get_string "Bisync statistics options" "$BSCSTRM_STATISTICS_OPTIONS"
					BSCSTRM_STATISTICS_OPTIONS=$($GET_RC)
					;;
			BSCSTRM_MODEM_CONFIG_OPTIONS)
  					get_string "Bisync modem config options" "$BSCSTRM_MODEM_CONFIG_OPTIONS"
					BSCSTRM_MODEM_CONFIG_OPTIONS=$($GET_RC)
					;;
				again)
					;;

				*)	$choice     #Choice is a command 
					;;
			esac
			return 0
			;;
		2)	choice=${choice%\"*\"}
			choice=${choice// /}
			device_setup_help $choice
			return 0	
			;;
		*)  	return 1	
			;;
	esac
}

#============================================================
# TE1
# get_device_te_setup
#
# 	General T1/E1 Hardware Setup
#	
#============================================================
function gen_device_te_setup () {

	local menu_opt_te1
	local menu_size

	if [ $MEDIA = T1 ]; then
	menu_opt_te1=" 'get_media_type' 'MEDIA ---------------> $MEDIA' \
		       'get_lcode_type' 'Line decoding -------> $LCODE' \
		       'get_frame_type' 'Framing -------------> $FRAME' \
		       'get_te_clock'   'TE clock mode -------> $TE_CLOCK' \
		       'get_active_chan' 'Act. channels -------> $ACTIVE_CH' \
		       'get_lbo_type'    'LBO -----------------> $LBO' \
		       'get_highimpedance_type'   'High-Impedance Mode -> $HIGHIMPEDANCE' \\"
	menu_size=7
	else
	menu_opt_te1=" 'get_media_type' 'MEDIA ----------> $MEDIA' \
		       'get_lcode_type' 'Line decoding --> $LCODE' \
		       'get_frame_type' 'Framing --------> $FRAME' \
		       'get_te_clock'   'TE clock mode --> $TE_CLOCK' \
		       'get_active_chan' 'Act. channels --> $ACTIVE_CH' \\"
	menu_size=5
	fi

	echo "$menu_opt_te1" >> $DEVOPT
	echo " 2> $MENU_TMP" >> $DEVOPT


	#The menues are completed, now print the menu to the user and 
        #wait for result

	menu_options=`cat $DEVOPT` 
	rm -f $DEVOPT

	menu_instr=""

	menu_name "WANPIPE$DEVICE_NUM T1/E1 HARDWARE CONFIGURATION" "$menu_options" "$menu_size" "$menu_instr" "$BACK" 20 50 
	rc=$?
	
	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP


	#We got the result, take appropriate action 
	case $rc in
		0)	$choice #Choice is a command
			return 0
			;;
		2)	choice=${choice%\"*\"}
			choice=${choice// /}
			device_setup_help $choice
			return 0	
			;;
		*)  	return 1	
			;;
	esac
}


#============================================================
# TE3
# get_device_te3_setup
#
# 	General T3/E3 Hardware Setup
#	
#============================================================
function gen_device_te3_setup () {

	local menu_opt_te1
	local menu_size

	menu_opt_te1=" 'get_media_te3_type' 'Media ---------------> $MEDIA' \
		       'get_lcode_te3_type' 'Line decoding -------> $LCODE' \
		       'get_frame_te3_type' 'Framing -------------> $FRAME' \
		       'get_TE3_FCS'        'FCS (CRC) -----------> $TE3_FCS' \
		       'get_TE3_RXEQ'       'RXEQ ----------------> $TE3_RXEQ' \
		       'get_TE3_TAOS'       'TAOS ----------------> $TE3_TAOS' \
		       'get_TE3_LBMODE'     'LBMODE --------------> $TE3_LBMODE' \
		       'get_TE3_TXLBO'      'TXLBO ---------------> $TE3_TXLBO' \
		       'again'              ' ' \
		       'get_TE3_FRACTIONAL' 'FRACTIONAL ----------> $TE3_FRACTIONAL' \
		       'get_TE3_RDEVICE'    'Remote Frac Device --> $TE3_RDEVICE' \\"

	if [ $TE3_FRACTIONAL = YES ]; then
	menu_size=11
	else
	menu_size=10
	fi

	echo "$menu_opt_te1" >> $DEVOPT
	echo " 2> $MENU_TMP" >> $DEVOPT


	#The menues are completed, now print the menu to the user and 
        #wait for result

	menu_options=`cat $DEVOPT` 
	rm -f $DEVOPT

	menu_instr=""

	menu_name "WANPIPE$DEVICE_NUM T3/E3 HARDWARE CONFIGURATION" "$menu_options" "$menu_size" "$menu_instr" "$BACK" 20 50 
	rc=$?
	
	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP


	#We got the result, take appropriate action 
	case $rc in
		0)	$choice #Choice is a command
			return 0
			;;
		2)	choice=${choice%\"*\"}
			choice=${choice// /}
			device_setup_help $choice
			return 0	
			;;
		*)  	return 1	
			;;
	esac
}




function ip_setup_menu () {
	
	local ret
	local rc
	local menu_options
	local num="$1"

	if [ -z "${NMSK_IP[$num]}" ]; then

		#The BRIDGE NODE netmask cannot be 255.255.255.255
		#because the routing table will not be updated
		#on startup.  Reason: no PointoPoint address.
		if [ ${OP_MODE[$num]} = "BRIDGE_NODE" ]; then
			NMSK_IP[$num]="255.255.255.0"
		else
			NMSK_IP[$num]="255.255.255.255"
		fi
	fi
	if [ -z "${DEFAULT_IP[$num]}" ]; then
		DEFAULT_IP[$num]=NO
		NETWRK_IP[$num]=" "
	fi

	if [ $PROTOCOL = WAN_ADSL ]; then
	 	if [ $ADSL_EncapMode = PPP_LLC_OA ] || [ $ADSL_EncapMode = PPP_VC_OA ]; then
			L_IP[$num]=${L_IP[$num]:-0.0.0.0};
			GATE_IP[$num]=${GATE_IP[$num]:-0.0.0.0};
			R_IP[$num]=${R_IP[$num]:-0.0.0.0};
			DYNAMIC_INTR[$num]=${DYNAMIC_INTR[$num]:-YES};
		fi
	else
		DYNAMIC_INTR[$num]=${DYNAMIC_INTR[$num]:-NO}
	fi

	if [ ${OP_MODE[$num]} = BRIDGE_NODE ]; then

		if [ ${DEFAULT_IP[$num]} = NO ]; then
	menu_options="'get_local_ip' 'Local IP Addr. ------------> ${L_IP[$num]}' \
		      'get_netmask'  'Net Mask IP Addr. ---------> ${NMSK_IP[$num]}' \
		      'get_default'  'Default Gateway -----------> ${DEFAULT_IP[$num]}' 2> $MENU_TMP"
		else
	menu_options="'get_local_ip' 'Local IP Addr. ------------> ${L_IP[$num]}' \
		      'get_netmask'  'Net Mask IP Addr. ---------> ${NMSK_IP[$num]}' \
		      'get_default'  'Default Gateway -----------> ${DEFAULT_IP[$num]}'\
		      'get_gate_ip'  '        Gateway IP --------> ${GATE_IP[$num]}' 2> $MENU_TMP"
		fi

	elif [ $PROTOCOL = "WAN_ADSL" ] && [ "$ADSL_EncapMode" = "ETH_LLC_OA" -o "$ADSL_EncapMode" = "ETH_VC_OA" ]; then

		if [ ${NMSK_IP[$num]} = "255.255.255.255" ]; then
			NMSK_IP[$num]="255.255.255.0"
		fi

		if [ ${DEFAULT_IP[$num]} = NO ]; then
	menu_options="'get_local_ip' 'Local IP Addr. ------------> ${L_IP[$num]}' \
		      'get_netmask'  'Net Mask IP Addr. ---------> ${NMSK_IP[$num]}' \
		      'get_default'  'Default Gateway -----------> ${DEFAULT_IP[$num]}' 2> $MENU_TMP"
		else
	menu_options="'get_local_ip' 'Local IP Addr. ------------> ${L_IP[$num]}' \
		      'get_netmask'  'Net Mask IP Addr. ---------> ${NMSK_IP[$num]}' \
		      'get_default'  'Default Gateway -----------> ${DEFAULT_IP[$num]}'\
		      'get_gate_ip'  '        Gateway IP --------> ${GATE_IP[$num]}' 2> $MENU_TMP"
		fi

	elif [ $PROTOCOL = "WAN_ATM" ] && [ "${atm_encap_mode[$num]}" = "ETH_LLC_OA" -o "${atm_encap_mode[$num]}" = "ETH_VC_OA" ]; then

		if [ ${NMSK_IP[$num]} = "255.255.255.255" ]; then
			NMSK_IP[$num]="255.255.255.0"
		fi

		if [ ${DEFAULT_IP[$num]} = NO ]; then
	menu_options="'get_local_ip' 'Local IP Addr. ------------> ${L_IP[$num]}' \
		      'get_netmask'  'Net Mask IP Addr. ---------> ${NMSK_IP[$num]}' \
		      'get_default'  'Default Gateway -----------> ${DEFAULT_IP[$num]}' 2> $MENU_TMP"
		else
	menu_options="'get_local_ip' 'Local IP Addr. ------------> ${L_IP[$num]}' \
		      'get_netmask'  'Net Mask IP Addr. ---------> ${NMSK_IP[$num]}' \
		      'get_default'  'Default Gateway -----------> ${DEFAULT_IP[$num]}'\
		      'get_gate_ip'  '        Gateway IP --------> ${GATE_IP[$num]}' 2> $MENU_TMP"
		fi

	else
		if [ ${DEFAULT_IP[$num]} = NO ]; then
	menu_options="'get_local_ip' 'Local IP Addr. ------------> ${L_IP[$num]}' \
		      'get_rmt_ip'   'Point-to-Point IP Addr.----> ${R_IP[$num]}' \
		      'get_netmask'  'Net Mask IP Addr. ---------> ${NMSK_IP[$num]}' \
		      'get_dyn_intr' 'Dynamic Interface Config --> ${DYNAMIC_INTR[$num]}'\
		      'get_default'  'Default Gateway -----------> ${DEFAULT_IP[$num]}' 2> $MENU_TMP"
		else
	menu_options="'get_local_ip' 'Local IP Addr. ------------> ${L_IP[$num]}' \
		      'get_rmt_ip'   'Point-to-Point IP Addr.----> ${R_IP[$num]}' \
		      'get_netmask'  'Net Mask IP Addr. ---------> ${NMSK_IP[$num]}' \
		      'get_dyn_intr' 'Dynamic Interface Config --> ${DYNAMIC_INTR[$num]}'\
		      'get_default'  'Default Gateway -----------> ${DEFAULT_IP[$num]}' \
		      'get_gate_ip'  '        Gateway IP --------> ${GATE_IP[$num]}' 2> $MENU_TMP"
		fi

	fi
	
	menu_instr="	Interface specify IP addresses supplied by your ISP \
				"
  	menu_name "${IF_NAME[$num]}: IP CONFIGURATION" "$menu_options" 6 "$menu_instr" "$BACK" 20 50 
	rc=$?
	
	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in

		0) 
			case $choice in
			get_local_ip)
				get_string "Please specify the Local IP Address" "${L_IP[$num]}" "IP_ADDR" 
				L_IP[$num]=$($GET_RC)
				;;
			get_rmt_ip)
				get_string "Please specify the Point-to-Point IP Address" "${R_IP[$num]}" "IP_ADDR"
				R_IP[$num]=$($GET_RC)
				;;
			get_netmask)
				get_string "Please specify the Net Mask Addr" "${NMSK_IP[$num]}" "IP_ADDR"
				NMSK_IP[$num]=$($GET_RC)
				;;
			get_bcast)
				get_string "Please specify the Broadcast IP Address" "${BCAST_IP[$num]}" "IP_ADDR"
				BCAST_IP[$num]=$($GET_RC)
				;;
			#This is not begin used should be removed
			get_network)
				get_string "Please specify the Network IP Address" "${NETWRK_IP[$num]}" "IP_ADDR"
				NETWRK_IP[$num]=$($GET_RC)
				;;
			#This is not being used should be removed
			get_gateway)
				get_string "Please specify the Gateway IP Address" "${GATE_IP[$num]}" "IP_ADDR"
				GATE_IP[$num]=$($GET_RC)
				;;
			get_default)
				warning get_default
				if [ $? -eq 0 ]; then
					DEFAULT_IP[$num]=YES
					GATE_IP[$num]=${R_IP[$num]}
				else
					DEFAULT_IP[$num]=NO
					GATE_IP[$num]=" "
				fi
				;;
			get_gate_ip)
				get_string "Please specify the Gateway IP Address" "${G_IP[$num]}" "GATE_ADDR" 
				GATE_IP[$num]=$($GET_RC)
				;;
			get_dyn_intr)
				warning get_dyn_intr
				if [ $? -eq 0 ]; then
					DYNAMIC_INTR[$num]=YES
				else
					DYNAMIC_INTR[$num]=NO
				fi
				;;

			esac
			return 0
			;;
		2)	choice=${choice%\"*\"}
			choice=${choice// /}
			ip_setup_help $choice	
			return 0 
			;;
		*) return 1
			;;

	esac

}

function get_operation_mode () {

	local ret
	local menu_options
	local num="$1"
	local annexg="$2"

	if [ $PROTOCOL = WAN_PPP ]; then
		menu_options="'WANPIPE' 'WANPIPE' \
			      'BRIDGE' 'BRIDGE' \
			      'BRIDGE_NODE' 'BRIDGED NODE' 2> $MENU_TMP"
	elif [ $PROTOCOL = WAN_ATM ]; then
		menu_options="'WANPIPE' 'WANPIPE' \
			      'BRIDGE' 'BRIDGE' 2> $MENU_TMP"
	
	elif [ $PROTOCOL = WAN_POS ]; then
		menu_options="'API' 'API' 2> $MENU_TMP"

	elif [ $PROTOCOL = WAN_MFR ] || [ $PROTOCOL = WAN_MULTPROT ]; then

		if [ "${OP_MODE[$num]}" = ANNEXG ]; then
			menu_options="'WANPIPE' 'WANPIPE' \
				      'DSP'     'DSP' \
				      'API' 'API' 2> $MENU_TMP"
		else
		
			menu_options="'WANPIPE' 'WANPIPE' \
				      'ANNEXG' 'ANNEXG'  2> $MENU_TMP"
		fi
		
	elif [ $PROTOCOL = WAN_FR ] || [ $PROTOCOL = WAN_CHDLC ]; then
		menu_options="'WANPIPE' 'WANPIPE' \
			      'API' 'API' \
			      'BRIDGE' 'BRIDGE' \
			      'BRIDGE_NODE' 'BRIDGED NODE' 2> $MENU_TMP"
			      
	elif [ $PROTOCOL = WAN_BITSTRM ]; then
		menu_options="'API' 'API' \
			      'SWITCH' 'SWITCH'\
			      'WANPIPE' 'WANPIPE'\
			      'TDM_VOICE' 'TDM Voice' 2> $MENU_TMP"

	elif [ $PROTOCOL = WAN_AFT ]; then
		menu_options="'API' 'API' \
			      'WANPIPE' 'WANPIPE'\
			      'TDM_VOICE' 'TDM Voice' 2> $MENU_TMP"

	elif [ $PROTOCOL = WAN_AFT_TE3 ]; then
		menu_options="'API' 'API' \
			      'WANPIPE' 'WANPIPE' 2> $MENU_TMP"

	elif [ $PROTOCOL = WAN_ADSL ]; then

		if [ "$ADSL_EncapMode" != ETH_LLC_OA -a "$ADSL_EncapMode" != ETH_VC_OA ]; then

			menu_options="'WANPIPE' 'WANPIPE' 2> $MENU_TMP"
		else
			menu_options="'PPPoE' 'PPPoE' \
			      	      'WANPIPE' 'WANPIPE' 2> $MENU_TMP"
		fi
	else
		menu_options="'WANPIPE' 'WANPIPE' \
			      'API' 'API' 2> $MENU_TMP"
	fi

	AUTO_INTR_CFG=${AUTO_INTR_CFG:-NO}
	
	if [ $AUTO_INTR_CFG = YES ]; then
		if [ $PROTOCOL = WAN_FR ]; then
		menu_options="'API' 'API' \
			      'BRIDGE' 'BRIDGE' 2> $MENU_TMP"
		else	
		menu_options="'API' 'API' 2> $MENU_TMP"
		fi

		if [ $PROTOCOL = WAN_MFR ]; then
		menu_options="'ANNEXG' 'ANNEXG' 2> $MENU_TMP"
		fi

	fi

	menu_instr="Please select Wanpipe Operation Mode \
					"
	menu_name "OPERATION MODE CONFIGURATION" "$menu_options" 4 "$menu_instr" "$BACK"
	retval=$?

	input=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $retval in
	  	0)	if [ "$annexg" != "ANNEXG" ]; then
				OP_MODE[$num]=$input
			else
				echo "Writting the opmode for Annexg $input"	
				ANNEXG_OPMODE=$input
			fi
			
			return 0 
			;;
	  	1) 	return 1
			;;
		2)	interface_menu_help "get_operation_mode"
			get_operation_mode $1
			;;
	  	255) 	return 1
			;;
	esac       
}


function device_init () {

	#Common
	IOPORT=${IOPORT:-0x360}
	IRQ=${IRQ:-7}
	S514CPU=${S514CPU:-A}
	PCISLOT=${PCISLOT:-0}
	PCIBUS=${PCIBUS:-0}
	INTERFACE=${INTERFACE:-V35}
       	CLOCKING=${CLOCKING:-External}
	FIRMWARE=${FIRMWARE:-$WAN_FIRMWARE_DIR}
	MEMORY=${MEMORY:-Auto}
     	BAUDRATE=${BAUDRATE:-1540000}
	FR_ISSUE_FS=${FR_ISSUE_FS:-YES}
	
	CONNECTION=${CONNECTION:-Permanent}
	LINECODING=${LINECODING:-NRZ}
	LINEIDLE=${LINEIDLE:-Flag}


	MTU=${MTU:-1500}
	UDPPORT=${UDPPORT:-9000}

	#PPP Specific
	IPMODE=${IPMODE:-STATIC}

	#Chdlc Specific
	COMMPORT=${COMMPORT:-PRI}
	REC_ONLY=${REC_ONLY:-NO}
	TTY_MINOR=${TTY_MINOR:-0}
	TTY_MODE=${TTY_MODE:-Sync}

	#Frame Relay Specific
	if [ $PROTOCOL = WAN_FR ] || [ $PROTOCOL = WAN_MFR ]; then
		STATION=${STATION:-CPE}
	else
		STATION=${STATION:-DTE}
	fi

	SIGNAL=${SIGNAL:-ANSI}
	T391=${T391:-10}
	T392=${T392:-16}
	N391=${N391:-6}
	N392=${N392:-3}
	N393=${N393:-4}
	TTL=${TTL:-255} 

	#X25 Specific
	if [ $PROTOCOL = WAN_X25 -o $PROTOCOL = WAN_ADCCP ]; then
		LOW_PVC=${LOW_PVC:-0}
		HIGH_PVC=${HIGH_PVC:-0}
		LOW_SVC=${LOW_SVC:-0}
		HIGH_SVC=${HIGH_SVC:-0}
		HDLC_WIN=${HDLC_WIN:-7}
		PACKET_WIN=${PACKET_WIN:-7}
		DEF_PKT_SIZE=${DEF_PKT_SIZE:-1024}
		MAX_PKT_SIZE=${MAX_PKT_SIZE:-1024}
		CCITT=${CCITT:-1988}
		LAPB_HDLC_ONLY=${LAPB_HDLC_ONLY:-NO}
		CALL_LOGGING=${CALL_LOGGING:-YES}
		OOB_ON_MODEM=${OOB_ON_MODEM:-NO}
		HDLC_STATION_ADDR=${HDLC_STATION_ADDR:-1}
		X25_T1=${X25_T1:-1}
		X25_T2=${X25_T2:-0}
		X25_T4=${X25_T4:-1}
		X25_N2=${X25_N2:-10}
		X25_T10_T20=${X25_T10_T20:-30}
		X25_T11_T21=${X25_T11_T21:-30}
		X25_T12_T22=${X25_T12_T22:-30}
		X25_T13_T23=${X25_T13_T23:-30}
		X25_T16_T26=${X25_T16_T26:-30}
		X25_T16_T26=${X25_T16_T26:-30}
		X25_T28=${X25_T28:-30}
		X25_R10_R20=${X25_R10_R20:-5}
		X25_R12_R22=${X25_R12_R22:-5}
		X25_R13_R23=${X25_R13_R23:-5}
	fi
}

# TE1 Initialize T1/E1 hardware parameters
function device_te_init() {

	DEVICE_TE1_TYPE=${DEVICE_TE1_TYPE:-NO}
	MEDIA=${MEDIA:-T1}
	LCODE=${LCODE:-B8ZS}
	FRAME=${FRAME:-ESF}
	ACTIVE_CH=${ACTIVE_CH:-ALL}
	LBO=${LBO:-0DB}
	TE_CLOCK=${TE_CLOCK:-NORMAL}
	HIGHIMPEDANCE=${HIGHIMPEDANCE:-NO}
}

function device_te3_init() {

	DEVICE_TE3_TYPE=${DEVICE_TE3_TYPE:-NO}
	MEDIA=${MEDIA:-DS3}	
	LCODE=${LCODE:-B3ZS}	
	FRAME=${FRAME:-C-BIT}

	TE3_FCS=${TE3_FCS:-16}		
	TE3_RXEQ=${TE3_RXEQ:-NO}	
	TE3_TAOS=${TE3_TAOS:-NO}	
	TE3_LBMODE=${TE3_LBMODE:-NO}	
	TE3_TXLBO=${TE3_TXLBO:-NO}	

	TE3_FRACTIONAL=${TE3_FRACTIONAL:-NO}
	TE3_RDEVICE=${TE3_RDEVICE:-KENTROX}
}


function get_pos_prot () {

	local menu_options
	local rc
	local choice
	local num="$1"

	menu_options="'pos4680.sfm' 'IBM 4680 (default)' \
		      'pos2126.sfm' 'NCR 2126' \
		      'pos2127.sfm' 'NCR 2127' \
		      'pos1255.sfm' 'NCR 1255' \
		      'pos7000.sfm' 'NCR 7000' \
		      'posicl.sfm'  'ICL' 2> $MENU_TMP"

	menu_instr="Select POS Protocol \
						"

	menu_name "${IF_NAME[$num]}: Pos Protocol" "$menu_options" 6 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	firm_name=$choice     #Choice is a command 
			if [ $choice = pos4680.sfm ]; then
				pos_protocol="IBM4680"
			elif [ $choice = pos2127.sfm ]; then
				pos_protocol="NCR2127"	
			elif [ $choice = pos2126.sfm ]; then
				pos_protocol="NCR2126"
			elif [ $choice = pos1255.sfm ]; then
				pos_protocol="NCR1255"
			elif [ $choice = pos7000.sfm ]; then
				pos_protocol="NCR7000"
			else
				pos_protocol="ICL"	
			fi
			return 0
			;;

		2)	device_setup_help "get_pos_prot"
			get_pos_prot
			;;

		*) 	return 1
			;;
	esac


}

function get_multicast () {

	local menu_options
	local rc
	local choice
	local num="$1"

	menu_options="'NO'  'Disable Multicast (default)' \
		      'YES' 'Enable Multicast' 2> $MENU_TMP"


	menu_instr="Enable or Disable Multicast Option \
						"

	menu_name "${IF_NAME[$num]}: MULTICAST CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	MULTICAST[$num]=$choice     #Choice is a command 
			return 0
			;;
		2)	interface_setup_help "get_multicast"
			get_multicast "$1"
			;;
		*) 	return 1
			;;
	esac

}




function get_s514_cpu () {
 
	local menu_options
	local rc
	local choice

	menu_options="'A' 'CPU A' \
		      'B' 'CPU B' 2> $MENU_TMP"


	menu_instr="Please select S514 PCI, CPU Number \
						"

	menu_name "S514 CPU CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	S514CPU=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "get_s514_cpu"
			get_s514_cpu
			;;
		*) 	return 1
			;;
	esac
}


function get_tty_mode () {
 
	local menu_options
	local rc
	local choice

	menu_options="'Sync'  'Sync Leased Line' \
		      'Async' 'Async Serial Line' 2> $MENU_TMP"


	menu_instr="Please select the Wanpipe TTY operation mode \
						"

	menu_name "WANPIPE TTY CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	TTY_MODE=$choice     #Choice is a command 
			if [ $TTY_MODE = Async ]; then
				INTERFACE="RS232";
			fi
			return 0
			;;
		2)	device_setup_help "get_tty_mode"
			get_tty_mode
			;;
		*) 	return 1
			;;
	esac
}


function get_s508_io () {
 
	local menu_options
	local rc
	local choice

	menu_options="'0x360' '0x360-0x363 Io Range (default)' \
		      '0x390' '0x390-0x393 Io Range' \
		      '0x380' '0x380-0x383 Io Range' \
		      '0x350' '0x350-0x352 Io Range' \
		      '0x300' '0x300-0x393 Io Range' \
		      '0x280' '0x280-0x283 Io Range' \
		      '0x270' '0x270-0x273 Io Range' \
		      '0x250' '0x250-0x253 Io Range' 2> $MENU_TMP"

	menu_instr="Please select S508, IO Range. Refer to hardware manual \
								"
	menu_name "S508 IO PORT CONFIGURATION" "$menu_options" 7 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	IOPORT=$choice     #Choice is a command 
			return 0
			;;
		2)  	device_setup_help "get_s508_io"
			get_s508_io
			;;
		*) 	return 1
			;;
	esac
}

	
function get_interface () {
 
	local menu_options
	local rc
	local choice


	if [ -z "$COMMPORT" ]; then
		COMMPORT=PRI
	fi

	if [ $DEVICE_TYPE = S508 -a  $COMMPORT = SEC ];
	then
		menu_options="'RS232' 'RS232 Interface' 2> $MENU_TMP"
	else
		menu_options="'V35' 'V35 Interface' \
		      	      'RS232' 'RS232 Interface' 2> $MENU_TMP"

	fi

	menu_instr="Please select $DEVICE_TYPE, Interface Type \
						"

	menu_name "WAN INTERFACE CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	INTERFACE=$choice     #Choice is a command 
			return 0
			;;
		2) 	device_setup_help "get_interface"
			get_interface
			;;
		*) 	return 1
			;;
	esac
}


function get_ignore_fe () {
 
	local menu_options
	local rc
	local choice


	menu_instr="Please Enable or Disable Front End Status \
						"

	menu_options="'NO'  'FRONT END should/will affect link state' \
	      	      'YES' 'Ignore FRONT END: will NOT affect link state' 2> $MENU_TMP"

	menu_name "FRONT END STATUS CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	IGNORE_FRONT_END=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "get_ignore_fe"
			get_ignore_fe
			;;
		*) 	return 1
			;;
	esac
}



function get_clocking () {
 
	local menu_options
	local rc
	local choice


	if [ $DEVICE_TE1_TYPE = "YES" ] || [ $DEVICE_56K_TYPE = "YES" ]; then
		menu_options="'External' 'External Clocking' 2> $MENU_TMP"
	else
		menu_options="'External' 'External Clocking' \
	      	      'Internal' 'Internal Clocking' 2> $MENU_TMP"
	fi
	menu_instr="Please specify a Clocking Source \
						"

	menu_name "WANPIPE$DEVICE_NUM CLOCKING CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	CLOCKING=$choice     #Choice is a command 
			return 0
			;;
		2)	device_setup_help "get_clocking"
			get_clocking
			;;
		*) 	return 1
			;;
	esac
}

function get_memory () {
 
	local menu_options
	local rc
	local choice


	menu_options="'Auto' 	   'Auto Detect Memory (Default)' \
                      'get_string' 'Custom Setup' 2> $MENU_TMP"


	menu_instr="Please specify the wanpipe$DEVICE_NUM physical Memory Address. 					\
				"

	menu_name "WANPIPE MEM-ADDR CONFIGURATION" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	if [ $choice = get_string ]; then
				$choice "Please specify Memory Address in Hex" "0xEE000" 
				MEMORY=$($GET_RC)
			else
				MEMORY=$choice     #Choice is a command 
			fi
			return 0
			;;
		2)	device_setup_help "get_memory"
			get_memory
			;;
		*) 	return 1
			;;
	esac
}

function get_device_type () {

	local choice
	local rc

	local probe=${1:-NO}

	

	# TE1 Add new device type
	if [ $probe = "PROBE" ]; then
		menu_options="'S514'     'S514 V35   PCI Card' \
			      'S514'     'S514 FT1   PCI Card' \	
			      'S514-TE1' 'S514 T1/E1 PCI Card' \
			      'S514-56k' 'S514 56K PCI Card' 2> $MENU_TMP"
	else

		if [ $PROTOCOL = "WAN_ADSL" ]; then 
		menu_options="'S518' 'Wanpipe ADSL Card' 2> $MENU_TMP"
		elif [ $PROTOCOL = "WAN_AFT" ]; then
		menu_options="'AFT-TE1' 'Wanpipe AFT T1/E1 Card' 2> $MENU_TMP"
		elif [ $PROTOCOL = "WAN_AFT_TE3" ]; then
		menu_options="'AFT-TE3' 'Wanpipe AFT T3/E3 Card' 2> $MENU_TMP"
		else
		menu_options="'S514' 	 'S514 V35   PCI Card' \
			      'S514' 	 'S514 FT1   PCI Card' \
			      'S514-TE1' 'S514 T1/E1 PCI Card' \
			      'S514-56k' 'S514 56K   PCI Card' \
			      'S508' 	 'S508 ISA Card' 2> $MENU_TMP"
		fi
	fi

	menu_instr="Please specify Sangoma Card Type. Refer to User Manual \
									"
	menu_name "SANGOMA CARD TYPE" "$menu_options" 4 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	DEVICE_TYPE=$choice     #Choice is a command 
			# TE1 For T1/E1 board, set DEVICE_TYPE to S514 and
			#     set DEVICE_TE1_TYPE to YES
			DEVICE_TE1_TYPE=NO
			DEVICE_56K_TYPE=NO
			if [ $DEVICE_TYPE = S514-TE1 ]; then
				DEVICE_TYPE=S514
				DEVICE_TE1_TYPE=YES
			elif [ $DEVICE_TYPE = S514-56k ]; then
				DEVICE_TYPE=S514
				DEVICE_56K_TYPE=YES
			elif [ $DEVICE_TYPE = AFT-TE1 ]; then
				DEVICE_TYPE=AFT
				DEVICE_TE1_TYPE=YES
			elif [ $DEVICE_TYPE = AFT-TE3 ]; then
				DEVICE_TYPE=AFT
				DEVICE_TE3_TYPE=YES
				DEVICE_TE1_TYPE=NO
			fi
			;;
		2)	device_setup_help "get_device_type" 
			get_device_type
			;;
		*) 	return 1
			;;
	esac

	init_bstrm_opt
}

function g_ADSL_EncapMode() {

	local choice
	local rc
	local num=$1

	menu_options="'ETH_LLC_OA'   'Bridged Eth LLC over ATM (PPPoE)' \
	              'ETH_VC_OA'    'Bridged Eth VC over ATM' \
		      'IP_LLC_OA'    'Classical IP LLC over ATM' \
		      'IP_VC_OA'     'Routed IP VC over ATM' \
		      'PPP_LLC_OA'   'PPP (LLC) over ATM' \
		      'PPP_VC_OA'    'PPP (VC) over ATM (PPPoA)' 2> $MENU_TMP"

	menu_instr="Please specify ATM Encapsulation Mode. \
									"
	menu_name "ATM Encapsulation MODE" "$menu_options" 6 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	if [ -z $num ]; then
				ADSL_EncapMode=$choice     #Choice is a command 
			else
				atm_encap_mode[$num]=$choice
			fi

			#if [ $ADSL_EncapMode = IP_LLC_OA ] || [ $ADSL_EncapMode = IP_VC_OA ]; then
			#	MTU=9188
			#else
			#	MTU=1500
			#fi
			;;
		2)	
			if [ -z $num ]; then
			device_setup_help "g_ADSL_EncapMode" 
			else
			device_setup_help "g_ATM_EncapMode"
			fi
			g_ADSL_EncapMode $num
			;;
		*) 	return 1
			;;
	esac
}




function get_atm_sync_mode (){

	local choice
	local rc

	menu_options="'AUTO'   'Automatic (default)' \
		      'MANUAL' 'Manual' 2> $MENU_TMP"

	menu_instr="Please specify ATM sync mode \
									"
	menu_name "ATM SYNC MODE" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	ATM_SYNC_MODE=$choice     #Choice is a command 
			;;
		2)	device_setup_help "get_atm_sync_mode" 
			get_atm_sync_mode
			;;
		*) 	return 1
			;;
	esac



}

function get_active_chan () {

	local choice
	local rc

	get_string "Please specify the active channels" "$ACTIVE_CH" "get_act_chan"
	rc=$?

	case $rc in
		0) 	ACTIVE_CH=$($GET_RC)     #Choice is a command 
			;;
		1)	device_setup_help "get_active_chan" 
			get_active_chan
			;;
		*) 	return 1
			;;
	esac
}

function bstrm_get_active_chan () {

	local choice
	local rc
	local num=$1
	
	get_string "Please specify which T1/E1 channels to bind in to" "${BSTRM_ACTIVE_CH[$num]}" "bstrm_get_act_chan"
	rc=$?

	case $rc in
		0) 	BSTRM_ACTIVE_CH[$num]=$($GET_RC)
			;;
		1)	interface_menu_help "bstrm_get_active_chan"  
			bstrm_get_active_chan $num
			;;
		*) 	return 1
			;;
	esac
}

function bstrm_get_switch_dev () {

	local choice
	local rc
	local num=$1
	
	get_string "Please specify a bitstreaming device to bind into" "${BSTRM_SWITCH_DEV[$num]}" "bstrm_get_switch_dev"
	rc=$?

	case $rc in
		0) 	BSTRM_SWITCH_DEV[$num]=$($GET_RC)
			;;
		1)	interface_menu_help "bstrm_get_switch_dev" 
			bstrm_get_active_chan $num
			;;
		*) 	return 1
			;;
	esac
}




function probe_wanpipe_hw ()
{
	local devices

	${WAN_BIN_DIR}/wanrouter hwprobe > /dev/null
	if [ $? -ne 0 ]; then
		error "NO_HWPROBE"
		return
	fi

	if [ $OS_SYSTEM = "Linux" ]; then
		if [ ! -e "/proc/net/wanrouter/hwprobe" ]; then
			error "NO_HWPROBE"	
			return;
		fi

		devices=`cat /proc/net/wanrouter/hwprobe | cut -d'.' -f2 | grep :`
	else
		
		devices=`wanconfig hwprobe | cut -d'.' -f2 | grep :`

	fi

	if [ -z "$devices" ]; then
		error "NO_DEV_IN_PROBE"	
		return;
	fi
	#touch $WAN_LOCK

	devices=${devices// /}	
	devices=${devices//:/ }

	echo "$devices" > $PROD_HOME/tmp.$$
	
	while read dev 
	do

		unset val
		val=`echo $dev | grep CardCnt`
		if [ ! -z "$val" ]; then
			continue
		fi

	
		if [ $PROTOCOL = "WAN_FR" ]  || \
		   [ $PROTOCOL = "WAN_PPP" ] || \
		   [ $PROTOCOL = "WAN_BSC" ] || \
		   [ $PROTOCOL = "WAN_X25" ] || \
		   [ $PROTOCOL = "WAN_ADCCP" ]; then
			
			unset val
			val=`echo $dev | grep SEC`
			if [ ! -z "$val" ]; then
				continue
			fi
		fi

		if [ $PROTOCOL = "WAN_ADSL" ]; then
			unset val
			val=`echo $dev | grep S518`
			if [ -z "$val" ]; then
				continue
			fi
		
		elif [ $PROTOCOL = "WAN_AFT" ]; then
			unset val
			val=`echo $dev | grep AFT`
			if [ -z "$val" ]; then
				continue
			fi

		elif [ $PROTOCOL = "WAN_AFT_TE3" ]; then
			unset val
			val=`echo $dev | grep A105`
			if [ -z "$val" ]; then
				continue
			fi

		else
			unset val
			val=`echo $dev | grep S518`
			if [ ! -z "$val" ]; then
				continue
			fi	

			if [ $PROTOCOL != "WAN_FR" ] && 
			   [ $PROTOCOL != "WAN_PPP" ] && 
			   [ $PROTOCOL != "WAN_CHDLC" ]; then
				unset val  

				val=`echo $dev | grep AFT-A14`
				if [ -z "$val" ]; then

					val=`echo $dev | grep AFT`
					if [ ! -z "$val" ]; then
						continue
					fi	

					val=`echo $dev | grep A105`
					if [ ! -z "$val" ]; then
						continue
					fi	

				fi

			fi
		fi

		echo "'$dev'   '$dev' \\" >> $PROD_HOME/dev_probe.$$ 
	done < $PROD_HOME/tmp.$$

	rm -f $PROD_HOME/tmp.$$

	if [ ! -f $PROD_HOME/dev_probe.$$ ]; then
		error "NO_DEV_IN_PROBE"
		return;
	fi

	echo " 2> $MENU_TMP" >> $PROD_HOME/dev_probe.$$

	menu_options=`cat $PROD_HOME/dev_probe.$$`
	rm -f $PROD_HOME/dev_probe.$$
	
	menu_instr="Please select a WANPIPE device \
											"
	menu_name "WANPIPE HARDWARE PROBE INFO" "$menu_options" 8 "$menu_instr" "$BACK" 20 55
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	DEVICE_TYPE=${choice%%-*}
		
			case $DEVICE_TYPE in

			S514*)
				TYPE=`echo $choice | cut -d' ' -f1 | cut -d'-' -f2`
				case $TYPE in
					4)
						DEVICE_TYPE=S514
						DEVICE_TE1_TYPE=YES
						DEVICE_56K_TYPE=NO
						DEVICE_TE3_TYPE=NO
						;;
					5)
						DEVICE_TYPE=S514
						DEVICE_TE1_TYPE=NO
						DEVICE_56K_TYPE=YES
						DEVICE_TE3_TYPE=NO
						;;
					7)	
						DEVICE_TYPE=S514
						DEVICE_TE1_TYPE=YES
						DEVICE_56K_TYPE=NO
						DEVICE_TE3_TYPE=NO
						;;

					8)
						DEVICE_TYPE=S514
						DEVICE_TE1_TYPE=YES
						DEVICE_56K_TYPE=NO
						DEVICE_TE3_TYPE=NO
						;;

					*)
						DEVICE_TYPE=S514
						DEVICE_TE1_TYPE=NO
						DEVICE_56K_TYPE=NO
						DEVICE_TE3_TYPE=NO
						;;
				esac
				
				S514CPU=`echo $choice | cut -d'=' -f5 | cut -d' ' -f1` 
				PCISLOT=`echo $choice | cut -d'=' -f2 | cut -d' ' -f1`
				PCIBUS=`echo $choice | cut -d'=' -f3 | cut -d' ' -f1`
				COMMPORT=`echo $choice | cut -d'=' -f6 | cut -d' ' -f1`
				AUTO_PCI_CFG="NO"
				;;
		
			S518*)
			
				DEVICE_TYPE=S518;
				DEVICE_TE1_TYPE=NO
				DEVICE_56K_TYPE=NO
				DEVICE_TE3_TYPE=NO
		
				S514CPU=`echo $choice | cut -d'=' -f5 | cut -d' ' -f1` 
				PCISLOT=`echo $choice | cut -d'=' -f2 | cut -d' ' -f1`
				PCIBUS=`echo $choice | cut -d'=' -f3 | cut -d' ' -f1`
				COMMPORT=`echo $choice | cut -d'=' -f6 | cut -d' ' -f1`
				AUTO_PCI_CFG="NO"
				;;
		
			AFT*)

				DEVICE_TYPE=AFT;
				DEVICE_56K_TYPE=NO
				DEVICE_TE3_TYPE=NO
		
				TYPE=`echo $choice | cut -d' ' -f1 | cut -d'-' -f2`
				if [ "$TYPE" = "A144" ] ||  [ "$TYPE" = "A142" ]; then
					DEVICE_TE1_TYPE=NO;
				else
					PROTOCOL=WAN_AFT
					DEVICE_TE1_TYPE=YES
				fi
		
				S514CPU=`echo $choice | cut -d'=' -f5 | cut -d' ' -f1` 
				PCISLOT=`echo $choice | cut -d'=' -f2 | cut -d' ' -f1`
				PCIBUS=`echo $choice | cut -d'=' -f3 | cut -d' ' -f1`
				COMMPORT=`echo $choice | cut -d'=' -f6 | cut -d' ' -f1`
				AUTO_PCI_CFG="NO"
				;;

			A105*)
				PROTOCOL=WAN_AFT_TE3
				DEVICE_TYPE=AFT;
				DEVICE_TE3_TYPE=YES
				DEVICE_56K_TYPE=NO
				DEVICE_TE1_TYPE=NO
		
				S514CPU=`echo $choice | cut -d'=' -f5 | cut -d' ' -f1` 
				PCISLOT=`echo $choice | cut -d'=' -f2 | cut -d' ' -f1`
				PCIBUS=`echo $choice | cut -d'=' -f3 | cut -d' ' -f1`
				COMMPORT=`echo $choice | cut -d'=' -f6 | cut -d' ' -f1`
				AUTO_PCI_CFG="NO"
				;;

				

			*)
				IOPORT=`echo $choice | cut -d'=' -f2 | cut -d' ' -f1`
				COMMPORT=`echo $choice | cut -d'=' -f3 | cut -d' ' -f1`
				DEVICE_56K_TYPE=NO
				DEVICE_TE1_TYPE=NO
				;;
			esac
			;;
		2)	device_setup_help "probe_wanpipe_hw" 
			probe_wanpipe_hw
			;;
		*)	return 1
			;;
	esac

	init_bstrm_opt	

	return 0;
}




function check_config () {

	local num

	if [ -z "$DEVICE_NUM" ]; then
		error "DEVICE_NUM" 
		return 1
	fi
	
	#Sync TTY PPP has no network interface
	if [ "$PROTOCOL" = WAN_TTYPPP ] || [ "$PROTOCOL" = WAN_MLINK_PPP ] || [ "$PROTOCOL" = WAN_EDU_KIT ]; then
		return 0
	fi

#NC: Used by PPPD not used any more
#	if [ "$OS_SYSTEM" = Linux ] && [ "$PROTOCOL" = WAN_ADSL ]; then
#		if [ "$ADSL_EncapMode" = PPP_LLC_OA ] || [ "$ADSL_EncapMode" = PPP_VC_OA ]; then
#			return 0
#		fi
#	fi	

	if [ -z "$NUM_OF_INTER" ]; then
		error "NUM_OF_INTER" 
		return 1
	fi

	if [ $NUM_OF_INTER -eq 0 ]; then
		error "NUM_OF_INTER" 
		return 1
	fi

	if [ -z "$DEVICE_TYPE" ]; then
		error "DEVICE_TYPE" 
		return 1
	fi

	num=0
	while [ $num -ne $NUM_OF_INTER ];
	do
		num=$((num+1))
		if [ -z "${IF_NAME[$num]}" ]; then
			error "IF_NAME" $num
			return 1
		fi

		if [ -z "${DLCI_NUM[$num]}" -a $PROTOCOL = WAN_FR ]; then
			error "DLCI_NUM" $num
			return 1
		fi

		if [ -z "${DLCI_NUM[$num]}" -a $PROTOCOL = WAN_MFR ]; then
			error "DLCI_NUM" $num
			return 1
		fi


		#if [ -z "${X25_ADDR[$num]}" -a $PROTOCOL = WAN_X25 ]; then
		#	error "X25_ADDR" $num
		#	return 1
		#fi
	done

	num=0
	while [ $num -ne $NUM_OF_INTER ];
	do
		num=$((num+1))
		if [ -z "${INTR_STATUS[$num]}" ]; then
			error "INTR_STATUS" $num
			return 1
		fi

		if [ $PROTOCOL = WAN_X25 ]; then
			if [ "${PROT_STATUS[$num]}" = Unconfigured -a  ${CH_TYPE[$num]} = SVC ];
			then
				error "PROT_STATUS" $num
				return 1
			fi
		fi

		#if [ "${OP_MODE[$num]}" = "BRIDGE_NODE" ] || [ "${OP_MODE[$num]}" = "BRIDGE" ];
		#then
		#	if  [ $MTU -lt 1520 ]; then
		#		error "MTU_BRIDGE"
		#		return 1
		#	fi
		#fi
	done

	num=0
	while [ $num -ne $NUM_OF_INTER ];
	do

		num=$((num+1))

		if [ ${OP_MODE[$num]} = API ] || [ ${OP_MODE[$num]} = BRIDGE ] || [ ${OP_MODE[$num]} = ANNEXG ]; then
			continue
		fi

		if [ ${OP_MODE[$num]} = SWITCH ]; then
			continue
		fi

		if [ ${OP_MODE[$num]} = TDM_VOICE ]; then
			continue
		fi

		if [ $PROTOCOL = "WAN_ADSL" ] && [ ${OP_MODE[$num]} = PPPoE ]; then

			if [ $ADSL_EncapMode != ETH_LLC_OA ] && [ $ADSL_EncapMode != ETH_VC_OA ]; then
				error "NON_ETH_PPPoE" $num
				return 1
			fi
		
			if [ $ADSL_EncapMode = ETH_VC_OA ]; then
				warning "ETH_VC_PPPoE" $num
				if [ $? -ne 0 ]; then
					return 1
				fi
			fi
		
			continue
		fi

		if [ -z "${IP_STATUS[$num]}" ]; then
			error "IP_STATUS" $num
			return 1
		fi
		if [ -z "${L_IP[$num]}" ]; then
			error "L_IP" $num
			return 1
		fi

		if [ -z "${R_IP[$num]}" ] && [ "${OP_MODE[$num]}" != "BRIDGE_NODE" ]; then 

			if [ $PROTOCOL = "WAN_ADSL" ]; then
			
				if [ "$ADSL_EncapMode" != "ETH_LLC_OA" ] && [ "$ADSL_EncapMode" != "ETH_VC_OA" ]; then
					error "R_IP" $num
					return 1
				fi

			elif [ $PROTOCOL = "WAN_ATM" ]; then

				if [ "${atm_encap_mode[$num]}" != "ETH_LLC_OA" ] && [ "${atm_encap_mode[$num]}" != "ETH_VC_OA" ]; then
					error "R_IP" $num
					return 1
				fi
			
			else
		
				error "R_IP" $num
				return 1
			fi
		fi
		if [ -z "${NMSK_IP[$num]}" ]; then
			error "NMSK_IP" $num
			return 1
		fi

		if [ "${OP_MODE[$num]}" = "BRIDGE_NODE" ] && \
			[ ${NMSK_IP[$num]} = "255.255.255.255" ]; then
			error "NMSK_BRIDGE_IP" $num
			return 1
		fi
	done

	# Check for the X25 Channel, Network intreface
	# matching.  Number of channels must be equal
	# to number of interfaces defined.
	if [ $PROTOCOL = WAN_X25 ]; then

		max_channels=0;
	
		if [ $HIGH_SVC -ne 0 ]; then

			if [ $LOW_SVC -eq 0 ]; then
				error "X25_LOW_SVC_ZERO" 
				return 1;
			fi
		
			max_channels=$((HIGH_SVC-LOW_SVC+1));
		fi

		if [ $HIGH_PVC -ne 0 ]; then

			if [ $LOW_PVC -eq 0 ]; then
				error "X25_LOW_PVC_ZERO"
				return 1;
			fi

			max_channels=$((HIGH_PVC-LOW_PVC + 1 + max_channels))
		
		fi

		if [ $max_channels -ne $NUM_OF_INTER ]; then
			error "X25_CHANNEL_NUM_MISMATCH" 
			return 1
		fi
	fi

}

function parse_config_file () {

	local pvc_tst
	local devname
	tmp_ifs=$IFS
	IFS="="
	ifnum=0
	num=0

	unset profile_file

	while read name value ;
	do
		name=${name// /}

		#Convert to upper case
		name=`echo $name | tr a-z A-Z`

		case $name in

		\[*\.*\]*)
			tmp=${name//[/}
			tmp=${tmp//]/}
			tmp=${tmp// /}
			tmp=`echo $tmp | tr A-Z a-z`

			#echo "skipping $name: profile $tmp"

			rc=`echo $tmp | grep lapb -i`
			if [ "$rc" = "" ]; then
				profile_file=$X25_PROFILE_DIR/$tmp

				if [ ! -d $X25_PROFILE_DIR ]; then
					\mkdir -p $X25_PROFILE_DIR
				fi
			else
				profile_file=$LAPB_PROFILE_DIR/$tmp

				if [ ! -d $LAPB_PROFILE_DIR ]; then
					\mkdir -p $LAPB_PROFILE_DIR
				fi
			fi
		
			name=`echo $name | tr A-Z a-z`

			echo "$name"  > $profile_file

			continue;
			;;
			
		\[*\]*) 
			unset profile_file	
			;;
		esac

		case $value in
		
		*x25*)
			name=`echo $name | tr A-Z a-z`
			echo "$name = $value" >> $ANNEXG_DIR/${IF_NAME[$ifnum]}".x25"  
			continue
			;;
		*lapb*)
			name=`echo $name | tr A-Z a-z`
			echo "$name = $value" > $ANNEXG_DIR/${IF_NAME[$ifnum]}".lapb"
			continue
			;;
		*dsp*)
			name=`echo $name | tr A-Z a-z`
			echo "$name = $value" >> $ANNEXG_DIR/${IF_NAME[$ifnum]}".x25"
			continue
			;;
		esac


		if [ ! -z $profile_file ]; then
			name=`echo $name | tr a-z A-Z`
			if [ ! -z $name ]; then
				echo "$name = $value" >> $profile_file
			fi
			continue;
		fi

		value=${value%%#*}
		value=${value// /}

	#--------- Interface Definition --------------------

		if [ ! -z $DEVICE_NUM ]; then
			devname="wanpipe"$DEVICE_NUM
			case $value in
		
			*$devname*)

				  ifnum=$((ifnum+1))

				  #Conver to lower case
				  name=`echo $name | tr A-Z a-z`

				  IF_NAME[$ifnum]=$name

				  if [ -f "$WAN_SCRIPTS_DIR/wanpipe$DEVICE_NUM-${IF_NAME[$ifnum]}-start" ] || 
	   			     [ -f "$WAN_SCRIPTS_DIR/wanpipe$DEVICE_NUM-${IF_NAME[$ifnum]}-stop" ]; then
					SCRIPT_CFG[$ifnum]=Enabled
				  else
					SCRIPT_CFG[$ifnum]=Disabled
				  fi

				  #echo "If_name $ifnum = ${IF_NAME[$ifnum]}"

				  if [ $PROTOCOL = WAN_FR ] || [ $PROTOCOL = WAN_MFR ] || [ $PROTOCOL = WAN_AFT ] || [ $PROTOCOL = WAN_AFT_TE3 ] ; then
					DLCI_NUM[$ifnum]=`echo $value | cut -f2 -d ','`
				  elif [ $PROTOCOL = WAN_X25 ]; then
					X25_ADDR[$ifnum]=`echo $value | cut -f2 -d ','`
					pvc_tst=${X25_ADDR[$ifnum]}
					pvc_tst=${pvc_tst%@*}
					pvc_tst=${pvc_tst// /}
					if [ -z $pvc_tst ]; then
						CH_TYPE[$ifnum]=SVC
						X25_ADDR[$ifnum]=${X25_ADDR[$ifnum]/@/}
					else
						CH_TYPE[$ifnum]=PVC
						X25_ADDR[$ifnum]=$pvc_tst
					fi
				  fi

				  OP_MODE[$ifnum]=`echo $value | cut -f3 -d ','`
				  if [ ${OP_MODE[$ifnum]} = API ]; then
					INTR_STATUS[$ifnum]=setup;
				  fi
				  NUM_OF_INTER=$ifnum
				  ;; 
			esac


			case $value in

			*ANNEXG*) 

				if [ ! -d $ANNEXG_DIR ]; then
					\mkdir -p $ANNEXG_DIR
				fi
			
				if [ -f $ANNEXG_DIR/${IF_NAME[$ifnum]}".lapb" ]; then
					rm -f $ANNEXG_DIR/${IF_NAME[$ifnum]}".lapb"
				fi

				if [ -f $ANNEXG_DIR/${IF_NAME[$ifnum]}".x25" ]; then
					rm -f $ANNEXG_DIR/${IF_NAME[$ifnum]}".x25"
				fi
				;;
			esac
		fi
	
	#-----------  Rest of the configuration file --------------------------
	
		case $name in 

		WANPIPE*) DEVICE_NUM=${name##WANPIPE}
			  setup_name_prefix $DEVICE_NUM
			  PROTOCOL=`echo $value | cut -f1 -d ','`
			  case $PROTOCOL in
                          WAN_FR)
                                backtitle="WANPIPE Configuration Utility: Frame Relay Setup"
				firm_name="fr514.sfm"
                                ;;
			  WAN_MFR)
                                backtitle="WANPIPE Configuration Utility: Frame Relay Setup"
				firm_name="cdual514.sfm"
                                ;;	
                          WAN_CHDLC)
                                backtitle="WANPIPE Configuration Utility: CHDLC Setup"
				firm_name="cdual514.sfm"
                                ;;
                          WAN_PPP)
                                backtitle="WANPIPE Configuration Utility: PPP Setup"
				firm_name="ppp514.sfm"
                                ;;
			  WAN_X25)
				backtitle="WANPIPE Configuration Utility: X25 Setup"
				firm_name="x25_514.sfm"
				;;
			  WAN_MULTPPP)
			  	backtitle="WANPIPE Configuration Utility: MultPort RAW/PPP/CHDLC Setup"
				firm_name="cdual514.sfm"
				;;
			  WAN_MULTPROT)
			  	backtitle="WANPIPE Configuration Utility: Multi Protocol Setup"
				firm_name="cdual514.sfm"
				;;	
			  
			  WAN_TTYPPP)
			  	backtitle="WANPIPE Configuration Utility: Sync/Async TTY PPP Setup"
				firm_name="cdual514.sfm"
				;;
			  WAN_BITSTRM)
			  	backtitle="WANPIPE Configuration Utility: Bit Stream Setup"
				firm_name="bitstrm.sfm"
				;;
                          WAN_AFT)
			  	backtitle="WANPIPE Configuration Utility: AFT TE1 Setup"
				firm_name="cdual514.sfm"
				;;
			  WAN_AFT_TE3)
			  	backtitle="WANPIPE Configuration Utility: AFT TE3 Setup"
				firm_name="cdual514.sfm"
				;;

			  WAN_EDU_KIT) 
				backtitle="WANPIPE Configuration Utility: Educational Kit Setup"
				firm_name="edu_kit.sfm"
			  	;;
			  WAN_BSC)
			  	backtitle="WANPIPE Configuration Utility: BiSync Setup"
				firm_name="bscmp514.sfm"
			  	;;
			  WAN_BSCSTRM)
			  	backtitle="WANPIPE Configuration Utility: BiSync Streaming Setup"
				firm_name="bscstrm514.sfm"
			  	;;
			  WAN_ADSL)
			  	backtitle="WANPIPE Configuration Utility: ADSL Setup"
				firm_name="adsl.sfm"
			  	;;
			  WAN_ATM)
			  	backtitle="WANPIPE Configuration Utility: ATM Setup"
				firm_name="atm514.sfm"
			  	;;
			  WAN_SS7)
			  	backtitle="WANPIPE Configuration Utility: SS7 Setup"
				firm_name="ss7.sfm"
				PROT_STATUS[1]="Setup Done"
			  	;;
			  WAN_POS)
			  	backtitle="WANPIPE Configuration Utility: POS Setup"
				firm_name="pos4680.sfm"
			  	;;
			  WAN_ADCCP)
				backtitle="WANPIPE Configuration Utility: ADCCP LAPB Setup"
				firm_name="adccp.sfm"
			  	;;
			  WAN_MLINK_PPP)
			  	backtitle="WANPIPE Configuration Utility: MultiLink PPP Setup"
				firm_name="cdual514.sfm"
				;;
			  *)
				echo "Error in parsing $PROD_CONF/wanpipe$DEVICE_NUM.conf"
				echo "Illegal protocol : $PROTOCOL"
				cleanup
				exit 1
				;;
                          esac
	
			  #echo "DEVICE_NUM=$DEVICE_NUM"
			  #echo "Protocol = $PROTOCOL"
			  ;;
			  
	#-----------Hardware Setup ---------------------

		CARD_TYPE*)
			  if [ "$value" = S51X ]; then
				DEVICE_TYPE="S514"
			  elif [ "$value" = S50X ]; then
			  	DEVICE_TYPE="S508"
			  elif [ "$value" = S518 ] || [ "$value" = ADSL ]; then
			  	DEVICE_TYPE="S518"
			  elif [ "$value" = AFT ]; then
			  	DEVICE_TYPE="AFT"
			  else
			  	DEVICE_TYPE="S514"
			  fi
			  ;;
	
		S514CPU*) S514CPU=$value
			  ;;

		AUTO_PCISLOT*)
			  AUTO_PCI_CFG=$value
			  ;;
		PCISLOT*) PCISLOT=$value
			  #echo "PCISLOT=$PCISLOT" 
			  ;;
		PCIBUS*)
			  PCIBUS=$value
			  ;;
		IOPORT*)  IOPORT=$value
			  #echo "IOPORT=$IOPORT"
			  ;;
		IRQ*)	  IRQ=$value
			  #echo "IRQ=$IRQ"
			  ;;
		# TE1
		MEDIA*)  
			  if [ $value = 56K ]; then
			  	MEDIA=$value
			  	DEVICE_56K_TYPE=YES
			  else
			  	MEDIA=$value
			  	DEVICE_TE1_TYPE=YES
			  fi
			  #echo "MEDIA=$MEDIA" 
			  ;;
		FE_MEDIA*)  
			  if [ $value = 56K ]; then
			  	MEDIA=$value
			  	DEVICE_56K_TYPE=YES
			  else
			  	MEDIA=$value
			  	DEVICE_TE1_TYPE=YES
			  fi
			  #echo "MEDIA=$MEDIA" 
			  ;;
		LCODE*)   LCODE=$value
			  #echo "LCODE=$LCODE" 
			  ;;
		FE_LCODE*)   LCODE=$value
			  #echo "LCODE=$LCODE" 
			  ;;
		FRAME*)   FRAME=$value
			  #echo "FRAME=$FRAME" 
			  ;;
		FE_FRAME*)   FRAME=$value
			  #echo "FRAME=$FRAME" 
			  ;;
		TE_CLOCK*) TE_CLOCK=$value
			  #echo "TE_CLOCK=$TE_CLOCK" 
			  ;;
		ACTIVE_CH*) 
			  if [ $num -eq 0 ]; then
				ACTIVE_CH=$value
			  else
				BSTRM_ACTIVE_CH[$num]=$value
			  fi
			  ;;
		FE_ACTIVE_CH*) 
			  if [ $num -eq 0 ]; then
				ACTIVE_CH=$value
			  else
				BSTRM_ACTIVE_CH[$num]=$value
			  fi
			  ;;
		LBO*)     LBO=$value
			  #echo "LBO=$LBO" 
			  ;;
		FE_LBO*)     LBO=$value
			  #echo "LBO=$LBO" 
			  ;;
		HIGHIMPEDANCE*) HIGHIMPEDANCE=$value
			  ;;
		FE_HIGHIMPEDANCE*) HIGHIMPEDANCE=$value
			  ;;
		TTY_MINOR*)
			  TTY_MINOR=$value	
	                  ;;
		TTY_MODE*)TTY_MODE=$value
			  ;;
		
		TTY*)   if [ $value = YES ]; then
			PROTOCOL=WAN_TTYPPP
			backtitle="WANPIPE Configuration Utility: Sync/Async TTY PPP Setup"
			firm_name="cdual514.sfm"
			fi
			;;
		
		FIRMWARE*) FIRMWARE=${value%/*}
			  #echo "FIRMWARE=$FIRMWARE" 
			  ;;
		MEMADDR*) MEMORY=${value%/*}
			  #echo "MEMORY=$MEMORY"
			  ;;
		INTERFACE*)INTERFACE=$value
			  #echo "INTERFACE=$INTERFACE" 
			  ;;
		NUMBER_OF_DLCI*)
			  NUMBER_OF_DLCI=$value
			  #echo "NUMBER_OF_DLCI=$NUMBER_OF_DLCI"
			  ;;
		CLOCKING*) CLOCKING=$value
			  #echo "CLOCKING=$CLOCKING" 
			  ;;
		BAUDRATE*) BAUDRATE=$value
			  #echo "BAUDRATE=$BAUDRATE" 
			  ;;
		MTU*)	 
			  if [ $num -eq 0 ]; then
				MTU=$value
			  else
				IFACE_MTU[$num]=$value
			  fi
			  ;;

		MRU*)	 
			  if [ $num -eq 0 ]; then
				MRU=$value
			  else
				IFACE_MRU[$num]=$value
			  fi
			  ;;
		UDPPORT*) UDPPORT=$value
			  #echo "UDPPORT=$UDPPORT" 
			  ;;
		STATION_ADDR*)  HDLC_STATION_ADDR=$value
			  ;;

		STATION*) 
			  if [ $num -eq 0 ]; then
			          STATION=$value
				  #echo "STATION=$STATION" 
			  else
				  STATION_IF[$num]=$value
			  fi
	
			  if [ $PROTOCOL = "WAN_POS" ]; then
			  	pos_protocol=$STATION
				if [ $STATION = IBM4680 ]; then
					firm_name="pos4680.sfm"
				elif [ $STATION = NCR2127 ]; then
					firm_name="pos2127.sfm"	
				elif [ $STATION = NCR2126  ]; then
					firm_name="pos2126.sfm"
				elif [ $STATION = NCR1255 ]; then
					firm_name="pos1255.sfm"
				elif [ $STATION = NCR7000 ]; then
					firm_name="pos7000.sfm"
				else
					firm_name="posicl.sfm"	
				fi
			  fi
			  ;;
		SIGNALLING*) 
		
			  if [ $num -eq 0 ]; then
				SIGNAL=$value
			  else
				SIGNAL_IF[$num]=$value
			  fi
			    #echo "SIGNAL=$SIGNAL" 
			  ;;
		T391*)	  T391=$value
			  #echo "T391=$T391" 
			  ;;
		T392*)	  T392=$value
			  #echo "T392=$T392" 
			  ;;
		N391*)	  N391=$value
			  #echo "N391=$N391" 
			  ;;
		N392*)	  N392=$value
			  #echo "N392=$N392"
			  ;;
		N393*) 	  N393=$value
			  #echo "N393=$N393" 
			  ;;
		TTL*)	  TTL=$value
			  #echo "TTL=$TTL" 
			  ;;
		IGNORE_FRONT_END*)
			  IGNORE_FRONT_END=$value
			  ;;
		IP_MODE*) IPMODE=$value
			  #echo "IPMODE=$IPMODE"
			  ;;
		COMMPORT*) COMMPORT=$value
			  #echo "COMMPORT=$COMMPORT"
			  ;;
		RECEIVE_ONLY*) REC_ONLY=$value
			  ;;	
		LOWESTPVC*) LOW_PVC=$value
			  #echo "LOWESTPVC=$LOW_PVC"
			  ;;
		HIGHESTPVC*) HIGH_PVC=$value
			  #echo "HIGHESTPVC=$HIGH_PVC"
			  ;;
		LOWESTSVC*) LOW_SVC=$value
			  #echo "LOWESTSVC=$LOW_SVC"
			  ;;
		HIGHESTSVC*) HIGH_SVC=$value
			  #echo "HIGHESTSVC=$HIGH_SVC"
			  ;;
		HDLCWINDOW*) HDLC_WIN=$value
			  #echo "HDLCWINDOW=$HDLC_WIN"
			  ;;
		PACKETWINDOW*) PACKET_WIN=$value 	
			  #echo "PACKETWINDOW=$PACKET_WIN"
			  ;;
		DEF_PKT_SIZE*)
				DEF_PKT_SIZE=$value
			  ;;
		MAX_PKT_SIZE*)
				MAX_PKT_SIZE=$value
			  ;;
		CCITTCOMPAT*) CCITT=$value
			  #echo "CCITTCOMPAT=$CCITT"
			  ;;
		LAPB_HDLC_ONLY*) LAPB_HDLC_ONLY=$value
			  #echo "LAPB_HDLC_ONLY=$LAPB_HDLC_ONLY"
			  ;;
		CALL_SETUP_LOG*) CALL_LOGGING=$value
			  #echo "CALL_SETUP_LOG=$CALL_LOGGING"
			  ;;
		OOB_ON_MODEM*)  OOB_ON_MODEM=$value
			  ;;
		T10_T20*)	X25_T10_T20=$value
			;;
		T11_T21*)	X25_T11_T21=$value
			;;
		T12_T22*)	X25_T12_T22=$value
			;;
		T13_T23*)	X25_T13_T23=$value
			;;
		T16_T26*)	X25_T16_T26=$value
			;;
		T16_T26*)	X25_T16_T26=$value
			;;
		T28*)	X25_T28=$value
			;;
		T1*)	X25_T1=$value
			;;
		T2*)	X25_T2=$value
			;;
		T4*)	X25_T4=$value
			;;
		N2*)	X25_N2=$value
			;;
		R10_R20*)	X25_R10_R20=$value
			;;
		R12_R22*)	X25_R12_R22=$value
			;;
		R13_R23*)	X25_R13_R23=$value
			;;

		SYNC_OPTIONS*)	SYNC_OPTIONS=$value
			;;
		RX_SYNC_CHAR*)
				RX_SYNC_CHAR=$value
			;;
		MONOSYNC_TX_TIME_FILL_CHAR*)
				MSYNC_TX_TIMER=$value
			;;
		MAX_LENGTH_TX_DATA_BLOCK*)
				MAX_TX_BLOCK=$value
			;;
		RX_COMPLETE_LENGTH*)
				RX_COMP_LEN=$value
			;;
		RX_COMPLETE_TIMER*)
				RX_COMP_TIMER=$value
			;;
		FR_ISSUE_FS*)
				FR_ISSUE_FS=$value
			;;

		CONNECTION*)
			CONNECTION=$value
			;;
		
		LINECODING*)
			LINECODING=$value
			;;
		
		LINEIDLE*)
			LINEIDLE=$value
			;;


		LINE_CONFIG_OPTIONS*)
			LINE_CONFIG_OPTIONS=$value
			;;
			
		MODEM_CONFIG_OPTIONS*)
			MODEM_CONFIG_OPTIONS=$value
			;;
			
		MODEM_STATUS_TIMER*)
			MODEM_STATUS_TIMER=$value
			;;
		API_OPTIONS*)
			API_OPTIONS=$value
			;;
		PROTOCOL_OPTIONS*)
			PROTOCOL_OPTIONS=$value
			;;
		PROTOCOL_SPECIFICATION*)
			PROTOCOL_SPECIFICATION=$value
			;;
		STATS_HISTORY_OPTIONS*)
			STATS_HISTORY_OPTIONS=$value
			;;
		MAX_LENGTH_MSU_SIF*)
			MAX_LENGTH_MSU_SIF=$value
			;;
		MAX_UNACKED_TX_MSUS*)
			MAX_UNACKED_TX_MSUS=$value
			;;
		LINK_INACTIVITY_TIMER*)
			LINK_INACTIVITY_TIMER=$value
			;;
		T1_TIMER*)
			T1_TIMER=$value
			;;
		T2_TIMER*)
			T2_TIMER=$value
			;;
		T3_TIMER*)
			T3_TIMER=$value
			;;
		T4_TIMER_EMERGENCY*)
			T4_TIMER_EMERGENCY=$value
			;;
		T4_TIMER_NORMAL*)
			T4_TIMER_NORMAL=$value
			;;
		T5_TIMER*)
			T5_TIMER=$value
			;;
		T6_TIMER*)
			T6_TIMER=$value
			;;
		T7_TIMER*)
			T7_TIMER=$value
			;;
		T8_TIMER*)
			T8_TIMER=$value
			;;
		N1*)
			N1=$value
			;;
		N2*)
			N2=$value
			;;
		TIN*)
			TIN=$value
			;;
		TIE*)
			TIE=$value
			;;

		ADSL_WATCHDOG*)
			ADSL_WATCHDOG=$value
			;;

		ATM_AUTOCFG*)
			ADSL_ATM_autocfg=$value
			;;

		ENCAPMODE*)
			if [ $num -eq 0 ]; then
				ADSL_EncapMode=$value
			else
				atm_encap_mode[$num]=$value
			fi
			;;
		VCI*)
			if [ $num -eq 0 ]; then
				ADSL_Vci=$value
			else
				atm_vci[$num]=$value
			fi
			;;
		VPI*)
			if [ $num -eq 0 ]; then
				ADSL_Vpi=$value
			else
				atm_vpi[$num]=$value
			fi
			;;
	
		ATM_HUNT_TIMER*)
			ATM_HUNT_TIMER=$value
			;;
		
		ATM_CELL_CFG*)
			ATM_CELL_CFG=$value
			;;
		
		ATM_CELL_PT*)
			ATM_CELL_PT=$value
			;;
			
		ATM_CELL_CLP*)
			ATM_CELL_CLP=$value
			;;

		ATM_CELL_PAYLOAD*)
			ATM_CELL_PAYLOAD=$value
			;;
	
		ATM_SYNC_OFFSET*)
			ATM_SYNC_OFFSET=$value;
			;;

		ATM_SYNC_DATA*)
			ATM_SYNC_DATA=$value;
			;;

		ATM_SYNC_MODE*)
			ATM_SYNC_MODE=$value;
			;;

		RXBUFFERCOUNT*)
			ADSL_RxBufferCount=$value
			;;
		TXBUFFERCOUNT*)
			ADSL_TxBufferCount=$value
			;;
		ADSLSTANDARD*)
			ADSL_Standard=$value
			;;
		ADSLTRELLIS*)
			ADSL_Trellis=$value
			;;
		ADSLTXPOWERATTEN*)
			ADSL_TxPowerAtten=$value
			;;
		ADSLCODINGGAIN*)
			ADSL_CodingGain=$value
			;;
		ADSLMAXBITSPERBIN*)
			ADSL_MaxBitsPerBin=$value
			;;
		ADSLTXSTARTBIN*)
			ADSL_TxStartBin=$value
			;;
		ADSLTXENDBIN*)
			ADSL_TxEndBin=$value
			;;
		ADSLRXSTARTBIN*)
			ADSL_RxStartBin=$value
			;;
		ADSLRXENDBIN*)
			ADSL_RxEndBin=$value
			;;
		ADSLRXBINADJUST*)
			ADSL_RxBinAdjust=$value
			;;
		ADSLFRAMINGSTRUCT*)
			ADSL_FramingStruct=$value
			;;
		ADSLEXPANDEDEXCHANGE*)
			ADSL_ExpandedExchange=$value
			;;
		ADSLCLOCKTYPE*)
			ADSL_ClockType=$value
			;;
		ADSLMAXDOWNRATE*)
			ADSL_MaxDownRate=$value
			;;
			
		SUERM_ERROR_THRESHOLD*)
			SUERM_ERROR_THRESHOLD=$value
			;;
		SUERM_NUMBER_OCTETS*)
			SUERM_NUMBER_OCTETS=$value
			;;
		SUERM_NUMBER_SUS*)
			SUERM_NUMBER_SUS=$value
			;;
		SIE_INTERVAL_TIMER*)
			SIE_INTERVAL_TIMER=$value
			;;
		SIO_INTERVAL_TIMER*)
			SIO_INTERVAL_TIMER=$value
			;;
		SIOS_INTERVAL_TIMER*)
			SIOS_INTERVAL_TIMER=$value
			;;
		FISU_INTERVAL_TIMER*)
			FISU_INTERVAL_TIMER=$value
			;;

		BSCSTRM_ADAPTER_FR*)
			BSCSTRM_ADAPTER_FR=$value
			;;
		BSCSTRM_MTU*)
			BSCSTRM_MTU=$value
			;;
		BSCSTRM_EBCDIC*)
			BSCSTRM_EBCDIC=$value	
			;;
		BSCSTRM_RB_BLOCK_TYPE*)
			BSCSTRM_RB_BLOCK_TYPE=$value
			;;
		BSCSTRM_NO_CONSEC_PAD_EOB*)
			BSCSTRM_NO_CONSEC_PAD_EOB=$value
			;;
		BSCSTRM_NO_ADD_LEAD_TX_SYN_CHARS*)
			BSCSTRM_NO_ADD_LEAD_TX_SYN_CHARS=$value
			;;
		BSCSTRM_NO_BITS_PER_CHAR*)
			BSCSTRM_NO_BITS_PER_CHAR=$value
			;;
		BSCSTRM_PARITY*)
			BSCSTRM_PARITY=$value
			;;
		BSCSTRM_MISC_CONFIG_OPTIONS*)
			BSCSTRM_MISC_CONFIG_OPTIONS=$value
			;;
		BSCSTRM_STATISTICS_OPTIONS*)
			BSCSTRM_STATISTICS_OPTIONS=$value
			;;
		BSCSTRM_MODEM_CONFIG_OPTIONS*)
			BSCSTRM_MODEM_CONFIG_OPTIONS=$value
			;;

		\[DEVICES\]*)
			;;
		\[INTERFACES\]*)
			;;
		\[WANPIPE*\]*)
			;;
			
	#--------- Interface Setup ---------------------
		\[*\]*) 
			num=$((num+1))
			INTR_STATUS[$num]=setup
			PROT_STATUS[$num]="Setup Done"
			;;

		MULTICAST*) 
			MULTICAST[$num]=$value
			#echo "MULTICAST $num =${MULTICAST[$num]}"
			  ;;
		INARPINTERVAL*) 
			INARP_INT[$num]=$value
			#echo "INARPINTERVAL $num =${INARP_INT[$num]}"
			  ;;
		INARP_RX*)
			INARP_RX[$num]=$value
			;;
		INARP*)   
			INARP[$num]=$value
			#echo "INARP $num =${INARP[$num]}"
			  ;;
		CIR*)  	CIR[$num]=$value
			#echo "CIR $num =${CIR[$num]}"
			  ;;
		BC*)	BC[$num]=$value
			#echo "BC $num =${BC[$num]}"
			  ;;
		BE*)	BE[$num]=$value	
			#echo "BE $num =${BE[$num]}"
			  ;;
		IGNORE_DCD*) 
			DCD[$num]=$value
			MPPP_MODEM_IGNORE[$num]=$value
			#echo "DCD $num =${DCD[$num]}"
			  ;;
		IGNORE_CTS*) 
			CTS[$num]=$value
			MPPP_MODEM_IGNORE[$num]=$value
			#echo "CTS $num =${CTS[$num]}"
			  ;;
		IGNORE_KEEP*)
			KEEP[$num]=$value	
			#echo "KEEP $num =${KEEP[$num]}"
			  ;;
		HDLC_STREAMING*) 
			STREAM[$num]=$value
			#echo "STREAM $num =${STREAM[$num]}"
			  ;;
		SEVEN_BIT_HDLC*)
			SEVEN_BIT_HDLC[$num]=$value
			  ;;
		KEEPALIVE_TX_TIMER*)
			TXTIME[$num]=$value
			#echo "TXTIME $num =${TXTIME[$num]}"
			  ;;
		KEEPALIVE_RX_TIMER*)
			RXTIME[$num]=$value
			#echo "RXTIME $num =${RXTIME[$num]}"
			  ;;
		KEEPALIVE_ERR_MARGIN*)
			ERR[$num]=$value
			#echo "ERR $num =${ERR[$num]}"
			  ;;
		SLARP_TIMER*)
			SLARP[$num]=$value
			if [ ${SLARP[$num]} -eq 0 ]; then
				CHDLC_IPMODE[$num]=NO;
			else
				CHDLC_IPMODE[$num]=YES;
			fi
			#echo "SLARP $num =${SLARP[$num]}"
			  ;;
		PAP*)
			PAP[$num]=$value
			#echo "PAP $num =${PAP[$num]}"
			  ;;
		CHAP*)	
			CHAP[$num]=$value
			#echo "CHAP $num =${CHAP[$num]}"
			  ;;
		IPX*)
			IPX[$num]=$value
			#echo "IPX $num =${IPX[$num]}"
			;;
		NETWORK*)
			NETWORK[$num]=$value
			#echo "NETWORK $num =${NETWORK[$num]}"
			;;
		USERID*)
			USERID[$num]=$value
			#echo "USERID $num =${USERID[$num]}"
			  ;;
		PASSWD*)
			PASSWD[$num]=$value
			#echo "PASSWD $num =${PASSWD[$num]}"
			  ;;
		SYSNAME*)
			SYSNAME[$num]=$value
			#echo "SYSNAME $num =${SYSNAME[$num]}"
			  ;;
		IDLETIMEOUT*)
			IDLE_TIMEOUT[$num]=$value
			#echo "IDLETIMEOUT $num =${IDLE_TIMEOUT[$num]}"
			  ;;

		IDLE_FLAG*)
			IDLE_FLAG[$num]=$value
			  ;;
			
		HOLDTIMEOUT*)
			HOLD_TIMEOUT[$num]=$value
			#echo "HOLDTIMEOUT $num =${HOLD_TIMEOUT[$num]}"
			;;
		DYN_INTR_CFG*)
			DYNAMIC_INTR[$num]=$value
			;;
		TRUE_ENCODING_TYPE*)
			TRUE_ENCODING[$num]=$value
			;;
		X25_SRC_ADDR*)
			SRC_ADDR[$num]=$value
			#echo "X25_SRC_ADDR $num =${SRC_ADDR[$num]}"
			;;
		X25_ACCEPT_DST_ADDR*)
			X25_ACC_DST_ADDR[$num]=$value
			#echo "ACCEPT_CALLS_FROM $num =${X25_ACCEPT_CALLS_FROM[$num]}"
			;;
		X25_ACCEPT_SRC_ADDR*)
			X25_ACC_SRC_ADDR[$num]=$value
			#echo "ACCEPT_CALLS_FROM $num =${X25_ACCEPT_CALLS_FROM[$num]}"
			;;
		X25_ACCEPT_USR_DATA*)
			X25_ACC_USR_DATA[$num]=$value
			#echo "ACCEPT_CALLS_FROM $num =${X25_ACCEPT_CALLS_FROM[$num]}"
			;;

		MPPP_PROT*)
			MPPP_PROT[$num]=$value
			;;

		PROTOCOL*)
			MPPP_PROT[$num]=$value
			;;	

		SW_DEV_NAME*) BSTRM_SWITCH_DEV[$num]=$value
			;;

		OAM_LOOPBACK_INT*)
			atm_oam_loopback_int[$num]=$value
			;;

		OAM_LOOPBACK*)
			atm_oam_loopback[$num]=$value
			;;
		
		OAM_CC_CHECK_INT*)
			atm_oam_cc_check_int[$num]=$value
			;;
	
		OAM_CC_CHECK*)
			atm_oam_cc_check[$num]=$value
			;;
	
		ATMARP_INT*)
			atm_arp_int[$num]=$value
			;;
	
		ATMARP*)
			atm_arp[$num]=$value
			;;

		*) 	#echo -e "\n\nNot found *$name*"
			;;
		esac

	done < $PROD_CONF/wanpipe$DEVICE_NUM.conf 

	#------ IP Configuration ------------

	#echo "Num or interface is $NUM_OF_INTER"

	if [ "$PROTOCOL" = WAN_TTYPPP ] || [ "$PROTOCOL" = WAN_MLINK_PPP ] || [ "$PROTOCOL" = WAN_EDU_KIT ]; then
		IFS=$tmp_ifs
		return
	fi

#NC Used by PPPD not used any more
#	if [ "$OS_SYSTEM" = Linux ] && [ "$PROTOCOL" = WAN_ADSL ] && [ "$ADSL_EncapMode" = PPP_LLC_OA -o "$ADSL_EncapMode" = PPP_VC_OA ]; then
#		IFS=$tmp_ifs
#		return
#	fi	
	

	if [ $LINUX_DISTR = debian ]; then
		IFS=$tmp_ifs
	fi

	num=0
	while [ $num -ne $NUM_OF_INTER ]; 
	do 
		num=$((num+1))

		if [ ! -f "$INTERFACE_DIR/$NEW_IF${IF_NAME[$num]}" ];
		then 
			if [ ${OP_MODE[$num]} = WANPIPE ] || \
			   [ ${OP_MODE[$num]} = BRIDGE_NODE ]; then
				
				echo "File not found $INTERFACE_DIR/$NEW_IF${IF_NAME[$num]}"  
			fi
			continue
		fi

		IP_STATUS[$num]="Setup Done"		

		while read name value; 
		do

	 	#Conver to upper case
	  	name=`echo $name | tr a-z A-Z`

		case $name in

		*IPADDR*) 
			L_IP[$num]=$value
			#echo "L_IP=${L_IP[$num]}"
			  ;;
		*ADDRESS*)
			L_IP[$num]=$value
			#echo "L_IP=${L_IP[$num]}"
			  ;;
		*NETMASK*)
			NMSK_IP[$num]=$value
			#echo "NMSK_IP=${NMSK_IP[$num]}"
			  ;;
		*POINTOPOINT*)
			R_IP[$num]=$value
			#echo "R_IP=${R_IP[$num]}"
			  ;;
		*BROADCAST*)
			BCAST_IP[$num]=$value
			#echo "BCAST_IP=${BCAST_IP[$num]}"
			  ;;
		*GATEWAY*)
			GATE_IP[$num]=$value
			if [ ! -z ${GATE_IP[$num]} ]; then
				DEFAULT_IP[$num]=YES
			fi
			#echo "GATE_IP=${GATE_IP[$num]}"
			  ;;
		esac

		done < $INTERFACE_DIR/$NEW_IF${IF_NAME[$num]}
	done
	IFS=$tmp_ifs


	IFS="="
	while read name value ;
	do
		name=${name// /}

		#Convert to upper case
		name=`echo $name | tr a-z A-Z`

		value=${value%%#*}
		value=${value// /}

		case $value in

		*ANNEXG*) 
			annexg_setup_menu "$ifnum" "${IF_NAME[$ifnum]}" "readonly"
			ANNEXG_STATUS[$ifnum]="Configured"
			;;
		esac

	done < $PROD_CONF/wanpipe$DEVICE_NUM.conf 


	if [ -f "$WAN_SCRIPTS_DIR/wanpipe$DEVICE_NUM-start" ] || 
	   [ -f "$WAN_SCRIPTS_DIR/wanpipe$DEVICE_NUM-stop" ]; then
		DEVICE_SCRIPTS_CFG=Enabled
	else
		DEVICE_SCRIPTS_CFG=Disabled
	fi
	
	IFS=$tmp_ifs
}

function save_configuration () {

	local date=`date`
	local prot
	
	WAN_CONFIG=$PROD_CONF/wanpipe$DEVICE_NUM.conf

	cat <<EOM > $WAN_CONFIG

#================================================
# WANPIPE$DEVICE_NUM Configuration File
#================================================
#
# Date: $date
#
# Note: This file was generated automatically
#       by ${WAN_BIN_DIR}/wancfg program.
#
#       If you want to edit this file, it is
#       recommended that you use wancfg program
#       to do so.
#================================================
# Sangoma Technologies Inc.
#================================================

EOM

	# -------------- [device] Area --------------------
	
	if [ $PROTOCOL = WAN_TTYPPP ]; then
		prot=WAN_CHDLC
	else	
		prot=$PROTOCOL
	fi

	echo "[devices]" >> $WAN_CONFIG

	echo "SAVING PROTOCOL=$PROTOCOL DEVICE_TYPE=$DEVICE_TYPE"

	if [ "$PROTOCOL" = "WAN_MULTPROT" ] && [ "$DEVICE_TYPE" = "AFT" ]; then
		echo "wanpipe$DEVICE_NUM = WAN_AFT_SERIAL, Comment" 	>> $WAN_CONFIG
	else
		echo "wanpipe$DEVICE_NUM = $prot, Comment" 	>> $WAN_CONFIG
	fi

	# -------------- [interface] Area -----------------

	skip_if=0
	
	if [ $PROTOCOL = WAN_TTYPPP ] || [ "$PROTOCOL" = WAN_MLINK_PPP ] || [ $PROTOCOL = WAN_EDU_KIT ]; then
		skip_if=1
	fi

#NC: Used by PPPD not used any more
#	if [ "$OS_SYSTEM" = Linux ] && [ "$PROTOCOL" = WAN_ADSL ] && [ "$ADSL_EncapMode" = PPP_LLC_OA -o "$ADSL_EncapMode" = PPP_VC_OA ]; then
#		skip_if=1
#	fi	

	if [ $skip_if -eq 0 ]; then

		echo -e "\n[interfaces]" 			>> $WAN_CONFIG
		if_num=0
		while [ $if_num -ne $NUM_OF_INTER ]; 
		do 
			if_num=$((if_num+1))

			if [ $PROTOCOL = WAN_X25 ]; then
				if [ ${CH_TYPE[$if_num]} = PVC ]; then
					DLCI_NUM[$if_num]="${X25_ADDR[$if_num]}"
				else
					DLCI_NUM[$if_num]="@${X25_ADDR[$if_num]}"
				fi
			fi

			echo "${IF_NAME[$if_num]} = wanpipe$DEVICE_NUM, ${DLCI_NUM[$if_num]}, ${OP_MODE[$if_num]}, Comment"				  >> $WAN_CONFIG 	

			if [ ${OP_MODE[$if_num]} = ANNEXG ]; then
				IF_NAME[$if_num]=${IF_NAME[$if_num]/ //}
				if [ -f "$ANNEXG_DIR/${IF_NAME[$if_num]}.lapb" ]; then
					cat "$ANNEXG_DIR/${IF_NAME[$if_num]}.lapb" >> $WAN_CONFIG
				
					if [ -f "$ANNEXG_DIR/${IF_NAME[$if_num]}.x25" ]; then
						cat "$ANNEXG_DIR/${IF_NAME[$if_num]}.x25" >> $WAN_CONFIG
					else
						echo "**** File NOT FOUND $ANNEXG_DIR/${IF_NAME[$if_num]}.x25"
					fi
				else
					echo "**** File NOT FOUND $ANNEXG_DIR/${IF_NAME[$if_num]}.lapb"
				fi
				echo >> $WAN_CONFIG
			fi
		done
	fi

	# -------------- [wanpipe#] Area -------------------

	echo -e "\n[wanpipe$DEVICE_NUM]" 		>> $WAN_CONFIG
	if [ "$DEVICE_TYPE" = S514 ]; then
		echo "CARD_TYPE 	= S51X"			>> $WAN_CONFIG
		echo "S514CPU 	= $S514CPU" 		>> $WAN_CONFIG
		echo "AUTO_PCISLOT	= $AUTO_PCI_CFG"    >> $WAN_CONFIG
		echo "PCISLOT 	= $PCISLOT" 		>> $WAN_CONFIG
		PCIBUS=${PCIBUS:-0}
		echo "PCIBUS		= $PCIBUS"	>> $WAN_CONFIG
	
	elif [ "$DEVICE_TYPE" = AFT ]; then
		echo "CARD_TYPE 	= AFT"			>> $WAN_CONFIG
		echo "S514CPU 	= $S514CPU" 		>> $WAN_CONFIG
		echo "AUTO_PCISLOT	= $AUTO_PCI_CFG"    >> $WAN_CONFIG
		echo "PCISLOT 	= $PCISLOT" 		>> $WAN_CONFIG
		PCIBUS=${PCIBUS:-0}
		echo "PCIBUS		= $PCIBUS"	>> $WAN_CONFIG
	
	elif [ "$DEVICE_TYPE" = "S518" ]; then
		echo "CARD_TYPE 	= S518"			>> $WAN_CONFIG
		echo "AUTO_PCISLOT	= $AUTO_PCI_CFG"    >> $WAN_CONFIG
		echo "PCISLOT 	= $PCISLOT" 		>> $WAN_CONFIG
		PCIBUS=${PCIBUS:-0}
		echo "PCIBUS		= $PCIBUS"	>> $WAN_CONFIG

	else
		echo "CARD_TYPE = S50X"			>> $WAN_CONFIG
		echo "IOPORT 		= $IOPORT" 	>> $WAN_CONFIG
		echo "IRQ 		= $IRQ" 	>> $WAN_CONFIG
	fi

	# TE1 Output T1/E1 hardware configuration.
	if [ "$DEVICE_TE1_TYPE" = YES ]; then
		echo "FE_MEDIA 		= $MEDIA" 	>> $WAN_CONFIG
		echo "FE_LCODE 		= $LCODE" 	>> $WAN_CONFIG
		echo "FE_FRAME 		= $FRAME" 	>> $WAN_CONFIG
		echo "TE_CLOCK 	= $TE_CLOCK" 		>> $WAN_CONFIG
		echo "TE_ACTIVE_CH	= $ACTIVE_CH" 	>> $WAN_CONFIG
		if [ "$MEDIA" = T1 ]; then
			echo "TE_LBO 		= $LBO" >> $WAN_CONFIG
			echo "TE_HIGHIMPEDANCE	= $HIGHIMPEDANCE" >> $WAN_CONFIG
		fi
	elif [ "$DEVICE_56K_TYPE" = YES ]; then
		echo "FE_MEDIA 		= 56K" 	>> $WAN_CONFIG
	
	else
		FE_MEDIA=""
	fi

	if [ $PROTOCOL != WAN_ADSL ]; then
		echo "Firmware	= $FIRMWARE/$firm_name" 	>>$WAN_CONFIG
	fi
	
	if [ $PROTOCOL = WAN_EDU_KIT ]; then
		return
	fi

	
	if [ -z "$MEMORY" ]; then
		MEMORY=Auto
	fi
	if [ "$MEMORY" != Auto ]; then 
		echo "MemAddr		= $MEMORY" 	>> $WAN_CONFIG
	fi
	if [ $PROTOCOL = WAN_CHDLC ]; then
		echo "CommPort	= $COMMPORT" 		>> $WAN_CONFIG
		echo "Receive_Only	= $REC_ONLY"	>> $WAN_CONFIG
		CONNECTION=${CONNECTION:-Permanent}
		LINECODING=${LINECODING:-NRZ}
		LINEIDLE=${LINEIDLE:-Flag}
		echo "Connection	= $CONNECTION"  >> $WAN_CONFIG
		echo "LineCoding	= $LINECODING" >> $WAN_CONFIG
		echo "LineIdle	= $LINEIDLE"	>> $WAN_CONFIG
	fi
	if [ $PROTOCOL = WAN_MULTPPP ] || [ $PROTOCOL = WAN_MULTPROT ]; then
		if [ $DEVICE_TYPE = "AFT" ]; then
			echo "FE_LINE	= $COMMPORT" 		>> $WAN_CONFIG
		else
			echo "CommPort	= $COMMPORT" 		>> $WAN_CONFIG
		fi
	fi
	if [ $PROTOCOL = WAN_BITSTRM ] || [ $PROTOCOL = WAN_BSCSTRM ]; then
		echo "CommPort	= $COMMPORT" 		>> $WAN_CONFIG
	fi
	if [ $PROTOCOL = WAN_TTYPPP ]; then
		if [ $TTY_MODE = Async ]; then
			echo "CommPort	= SEC" 		>> $WAN_CONFIG
		else
			echo "CommPort	= $COMMPORT" 		>> $WAN_CONFIG
		fi
		echo "TTY		= YES"		>> $WAN_CONFIG
		echo "TTY_Minor 	= $TTY_MINOR"	>> $WAN_CONFIG
		echo "TTY_Mode	= $TTY_MODE"	>> $WAN_CONFIG
	fi

	if [ $DEVICE_TYPE != S518 ]; then
		echo "Interface 	= $INTERFACE" 		>> $WAN_CONFIG
	fi

	if [ $PROTOCOL != WAN_TTYPPP ] || [ "$TTY_MODE" = Sync ]; then
		if [ $DEVICE_TYPE != S518 ]; then
			echo "Clocking 	= $CLOCKING" 			>> $WAN_CONFIG
			echo "BaudRate 	= $BAUDRATE" 			>> $WAN_CONFIG
		fi
	fi
	
	echo "MTU 		= $MTU" 		>> $WAN_CONFIG
	echo "UDPPORT 	= $UDPPORT" 			>> $WAN_CONFIG

	if [ $DEVICE_TYPE = S518 ]; then

		echo " "		>> $WAN_CONFIG

		if [ $PROTOCOL = WAN_ADSL ]; then
			echo "EncapMode	= $ADSL_EncapMode" >> $WAN_CONFIG
			echo "ATM_AUTOCFG	= $ADSL_ATM_autocfg" >> $WAN_CONFIG
			echo "Vci		= $ADSL_Vci"  >> $WAN_CONFIG
			echo "Vpi		= $ADSL_Vpi"  >> $WAN_CONFIG	
			echo "ADSL_WATCHDOG	= $ADSL_WATCHDOG" >> $WAN_CONFIG
		fi

#NC: Used by PPPD not used anymore
#		if [ "$OS_SYSTEM" = Linux ] && [ "$PROTOCOL" = WAN_ADSL ] && [ "$ADSL_EncapMode" = PPP_LLC_OA -o "$ADSL_EncapMode" = PPP_VC_OA ]; then
#			echo "TTY_Minor 	= $TTY_MINOR"	>> $WAN_CONFIG
#		fi

		echo " "		>> $WAN_CONFIG

		echo "Verbose			= 1"			>> $WAN_CONFIG	
		echo "RxBufferCount		= $ADSL_RxBufferCount" 	>> $WAN_CONFIG
		echo "TxBufferCount		= $ADSL_TxBufferCount" 	>> $WAN_CONFIG
		echo "AdslStandard		= $ADSL_Standard"   	>> $WAN_CONFIG
		echo "AdslTrellis		= $ADSL_Trellis"	>> $WAN_CONFIG
		echo "AdslTxPowerAtten		= $ADSL_TxPowerAtten"	>> $WAN_CONFIG
		echo "AdslCodingGain		= $ADSL_CodingGain"	>> $WAN_CONFIG
		echo "AdslMaxBitsPerBin		= $ADSL_MaxBitsPerBin"	>> $WAN_CONFIG
		echo "AdslTxStartBin		= $ADSL_TxStartBin"	>> $WAN_CONFIG
		echo "AdslTxEndBin		= $ADSL_TxEndBin"	>> $WAN_CONFIG
		echo "AdslRxStartBin		= $ADSL_RxStartBin"	>> $WAN_CONFIG
		echo "AdslRxEndBin		= $ADSL_RxEndBin"	>> $WAN_CONFIG
		echo "AdslRxBinAdjust		= $ADSL_RxBinAdjust"	>> $WAN_CONFIG
		echo "AdslFramingStruct		= $ADSL_FramingStruct"	>> $WAN_CONFIG
		echo "AdslExpandedExchange	= $ADSL_ExpandedExchange"	>> $WAN_CONFIG
		echo "AdslClockType		= $ADSL_ClockType"	>> $WAN_CONFIG	
		echo "AdslMaxDownRate		= $ADSL_MaxDownRate"	>> $WAN_CONFIG

		echo " "		>> $WAN_CONFIG


	fi
	
	if [ $PROTOCOL = WAN_FR ] || [ $PROTOCOL = WAN_MFR ]; then
		
		if [ $PROTOCOL = WAN_MFR ]; then
			echo "CommPort	= $COMMPORT" 		>> $WAN_CONFIG
		fi
		echo "NUMBER_OF_DLCI 	= $NUM_OF_INTER" >> $WAN_CONFIG
		echo "Station 	= $STATION" 		>> $WAN_CONFIG
		echo "Signalling 	= $SIGNAL" 	>> $WAN_CONFIG
		echo "T391 		= $T391" 	>> $WAN_CONFIG
		echo "T392 		= $T392" 	>> $WAN_CONFIG
		echo "N391 		= $N391" 	>> $WAN_CONFIG
		echo "N392 		= $N392" 	>> $WAN_CONFIG
		echo "N393 		= $N393" 	>> $WAN_CONFIG
		FR_ISSUE_FS=${FR_ISSUE_FS:-YES}
		echo "FR_ISSUE_FS	= $FR_ISSUE_FS" >> $WAN_CONFIG
	fi
	if [ $PROTOCOL = WAN_PPP ]; then
		echo "IP_MODE		= $IPMODE" 	>> $WAN_CONFIG
	fi

	if [ $PROTOCOL = WAN_ADCCP ]; then
		echo "Station		= $STATION" 		>> $WAN_CONFIG
		echo "HdlcWindow	= $HDLC_WIN"		>> $WAN_CONFIG
		echo "CALL_SETUP_LOG 	= $CALL_LOGGING"	>> $WAN_CONFIG
		echo "OOB_ON_MODEM	= $OOB_ON_MODEM"	>> $WAN_CONFIG
		echo "T1		= $X25_T1"		>> $WAN_CONFIG
		echo "T2		= $X25_T2"		>> $WAN_CONFIG
		echo "T4		= $X25_T4"		>> $WAN_CONFIG
		echo "N2		= $X25_N2"		>> $WAN_CONFIG
		HDLC_STATION_ADDR=${HDLC_STATION_ADDR:-1}
		echo "STATION_ADDR	= $HDLC_STATION_ADDR"	>> $WAN_CONFIG
	fi
	if [ $PROTOCOL = WAN_X25 ]; then
		echo "Station		= $STATION" 		>> $WAN_CONFIG	
		echo "LowestPVC	= $LOW_PVC"		>> $WAN_CONFIG
		echo "HighestPVC 	= $HIGH_PVC"		>> $WAN_CONFIG
		echo "LowestSVC 	= $LOW_SVC"		>> $WAN_CONFIG
		echo "HighestSVC	= $HIGH_SVC"		>> $WAN_CONFIG
		echo "HdlcWindow	= $HDLC_WIN"		>> $WAN_CONFIG
		echo "PacketWindow	= $PACKET_WIN"		>> $WAN_CONFIG
		echo "DEF_PKT_SIZE	= $DEF_PKT_SIZE"	>> $WAN_CONFIG
		echo "MAX_PKT_SIZE	= $MAX_PKT_SIZE"	>> $WAN_CONFIG
		echo "CCITTCompat	= $CCITT"		>> $WAN_CONFIG
		echo "X25Config	= 0x0090"		>> $WAN_CONFIG
		echo "LAPB_HDLC_ONLY	= $LAPB_HDLC_ONLY"	>> $WAN_CONFIG
		echo "CALL_SETUP_LOG 	= $CALL_LOGGING"	>> $WAN_CONFIG
		echo "OOB_ON_MODEM	= $OOB_ON_MODEM"	>> $WAN_CONFIG
		echo "T1		= $X25_T1"		>> $WAN_CONFIG
		echo "T2		= $X25_T2"		>> $WAN_CONFIG
		echo "T4		= $X25_T4"		>> $WAN_CONFIG
		echo "N2		= $X25_N2"		>> $WAN_CONFIG
		echo "T10_T20		= $X25_T10_T20"		>> $WAN_CONFIG
		echo "T11_T21		= $X25_T11_T21"		>> $WAN_CONFIG
		echo "T12_T22	 	= $X25_T12_T22"		>> $WAN_CONFIG
		echo "T13_T23		= $X25_T13_T23"		>> $WAN_CONFIG
		echo "T16_T26		= $X25_T16_T26"		>> $WAN_CONFIG
		echo "T16_T26		= $X25_T16_T26"		>> $WAN_CONFIG
		echo "T28		= $X25_T28"		>> $WAN_CONFIG
		echo "R10_R20		= $X25_R10_R20"		>> $WAN_CONFIG
		echo "R12_R22		= $X25_R12_R22"		>> $WAN_CONFIG
		echo "R13_R23		= $X25_R13_R23"		>> $WAN_CONFIG
	fi

	if [ $PROTOCOL = WAN_POS ]; then
		echo "Station		= $pos_protocol" >> $WAN_CONFIG
	fi

	if [ $PROTOCOL = WAN_BITSTRM ]; then
	
		echo 	>> $WAN_CONFIG
		echo "sync_options 		 = $SYNC_OPTIONS"	>> $WAN_CONFIG
		echo "rx_sync_char 		 = $RX_SYNC_CHAR" 	>> $WAN_CONFIG
		echo "monosync_tx_time_fill_char = $MSYNC_TX_TIMER" 	>> $WAN_CONFIG
		echo "max_length_tx_data_block = $MAX_TX_BLOCK" 	>> $WAN_CONFIG
		echo "rx_complete_length 	 = $RX_COMP_LEN" 	>> $WAN_CONFIG
		echo "rx_complete_timer 	 = $RX_COMP_TIMER" 	>> $WAN_CONFIG
		echo "rbs_ch_map		 = $RBS_CH_MAP"		>> $WAN_CONFIG 
		echo 	>> $WAN_CONFIG
	fi

	echo "TTL 		= $TTL" 		>> $WAN_CONFIG

	IGNORE_FRONT_END=${IGNORE_FRONT_END:-NO}
	echo "IGNORE_FRONT_END  = $IGNORE_FRONT_END" 	>> $WAN_CONFIG


	if [ $skip_if = 1 ]; then
		return
	fi

	if [ $PROTOCOL = WAN_SS7 ]; then
		echo 	>> $WAN_CONFIG
		echo "LINE_CONFIG_OPTIONS	=${LINE_CONFIG_OPTIONS:-0x0001}" >> $WAN_CONFIG
		echo "MODEM_CONFIG_OPTIONS	=${MODEM_CONFIG_OPTIONS:-0x0000}" >> $WAN_CONFIG
		echo "MODEM_STATUS_TIMER	=${MODEM_STATUS_TIMER:-500}" >> $WAN_CONFIG
		echo "API_OPTIONS		=${API_OPTIONS:-0x0000}"  >> $WAN_CONFIG
		echo "PROTOCOL_OPTIONS	=${PROTOCOL_OPTIONS:-0x0000}"  >> $WAN_CONFIG
		echo "PROTOCOL_SPECIFICATION	=${PROTOCOL_SPECIFICATION:-ANSI}"  >> $WAN_CONFIG 
		echo "STATS_HISTORY_OPTIONS	=${STATS_HISTORY_OPTIONS:-0x001F}" >> $WAN_CONFIG
		echo "MAX_LENGTH_MSU_SIF	=${MAX_LENGTH_MSU_SIF:-272}" >> $WAN_CONFIG
		echo "MAX_UNACKED_TX_MSUS	=${MAX_UNACKED_TX_MSUS:-10}" >> $WAN_CONFIG
		echo "LINK_INACTIVITY_TIMER	=${LINK_INACTIVITY_TIMER:-5}" >> $WAN_CONFIG
		echo "T1_TIMER		=${T1_TIMER:-1300}" >> $WAN_CONFIG
		echo "T2_TIMER		=${T2_TIMER:-1150}" >> $WAN_CONFIG
		echo "T3_TIMER		=${T3_TIMER:-1150}" >> $WAN_CONFIG
		echo "T4_TIMER_EMERGENCY	=${T4_TIMER_EMERGENCY:-60}" >> $WAN_CONFIG
		echo "T4_TIMER_NORMAL		=${T4_TIMER_NORMAL:-230}" >> $WAN_CONFIG
		echo "T5_TIMER		=${T5_TIMER:-10}" >> $WAN_CONFIG
		echo "T6_TIMER		=${T6_TIMER:-400}" >> $WAN_CONFIG
		echo "T7_TIMER		=${T7_TIMER:-100}" >> $WAN_CONFIG
		echo "T8_TIMER		=${T8_TIMER:-100}" >> $WAN_CONFIG
		echo "N1			=${N1:-1}" >> $WAN_CONFIG
		echo "N2			=${N2:-2}" >> $WAN_CONFIG
		echo "TIN			=${TIN:-10}" >> $WAN_CONFIG
		echo "TIE			=${TIE:-11}" >> $WAN_CONFIG
		echo "SUERM_ERROR_THRESHOLD	=${SUERM_ERROR_THRESHOLD:-20}" >> $WAN_CONFIG
		echo "SUERM_NUMBER_OCTETS	=${SUERM_NUMBER_OCTETS:-21}" >> $WAN_CONFIG
		echo "SUERM_NUMBER_SUS	=${SUERM_NUMBER_SUS:-1024}" >> $WAN_CONFIG
		echo "SIE_INTERVAL_TIMER	=${SIE_INTERVAL_TIMER:-30}" >> $WAN_CONFIG
		echo "SIO_INTERVAL_TIMER	=${SIO_INTERVAL_TIMER:-31}" >> $WAN_CONFIG
		echo "SIOS_INTERVAL_TIMER	=${SIOS_INTERVAL_TIMER:-32}" >> $WAN_CONFIG
		echo "FISU_INTERVAL_TIMER	=${FISU_INTERVAL_TIMER:-33}" >> $WAN_CONFIG

		echo 	>> $WAN_CONFIG
	fi


	if [ $PROTOCOL = WAN_BSCSTRM ]; then
		echo 	>> $WAN_CONFIG
		echo "BSCSTRM_ADAPTER_FR	= $BSCSTRM_ADAPTER_FR" >> $WAN_CONFIG
		echo "BSCSTRM_MTU		= $BSCSTRM_MTU" >> $WAN_CONFIG
		echo "BSCSTRM_EBCDIC		= $BSCSTRM_EBCDIC" >> $WAN_CONFIG
		echo "BSCSTRM_RB_BLOCK_TYPE	= $BSCSTRM_RB_BLOCK_TYPE" >> $WAN_CONFIG
		echo "BSCSTRM_NO_CONSEC_PAD_EOB	= $BSCSTRM_NO_CONSEC_PAD_EOB" >> $WAN_CONFIG
		echo "BSCSTRM_NO_ADD_LEAD_TX_SYN_CHARS	= $BSCSTRM_NO_ADD_LEAD_TX_SYN_CHARS " >> $WAN_CONFIG
		echo "BSCSTRM_NO_BITS_PER_CHAR	= $BSCSTRM_NO_BITS_PER_CHAR" >> $WAN_CONFIG
		echo "BSCSTRM_PARITY	= $BSCSTRM_PARITY" >> $WAN_CONFIG
		echo "BSCSTRM_MISC_CONFIG_OPTIONS	= $BSCSTRM_MISC_CONFIG_OPTIONS" >> $WAN_CONFIG
		echo "BSCSTRM_STATISTICS_OPTIONS	= $BSCSTRM_STATISTICS_OPTIONS" >> $WAN_CONFIG
		echo "BSCSTRM_MODEM_CONFIG_OPTIONS	= $BSCSTRM_MODEM_CONFIG_OPTIONS" >> $WAN_CONFIG
		echo 	>> $WAN_CONFIG
	fi

	if [ $PROTOCOL = WAN_ATM ]; then
		echo 	>> $WAN_CONFIG
		echo "ATM_SYNC_MODE	= $ATM_SYNC_MODE" >> $WAN_CONFIG
		echo "ATM_SYNC_DATA	= $ATM_SYNC_DATA" >> $WAN_CONFIG
		echo "ATM_SYNC_OFFSET	= $ATM_SYNC_OFFSET" >> $WAN_CONFIG
		echo "ATM_HUNT_TIMER	= $ATM_HUNT_TIMER" >> $WAN_CONFIG
		echo 	>> $WAN_CONFIG
		echo "ATM_CELL_CFG	= $ATM_CELL_CFG" >> $WAN_CONFIG
		echo "ATM_CELL_PT	= $ATM_CELL_PT" >> $WAN_CONFIG
		echo "ATM_CELL_CLP	= $ATM_CELL_CLP" >> $WAN_CONFIG
		echo "ATM_CELL_PAYLOAD	= $ATM_CELL_PAYLOAD" >> $WAN_CONFIG
		echo 	>> $WAN_CONFIG
	fi

	if_num=0
	while [ $if_num -ne $NUM_OF_INTER ]; 
	do 
		if_num=$((if_num+1))

		if [ ${OP_MODE[$if_num]} = ANNEXG ]; then
			if [ -f $LAPB_PROFILE_DIR/"lapb."${IF_NAME[$if_num]} ]; then
				echo  >> $WAN_CONFIG
				cat $LAPB_PROFILE_DIR/"lapb."${IF_NAME[$if_num]} >> $WAN_CONFIG
				echo  >> $WAN_CONFIG
			fi
			if [ -f $X25_PROFILE_DIR/"x25."${IF_NAME[$if_num]} ]; then
				echo  >> $WAN_CONFIG
				cat $X25_PROFILE_DIR/"x25."${IF_NAME[$if_num]} >> $WAN_CONFIG
				echo  >> $WAN_CONFIG
			fi
		fi
	done

	#--------- [wp#_<protocol>] area ----------------------------
			
	if_num=0
	while [ $if_num -ne $NUM_OF_INTER ];
	do
		if_num=$((if_num+1))


		TRUE_ENCODING[$if_num]=${TRUE_ENCODING[$if_num]:-NO}

		case  $PROTOCOL in

		WAN_FR)
			echo >> $WAN_CONFIG
			echo "[${IF_NAME[$if_num]}]" >> $WAN_CONFIG
			if [ -z "${CIR[$if_num]}" ]; then
				CIR[$if_num]=NO
			fi
			if [ ${CIR[$if_num]} != NO ]; then
				echo "CIR 		= ${CIR[$if_num]}" >> $WAN_CONFIG
				echo "BC  		= ${BC[$if_num]}" >> $WAN_CONFIG
				echo "BE  		= ${BE[$if_num]}" >> $WAN_CONFIG
			fi
			echo "MULTICAST 	= ${MULTICAST[$if_num]}" >> $WAN_CONFIG
			echo "INARP 		= ${INARP[$if_num]}" >> $WAN_CONFIG
			echo "INARPINTERVAL 	= ${INARP_INT[$if_num]}" >> $WAN_CONFIG
			
			INARP_RX[$if_num]=${INARP_RX[$if_num]:-NO}
			
			echo "INARP_RX	= ${INARP_RX[$if_num]}" >> $WAN_CONFIG
			echo "IPX		= ${IPX[$if_num]}" >> $WAN_CONFIG
			if [ ${IPX[$if_num]} = YES ]; then
				echo "NETWORK		= ${NETWORK[$if_num]}"  >> $WAN_CONFIG
			fi
			echo "TRUE_ENCODING_TYPE	= ${TRUE_ENCODING[$if_num]}" >> $WAN_CONFIG
			;;
		WAN_MFR)
			echo >> $WAN_CONFIG
			echo "[${IF_NAME[$if_num]}]" >> $WAN_CONFIG
			if [ -z "${CIR[$if_num]}" ]; then
				CIR[$if_num]=NO
			fi
			if [ ${CIR[$if_num]} != NO ]; then
				echo "CIR 		= ${CIR[$if_num]}" >> $WAN_CONFIG
				echo "BC  		= ${BC[$if_num]}" >> $WAN_CONFIG
				echo "BE  		= ${BE[$if_num]}" >> $WAN_CONFIG
			fi
			echo "MULTICAST 	= ${MULTICAST[$if_num]}" >> $WAN_CONFIG
			echo "INARP 		= ${INARP[$if_num]}" >> $WAN_CONFIG
			echo "INARPINTERVAL 	= ${INARP_INT[$if_num]}" >> $WAN_CONFIG
			
			INARP_RX[$if_num]=${INARP_RX[$if_num]:-NO}
			
			echo "INARP_RX	= ${INARP_RX[$if_num]}" >> $WAN_CONFIG
			echo "IPX		= ${IPX[$if_num]}" >> $WAN_CONFIG
			if [ ${IPX[$if_num]} = YES ]; then
				echo "NETWORK		= ${NETWORK[$if_num]}"  >> $WAN_CONFIG
			fi
			echo "TRUE_ENCODING_TYPE	= ${TRUE_ENCODING[$if_num]}" >> $WAN_CONFIG
			;;

		WAN_PPP)
			echo >> $WAN_CONFIG

			PAP[$if_num]=${PAP[$if_num]:-NO}
			CHAP[$if_num]=${CHAP[$if_num]:-NO}
			
			echo "[${IF_NAME[$if_num]}]" 		>> $WAN_CONFIG	
			echo "MULTICAST 	= ${MULTICAST[$if_num]}">> $WAN_CONFIG
			echo "PAP       	= ${PAP[$if_num]}" 	>> $WAN_CONFIG
			echo "CHAP		= ${CHAP[$if_num]}" 	>> $WAN_CONFIG
			echo "IPX		= ${IPX[$if_num]}" 	>> $WAN_CONFIG
			if [ ${IPX[$if_num]} = YES ]; then
				echo "NETWORK		= ${NETWORK[$if_num]}"  >> $WAN_CONFIG
			fi

			if [ ${PAP[$if_num]} = YES -o ${CHAP[$if_num]} = YES ]; then
				echo "USERID 	= ${USERID[$if_num]}" 	>> $WAN_CONFIG
				echo "PASSWD   	= ${PASSWD[$if_num]}" 	>> $WAN_CONFIG
				echo "SYSNAME  	= ${SYSNAME[$if_num]}" 	>> $WAN_CONFIG
			fi
			;;

		WAN_ADSL)
			echo >> $WAN_CONFIG
			echo "[${IF_NAME[$if_num]}]" 		>> $WAN_CONFIG
			if [ "$OS_SYSTEM" = Linux ] && [ "$ADSL_EncapMode" = PPP_LLC_OA -o "$ADSL_EncapMode" = PPP_VC_OA ]; then
				PAP[$if_num]=${PAP[$if_num]:-NO}
				CHAP[$if_num]=${CHAP[$if_num]:-NO}

				echo "PAP 		= ${PAP[$if_num]}"		>> $WAN_CONFIG
				echo "CHAP		= ${CHAP[$if_num]}"		>> $WAN_CONFIG
				if [ ${PAP[$if_num]} = YES -o  ${CHAP[$if_num]} = YES ]; then
					echo "USERID    	= ${USERID[$if_num]}"	>> $WAN_CONFIG
					echo "PASSWD		= ${PASSWD[$if_num]}"	>> $WAN_CONFIG
				fi
			fi

			;;

		WAN_ATM)
			echo >> $WAN_CONFIG
			echo "[${IF_NAME[$if_num]}]" 		>> $WAN_CONFIG
			echo "ENCAPMODE	= ${atm_encap_mode[$if_num]}" >> $WAN_CONFIG
			echo "VPI		= ${atm_vpi[$if_num]}"  >> $WAN_CONFIG
			echo "VCI		= ${atm_vci[$if_num]}"  >> $WAN_CONFIG	
			echo >> $WAN_CONFIG
			echo "OAM_LOOPBACK	= ${atm_oam_loopback[$if_num]}" >> $WAN_CONFIG
			echo "OAM_LOOPBACK_INT	= ${atm_oam_loopback_int[$if_num]}" >> $WAN_CONFIG
			echo "OAM_CC_CHECK	= ${atm_oam_cc_check[$if_num]}" >> $WAN_CONFIG
			echo "OAM_CC_CHECK_INT	= ${atm_oam_cc_check_int[$if_num]}" >> $WAN_CONFIG
			echo >> $WAN_CONFIG
			echo "ATMARP		= ${atm_arp[$if_num]}" >> $WAN_CONFIG
			echo "ATMARP_INT	= ${atm_arp_int[$if_num]}" >> $WAN_CONFIG
			;;

		WAN_CHDLC)
			echo >> $WAN_CONFIG
			echo "[${IF_NAME[$if_num]}]" 			>> $WAN_CONFIG	
			echo "MULTICAST		= ${MULTICAST[$if_num]}">> $WAN_CONFIG
			echo "IGNORE_DCD		= ${DCD[$if_num]}" 	>> $WAN_CONFIG
			echo "IGNORE_CTS		= ${CTS[$if_num]}" 	>> $WAN_CONFIG
			echo "IGNORE_KEEPALIVE	= ${KEEP[$if_num]}" 	>> $WAN_CONFIG
			echo "HDLC_STREAMING		= ${STREAM[$if_num]}" 	>> $WAN_CONFIG
			echo "KEEPALIVE_TX_TIMER	= ${TXTIME[$if_num]}" 	>> $WAN_CONFIG
			echo "KEEPALIVE_RX_TIMER	= ${RXTIME[$if_num]}" 	>> $WAN_CONFIG
			echo "KEEPALIVE_ERR_MARGIN	= ${ERR[$if_num]}"	>> $WAN_CONFIG
			echo "SLARP_TIMER		= ${SLARP[$if_num]}"	>> $WAN_CONFIG
			echo "TRUE_ENCODING_TYPE	= ${TRUE_ENCODING[$if_num]}" >> $WAN_CONFIG
			;;
		WAN_MULTP*)
			echo >> $WAN_CONFIG
			echo "[${IF_NAME[$if_num]}]" 			>> $WAN_CONFIG	
			echo "HDLC_STREAMING	= YES" 	>> $WAN_CONFIG
		
			if [ $DEVICE_TYPE = "AFT" ]; then
				echo "ACTIVE_CH	= 1" 	>> $WAN_CONFIG
			else
				MPPP_PROT[$if_num]=${MPPP_PROT[$if_num]:-MP_PPP}
				MPPP_MODEM_IGNORE[$if_num]=${MPPP_MODEM_IGNORE[$if_num]:-YES}
				
				echo "MPPP_PROT	= ${MPPP_PROT[$if_num]}" >>$WAN_CONFIG
				echo "IGNORE_DCD	= ${MPPP_MODEM_IGNORE[$if_num]}" >>$WAN_CONFIG 
				
				echo "IGNORE_CTS	= ${MPPP_MODEM_IGNORE[$if_num]}" >>$WAN_CONFIG


				if [ "$OS_SYSTEM" = Linux ] && [ "${MPPP_PROT[$if_num]}" = MP_PPP ]; then
					PAP[$if_num]=${PAP[$if_num]:-NO}
					CHAP[$if_num]=${CHAP[$if_num]:-NO}

					echo "PAP 		= ${PAP[$if_num]}"		>> $WAN_CONFIG
					echo "CHAP		= ${CHAP[$if_num]}"		>> $WAN_CONFIG
					if [ ${PAP[$if_num]} = YES -o  ${CHAP[$if_num]} = YES ]; then
						echo "USERID    	= ${USERID[$if_num]}"	>> $WAN_CONFIG
						echo "PASSWD		= ${PASSWD[$if_num]}"	>> $WAN_CONFIG
					fi
				fi
			fi

			;;

		WAN_BITSTRM)

			echo >> $WAN_CONFIG
			echo "[${IF_NAME[$if_num]}]" 			>> $WAN_CONFIG
			if [ "$DEVICE_TE1_TYPE" = YES ]; then
				echo "ACTIVE_CH	= ${BSTRM_ACTIVE_CH[$if_num]}"		>> $WAN_CONFIG
			fi
			if [ ${OP_MODE[$if_num]} = SWITCH ]; then
				echo "SW_DEV_NAME	= ${BSTRM_SWITCH_DEV[$if_num]}" >> $WAN_CONFIG
			fi

			if [ ${OP_MODE[$if_num]} = WANPIPE ]; then
				echo "PROTOCOL	= ${MPPP_PROT[$if_num]}" >> $WAN_CONFIG
				echo "IGNORE_DCD	= ${MPPP_MODEM_IGNORE[$if_num]}" >>$WAN_CONFIG 
				echo "IGNORE_CTS	= ${MPPP_MODEM_IGNORE[$if_num]}" >>$WAN_CONFIG
			fi

			if [ ${OP_MODE[$if_num]} = API ] || [ ${OP_MODE[$if_num]} = SWITCH ]; then
				echo "HDLC_STREAMING	= ${STREAM[$if_num]}" 	>> $WAN_CONFIG
				echo "SEVEN_BIT_HDLC	= ${SEVEN_BIT_HDLC[$if_num]}" 	>> $WAN_CONFIG
			fi
			;;

		WAN_AFT*)
			echo >> $WAN_CONFIG
			echo "[${IF_NAME[$if_num]}]" 			>> $WAN_CONFIG
			if [ "$DEVICE_TE1_TYPE" = YES ]; then
				echo "ACTIVE_CH	= ${BSTRM_ACTIVE_CH[$if_num]}"		>> $WAN_CONFIG
			fi

			if [ ${OP_MODE[$if_num]} = WANPIPE ]; then
				echo "PROTOCOL	= ${MPPP_PROT[$if_num]}" >> $WAN_CONFIG

				if [ ${MPPP_PROT[$if_num]} = MP_FR ]; then
				echo "STATION		= ${STATION_IF[$if_num]}" >> $WAN_CONFIG
				echo "SIGNALLING	= ${SIGNAL_IF[$if_num]}" >> $WAN_CONFIG

				fi
				
				echo "IGNORE_DCD	= ${MPPP_MODEM_IGNORE[$if_num]}" >>$WAN_CONFIG 
				echo "IGNORE_CTS	= ${MPPP_MODEM_IGNORE[$if_num]}" >>$WAN_CONFIG
			fi

			echo "HDLC_STREAMING	= ${STREAM[$if_num]}" 	>> $WAN_CONFIG

			if [ ${STREAM[$if_num]} = NO ]; then

				IDLE_FLAG[$if_num]=${IDLE_FLAG[$if_num]:-0}
				IFACE_MTU[$if_num]=${IFACE_MTU[$if_num]:-$MTU}
				IFACE_MRU[$if_num]=${IFACE_MRU[$if_num]:-$MTU}
			
				echo "IDLE_FLAG	= ${IDLE_FLAG[$if_num]}" >> $WAN_CONFIG
				echo "MTU		= ${IFACE_MTU[$if_num]}" >> $WAN_CONFIG
				echo "MRU		= ${IFACE_MRU[$if_num]}" >> $WAN_CONFIG
			fi
			;;

		WAN_ADCCP)
			echo >> $WAN_CONFIG
			echo "[${IF_NAME[$if_num]}]" 		>> $WAN_CONFIG	
			;;

		WAN_X25)
			echo >> $WAN_CONFIG
			echo "[${IF_NAME[$if_num]}]" 		>> $WAN_CONFIG	
			if [ ${OP_MODE[$if_num]} = WANPIPE ] && [ ${CH_TYPE[$if_num]} = SVC ]; then
				echo "IdleTimeout 	= ${IDLE_TIMEOUT[$if_num]}" >> $WAN_CONFIG
				echo "HoldTimeout 	= ${HOLD_TIMEOUT[$if_num]}" >> $WAN_CONFIG

				if [ ! -z ${SRC_ADDR[$if_num]} ]; then
				echo "X25_SRC_ADDR      = ${SRC_ADDR[$if_num]}" >> $WAN_CONFIG
				else
				echo "#X25_SRC_ADDR      =" >> $WAN_CONFIG
				fi
		
				if [ ! -z "${X25_ACC_DST_ADDR[$if_num]}" ]; then
				echo "X25_ACCEPT_DST_ADDR = ${X25_ACC_DST_ADDR[$if_num]}" >> $WAN_CONFIG
				else
				echo "#X25_ACCEPT_DST_ADDR =" >> $WAN_CONFIG
				fi
				
				if [ ! -z "${X25_ACC_SRC_ADDR[$if_num]}" ]; then
				echo "X25_ACCEPT_SRC_ADDR = ${X25_ACC_SRC_ADDR[$if_num]}" >> $WAN_CONFIG
				else
				echo "#X25_ACCEPT_SRC_ADDR =" >> $WAN_CONFIG
				fi
				
				if [ ! -z "${X25_ACC_USR_DATA[$if_num]}" ]; then
				echo "X25_ACCEPT_USR_DATA = ${X25_ACC_USR_DATA[$if_num]}" >> $WAN_CONFIG
				else
				echo "#X25_ACCEPT_USR_DATA =" >> $WAN_CONFIG
				fi
			fi
			;;
		*)
			;;
		esac


		DYNAMIC_INTR[$if_num]=${DYNAMIC_INTR[$if_num]:-NO}
		DEFAULT_IP[$if_num]=${DEFAULT_IP[$if_num]:-NO}

		if [ ${OP_MODE[$if_num]} = WANPIPE -a $PROTOCOL != WAN_X25 ]; then 
			echo "DYN_INTR_CFG	= ${DYNAMIC_INTR[$if_num]}"  >> $WAN_CONFIG	
		fi

		if [ ${DEFAULT_IP[$if_num]} = YES -a $PROTOCOL != WAN_X25 ]; then
			echo "GATEWAY		= YES"		    >> $WAN_CONFIG
		fi
	
	done		

	if_num=0
	while [ $if_num -ne $NUM_OF_INTER ];
	do
		if_num=$((if_num+1))

		if [ ${OP_MODE[$if_num]} = API ] || [ ${OP_MODE[$if_num]} = BRIDGE ]; then
			continue
		fi	

		if [ ${OP_MODE[$if_num]} = SWITCH ]; then
			continue
		fi	

		if [ ${OP_MODE[$if_num]} = "BRIDGE_NODE" ]; then
			R_IP[$if_num]=" "
		fi

		date=`date`
	
		if [ $LINUX_DISTR = debian ]; then
	
			cat <<EOM > $INTERFACE_DIR/"$NEW_IF"${IF_NAME[$if_num]}
# Wanrouter interface configuration file
# name:	${IF_NAME[$if_num]}
# date:	$date
#
iface ${IF_NAME[$if_num]} inet static
	address ${L_IP[$if_num]}
	netmask ${NMSK_IP[$if_num]}
	pointopoint ${R_IP[$if_num]}
EOM

			if [ ${DEFAULT_IP[$if_num]} = YES ]; then
				cat <<EOM >> $INTERFACE_DIR/"$NEW_IF"${IF_NAME[$if_num]}
	gateway ${GATE_IP[$if_num]}
EOM
			fi

		else
		
		# Create interface directory if it didn't exists
		[ ! -d $INTERFACE_DIR ] && mkdir -p $INTERFACE_DIR

		cat <<EOM > $INTERFACE_DIR/"$NEW_IF"${IF_NAME[$if_num]}
# Wanrouter interface configuration file
# name:	${IF_NAME[$if_num]}
# date:	$date
#
DEVICE=${IF_NAME[$if_num]}
IPADDR=${L_IP[$if_num]}
NETMASK=${NMSK_IP[$if_num]}
POINTOPOINT=${R_IP[$if_num]}
ONBOOT=yes
EOM
		
			if [ ${DEFAULT_IP[$if_num]} = YES ]; then
				cat <<EOM >> $INTERFACE_DIR/"$NEW_IF"${IF_NAME[$if_num]}
GATEWAY=${GATE_IP[$if_num]}
EOM
			fi

		
		fi
	done
}


auto_intr_config ()
{
	local num
	local name

	while true
	do
		interface_menu 1
		rc=$?
		if [ $rc -eq 1 ]; then
			break;
		fi
	done

	PROT_STATUS[1]="Setup Done"
	INTR_STATUS[1]="setup"

	#ALEX: ADD FREEBSD STUFF 
  	#

	case $PROTOCOL in
	WAN_FR)
		name=$name_prefix"fr"
		;;
	WAN_MFR)
		name=$name_prefix"mfr"
		;;
	
	WAN_X25)
		if [ ${CH_TYPE[1]} = SVC ]; then
			name=$name_prefix"svc"
		else
			name=$name_prefix"pvc"
		fi
		;;
	esac

	num=1;
	while [ $num -ne $NUM_OF_INTER ];
	do
		num=$((num+1))
	
		OP_MODE[$num]=${OP_MODE[1]}
		PROT_STATUS[$num]=${PROT_STATUS[1]}
		IP_STATUS[$num]=${IP_STATUS[1]}
		INTR_STATUS[$num]=${INTR_STATUS[1]}
			
		case $PROTOCOL in

		WAN_FR)	
			DLCI_NUM[$num]=$((DLCI_NUM[1]+$num-1))	
			name=$name_prefix"fr"${DLCI_NUM[$num]}
			IF_NAME[$num]=$name
			CIR[$num]=${CIR[1]}
			BC[$num]=${BC[1]}
			BE[$num]=${BE[1]}
			MULTICAST[$num]=${MULTICAST[1]}	
			INARP[$num]=${INARP[1]}
			INARP_INT[$num]=${INARP_INT[1]}
			INARP_RX[$num]=${INAR_RX[1]}
			;;

		WAN_MFR)	
			DLCI_NUM[$num]=$((DLCI_NUM[1]+$num-1))	
			name=$name_prefix"mfr"${DLCI_NUM[$num]}
			IF_NAME[$num]=$name
			CIR[$num]=${CIR[1]}
			BC[$num]=${BC[1]}
			BE[$num]=${BE[1]}
			MULTICAST[$num]=${MULTICAST[1]}	
			INARP[$num]=${INARP[1]}
			INARP_INT[$num]=${INARP_INT[1]}
			INARP_RX[$num]=${INAR_RX[1]}

			if [ ${OP_MODE[1]} = ANNEXG -a -d "$ANNEXG_DIR" ]; then
				if [ -f "$ANNEXG_DIR/${IF_NAME[1]}.lapb" ]; then	
				sed s/fr.*lapb.*=/fr${DLCI_NUM[$num]}lapb\ =/ $ANNEXG_DIR/${IF_NAME[1]}".lapb" > $ANNEXG_DIR/${IF_NAME[$num]}".lapb"

				fi	
				if [ -f "$ANNEXG_DIR/${IF_NAME[1]}.x25" ]; then
				echo "Writing a file $ANNEXG_DIR/${IF_NAME[$num]}.x25"
				sed s/f.*s/f${DLCI_NUM[$num]}s/ $ANNEXG_DIR/${IF_NAME[1]}".x25" > $ANNEXG_DIR/${IF_NAME[$num]}".x25"
				fi
			fi

			;;

		WAN_X25)
			IF_NAME[$num]=$name$num
			IDLE_TIMEOUT[$num]=${IDLE_TIMEOUT[1]}
			HOLD_TIMEOUT[$num]=${HOLD_TIMEOUT[1]}
			SRC_ADDR[$num]=${SRC_ADDR[1]}
			X25_ACCEPT_CALLS_FROM[$num]=${X25_ACCEPT_CALLS_FROM[1]}
			if [ ${CH_TYPE[1]} = PVC ]; then
				X25_ADDR[$num]=$((X25_ADDR[1]+$num-1))
			else
				X25_ADDR[$num]=${X25_ADDR[1]}
			fi
			CH_TYPE[$num]=${CH_TYPE[1]}
			;;
		esac	
		
	done 
}



function configure_scripts()
{
	local dev=$1
	local ifname=$2
	local input=""
	local start_template=$WAN_SCRIPTS_DIR/start.$$
	local stop_template=$WAN_SCRIPTS_DIR/stop.$$
	
	if [ "$ifname" != "" ]; then
		start_file="$WAN_SCRIPTS_DIR/$dev-$ifname-start"
		stop_file="$WAN_SCRIPTS_DIR/$dev-$ifname-stop"
		cat << EOM > $start_template
#!/bin/sh
#
# WANPIPE Interface ($ifname) Start Script
#
# Description:
# 	This script is called by ${WAN_BIN_DIR}/wanrouter
#       after the interface has been started using
#       ifconfig.
#
#       Use this script to add extra routes or start
#       services that relate directly to this
#       particular interafce ($ifname)
#
# Arguments:  
#       $1 = wanpipe device name    (wanpipe1)
#       $2 = wanpipe interface name (wan0)
#
# Dynamic Mode:
#       This script will be called by the 
#       wanpipe kernel driver any time the 
#       state of the interface changes.
#
#       Eg: If interface goes up and down,
#           or changes IP addresses.
#  
#       Wanpipe Syncppp Driver will call this
#       script when IPCP negotiates new IP
#       addresses
#
# Dynamic Environment Variables:
#       These variables are available only when
#       the script is called via wanpipe kernel
#       driver.  
#
#       $WAN_ACTION
#       $WAN_DEVICE
#       $WAN_INTERFACE
#
EOM


		cat << EOM > $stop_template
#!/bin/sh
#
# WANPIPE Interface ($ifname) Stop Script
#
# Description:
# 	This script is called by ${WAN_BIN_DIR}/wanrouter
#       after the interface has been stopped using
#       ifconfig.
#
#       Use this script to remove routes or stop
#       services that relate directly to this
#       particular interafce ($ifname)
#
# Arguments:  
#       $1 = wanpipe device name    (wanpipe1)
#       $2 = wanpipe interface name (wan0)
#

EOM

	elif [ "$dev" != "" ]; then
		start_file="$WAN_SCRIPTS_DIR/$dev-start"
		stop_file="$WAN_SCRIPTS_DIR/$dev-stop"

		cat << EOM > $start_template
#!/bin/sh
#
# WANPIPE Device ($dev) Start Script
#
# Description:
# 	This script is called by ${WAN_BIN_DIR}/wanrouter
#       after the device $dev has been started.
#
#       Use this script to add extra routes or start
#       services that relate directly to this
#       particular device ($dev).
#
# Arguments:  
#       $1 = wanpipe device name    (wanpipe1)
#

EOM


		cat << EOM > $stop_template
#!/bin/sh
# WANPIPE Device ($dev) Start Script
#
# Description:
# 	This script is called by ${WAN_BIN_DIR}/wanrouter
#       after the device $dev has been stopped.
#
#       Use this script to remove extra routes or stop
#       services that relate directly to this
#       particular device ($dev).
#
# Arguments:  
#       $1 = wanpipe device name    (wanpipe1)
#

EOM


	else
		start_file="$WAN_SCRIPTS_DIR/start"
		stop_file="$WAN_SCRIPTS_DIR/stop"

		cat << EOM > $start_template
#!/bin/sh
#
# WANPIPE Global Device Start Script
#
# Description:
# 	This script is called by ${WAN_BIN_DIR}/wanrouter
#       after all wanpipe devices in $PROD_HOME/wanrouter.rc
#       have been started.
#
#       Use this script to add extra routes or start
#       services that relate to all wanpipe devices.
#
# Arguments: None 
#

EOM


		cat << EOM > $stop_template
#!/bin/sh
#
# WANPIPE Global Device Stop Script
#
# Description:
# 	This script is called by ${WAN_BIN_DIR}/wanrouter
#       after all wanpipe devices in $PROD_HOME/wanrouter.rc
#       have been stopped.
#
#       Use this script to remove extra routes or stop
#       services that relate to all wanpipe devices.
#
# Arguments: None
#

EOM

	fi

	menu_options="'START' 'Configure START Script' \
		      'STOP'  'Configure STOP  Script' 2> $MENU_TMP"

	menu_instr="Wanpipe Script Configuraiton \
									"

	menu_name "WANPIPE SCRIPT CONFIG" "$menu_options" 2 "$menu_instr" "$BACK"
	rc=$?

	choice=`cat $MENU_TMP`
	rm -f $MENU_TMP

	case $rc in
		0) 	
			if [ $choice = START ]; then

				if [ ! -f $start_file ]; then
					cat $start_template > $start_file
				fi
			
				eval "vi $start_file"
			else

				if [ ! -f $stop_file ]; then
					cat $stop_template > $stop_file
				fi
				
				eval "vi $stop_file"
			fi
			
			configure_scripts $1 $2
			;;
		2)	
			configure_scripts $1 $2
			;;

		*) 	
			;;
	esac


	rm -f $stop_template 
	rm -f $start_template

	return 0
}
