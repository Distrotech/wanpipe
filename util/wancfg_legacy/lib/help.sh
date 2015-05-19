#!/bin/sh

#============================================================
# start_menu_help
#
#
#
#============================================================

function start_menu_help () {


	cat <<EOM > $TEMP
	CONFIGURATION OPTIONS

Create a new Configuration File
-------------------------------
	Creates a new wanpipe#.conf file from 
scratch. 

Edit existing Configuration File
--------------------------------
	Loads already existing configuration
file for  reconfiguration.

EOM
	text_area 

}

#============================================================
# create_menu_help 
#
#
#
#============================================================


function create_menu_help () {

	local opt=$1

	case $opt in

	select_protocol)

	cat <<EOM > $TEMP

	PROTOCOL DEFINITION	

Select one of the supported WAN protocols.

Options: Frame Relay, Cisco HDLC, PPP and X25

Warning: If you change the protocol, the current 
         configuration will be reset.

EOM
	;;

	device_setup)
	
	cat <<EOM >$TEMP

	HARDWARE CONFIGURATION

Setup and configure hardware parameters for the
Sangoma Wanpipe(tm) card.

EOM
	;;

	define_interface)
	
	cat <<EOM >$TEMP

	NETWORK INTERFACE/PROTOCOL CONFIGURATION

Define and configure network interfaces and 
interface specific protocol options for the 
Sangoma Wanpipe(tm) drivers.	


EOM
	;;

	manual)

	cat <<EOM >$TEMP

	MANUAL API INTERFACE CONFIGURATION

Manual configuration is necessary for all
non API interfaces.

However, if all interfaces are being configured 
for API than Auto Configuration option 
can be used.
EOM
	;;
	auto_intr_config)

	cat <<EOM >$TEMP

	AUTOMATIC API INTERFACE CONFIGURATION

Configure a single API interface, and the rest of the 
interfaces will be configured automatically.  API 
interface options are usually same for all 
interfaces, thus the automatic method will save 
time in configuring large number of API interfaces.

NOTE: Auto Configuration option will not work
      for interfaces that are not configured for
      API.
EOM
	;;

	
	
	*)
		return
	;;
	esac
	
	text_area 20 60  

}

#============================================================
# select_protocol_help  
#
#
#
#============================================================

function select_protocol_help () {

	local opt=$1

	case $opt in

	WAN_FR)
		cat <<EOM >$TEMP

	FRAME RELAY PROTOCOL

Select this option if you are planning to connect 
a WANPIPE card to a frame relay network, or use frame 
relay API to develop custom applications over the 
Frame Relay protocol. 

This feature also contains the Ethernet Bridging 
over Frame Relay, where a WANPIPE frame relay link can 
be directly connected to the Linux kernel bridge. 

The Frame Relay option is supported on PRIMARY port
ONLY on S514-PCI and S508-ISA cards.

Consult your ISP provider on which protocol
to use.

EOM
		;;

	WAN_MFR)
		cat <<EOM >$TEMP

	MULTI-PORT FRAME RELAY PROTOCOL

Select this option if you are planning to 
connect a WANPIPE card to a leased line using 
Frame Relay protocol. 

The MultiPort Frame Relay uses the Kernel 
Frame Relay stack protocol over the Sangoma 
HDLC Streaming adapter.  
Using MultiPort Frame Relay each Sangoma 
adapter port is capable of supporting an 
independent Frame Relay connection.

For example, a single Quad-Port PCI adapter can 
support up to four independent Frame Relay links. 

IMPORTANT:
  This option should only be used if Frame Relay 
  MUST run on both PRIMARY and SECONDARY port 
  of the Sangoma  adapter.  Otherwise, it is 
  recommended that standard WANPIPE FR is used.

Consult your ISP provider on which protocol
to use.

EOM
		;;	

	WAN_ATM)
		cat <<EOM >$TEMP

	ATM PROTOCOL

Select this option if you are connecting
to an ATM network via Sangoma S series 
S514/S508 cards.

ATM Protocols: 
	Ethernet over ATM (LLC/VC) (PPPoE)
	Classical IP over ATM (LLC/VC)
	
Consult your ISP provider on which protocol
to use.

EOM
		;;


	WAN_ADSL)
		cat <<EOM >$TEMP

	ADSL PROTOCOL

Select this option if you are using a
Sangoma S518 ADSL card.  

ATM Protocols: 
	Ethernet over ATM (LLC/VC) (PPPoE)
	Classical IP over ATM (LLC/VC)
	PPP over ATM (LLC/VC)
	
DSL Standards: T.413, G.DMT, G.lite ...

Consult your ISP provider on which protocol
to use.

EOM
		;;

	WAN_BITSTRM)
		cat <<EOM >$TEMP
	
	BITSTREAMING PROTOCOL

Support for Bitstream Protocol over S514/S508 cards. 
Bitstream protocol can Tx/Rx data over individual 
T1/E1 DS0's as well as Tx/Rx raw bit streams.

The bitstreaming protocol can act as a drop
and insert, i.e. it can split the T1/E1 DSO's
into separate data channels that can be used
independently to tx/rx data.

Moreover, one can setup IP,PPP or CHDLC over
each DSO or combinations of DSOs.

Supports: S514-PCI and S508-ISA cards

IMPORTANT:
This protocol is not installed by default, to
install:	
	./Setup install --protocol=BITSTRM
	

EOM
		;;

	WAN_BSC)
		cat <<EOM >$TEMP

	MULTIPOINT BISYNC PROTOCOL

Support for Multi-Point Bisync protocol API.
Used to develop custom applications over
standard BiSync protocol.

Supports: S514-PCI and S508-ISA cards

IMPORTANT:
This protocol is not installed by default, to
install:	
	./Setup install --protocol=BISYNC


EOM
		;;

	WAN_BSCSTRM)
		cat <<EOM >$TEMP

	BISYNC STREAMING PROTOCOL

Support for Bisync streaming protocol API.
Used to develop custom applications over
custom BiSync protocols, such as NASDAQ 
or bisync feeds.

Supports: S514-PCI and S508-ISA cards

IMPORTANT:
This protocol is not installed by default, to
install:	
	./Setup install --protocol=BSCSTRM

EOM
		;;

	WAN_SS7)
	cat <<EOM >$TEMP

	SS7 Layer 2 API PROTOCOL

Support for SS7 Layer 2 API protocol.
Used to develop custom applications over
Layer 2 SS7.

Supports: S514-PCI and S508-ISA cards

IMPORTANT:
This protocol is not installed by default, to
install:	
	./Setup install --protocol=SS7

EOM
		;;

	WAN_POS)
	cat <<EOM >$TEMP

	POS (Point of Sale) API PROTOCOL

Support for POS (S515/S508) hardware and
custom POS API protocols.

Pointo of Sale protocols include 
IBM 4860, 
NCR 2126, 
NCR 2127, 
NCR 1255, 
NCR 7000,
ICL

Supports: S515-PCI and S509-ISA cards

IMPORTANT:
This protocol is not installed by default, to
install:	
	./Setup install --protocol=POS

EOM
		;;

	WAN_CHDLC)
		cat <<EOM >$TEMP

	CISCO HDLC PROTOCOL 

Say Y to this option if you are planning to 
connect a WANPIPE card to a leased line using the Cisco 
HDLC protocol. 

It supports Dual Port Cisco HDLC as well as 
HDLC streaming API interface over S514-PCI/S508-ISA cards. 

This feature also contains the Ethernet Bridging 
over CHDLC, where a WANPIPE CHDLC link can 
be directly connected to the Linux kernel bridge. 

Consult your ISP provider on which protocol
to use.


EOM
		;;
	WAN_PPP)
		cat <<EOM >$TEMP

	POINT TO POINT (PPP) PROTOCOL 

Select this option if you are planning to connect 
a WANPIPE card to a leased line using Point-to-Point 
protocol (PPP). The PPP option
is supported on S514-PCI/S508-ISA cards.

This feature also contains the Ethernet Bridging 
over PPP, where a WANPIPE PPP link can 
be directly connected to the Linux kernel bridge. 

IMPORTANT:
This is the Standard WANPIPE PPP protocol driver,
that is to be used in most cases, since it has 
full debugging capabilities.  The TTY PPP and 
MultiPort PPP implementations use the Kernel 
PPP layer which has limited debugging capabilities
but can operaton on both PRIMARY and SECONDARY
ports !

The PPP option is supported on PRIMARY port ONLY
on S514-PCI/S508-ISA cards.

Consult your ISP provider on which protocol
to use.

EOM
		;;

	WAN_EDU_KIT)
		cat <<EOM >$TEMP
	
	WAN EDUKIT PROTOCOL

Support for Wanpipe EduKit Protocol.

IMPORTANT:
This protocol is not installed by default, to
install:	
	./Setup install --protocol=EDU
	
EOM
		;;
	WAN_X25)
		cat <<EOM >$TEMP

	X25 PROTOCOL 

	Select this option if you are planning 
to connect a WANPIPE card to an X.25 network.  Note, 
this feature also includes the X.25 API support used 
to develope custom applications over the X.25 protocol.

The X.25 option is supported on PRIMARY port ONLY on 
S514-PCI and S508-ISA cards.

Consult your ISP provider on which protocol
to use.

EOM
	;;

	WAN_MULTPPP)
		cat <<EOM >$TEMP

	MULTI-PORT: KERNEL RAW HDLC/PPP/CHDLC Protocols

	Select this option if you are planning to 
connect a WANPIPE card to a leased line using 
any of the above protocols. 

The MultiPort PPP uses the Linux Kernel SyncPPP 
protocol over the Sangoma HDLC Streaming adapter.  
Using MultiPort PPP each Sangoma adapter port 
is capable of supporting an independent 
PPP connection.

For example, a single Quad-Port PCI adapter can 
support up to four independent PPP links. 

This option is supported on S514-PCI/S508-ISA cards.

IMPORTANT:
  This option should only be used if PPP MUST run
  on both PRIMARY and SECONDARY port of the Sangoma
  adapter.  Otherwise, it is recommended that 
  standard WANPIPE PPP is used.

Consult your ISP provider on which protocol
to use.

EOM
	;;
	
	WAN_MULTPROT)
		cat <<EOM >$TEMP

	MULTI-PORT: KERNEL MULTI PROTOCOL Driver

Protocols:  Raw HDLC, PPP, CHDLC, X25 (LAPB,DSP)

	Select this option if you are planning to 
connect a WANPIPE card to a leased line using 
any of the above protocols. 

The MultiPort Multi Protocol driver implements 
above WAN protocol as kernel protocol stacks 
running above Sangoma HDLC Streaming adapter.

The Multi Protocol Driver can utilize both
Sangoma adapter ports PRI and SEC.
Where each each port PRI and SEC, is capable 
of supporting an independent WAN connection.

For example, a single Quad-Port PCI adapter can 
support up to four independent WAN links. 

This option is supported on S514-PCI/S508-ISA cards.

Consult your ISP provider on which protocol
to use.

EOM
	;;
	
	WAN_TTYPPP)
		cat <<EOM >$TEMP

	WANPIPE TTY SYNC/ASYNC PPP

Using this option, WANPIPE device drivers can
act as sync or async tty (serial) drivers, that 
can be used by the kernel PPP layer and the 
PPPD daemon to establish sync or async PPP 
connections.

The PPPD daemon can operate in ether SYNC or ASYNC
mode.  Furthermore, using the MULTILINK protocol,
multiple physical lines can be bundled into a one
logical connection (2.4.X kernels ONLY).

ex: Using the PPPD SYNC MULTILINK, multiple WANPIPE 
    devices can be bound into one logical connection 
    to provide greater than T1 throughput.

    NOTE: The remote side must also support MULTILINK
          PPP protocol.

SYNC Mode:

  The WANPIPE device driver operates as a Sync
  TTY serial driver that interfaces to a Sync 
  Leased line (ex: Fractinal T1).  Using the 
  kernel sync PPP layer and the PPPD Sync daemon
  a PPP protocol connection can be established 
  over a T1 line.

  NOTE: This option works on both ports on
  	of S514-PCI/S508-ISA card.

ASYNC Mode:

  The WANPIPE device driver operates as a Async 
  TTY serial driver that can communicate to an
  external MODEM to establish a dial up connection
  to the ISP.  Using the PPPD Async and the kernel PPP
  layer, a PPP protocol connection can be established
  established via dialup.

  NOTE: This option only works on a SECONDARY Port
        of the S514-PCI/S508-ISA card.

ttyWP{X} Device:

To interface a PPPD daemon to the WANPIPE TTY driver
a /dev/ttyWPX X={0,1,3...) device must be created.

  ex: mknod -m 666 /dev/ttyWP0 c 240 0

  Note: 240 is the Major Number
  	0   is the Minor Number


IMPORTANT:
  This option should only be used if MULTILINK option
  desired, to bundle T1 connections together. 
  Otherwise, it is recommended that standard 
  WANPIPE PPP is used.

EOM
	;;
	*)
		return
		;;

	esac

	text_area 20 60
}




#============================================================
# def_device_opt_help  
#
#
#
#============================================================


function def_device_opt_help () {

	cat <<EOM >$TEMP

	WAN Device Definition Options

Wanpipe Device:
--------------
	Select this option to define a Wan device name.
	If this is a first card in the system, the 
	device should be: wanpipe1.

WAN Protocol:
------------
	Select this option to specify a protocol, which
	should be loaded onto the above device.
EOM

	text_area

}

#============================================================
# interface_num_menu_help
#
#
#
#============================================================


function interface_num_menu_help () {

	if [ $1 -eq 0 ]; then

	cat <<EOM > $TEMP

	MAIN NETWORK INTERFACE CONFIGURATION

	Each network interface has the following
options: Interface Name and Operation Mode. Frame
Relay also has the option of DLCI.  

	Thus, select and configure each interface
displayed in the menu.  When the configuration is
complete the interface name will be displayed. 

EOM
	else

	cat <<EOM > $TEMP

	MAIN INTERFACE/PROTOCOL SETUP





EOM
	fi

	text_area

}

#============================================================
# interface_menu_help 
#
#
#
#============================================================

function interface_menu_help () {
	local opt="$1"

	case $opt in

	get_interface_name)

	cat <<EOM > $TEMP
Interface Name
--------------

	Name of the wanpipe interface which will
be displayed in ifconfig. This interface name will
also be associated with the local and remote IP of
device wanpipe$DEVICE_NUM. 

	The Wanpipe configurator always prepends the
selected interface name with 'wp#' where #=1,2,3... is 
the device number selected previously. In this case the 
interface name will be prepended with 'wp$DEVICE_NUM'.

eg)
   The way to decode the interface name: 'wp1_ppp' 
   is the following: Interface name ppp on device 
                     wanpipe1.
EOM
		;;

	
	get_operation_mode)

	cat <<EOM >$TEMP

Operation Mode
==============

	Wanpipe drivers can work in four modes:

  WANPIPE Mode 	
  ------------     
	Driver links to the IP stack, and behaves like 
  an Ethernet device. The linux routing stack 
  determines how to send and receive packets.

  API Mode:
  ---------
	Wanpipe driver uses a special RAW socket to 
  directly couple the user application to the driver.
  Thus, user application can send and receive raw data 
  to and from the wanpipe driver.

  Note: API option is currently only available on 
        CHDLC and Frame Relay protocols.
         
  BRIDGE Mode:
  ------------
  	For each frame relay network interface
  a virtual ethernet network interface is configured.
  
  Since the virtual network interface is on a BRIDGE
  device it is linked into the kernel bridging code.

  Once linked to the kenrel bridge, ethernet
  frames can be transmitted over a WANPIPE frame
  relay connections.

  BRIDGED NODE Mode:
  ------------------
  
  	For each frame relay network interface
  a virtual ethernet netowrk interface is configured.

  However, this interface is not on the bridge itself,
  it is connected remotely via frame relay link to the
  bridge, thus it is  a node.

  Since the remote end of the frame link is connected
  to the bridge, this interface will be able to send
  and receive ethernet frames over the frame relay
  line.

  Note: Bridging is only supprted under Frame Relay


  PPPoE Mode:
  -----------

  	The ADSL card configured for Bridged LLC Ethernet 
  over ATM encapsulation, usually uses PPPoE protocol
  to connect to the ISP.

  With this option enabled, NO IP setup is necessary
  because PPPoE protocol will obtain IP info from
  the ISP.
  

  ANNEXG Mode:
  ------------

 	The ANNEXG mode enables wanpipe kernel 
  protocol layers that will run on top of the 
  current interface. 
  
  Supporter MPAPI protocol are: X25/LAPB
  
EOM
	;;

	g_ADSL_EncapMode)
		cat <<EOM >$TEMP

	ATM PROTOCOL ENCAPSULATION MODE

The ATM Encapsulation Mode is used to setup 
multiprotocol headers over ATM.  

LLC: An ATM PDU header is used to indetify
     the protocol running over ATM.

VC:  No ATM PDU header is used.  

The LLC or VC information is determined from 
your ISP.

The ATM driver currently supports the
following protocol over ATM:

Options:
--------
   Bridged Ethernet LLC over ATM  (Ethernet/ATM, PPPoE/ATM)
   Bridged Ethernet VC  over ATM  (Ethernet/ATM)
   Classical IP LLC over ATM      (Raw IP/ATM)
   Routed IP VC over ATM          (Raw IP/ATM)

EOM
	;;

	get_atm_vci)
	
	cat <<EOM >$TEMP

	ATM VCI 

Virtual channel identifier.

Together, the VPI and VCI comprise the VPCI. 
These fields represent the routing information 
within the ATM cell.

EOM
	;;

	get_atm_vpi)
	
	cat <<EOM >$TEMP

	ATM VPI

Virtual path identifier.

Together, the VPI and VCI comprise the VPCI. 
These fields represent the routing information 
within the ATM cell.

EOM
	;;



	get_ppp_chdlc_mode)
	
	cat <<EOM > $TEMP

	MULTIPORT PROTOCOL

	The kernel MultiPort module
supports both PPP and CHDLC protocols.
Please select one:

PPP: Point to Point Protocol

CHDLC: Cisco HDLC protocol

Default: PPP


EOM
	;;

# TE1 BITSTREAMING ---------------------------------------------
	bstrm_get_active_chan)
		cat <<EOM >$TEMP

	BITSTREAM BOUND T1/E1 ACTIVE CHANNELS 

Bitstreaming protocol can bound into any single
or conbination of T1/E1 Active channels.  

Using socket interface API user can send and 
receive data on specified T1/E1 channels.

In SWITCHed mode, the selected channels can be
connected to channels of another T1/E1 card. Thus,
being able to switch T1/E1 channels in software
between multiple T1/E1 lines.


Please specify T1/E1 channles to bind to a 
bitstremaing device.

You can use the follow format:
o all     - for full links
o x.a-b.w - where the letters are numbers; to specify
	    the channels x and w and the range of 
	    channels a-b, inclusive for fractional
	    links.
Example: 
o ALL	   - All channels will be selcted
o 1        - A first channel will be selected
o 1.3-8.12 - Channels 1,3,4,5,6,7,8,12 will be selected.

EOM
	;;

	bstrm_get_switch_dev)
		cat <<EOM >$TEMP

	BITSTREAM BOUND SWITCH DEVICE 

In SWITCHed mode, the selected channels can be
connected to channels of another T1/E1 card. Thus,
being able to switch T1/E1 channels in software
between multiple T1/E1 lines.

The SWITCH device: 
	is the network interface of another
	WANPIPE bitstreaming device. 

Eg: wanpipe1 wp1_bstrm <--> wanpipe2 wp2_bstrm

	Where: wp1_bstrm is bound to channels: 1 and 2
	       wp2_bstrm is bound to cahnnels: 1 and 2


In this examble all data from channels 1 and 2 on 
device wanpipe1 will be SWITCHED to device wanpipe2
and vice versa.

Example: 
o wp2_bstrm 

EOM

	;;


	get_dlci)

	cat <<EOM > $TEMP
	
DLCI Number
-----------

	DLCI number specifies a frame relay
logical channel.  Frame Relay protocol multiplexes the
physical line into number of channels, each one of 
these channels has a DLCI number associated with it.

Please enter the DLCI number as specified by your 
ISP.

EOM
	;;

	prot_inter_setup)

	cat <<EOM > $TEMP

Interface Specific Protocol Setup
---------------------------------

	Further protocol setup. The following
options are network interface specific.

EOM
	;;

	num_of_dlci)
	cat <<EOM >$TEMP

Number of DLCIs supported
-------------------------

Number of DLCIs supported by this Frame
Relay connection. 

For each DLCI, a network interface will be 
created. 

Each network interface must be configured with 
an IP address. 

EOM
	;;

	ip_setup)

	cat <<EOM > $TEMP

IP Address Setup
----------------

	IP addresses must be defined for each
network interface.  

EOM
	;;
	get_x25_address)
		cat <<EOM > $TEMP

X25 Address Setup
-----------------
	
	The x25 address is interpreted differently
by PVC and SVC channels.

PVC:
     The x25 address must represent a specific
     LCN number. This LCN number must be in PCV range
     defined in hardware setup: LowestPVC to HighestPVC.

SVC:
     The x25 address must represent a proper X25
     destination address, which will be used in call
     setup. Leave blank to disable outgoing calls. 
EOM
	;;

	get_src_addr)
		cat <<EOM >$TEMP

	X25: 	X25_SRC_ADDR 

	The local source address used when placing a 
call. If left blank, no source address will be used
during call setup.

Blank by default. 

EOM
		;;

	get_accept_dest_addr)
		cat <<EOM >$TEMP
 
	X25: INCOMING DESTINATION (-d) ACCEPT PATTERN 

	This is a substring used to match 
incoming X25 destination/called address. The correctly 
matched address will be accepted and all other rejected.

Options:
  <address>  : The incoming (-d) address must match the
               specified address.
  *<address> : The incoming (-d) address string must
               terminate with the specified address.
  <address>* : The incoming (-d) address string must
               start with the specified address.
  *<address>*: The incoming (-d) address string must
               contain the specified address.
  *          : Accept a call with any -d address.
  blank      : Do not accept any incoming calls

Default: *

NOTE: This option works in conjunction with 
      Accept -s string, and Accept -u string.
      
      eg: Accept Strings for: -d -s -u
          -d *  -s * -u *  : accept all calls
          -d    -s   -u    : reject all calls	

EOM
 		;;		

	get_accept_src_addr)
		cat <<EOM >$TEMP
 
	X25: INCOMING SOURCE (-s) ACCEPT PATTERN 

	This is a substring used to match 
incoming X25 source/calling address. The correctly 
matched address will be accepted and all other rejected.

Options:
  <address>  : The incoming (-s) address must match the
               specified address.
  *<address> : The incoming (-s) address string must
               terminate with the specified address.
  <address>* : The incoming (-s) address string must
               start with the specified address.
  *<address>*: The incoming (-s) address string must
	       contain the specified address.
  *          : Accept a call with any -s address.
  blank      : Do not accept any incoming calls


Default: *

NOTE: This option works in conjunction with 
      Accept -d string, and Accept -u string.
      
      eg: Accept Strings for: -d -s -u
          -d *  -s * -u *  : accept all calls
          -d    -s   -u    : reject all calls	
EOM
 		;;	
	
	get_accept_usr_data)
		cat <<EOM >$TEMP
 
	X25: INCOMING USER DATA (-u) ACCEPT PATTERN 

	This is a substring used to match 
incoming X25 user data string. The correctly matched
string will be accepted and all other rejected.

Options:
  <string>   : The incoming (-u) string must match the
	       specified user data.
  *<address> : The incoming (-u) string must
               terminate with the specified user data.
  <address>* : The incoming (-u) string must
               start with the specified user data.
  *<address>*: The incoming (-u) string must
               contain the specified user data.
  *          : Accept a call with any -u value.
  blank      : Do not accept any incoming calls

Default: *

NOTE: This option works in conjunction with 
      Accept -d string, and Accept -s string.
      
      eg: Accept Strings for: -d -s -u
          -d *  -s * -u *  : accept all calls
          -d    -s   -u    : reject all calls	
EOM
 		;;	


	num_of_bitstrm)
		cat <<EOM > $TEMP

Number of TE1 Bitstreaming Connections
--------------------------------------

Each BitStreaming network interface can be
bounded to arbitrary number of TE1 physical 
channels.  

Once bounded, user API application can TX/RX
data to bound TE1 channels.

Note: This option is only available on cards
      that have onboard TE1 CSU/DSU's.


Default: 1 Network interface bound to all 
           T1 or E1 channels.

EOM
 		;;
		
	num_of_lcn)
		cat <<EOM > $TEMP

Number of LCNs / Network Interfaces
-----------------------------------

	For each LCN configured there must be
a network interface defined. Thus, number
of network interfaces should be equal to the 
sum of PVC and SVC channels defined in 
hardware setup.

Default: 1

EOM
	;;
	get_channel_type)
		cat <<EOM >$TEMP
	X25: 	CHANNEL_TYPE

	This option marks a channel as PVC or
SVC. It is used by the wancfg application to 
determine if the X25 Address field is an address
or an LCN number.

Options:
--------

 PVC:	Permanent Virtual Circuit. The x25 address field 
	indicates the LCN number to be used by the PVC.

 SVC:	Switched Virtual Circuit.  The x25 address field 
	specifies the place call and accept call 
	addressing information.

EOM
	;;
	
	get_lapb_t1_timer)
		cat <<EOM >$TEMP

	LABP DTE T1 Timer

The period of Timer T1, at the end of which 
retransmission of a frame may be initated. 

The DTE Timer T1 must be greater than
the maximum time between transmission of
frames and the reception of the corresponding
frame return as an answer to this frame (T2 timer).

T1 > T2

EOM
	;;

	get_lapb_t2_timer)
		cat <<EOM >$TEMP

	LAPB T2 Timer

T2 is the amount of time available at the DTE
or DCE/remote DTE before the acknowledgement
frame must be initated in order
to ensure its receipt by the DCE/remote DTE 
or DTE, respectively, prior to T1 running out.

T2 < T1

EOM
	;;
	get_lapb_t3_timer)
		cat <<EOM >$TEMP

	LAPB T3 Timer

The period of optional Timer T3 shall provide
an adequate interval of time to justify
considreing the data link to be
disconnected (out of service) state.

T3 >>> T4

EOM
	;;
	get_lapb_t4_timer)
		cat <<EOM >$TEMP

	LAPB T4 Timer

The parameter T4 isa system parameter which
represents the maximum time a DTE will allow
without frames being exchanged on the
data link.  

T4 >> T1

EOM
	;;
	get_lapb_n2_timer)
		cat <<EOM >$TEMP

	LAPB N2

Maximum number of transmissions N2

The value of N2 shall indicate number
of attempts that shall be made by DTE
or DCE to complete the sucessful
tranmsission of a frame.


EOM
	;;
	get_lapb_modulus)
		cat <<EOM >$TEMP

	Lapb Modulus

Each I Frame shall be sequentailly numbered and may
have the value 0 through modulus minus one. (Where
"modulus" is the modulus of the sequence numbers).

The modulus equals 8 or 128 and the sequence numbers
cycle through the entire range.

Options:
--------
Modulus Values:  8 or 128

Default: 8

EOM
	;;
	get_lapb_window)
		cat <<EOM >$TEMP

	LAPB Window

Number of unacknowledged frames allowed
by the LAPB Stack.

This number is usually identical to the
X25 window.

Default: 7


EOM
	;;
	get_lapb_pkt_sz)
		cat <<EOM >$TEMP

	LAPB Maximum Packet Size

Maximum packet size allowed to be
tramsitted by the Lapb Stack.

Default: 1024

EOM
	;;
			
	*)
		return
	;;
	esac
	text_area 20

}

#============================================================
# interface_setup_help
#
#
#
#============================================================


function interface_setup_help () {


	local opt=$1


	case $opt in

	get_cir)
		cat <<EOM >$TEMP

	Frame Relay: Committed Information Rate (CIR)

	This parameter enables or disables Committed 
Information rate on the board.  Options are 1 - 512 kbps.    
 
NOTE THAT THIS VALUE REFERS TO ONLY TRANSFERRING  
DATA PACKETS.   
 
NOTE: In order to disable the CIR:  CIR, BC and BE 
      fields must be commented out.  The interface 
      will fail if any of these are left out. 

Default: NO 

EOM
		;;
	get_be)
		cat <<EOM >$TEMP

	Frame Relay: Excess Burst Size (BE) 

	This parameter states the Excess Burst Size. 

Options are:  0 - 512 kbits. 

Default: NO

EOM
		;;
	get_bc)
		cat <<EOM >$TEMP

	Frame Relay: Committed Burst Size (BC) 

	This parameter states the Committed Burst Size. 

Options are 1 - 512 kbits.  
           
NOTE: For wanpipe drivers 2.1.X and greater: 
      If CIR is enabled, the driver will set BC to the
      CIR value. 

EOM
		;;
	get_encoding)
		cat <<EOM >$TEMP

	TRUE ENCODING TYPE

	This parameter is used to set the interface
encoding type to its true value.  

	By default, the interface type is set to PPP.  
This tricks the tcpdump, ethereal and others to think
that a WANPIPE interface is just a RAW-Pure IP interface.
(Which it is from the kernel point of view). By leaving
this option set to NO, all packet sniffers, like tcpdump,
will work with WANPIPE interfaces.

        However, in some cases interface type can be
used to determine the protocol on the line. By setting
this option the interface type will be set to 
its true value.  However, for Frame Relay and CHDLC,
the tcpdump support will be broken :(

Options are:
	NO: Set the interface type to RAW-Pure IP (PPP)
	YES: Set the interface type to its true value.
	
Default: NO
EOM
		;;
	get_multicast)
		cat <<EOM >$TEMP

	MULTICAST

	If enabled driver sets the IFF_MULTICAST 
bit in network device structure.

Available options are: 

 YES : Set the IFF_MULTICAST bit in device structure 
 NO  : Clear the IFF_MULTICAST bit in device structure 
      
Default: NO

EOM
		;;

	get_inarp)
		cat <<EOM >$TEMP

	INVERSE ARP TRANSMISSION
	
	This parameter enables or disables transmission 
of Inverse Arp Request packets. 

Default: NO
 
EOM
		;;
	get_inarp_int)
		cat <<EOM >$TEMP

	INVERSE ARP INTERVAL

	This parameter sets the time interval in seconds 
between Inverse ARP Request. 

Default: 10 sec 

EOM
		;;

	get_inarp_rx)
		cat <<EOM >$TEMP

	DISABLE or ENABLE INCOMING INVERSE ARP SUPPORT 
	
	This parameter enables or disables support 
for incoming Inverse Arp Request packets. If the
InARP transmission is enabled this option will be 
enabled by default.  However, if InArp transmission
is disabled, this option can be used to ignore 
any incoming in-arp requests.  

The arp requests put considerable load on the
protocol, furthermore, illegal incoming arp requests
can potentially cause denial of service, since each
pakcet must be handled by the driver.  

Default: NO
 
EOM
		;;
	
	get_ig_dcd)
		cat <<EOM >$TEMP

	CHDLC: IGNORE DCD 

	This parameter decides whether DCD will be ignored or 
not when determining active link status for Cisco HDLC. 

Options:  

 YES: Ignore DCD  
 NO : Do not ignore DCD  

 Default: NO 

EOM
		;;
	get_ig_cts)
		cat <<EOM >$TEMP

	CHDLC: IGNORE CTS 

	This parameter decides whether CTS will be ignored or 
not when determining active link status for Cisco HDLC. 

Options:  

 YES : Ignore CTS 
 NO  : Do not ignore CTS  

 Default: NO 



EOM
		;;
	get_ig_keep)
		cat <<EOM >$TEMP

	CHDLC: IGNORE KEEPALIVE Packets

	This parameter decides whether Keep alives will be  
ignored or not when determining active link status for 
Cisco HDLC.  

Options:   
 YES : Ignore Keep Alive packets 
 NO  : Do not ignore Keep Alive packets   

 Default: NO 

EOM
		;;
	get_stream)
		cat <<EOM >$TEMP
	HDLC Streaming Help

	This parameter turns off Cisco HDLC protocol, and 
uses raw frames without polling for communication.
	 
Options:

 YES : Enable HDLC Streaming
 NO  : Disable HDLC Streaming

 Default:NO


EOM
		;;
	get_tx_time)
		cat <<EOM >$TEMP

	KEEPALIVE_TX_TIMER 

	This parameter states the interval between keep alive. 
If you are set to ignore keepalives then this value is 
meaningless. The value of this parameter is given in  
milliseconds.  
 
Range:   0 - 60000 ms.  
Default: 10000 ms 
	
EOM
		;;
	get_rx_time)
		cat <<EOM >$TEMP

 	KEEPALIVE_RX_TIMER 

	This parameter states the interval to expect keepalives 
If you are set to ignore keepalives then this value is 
meaningless. The value of this parameter is given in 
milliseconds.   
 
Range  : 10 - 60000 ms.  
Default: 11000 ms 

EOM
		;;
	get_err_mar)
		cat <<EOM >$TEMP

	KEEPALIVE_ERR_MARGIN

	This parameter states the number of consecutive keep  
alive timeouts before bringing down the link.  If  you  
are set to ignore keepalives then this value is meaning 
less.   
 
Range: 1 - 20.   
Default: 5  

EOM
		;;
	get_slarp)
		cat <<EOM >$TEMP

	CHDLC IP ADDRESSING MODE 

Used to enable STATIC or DYNAMIC IP addressing

Options:
-------
   YES : Use Dynamic IP Addressing
   NO  : Use Static IP Addressing

Note: Dummy IP addresses must be supplied for
      LOCAL and POINTOPOINT IP addresses, in 
      ip interface setup, regardless of the IP MODE
      used.
      
EOM
		;;
	get_pap)
		cat <<EOM >$TEMP

	PPP Authentication Protocol (PAP)

	This parameter enables or disables the use of PAP. 

Available options are: 
     
YES : Enable PAP    
NO  : Disable PAP 
 
This option is needed regardless of the station being 
an AUTHENTICATOR. 
 
EOM
		;;
	get_chap)
		cat <<EOM >$TEMP

	CHAP

	This parameter enables or disables the use of CHAP. 

Available options are: 
      
YES : Enable CHAP 
NO  : Disable CHAP 
 
This option is needed regardless of the station being 
an AUTHENTICATOR. 

EOM
		;;
	get_ipx)
		cat <<EOM >$TEMP
	Ipx Help
EOM
		;;
	get_network)
		cat <<EOM >$TEMP
	Ipx Network Help
EOM
		;;
	get_userid)
		cat <<EOM >$TEMP

	PPP: USERID

	This parameter is dependent on the AUTHENTICATOR 
parameter.  If AUTHENTICATOR is set to NO then 
you will simply enter in your login name that the 
other side specified to you. 
 
	If AUTHENTICATOR is set to YES then you will have to 
maintain a list of all the users that are valid for 
authentication purposes. If your list contains ONLY 
ONE MEMBER then simply enter in the login name.  If 
the list contains more than one member then follow 
the below format: 

            USERID = LOGIN1 / LOGIN2 / LOGIN3....so on 
      
The "/" separators are VERY IMPORTANT if you have  
more than one member to support. 
	
EOM
		;;
	get_passwd)
		cat <<EOM >$TEMP

	PPP: PASSWD
	
	This parameter is dependent on the AUTHENTICATOR  
parameter.  If AUTHENTICATOR is set to NO then you 
will simply enter in your password for the login 
name that the other side specified to you. 
 
	If AUTHENTICATOR is set to YES then you will 
have to maintain a list of all the passwords for all the  
users that are valid for authentication purposes.  If 
your list contains ONLY ONE MEMBER then simply enter 
in the password for the corresponding login name in 
the USERID parameter.  If the list contains more than 
one member then follow the format below: 
 
      PASSWD = PASS1 / PASS2 / PASS3....so on 
 
The "/" separators are VERY IMPORTANT if you have more 
than one member to support.  The ORDER of your passwords 
is very important.  They correspond to the order of  
the userids. 

EOM
		;;
	get_sysname)
		cat <<EOM >$TEMP

	PPP: SYSNAME 

	This parameter is dependent on the AUTHENTICATOR  
parameter.  If AUTHENTICATOR is set to NO then you can 
simply ignore this parameter. (comment it ) 
 
If AUTHENTICATOR is set to YES then you have to enter 
Challenge system name which can be no longer than 31 
characters. 


EOM
		;;
	get_idle_timeout)
		cat <<EOM >$TEMP

	X25: 	IDLETIMEOUT 

	The time in seconds before an SVC will 
disconnect if there is no data over the link.  

The default is 90 seconds.

EOM
		;;

	get_hold_timeout)
		cat <<EOM >$TEMP
 
	X25: 	HOLDTIMEOUT 

	The time in seconds to wait before 
retrying a failed connection.  

The default is 10 seconds. 

EOM
		;;		


	get_src_addr)
		cat <<EOM >$TEMP

	X25: 	X25_SRC_ADDR 

	The local source address used when placing a call.
If left blank, nothing will be specified when placing a call.

Blank by default. 

EOM
		;;

	get_accept_calls_from)
		cat <<EOM >$TEMP
 
	X25: 	ACCEPT_CALLS_FROM 

	The only remote source address to accept calls from.
If left blank, all calls will be accepted.

Blank by default.

EOM
 		;;		
	*)
		return
		;;
	esac

	text_area 20 70
}

#============================================================
# device_setup_help
#
#
#
#============================================================


function device_setup_help () {

	local opt=$1

	case $opt in

	get_device_type)
		cat <<EOM >$TEMP

	ADAPTER TYPE 

	Adapter type for Sangoma wanpipe cards.

Options:
-------
S508 : This option includes:
       S508FT1:  ISA card with an onboard Fractional T1 CSU/DSU
       S508   :  ISA card with V35/RS232 interface.
       
S514 : This option includes:
       S514FT1:  PCI card with an onboard Fractional T1 CSU/DSU
       S514-1 :  PCI card with V35/RS232 interface, 1 CPU.
       S514-2 :  PCI card with V35/RS232 interface, 2 CPU.

S514 T1/E1: PCI card with an onboard Fractional T1/E1 CSU/DSU.

S514 56K:   PCI card with an onboard 56K CSU/DSU.

EOM
	;;
#-----------------------------------------------------------
	get_s514_cpu)
		cat <<EOM >$TEMP

	S514 CPU 

	The new PCI cards contain dual CPUs, where each cpu
can support a different protocol.  Thus its like having two 
cards in one.  (s514 Only)
EOM
	;;
#-----------------------------------------------------------

	get_s514_auto)
		cat <<EOM >$TEMP

	PCI SLOT AUTO-DETECTION

Select this option to enable PCI SLOT autodetection.
Otherwise, you must enter a valid PCI SLOT location
of your S514 card. 

NOTE: This option will only work for single S514
      implementations. 

      In multi-adaptor situations a correct PCI
      slot must be supplied for each card.
EOM
	;;

#-----------------------------------------------------------
	get_s514_slot)
		cat <<EOM >$TEMP

	S514 PCI SLOT 

	The new S514 cards work on a PCI bus. Thus a correct
PCI slot number must be selected.  The autodetect
functions makes this process painless. Moreover, S514
PCI cards use interrupt sharing, thus IRQ value is
not needed.  (s514 Only)

Available Option:

N : Actual PCI Slot Number (where N = any integer)

EOM
	;;

#-----------------------------------------------------------
	get_s514_bus)
		cat <<EOM >$TEMP

	S514 PCI BUS 

	The new S514 cards work on a PCI bus. Thus a correct
pci bus number must be selected.  The autodetect
functions makes this process painless. Moreover, S514
PCI cards use interrupt sharing, thus IRQ value is
not needed.  (s514 Only)

Available Option:

N : Actual PCI Bus Number (where N = any integer)

Default=0
EOM
	;;


#-----------------------------------------------------------
	get_s508_io)
		cat <<EOM >$TEMP

	S508 IO PORT

	Adapters I/O port address.  Make sure there is no 
conflict in /proc/ioports. (Mandatory for all cards and protocols) 
IO port address values are set by the jumpers on the S508 board.
Refer to the user manual for the jumper setup. 

EOM
	;;
#-----------------------------------------------------------
	get_s508_irq)
		cat <<EOM >$TEMP

	S508 IRQ 

	Adapters interrupt request level.  Make sure there is 
no conflict in /proc/interrupts. (Mandatory for all cards and 
protocol)  These are software set. 

EOM
	;;
#-----------------------------------------------------------

	get_lapb_station)
		cat <<EOM >$TEMP

	LAPB Station

DTE:  Customer premises equpment
      
DCE:  Telco X25 Switch

By default the customer setting
is always DTE, unless otherwise
stated by the ISP.


Default: DTE

EOM

	;;

	g_ADSL_EncapMode)
		cat <<EOM >$TEMP

	ADSL:ATM PROTOCOL ENCAPSULATION MODE

The ADSL ATM Encapsulation Mode is used to setup 
multiprotocol headers over ATM.  

LLC: An ATM PDU header is used to indetify
     the protocol running over ATM.

VC:  No ATM PDU header is used.  

The LLC or VC information is determined from 
your ISP.

The ADSL driver currently supports the
following protocol over ATM:

Options:
--------
   Bridged Ethernet LLC over ATM  (Ethernet/ATM, PPPoE/ATM)
   Bridged Ethernet VC  over ATM  (Ethernet/ATM)
   Classical IP LLC over ATM      (Raw IP/ATM)
   Routed IP VC over ATM          (Raw IP/ATM)
   PPP LLC over ATM               (PPP over ATM, PPPoA)
   PPP VC over ATM                (PPP over ATM)

EOM
	;;

	ADSL_Vci)
	
	cat <<EOM >$TEMP

	ATM VCI 

Virtual channel identifier.

Together, the VPI and VCI comprise the VPCI. 
These fields represent the routing information 
within the ATM cell.

EOM
	;;

	ADSL_Vpi)
	
	cat <<EOM >$TEMP

	ATM VPI

Virtual path identifier.

Together, the VPI and VCI comprise the VPCI. 
These fields represent the routing information 
within the ATM cell.

EOM
	;;


	g_ATM_EncapMode)
		cat <<EOM >$TEMP

	ATM PROTOCOL ENCAPSULATION MODE

The ATM Encapsulation Mode is used to setup 
multiprotocol headers over ATM.  

LLC: An ATM PDU header is used to indetify
     the protocol running over ATM.

VC:  No ATM PDU header is used.  

The LLC or VC information is determined from 
your ISP.

The ATM driver currently supports the
following protocol over ATM:

Options:
--------
   Bridged Ethernet LLC over ATM  (Ethernet/ATM, PPPoE/ATM)
   Bridged Ethernet VC  over ATM  (Ethernet/ATM)
   Classical IP LLC over ATM      (Raw IP/ATM)
   Routed IP VC over ATM          (Raw IP/ATM)

EOM
	;;


	ADSL_ATM_autocfg)

	cat <<EOM >$TEMP
	
	ADSL ATM VPI/VCI Autoconfig

ATM uses the VPI/VCI values to multiplex
data over a physical line.

The VPI/VCI number can be autodetected
if the remote side sends us non-idle
ATM frames.  Based on incoming frames
we can extract the VPI/VCI numbers and
configure the Tx SAR acordingly.

WARNING: 
  The Autodetect will configure on first
  received ATM Cell. It is possible that
  invalid VPI/VCI values are detected if 
  incoming cells have different VPI/VCI values.

  Autodetect is not possible if the remote
  end does not tx any ATM cells. It might 
  take a long time for remote end
  send a first ATM cell.

Use this option ONLY if you don't know
your VPI/VCI configuraiton.  

Options:
--------

YES: Enable ATM VPI/VCI Autoconfig
NO : Disable ATM VPI/VCI Autoconfig
     and use user defined values

EOM
	;;	
# TE1 ------------------------------------------------------
	get_media_type)
		cat <<EOM >$TEMP

	MEDIA TYPE 

	The new S515 cards support two hardware media types:
		T1 connection
		E1 connection

EOM
	;;
	
# TE1 ------------------------------------------------------
	get_lcode_type)
		cat <<EOM >$TEMP

	T1/E1 LINE DECODING 

	Please select one of the follow optins for lien decoding:
 		o for T1:
			AMI or B8ZS
 		o for E1:
			AMI or HDB3

EOM
	;;
# TE1 ------------------------------------------------------
	get_frame_type)
		cat <<EOM >$TEMP

	T1/E1 FRAMIING MODE 

	Please select one of the follow options:
 		o for T1:
			D4 or ESF or UNFRAMED
 		o for E1:
			non-CRC4 or CRC4 or UNFRAMED

EOM
	;;
# TE1 ------------------------------------------------------
	get_rx_sens)
		cat <<EOM >$TEMP

	THE RECEIVER SENSITIVITY 

	The new S515 cards support two receiver sensitivity:
		LH - Long Haul
		SH - Short Haul

EOM
	;;
# TE1 ------------------------------------------------------
	get_te_clock)
		cat <<EOM >$TEMP

	T1/E1 CLOCK MODE 

	Select T1/E1 clock mode:
		Normal - normal clock mode (default)
		Master - master clock mode

EOM
	;;
# TE1 ------------------------------------------------------
	get_active_chan)
		cat <<EOM >$TEMP

	THE ACTIVE CHANNELS 

	Please specify the active channels for the current device.
	You can use the follow format:
	o all     - for full links
	o x.a-b.w - where the letters are numbers; to specify
		    the channels x and w and the range of 
		    channels a-b, inclusive for fractional
		    links.
	Example: 
	o 1.3-8.12 - Channels 1,3,4,5,6,7,8,12 will be active.

EOM
	;;


# TE1 ------------------------------------------------------
	get_lbo_type)
		cat <<EOM >$TEMP

	T1 LINE BUILD OUT 

	Please specify one of the follow options for T1 Line Build Out (T1 only):
		o for Long Haul
			1.  CSU: 0dB 
			2.  CSU: 7.5dB 
			3.  CSU: 15dB 
			4.  CSU: 22.5dB
		o for Short Haul
			5.  DSX: 0-110ft
			6.  DSX: 110-220ft
			7.  DSX: 220-330ft
			8.  DSX: 330-440ft
			9.  DSX: 440-550ft
			10. DSX: 550-660ft  

EOM
	;;

# TE1 ------------------------------------------------------
	get_highimpedance_type)
		cat <<EOM >$TEMP

	T1 HIGH-IMPEDANCE MODE 

	Use High-Impedance mode to monitor the existing T1 line.

	Enable or Disable High-Impedance mod:
		NO  - Disable High-Impedance mode (default)
		YES - Enable High-Impedance mode

EOM
	;;

#-----------------------------------------------------------
	get_tty_minor)
		cat <<EOM >$TEMP

	TTY Port/Minor Number

	The TTY Minor number binds the wanpipe device
to the /dev/ttyWPX. The PPPD uses the /dev/ttyWPX to 
communicate to the Wanpipe card.  

	Ex: TTY Minor number 0 corresponds to /dev/ttyWP0.

	Options: 0 to 3
	Default: 0
EOM
	;;
#-----------------------------------------------------------
	get_firmware)
		cat <<EOM >$TEMP

	PROTOCOL FIRMWARE 
 
	Full, absolute path to wanpipe firmware files. 
(Mandatory for all cards) 

Default: /usr/lib/wanrouter/wanpipe

EOM
	;;
#-----------------------------------------------------------
	get_pos_prot)
		cat <<EOM >$TEMP
	
	POS PROTOCOLS

POS Supported Protocols are:
	IBM 4680
	NCR 2126
	NCR 2127
	NCR 1155
	NCR 7000
	ICL

Default: IBM 4680

Please contact your ISP to determine 
your line protocol.

EOM
	;;
#-----------------------------------------------------------
	get_memory)
		cat <<EOM >$TEMP

	MEMORY ADDRESS

	Address of the adapter shared memory window.  
If set to Auto the memory address is determined 
automatically. 

Default: Auto

EOM
	;;
#-----------------------------------------------------------
	get_interface)
		cat <<EOM >$TEMP

	INTERFACE

	Physical interface type. Available options are: 
o RS232   RS-232C interface (V.10) 
o V35     V.35 interface (X.21/EIA530/RS-422/V.11) 
 
DOES NOT APPLY TO FT1 CARDS. 
EOM
	;;
#-----------------------------------------------------------
	get_clocking)
		cat <<EOM >$TEMP

	CLOCKING 

	Source of the adapter transmit and receive clock  
signals.  Available options are: 
 
External:  Clock is provided externally (e.g. by  
           the modem or CSU/DSU).  Use this for FT1 boards. 

Internal:  Clock is generated on the adapter. 
           When there are two Sangoma Cards back 
           to back set clocking to Internal on one 
           of the cards. 
      
Note: Jumpers must be set for internal or external 
      clocking for RS232 communication on:
      s508 board: RS232 SEC port
      s514 board: RS232 PRI and SEC port

EOM
	;;
#-----------------------------------------------------------
	get_baudrate)
		cat <<EOM >$TEMP

	BAUDRATE 

	Data transfer rate in bits per second.  These values 
are meaningful if internal clocking is selected.  
(like in a back-to-back testing configuration) 

EOM
	;;
#-----------------------------------------------------------
	get_mtu)
		cat <<EOM >$TEMP

	MTU

	Maximum transmit unit size (in bytes).  This value  
limits the maximum size of the data packet that can be  
sent over the WAN link, including any encapsulation  
header that router may add.  

The MTU value is relevant to the IP interface.
This value informs the kernel IP stack that
of the maximum IP packet size this interface
will allow.

Default: 1500 

EOM
	;;

	get_def_pkt_size)
		cat <<EOM >$TEMP

	X25 DEFAULT PACKET SIZE

	Maximum transmit unit size (in bytes).  This value  
limits the maximum size of the data packet that can be  
sent over the WAN link, including any encapsulation  
header that router may add.  

The x25 stack uses this value to negotiate 
svc connection with the remote end.

Default: 1024

EOM
	;;

	get_max_pkt_size)
		cat <<EOM >$TEMP

	X25 MAXIMUM PACKET SIZE

	Maximum transmit unit size (in bytes).  This value  
limits the maximum size of the data packet that can be  
sent over the WAN link, including any encapsulation  
header that router may add.  

The x25 stack uses this value to negotiate 
svc connection with the remote end.

Default: 1024

EOM
	;;



#-----------------------------------------------------------
	get_udpport)
		cat <<EOM >$TEMP

	UDPPORT 

	The UDP port to is used by the UDP management/debug   
utilites.  For each WANPIPE protocol exists a UDP debug
monitor that can be used to debug the driver,card and a 
leased line.

UDP monitors located in /usr/sbin directory:
        fpipemon:  Frame Relay 
        cpipemon:  Cisco HDLC 
        ppipemon:  Standard WANPIPE PPP
       mppipemon:  Mulit-Port PPP (Using Kernel SyncPPP)
        xpipemon:  X25

The UDP debugging could be seen as a security issue.
To disable UDP debugging set the value of the
UDP Port to 0.

Options:	
--------
	UDP Ports: 9000 to 9999
	To disable UDP port set this value to 0.

	Default 9000
EOM
	;;
#-----------------------------------------------------------
	get_station)
		cat <<EOM >$TEMP

	Frame Relay: STATION

	This parameter specifies whether the  adapter 
should operate as a Customer  Premises Equipment (CPE) 
or emulate a  frame relay switch (Access Node).   

Available options are: 
     CPE     CPE mode (default) 
     Node    Access Node (switch emulation  
                           mode) 

Default: CPE

EOM
	;;

#-----------------------------------------------------------
	get_connection)

		cat <<EOM >$TEMP

	CHDLC PHYSICAL OPERATION

	The CHDLC firmware can operate in two
modes: Permanent and Switched.  

Permanent:  The RTS and DTS are kept high during
            Tx and Rx of data. For most connections
	    this is a default behaviour. It supports
	    full duplex operatoin.

Switched:   The RTS is raised only during Tx of data.
	    This mode is usually for half duplex lines,
	    for example a terminal that is connected
	    to a mainframe or a satelite. 

Default: Permanent

EOM
	;;

#-----------------------------------------------------------
	get_linecode)

		cat <<EOM >$TEMP

	CHDLC PHYSICAL LINE CODING

	The two coding standards are 
NRZ and NRZI.  Please consult you ISP on which
encoding you require.  By default most todays lines
use NRZ (default) encoding.  Thus if unsure leave
as default.

Default: NRZ

EOM
	;;
#-----------------------------------------------------------
	get_lineidle)

		cat <<EOM >$TEMP

	CHDLC PHYSICAL LINE IDLE

	Between each Tx frame, the hardware
either idles a FLAG or MARK.  By default almost
all todays physical lines idle FALG, however in 
some instances like satelite links, where
bandwidth is an issue, MARK might be used during
line idle.

Default: FLAG

EOM
	;;

#-----------------------------------------------------------
	get_signal)
		cat <<EOM >$TEMP

	Frame Relay: SIGNALLING

	This parameter specifies frame relay link management  
type.  Available options are: 
                  
  ANSI    ANSI T1.617 Annex D (default) 
  Q933    ITU Q.933A 
  LMI     LMI 
  No      Turns off polling, thus DLCI becomes active 
	  regardless of the remote end. 
          (This option is for NODE configuration only)

Default: ANSI

EOM


	;;

	get_ignore_fe)
		cat <<EOM >$TEMP

	DISABLE/ENABLE FRONT END STATUS

	This option can ether Enable
or Disable FRONT END (CSU/DSU) status.

Enable:  The front end status will affect
         the link state.  Thus if CSU/DSU looses
	 sync, the link state will become
	 disconnected.

Disable: No matter what state the front end (CSU/DSU)
         is in, the link state will only depend
	 on the state of the protocol.

Options to ignore front end:
----------------------------

  NO: This option enables the front end status
      and the link will be affected by it.

  YES: This option disables the front end
       status fron affecting the link state.

Default: NO

EOM
	;;

	get_fs_issue)
		cat <<EOM >$TEMP

	ISSUE A FULL STATUS REQUEST ON STARTUP

	This option is used to speed up frame
relay connection.  In theory, frame relay takes
up to a minute to connect, because a full status
request timer takes that long to timeout.  

This option, forces a full status request as soon
as the protocol is started, thus a line would come
up faster.

In some VERY rare cases, this causes problems with
some frame relay switches.  In such situations this
option should be disabled.  

Available options are: 

 YES : Enable Full Status request on startup 
 NO  : Disable Full Status request on startup 
      
Default: YES

EOM
	;;

	
#-----------------------------------------------------------
	get_T391)
		cat <<EOM >$TEMP

	Frame Relay: T391 TIMER 

	This is the Link Integrity Verification Timer value in  
seconds.  It should be within a range from 5 to 30 and  
is relevant only if adapter is configured as CPE. 

EOM
	;;
#-----------------------------------------------------------
	get_T392)
		cat <<EOM >$TEMP

	Frame Relay: T392 TIMER

	This is the Polling Verification Timer value in seconds. 
It should be within a range from 5 to 30 and is relevant 
only if adapter is configured as Access Node.

EOM
	;;
#-----------------------------------------------------------
	get_N391)
		cat <<EOM >$TEMP

	Frame Relay: N391 

	This is the Full Status Polling Cycle Counter. Its  
value should be within a range from 1 to 255 and is  
relevant only if adapter is configured as CPE. 

EOM
	;;
#-----------------------------------------------------------
	get_N392)
		cat <<EOM >$TEMP
	
	Frame Relay: N392 

	This is the Error Threshold Counter. Its value should  
be within a range from 1 to 10 and is relevant for both  
CPE and Access Node configurations.

EOM
	;;
#-----------------------------------------------------------
	get_N393)
		cat <<EOM >$TEMP

	Frame Relay: N393 

	This is the Monitored Events Counter. Its value should  
be within a range from 1 to 10 and is relevant for both  
CPE and Access Node configurations. 

EOM
	;;
#-----------------------------------------------------------
	get_ttl)
		cat <<EOM >$TEMP

	TTL

	This keyword defines the Time To Live for a UDP 
packet used by the monitoring system.  The user can  
control the scope of a UDP packet by associating a  
number that decrements with each hop.

EOM
	;;
#-----------------------------------------------------------
	get_commport)
		cat <<EOM >$TEMP

	CHDLC COMMUNICATION PORT

	Currently only CHDLC can use both ports on a S508 or
S514PCI card at the same time. Thus, two physical links can be 
hooked up to a single card using CHDLC protocol.

Availabe options:

PRI: Primary High speed port
	S508: up to 2Mbps
	S514: up to 4Mbps

SEC: Secondary Low speed port
	S508: up to 256Kbps
	S514: up to 512Kbps

Default: PRI
 
EOM
	;;
#-----------------------------------------------------------
	get_ipmode)
		cat <<EOM >$TEMP

	PPP IP MODE 

   Configure PPP IP addressing mode. 

Options: 
--------

STATIC:   
 	No IP addresses are requested or supplied. 
	Driver uses IP addresses supplied by the user.
HOST:
	Provide IP addresses to the remote peer upon 
	peer request. 
	Note: 	This option may cause problems against 
		routers that do not support dynamic ip 
		addressing.
PEER:
	Request both local and remote IP addresses
	from the remote host. If the remote host does not 
	respond, configuration will fail.  

Default: STATIC 

EOM
	;;
#-----------------------------------------------------------
	get_hdlc_station)

		cat <<EOM >$TEMP

	X25:	STATION

	This parameter defines whether the  
adapter should operate as a Data Terminal Equipment (DTE) 
or Data Circuit Equipment (DCE).  

Normally, you should select DTE mode.  DCE mode  
is primarily used in back-to-back  testing configurations. 

Options: 
--------
	DTE     DTE mode (default) 
	DCE     DCE mode 
      
Note:
-----
	DOES NOT APPLY TO HDLC (LAPB) API. 
EOM
	;;	
#-----------------------------------------------------------
	get_ccitt)

		cat <<EOM >$TEMP

	X25:	CCITT COMPATIBILITY 
 
	This parameter defines CCITT X.25 
compatibility level. 

Options: 
-------
	1988   1988 version of the Recommendation X.25 (default) 
	1984   1984 version of the Recommendation X.25 
	1980   1980 version of the Recommendation X.25 

EOM
	;;
#-----------------------------------------------------------
	get_hdlc_only)
		cat <<EOM >$TEMP

	X25:	LAPB_HDLC_ONLY

	This option disables the X25 protocol level, and
allows the user control at the LAPB HDLC level.

Options:
--------
	YES:  Disable X25 and run in LAPB HDLC Mode
	NO :  Use both X25 and LAPB HDLC protocols.

EOM
	;;
#-----------------------------------------------------------
	get_logging)
		cat <<EOM >$TEMP

	X25: CALL_SETUP_LOG

	This option enables or disables call setup
logging. Logs are usually located in /var/log/messages.
By setting this option to YES, every state change will
be logged.  

Note: Logging will impeed the driver performance during 
      heavy traffic conditions, where calls are setup   
      and cleared on high intervals. 

Options:
-------
	YES:   Enable call setup logging
	NO :   Disable call setup logging
EOM
	;;

	get_oob_modem)
		cat <<EOM >$TEMP
	X25: OOB_ON_MODEM

	This option enables or disables out-of-band
modem status change messages.  Therefore, one would
enable this option in order for an api applicaton 
to receive modem status changes.  

Note: modem status change has no effect on the x25 
      protocol. The link status depends on the HDLC
      protocol.

Options:
--------
	NO:   Disable OOB messaging on modem status.
	YES:  Enable OOB messaging on modem status.

EOM
	;;
#-----------------------------------------------------------
	get_*_pvc)
		cat <<EOM >$TEMP

	X25:	LOWEST PVC / HIGHEST PVC

	These parameters are used to define permanent virtual  
circuit range as assigned by your the X.25 service  
provider.  Valid values are between 0 and 4095.  

Default values for both parameters are 0, meaning that no PVCs  
are assigned.  

Note that maximum number of both  
permanent and virtual circuits should not exceed 255.

IMPORTANT:
	For each LCN defined here, there must be a network 
	interface setup in interface/protocol setup section.
EOM
	;;
#------------------------------------------------------------
	get_*_svc)
		cat <<EOM >$TEMP

	X25:	LOWEST SVC / HIGHEST SVC

	These parameters are used to define switched virtual  
circuit range as assigned by your the X.25 service  
provider.  Valid values are between 0 and 4095.  

Default values for both parameters are 0, meaning that no SVCs  
are assigned.  

Note that maximum number of both permanent and virtual 
circuits should not exceed 255. 

IMPORTANT:
	For each LCN defined here, there must be a network 
	interface setup in interface/protocol setup section.
EOM
	;;
#-----------------------------------------------------------
	get_hdlc_win)
		cat <<EOM >$TEMP

	X25:	HDLC WINDOW

	This parameter defines the size of the HDLC frame  
window, i.e. the maximum number of sequentially  
numbered Information frames that can be sent without  
waiting for acknowledgment.  

Valid values are from 1 to 7.  

Default is 7. 

EOM
	;;
#-----------------------------------------------------------
	get_packet_win)
		cat <<EOM >$TEMP

	X25:	PACKET WINDOW

	This parameter defines the default size of the X.25  
packet window, i.e. the maximum number of sequentially  
numbered data packets that can be sent without waiting 
for acknowledgment.  

Valid values are from 1 to 7.   

Default is 7. 
EOM
	;;
#-----------------------------------------------------------
	get_rec_only)
		cat <<EOM >$TEMP

	CHDCL: 	RECEIVE ONLY

	This parameter enables Receive only communications.
In this mode of operation no transmission is possible for
both primary and secondary CHDLC ports.  

When in receive only mode, both ports can run at 2.75 Mbps
maximum respectively. 

Note: Receive Only Mode, applies to S514 PCI cards 
      exclusively.

Default is NO.

Options:
-------
	YES:   Enable receive only communications. 
	NO :   Disable receive only communications.

EOM
	;;
	get_T1)
		cat <<EOM >$TEMP

	X25:	T1 Timer

	The period timer T1 is used for various link level
retransmission and recovery procedures.

Valid values are 1 to 30 seconds.

Default: 3

	
EOM
	;;
	get_T2)
		cat <<EOM >$TEMP

	X25:	T2 Timer

	T2 is the amount of time available at the
station before an before acknowledging frame must 
be initiated in order to ensure its receipt by the
remote station, prior to timer T1 running out at
that station.

Valid values are 0 to 29 seconds.

Default: 0 

Note: T2 timer must be lower than T1 !!! 
	
EOM
	;;

	get_T4)
		cat <<EOM >$TEMP

	X25:	T4 Timer

	T4 timer is used to allow the Sangoma HDLC
code to issue a link level Supervisory frame 
periodically during the 'quiescent' ABM link phase.

Options:
--------
	0    : No unnecessary Supervisory frames
        T4*T1: Seconds between each transmission

	ex: T1=3 and T4=10, Therefore, 30 seconds between
            transmissions.

Valid values for this parameter are 0 to 240.

Default: 240

EOM
	;;
	get_N2)
		cat <<EOM >$TEMP

	X25:	N2 Timer

	Maximum number of transmissions and 
retransmission of an HDLC frame at T1 interfals 
before a state change occurs.

Valid values are 1 to 30 seconds.

Default: 10
	
EOM
	;;

	get_T10_T20)
		cat <<EOM >$TEMP

	X25:	T10_T20 Timer

	Timeout on Restart Indication/Request packets.

Valid values are 1 to 255 seconds.

Default: 30
	
EOM
	;;
	get_T11_T21)
		cat <<EOM >$TEMP

	X25:	T11_T21 Timer

	Timeout on Incoming Call / Call Request.

Valid values are 1 to 255 seconds.

Default: 30
	
EOM
	;;

	get_T12_T22)
		cat <<EOM >$TEMP

	X25:	T12_T22 Timer

	Timeout on Reset Indication/Request packets.

Valid values are 1 to 255 seconds.

Default: 30
	
EOM
	;;
	get_T13_T23)
		cat <<EOM >$TEMP

	X25:	T13_T23 Timer

	Timeout on Clear Indication/Request packets.

Valid values are 1 to 255 seconds.

Default: 30
	
EOM
	;;
	get_T16_T26)
		cat <<EOM >$TEMP

	X25:	T16_T26 Timer

	Timeout on Interrupt packets.

Valid values are 1 to 255 seconds.

Default: 30
	
EOM
	;;
	get_T28)
		cat <<EOM >$TEMP

	X25:	T_28 Timer

	Timeout on Registration Request packets.

Valid values are 1 to 255 seconds.

Default: 30
	
EOM
	;;
	get_R10_R20)
		cat <<EOM >$TEMP

	X25:	R10_R20 Timer

	Retransmission count for Restart
Indication/Request packets.

Valid values are 0 to 250 seconds.

Default: 5
	
EOM
	;;
	get_R12_R22)
		cat <<EOM >$TEMP

	X25:	R12_R22 Timer

	Retransmission count for Reset
Indication/Request packets.

Valid values are 0 to 250 seconds.

Default: 5
	
EOM
	;;
	get_R13_R23)
		cat <<EOM >$TEMP

	X25:	R13_R23 Timer

	Retransmission count for Clear
Indication/Request packets.

Valid values are 0 to 250 seconds.

Default: 5
	
EOM
	;;
	get_tty_mode)
		cat <<EOM >$TEMP

	TTY OPERATION MODE

	Wanpipe TTY driver can operatin in two different
modes, SYNC and ASYNC.

SYNC:	The TTY sync mode along wiht the pppd daemon 
        (sync option) can be used to establish a PPP
	connection over the Fractional T1 sync leased.

	Maximum baudrate depends on the fractinal T1 line.
	Usually 1.5Mbps.


ASYNC: 	The TTY asymc mode simulates a serial port driver,
 	that can interface to a 56K modem.  Using TTY async
	serial driver along with the pppd daemon (async option)
	and a chat dial up script one can estabish a PPP 
	connection over a modem: 64K telephone lines.

	Note: Maximum baudrate depends on the modem.
	      Usually 56K.

Default: SYNC

EOM
	;;

	get_sync_opt)
		cat <<EOM >$TEMP
	SYNC OPTIONS

	T1/E1 Cards:
		Must user external sync options
		
		Value= 0x0010

	V35/RS232 Cards:
		Should use the monosync option

		Value= 0x0001
		
	
EOM
	;;
	
	get_rx_sync_ch)
		cat <<EOM >$TEMP
	RX SYNC CHAR

	This value is used by the firmware to
	sync up on a stream of data.  Usually this
	value is set to the IDLE HDLC FLAG.

	For T1/E1 Channel splitting this value
	is not used. Leave it as default.

	Default: 0x7F

EOM
	;;
	
	get_msync_fill_ch)
		cat <<EOM >$TEMP
	MONO SYNC FILL CHAR

	Bitstreaming driver uses this value in two ways:

	HDLC Encoding/Decoding Disabled:
		This value is used to fill in the TX frame
		when there is no user data to TX.  
		
	
	HDLC Encoding/Decoding Enabled:
		This value must be set to a valid HDLC 
		flag. An HDLC flag must contain 6 ones
		in a row.  
		
		A default HDLC flag is: 0x7F

	Default: 0x7F	

EOM
	;;

	get_max_tx_block)
		cat <<EOM >$TEMP
	MAX TX BLOCK

	Maximum transmit packet size.

	T1/E1 Channel Splitting:
	
	  When running in TE1 mode: channel splitting
	  The TX packet will always be the MAX TX BLOCK
	  value.  Thus if the value is set too high,
	  there will be a high delay between frames 
	  sent (Not good for TDM Voice).

	  In this case it is recomended that MAX TX BLOCK
	  value be a multiple of T1 (N*24) or E1(N*32) 
	  channels. 
	
	  It is always a good idea to have to MAX TX and
	  MAX RX values EQUAL !!!

	  Minimum Tx Block: 10 * Channels

	  Default: T1=720  E1=960   (30 * Channels)


	V35/RS232 Bit Streaming:
	
	  Default: 4096

EOM
	;;
	get_rx_comp_length)
		cat <<EOM >$TEMP
		
	RX COMPLETE LENGTH

	The firware will buffer RX COMPLETE LENGTH number
	of bytes before triggering RX interrupt to the
	driver.

	T1/E1 Channel Splitting:
	
	  When running in TE1 mode: channel splitting
	  The RX packet will always be the RX COMPLETE LENGTH 
	  value.  Thus if the value is set too high,
	  there will be a high delay between frames 
	  received (Not good for TDM Voice).

	  In this case it is recomended that RX COMPLETE LENGTH
	  value be a multiple of T1 (N*24) or E1(N*32) 
	  channels. 

          It is always a good idea to have to MAX TX and
	  MAX RX values EQUAL !!!

	  Minimum Tx Block: 10 * Channels

	  Default: T1=720  E1=960   (30 * Channels)


	V35/RS232 Bit Streaming:
	
	  Default: 4096	

EOM
	;;

	get_rx_comp_timer)
		cat <<EOM >$TEMP
		
	RX COMPLETE TIMER

	The firware will wait 1500 ms for the
	rx buffer to reach MAX RX SIZE. 
	
	If the firware RX buffer does not reach MAX size
	in that time, it will trigger an RX interrupt
	to the driver, and deliver a partial frame.
	
	Default: 1500

EOM
	;;

	adsl_get_std_item)
		cat <<EOM >$TEMP
		
Define ADSL Standard Compliance:
	T1.413/G992.1(G.dmt)/G992.2(G.lite)
	Alcatel 1.4/Alcatel/ADI
	Multimode/T1.413 AUTO

Default: Multimode (the recommended setting for both 
	 CO and CPE).

EOM
	;;
	
	adsl_get_trellis)
		cat <<EOM >$TEMP
		

Enable/Disable Trellis coding. 
The TELLIS_LITE_ONLY_DISABLE choice, enables trellis 
for G.DMT or T1.413 and disables it for G.lite.

Default: Trellis Enable.

EOM
	;;
	
	ADSL_TxPowerAtten)
		cat <<EOM >$TEMP
		
The value of transmit power attenuation can be from, 
0 to 12 dB, programmable in 1dB increments.

EOM
	;;
	
	adsl_get_codinggain)
		cat <<EOM >$TEMP
		
Coding gain is the improvement, or gain, due to trellis/RS
coding. Automatic coding gain seclection is recommended for
automatic bit allocation depending upon line conditions.
Otherwise, requested coding gain is selectable from 0 to 7 dB
in 1 dB increments.

Default: Automatic coding gain.

EOM
	;;
	
	ADSL_MaxBitsPerBin)
		cat <<EOM >$TEMP
		
The maximum number of receive bits per bin can be selected.
Possible values can be less than or equal to 15.

EOM
	;;
	
	ADSL_TxStartBin)
		cat <<EOM >$TEMP
		
The lowest bin number allowed for the transmit signal can be
selected. This allows the customer to limit bins used for 
special configurations.

	0x06-0x1F	upstream
	0x06-0xFF	downstrem (0x20-0xFF for FDM)

EOM
	;;
	
	ADSL_TxEndBin)
		cat <<EOM >$TEMP
		
The highest bin number allowed for the transmit signal can be
selected. This allows the customer to limit bins used for 
special configurations.

	0x06-0x1F	upstream
	0x06-0xFF	downstrem (0x20-0xFF for FDM)

EOM
	;;
	
	ADSL_RxStartBin)
		cat <<EOM >$TEMP
		
The lowest bin number allowed for the receive signal can be
selecte. This allows the customer to limit bins used for 
special configurations.

EOM
	;;
	
	ADSL_RxEndBin)
		cat <<EOM >$TEMP
		
The highst bin number allowed for the receive signal can be
selected. This allows the customer to limit bins used for 
special configurations.

EOM
	;;
	
	adsl_get_rxbinadjust)
		cat <<EOM >$TEMP
		
Automatic bin adjustment can be enabled or disabled by 
selecting the following:
	ADSL_RX_BIN_ENABLE  - automaticcale adjusts bin
			      numbers (bin limitations
			      specified by RxStartBin
			      and RxEndBin options are
			      ignored).

	ADSL_RX_BIN_DISABLE - automaticcale adjusts bin
			      numbers (bin limitations 
			      specified by RxStartBin
			      and RxEndBin options are 
			      employed).

EOM
	;;
	
	adsl_get_framingtype)
		cat <<EOM >$TEMP
		
Select Framing Structure Type (0,1,2,3).
This value is ignored for G.lite.

EOM
	;;
	
	adsl_get_expandedexchange)
		cat <<EOM >$TEMP
		
Only for T1.413, Expanded Exchange Sequence (EES) can be
enabled by selecting ADSL_EXPANDED_EXCHANGE or it can be
disable by selecting ADSL_SHORT_EXCHANGE. EES should
normally be enabled.

EOM
	;;
	
	adsl_get_clocktype)
		cat <<EOM >$TEMP
		
The value ADSL_CLOCK_CRYSTAL selects the use of a 17.28 MHz 
crystal, whereas the value ADSL_CLOCK_OSCILLATOR selects the
use of a 34.56 MHz oscillator.

EOM
	;;
	
	ADSL_MaxDownRate)
		cat <<EOM >$TEMP
		
Use this value to limit the downstream data rate.
The max downrate is 8192Kbps.  By default this
value is used to achieve the highest baudrate
during negotiation.  

However, user can specify a lower value in order
to force the remote end to lower baud rate.

Default: 8192Kbps.

EOM
	;;

	
	*)
		return
	;;
	esac
	text_area 20 70

}


function ip_setup_help () {

	local opt=$1

	case $opt in

	get_local_ip)
		cat <<EOM >$TEMP

	LOCAL IP ADDRESS

WARNING: This program does no checking of any of the IP 
         entries. Correct configuration is up to you ;) 
         Additional routes may have to be added manually,
         to resolve routing problems.
         
	Defines IP address for this interface.  IP addresses 
are written as four dot-separated decimal numbers from 0 to 255 
(e.g. 192.131.56.1).  Usually this address is assigned to you by 
your network administrator or by the Internet service provider.

Default: Consult your ISP

EOM
		;;
	get_rmt_ip)
		cat <<EOM >$TEMP

	POINTOPOINT IP ADDRESS

	Most WAN links are of point-to-point type, which means 
that there is only one machine connected to the other end of the 
link and its address is known in advance.  This option is the 
address of the 'other end' of the link and is usually assigned 
by the network administrator or Internet service provider.

Default: Consult your ISP

EOM
		;;
	get_netmask)
		cat <<EOM >$TEMP

	NETMASK IP ADDRESS

	Defines network address mask used to separate host 
portion of the IP address from the network portions.  

	The default of 255.255.255.255 specifies a 
point-to-point connection which is almost always OK.

        You may want to override this  when you create 
sub-nets within your particular address class.  In this case 
you have to supply a netmask.  Like the IP address, the 
netmask is written as four dot-separated decimal numbers from 
0 to 255 (e.g. 255.255.255.0).

Default: Consult your ISP

EOM
		;;
	get_bcast)
		cat <<EOM >$TEMP

	BROADCAST IP ADDRESS

	This option is reserved for media supporting broadcast 
addressing, such as Ethernet

Default: Leave Empty 

EOM
		;;

	get_default)
		cat <<EOM >$TEMP

	SET DEFAULT ROUTE

	This option will create a default route, meaning 
that all network traffic with no specific route found 
in the routing table will be forwarded to this interface.  

Default route is useful for connections to the 
'outside world', such as Internet service provider.

If unsure select NO.

Options:
--------
   YES : Set default route
   NO  : No default route

EOM
		;;

	get_dyn_intr)
		cat <<EOM >$TEMP

	DYNAMIC INTERFACE CONFIGURATION

WANPIPE

   The driver will dynamically bring up/down the
   interface to reflect the state of the physical 
         link (connected/disconnected).

   If you are using SNMP, enable this option.
   The SNMP uses the state of the interface to 
   determine the state of the connection.


ADSL PPPoA and MULTIPORT PPP

   Used by PPP protocol to indicate dynamic IP 
   negotiation.  


If unsure select NO.

Options:
--------
   YES:  Enable  dynamic interface configuration
   NO :  Disable dynamic interface configuration  

EOM
		;;

	get_network)
		cat <<EOM >$TEMP

	NETWORK IP ADDRESS

	This is the address of the network you are connecting 
to.  It is used to set up the kernel routing tables.  By supplying 
this address you instruct the kernel to direct all network traffic 
destined to that network to this interface.  

        Like any other IP address, the network address is 
written as four dot-separated decimal numbers from 0 to 255 
(e.g. 192.131.56.0).  Alternatively, you can use symbolic 
network name from the /etc/networks file in place of the 
numeric address.  
 
        A special case is the network address 0.  This will 
create a default route, meaning that all network traffic with no 
specific route found in the routing table will be forwarded 
to this interface.  Default route is useful for connections to 
the 'outside world', such as Internet service provider.

Default: Leave Empty

EOM
		;;
	get_gateway)
		cat <<EOM >$TEMP

	GATEWAY IP ADDRESS

	If the network you are connecting to is not directly 
reachable, you may also specify a gateway address.  Note however, 
that gateway has to be reachable, i.e. a static route to the 
gateway must exist.  The gateway address also follows usual IP 
address notation or can be replaced with the symbolic name 
from the /etc/host file.

Default: Leave Empty

EOM
		;;
	*)
		return
		;;
	esac
	text_area 20 70

}







#============================================================
# error
#
#
#
#============================================================


function error () {

	local opt=$1
	local num=$2

	cat <<EOM >$TEMP
	!!! WANPIPE CONFIGURATOR FAILURE !!!

	Configuration parse check failed due to 
	unconfigured options.	 

EOM

	case $opt in


	DEVICE_NUM)
		cat <<EOM >>$TEMP
Error:
------
	Wanpipe device name has not been defined.
EOM
		;;

	NUM_OF_INTER)
		cat <<EOM >>$TEMP

Error:
------
	No network interfaces specified.  
EOM
		;;
	DEVICE_TYPE)
		cat <<EOM >>$TEMP

Error:
------
	Physical device not configured. 

EOM
		;;
	IF_NAME)
		cat <<EOM >>$TEMP
Error:
------
	Interface name was not specified for interface
number $num.
EOM
		;;
	DLCI_NUM)
		cat <<EOM >>$TEMP


Error:
-----
	DLCI number was not specified for
interface number $num.
EOM
		;;
	INTR_STATUS)
		cat <<EOM >>$TEMP

Error:
------
	Protocol information for Interface ${IF_NAME[$num]} 
was not configured.
EOM
		;;	
	PROT_STATUS)
		cat <<EOM >>$TEMP

Error:
------
	Protocol specific informatoin for interface 
${IF_NAME[$num]} was not configured.
EOM
		;;

	CIR_BC)
		cat <<EOM >>$TEMP

Error:
------
	When CIR is enabled, BC must also be enabled. 
In most cases CIR and BC should be the same value.
EOM
		;;
	CIR_BE)
		cat <<EOM >>$TEMP

Error:
------
	When CIR is enabled, BE must also be enabled.
In most cases BE is set to zero.
EOM
		;;
	IP_STATUS)
		cat <<EOM >>$TEMP

Error:
------
	IP address information for Interface 
${IF_NAME[$num]} was not configured.		

EOM
		;;
	L_IP)
		cat <<EOM >>$TEMP

Error:
------
	Local IP Address has not been defined for
interface ${IF_NAME[$num]}.
EOM
		;;
	R_IP)	
		cat <<EOM >>$TEMP

Error:
------
	Remote IP Address has not been defined for
interface ${IF_NAME[$num]}.
EOM
		;;
	NMSK_IP)
		cat <<EOM >>$TEMP

Error:
------
	Netmask address has not been defined for 
interface ${IF_NAME[$num]}.	
EOM
		;;
	NMSK_BRIDGE_IP)
		cat <<EOM >>$TEMP

Error:
------
	BRIDGE NODE: Illegal IP Netmask 
on interface ${IF_NAME[$num]}

	The BRIDGE NODE netmask cannot set to 
255.255.255.255 because the routing table will 
not be updated. 
Reason: There is no PointoPoint address.
EOM
		;;
	
	MTU_BRIDGE)
		cat <<EOM >$TEMP

Error:
------
	Illegal BRIDGE MTU value.
The minumum BRIDGE MTU must set to 1520.
EOM
		;;

	NON_ETH_PPPoE)
	cat <<EOM >$TEMP

Error:
------
	Illegal interface operation mode: PPPoE
for ADSL protocol: $ADSL_EncapMode

Note, that PPPoE operation can only work for
Bridge Ethernet LCC/VC encapsulations.

EOM
		;;


	X25_CHANNEL_NUM_MISMATCH)
		cat <<EOM >$TEMP

	X25 CHANNEL NUMBER MISMATCH

Error:
-----
	The number of channels defined in 
HIGH_PVC to LOW_PVC and HIGH_SVC to LOW_SVC
does not match to number of network interfaces
defined.
	
	There must be a one to one relationship between
the number of interfaces defined and number of channels
specified.
	
	Only after all network interfaces are started
will the driver enable X25 communications.

Solution:
--------
	Therefore change the HIGH_PVC, LOW_PVC, HIGH_SVC
and LOW_SVC values to reflect the number of network 
interfaces defined.
EOM
		;;
	X25_LOW_SVC_ZERO)
		cat <<EOM >$TEMP

	X25 LOW_SVC VARIABLE IS ZERO
Error:
-----
	If the HIGH_SVC value is defined than
the LOW_SVC value must also be defined.

Example:
-------
For a single SVC channel:
	HIGH_SVC=1
	LOW_SVC =1

EOM
		;;
	X25_LOW_PVC_ZERO)
		cat <<EOM >$TEMP

	X25 LOW_PVC VARIABLE IS ZERO
Error:
-----
	If the HIGH_PVC value is defined than
the LOW_PVC value must also be defined.

Example:
-------
For a single PVC channel:
	HIGH_PVC=1
	LOW_PVC =1

EOM
		;;

	CREATE_TTYOK)
		cat <<EOM >$TEMP
		
 Device /dev/ttyWP$TTY_MINOR created successfully!

EOM
		;;

	CREATE_TTYFAIL)
		cat <<EOM >$TEMP
		
	PPPD Daemon Configuration FAILED !
		
 Failed to create device /dev/ttyWP$TTY_MINOR !
 
 Please try to create it manually using the
 following command:
    mknod -m 666 /dev/ttyWP$TTY_MINOR c 226 $TTY_MINOR

EOM
		;;
	
	CREATE_PPPDOK)
		cat <<EOM >$TEMP
	
	PPPD Daemon Configured OK!
	
PPPD daemon $TTY_MODE configuration file
/etc/ppp/options.ttyWP$TTY_MINOR created 
successfully!

PPPD daemon call script created successfully!
/etc/ppp/peers/isp_wanpipe$DEVICE_NUM

To start pppd run:
	pppd call isp_wanpipe$DEVICE_NUM

EOM

	if [ $TTY_MODE = Async ]; then
		cat <<EOM >>$TEMP
	
IMPORTANT:  

The pppd is setup to use /etc/ppp/redialer
to establish a modem connection.  Please
edit the /etc/ppp/redialer and change:

	1. Phone Number
	2. User Login and Password

before starting Async pppd.

EOM
	fi

		;;

	CREATE_PPPDOK)
		cat <<EOM >$TEMP
		
Failed to create PPPD daemon $TTY_MODE 
configuration file:
     /etc/ppp/options.ttyWP$TTY_MINOR.

Refer to the WanpipeConfigManual.(pdf/txt)
for pppd configuration instructions.

EOM
		;;



	SAVE_OK)
		cat <<EOM >$TEMP

          !! SAVE SUCCESSFUL !! 

   Wanpipe configuraton for device wanpipe$DEVICE_NUM
         was saved successfully in:  
       
             $WAN_CONFIG !
EOM
		;;
	FIRMWARE)
		cat <<EOM >$TEMP

	FIRMWARE PATH ERROR !!!
	
The directory $FIRMWARE 
does not exist.

EOM
		;;
	PVC_ADDR)
		cat <<EOM >$TEMP

	X25 PVC Address Error !!!

The PVC address should be set to the
active LCN number defined in hardware
configuration section.

PVC address must be an integer.

EOM
		;;

	SVC_ADDR)

		cat <<EOM >$TEMP
	
	X25 SVC Address Error !!!

The SVC address must be a correct X25 address
supplied by the ISP. Furthermore, the address
must have @ as a first characters.
  ex: @124

To accept all calls set the SVC address to @.

Note the above applies only to X25 Wanrouter, 
not API !!! 
EOM
		;;
	
	X25_ADDR)
		
		cat <<EOM >$TEMP

	X25 ADDRESS Error !!!

The X25 Address must be initialized.
Return to the interface setup, and
input a desired X25 Address.

EOM
		;;
	ASYNC_PRI)
		cat <<EOM >$TEMP

	TTY ASYNC Error !!!

Async TTY is only supported on a SECONDARY
port.  Please change the COMMPORT to SEC.

EOM
		;;

	ASYNC_CLOCKING)
		cat <<EOM >$TEMP
	
	TTY ASYNC Error !!!

Async TTY must always have Internal clocking.
Please set the clocking mode to Internal.

EOM
		;;

	NO_HWPROBE)
		cat <<EOM >$TEMP

	HARDWARE PROBE ERROR

WANPIPE kernel modules are NOT loaded.  

Possible Errors
---------------
    1. No wanpipe cards installed on this 
       system, hw probe failed ('wanrouter hwprobe').

    2. WANPIPE kernel modules are not installed
       correctly. 
    	 eg: '$MODULE_LOAD wanpipe' fails.

EOM
		;;


	NO_DEV_IN_PROBE)
		cat <<EOM >$TEMP

	HARDWARE PROBE ERROR

No WANPIPE hardware found on this computer.
Proceed with manual hardware configuration.

Possible Errors
---------------

    1. No wanpipe cards installed on this 
       system, hw probe file is empty.
         cat /proc/net/wanrouter/hwprobe (Linux OS)
         wanrouter hwprobe (FreeBSD/OpenBSD/NetBSD)

    2. No WANPIPE devices capable of supporting the
       selected protocol. (eg: NO ADSL card)

    3. WANPIPE kernel modules are not installed
       correctly. 
    	 eg: '$MODULE_LOAD wanpipe' fails.	

EOM
		;;

	*)
		return
		;;
	esac

	text_area
}

function warning () {

	local opt=$1

	case $opt in

	EDIT_NOT_EXIST)
		cat <<EOM >$TEMP

	!!! WANCONFIG WARNING !!!

   Wan Device 'wanpipe$DEVICE_NUM' does not exist.

          Would you like to create it ?

Options:
-------
   YES : Create new confguration for wanpipe$DEVICE_NUM
   NO  : Choose a different Wan device name
EOM
		;;

	get_seven_bit_hdlc)
		cat <<EOM >$TEMP
		
	SEVEN BIT HDLC 

Enable seven bit hdlc engine.
Used by Voice over IP DS0's.

YES: Enable
NO:  Disable

Default: NO
EOM
		;;

	ADSL_WATCHDOG)
		cat <<EOM >$TEMP

	ADSL LINE WATCDOG

The ADSL Line Watchdog will send out
ATM OAM Loopback request to the remote 
side every 10 seconds.  If the 4 consecutive
ATM requests fail, the line will be 
declared inactive, and driver will set the
S518 card into Re-training mode.

Note: Some networks do not support ATM OAM 
      Loopback requests.  In that case, driver
      will automatically disable the watchdog.
      
      Thus, its always safe to enable this option.

Options:  
-------
   YES:  Eanble ADSL Line Watchdog
   NO:   Disable ADSL Line Watchdog 

Default: YES
	
EOM
		;;

	ETH_VC_PPPoE)
		cat <<EOM >$TEMP

	!!! WANCONFIG WARNING !!!

   You have select Bridged Ethernet VC over ATM
ATM protocol with PPPoE.

This is not a standard configuration for PPPoE.
Usually the ATM protocol used is Bridged Ethernet
LLC over ATM

Proceed only if you now what you are doing !

Options:
-------
   YES : Stay with current configuration
   NO  : Go back and let me change to ETH LLC over ATM

EOM
		;;

	NEW_EXIST)

		cat <<EOM >$TEMP

	!!! WANCONFIG WARNING !!!

  Wan Device 'wanpipe$DEVICE_NUM' already exists. 

          Would you like to edit it ?  

Options: 
-------
   YES : Proceed to edit wanpipe$DEVICE_NUM 
   NO  : Proceed to overwrite wanpipe$DEVICE_NUM 
         configuration
EOM
		;;

	ADSL_ATM_autocfg)
		cat <<EOM >$TEMP

	ADSL ATM AUTO CONFIGURATION

If this option is enabled, the ATM layer
will autodetect the VCI and VPI numbers.


Options:
--------
   YES  : Enable ATM auto configuration
   NO   : Disable ATM auto cfg, manually 
          specify VCI and VPI values.

EOM
		;;

	MULTILINK_OPT)
		cat <<EOM >$TEMP

	     PPP MULTILINK SUPPORT    	

PPP MULTILINK protocol can bundle multiple physical
connections into a single logical channel.  Choose
this option if you want to bundle multiple wanpipe
connections into one.

The first MULTILINK device is called the MASTER.
This device will be configured with a network
interface and IP address.  

All subsequent wanpipe devices should be configured
as SLAVE so they can be bound together with the
MASTER device, under its IP address.

IMPORTANT
The remote side must support the MULTILINK
         protocol !

     Would you like to enable the PPP MULTILINK
                  support?
Options:
-------
   YES : Enable MULTILINK Support
   NO  : Disable MUTLTILINK Support

EOM
		;;

	MULTILINK_MASTER)
		cat <<EOM >$TEMP

	MUTLTILINK MASTER

The first MULTILINK device is called the MASTER.
This device will be configured with a network
interface and IP address.  

All subsequent wanpipe devices should be configured
as SLAVE so they can be bound together with the
MASTER device, under its IP address.
 
    Should this WANPIPE device be considered a 
                 MULTILINK MASTER?  

Options:
-------
   YES : Configure this device as MULTILINK MASTER
   NO  : Configure this device as MULTILINK SLAVE

EOM
		;;

	PAPCHAP_SECRETS_UPDATE)
		cat <<EOM >$TEMP

Enable PAP/CHAP authentication and
update the /etc/ppp secrets files.
The original pap/chap secrets files will
be saves as *.orig.

Options:
-------
   YES: Enable PAP/CHAP authentication
   NO:  Do not enable PAP/CHAP authentication

EOM
		;;
	
	PPP_DIR_NOT_FOUND)
		cat <<EOM >$TEMP

Warning: /etc/ppp directory not found!
Make sure that the pppd daemon has been 
installed.

Would you like me to create the /etc/ppp
directory?  

Options:
--------
   YES: Create /etc/ppp directory
   NO:  Cancel PPPD configuration

EOM
		;;
	
	AUTO_PCI_CFG)
		cat <<EOM >$TEMP
		
	S514 PCI SLOT AUTO-DETECTION

Select this option to enable PCI SLOT autodetection.
Otherwise, you must enter a valid PCI SLOT location
of your S514 card. 

NOTE: This option will only work for single S514
      implementations. 

Options:
-------
   YES : Enable,  Auto detect a S514 PCI Slot 
   NO  : Disablem Enter a PCI Slot manually.דדדהדדדדדדדד

EOM
		;;

	X25_CALL_LOGGING)
		cat <<EOM >$TEMP

	Enable/Disable X25 Call Logging

This option is used to enable or disable
x25 call logging.  The logs can get very
large when there are high number of
calls being made continously.  For this
reason, the call logging should be left
disabled in production systems.  

Options:
-------
   YES : Enable, X25 Call logging 
   NO  : Disable X25 Call loggingדדדה

EOM
		;;


	SAVE_CONFIG)
		cat <<EOM >$TEMP

	     SAVE CONFIGURATION    	

     Would you like to save your wanpipe$DEVICE_NUM
               configuration ?

Options:
-------
   YES : Save Configuration and Exit
   NO  : Do not save and Exit

EOM
		;;
	ARE_YOU_SURE)
		cat <<EOM >$TEMP

	!!! CONCLUDE CONFIGURATION !!!

	Are you sure you want conclude the
	configuration for device wanpipe$DEVICE_NUM ?


Options:
--------
   YES : Proceed to EXIT with an option to save. 
   NO  : Go back 

EOM
		;;


	CREATE_TTYDEV)
		cat <<EOM >$TEMP

	Device /dev/ttyWP$TTY_MINOR does not exist!
	
The /dev/ttyWP$TTY_MINOR must be created if pppd daemon
is going to be used with the WANPIPE TTY driver.

You can ether create device manually or wancfg can do
it for you:
	ex: mknod -m 666 /dev/ttyWP$TTY_MINOR c 226 $TTY_MINOR

  	Would you like to create a /dev/ttyWP$TTY_MINOR
                        device?
		
Options:
--------
  YES: Create /dev/ttyWP$TTY_MINOR
  NO:  Skip this step, I will create it manually. 

EOM
		;;

	CREATE_PPPDOPT)
		cat <<EOM >$TEMP

	PPPD DAEMON CONFIGURATION

   Would you like to install a default 
$TTY_MODE pppd configuration file in /etc/ppp 
directory for device /dev/ttyWP$TTY_MINOR ?

The configuration file will be called
/etc/ppp/options.ttyWP$TTY_MINOR.

NOTE: Refer to the WanpipeConfigManual.(pdf/txt)
      for more information.

Options:
--------
  YES: Install the $TTY_MODE config file.
  NO:  I will do it manually.

EOM
		;;

	PROTOCOL_CHANGE)
		cat <<EOM >$TEMP

	!!! WANCONFIG WARNING !!!

	If you change the protocol, your 
current wanpipe configuration will be erased.

  Would you like to change the protocol ?


Options:
-------
   YES: Proceed to change the protocol.
   NO : Do not change the protocol.
EOM
		;;
	IP_ADDR)
		cat <<EOM >$TEMP

	Illegal IP address specified !!

        Would you like to try again ?

Note:
----
IP addresses are written as four dot-separated 
decimal numbers from 0 to 255 
(e.g. 192.131.56.1)

Options:
--------
   YES : Input a new IP address again
   NO  : Stick with the previously defined 
         address

EOM
		;;

	BAD_STRING)
		cat <<EOM >$TEMP

            !!! STRING ERROR !!! 
	
      You have selected an invalid string.

        Would you like to try again ?

Options:
-------
   YES : Input a new string again
   NO  : Use the previously defined string.
	
EOM
		;;

	get_default)

		cat <<EOM >$TEMP

            SET DEFAULT ROUTE
	
    Would you like to set up this interface
            as a default route ?

Options:
-------
   YES:  Set Default Route
   NO :  No Default Route   

EOM
		;;
	get_dyn_intr)
	
		cat <<EOM >$TEMP

         DYNAMIC INTERFACE CONFIGURATION
	
   Would you like to enable dynamic interface
               configuration ? 

   For more information please refer to the help
         option in the previous menu.

Options:
-------
   YES:  Enable  dynamic interface configuration
   NO :  Disable dynamic interface configuration   

EOM
		;;

	get_slarp)
		cat <<EOM >$TEMP
	
	CHDLC IP ADDRESSING MODE

    Would you like to use dynamic IP 
           addressing mode ?

Opitons:
-------
   YES : Use dynamic IP addressing
   NO  : Use static IP addressing

EOM
		;;

	atm_oam_loopback)
		
		cat <<EOM >$TEMP

         ATM OAM LOOPBACK REQUEST
	
   Would you like to enable ATM OAM loopback
   requests. i.a ATM PING ? 

   ATM OAM Loopback Requests, or ATM PING can 
   be used for pvc activation or a pvc keepalive
   check. Usullay used for testing the PVC.
   
   By default this option should be set to NO
   because the SWITCH is usually the one to
   sends the OAM loopback requests.
  
Options:
-------
   YES:  Enable OAM Loopback Requests 
   NO :  Disable OAM Loopback Requests

EOM

		;;

	atm_oam_cc_check)

		cat <<EOM >$TEMP

         ATM OAM CONTINUITY CHECK
	
   Would you like to enable ATM OAM Continuity
   Checking. i.e ATM Keepalive packets ? 

   ATM OAM Continuity Requests, or ATM Keepalive
   packets are used to indicat to the remote
   SWITCH that the PVC is active.

   The remote SWITCH will bring the PVC down
   if it doesn't receive number of CC packets
   in a defined period of time.
   
   By default this option should be set to YES
   unless the SWITCH explicity specifies that
   Continuity packets are not to be used.
  
Options:
-------
   YES:  Enable OAM Continuity Checking 
   NO :  Disable OAM Continuity Checking

EOM
		;;

	atm_arp)

	cat <<EOM >$TEMP

         ATM ARP REQUEST
	
   Would you like to enable ATM ARP Requests ?

   ATM ARP Requests are used to negotiate IP 
   information between WANPIPE and the SWITCH.
  
   Some SWITCHES expect clients to send ATM
   ARPs before the PVC is enabled.
   
   By default this option should be set to YES
   unless the SWITCH explicity specifies that
   ATMARPs are not to be used.
  
   NOTE: This option is only available for
         Classical IP over ATM Encapsulation.
  
Options:
-------
   YES:  Enable ATM ARP Requests 
   NO :  Disable ATM ARP Requests

EOM

		;;
		
	mlinkppp_bundle)

	cat <<EOM >$TEMP
		Multi-Link PPP

   Do you want this bundle will loaded by default?

Options:
-------
   YES:  This will load bundle configuration 
	 automaticaly after running `mpd`.
   NO :  This will not load bundle configuration
	 after running `mpd'.

EOM

	esac

	text_yesno 20 55
}



