################################################################################
# wanpipe-ftdm RPM Spec file
#
# Maintained by: Konrad Hammel (konrad@sangoma.com)
#
#	2010-07-20	-	Initial version
#	2010-07-21	-	Added supported for BRID
#					Added check for existing wanrouter.rc
#	2010-07-21	-	Updated so that libSangoma is added to rpm
#	2010-08-26	-	Added support for syslog configuration
#
################################################################################


# OPTIONS
#
# Default: TDMAPI, FreeTDM  
#   rpmbuild -tb [wanpipe.x.tgz]  --define 'kernel $(uname -r)' --defeine 'ksrc /lib/modules/$(uname r)/build' --define 'karch $(uname -m)' 
#
# With DAHDI Support
#   rpmbuild -tb [wanpipe.x.tar.gz] --define 'with_dahdi 1' --define 'dahdi_dir /usr/src/dahdi' --define 'dahdi_ver 2.3.1'
#
# With ZAPTEL Support
#   rpmbuild -tb [wanpipe.x.tar.gz] --define 'with_zaptel 1' --define 'zaptel_dir /usr/src/zaptel' --define 'zaptel_ver 1.4.12'
#

%define NAME			wanpipe
%define VERSION           7.0.10
%define RELEASE			0
%define KVERSION		%{?kernel}
%define KSRC			%{?ksrc}
%define KARCH			%{?karch}
%define LIBPREFIX		%{?libprefix}
%define MODULES_DIR		/lib/modules
%define META_CONF		/etc/wanpipe/wanrouter.rc

%define DAHDI_DIR		/usr/src/dahdi
%define DAHDI_VER 0
%define ZAPTEL_DIR		/usr/src/zaptel
%define ZAPTEL_VER 0
%define debug_package %{nil}


%{!?kernel: %{expand: %%define KVERSION %(uname -r)}}
%{!?ksrc: %{expand: %%define KSRC /lib/modules/%(uname -r)/build}}
%{!?karch: %{expand: %%define KARCH %(uname -m)}}
%{!?libprefix:%define LIBPREFIX /usr }

%define KVER_REL %(echo %{KVERSION} | sed -e s/-/./g)

%define RELEASE kernel.%{KVER_REL}


%{?dahdi_dir: %define DAHDI_DIR %{?dahdi_dir} }
%{?with_dahdi: %{expand: %%define DAHDI_VER %(cat %{DAHDI_DIR}/include/dahdi/version.h | grep VER | cut -d'"' -f2) }}
%{?with_dahdi: %define RELEASE kernel.%{KVER_REL}.dahdi.%{DAHDI_VER}}

%{?zapel_dir: %define ZAPTEL_DIR %{?zapel_dir} }
%{?with_zaptel: %{expand: %%define ZAPTEL_VER %(cat %{ZAPTEL_DIR}/version.h | grep VER | cut -d'"' -f2) }}
%{?with_zaptel: %define RELEASE kernel.%{KVER_REL}.zaptel.%{ZAPTEL_VER}}



Summary:        Sangoma WANPIPE package for Linux. It contains the WANPIPE kernel drivers and configuration/startup/debugging utilities for Linux.
Name:           %{NAME}
Version:        %{VERSION}
Release:        %{RELEASE}
License:        GPL
Group:          Applications/Communications
Vendor:         Sangoma Technologies Inc.
Url:            www.sangoma.com
Source0:		ftp://ftp.sangoma.com/%{name}-%{version}.tgz
Group:          Networking/WAN

BuildRoot: %{_tmppath}/%{name}-%(id -un) 

AutoReq: 		1		

Requires: coreutils
Requires: kernel-%{KARCH} = %{KVERSION}


%define build_for_dahdi 0
%{?with_dahdi:%define build_for_dahdi 1} 	
%define build_for_zaptel 0
%{?with_zaptel:%define build_for_zaptel 1} 	

%{?with_dahdi:Requires:  dahdi = %{DAHDI_VER}} 
%{?with_zaptel:Requires:  zaptel = %{ZAPTEL_VER}} 



################################################################################
%description
################################################################################
Linux Drivers for Sangoma AFT Series of cards and S Series of Cards.

################################################################################
%prep
################################################################################
%setup

################################################################################
%build
################################################################################

%if %{build_for_dahdi}
make KDIR=%{KSRC} KVER=%{KVERSION} WARCH=%{KARCH} INSTALLPREFIX=%{buildroot} LIBPREFIX=%{LIBPREFIX} dahdi DAHDI_DIR=%{DAHDI_DIR}
%else
%if %{build_for_zaptel}
make KDIR=%{KSRC} KVER=%{KVERSION} WARCH=%{KARCH} INSTALLPREFIX=%{buildroot} LIBPREFIX=%{LIBPREFIX} zaptel ZAPDIR=%{ZAPTEL_DIR}
%else
make KDIR=%{KSRC} KVER=%{KVERSION} WARCH=%{KARCH} INSTALLPREFIX=%{buildroot} LIBPREFIX=%{LIBPREFIX} freetdm 
%endif
%endif


################################################################################
%install
################################################################################
mkdir -p %{buildroot}/usr
mkdir -p %{buildroot}/etc

make INSTALLPREFIX=%{buildroot} LIBPREFIX=%{LIBPREFIX} KVER=%{KVERSION} install
#make INSTALLPREFIX=%{buildroot} LIBPREFIX=%{LIBPREFIX} KVER=%{KVERSION} install_pri
#make INSTALLPREFIX=%{buildroot} LIBPREFIX=%{LIBPREFIX} KVER=%{KVERSION} install_bri

# remove the self-referencing symbolic link
rm -rf %{buildroot}/usr/include/wanpipe/linux
rm -rf %{buildroot}/var/tmp

# remove /etc/wanpipe/wanrouter.rc...we need to build this on the install so
# that it doesn't overwrite the existing one
rm -rf %{buildroot}/etc/wanpipe/wanrouter.rc


################################################################################
%check
################################################################################
#nothing to do

################################################################################
%clean
################################################################################
[ %{buildroot} != "/" ] && rm -rf %{buildroot}

################################################################################
%pre
################################################################################
#nothing to do

################################################################################
%post
################################################################################
# re-create the self referencing symbolic link in the include dir
ln -s /usr/include/wanpipe/ /usr/include/wanpipe/linux

#update ldconfig
ldconfig

# check dependancies for the new modules
depmod -ae -F /boot/System.map-%{KVERSION} %{KVERSION}

# check if the /etc/wanpipe/wanrouter.rc exists, if not create it
if [ -e %{META_CONF} ]; then
	echo "Found existing Wanpipe configuration..."
else
	echo "No existing Wanpipe configuration found..."
	cp /etc/wanpipe/samples/wanrouter.rc %{META_CONF}
fi

#update syslog
if [ -e  /etc/syslog.conf ]; then
	mysyslog='syslog'
elif [ -e /etc/rsyslog.conf ] ; then
	mysyslog='rsyslog'
else
	mysyslog=" "
fi
if [  $mysyslog != " " ]; then
	eval "grep "local2.*sangoma_mgd" /etc/$mysyslog.conf" > /dev/null 2> /dev/null
	if [ $? -ne 0 ]; then
		eval "grep "local2" /etc/$mysyslog.conf " > /dev/null 2> /dev/null
		echo -e "\n# Sangoma Media Gateway log" > tmp.$$
		echo -e "local2.*                /var/log/sangoma_mgd.log\n" >> tmp.$$
		eval "cat /etc/$mysyslog.conf tmp.$$ > tmp1.$$"
		cp -f tmp1.$$ /etc/$mysyslog.conf
		eval "/etc/init.d/$mysyslog restart"
	fi
	eval "grep "local3.*sangoma_bri" /etc/$mysyslog.conf" > /dev/null 2> /dev/null
	if [ $? -ne 0 ]; then
		eval "grep "local3" /etc/$mysyslog.conf " > /dev/null 2> /dev/null
		echo -e "\n# Sangoma BRI Daemon (smg_bri)  log" > tmp.$$
		echo -e "local3.*                /var/log/sangoma_bri.log\n" >> tmp.$$
		eval "cat /etc/$mysyslog.conf tmp.$$ > tmp1.$$"
		cp -f tmp1.$$ /etc/$mysyslog.conf
		eval "/etc/init.d/$mysyslog restart"
	fi
fi
if [ -f tmp1.$$ ]; then
	rm -f  tmp1.$$
fi
if [ -f tmp.$$ ]; then
	rm -f  tmp.$$
fi

# install start scripts
rm -f /etc/init.d/wanrouter
ln -s /usr/sbin/wanrouter /etc/init.d/wanrouter

\cp -f /etc/wanpipe/wancfg_zaptel/setup-sangoma /usr/local/sbin/setup-sangoma
chmod 755 /usr/local/sbin/setup-sangoma

chkconfig --add wanrouter

# we're done...print a happy message
cat <<EOM
*** Sangoma Wanpipe was successfully installed.
    Hardware Probe: /usr/sbin/wanrouter hwprobe
    Wanpipe Config: /usr/sbin/wancfg_fs
    Wanpipe Start : /usr/sbin/wanrouter start

EOM

################################################################################
%preun
################################################################################

# If action is uninstall ($1 = 0) then proceed,
# otherwise ($1 = 1) it is an upgrade
if [ "$1" = "0" ]; then
  echo "Uninstalling WANPIPE..."
  # Remove initialization scripts.
  chkconfig --del wanrouter
  if [ -f /etc/init.d/wanrouter ]; then
    rm /etc/init.d/wanrouter
  fi
  if [ -d /usr/include/wanpipe/linux ]; then
    rm /usr/include/wanpipe/linux
  fi
fi

################################################################################
%postun
################################################################################
# If action is uninstall ($1 = 0) then proceed,
# otherwise ($1 = 1) it is an upgrade
if [ "$1" = "0" ]; then
  echo "Done"
fi


################################################################################
%files
################################################################################
/*

################################################################################

%changelog

* Tue Feb 11 2014 Nenad Corbic <ncorbic@sangoma.com> -  7.0.10
==================================================================

- Support for Dahdi 2.9
- Fixes for DMA resync on T116 and A116 cards
  Used to resync DMA in case of T1/E1 sync loos.

* Mon Jan 7 2014 Nenad Corbic <ncorbic@sangoma.com> -  7.0.9
==================================================================

- Support for Dahdi 2.8
- Support for Centos 6.5
- Support for kernels up to 3.12
- Fixes A116 clocking for dahdi when link is down
- Updates to A116 for possible NMI interrupt erros on some motherboards.

* Wed Nov 14 2013 Nenad Corbic <ncorbic@sangoma.com> -  7.0.8
==================================================================

- Updated wanpipemon to trace on multiple dchan in a single span
  eg: wanpipemon -i w1g1 -chan 1 -c trd
  If chan is not specificed then all dchans will be traced together.
- Updated sample tapping application.
  Allow configuration for dchan and seven bit hdlc
- Fixed GSM drivers for Windows

* Mon Jul 15 2013 Nenad Corbic <ncorbic@sangoma.com> -  7.0.5
==================================================================

- Updated for Dahdi 2.7
- Fixed full dahdi startup on wanpipe dahdi mode.

* Wed May 22 2013 Nenad Corbic <ncorbic@sangoma.com> -  7.0.3
==================================================================

- Fixed file structure of sample_data_tapping application
- No driver changes from 7.0.2


* Fri May 10 2013 Nenad Corbic <ncorbic@sangoma.com> -  7.0.2
==================================================================

- Added sync error statistic in wanpipeomon Ta command
  Sync error indicates clocking or synchronizatio errors
  on T1/E1 lines.  Provides added statistic to debug line,
  slip errors on T1/E1.

- Fixed DAHDI reconfiguration issues.
  running dahdi_cfg multiple times will not bring down T1/E1 link
  Added config option to change clocking from MASTER to NORMAL
  from dahdi system.conf

- New sample_data_tapping sample application for T116 and other
  tapping boards.


* Tue Mar 19 2013 Nenad Corbic <ncorbic@sangoma.com> -  7.0.1
==================================================================

- Update to wancfg_dahdi configuration for GSM
- Update to setup-sangoma configuration for GSM
- Potential bug on digital board startup due to un-initialized variable
  Introduced in 7.0.0
- Update for Centos 6.4
  New kernel broke drivers
- Update the DTMF poll frequency to 10ms
  This will proved much more responsive DTMF events reporting.
  Adds negligible load in performance testing.
- Fixed a possible memory corruption issue on cdev write.
  This could affect Linux FreeSWITCH or TDMAPI/Libsangoma Customers.
  Issues have been reported by customers on very few random systems. 
  Finally was able to reproduce in lab and fix it for good.
  Does not affect Asterisk/DAHDI.


* Thu Feb 10 2013 Nenad Corbic <ncorbic@sangoma.com> -  7.0.0
==================================================================

- Updated Major revision in order to syncronize versions with Windows.
  There is no major driver change from 3.5.28 to 7.0.0
- Support for T116 16 Port T1/E1 Tapping Card
- Added sample_data_tapping application for tapping customers
  wanpipe-7.0.0/api/libsangoma/examples/sample_data_tapping
- Wanpipe DAHDI Interation
  Updated wanpipe dahdi integration
  Allow dahdi system.conf to configure sangoma drivers
  This feature removes the need for Sangoma wanpipe configuration files.
  Currently this feature is being used by OEM vendors.
  More information on Sangoma wiki - Asterisk Section.
- Bug fix in UBXFXO causing a kernel crash.
- Fixed for 3.5.4 kenrel
- Updated rx/tx fifo thresholds in case of excessive fifo error recovery.
- Default lowered to 50 per sec
- Allows configuration of rx/tx fifo thresholds


* Thu Aug 10 2012 Nenad Corbic <ncorbic@sangoma.com> -  3.5.28
==================================================================

- Added A116 card support
- Minor fixes for R2 CAS for NBE

* Thu May 6 2012 Nenad Corbic <ncorbic@sangoma.com> -  3.5.27
==================================================================

- GSM Bug fix for dahdi
  Fixes startup issues for Asterisk/Dahdi mode. 

- Wanpipe API/libsangoma device read bug on 64bit machines
  This issue can cause infrequent soft lockups on some 64bit kernels
  Affects FreeSWITCH and FreeTDM and libsangoma mode only.
  Does not affect Asterisk/Dahdi mode.

- Fixed B700 issues related to the GSM code.

- Fixed Setup to properly build wan protocols for latest kernels.


* Tue Apr 24 2012 Nenad Corbic <ncorbic@sangoma.com> -  3.5.26
==================================================================

- Wanpipe W400 GSM support: Asterisk/Dahdi & FreeSWITCH/FreeTDM
- Wanpipe B610 FXS support
- Fixed Dahdi 2.5 and higher hwec autodetection 
  For dahdi 2.5 and higher the HWEC option was needed in 
  dahdi/system.conf to enable onboard hwec. 
  This fix allows default mg2 software echo to be specified
  in dahdi/system.conf, and dahdi will autodetect the sangoma onboard hwec.
- Fixed sangoma hwec boards to use dahdi software ec if hwec is turned off.
  Wanpipe driver did not allow sangoma hwec enabled boards to use dahdi 
  software ec even if hwec was turned off in wanpipe1.conf
  This has now been fixed. 
  If TDMV_HWEC=NO is set in wanpipe1.conf dahdi will now use software ec.
- Fixed BRI dchan reliability 
- Fixed BRI for 64bit 
  The fix for audio issue in 3.5.25 caused the issue on 64bit.
- Dahdi build bug fixes for Trixbox
- Zaptel build bug fixes from 3.5.25
- Build fixes for 3.1.X kernel.
- Fake polarity feature used for Euro Caller ID
- E1 NCRC CAS Framer Configuration for Latin America
  Previous NCRC Framer config did not work in all cases.
  Affects protocols such as MFC-R2 or CAS
- Analog boards with network sync option set to yes: 
  Default to fax mode buffering.
  Use "when full" dahdi buffer policy. 
  Improves faxing from PRI to Analog with sync cable.
- Fixed wancfg_dahdi to:
  start asterisk using safe_asterisk
  to properly detect dahdi version



* Tue Feb 21 2012 Nenad Corbic <ncorbic@sangoma.com> -  3.5.25
==================================================================

- Dahdi 2.6 support
- Linux 3.2.6 support
- Support for B610 Single FXS board
- Support for B500 4 port BRI board
- Reduced memory foot print of the driver
  by removing some unused statistics structures
- New HWEC firmare v1.7.4 fixes delayed fax issus 
  when fax is placed through the echo canceller.
- Fixed BRI noise issue: Where a BRI channel could
  be started in a corrupted state.
- New A108 firmware V44
  Fixes unreliable front end chip access on some
  newer sandy bridge architectures.
  Effect of this bug was unreliable T1/E1 link connection.
- Added verbose help on wan_ec_client

* Tue Nov 15 2011 Nenad Corbic <ncorbic@sangoma.com> -  3.5.24
==================================================================

- Bug fixes in Setup
- Bug fixes in wancfg_dahdi
- Updated scripts for Debian/Ubuntu
- Added MTP1 msu/lssu filter option
- New LINEAR mapping of A108 ports for use with T3Mux
- Updates and fixes to MTP1 engine
- Fix for potential memory leak in proc file system
- Fix for analog dma sync on 64bit kernels 
- Dahdi BRI fix: wakeup of powered down line
- Fixed RPM spec for trixbox

* Wed Sep 07 2011 Nenad Corbic <ncorbic@sangoma.com> -  3.5.23
==================================================================

- Bug fixes in Setup
- Bug fixes in compiling and configufing old deprecated SMG/BRI
- Fixed rpmbuild using Setup

* Wed Aug 24 2011 Nenad Corbic <ncorbic@sangoma.com> -  3.5.22
==================================================================

- Bug introducted in .21 release for analog board.
  Changed the way wanpipe enumerates analog channels
  breaks backward compatibility. Reverted to original.

- Setup install script update
  Removed old and legacy products out of Setup compile options.


* Tue Aug 23 2011 Nenad Corbic <ncorbic@sangoma.com> -  3.5.21
==================================================================

- T1 AMI fix
- Fixed for 2.6.39 and 3.0.1 kernels
- Fixed for dahdi 2.5
- Fixed BRI wancfg_dahdi config
- Added HW Echo Cancellation Clock failover.
  If EC is using clock from port 1 and that port goes down.
  The clock source will be taken from another connected port.
- Minor bug fix in hdlc test sample app
- Bug fix libsangoma: multilple wait object issue
- BRI for DAHDI
- B601 receives timing from analog port when T1 is down.
- Libsangoma ss7 hw config status
- Edge cases bug fixes on multi port restart
- Updated v44 for A104 firmware
- Analog 64bit 8GIG memory issue dma sync fix
- Support for ss7 firmware V44 on A104 only.
- Fixed logger - caused slow prints on some kernels due
  to use of vprintk
- Added Global Poll IRQ mode for efficient high density
  hdlc tx/rx
- wanpipemon added led blink option to idetnify port
  Removed the led on/off 
  wanpipemon -i w1g1 -c dled_blink -timeout 10 #with timeout
  wanpipemon -i w1g1 -c dled_blink             #without timeout              
- wanpipemon documented performance statatistics
  wanpipemon -p aft
- wanpipemon added led ctrl
  Used to visually identify a port from software
  wanpipemon -p aft for documentation           
  Added this feature in libsangoma as well.
- Minor updates in wanpipe spec file
- Bug fixes in wancfg legacy
- aftpipemon fixed for 24port analog
- B601 mtu fix for FreeSWITCH
  Causes audio issues
- Added performance stats
- Fixed B601 for TDMAPI & FreeSWITCH Dchannel 
- Confirmed that B601 works for Asterisk (dahdi hdlc mode)
- Fixed rescan feature
- Libsangoma builds by default now 
  wanec utilities now depend on libsangoma
- Libsangoma contains the full wanec api
- Libsangoma wanec API for TDM API
- Added libsangoma hwec functions for Linux
- Hwec audio_mem_load added to the api.


* Mon Apr 11 2011 Nenad Corbic <ncorbic@sangoma.com> -  3.5.20
==================================================================

- Fixed customer id read
- Added hwrescan libsangoma command
- Updated the analog ring debouncing threshold
  so that ring is properly debounced
- Fixed AIS alarm clear flag bug
  The AIS alarm flag was not being cleared in the driver.
- New Octasic Image 1.6.2 
  Fix for AGC (Automatic Gain Control)
  AGC now meets Microsoft Audio Quality Spec
- Fixed start script for ubuntu

* Fri Mar 1 2011 Nenad Corbic <ncorbic@sangoma.com> -  3.5.19
==================================================================

- Fixed scripts for Ubuntu
- TDM API updated Tone Event API to include tone type
  DTMF, FAX_1100, FAX_2100, FAX_2100_WSPR
- Fixes for latest 2.6.36 linux kernel
- Fixes for TTY Driver for 2.6.32 linux kernel
- BRI default idle set to 0xFF so it does not interfere
  with multi-port mode.
- BRI NT & TE activation/deactivation logic update.
- Dahdi Yellow alarm reporting fix
  Wanpipe driver did not report yellow alarms properly to dahdi
- T1 Automatic AIS on LOS option now optional
  Previoulsy the T1 code enabled automatic AIS on LOS by default.
  This is now an option TE_AIS_AUTO_ON_LOS=YES|NO
  Default behaviour is to send Yellow alarm on link down.
- Fixed a TE1 startup race condition bug.
  It was possible for T1/E1 interrupt to occour before
  configuration was completel
- Fixed wanpipe.spec for dahdi RPM build
- Added serial clock recovery feature


* Fri Nov 22 2010 Nenad Corbic <ncorbic@sangoma.com> -  3.5.18
==================================================================

- BRI Multi-Point fix idle 0xFF
- BRI fix for XEN virtualization
- Fixed front end interrupt issue 
- wancfg_fs (fix the little bugs)
- update to support the latest dahdi-linux
- fix dahdi_scan, now reporting the right values instead of showing blank
- new echo canceler image
- fix for the echo canceler for A500 for freetdm mode. 
  HWEC will turn on when there is a call on the bri line.
- Added SW HDLC into the core. B
- B601 is now supported on FreeTDM/FreeSWITCH and TMDAPI
- Added wan_fxotune utility to utils directory
  Used to tune fxo boards under TDM API or FreeSWITCH mode.



* Fri Oct 08 2010 Nenad Corbic <ncorbic@sangoma.com> -  3.5.17
===================================================================

- Critical Bug fix in WAN mode.  Bug introduced in 3.5.16 


* Fri Sep 27 2010 Nenad Corbic <ncorbic@sangoma.com> -  3.5.16
===================================================================

- Dahdi 2.4 Support
- Fixed BRI B500/B700 hwec enable on call start caused in 3.5.12 release.
- Bug fix in voice+data mixed mode where dchan could get stuck due to
  dma overruns.
- Bug fix in tdmapi where excessive memory was allocated on pre-allocation buffers.
- Bug fix tdmapi defaults to 20ms chunk size instead of 10ms
- Bug fix broken support for A101/2 legacy EOL boards. 
- New XEN Support 
  TDM Voice will now work properly on xen virtualized machines
- Fix for 64bit 8gig issues
- New rpmbuld spec files.
  rpmbuild -tb wanpipe-3.5.16.tgz
  rpmbuild -tb wanpipe-3.5.16.tgz --define 'with_dahdi 1' --define 'dahdi_dir /usr/src/dahdi'


* Fri Aug 27 2010 Nenad Corbic <ncorbic@sangoma.com> -  3.5.15
===================================================================

- Fixed B600 and B601 warning messages introduced in 3.5.14
- New Firmware for A108&A104 V43
  Fixes PCI parity errors on new dell,ibm boxes
- Libsangoma added rw fe reg, and rx/tx gains
- Build script does not polute the linux source any more.
- Bug fix in B800 detect code
- Fixed wanfcg_fs for freeswitch 
- 


* Tue Jun 29 2010 Nenad Corbic <ncorbic@sangoma.com> -  3.5.14
===================================================================

- Fixes stop script for Asterisk 1.6.2
- Compile fix in legacy api sample code
- Skipped .13 releaes went straight to 14 :)

* Mon Jun 28 2010 Nenad Corbic <ncorbic@sangoma.com> -  3.5.12
===================================================================

- Fixed Dahdi 2.3 Support
- Fixed FreeSwitch Openzap HardHDLC option for AFT boards
- Fixed wanpipemon support for non aft boards.
- Merged USB FXO code from 3.6 release
- USB FXO bug fix for 2.6.32 kernels
- Support for B800 Analog board
- Fixed alarm reporting in DAHDI/ZAPTEL

- Added Extra EC DSP Configuration Options
  HWEC_OPERATION_MODE	= OCT_NORMAL 	# OCT_NORMAL: echo cancelation enabled with nlp (default)  
										# OCT_SPEECH: improves software tone detection by disabling NLP (echo possible)
										# OCT_NO_ECHO:disables echo cancelation but allows VQE/tone functions. 
  HWEC_DTMF_REMOVAL		= NO          	# NO: default  YES: remove dtmf out of incoming media (must have hwdtmf enabled)
  HWEC_NOISE_REDUCTION	= NO 			# NO: default  YES: reduces noise on the line - could break fax
  HWEC_ACUSTIC_ECHO		= NO			# NO: default  YES: enables acustic echo cancelation
  HWEC_NLP_DISABLE		= NO			# NO: default  YES: guarantees software tone detection (possible echo)   
  HWEC_TX_AUTO_GAIN		= 0    			# 0: disable   -40-0: default tx audio level to be maintained (-20 default)
  HWEC_RX_AUTO_GAIN		= 0				# 0: disable   -40-0: default rx audio level to be maintained (-20 default)
  HWEC_TX_GAIN			= 0				# 0: disable   -24-24: db values to be applied to tx signal
  HWEC_RX_GAIN			= 0				# 0: disable   -24-24: db values to be applied to tx signal  

- Added AIS BLUE Alarm Maintenance Startup option
  Allows a port to be started in BLUE alarm.

  TE_AIS_MAINTENANCE = NO         	#NO: defualt  YES: Start port in AIS Blue Alarm and keep line down
									#wanpipemon -i w1g1 -c Ttx_ais_off to disable AIS maintenance mode
									#wanpipemon -i w1g1 -c Ttx_ais_on to enable AIS maintenance mode           
  
- Fixed Legacy XDLC compile	
- Fixed core edge case scenarios where
  potential race condition could occour.



* Thu Apr 08 2010 Nenad Corbic <ncorbic@sangoma.com> -  3.5.11
===================================================================

- Fix for 2.6.31 and higher kernels
- TDM API Analog rx gain feature
- Disabled default NOISE REDUCTION feature in hwec
  that was enabled in 3.5.9 release. 
- Updates to T1/E1 Loopback and BERT Test	


* Wed Jan 11 2010 Nenad Corbic <ncorbic@sangoma.com> -  3.5.10
===================================================================

- Release cleanup script earsed libsangoma.c during release packaging.
  I have update release procedure so this does not happen again.
  This release has no functionl differences aside from the missing
  file from 3.5.9 release.

* Wed Dec 30 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.9
===================================================================

- New logger dev feature
- Bug fix in tx fifo handler
- Dahdi 2.2 broke wanpipe rbs support.
- Fixed free run interrupt supported on V38 (A108)
- Fixed RBS signalling for E1 channel 31
- Added Front end Reset Detection
  -> Support for new A108 Firmware V40
- Fixed RTP TAP bug: Caused high system load on RTP TAP usage.
- Added excessive fifo error sanity check. Fixes random pci dma errors.
- HWEC: Increased EC VQE Delay: Fixes random fax failure due to hwec.
- HWEC: Check state before bypass enable.
- HWEC: Disable bypass on release
- HWEC: Enabled Noise Reduction by default
- HWEC: Enabled Auto Gain Control by default
- HWEC: To disable Noise Reduction and Gain control set
        -> HWEC_NOISE_REDUCTION_DISABLE=NO in [wanpipe] section of wanpipe1.conf
		To check if Noise Reduction or Gain control are set
		-> wan_ec_client wanpipe1 stats 1


* Thu Oct 02 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.8
===================================================================

- Bug fix in sangoma_prid PRI stack for FreeSwitch & Asterisk.
  There was a slow memory leak.


* Thu Sep 04 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.7
===================================================================

- New Telesoft PRI Stack Support for FreeSwitch & Asterisk
  For Asterisk: The new stack uses the existing Sangoma Media Gateway
                architecture currently used by SS7 and BRI.
                -> run: wancfg_dahdi or wancfg_zaptel to configure
				   for sangoma prid stack.

  For FreeSwitch: The new stack binds to openzap directly just like
                  current SS7 and BRI.
				-> run: wancfg_fs to configure freeswitch for
				        sangoma prid, brid, ss7.

- Fixed Tx Tristate 

- Updated yellow alarm handling for Dallas maxim boards
  (A101/2/4/8)

- Autodetect USB support so that driver will compile
  correctly on kernel without USB support

- Added DAHDI Red alarm for Analog


* Thu Aug 20 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.6
===================================================================

- Update to T1 Yellow Alarm handling.
  In some cases Yellow alarm did not turn off poperly causing
  line to stay down an board startup.
- Update configuration utility
  wancfg_fs updated for sangoma_prid configuration. Added wancfg_openzap
  for OpenZap Configuration 


* Mon Aug 17 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.5
===================================================================

- Dahdi 2.2 Support
- BRI Update - Added T1 timer for NT module
- AFT Core Update - optimized dma ring buffer usage
- TDM API - refractoring and optimization
- Updated for 2.6.30 kernel

- New firmawre feature for A101/2/5/8: Free Run Timer Interrupt 
  The AFT T1/E1 boards will now provide perfect timing to zatpel/dahdi
  even when the ports are not connected. The free run interrupt
  will be enabled when all zaptel/dahdi ports are down, or on
  inital board start. To test this feature just start a wanpipe 
  port with zaptel/dahdi and run zttest. 
  A108 firmare V38 
  A104/2/1/ firmware V36

- AFT T1/E1 front end update
  Added OOF alarm treshold, so that line does not go down
  on very first OOF alarm.

- Added module inc cound when zaptel/dahdi starts.
  So wanpipe drivers do not crash if one tries to unload 
  zaptel/dahdi before stopping wanpipe drivers.


* Thu Jul 07 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.4.8
===================================================================

- Updated for B700 Dchan Critical Timeout
- Fix for FAX detect on PRI
- Updated for 2.6.21 kernel TASK QUEUE REMOVAL caused 
  unexpected behaviour.
- Updated wancfg_zaptel for fax detect

* Thu Jul 03 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.4.3
===================================================================

- Added DAHDI 2.2 Support


* Thu Jul 02 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.4.2
===================================================================

- AFT 64bit update
  No need for --64bit_4G flag any more. 
  The 64bit check is now down in the driver.

- TDM API
  Updated the Global TDM Device
  This device can be used to read events an all boards configured in
  TDM API mode.

- Libsangoma verion 3.1.0
  Added a function to check if hwec is supported


* Tue Jun 30 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.4.1
===================================================================

- Sangoma MGD update v.1.48
  Disable hwec on data calls


* Mon Jun 29 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.4
===================================================================

- E1 Voice Bug fix introduced in 3.5.3 

- Removed NOISE REDUCTION enabled by default.
  The noise reduction is disabled by default and should be
  enabled using HWEC_NOISE_REDUCTION = YES 
 
- Fixed libsangoma enable dtmf events functionality



* Tue Jun 25 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.3
===================================================================

- New Makefile build system
  Note this does not replace Setup. Makefile build system can be
  used by power users.
  Asterisk
     make dahdi DAHDI_DIR=<abs path to dahdi>
	 make install
     make zaptel ZAPDIR=<abs path to zaptel>
	 make install

  FreeSwitch
     make openzap
	 make install

  TDM API 
     make all_src
	 make install

- Updated libsangoma API
  Redesigned wait object for Linux/Windows integration.

- Turned on HWEC Noise Reduction by default
  To disable noise reduction specify
  HWEC_NOISE_REDUCTION_DISABLE=YES in [wanpipe1] section of wanpipe
  config file.

- Regression tested for FreeSwitch+OpenZAP

- Updated dma buffers in ZAPTEL and TDM API mode.
- Bug fixes for Mixed Data + Voice Mode

- Bug fix on TDM API mode. 
  Flush buffers could interfere with tx/rx data.

- Added BRI DCHAN monitor in case task is not scheduled by the
  system.  Sanity check.
- Fixed libsangoma stack overflow check that failed on some kernels.


* Fri May 08 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.2
===================================================================

- B700 PCIe boards were being displayed as PCI boards in hwprobe
- Bug fix in wancfg_zaptel 

* Thu May 07 2009 Nenad Corbic <ncorbic@sangoma.com> -  3.5.1
===================================================================

- New Hardware Support
  B700 - Mixed BRI & Analog
  B600 - Analog 4FXO/FXS
  USB-FXO - USB Fxo device

- New Unified API for Linux & Windows 
  API Library - libsangoma
  Unified Voice API for Linux & Windows
  
  -More Info
  http://wiki.sangoma.com/wanpipe-api

  - SPAN mode API
  - CHAN mode API

- Unified driver for Linux & Windows
- Updated BRI Stack and Support
- New BRI A500 & B700 firmware that fixes PCI parity errors.
  On some systems A500 & B700 boards can generate parity errors.

- FreeSwitch Tested
- Update for 2.6.26 kernel

Note this is a major release. It has been fully regression
tested and stress tested in the lab and in the field.


- - END - 
