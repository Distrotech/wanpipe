#!/bin/sh


#################################################################
########		L I N U X			#########
#################################################################
init_pppd_scripts ()
{
	if [ $OS_SYSTEM != "Linux" ]; then
		return 1;
	fi 


	if [ $PROTOCOL = WAN_ADSL ]; then

		get_string "Please specify PAP/CHAP User Name" ""  
		PAPCHAP_UNAME=$($GET_RC)

		get_string "Please specify PAP/CHAP Password" ""  
		PAPCHAP_PASSWD=$($GET_RC)

		warning "PAPCHAP_SECRETS_UPDATE"
		if [ $? -eq 0 ]; then
	
			if [ ! -d /etc/ppp/ ]; then
				warning "PPP_DIR_NOT_FOUND"
				if [ $? -eq 0 ]; then
					\mkdir -p /etc/ppp
				else
					return 1;
				fi	
			fi
		
			if [ -e /etc/ppp/pap-secrets ]; then
				mv /etc/ppp/pap-secrets /etc/ppp/pap-secrets.orig
			fi
			if [ -e /etc/ppp/chap-secrets ]; then
				mv /etc/ppp/chap-secrets /etc/ppp/chap-secrets.orig
			fi

			echo -e "\"$PAPCHAP_UNAME\"\t*\t$PAPCHAP_PASSWD\t*" > /etc/ppp/pap-secrets
			chmod 600 /etc/ppp/pap-secrets
			echo -e "\"$PAPCHAP_UNAME\"\t*\t$PAPCHAP_PASSWD\t*" > /etc/ppp/chap-secrets
			chmod 600 /etc/ppp/chap-secrets
		fi
		
	PPPD_OPTION="
# ---------------------------------------------
# PPPD Daemon SYNC configuration file
# ---------------------------------------------
# options.ttyWP$TTY_MINOR
#
#  - PPPD daemon must bind to device /dev/ttyWP$TTY_MINOR
#  - The /dev/ttyWP$TTY_MINOR is binded to wanpipe$DEVICE_NUM
#
# Created automatically by /usr/sbin/wancfg
# Date: `date`
#
name \"$PAPCHAP_UNAME\"
debug 

-detach

noipdefault
defaultroute
ipcp-accept-remote
ipcp-accept-local
nobsdcomp
nodeflate
nopcomp
novj
novjccomp
noaccomp -am
"

	PPPD_CALL="
#------------------------------------------------
# PPPD Call SYNC Script
#------------------------------------------------
# This call script will call /etc/ppp/options.ttyWP$TTY_MINOR
# to setup the PPPD dameon.
#
# It will start PPPD SYNC protocol.
#
# Created automatically by /usr/local/wanrouter/wancfg
# Date: `date`
#
ttyWP$TTY_MINOR		#Bind to WANPIPE device wanpipe3
sync			#Connect in sync mode
lcp-echo-failure 0	#No link verification messages
"
		return 0;
	fi


	MULTILINK=NO
	MULTILINK_MODE=SLAVE

	warning "MULTILINK_OPT"
	if [ $? -eq 0 ]; then
		MULTILINK=YES

		warning "MULTILINK_MASTER"
		if [ $? -eq 0 ]; then
			MULTILINK_MODE=MASTER
		fi
	fi

	if [ $TTY_MODE = Sync ]; then

	PPPD_OPTION="
# ---------------------------------------------
# PPPD Daemon SYNC configuration file
# ---------------------------------------------
# options.ttyWP$TTY_MINOR
#
#  - PPPD daemon must bind to device /dev/ttyWP$TTY_MINOR
#  - The /dev/ttyWP$TTY_MINOR is binded to wanpipe$DEVICE_NUM
#
# Created automatically by /usr/sbin/wancfg
# Date: `date`
#
asyncmap 0

debug		#PPPD will output connection information

nodetach	#Do not detach into background

defaultroute	#This interface will be a default gateway.

nodeflate	#Do not use compression
nobsdcomp	#Do not use bsd compression
noccp		#Disable CCP (Compression Control Protocol)
noaccomp	#Disable  Address/Control compression
nopcomp		#Disable  protocol  field compres.
novj		#Disable Van Jacobson TCP/IP header compres.
novjccomp	#Disable the connection-ID compression
nopredictor1	#Do not accept or agree to Predictor-1 compres.


# Read pppd(8) for more information.
# Read /usr/local/wanrouter/PPP-HOWTO for more info.
"

	PPPD_CALL="
#------------------------------------------------
# PPPD Call SYNC Script
#------------------------------------------------
# This call script will call /etc/ppp/options.ttyWP$TTY_MINOR
# to setup the PPPD dameon.
#
# It will start PPPD SYNC protocol.
#
# Created automatically by /usr/local/wanrouter/wancfg
# Date: `date`
#
ttyWP$TTY_MINOR		#Bind to WANPIPE device wanpipe$DEVICE_NUM
sync		#Connect in sync mode
lcp-echo-failure 0	#No link verification messages
"

	if [ $MULTILINK = YES ]; then
		PPPD_CALL=$PPPD_CALL"
multilink	#Enable Multilink Protocol
"

		if [ $MULTILINK_MODE = SLAVE ]; then
			PPPD_CALL=$PPPD_CALL"
noip 		#No IP addresses for a slave
"
		fi

	fi

	if [ $MULTILINK = NO ] || [ $MULTILINK_MODE = MASTER ]; then

		get_string "Please specify the Local IP Address" "" "IP_ADDR" 
		L_IP=$($GET_RC)

		get_string "Please specify the Point-to-Point IP Address" "" "IP_ADDR" 
		R_IP=$($GET_RC)

		PPPD_CALL=$PPPD_CALL"
$L_IP:$R_IP	#Local and remote IP addresses
			#STATIC IP mode.
"
	fi

else

	PPPD_OPTION="
# ---------------------------------------------
# PPPD Daemon ASYNC configuration file
# ---------------------------------------------
# options.ttyWP$TTY_MINOR
#
#  - PPPD daemon must bind to device /dev/ttyWP$TTY_MINOR
#  - The /dev/ttyWP$TTY_MINOR is binded to wanpipe$DEVICE_NUM
#
# Created automatically by /usr/local/wanrouter/wancfg
# Date: `date`
#
asyncmap 0

modem		#Interface to a modem

debug		#PPPD will output connection information

crtscts		#Enable hardware flow control

noipdefault	#Enable dynamice IP addressing
	        #Obtaint Local and Remote IP addresses from
		#peer.

-detach		#Do not detach into background

defaultroute	#This interface will be a default gateway.

#nodeflate	#Do not use compression
#nobsdcomp	#Do not use bsd compression
#noccp		#Disable CCP (Compression Control Protocol)
#noaccomp	#Disable  Address/Control compression
#nopcomp		#Disable  protocol  field compres.
#novj		#Disable Van Jacobson TCP/IP header compres.
#novjccomp	#Disable the connection-ID compression
#nopredictor1	#Do not accept or agree to Predictor-1 compres.


persist		#If the link goes down, keep retrying to connect 



# Read pppd(8) for more information.
# Read /usr/local/wanrouter/PPP-HOWTO for more info.
"

	get_integer "Please specify the Baud Rate in bps" "38400" "64" "4096000" "device_setup_help get_baudrate"
	BAUDRATE=$($GET_RC)

	PPPD_CALL="
#------------------------------------------------
# PPPD Call ASYNC Script
#------------------------------------------------
# This call script will call /etc/ppp/options.ttyWP$TTY_MINOR
# to setup the PPPD dameon.
#
# It will start PPPD ASYNC protocol.
#
# Created automatically by /usr/local/wanrouter/wancfg
# Date: `date`
#
ttyWP$TTY_MINOR		#Bind to WANPIPE device wanpipe$DEVICE_NUM
$BAUDRATE		#Baud rate of the PPP link
connect '/etc/ppp/redialer'
"

	if [ $MULTILINK = YES ]; then
		PPPD_CALL=$PPPD_CALL"
multilink	#Enable Multilink Protocol
"

		if [ $MULTILINK_MODE = SLAVE ]; then
			PPPD_CALL=$PPPD_CALL"
noip 		#No IP addresses for a slave
"
		fi

	fi
fi

	return 0;
}

function create_multilink_ppp_linux_cfg() {

	if [ ! -e "/dev/ttyWP$TTY_MINOR" ]; then
		warning "CREATE_TTYDEV"
		if [ $? -eq 0 ]; then
			eval "mknod -m 666 /dev/ttyWP$TTY_MINOR c 240 $TTY_MINOR 2> /dev/null"
			if [ $? -eq 0 ]; then 
				error "CREATE_TTYOK"
			else	
				error "CREATE_TTYFAIL"
			fi
		fi
	fi

	warning "CREATE_PPPDOPT"
	if [ $? -eq 0 ]; then
		if [ -f "/etc/ppp/options.ttyWP$TTY_MINOR" ]; then
			mv /etc/ppp/options.ttyWP$TTY_MINOR /etc/ppp/options.ttyWP$TTY_MINOR.old
		fi

		if [ "$TTY_MODE" = Sync ] || [ $PROTOCOL = WAN_ADSL ]; then
			init_pppd_scripts
			if [ $? -eq 0 ]; then
				echo "$PPPD_OPTION" > /etc/ppp/options.ttyWP$TTY_MINOR
				echo "$PPPD_CALL"   > /etc/ppp/peers/isp_wanpipe$DEVICE_NUM
			fi
		else
			eval "$PPPCONFIG --ttyport /dev/ttyWP$TTY_MINOR --provider isp_wanpipe$DEVICE_NUM"
		fi

		error "CREATE_PPPDOK"
	fi

}

#################################################################
########	F R E E B S D / O P E N B S D		#########
#################################################################
MPD_CONF_DIR="/usr/local/etc/mpd"
MPD_CONF="mpd.conf"
MPD_LINKS="mpd.links"
MPD_SECRET="mpd.secret"
MPD_SCRIPT="mpd.script"

# Function: parse_mpd_conf_file
# $1 - bundle name
# $2 - link name
# $3 - auto bundle loading (YES|NO)
function parse_mpd_conf_file() {

	local bundle_config=$1_config
	local bundle_configured=0
	local bundle_configuring=0
	local new_cmd_bundle=0
	local set_iface_addr_cmd=0
	local lip
	local rip

	if [ ! -f $MPD_CONF_DIR/$MPD_CONF ]; then
		cat << EOM >> $MPD_CONF_DIR/$MPD_CONF
#################################################################
#
#	MPD configuration file
#
# This file defines the configuration for mpd: what the
# bundles are, what the links are in those bundles, how
# the interface should be configured, various PPP parameters,
# etc. It contains commands just as you would type them
# in at the console. A blank line ends an entry. Lines
# starting with a "#" are comments and get completely
# ignored.
#
# $Id: mlinkppp_lib.sh,v 1.2 2004-01-28 17:14:48 sangoma Exp $
#
#################################################################

EOM
	fi

	while read line
	do
		if [ $bundle_configuring -eq 0 ]; then
			if [ "$line" = "$bundle_config:" ]; then
				# Bundle already configured
				bundle_configured=1
				bundle_configuring=1
				echo "$line" >> tmp.$$
			else
				cmd=`echo $line | cut -d' ' -f1`
				case $cmd in
				[#]*)
					echo "$line" >> tmp.$$
					;;
				[a-zA-Z0-9]*:)
					echo "$line" >> tmp.$$
					;;
				*)
					echo "    $line" >> tmp.$$
					;;
				esac
			fi
		else
			cmd=`echo $line | cut -d' ' -f1`
			case $cmd in
			[#]*)
				echo "$line" >> tmp.$$	
				;;
			[a-zA-Z0-9]*:)
				# End of bundle configuration
				bundle_configuring=0
				echo "    $line" >> tmp.$$	
				;;
			new*)
				new_cmd_bundle=1
				line1=${line/$2/""}
				if [ "$line1" = "$line" ]; then
					echo "    $line $2" >> tmp.$$
				else
					echo "    $line" >> tmp.$$
				fi
				;;
			*)
				echo "    $line" >> tmp.$$	
				;;
			esac
		fi


	done < $MPD_CONF_DIR/$MPD_CONF

	if [ $bundle_configured -eq 0 ]; then
		get_string "Please specify the Local IP Address for the Bundle $1" "" "IP_ADDR" 
		lip=$($GET_RC)
		get_string "Please specify the Point-to-Point IP Address for the Bundle $1" "" "IP_ADDR"
		rip=$($GET_RC)
		cat << EOM >> tmp.$$


$bundle_config:
    new $1 $2
    set iface addrs $lip $rip
    open iface
EOM
	fi
	mv tmp.$$ $MPD_CONF_DIR/$MPD_CONF
}

# Function: parse_mpd_links_file
# $1 - link name
# $2 - device number
function parse_mpd_links_file() {

	local nodename=wanpipe$2
	local link_configured=0
	local link_configuring=0

	if [ ! -f $MPD_CONF_DIR/$MPD_LINKS ]; then
		cat << EOM > $MPD_CONF_DIR/$MPD_LINKS
#################################################################
#
#	MPD links file
#
# In this file you define the various "links" that comprise
# a bundle. Each link corresponds to a single serial device.
# These are commands that could be typed into the console directly.
#
# This file should only contain configuration for a link if
# that configuration is specific to that particular link. That
# is, things like device name and bandwidth. Other generic link
# options like LCP parameters belong in "mpd.conf".
#
# The first command for each link should be "set link type ..."
#
# $Id: mlinkppp_lib.sh,v 1.2 2004-01-28 17:14:48 sangoma Exp $
#
#################################################################


EOM
	fi

	while read line
	do
		if [ $link_configuring -eq 0 ]; then
			if [ "$line" = "$1:" ]; then
				# Bundle already configured
				link_configured=1
				link_configuring=1
				echo "$line" >> tmp.$$
			else
				cmd=`echo $line | cut -d' ' -f1`
				case $cmd in
				[#]*)
					echo "$line" >> tmp.$$
					;;
				[a-zA-Z0-9]*:)
					echo "$line" >> tmp.$$
					;;
				*)
					echo "    $line" >> tmp.$$
					;;
				esac
			fi
		else
			cmd=`echo $line | cut -d' ' -f1`
			case $cmd in
			[#]*)
				echo "$line" >> tmp.$$
				;;				
			[a-zA-Z0-9]*:)
				# End of links configuration
				link_configuring=0
				echo "$line" >> tmp.$$
				;;				
			*)	
				line1=${line#set[ ]*ng[ ]*node[ ]*}
				if [ "$line1" != "$line" ]; then
					line2=${line1/$nodename:/""}
					if [ "$line1" = "$line2" ]; then
						echo "    set ng node $nodename:" >> tmp.$$
					else
						echo "    $line" >> tmp.$$
					fi
				else
					echo "    $line" >> tmp.$$
				fi
			esac
		fi

	done < $MPD_CONF_DIR/$MPD_LINKS

	if [ $link_configured -eq 0 ]; then
		cat << EOM >> tmp.$$

$1:
    set link type ng
    set ng node $nodename:
    set ng hook rawdata
EOM
	fi

	mv tmp.$$ $MPD_CONF_DIR/$MPD_LINKS
}


# Function: create_multilink_ppp_bsd_cfg
# $1 - device number
function create_multilink_ppp_bsd_cfg() {

	local auto_bundle_load
	local bundle_name
	local link_name
	local lip
	local rip

	get_string "Please specify the MPD configuration directory" "$MPD_CONF_DIR"
	MPD_CONF_DIR=$($GET_RC)

	# Make current bundle default 
	#warning mlinkppp_bundle
	#if [ $? -eq 0 ]; then
	#	auto_bundle_load=YES
	#else
	#	auto_bundle_load=NO
	#fi

	get_string "Please specify Bundle Name" ""
	bundle_name=$($GET_RC)

	get_string "Please specify Link Name" ""
	link_name=$($GET_RC)

	parse_mpd_conf_file $bundle_name $link_name $auto_bundle_load
	parse_mpd_links_file $link_name $1

	# Create security file

	return 0
}


function create_multilink_ppp_cfg() {

	local err

	if [ $OS_SYSTEM = "Linux" ]; then
		create_multilink_ppp_linux_cfg $1
	elif [ $OS_SYSTEM = "FreeBSD" ]; then
		create_multilink_ppp_bsd_cfg $1
	fi
	
	return 0
}



