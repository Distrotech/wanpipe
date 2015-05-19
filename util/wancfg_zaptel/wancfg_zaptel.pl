#!/usr/bin/perl
# config-zaptel.pl 
# Sangoma Dahdi/Zaptel/TDM API/SMG Configuration Script.
#
# Copyright     (c) 2009-2012, Sangoma Technologies Inc.
#
#               This program is free software; you can redistribute it and/or
#               modify it under the terms of the GNU General Public License
#               as published by the Free Software Foundation; either version
#               2 of the License, or (at your option) any later version.
# ----------------------------------------------------------------------------
# Mar 09   2011  2.50   Yannick Lam     Fixed bug in wancfg_tdmapi for A500
# Dec 02   2010  2.49   Yannick Lam     Fixed wancfg_fs bug (removea all the unwsnted msg in summary, fix the sangoma_pri_spans, fixed the name of cards when inputing context name
# Nov 09   2010  2.48   Yannick Lam     Fixed wancfg_fs bug(take out asterisk and smg_ctrl stuffs, fixed context, fixed config profile)
# Oct 12   2010  2.47   Yannick Lam     Added d-channel in freetdm.conf for SMG/trillium stack(trillium=freetdm_conf ; socket mode=freetdm_conf_legacy)
# Sep 09   2010  2.46   Yannick Lam     Support for SMG/trillium stack(trillium=freetdm_conf ; socket mode=freetdm_conf_legacy)
# Aug 25   2010  2.45   Yannick Lam     B800 support
# Aug 17   2010  2.44   Yannick Lam     wancfg_fs give you option to do openzap or freetdm(conf and xml files are generated)
# Jul 19   2010  2.43   Yannick Lam     Fixed bug in wancfg_ftdm
# Jun 29   2010  2.42   Yannick Lam     Fixed issue with asterisk-1.6.2 and up for stop
# May 14   2010  2.41   Yannick Lam     Added wancfg_ftdm (script for freetdm)
# May 04   2010  2.40   Yannick Lam     Fix smg_pri.conf for wancfg_fs
# Nov 26   2009  2.39   Jignesh Patel   Fix woomera.conf for sangoma_pri & openzap.conf for E1
# Oct 29   2009  2.38	Jignesh Patel	Minor changes to signalling order dahdi start script update
# Sep 07   2009  2.37 	Jignesh Patel	Added B601 support
# Aug 27   2009  2.36	Jignesh Patel   Added smg.rc file for smg_ctrl script
# Aug 24   2009	 2.36	Jignesh Patel	Added FreeSwitch conf for T1/E1 sangoma_pri
# 										opnezap.conf.xml support for Boost-PRI/BRI and analog
# Jul 06   2009  2.35	Jignesh Patel 	Update enable/disable chan_woomera.so loading in Asterisk
# May 26   2009	 2.34	Jignesh Patel 	Added support for USB FXO HW EC/DTMF/ Dahdi
# May 5	   2009	 2.33	Jignesh Patel	Update start script for smgbri and wancfg_fs update
# Apr 28   2009  2.32					smg_ctrl boot script update
# Apr 27   2009  2,31   Yannick Lam     Added support --silent --safe_mode for smg
# Apr 20   2009  2.30   Yannick Lam     Added support for fax detect support and smg_ctrl safe_start 
# Apr 13   2009  2.29   Yannick Lam     Update for start/script with --silent
# Apr 07   2009  2.28   Jignesh Patel	Added freeswitch conf for analog and BRI
# Mar 17   2009  2.27   Jignesh Patel   Added HWDTMF Option for BRI
# Mar 10   2009  2.26   Yannick Lam     Added Config only Option 
# Jan 09   2009	 2.25	Jignesh Patel 	Removed LAW option for FlexBRI-analog
# Dec 11   2008  2.24   Konrad Hammmel  Added support for BUSID and multiple USBFXO
# Nov 26   2008  2.23	Jignesh Patel   Added support for B600 and B700 
# Oct 16   2008  2.22	Jignesh Patel	Added Support for A600 
# Oct 07   2008  2.21   Jignesh Patel 	Dahdi Soft-EC conf support for Analog & A101/2 cards
# Sep 30   2008  2.20	Jignesh Patel   Configuration Support for Dahdi
# Aug 20   2008  2.19	Jignesh Patel 	Suppor for HP TDM API for A10x added hp_a100
# 					Update A10x.pm-for SH and old a101/2cards
# Aug 1	   2008  2.17 	Jignesh Patel 	Support for Analog & a101/2 card for wancfg_tdmapi
# 					Update A10u.pm,A20x.pm,wanpipe.tdm.a10u/a200
# Jul 22   2008  2.17	Jignesh Patel	Support for FreeBSD ztcfg path 
# Jun 6	   2008  2.16	Jignesh Patel	Added Zaptel Timer option for a500 cards
# May 28   2008  2.15   Jignesh Patel   Minor d-chan update for FreeBSD- A10x.pm update
# May 27   2008  2.14   Jignesh Patel   Added XMPT2 only option for SS7 
# May 27   2008  2.14   Jignesh Patel   Updated ss7_a100x templates for new XMPT2 interface
# May 22   2008  2.13 	Jignesh Patel   Added confirmation /dev/zap/* loaded for hardhdlc test
# Apr 28   2008  2.13   Jignesh Patel   Added TDM_OP_MODE for Analog and confirmation check
# Mar 28   2008  2.12   Jignesh Patel 	Removed BRI master clock and update unload_zap
# Jan 02   2008	 2.11  	David Yat Sin  	Support for per span configuration in silent mode
# Jan 02   2008	 2.10  	David Yat Sin  	Added option for BRI master clock
# Dec 15   2007	 2.9  	David Yat Sin  	Support for Zaptel hardware hdlc for Zaptel 1.4
# Nov 29   2007	 2.8  	David Yat Sin  	Support for BRI cards on Trixbox
# Aug 22   2007	 2.7  	David Yat Sin  	support for hardware DTMF option
# Aug 15   2007	 2.6  	David Yat Sin  	support for BRI cards in SMG mode	
# Jul 20,  2007  2.5 	David Yat Sin  	silent option	
# Jun 13,  2007  	Yuan Shen      	SS7 support
# Jan 15,  2007  	David Yat Sin  	support for non-trixbox installations. Major 
#					changes to script.
# Jan 8,   2007  	David Yat Sin  	script renamed to config-zaptel.pl
# Jan 5,   2007  2.1	David Yat Sin  	v2.1 fix for analog cards inserted in wrong
#					context
# Dec 12,  2006  2.0	David Yat Sin  	2.0 support for T1/E1 cards
# Oct 13,  2006  	David Yat Sin  	Added --opermode and --law option
# Oct 12,  2006  	David Yat Sin  	Initial version
# ============================================================================

system('clear');
print "\n########################################################################";
print "\n#    		           Sangoma Wanpipe                                  #";
print "\n#        Dahdi/Zaptel/SMG/TDMAPI/BOOT Configuration Script             #";
print "\n#                             v2.39                                    #";
print "\n#                     Sangoma Technologies Inc.                        #";
print "\n#                        Copyright(c) 2013.                            #";
print "\n########################################################################\n\n";

use strict;
#use warnings;
#use diagnostics;
use Card;
use A10x;
use A10u;
use A20x;
use W40x;
use A50x;
use U10x;
use analogspan;
use boostspan;



my $FALSE = 1;
my $TRUE = 0;
my $zaptelprobe=' ';

my $etc_dir="";
my $rc_dir="";
my $include_dir="";
my $module_list="";
my $module_load="";
my $module_unload="";
my $os_type_name="";
my $dchan_str="dchan";
my $have_update_rc_d = 0;
my $modules_loaded="";

my $os_type_list=`sysctl -a 2>/dev/null |grep ostype`;
if ($os_type_list =~ m/Linux/){
	$os_type_name="Linux";
	$etc_dir="/etc";
	$rc_dir=$etc_dir;
	$module_load="modprobe";
	$module_unload="modprobe -r";
	$module_list="lsmod";
	$modules_loaded="lsmod";
	$include_dir="/usr/include";
}elsif ($os_type_list =~ m/FreeBSD/){
	$os_type_name="FreeBSD";
	$etc_dir="/usr/local/etc";
	$rc_dir="/etc";
	$module_load="kldload";
	$module_unload="kldunload";
	$module_list="kldstat";
	$modules_loaded="kldstat";
}else{
	print("Failed to determine OS type\n");
	print("Exiting...\n");
	exit(1);
}

#check if os have update-rc.d
if (system('which update-rc.d 1>>/dev/null 2>>/dev/null') == 0) {
	$have_update_rc_d = 1;
}

my $no_boot=$FALSE;
my $no_smgboot=$FALSE;
my $boot_only=$FALSE;
my $no_hwdtmf=$FALSE;
my $usb_device_support=$TRUE;
my $startup_string="";
my $cfg_string="";
my $first_cfg=1;
my $zaptel_conf="";
my $zapata_conf="";
my $bri_conf="";
my $pri_conf="";
my $woomera_conf="";
my $devnum=1;
my $current_zap_span=1;
my $current_tdmapi_span=1;
my $current_zap_channel=1;
my $num_gsm_devices=0;
my $num_gsm_devices_total=0;
my $num_analog_devices=0;
my $num_analog_devices_total=0;
my $num_usb_devices_total=0;
my $num_usb_devices=0;
my $num_bri_devices=0;
my $num_bri_devices_total=0;
my $num_digital_devices=0;
my $num_digital_devices_total=0;
my $num_ss7_config=0;
my $num_zaptel_config=0;
my $num_digital_smg=0;
my $line_media='';
my $max_chans=0;
my $ss7_tdmvoicechans='';
my $ss7_array_length=0;

my $ztcfg_path='';


# FS conf array

my @fxsspan;
my @fxospan;
my @boostbrispan;
my @boostprispan;
my $openzap_pos='';

my $config_openzap = $FALSE;
my $config_openzap_xml= $FALSE;

my $device_has_hwec=$FALSE;
my $device_has_normal_clock=$FALSE;
my @device_normal_clocks=("0");
my @woomera_groups=("0");
my $bri_device_has_master_clock=$FALSE;
			
my $is_tdm_api=$FALSE;
my $is_hp_tdm_api=$FALSE;
my $is_fs=$FALSE;
my $is_openzap=$FALSE;
my $is_data_api=$FALSE;
my $aft_family_type='A';

my $is_ftdm=$FALSE;
my $is_trillium=$FALSE;
my $config_freetdm = $FALSE;
my $config_freetdm_xml = $FALSE;
my $cfg_profile='';
my $bri_trunk_type='';

my $def_femedia='';
my $def_feclock='';

my $cfg_wanpipe_port=0;

my $def_hw_port_map='DEFAULT';
my $def_tapping_mode='NO';
my $def_hdlc_mode='NO';
my $def_gsm_option='';
my $def_bri_option='';
my $def_bri_default_tei='';
my $def_bri_default_tei_opt=$FALSE;
my $def_signalling='';
my $ftdm_signalling='pri_cpe';
my $def_sigmode='';
my $def_switchtype='';
my $def_zapata_context='';
my $def_fs_context='';
my $def_woomera_context='';
my $def_zapata_group='';
my $def_te_ref_clock='';
my $def_tdmv_dchan=0;
my $def_woomera_group='';
my $def_felcode='';
my $def_feframe='';
my $def_te_sig_mode='CCS';

my $def_hw_dtmf="YES";
my $def_hw_fax="NO";
my $def_tdm_law='';
my $def_tdm_opermode="FCC";
my $def_is_ss7_xmpt2_only='';
my $def_hw_dchan= '';

my @silent_femedias;
my @silent_feframes;
my @silent_felcodes;
my @silent_tdm_laws;
my @silent_feclocks;
my @silent_signallings;
my @silent_pri_switchtypes;
my @silent_zapata_contexts;
my @silent_woomera_contexts;
my @silent_zapata_groups;
my @silent_bri_conn_types;
my @silent_woomera_groups;
my @silent_hwdtmfs;
my @silent_first_chans;
my @silent_last_chans;


my $silent_hwdtmf="YES";
my $silent_femedia="T1";
my $silent_feframe="ESF";
#my $silent_feframe_e1="CRC4";

my $silent_felcode="B8ZS";
my $silent_tdm_law="MULAW";


my $silent_feclock="NORMAL";
my $silent_signalling="Zaptel/Dahdi - PRI CPE";
my $silent_pri_switchtype="national";
my $silent_zapata_context="from-pstn";
my $silent_zapata_context_opt = $FALSE;
my $silent_zapata_group_opt = $FALSE;
my $silent_zapata_context_fxo="from-pstn";
my $silent_zapata_context_fxs="from-internal";
my $silent_woomera_context="from-pstn";
my $silent_zapata_group="0";
my $silent_te_sig_mode='';
my $silent_hw_dchan='';

my $silent_bri_conn_type="point_to_multipoint";
my $silent_woomera_group="1";

my $silent_first_chan=1;
my $silent_last_chan=24;

my $def_bri_country="europe";
my $def_bri_operator="etsi";
my $def_bri_conn_type="Point to multipoint";

my $is_trixbox = $FALSE;
my $silent = $FALSE;
my $config_zaptel = $TRUE;
my $config_zapata = $TRUE;
my $config_woomera = $TRUE;
my $is_smg = $FALSE;
my $safe_mode = $FALSE;
my $silent_zapata_conf_file = $FALSE;
my $current_run_level=3;
my $zaptel_start_level=9;
my $fs_conf_dir="/usr/local/freeswitch/conf";

my $is_dahdi=$FALSE;
my $is_zaptel=$FALSE;

my $tdm_api_span_num=0;
my $zaptel_installed=$FALSE;
my $dahdi_installed=$FALSE;
my $dahdi_ver_major=2; 
my $dahdi_ver_minor=4;
my $modprobe_list=`$modules_loaded`;
my $is_ss7_xmpt2_only = $FALSE;
my $zaptel_dahdi_installed=$FALSE;
my $dahdi_echo='mg2';

my $def_chunk_size=40;
my $def_mtu_mru=' ';

my $wanpipe_conf_dir="$etc_dir/wanpipe";
my $asterisk_conf_dir="$etc_dir/asterisk";

my $current_dir=`pwd`;
chomp($current_dir);
my $cfg_dir='tmp_cfg';
my $curdircfg="$current_dir"."/"."$cfg_dir";

unless ( -d $curdircfg ) {
	$curdircfg = "mkdir " . $curdircfg;
	system ($curdircfg); 
}

my $wanrouter_rc_template="$current_dir/templates/wanrouter.rc.template";
my $smg_rc_template="$current_dir/templates/smg.rc.template";
my $smg_rc_file="$current_dir/$cfg_dir/smg.rc";
my $smg_rc_file_t="$wanpipe_conf_dir/smg.rc";
my $zaptel_conf_template="$current_dir/templates/zaptel.conf";
my $zaptel_conf_file="$current_dir/$cfg_dir/zaptel.conf";
my $zaptel_conf_file_t="$etc_dir/zaptel.conf";

my $zapata_conf_template="$current_dir/templates/zapata.conf";
my $zapata_conf_file="$current_dir/$cfg_dir/zapata.conf";
my $zapata_conf_file_t="$asterisk_conf_dir/zapata.conf";
my $zaptel_string="Zaptel";
my $zapata_string="Zapata";
my $wancfg_config="wancfg_zaptel";
my $zaptel_boot = "zaptel";
my $zaptel_cfg_script="zaptel_cfg_script";
my $zap_cfg = "ztcfg";
my $zap_com ="zap";


read_args();
if($boot_only==$TRUE){
	if ($os_type_list =~ m/FreeBSD/){
	        exit( &config_boot_freebsd());
	} else {
	        exit( &config_boot_linux());
	}
}



my $debug_info_file="$current_dir/$cfg_dir/debug_info";
my @hwprobe=`wanrouter hwprobe verbose`;
check_zaptel();
check_dahdi();

if ($is_dahdi == $TRUE) {
	if( $dahdi_installed==$FALSE ) {
		print("Warning: Dahdi modules not found: Wanpipe not build for DAHDI\n");
		if ( $zaptel_installed == $TRUE ) {
			print("Warning: Zaptel modules found - configuring for Zaptel!\n");
		} else {
			print("Error: Dahdi and/or Zaptel modules not found! Please install DAHDI and compile Wanpipe for DAHDI!\n");
			exit(1);
		}
	} 
}

if ($is_zaptel == $TRUE) {
	if( $zaptel_installed==$FALSE ) {
		print("Warning: Zaptel modules not found: Wanpipe not build for Zaptel\n");
		if ( $dahdi_installed==$TRUE ) {
			print("Warning: Dahdi modules found - configuring for Dahdi!\n");
		} else {
			print("Error: Dahdi and/or Zaptel modules not found! Please install Zaptel and compile Wanpipe for Zaptel!\n");
			exit(1);
		}
	}
}



if ($dahdi_installed== $TRUE) {
	$zapata_conf_file_t="$asterisk_conf_dir/chan_dahdi.conf";
	$zaptel_conf_file_t="$etc_dir/dahdi/system.conf";
	$zaptel_string="Dahdi";
	$dchan_str="hardhdlc";
	$zapata_string="chan_dahdi";
	$wancfg_config="wancfg_dahdi";
	$zaptel_boot="dahdi";
	$zaptel_cfg_script="dahdi_cfg_script";
	$zap_cfg="dahdi_cfg";
	$zap_com="dahdi";

}
if ($is_fs== $TRUE) {
	$config_woomera=$FALSE;
	$config_zapata = $FALSE; 
	$config_openzap= $TRUE;
	$config_openzap_xml=$TRUE;
	#$def_sigmode='pri_cpe';
}

if ($is_ftdm== $TRUE || $is_trillium == $TRUE) {
	$config_woomera=$FALSE;
	$config_zapata = $FALSE; 
	$config_freetdm= $TRUE;
	$config_freetdm_xml=$TRUE;
	$config_openzap= $FALSE;
	$config_openzap_xml=$FALSE;
	#$def_sigmode='pri_cpe';
}	
	


my $bri_conf_template="$current_dir/templates/smg_bri.conf";
my $bri_conf_file="$current_dir/$cfg_dir/smg_bri.conf";
my $bri_conf_file_t="$wanpipe_conf_dir/smg_bri.conf";

my $pri_conf_template="$current_dir/templates/smg_pri.conf";
my $pri_conf_file="$current_dir/$cfg_dir/smg_pri.conf";
my $pri_conf_file_t="$wanpipe_conf_dir/smg_pri.conf";

my $woomera_conf_template="$current_dir/templates/woomera.conf";
my $woomera_conf_file="$current_dir/$cfg_dir/woomera.conf";
my $woomera_conf_file_t="$asterisk_conf_dir/woomera.conf";

my $openzap_conf_template="$current_dir/templates/openzap.conf";
my $openzap_conf_file="$current_dir/$cfg_dir/openzap.conf";
my $openzap_conf_file_t="$fs_conf_dir/openzap.conf";

my $openzap_conf_xml_template="$current_dir/templates/openzap.conf.xml";
my $openzap_conf_xml_file="$current_dir/$cfg_dir/openzap.conf.xml";
my $openzap_conf_xml_file_t="$fs_conf_dir/autoload_configs/openzap.conf.xml";

my $freetdm_conf_template="$current_dir/templates/freetdm.conf";
my $freetdm_conf_file="$current_dir/$cfg_dir/freetdm.conf";
my $freetdm_conf_file_t="$fs_conf_dir/freetdm.conf";

my $freetdm_conf_xml_template="$current_dir/templates/freetdm.conf.xml";
my $freetdm_conf_xml_file="$current_dir/$cfg_dir/freetdm.conf.xml";
my $freetdm_conf_xml_file_t="$fs_conf_dir/autoload_configs/freetdm.conf.xml";

my $date=`date +%F`;
my $mdate= `date +"%a-%d-%b-%Y-%I-%M:%S-%p"`;
chomp($date);
my $debug_tarball="$wanpipe_conf_dir/debug-".$date.".tgz";
if( $zaptel_installed==$TRUE || $dahdi_installed==$TRUE ) {
	$zaptel_dahdi_installed=$TRUE;
}
	
if ($os_type_list =~ m/FreeBSD/ && $zaptel_dahdi_installed==$TRUE) {
	parse_wanrouter_rc();
	update_zaptel_cfg_script();
}

if( $zaptel_installed==$TRUE && $os_type_list =~ m/Linux/ && $is_fs == $FALSE) {
	set_zaptel_hwhdlc();
}


prepare_files();
config_t1e1();
config_bri();
config_gsm();
config_analog();


if($usb_device_support == $TRUE && $os_type_list =~ m/Linux/) {
	config_usbfxo();
}
if( $is_tdm_api == $FALSE ) {
	config_tdmv_dummy();
}
update_module_load();
summary();
apply_changes();


if ($os_type_list =~ m/FreeBSD/){
	config_boot_freebsd();
} else {
	config_boot_linux();
}

if($is_trillium == $FALSE){
	if ($is_tdm_api == $FALSE ){
		config_ztcfg_start();
		config_smg_ctrl_start();
		if($os_type_list =~ m/Linux/){
			config_smg_ctrl_boot();
		}
	}
}

clean_files();
print "Sangoma cards configuration complete, exiting...\n\n";


#######################################FUNCTIONS##################################################
sub get_card_name{
	my ($card_name) = @_;

	if ($card_name =~ m/\w+\d+/) {
		return $card_name;
	}

	if ( $card_name eq '400') {
		$card_name = "W".$card_name;
	} elsif ( $card_name eq '600' || $card_name eq '601' || $card_name eq '610' || $card_name eq '700') {
		$card_name = "B".$card_name;
	} else {
		$card_name = "A".$card_name;
	}
	return $card_name;
}


sub set_zaptel_hwhdlc{
	print "Checking for native zaptel hardhdlc support...";
        my $cnt = 0;
        while ($cnt++ < 30) {
             if ((system("ls /dev/zap* > /dev/null 2>  /dev/null")) == 0) {
	                   goto wait_done;
                } else {
                        print "." ;
                        sleep(1);
                }
        }
	print "Error";
	print "\n\n No /dev/zap* Found on the system \n";
	printf "     Contact Sangoma Support\n";
	print " Send e-mail to techdesk\@sangoma\.com \n\n";
	exit 1;
wait_done:

	if ((system("ztcfg -t -c $current_dir/templates/zaptel.conf_test > /dev/null 2>/dev/null")==0)){
		$dchan_str="hardhdlc";
		 print "Yes \n\n";

        } else {
                print "No \n\n";
	}
}

sub config_boot_freebsd{
	if($no_boot==$TRUE){
		return 1;
	}
	my $res;
	my $rc_init="";
	if (!open (FH,"$rc_dir/rc.conf")) {
		print "Warning: Could not open $rc_dir/rc.conf. Using empty template\n";
		open (FH,"$current_dir/templates/rc_init_template_freebsd");
	}

	while (<FH>) {
		$rc_init .= $_;
	}
	close (FH);

	print ("Would you like wanrouter to start on system boot?\n");
	$res= &prompt_user_list("YES","NO","");
	
	open (FH,">$rc_dir/rc.conf");
#	$rc_init =~ s/WAN_DEVICES\s*=.*/WAN_DEVICES="$startup_string"/g;
	if ( $res eq "YES" ) {
		$rc_init =~ s/wanpipe_enable\s*=.*/wanpipe_enable="YES"/g;
	} else {
		$rc_init =~ s/wanpipe_enable\s*=.*/wanpipe_enable="NO"/g;
	}
	print FH $rc_init;
	close (FH);

}

sub config_boot_linux {
	
	my $script_name="wanrouter";
	my $zaptel_stop_level=92;
	my $wanrouter_start_level=8;
	my $wanrouter_stop_level=91;
	my $command='';
#	my $rc_dir=$etc_dir;			

	#If we are configuring just one port do handle global init stuf 
	if ($cfg_wanpipe_port) {
		return 0;
	}

	my $res=`cat $etc_dir/inittab |grep id`;
	if ($res =~ /id:(\d+):initdefault/){
		$current_run_level=$1;
		print "Current boot level is $current_run_level\n";
	} else {
		print "Warning: Failed to determine init boot level, assuming 3\n";
		$current_run_level=3;
	}


	print "\nWanrouter boot scripts configuration...\n\n";
	print "Removing existing $script_name boot scripts...";

	if ($have_update_rc_d) {#debian/ubuntu
		$command = "update-rc.d -f $script_name remove >>/dev/null 2>>/dev/null";
	} else {
		$command="rm -f $rc_dir/rc?.d/*$script_name >/dev/null 2>/dev/null";
	}
	if(system($command) == 0){
		print "OK\n";
	} else {
		print "Not installed\n";
	}
	if($no_boot==$TRUE){ #remove old script anyway.
		return 1;
	}

	if($num_ss7_config!=0){
		$script_name="smgss7_init_ctrl";
	}

	$res='YES';
	if($silent==$FALSE){
		print ("Would you like $script_name to start on system boot?\n");
		$res= &prompt_user_list("YES","NO","");
	}
	
	if ( $res eq 'YES'){
		#examine system bootstrap files
		$command="find ".$etc_dir."/rc0.d >/dev/null 2>/dev/null";
		if (system($command) == 0){
			$rc_dir=$rc_dir;
		} else {
			$command="find ".$etc_dir."rc.d/rc0.d >/dev/null 2>/dev/null";
			if (system($command) == 0){
				$rc_dir=$etc_dir."/rc.d";
			} else {
				print "Failed to locate boostrap directory\n";
				print "wanrouter boot scripts will not be installed\n";
				return 1;
			}
		}
		print "Verifying $zaptel_string boot scripts...\n";
		if($zaptel_dahdi_installed==$TRUE){
			print "Verifying $zaptel_string boot scripts...";
			#find zaptel/dahdi start scripts
			$command="find $rc_dir/rc".$current_run_level.".d/*$zaptel_boot >/dev/null 2>/dev/null";
			if (system($command) == 0){
				$command="find $rc_dir/rc".$current_run_level.".d/*$zaptel_boot";
				$res=`$command`;
				if ($res =~ /.*S(\d+)$zaptel_boot/){
					$zaptel_start_level=$1;
					print "Enabled (level:$zaptel_start_level)\n";
				} else {
					print "\nfailed to parse zaptel boot level, assuming $zaptel_start_level";
				}
			} else {
				print "Not installed\n";
				$zaptel_start_level=0;
			}
			
			#find zaptel stop scripts
			print "Verifying $zaptel_string shutdown scripts...";
			$command="find ".$rc_dir."/rc6.d/*$zaptel_boot >/dev/null 2>/dev/null";
			if (system($command) == 0){
				$command="find ".$rc_dir."/rc6.d/*$zaptel_boot";
				$res=`$command`;
 				if ($res =~ /.*K(\d+)$zaptel_boot/){
					$zaptel_stop_level=$1;
					print "Enabled (level:$zaptel_stop_level)\n";
				} else {
					print "\nfailed to parse zaptel boot level, assuming $zaptel_stop_level\n";
				}
			} else {
				print "Not installed\n";
				$zaptel_stop_level=0;
			}
			if ($zaptel_start_level != 0){
				$wanrouter_start_level=$zaptel_start_level-1;	
			}
			if ($zaptel_stop_level != 0){
				$wanrouter_stop_level=$zaptel_stop_level-1;	
			}
		}
			
		my $wanrouter_start_script='';
		if($wanrouter_start_level < 10){
			$wanrouter_start_script="S0".$wanrouter_start_level.$script_name;
		} else {
			$wanrouter_start_script="S".$wanrouter_start_level.$script_name;
		}
		my $wanrouter_stop_script='';
		if($wanrouter_stop_level < 10){
			$wanrouter_stop_script="K0".$wanrouter_stop_level.$script_name;
		} else {
			$wanrouter_stop_script="K".$wanrouter_stop_level.$script_name;
		}

		$command="find $etc_dir/init.d/$script_name >/dev/null 2>/dev/null";
		if(system($command) !=0){
			$command="install -D -m 755 /usr/sbin/$script_name $rc_dir/init.d/$script_name";
			if(system($command) !=0){
				print "Failed to install $script_name script to $rc_dir/init.d/$script_name\n";
				print "$script_name boot scripts not installed\n";
				return 1;
			}
		}
   		
		if ($have_update_rc_d) {
			print "Enabling $script_name init scripts ...(start:$wanrouter_start_level, stop:$wanrouter_stop_level)\n";
			$command = "update-rc.d $script_name defaults $wanrouter_start_level $wanrouter_stop_level";
			if (system($command .' >>/dev/null 2>>/dev/null') != 0) {
				print "Command failed: $command\n";
 				return 1;
				}
		}else{
			print "Enabling $script_name boot scripts ...(level:$wanrouter_start_level)\n";
			my @run_levels= ("2","3","4","5");
			foreach my $run_level (@run_levels) {
				$command="ln -sf $rc_dir/init.d/$script_name ".$rc_dir."/rc".$run_level.".d/".$wanrouter_start_script;
				if(system($command) !=0){
					print "Failed to install $script_name init script to $rc_dir/rc$run_level.d/$wanrouter_start_script\n";
					print "$script_name start scripts not installed\n";
					return 1;
				}
			}	
					
			print "Enabling $script_name shutdown scripts ...(level:$wanrouter_stop_level)\n";
			@run_levels= ("0","1","6");
			foreach my $run_level (@run_levels) {
				$command="ln -sf $rc_dir/init.d/$script_name ".$rc_dir."/rc".$run_level.".d/".$wanrouter_stop_script;
				if(system($command) !=0){
					print "Failed to install $script_name shutdown script to $rc_dir/rc$run_level.d/$wanrouter_stop_script\n";
					print "$script_name stop scripts not installed\n";
					return 1;
				}
			}
		}	
	}
	return 0;	
}


sub config_ztcfg_start{
	if($silent==$TRUE) {
		printf("Removing old stop/start scripts....\n");
		exec_command("rm -rf $wanpipe_conf_dir/scripts/start");
		exec_command("rm -rf $wanpipe_conf_dir/scripts/stop");
		if($num_zaptel_config != 0){
			printf("Installing new $zaptel_string start script....\n");
			exec_command("mkdir -p $wanpipe_conf_dir/scripts");
			exec_command("cp -f $current_dir/templates/$zaptel_cfg_script $wanpipe_conf_dir/scripts/start");
		}
		return;
	}
	if ($num_zaptel_config ==0){
			exec_command("rm -rf $wanpipe_conf_dir/scripts/start");
			exec_command("rm -rf $wanpipe_conf_dir/scripts/stop");
			return;
	}
		#Zaptel/Dahdi init scripts not installed, prompt for wanpipe_zaptel_start_script
		print ("\nWould you like to execute \'$zap_cfg\' each time wanrouter starts?\n");
		if (&prompt_user_list("YES","NO","") eq 'YES'){
			if ( ! -d "$wanpipe_conf_dir/scripts" ) {
				exec_command("mkdir -p $wanpipe_conf_dir/scripts");
			} 
			exec_command("cp -f $current_dir/templates/$zaptel_cfg_script $wanpipe_conf_dir/scripts/start");
		} elsif(-e "$wanpipe_conf_dir/scripts/start") {
				print ("\nWould you like to remove old wanrouter start scripts ?\n");
				if (&prompt_user_list("YES","NO","") eq 'YES'){
					exec_command("rm -rf $wanpipe_conf_dir/scripts/start");
				}
		}
}

sub config_smg_ctrl_start{
	if( $is_fs == $FALSE && $is_smg == $FALSE ) {
		return;
	}
	
	if(($num_bri_devices == 0) && ($num_digital_smg == 0)){
		return;
	}
	my $res;
	if($silent==$FALSE) {
		print ("Would you like smg_ctrl to start/stop on wanrouter start \nor would you like to run smg_ctrl in safe start?\n");
		$res = &prompt_user_list("YES","NO","SAFE_START","");
	} else {
		if($safe_mode==$TRUE){
	                $res = "SAFE_START";
	} else{
		$res = "YES";
	      }
	}
	         if ($res eq "YES" || $res eq "SAFE_START"){
                #if zaptel start script is in $wanpipe_conf_dir/scripts/start, do not overwrite
                my $command="find ".$wanpipe_conf_dir."/scripts/start >/dev/null 2>/dev/null";
                if (system($command) == 0){

                        my $command="cat ".$wanpipe_conf_dir."/scripts/start | grep smg_ctrl >/dev/null 2>/dev/null";
                        if (system($command) == 0){
                        print("smgbri start script already installed!\n");
                        }else{
                        my $command="cat ".$current_dir."/templates/smgbri_start_script_addon >>".$wanpipe_conf_dir."/scripts/start";
                                if (system($command) == 0){
                                        print("smgbri start script installed successfully\n");
                                }
                        }
                } elsif(system($command) != 0) {

                        $command="cp -f ".$current_dir."/templates/smgbri_start_script ".$wanpipe_conf_dir."/scripts/start";
                                if (system($command) == 0){
                                        print("smgbri start scrtip installed scuessfully\n");
                                }
                } else {
                                print "failed to install smgbri start script\n";
                }
                $command="cp -f ".$current_dir."/templates/smgbri_stop_script ".$wanpipe_conf_dir."/scripts/stop";
                if (system($command) == 0){
                        print "smgbri stop script installed successfully\n";
                } else {
                print "failed to install smgbri start script\n";
                }
				if($res eq "SAFE_START") {
					
					update_start_smgbri_safe();
					$safe_mode=$TRUE;#set it true anyway for boot safe_start

				}
        }

}
	
sub check_zaptel{

	if ((system("$module_list | grep zaptel > /dev/null 2>  /dev/null")) == 0){
		$zaptel_installed=$TRUE;
		$dahdi_installed=$FALSE;
	} else {
		$zaptel_installed=$FALSE;
	}
}

sub check_dahdi
{
	
	if ((system("$module_list | grep dahdi > /dev/null 2>  /dev/null")) == 0){
		$dahdi_installed=$TRUE;
		$zaptel_installed=$FALSE;
	} else {
		$dahdi_installed=$FALSE;
	}

    if($dahdi_installed==$TRUE) {
        check_dahdi_ver();
    }
}

sub check_dahdi_ver
{
    my $ver_major;
    my $ver_minor;
    my $str_tmp; 
    my $strDahdi="DAHDI Version";

    $str_tmp = `dahdi_cfg -vt 2>&1 | grep \"$strDahdi\" `;
    if ($str_tmp =~ m/(\d).(\d)/) {
        $ver_major = $1;  
        $ver_minor = $2;
        #print "the major ver is $ver_major, minor version is $ver_minor.\n";
    }

    $dahdi_echo='mg2';

#	NC: This is not necessary as there was a bug in a driver
#   NC: that did not allow dahdi to autodetec hwec.
#    if ($ver_major > $dahdi_ver_major) {
#        $dahdi_echo='HWEC';
#    } elsif ($ver_major == $dahdi_ver_major) {
#        if ($ver_minor > $dahdi_ver_minor) {
#            $dahdi_echo='HWEC';
#        }
#    }
}


sub apply_changes{
	my $asterisk_command='';
	my $bri_command='';
	my $asterisk_restart=$FALSE;
	my $res='';
	my $asterisk_version1='';
	my $asterisk_version2='';

	if($silent==$FALSE) {system('clear')};
	
	if($silent==$TRUE){
		$res="Stop now";
	}elsif($is_tdm_api==$TRUE || $is_hp_tdm_api==$TRUE){
		print "\n Wanpipe configuration complete: choose action\n";
		$res=&prompt_user_list(	"Save cfg: Stop Wanpipe now",
					"Do not save cfg: Exit",
					"");
	}else{
		print "\n$zaptel_string and Wanpipe configuration complete: choose action\n";
		$res=&prompt_user_list("Save cfg: Restart Asterisk & Wanpipe now",
					"Save cfg: Restart Asterisk & Wanpipe when convenient",
					"Save cfg: Stop Asterisk & Wanpipe now", 
					"Save cfg: Stop Asterisk & Wanpipe when convenient",
					"Save cfg: Save cfg only (Not Recommended!!!)", 
					"Do not save cfg: Exit",
					"");
	}

	if ($cfg_wanpipe_port) {
		$cfg_string="wanpipe$cfg_wanpipe_port";
		$startup_string=$cfg_string;
		append_wanrouter_rc();
       	copy_config_files();
		exit 0;
	}
	
	if ($res =~ m/cfg only/){

		print "\nRemoving old configuration files...\n";
       	exec_command("rm -f $wanpipe_conf_dir/wanpipe*.conf");

       	gen_wanrouter_rc();
		if( $num_bri_devices != 0 | $num_digital_smg != 0) { 
			if($is_fs == $TRUE | $is_smg == $TRUE ){
				gen_smg_rc();
			}
		}
       	print "\nCopying new Wanpipe configuration files...\n";
       	copy_config_files();
       	if(($num_bri_devices != 0) && ($is_fs == $TRUE | $is_smg == $TRUE )){
			if($is_trillium == $FALSE){
               	print "\nCopying new sangoma_bri configuration files ($bri_conf_file_t)...\n";
      	        	exec_command("cp -f $bri_conf_file $bri_conf_file_t");
			}
		}
		if(($num_bri_devices != 0 | $num_digital_smg !=0) && $config_woomera == $TRUE) {
                	exec_command("cp -f $woomera_conf_file $woomera_conf_file_t");
		}
		if ($zaptel_dahdi_installed==$TRUE){
				if($config_zaptel==$TRUE){
						if ($num_zaptel_config !=0){
								print "\nCopying new $zaptel_string configuration file ($zaptel_conf_file_t)...\n";
								exec_command("cp -f $zaptel_conf_file $zaptel_conf_file_t");
						}
				}
		}

		if ($config_zapata==$TRUE || $is_trixbox==$TRUE){
				if ($num_zaptel_config !=0){
						print "\nCopying new $zapata_string configuration files ($zapata_conf_file_t)...\n";
						exec_command("cp -f $zapata_conf_file $zapata_conf_file_t");
				}
		}
        print "Saving files only\n";

		if ($os_type_list =~ m/FreeBSD/){
        	config_boot_freebsd();
		} else {
        	config_boot_linux();
			config_smg_ctrl_boot();
		}

        exit 0;
	}


	if ($res =~ m/Exit/){
		print "No changes made to your configuration files\n";
		exit 0;
	}
	if ($res =~ m/now/){
		$asterisk_version1 = `asterisk -V | cut -d' ' -f 2 | cut -d '.' -f 2`;
		$asterisk_version2 = `asterisk -V | cut -d' ' -f 2 | cut -d '.' -f 3`;
		
		if ($asterisk_version1 =~ m/UNKNOWN/) {
			$asterisk_command='core stop now';
		} elsif (($asterisk_version1 >= 6) && ($asterisk_version2 >= 2)) {
			$asterisk_command='core stop now';
		} else {
			$asterisk_command='stop now';
		}
	} else {
		$asterisk_command='core stop when convenient';
	}

	if ($res =~ m/Restart/){
		$asterisk_restart=$TRUE;
	} else {
		$asterisk_restart=$FALSE;
	}
	
	if ($is_trixbox==$TRUE){
		exec_command("amportal stop");
	} elsif ($is_tdm_api==$FALSE || $is_hp_tdm_api==$FALSE ){
		if (`(pidof asterisk)` != 0 ){
			print "\nStopping Asterisk...\n";
			exec_command("asterisk -rx \"$asterisk_command\"");
			sleep 5;
			while (`(pidof asterisk)` != 0 ){
				if ($asterisk_command eq "stop now"){
					print "Failed to stop asterisk using command: \'$asterisk_command\' Forcing Asterisk Down \n";
					execute_command("killall -9 safe_asterisk");
					execute_command("killall -9 asterisk");
				} else { 
					#stop when convenient option was selected
					print "Waiting for asterisk to stop when convenient...\n";
					sleep 3;
				}
			}

		}elsif ($is_trillium == $FALSE){
			if ($is_tdm_api == $FALSE) {
				print "\nAsterisk is not running...\n";
			}
		}
			
	} 
	if ($is_trillium == $FALSE){
		if($is_tdm_api==$FALSE) {
			if(-f "/usr/sbin/smg_ctrl" ){
				if( $is_fs != $FALSE ) {
					exec_command("/usr/sbin/smg_ctrl stop");
				}
			}
		}
	}


	print "\nStopping Wanpipe...\n";
	exec_command("wanrouter stop all");

	if ($zaptel_dahdi_installed==$TRUE){
		if($is_tdm_api==$FALSE){
			if ($is_hp_tdm_api==$FALSE){
				print "\nUnloading $zaptel_string modules...\n";
				unload_zap_modules();
			}
		}
	}

	print "\nRemoving old configuration files...\n";

	exec_command("rm -f $wanpipe_conf_dir/wanpipe*.conf");
	
	gen_wanrouter_rc();
	if($is_trillium == $FALSE){
		if ($is_tdm_api == $FALSE){
			if( $num_bri_devices != 0 | $num_digital_smg != 0) { 
				if($is_fs == $TRUE | $is_smg == $TRUE ){
					gen_smg_rc();
				}
			}
		}
	}
	print "\nCopying new Wanpipe configuration files...\n";
	copy_config_files();
	if(($num_bri_devices != 0)&&( $is_smg == $TRUE )){
		if($is_trillium == $FALSE){
			if($is_tdm_api == $FALSE){
				print "\nCopying new sangoma_bri configuration files ($bri_conf_file_t)...\n";
				exec_command("cp -f $bri_conf_file $bri_conf_file_t");
				if($config_woomera == $TRUE){
					exec_command("cp -f $woomera_conf_file $woomera_conf_file_t");
				}
			}
		}
	}
	if ($zaptel_dahdi_installed==$TRUE){
		if($config_zaptel==$TRUE){
			if ($num_zaptel_config !=0){
				print "\nCopying new $zaptel_string configuration file ($zaptel_conf_file_t)...\n";
				exec_command("cp -f $zaptel_conf_file $zaptel_conf_file_t");
			}
		}
	}  	
	
	if ($config_zapata==$TRUE || $is_trixbox==$TRUE){
		if ($num_zaptel_config !=0){
			print "\nCopying new $zapata_string configuration files ($zapata_conf_file_t)...\n";
			exec_command("cp -f $zapata_conf_file $zapata_conf_file_t");
		}
	}

	if( $num_digital_smg != 0){
		if(($is_trillium == $FALSE) || ($is_tdm_api == $FALSE)){
			print "\nCopying new sangoma_prid configuration files ($pri_conf_file_t)...\n";
			exec_command("cp -f $pri_conf_file $pri_conf_file_t");
				if($config_woomera == $TRUE){
				print "\nCopying new woomera configuration files ($woomera_conf_file_t)...\n";
				exec_command("cp -f $woomera_conf_file $woomera_conf_file_t");
			}
		}
	}
	
	if ($num_digital_smg !=0 || $num_bri_devices !=0){
		if ($is_trillium == $FALSE){
			if(($is_tdm_api == $FALSE) && ($is_fs == $TRUE | $is_smg == $TRUE)){
				print "\nCopying new smg.rc configuration files ($smg_rc_file_t)...\n";
				exec_command("cp -f $smg_rc_file $smg_rc_file_t");
			}
		}
	}

	if($config_openzap == $TRUE && $is_ftdm == $FALSE){
		print "\nCopying new openzap configuration files ($openzap_conf_file_t)...\n";
		exec_command("cp -f $openzap_conf_file $openzap_conf_file_t");

	}	
	#if($config_openzap_xml == $TRUE && $is_openzap == $FALSE){
	if($config_openzap_xml == $TRUE && $is_openzap == $FALSE){
		print "\nCopying new openzap-xml configuration files ($openzap_conf_xml_file_t)...\n";
		exec_command("cp -f $openzap_conf_xml_file $openzap_conf_xml_file_t");
	}

	if($config_freetdm == $TRUE){
		print "\nCopying new freetdm configuration files ($freetdm_conf_file_t)...\n";
		exec_command("cp -f $freetdm_conf_file $freetdm_conf_file_t");

	}	
	if($config_freetdm_xml == $TRUE && ($is_ftdm == $TRUE || $is_trillium == $TRUE)){
		print "\nCopying new freetdm configuration files ($freetdm_conf_xml_file_t)...\n";
		exec_command("cp -f $freetdm_conf_xml_file $freetdm_conf_xml_file_t");
	}


	if( $asterisk_restart == $TRUE && $is_tdm_api==$FALSE && $is_hp_tdm_api==$FALSE ){
		print "\nStarting Wanpipe...\n";
		exec_command("wanrouter start");

		if(($num_bri_devices != 0) && ($is_fs == $TRUE | $is_smg == $TRUE )){
			print "Loading SMG BRI...\n";
			sleep 2;
			exec_command("/usr/sbin/smg_ctrl start");
		}

		if ($num_zaptel_config != 0){	
			print "Loading $zaptel_string...\n";	
			sleep 2;
			if ($zaptel_installed==$TRUE){
				exec_command("$ztcfg_path\ztcfg  -v");
			}else{
				exec_command("dahdi_cfg  -v");
			}	 
		}
		if ($is_trixbox==$TRUE){
			print "\nStarting Amportal...\n";
			exec_command("amportal start");
			sleep 2;
		}elsif($config_zapata==$TRUE){
			print "\nStarting Asterisk...\n";
			exec_command("safe_asterisk");
			sleep 2;
	
				
			if ($num_zaptel_config != 0){	
				print "\nListing Asterisk channels...\n\n";
				exec_command("asterisk -rx \"$zap_com show channels\"");
			}
			print "\nType \"asterisk -r\" to connect to Asterisk console\n\n";
		}else{
		}
	}	
	print "\nWanrouter start complete...\n";
}



sub save_debug_info{
	my $version=`wanrouter version`;
	chomp($version);

	my $uname=`uname -a`;
	chomp($uname);

	my $issue='';

	if ($os_type_list =~ m/Linux/){
		$issue=`cat $etc_dir/issue`;
		chomp($issue);
	}

	my $debug_info="\n"; 
	$debug_info.="===============================================================\n";
	$debug_info.="                   SANGOMA DEBUG INFO FILE                     \n";
	$debug_info.="                   Generated on $date                          \n";
	$debug_info.="===============================================================\n";
	
	$debug_info.="\n\nwanrouter hwprobe\n";
	$debug_info.="@hwprobe\n";	
	$debug_info.="\nwanrouter version\n";
	$debug_info.="$version\n";	
	$debug_info.="\nKernel \"uname -a\"\n";
	$debug_info.="$uname\n";
	if ($os_type_list =~ m/Linux/){
		$debug_info.="\n$os_type_name distribution \"cat $etc_dir/issue\"\n"; 
		$debug_info.="$issue\n";
	}
	$debug_info.="EOF\n";
	
	open (FH,">$debug_info_file") or die "Could not open $debug_info_file";
	print FH $debug_info;
	close (FH);	
	exec_command("tar cvfz $debug_tarball $cfg_dir/* >/dev/null 2>/dev/null");
}

sub get_chan_no{
	my ($chan_name,$first_ch, $last_ch)=@_;
	
	my $res_ch=&prompt_user("\nInput the $chan_name channel for this span [$first_ch-$last_ch]\n");
	while(length($res_ch)==0 ||!($res_ch =~/(\d+)/) 
		|| $res_ch<$first_ch || $res_ch>$last_ch){
		print "Invalid channel, must be between $first_ch and $last_ch\n";
		$res_ch=&prompt_user("Input the channel for this port[$first_ch-$last_ch]");
	}
	return $res_ch;
}

sub get_zapata_context{
	my ($card_model,$card_port)=@_;
	my $context='';
	my @options = ("from-pstn", "from-internal","Custom");
	
	if($is_trixbox==$TRUE){
		@options = ("PSTN", "INTERNAL");
	}
	if ($silent==$FALSE){
		printf ("Select dialplan context for AFT-%s on port %s\n", get_card_name($card_model), $card_port);
		my $res = &prompt_user_list(@options,$def_zapata_context);
		if($res eq "PSTN"){
			$context="from-zaptel";
		}elsif($res eq "INTERNAL"| $res eq "from-internal"){
			$context="from-internal";
		}elsif($res eq "from-pstn"){
			$context="from-pstn";
		}elsif($res eq "Custom"){
			my $res_context=&prompt_user("Input the context for this port");
			while(length($res_context)==0){
				print "Invalid context, input a non-empty string\n";
				$res_context=&prompt_user("Input the context for this port",$def_zapata_context);
			}	
		
			$context=$res_context;
		}elsif($res eq $def_zapata_context){
			$context=$def_zapata_context;
		}else{
			print "Internal error:invalid context,group\n";
			exit 1;
		}
	} else {
		if($#silent_zapata_contexts >= 0){
			$silent_zapata_context=pop(@silent_zapata_contexts);
		}
		$context=$silent_zapata_context;	
	}
	$def_zapata_context=$context;
	return $context;	
}

sub get_context{
	my ($card_model,$card_port)=@_;
	my $context='';
	my @options = ("default", "public","Custom");

	if ($silent==$FALSE){    
		printf ("Select dialplan context for AFT-%s on port %s\n", $card_model, $card_port);
		my $res = &prompt_user_list(@options,$def_fs_context);
		if($res eq "default"){
			$context="default";
		}elsif($res eq "public"){
			$context="public";
		}elsif($res eq "Custom"){
			my $res_context=&prompt_user("Input the context for this port");
			while(length($res_context)==0){
				print "Invalid context, input a non-empty string\n";
				$res_context=&prompt_user("Input the context for this port",$def_fs_context);
			}
			$context=$res_context;
		}elsif($res eq $def_fs_context){
			$context=$def_fs_context;
		}else{
			print "Internal error:invalid context\n";
		}
		
	}
	$def_fs_context=$context;
	return $context;
}


sub get_woomera_context{
        my ($group,$card_model,$card_port,$bri_type)=@_;

        my $context='';
	my @options = ("from-pstn", "from-internal","Custom");

	if($bri_type eq 'bri_nt'){
	        @options = ("from-internal","Custom");
	} elsif ($bri_type eq 'bri_te'){
	        @options = ("from-pstn","Custom");
	}

        if($is_trixbox==$TRUE){
                @options = ("PSTN", "INTERNAL");
        }
        if ($silent==$FALSE){
                printf ("\nSelect dialplan context for group:%d\n", $group);
                my $res = &prompt_user_list(@options,$def_woomera_context);
                if($res eq "PSTN"){
                        $context="from-zaptel";
                }elsif($res eq "INTERNAL"| $res eq "from-internal"){
                        $context="from-internal";
                }elsif($res eq "from-pstn"){
                        $context="from-pstn";
                }elsif($res eq "Custom"){
                        my $res_context=&prompt_user("Input the context for this port");
                        while(length($res_context)==0){
                                print "Invalid context, input a non-empty string\n";
                                $res_context=&prompt_user("Input the context for this port",$def_woomera_context);
                        }

                        $context=$res_context;
                }elsif($res eq $def_woomera_context){
                        $context=$def_woomera_context;
                }else{
                        print "Internal error:invalid context,group\n";
                        exit 1;
                }
        } else {
		if($#silent_woomera_contexts >= 0){
			$silent_woomera_context=pop(@silent_woomera_contexts);
		}
                $context=$silent_woomera_context;
        }
        $def_woomera_context=$context;
        return $context;
}


sub append_wanrouter_rc{
	#update wanpipe startup sequence
	my $rcfile="";
	if (!open (FH,"$wanpipe_conf_dir/wanrouter.rc")) {
		open (FH,"$wanrouter_rc_template");
	}
	while (<FH>) {
		$rcfile .= $_;
	}
	close (FH);
	open (FH,">$current_dir/$cfg_dir/wanrouter.rc");
	if ($rcfile !~ m/WAN_DEVICES.*=.*$startup_string/) {
		$rcfile =~ s/(WAN_DEVICES.*=.*)"/$1 $startup_string"/g;
	}
	print FH $rcfile;
	close (FH);
}

sub gen_wanrouter_rc{
	#update wanpipe startup sequence
	my $rcfile="";
	if (!open (FH,"$wanpipe_conf_dir/wanrouter.rc")) {
		open (FH,"$wanrouter_rc_template");
	}
	while (<FH>) {
		$rcfile .= $_;
	}
	close (FH);
	open (FH,">$current_dir/$cfg_dir/wanrouter.rc");
	$rcfile =~ s/WAN_DEVICES\s*=.*".*/WAN_DEVICES="$startup_string"/g;
	print FH $rcfile;
	close (FH);
}

sub update_zapata_template{
	#update comments for zapata.conf or chan_dahdi.conf
	my $zapfile="";
	if (!open (FH,"$zapata_conf_file")) {
		printf("Unable to modify $zapata_conf_file\n");
		
	}
	while (<FH>) {
 		$zapfile .= $_;
	}
	close (FH);
	open (FH,">$zapata_conf_file");
	$zapfile =~ s/ZAPATA_STRING/$zaptel_string/g;
	$zapfile =~ s/LOCATION/$zapata_conf_file_t/g;
	$zapfile =~ s/WANCFG_CONFIG/$wancfg_config/g;
	$zapfile =~ s/DATE/$date/g;
	print FH $zapfile;
	close (FH);
}


sub update_zaptel_template{
	#update coments forzaptel.conf or chan_dahdi.conf 
	my $zapfile="";
	if (!open (FH,"$zaptel_conf_file")) {
		printf("Unable to modify $zaptel_conf_file\n");
		
	}
	while (<FH>) {
 		$zapfile .= $_;
	}
	close (FH);
	open (FH,">$zaptel_conf_file");
	$zapfile =~ s/ZAPATA_STRING/$zaptel_string/g;
	$zapfile =~ s/LOCATION/$zaptel_conf_file_t/g;
	$zapfile =~ s/WANCFG_CONFIG/$wancfg_config/g;
	$zapfile =~ s/DATE/$date/g;
	print FH $zapfile;
	close (FH);
}


sub prepare_files{
	
	if ($is_trixbox==$TRUE || ($silent_zapata_conf_file==$TRUE && $silent==$TRUE)){
		$zapata_conf_template="$current_dir/templates/zapata-auto.conf";
		$zapata_conf_file="$current_dir/$cfg_dir/zapata-auto.conf";
		$zapata_conf_file_t="$asterisk_conf_dir/zapata-auto.conf";
		
		if($dahdi_installed == $TRUE) {
			$zapata_conf_template="$current_dir/templates/dahdi-channels.conf";
			$zapata_conf_file="$current_dir/$cfg_dir/dahdi-channels.conf";
			$zapata_conf_file_t="$asterisk_conf_dir/dahdi-channels.conf";
		}

	}

	if ($silent==$FALSE){
		if ($is_trixbox==$FALSE && $is_smg==$FALSE && $is_tdm_api==$FALSE && $is_hp_tdm_api==$FALSE){
			print "Would you like to generate $zapata_conf_file_t\n";
			if (&prompt_user_list(("YES","NO","")) eq 'NO'){
				$config_zapata = $FALSE;
			}
		}elsif($is_fs==$TRUE && $is_openzap== $FALSE){
			print"Would you like to change FreeSWITCH Configuration Directory?\nDefault: $fs_conf_dir\n";
			if (&prompt_user_list(("NO","YES","NO")) eq 'YES'){

				my $autoload_configs;
				$fs_conf_dir=&prompt_user("Enter FreeSWITCH Conf Directory \n");
				while(! -d $fs_conf_dir){
					#print "Invalid FreeSwitch Configuration Directory, Please Enter FreeSwitch Configuration Directory\n";
					#$fs_conf_dir = "mkdir ".$fs_conf_dir;
					$autoload_configs = $fs_conf_dir."/autoload_configs";
					system(mkdir $fs_conf_dir);
					system(mkdir $autoload_configs);
					#$fs_conf_dir=&prompt_user("Input the FreeSwitch Conf Dir",$fs_conf_dir);
				}

				$autoload_configs = $fs_conf_dir."/autoload_configs";
				while(! -d $autoload_configs){
					system(mkdir $autoload_configs);
				}

				$openzap_conf_file_t="$fs_conf_dir/openzap.conf";
				$openzap_conf_xml_file_t="$fs_conf_dir/autoload_configs/openzap.conf.xml";
				$freetdm_conf_file_t="$fs_conf_dir/freetdm.conf";
				$freetdm_conf_xml_file_t="$fs_conf_dir/autoload_configs/freetdm.conf.xml"
			}
		}	
	}

#remove temp files
	my $tmp_cfg_dir="$current_dir"."/"."$cfg_dir";
	if ( -d "$current_dir"."/"."$cfg_dir") {
		exec_command("rm -f $current_dir/$cfg_dir/*");
	}

#backup existing configuration files
	if ( -f $zaptel_conf_file_t ) {
		exec_command("cp -f $zaptel_conf_file_t $zaptel_conf_file_t.bak ");
	} 

	if ( -f $zapata_conf_file_t ) {
		exec_command("cp -f $zapata_conf_file_t $zapata_conf_file_t.bak");
	}
	
	if( -f $openzap_conf_xml_file_t) {
		exec_command("cp -f $openzap_conf_xml_file_t $openzap_conf_xml_file_t.bak");	
	}
	if (-f $openzap_conf_file_t){
		exec_command("cp -f $openzap_conf_file_t $openzap_conf_file_t.bak");
	}

}
sub parse_wanrouter_rc
{	
	#Set ztcfg_path based on $etc_dir/wanpipe/wanrouter.rc
	if ( -f "$etc_dir/wanpipe/wanrouter.rc" ) {
		my $line= `cat $etc_dir/wanpipe/wanrouter.rc   | grep ZAPTEL_BIN_DIR`;
		chop($line);
			{
			my @parts = split(/=/,$line);
			$ztcfg_path="$parts[1]\/";
			}
		#Use this wanrouter.rc as new template 
		my $command="cp -f $etc_dir/wanpipe/wanrouter.rc  $current_dir/templates/wanrouter.rc.template.new > /dev/null 2>/dev/null ";
		$wanrouter_rc_template="$current_dir/templates/wanrouter.rc.template.new";

	}

}

sub update_zaptel_cfg_script(){

	#update zaptel_cfg_script based on ztcfg_path

	my $sscript ="";
	open (FH,"$current_dir/templates/$zaptel_cfg_script");
	
	while (<FH>) {
		$sscript .= $_;
	}
	close (FH);
	open (FH,">$current_dir/templates/$zaptel_cfg_script");
	$sscript =~ s/.*ztcfg.*/$ztcfg_path\ztcfg -v/;
	print FH $sscript;
	close (FH);
}
sub update_start_smgbri_safe{

	#update start script for safe start

	my $sscript ="";
	open (FH,"$wanpipe_conf_dir/scripts/start");
	
	while (<FH>) {
		$sscript .= $_;
	}
	close (FH);
	open (FH,">$wanpipe_conf_dir/scripts/start");
	$sscript =~ s/.*smg_ctrl start.*/smg_ctrl safe_start/;
	print FH $sscript;
	close (FH);
}

sub clean_files{
	exec_command("rm -rf $current_dir/$cfg_dir");
}

sub write_zapata_conf{
	my $zp_file="";
	open(FH, "$zapata_conf_template") or die "cannot open $zapata_conf_template";
	while (<FH>) {
		$zp_file .= $_;
	}
	close (FH);
	
	$zp_file.=$zapata_conf;	
	
	open(FH, ">$zapata_conf_file") or die "cannot open $zapata_conf_file";
		print FH $zp_file;
	close(FH);	
}

sub copy_config_files{
	my @wanpipe_files = split / /, $cfg_string; 	
	exec_command("cp -f $current_dir/$cfg_dir/wanrouter.rc $wanpipe_conf_dir");
	foreach my $wanpipe_file (@wanpipe_files) {
		exec_command("cp -f $current_dir/$cfg_dir/$wanpipe_file.conf $wanpipe_conf_dir");
	}
}

sub unload_zap_modules{
	my @modules_list = ("ztdummy","wctdm","wcfxo","wcte11xp","wct1xxp","wct4xxp","wctc4xxp","wcb4xxp","tor2","zttranscode","wcusb", "wctdm24xxp","xpp_usb","xpp" ,"wcte12xp","opvxa1200", "dahdi_dummy" ,"dahdi_transcode","dahdi_echocan_mg2", "dahdi", "zaptel");
	foreach my $module (@modules_list) {	
		if ($modprobe_list =~ m/$module /){
			exec_command("$module_unload $module");
		}
	}
}

sub write_zaptel_conf{
	my $zp_file="";
	open(FH, "$zaptel_conf_template") or die "cannot open $zaptel_conf_template";
	while (<FH>) {
		$zp_file .= $_;
	}
	close (FH);
	$zp_file=$zp_file.$zaptel_conf;	
	open(FH, ">$zaptel_conf_file") or die "cannot open $zaptel_conf_file";
		print FH $zp_file;
	close(FH);	
}

sub summary{
	if($devnum==1){
		if ($silent==$FALSE) {system('clear')};
		print "\n###################################################################";
		print "\n#                             SUMMARY                             #";
		print "\n###################################################################\n\n";
							
		print("\n\nNo Sangoma voice compatible cards found/configured\n\n"); 
		exit 0;
	}else{
		if ($num_zaptel_config != 0 && $config_zaptel==$TRUE){
			write_zaptel_conf();
			update_zaptel_template();
		}
		if($is_trillium == $FALSE){
			if($num_digital_smg != 0){
				write_pri_conf();
			}
		}

		if ($num_bri_devices != 0 && ($is_fs == $TRUE | $is_smg == $TRUE )){
			write_bri_conf();
		}

		if( $config_woomera == $TRUE && ($num_bri_devices != 0 | $num_digital_smg != 0)) { 
			write_woomera_conf();
		}

		if ($num_zaptel_config != 0 && $config_zapata==$TRUE){
			write_zapata_conf();
			update_zapata_template();
			
		}
	

		if($is_fs == $TRUE) {
			if($is_trillium == $FALSE){
				if($num_digital_devices != 0){
					write_pri_conf();
				}
			}
			if ($is_ftdm == $FALSE){
				write_openzap_conf();
				if($is_openzap == $FALSE){
					write_openzap_conf_xml();
				}
			}
			if ($is_ftdm == $TRUE){
				write_freetdm_conf_legacy();
				write_freetdm_conf_xml_legacy();
			}
			if ($is_trillium == $TRUE){
				write_freetdm_conf();
				write_freetdm_conf_xml()
			}	
		}

		save_debug_info();
		if ($silent==$FALSE) {system('clear')};
		my $file_list = 1;
		print "\n###################################################################";
		print "\n#                             SUMMARY                             #";
		print "\n###################################################################\n\n";

		print("  $num_digital_devices_total T1/E1 port(s) detected, $num_digital_devices configured\n");
		print("  $num_bri_devices_total ISDN BRI port(s) detected, $num_bri_devices configured\n");
		print("  $num_analog_devices_total analog card(s) detected, $num_analog_devices configured\n");
		print("  $num_gsm_devices_total GSM card(s) detected, $num_gsm_devices configured\n");
  	    print("  $num_usb_devices_total usb device(s) detected, $num_usb_devices configured\n");

		
		print "\nConfigurator will create the following files:\n";
		print "\t1. Wanpipe config files in $wanpipe_conf_dir\n";
		$file_list++;
		
		if (($num_bri_devices != 0) && ($is_trillium == $FALSE)){
			if(($is_tdm_api == $FALSE) && ($is_fs == $TRUE | $is_smg == $TRUE) ){
				print "\t$file_list. sangoma_brid config file $wanpipe_conf_dir/smg_bri\n";
				$file_list++;
			}
		}
		if(($num_digital_smg != 0) && ($is_trillium == $FALSE)){
			print "\t$file_list. sangoma_prid config file $wanpipe_conf_dir/smg_pri\n";
			$file_list++;
		}
		if($config_openzap == $TRUE){
			print "\t$file_list. openzap config file $fs_conf_dir/openzap\n";
			$file_list++;
		}
		
		if($config_openzap_xml == $TRUE){
			print "\t$file_list. openzap_xml config file $fs_conf_dir/openzap.conf.xml\n";
			$file_list++;
		}

		if($config_freetdm == $TRUE){
			print "\t$file_list. freetdm config file $fs_conf_dir/freetdm.conf\n";
			$file_list++;
		}
		
		if($config_freetdm_xml == $TRUE){
			print "\t$file_list. freetdm_xml config file $fs_conf_dir/freetdm.conf.xml\n";
			$file_list++;
		}
		
		if ($num_zaptel_config != 0){
			print "\t$file_list. $zaptel_string config file $zaptel_conf_file_t\n";
			$file_list++;
		}
				
		if (($num_digital_smg !=0 || $num_bri_devices !=0) && ($is_trillium == $FALSE)){
			if(($is_tdm_api == $FALSE) && ($is_fs == $TRUE | $is_smg == $TRUE) ){
				print "\t$file_list. smg.rc config file $wanpipe_conf_dir/smg.rc\n";
				$file_list++
			}
			
		}
		
		if ($config_zapata==$TRUE){
			print "\t$file_list. $zapata_string config file $zapata_conf_file_t\n";
			$file_list++
		}

		if ($num_bri_devices != 0 &&  $is_smg == $TRUE ){
			print "\t$file_list. woomera config file $asterisk_conf_dir/woomera.conf \n";
			$file_list++
		}

		if (($num_zaptel_config != 0) | ($config_zapata==$TRUE)){	
			print "\n\nYour original configuration files will be saved to:\n";
			$file_list=1;
		}	
			
		if ($num_zaptel_config != 0){
			print "\t$file_list. $zaptel_conf_file_t.bak \n";
			$file_list++;
		}

		if ($num_zaptel_config !=0 && $config_zapata==$TRUE){
			print "\t$file_list. $zapata_conf_file_t.bak \n\n";
		}
		
		print "\nYour configuration has been saved in $debug_tarball.\n";
		print "When requesting support, email this file to techdesk\@sangoma.com\n\n";
		print "\n###################################################################\n\n";
		if($silent==$FALSE){
			confirm_conf();
		}
	}
}
sub confirm_conf(){
	print "Configuration Complete! Please select following:\n";
	if(&prompt_user_list("YES - Continue", "NO - Exit" ,"") =~ m/YES/){
		return $?;
	} else {
		print "No changes made to your configuration files\n";
		print "exiting.....\n";	
		exit $?;
	}
}



sub exec_command{
	my @command = @_;
	if (system(@command) != 0 ){
		print "Error executing command:\n@command\n\n";
		if($silent==$FALSE){
			print "Would you like to continue?\n";
			if(&prompt_user_list("No - exit", "YES", "No") eq 'YES'){
				return $?;
			} else {
				exit $?;
			}
		}
	}
	return $?;
}

sub prompt_user{
	my($promptString, $defaultValue) = @_;
	if ($defaultValue) {
	print $promptString, "def=\"$defaultValue\": ";
	} else {
	print $promptString, ": ";
	}

	$| = 1;               # force a flush after our print
	$_ = <STDIN>;         # get the input from STDIN (presumably the keyboard)
	chomp;
	if ("$defaultValue") {
	return $_ ? $_ : $defaultValue;    # return $_ if it has a value
	} else {
	return $_;
	}
}

sub prompt_user_list{
	my @list = @_;
	my $defaultValue = @list[$#list];
	my $i;
	my $valid = 0;

	if ($silent==$TRUE){
		return $defaultValue;
	}

	for $i (0..$#list-1) {
		printf(" %s\. %s\n",$i+1, @list[$i]);
	}
	while ($valid == 0){
		my $list_sz = $#list;
		print "[1-$list_sz";	
		if ( ! $defaultValue eq ''){
			print ", ENTER=\'$defaultValue\']:";	
		}else{
			print "]:";
		}
		$| = 1;               
		$_ = <STDIN>;         
		chomp;
		if ( $_ eq '' & ! $defaultValue eq ''){
			print "\n";
			return $defaultValue;
		}
			
		if ( $_ =~ /(\d+)/ ){
			if ( $1 > $list_sz) {
				print "\nError: Invalid option, input a value between 1 and $list_sz \n";
			} else {
				print "\n";
				return @list[$1-1] ;
			}
		} else {
			print "\nError: Invalid option, input an integer\n";
		}
	}
}

sub read_args {
	my $arg_num;
	foreach $arg_num (0 .. $#ARGV) {
		$_ = $ARGV[$arg_num];
		if( /^--trixbox$/){
			$is_trixbox=$TRUE;
			$is_dahdi=$TRUE;
		}elsif ( /^--dahdi/){
			$is_dahdi=$TRUE;
		}elsif ( /^--zaptel/){
			$is_zaptel=$TRUE;
		}elsif ( /^--install_boot_script/){
                        $boot_only=$TRUE;
		}elsif ( /^--tdm_api/){
			$is_tdm_api=$TRUE;
		}elsif ( /^--data_api/){
			$is_data_api=$TRUE;
			$is_tdm_api=$TRUE;
		}elsif ( /^--smg$/){
			$is_smg=$TRUE;
		}elsif ( /^--hp_tdm_api/){
			$is_hp_tdm_api=$TRUE;
		}elsif ( /^--no_boot$/){
			$no_boot=$TRUE;	
		}elsif ( /^--no_smgboot$/){
			$no_smgboot=$TRUE;			
		}elsif ( /^--conf_fs$/){	#openzap.conf, openzap.conf.xml, wanpipe conf
			$is_fs=$TRUE;
			$is_tdm_api=$TRUE;#fs use tdmapi mode	
		}elsif ( /^--conf_openzap$/){	#openzap.conf, wanpipe conf
			$is_openzap=$TRUE;	#is_openzap means do not configure openzap.conf.xml 
			$is_fs=$TRUE;
			$is_tdm_api=$TRUE;#fs use tdmapi mode
			$config_openzap_xml=$FALSE;
		}elsif ( /^--ftdm_api$/){	#freetdm.conf, freetdm.conf.xml, wanpipe conf
			$is_fs=$TRUE;
			$is_ftdm=$TRUE;
			$is_tdm_api=$TRUE;#fs use tdmapi mode
			$is_openzap=$FALSE
		}elsif ( /^--trillium$/){       #freetdm.conf, freetdm.conf.xml, wanpipe conf
			$is_fs=$TRUE;
			$is_trillium=$TRUE;
			$is_tdm_api=$TRUE;#fs use tdmapi mode
			$is_openzap=$FALSE
		}elsif ( /^--no_hwdtmf$/){
			$no_hwdtmf=$TRUE;
		}elsif ( /^--silent$/){
			$silent=$TRUE;
		}elsif (/^--safe_mode$/){
			$safe_mode=$TRUE;
		}elsif ( /^--no-zapata$/){
			$config_zapata=$FALSE;
		}elsif ( /^--no-chan-dahdi$/){
			$config_zapata=$FALSE;
		}elsif ( /^--no-zaptel$/){
			$config_zaptel=$FALSE;
		}elsif ( /^--no-dahdi$/){
			$config_zaptel=$FALSE;
		}elsif ( $_ =~ /--zapata_context=(\w+)/){
			$silent_zapata_context_opt=$TRUE;
			$silent_zapata_context=substr($_,length("--zapata_context="));
			push(@silent_zapata_contexts, $silent_zapata_context);
		}elsif ( $_ =~ /--zapata_group=(\d+)/){
			$silent_zapata_group_opt=$TRUE;
			$silent_zapata_group=$1;
			push(@silent_zapata_groups, $silent_zapata_group);
		}elsif ( $_ =~ /--woomera_context=(\w+)/){
			$silent_woomera_context=substr($_,length("--woomera_context="));
			push(@silent_woomera_contexts, $silent_woomera_context);
		}elsif ( $_ =~ /--woomera_group=(\d+)/){
			$silent_woomera_group=$1;
			push(@silent_woomera_groups, $silent_woomera_group);
		}elsif ( $_ =~ /--fe_media=(\w+)/){
			$silent_femedia=$1;
			if(!($silent_femedia eq 'T1' || $silent_femedia eq 'E1')){
				printf("Invalid value for fe_media, should be T1/E1\n");
				exit(1);
			} else {	
				push(@silent_femedias, $silent_femedia);
				if($silent_femedia eq 'E1'){
					$silent_tdm_law="ALAW";	
					if(!($silent_feframe eq 'CRC4' || $silent_feframe eq 'NCRC4')){
						$silent_feframe='NCRC4';
					}
					if(!($silent_felcode eq 'HDB3' || $silent_felcode eq 'AMI')){
						$silent_felcode='HDB3';
					}
				}
			}
		}elsif ( $_ =~ /--fractional_chans=(\d+)-(\d+)/ ){
			$silent_first_chan=$1;
			$silent_last_chan=$2;
			push(@silent_first_chans, $silent_first_chan);
			push(@silent_last_chans, $silent_last_chan);
		}elsif ( $_ =~ /--hw_dtmf=(\w+)/){
			$silent_hwdtmf=$1;
			if(!($silent_hwdtmf eq 'YES' || $silent_hwdtmf eq 'NO')){
				printf("Invalid value for hw_dtmf, should be YES/NO\n");
				exit(1);
			} else {	
				push(@silent_hwdtmfs, $silent_hwdtmf);
			}
		}elsif ( $_ =~ /--fe_lcode=(\w+)/){
			$silent_felcode=$1;
			if(!($silent_felcode eq 'B8ZS' || $silent_felcode eq 'HDB3' || $silent_felcode eq 'AMI')){
				printf("Invalid value for fe_lcode, should be B8ZS/HDB3/AMI \n");
				exit(1);
			} else {	
				push(@silent_felcodes, $silent_felcode);
			}			
		}elsif ( $_ =~ /--fe_frame=(\w+)/){
			$silent_feframe=$1;
			if(!($silent_feframe eq 'ESF' || $silent_feframe eq 'D4' || $silent_feframe eq 'CRC4' || $silent_feframe eq 'NCRC4')){
				printf("Invalid value for fe_frame, should be ESF/D4/CRC4/NCRC4\n");
				exit(1);
			} else {	
				push(@silent_feframes, $silent_feframe);
			}	
		}elsif ( $_ =~ /--fe_te_sig=(\w+)/){
			$silent_te_sig_mode=$1;
			if(!($silent_te_sig_mode eq 'CCS' || $silent_te_sig_mode eq 'CAS')){
				printf("Invalid value for fe_te_sig, should be CCS/CAS\n");
				exit(1);
			}
		}elsif ( $_ =~ /--tdm_law=(\w+)/){
			$silent_tdm_law=$1;
			if(!($silent_tdm_law eq 'MULAW' || $silent_tdm_law eq 'ALAW')){
				printf("Invalid value for tdm_law, should be MULAW/ALAW\n");
				exit(1);
			} else {	
				push(@silent_tdm_laws, $silent_tdm_law);
			}
		}elsif ( $_ =~ /--fe_clock=(\w+)/){
			$silent_feclock=$1;
			if(!($silent_feclock eq 'NORMAL' || $silent_feclock eq 'MASTER' )){
				printf("Invalid value for fe_clock, should be NORMAL/MASTER\n");
				exit(1);
			} else {	
				push(@silent_feclocks, $silent_feclock);
			}			
		}elsif ( $_ =~ /--signalling=(\w+)/){
			my $tmp_signalling=$1;
			if ($tmp_signalling eq 'em'){
				$silent_signalling="Zaptel/Dahdi - E & M";
			}elsif ($tmp_signalling eq 'em_w'){
				$silent_signalling="Zaptel/Dahdi - E & M Wink";
			}elsif ($tmp_signalling eq 'pri_cpe'){
				$silent_signalling="Zaptel/Dahdi - PRI CPE";
			}elsif ($tmp_signalling eq 'pri_net'){
				$silent_signalling="Zaptel/Dahdi - PRI NET";
			}elsif ($tmp_signalling eq 'fxs_ls'){
				$silent_signalling="Zaptel/Dahdi - FXS - Loop Start";
			}elsif ($tmp_signalling eq 'fxs_gs'){
				$silent_signalling="Zaptel/Dahdi - FXS - Ground Start";
			}elsif ($tmp_signalling eq 'fxs_ks'){
				$silent_signalling="Zaptel/Dahdi - FXS - Kewl Start";
			}elsif ($tmp_signalling eq 'fxo_ls'){
				$silent_signalling="Zaptel/Dahdi - FXO - Loop Start";
			}elsif ($tmp_signalling eq 'fxo_gs'){
				$silent_signalling="Zaptel/Dahdi - FXO - Ground Start";
			}elsif ($tmp_signalling eq 'fxo_ks'){
				$silent_signalling="Zaptel/Dahdi - FXO - Kewl Start";
			}else{
				printf("Invalid option for --signalling, options are\n");
				printf("\t pri_cpe/pri_net/em/em_w\n");
				printf("\t fxo_ls/fxo_gs/fxo_ks\n");
				printf("\t fxs_ls/fxs_gs/fxs_ks\n");
				exit(1);
			}
			
			push(@silent_signallings, $silent_signalling);
						
		}elsif ( $_ =~ /--pri_switchtype=(\w+)/){
			my $tmp_switchtype=$1;
			if ($tmp_switchtype eq 'national'){
				$silent_pri_switchtype="national"
			}elsif ($tmp_switchtype eq 'dms100'){
				$silent_pri_switchtype="dms100"
			}elsif ($tmp_switchtype eq '4ess'){
				$silent_pri_switchtype="4ess"
			}elsif ($tmp_switchtype eq '5ess'){
				$silent_pri_switchtype="5ess"
			}elsif ($tmp_switchtype eq 'euroisdn'){
				$silent_pri_switchtype="euroisdn"
			}elsif ($tmp_switchtype eq 'ni1'){
				$silent_pri_switchtype="ni1"
			}elsif ($tmp_switchtype eq 'qsig'){
				$silent_pri_switchtype="qsig"
			} else {
				printf("Invalid option for --pri_switchtype, options are\n");
				printf("\t national/dms100/4ess/5ess/euroisdn/ni1/qsig");
				exit(1);
			}
			push(@silent_pri_switchtypes, $silent_pri_switchtype);
		}elsif ( $_ =~ /--conf_dir=(.*)/){
			$etc_dir=$1;
			if (! -d $etc_dir){	
				printf("Error: directory $1 does not exist\n");
				exit(1);
			}
		}elsif ( $_ =~/--zapata_auto_conf/){
			$silent_zapata_conf_file=$TRUE;
		
		}elsif ( $_ =~ /--fs_conf_dir=(.*)/){
			$fs_conf_dir=$1;
			if (! -d $fs_conf_dir){	
				printf("Error: FreeSwitch conf directory $1 does not exist\n");
				exit(1);
			}
		}elsif ( $_ =~ /--hw_dchan=(.*)/){
			$silent_hw_dchan=$1;
		}elsif ( $_ =~ /--tapping_mode$/){
			$def_tapping_mode='YES';
		}elsif ( $_ =~ /--data_hdlc=(.*)/){
			$def_hdlc_mode=$1;
		}elsif ( $_ =~ /--wanpipe_port=(.*)/){
			$cfg_wanpipe_port=$1;
			$current_zap_span=get_current_span_no($cfg_wanpipe_port);
			$current_tdmapi_span=get_current_span_no($cfg_wanpipe_port);
		}elsif ( $_ =~ /--wanpipe_span=(.*)/){
			$current_zap_span=$1;
			$current_tdmapi_span=$1;
		}else {
			printf("Error: Unrecognized parameter \"$_\" \n");
			exit(1);

		}
	}
	@silent_femedias = reverse(@silent_femedias);
	@silent_feframes = reverse(@silent_feframes);
	@silent_felcodes = reverse(@silent_felcodes);
	@silent_tdm_laws = reverse(@silent_tdm_laws);
	@silent_feclocks = reverse(@silent_feclocks);
	@silent_signallings = reverse(@silent_signallings);
	@silent_pri_switchtypes = reverse(@silent_pri_switchtypes);
	@silent_zapata_contexts = reverse(@silent_zapata_contexts);
	@silent_woomera_contexts = reverse(@silent_woomera_contexts);
	@silent_zapata_groups = reverse(@silent_zapata_groups);
	@silent_hwdtmfs = reverse(@silent_hwdtmfs);
	@silent_first_chans = reverse(@silent_first_chans);
	@silent_last_chans = reverse(@silent_last_chans);

	if ($is_trixbox==$TRUE){
		print "\nGenerating configuration files for Trixbox\n";
	}
	if ($is_smg==$TRUE){
		print "\nGenerating configuration files for Sangoma Media Gateway\n";
	}
	if ($is_tdm_api==$TRUE | $is_hp_tdm_api==$TRUE | $is_fs == $TRUE){
		$config_zapata = $FALSE;    
	}

}

#------------------------------BRI FUNCTIONS------------------------------------#
sub get_bri_country {
	$def_bri_country = "europe"; 
	return $def_bri_country;
}

sub get_woomera_group{
	if($silent==$TRUE){
		if($#silent_woomera_groups >= 0){
			$silent_woomera_group=pop(@silent_woomera_groups);
		}
		return $silent_woomera_group;
	}

	my $group;
	my $res_group=&prompt_user("\nInput the dialing group for this port\n",$def_woomera_group);
	while(length($res_group)==0 |!($res_group =~/(\d+)/)| $res_group eq '0'){
		print "Invalid group, input an integer greater than 0\n";
		$res_group=&prompt_user("Input the dialing group for this port",$def_woomera_group);
	}
	$group=$res_group;
	$def_woomera_group=$group;
	return $def_woomera_group;
}



sub get_bri_default_tei{
#	if($silent==$TRUE){
#		if($#silent_woomera_groups >= 0){
#			$silent_woomera_group=pop(@silent_woomera_groups);
#		}
#		return $silent_woomera_group;
#	}

	my $res_default_tei;
get_bri_default_tei:
	$res_default_tei=&prompt_user("\nInput the TEI for this port \n",$def_bri_default_tei);
	while(length($res_default_tei)==0 |!($res_default_tei =~/(\d+)/)){
		print "Invalid TEI value, must be an integer\n";
		$res_default_tei=&prompt_user("Input the TEI for this port ",$def_bri_default_tei);
	}
	$res_default_tei =~ /(\d+)/;	
	if(  $1 < 0 | $1 > 127)	{
			print "Invalid TEI value, must be between 0 and 127\n";
			goto get_bri_default_tei;
	}
	
	$def_bri_default_tei=$res_default_tei;
	return $def_bri_default_tei;
}



sub get_bri_operator {
#warning returning ETSI
	$def_bri_operator = "etsi";
	return $def_bri_operator;



	my @options = ( "European ETSI Technical Comittee", "France Telecom VN2", "France Telecom VN3",
			"France Telecom VN6", "Deutsche Telekom 1TR6", "British Telecom ISDN2",
			"Belgian V1", "Sweedish Televerket", "ECMA 143 QSIG");

	my @options_val = ("etsi", "ft_vn2", "ft_vn3", "ft_vn6", "dt_1tr6", "bt_isdn2", "bg_vi", "st_swd", "ecma_qsi");

	my $res = &prompt_user_list(@options,$def_switchtype);
	
	my $i;
	for $i (0..$#options){
		if ( $res eq @options[$i] ){
			$def_bri_operator=@options[$i];
			return @options_val[$i]; 
		}
	}	
	print "Internal error: Invalid PRI switchtype\n";
	exit 1;
}

sub write_bri_conf{
	my $bri_file="";
	open(FH, "$bri_conf_template") or die "cannot open $bri_conf_template";
	while (<FH>) {
		$bri_file .= $_;
	}
	close (FH);
	$bri_file=$bri_file.$bri_conf;	
	open(FH, ">$bri_conf_file") or die "cannot open $bri_conf_file";
		print FH $bri_file;
	close(FH);	
}

sub write_pri_conf{
	my $pri_file="";
#	printf("Generating T1 BOOST PRI Config...")
	open(FH, "$pri_conf_template") or die "cannot open $pri_conf_template";
	while (<FH>) {
		$pri_file .= $_;
	}
	close (FH);
	$pri_file=$pri_file.$pri_conf;	
	open(FH, ">$pri_conf_file") or die "cannot open $pri_conf_file";
		print FH $pri_file;
	close(FH);	
}

sub write_woomera_conf{
	my $woomera_file="";
	open(FH, "$woomera_conf_template") or die "cannot open $woomera_conf_template";
	while (<FH>) {
		$woomera_file .= $_;
	}
	close (FH);
	$woomera_file=$woomera_file.$woomera_conf;	
	open(FH, ">$woomera_conf_file") or die "cannot open $woomera_conf_file";
		print FH $woomera_file;
	close(FH);	
}


sub get_bri_conn_type{
	my ($port)=@_;
	
	if($silent==$TRUE){
		if($#silent_bri_conn_types >= 0){
			$silent_bri_conn_type=pop(@silent_bri_conn_types);
		}
		return $silent_bri_conn_type;
	}
        printf("\nSelect connection type for port %d\n", $port);
	my $conn_type;
	
	my @options = ( "Point to multipoint", "Point to point");
	my @options_val = ("point_to_multipoint", "point_to_point");

	my $res = &prompt_user_list(@options,$def_bri_conn_type);
	
	my $i;
	for $i (0..$#options){
		if ( $res eq @options[$i] ){
			$def_bri_conn_type=@options[$i];
			return @options_val[$i]; 
		}
	}	
	print "Internal error: Invalid BRI connection type\n";
	exit 1;
}

sub config_bri{
	my $a50x;
	if (!$first_cfg && $silent==$FALSE) {
		system('clear');
	}
	$first_cfg=0;
	print "-------------------------------------------\n";
	print "Configuring ISDN BRI cards [A500/B500/B700]\n";
	print "-------------------------------------------\n";
	my $skip_card=$FALSE;
	$zaptel_conf.="\n";
	$zapata_conf.="\n";
	
	foreach my $dev (@hwprobe) {
			
		if ($cfg_wanpipe_port && $cfg_wanpipe_port ne get_wanpipe_port($dev)) {
			next;	
		}

		if ( $dev =~ /.*AFT-A(\d+)(.*):.*SLOT=(\d+).*BUS=(\d+).*PORT=(\d+).*HWEC=(\w+).*/ ||
			 $dev =~ /.*AFT-B(\d+)(.*):.*SLOT=(\d+).*BUS=(\d+).*PORT=(\d+).*HWEC=(\w+).*/){
			$skip_card=$FALSE;

	

			if ($1 eq '500' || ($1 eq '700' && $5 < '5')){
				my $card = eval {new Card(); } or die ($@);
			
				$card->first_chan($current_zap_channel);
				$card->current_dir($current_dir);
				$card->cfg_dir($cfg_dir);
				$card->device_no($cfg_wanpipe_port?$cfg_wanpipe_port:$devnum);
				$card->card_model($1);
				$card->card_model_name($dev);
				$card->pci_slot($3);
				$card->pci_bus($4);
				

				my $hwec=0;
				if($6 gt 0){
					$hwec=1;	
				}
				if ($hwec==0){
					$card->hwec_mode('NO');
				}else{
					$card->hwec_mode('YES');
				}
				
				if($dahdi_installed == $TRUE && $is_smg == $FALSE && $is_tdm_api == $FALSE) {
					$card->dahdi_conf('YES');
					$card->dahdi_echo($dahdi_echo);
					if ($hwec == 0) {
						$card->dahdi_echo('mg2');
					}
				}

				if ($card->card_model eq '500' || $card->card_model eq '700'){
					$num_bri_devices_total++;
					if($5 eq '1'){
						$bri_device_has_master_clock=$FALSE;
					}
					if ($silent==$FALSE) {
						system('clear');
						print "\n-----------------------------------------------------------\n";
						printf("AFT-%s detected on slot:$3 bus:$4\n", $card->card_model_name());
						print "-----------------------------------------------------------\n";
					}
				
select_bri_option:
					if($silent==$FALSE){
						printf ("\nWould you like to configure AFT-%s port %s on slot:%s bus:%s\n",
										$card->card_model_name(), $5, $3, $4);
						my @options=("YES", "NO", "Exit");
						$def_bri_option=&prompt_user_list(@options,$def_bri_option);
					} else {
						$def_bri_option="YES";
					}

					
					if($def_bri_option eq 'YES'){
						$skip_card=$FALSE;
						if ($is_fs == $TRUE | $is_smg == $TRUE) {
							$bri_conf.="\n;Sangoma AFT-".$card->card_model_name()." port $5 [slot:$3 bus:$4 span:$current_tdmapi_span]";
						} else {
							$bri_conf.="\n;Sangoma AFT-".$card->card_model_name()." port $5 [slot:$3 bus:$4 span:$current_zap_span]";
						}
					}elsif($def_bri_option eq 'NO'){
						$skip_card=$TRUE;
					}else{
						print "Exit without applying changes?\n";
						if (&prompt_user_list(("YES","NO","YES")) eq 'YES'){
							print "No changes made to your configuration files.\n\n";
							exit 0;
						} else {
							goto select_bri_option;
						}
					}
					if ($skip_card==$FALSE){
						$startup_string.="wanpipe$devnum ";			
						$cfg_string.="wanpipe$devnum ";			
						$a50x = eval {new A50x(); } or die ($@);
						$a50x->card($card);
						$a50x->fe_line($5);
						$devnum++;
						$num_bri_devices++;
						if ($is_fs == $TRUE || $is_smg == $TRUE || $is_tdm_api == $TRUE ) {
							$card->tdmv_span_no($current_tdmapi_span);
							$current_tdmapi_span++;
						} else {
							$num_zaptel_config++;
							$card->tdmv_span_no($current_zap_span);
							$current_zap_span++;
							$current_zap_channel+=3;
						}
						if ($silent==$FALSE){
							if ($card->hwec_mode eq "YES"){
								$card->hw_dtmf(&prompt_hw_dtmf());
								if ($card->hw_dtmf eq "YES"){
									$card->hw_fax(&prompt_hw_fax());
								}
							} else {
								$card->hw_dtmf("NO");
								$card->hw_fax("NO");
							}
						}
					}else{
						printf ("%s on slot:%s bus:%s port:%s not configured\n", $card->card_model_name, $3, $4, $5);
						prompt_user("Press any key to continue");
					}

				}				
			}
		}elsif (($dev =~ /(\d+):NT/ | 
	 		$dev =~ /(\d+):TE/ )& 
	 		$skip_card==$FALSE ){
	 		my $context="";
	 		my $group="";
			my $bri_pos=$a50x->card->tdmv_span_no;
			
			if ($is_fs == $TRUE || $is_smg == $TRUE || $is_tdm_api == $TRUE ) {
				printf("\nConfiguring port %d on AFT-%s [slot:%d bus:%d span:%d]\n", $a50x->fe_line(), $a50x->card->card_model_name(), $a50x->card->pci_slot(), $a50x->card->pci_bus(), $current_tdmapi_span-1);
			} else {
				printf("\nConfiguring port %d on AFT-%s [slot:%d bus:%d span:%d]\n", $a50x->fe_line(), $a50x->card->card_model_name(), $a50x->card->pci_slot(), $a50x->card->pci_bus(), $current_zap_span-1);
			}
			my $conn_type="";
			if( $is_fs == $TRUE | $dev =~ /(\d+):TE/ | $is_smg == $TRUE ){
				$conn_type=get_bri_conn_type($a50x->fe_line());
			}
			my $country=get_bri_country();
			my $operator=get_bri_operator();
			if( ($is_trixbox==$TRUE && $silent==$FALSE)){
				if ( $dev =~ /(\d+):NT/ ){
					$context="from-internal";
					$group=1;
				} else {
					$context="from-zaptel";
					$group=2;
				}
			} elsif ($is_fs == $TRUE) {
				if ( $dev =~ /(\d+):NT/ ){
					#$context="from-internal";
					$group=1;
				} else {
					#$context="from-zaptel";
					$group=1;
				}
				
			} elsif ($is_fs == $FALSE && $is_smg == $FALSE) {
				$group=get_woomera_group();
				$context=get_zapata_context($a50x->card->card_model_name);
			}else {	
			    if ($is_tdm_api == $FALSE) {
				$group=get_woomera_group();
				#if a context has already been assigned to this group, do not prompt for options
				foreach my $f_group (@woomera_groups) {
					if($f_group eq $group){
						$context="WOOMERA_NO_CONFIG";
					}
				}			
				if(!($context eq "WOOMERA_NO_CONFIG")){
					if ( $dev =~ /(\d+):NT/ ){	
						$context=get_woomera_context($group, $a50x->card->card_model(),$a50x->fe_line(),'bri_nt');
					} else {
					 	$context=get_woomera_context($group, $a50x->card->card_model(),$a50x->fe_line(),'bri_te');
					}
					push(@woomera_groups, $group);
				}
			    }
			}
			if ($os_type_list =~ m/FreeBSD/){
				$a50x->gen_wanpipe_conf(1);
			} else {
				$a50x->gen_wanpipe_conf(0);
			}

			if ( $dev =~ /(\d+):NT/ ){	
				$bri_conf.=$a50x->gen_bri_conf($bri_pos,"bri_nt", $group, $country, $operator, $conn_type, '');
			    if ($is_fs == $TRUE) {
					my $boostspan = eval { new boostspan();} or die ($@);
					my $openzapspan = $current_tdmapi_span-1;

					$boostspan->span_type('NT');
					$boostspan->span_no("$openzapspan");
					$boostspan->chan_no('1-2');
					$boostspan->trunk_type('bri');
				#	$boostspan->print();
					push(@boostbrispan,$boostspan);
				}
			} else {
				my $current_bri_default_tei='127';
				if ($def_bri_default_tei_opt==$TRUE){
					$current_bri_default_tei=$def_bri_default_tei;
				}
				printf("\nConfiguring span:%s as TEI:%s\n", $bri_pos, $current_bri_default_tei);
  				if($silent==$FALSE){
					my @options = ("YES - Keep this setting", "NO  - Specify a different TEI");
					my $res = &prompt_user_list(@options, "YES");
					if ($res =~ m/NO/) {
						$def_bri_default_tei_opt=$TRUE;
						$current_bri_default_tei=get_bri_default_tei();
						}
				}
				if ($def_bri_default_tei_opt==$FALSE){
					$bri_conf.=$a50x->gen_bri_conf($bri_pos,"bri_te", $group, $country, $operator, $conn_type, '');
				} else { 
					$bri_conf.=$a50x->gen_bri_conf($bri_pos,"bri_te", $group, $country, $operator, $conn_type, $current_bri_default_tei);
				}

			    if ($is_fs == $TRUE) {
					my $boostspan = eval { new boostspan();} or die ($@);
					my $openzapspan = $current_tdmapi_span-1;
					$boostspan->span_type('TE');
					$boostspan->span_no("$openzapspan");
					$boostspan->chan_no('1-2');
					$boostspan->trunk_type('bri');
				#	$boostspan->print();
					push(@boostbrispan,$boostspan);
				}

			}
			
			if ($is_fs == $FALSE && $is_smg == $FALSE) {
                        	if ( $dev =~ /(\d+):NT/ ){
                                	$a50x->signalling("NT");
	                        } else {
					if($conn_type eq "point_to_multipoint") {
                                		$a50x->signalling("TEM");
					} else {
	                                	$a50x->signalling("TE");
					}
                        	}
	                        $a50x->card->zap_context($context);
                        	$a50x->card->zap_group($group);
				$zaptel_conf.=$a50x->gen_zaptel_conf();
				$zapata_conf.=$a50x->gen_zapata_conf();
			}
			
			if(!($context eq "WOOMERA_NO_CONFIG") && $config_woomera==$TRUE){
				$woomera_conf.=$a50x->gen_woomera_conf($group, $context);
			}
		}
	}
	if($num_bri_devices_total!=0){
		print("\nISDN BRI card configuration complete\n\n");
	} else {
		print("\nNo Sangoma ISDN BRI cards detected\n\n");
	}
	if($silent==$FALSE){
		prompt_user("Press any key to continue");
	}
}

sub config_gsm {
	my $w40x;

	$first_cfg=0;
	print "------------------------------------\n";
	print "Configuring GSM cards [W400]\n";
	print "------------------------------------\n";
	my $skip_card=$FALSE;
	$zaptel_conf.="\n";
	$zapata_conf.="\n";
	
	foreach my $dev (@hwprobe) {

		if ($cfg_wanpipe_port && $cfg_wanpipe_port ne get_wanpipe_port($dev)) {
			next;	
		}

		if ($dev =~ /.*AFT-W400(.*):.*SLOT=(\d+).*BUS=(\d+).*PORT=(\d+).*/) {
			$skip_card=$FALSE;

			my $card = eval {new Card(); } or die ($@);

			$card->first_chan($current_zap_channel);
			$card->current_dir($current_dir);
			$card->cfg_dir($cfg_dir);
			$card->device_no($devnum);
			$card->device_no($cfg_wanpipe_port?$cfg_wanpipe_port:$devnum);
			$card->card_model("400");
			$card->pci_slot($2);
			$card->pci_bus($3);			

			if($dahdi_installed == $TRUE) {
				$card->dahdi_conf('YES');
				$card->dahdi_echo('NO')
			}

			$num_gsm_devices_total++;

			if ($silent == $FALSE) {
				system('clear');
				print "\n-----------------------------------------------------------\n";
				printf("AFT-%s detected on slot:%d bus:%d port:%d\n", get_card_name($card->card_model), $card->pci_slot, $card->pci_bus, $4);
				print "-----------------------------------------------------------\n";
			}

			if($silent==$FALSE){
					printf ("\nWould you like to configure AFT-%s port %s on slot:%d bus:%d\n",
									get_card_name($card->card_model), $4, $card->pci_slot, $card->pci_bus);
					my @options=("YES", "NO", "Exit");
					$def_gsm_option=&prompt_user_list(@options,$def_gsm_option);
			} else {
					$def_gsm_option="YES";
			}
			
			if($def_gsm_option eq 'YES') {
				$w40x = eval {new W40x(); } or die ($@);
				$w40x->card($card);

				$w40x->fe_line($4);

				if ($#silent_tdm_laws >= 0) {
					$silent_tdm_law = pop(@silent_tdm_laws);
					$w40x->tdm_law($silent_tdm_law);
				} else {
					$w40x->tdm_law(&prompt_tdm_law());
				}
			
				$startup_string.="wanpipe$devnum ";
				$cfg_string.="wanpipe$devnum ";
					
				if($silent==$FALSE){
					prompt_user("Press any key to continue");
				}
			
				if( $is_tdm_api == $FALSE) {
					printf ("AFT-%s configured on slot:%d bus:%d span:%s\n", get_card_name($card->card_model), $card->pci_slot, $card->pci_bus, $current_zap_span);

					$current_zap_channel+=2;
					$num_zaptel_config++;
					$card->tdmv_span_no($current_zap_span);
					$current_zap_span++;
					
					$zaptel_conf.=$w40x->gen_zaptel_conf($dchan_str);
					$w40x->card->zap_context(&get_zapata_context($w40x->card->card_model, $w40x->fe_line));
					$w40x->card->zap_group(&get_zapata_group($w40x->card->card_model, $w40x->fe_line, $w40x->card->zap_context));
					$zapata_conf.=$w40x->gen_zapata_conf();
				} else {
					printf ("AFT-%s configured on slot:%s bus:%s span:$current_tdmapi_span\n", get_card_name($card->card_model), $card->pci_slot, $card->pci_bus);
					$w40x->is_tdm_api($TRUE);
					$card->tdmv_span_no($current_tdmapi_span);
					$current_tdmapi_span++;
				}
				$w40x->gen_wanpipe_conf();
				$devnum++;
				$num_gsm_devices++;
			} else {
				printf ("AFT-%s on slot:%s bus:%s not configured\n", get_card_name($card->card_model), $card->pci_slot, $card->pci_bus);
				prompt_user("Press any key to continue");
			}
		} # if ($dev =~ /.*AFT-W400(.*):.*SLOT=(\d+).*BUS=(\d+).*PORT=(\d+).*HWEC=(\w+)
	}
	if($num_gsm_devices_total!=0){
		print("\nGSM cards configuration complete.\n");
	} else {
		print("\nNo Sangoma GSM cards detected\n\n");
	}
	if ($silent==$FALSE) {
		prompt_user("Press any key to continue");
	}
}

#------------------------------T1/E1 FUNCTIONS------------------------------------#

sub get_te_ref_clock{
	my @list_normal_clocks=@_;
	my @f_list_normal_clocks;
	my $f_port;
	foreach my $port (@list_normal_clocks) {
		if ($port eq '0'){
			$f_port = "Free run";
		} else {
			$f_port = "Port ".$port;
		}
		push(@f_list_normal_clocks, $f_port);
	}		

	my $res = &prompt_user_list(@f_list_normal_clocks, "Free run");
	my $i;
	
	for $i (0..$#f_list_normal_clocks){
		if ( $res eq @f_list_normal_clocks[$i] ){
			return @list_normal_clocks[$i];
		}
	}

	print "Internal error: Invalid reference clock\n";
	exit 1;

}


sub  prompt_user_hw_dchan{
	my ($card_model, $port, $port_femedia) = @_;
	my $res_dchan='';
	my $dchan;

	#$def_hw_dchan = 24;
	printf("Hardware HDLC can be performed on the data channel.\n");
prompt_hw_dchan:
	$res_dchan = &prompt_user("Select the data channel on A$card_model, port:$port, select 0 for unused.\n","$def_hw_dchan");
	while(length($res_dchan)==0 |!($res_dchan =~/(\d+)/)){
		print "Invalid channel, input an integer (0 for unused).\n";
		$res_dchan=&prompt_user("Select the data channel","$def_hw_dchan");
	}
	if ( $res_dchan < 0){
		printf("Error: channel cannot have negative value\n");
		$res_dchan='';
		goto prompt_hw_dchan;
	}
	if ( $port_femedia eq 'T1' && $res_dchan > 24){
		printf("Error: only 24 channels available on T1\n");
		$res_dchan='';
		goto prompt_hw_dchan;
	}elsif ($port_femedia eq 'E1' && $res_dchan > 31){
		printf("Error: only 31 channels available on E1\n");
		$res_dchan='';
		goto prompt_hw_dchan;
	}
	if ($res_dchan == 0){
		printf("HW HDLC channel not used\n");
	}else{
		printf("HW HDLC channel set to:%d\n", $res_dchan);
	}
	return $res_dchan;
}


sub get_zapata_group{
	my ($card_model,$card_port,$context)=@_;
	my $group='';
	if($silent==$TRUE){
		if($#silent_zapata_groups >= 0){
			$silent_zapata_group=pop(@silent_zapata_groups);
		}
		$silent_zapata_group =
		return $silent_zapata_group;
	}
	if($is_trixbox==$TRUE){
		if($context eq "from-zaptel"){
			$group=0;	
			return $group;
		}elsif($context eq "from-internal"){
			$group=1;
			return $group;
		}else{
			print "Internal error:invalid group for Trixbox\n";
		}	
	}else{
		if($context eq "from-pstn"){
			$group=0;
		}elsif($context eq "from-internal"){
			$group=1;
		}else{
			my $res_group=&prompt_user("\nInput the group for this port\n",$def_zapata_group);
			while(length($res_group)==0 |!($res_group =~/(\d+)/)){
				print "Invalid group, input an integer.\n";
				$res_group=&prompt_user("Input the group for this port",$def_zapata_group);
			}
			$group=$res_group;
			$def_zapata_group=$group;
		}      
	}

	return $group;
}



sub prompt_hw_dtmf {
#HW DTMF not supported in the 3.2 drivers
#	return "NO";
	if( $no_hwdtmf == $TRUE){
		return "NO";
	}

	if ($is_data_api == $TRUE) {
		return "NO";
	}

	print("Would you like to enable hardware DTMF detection?\n");
	my @options = ("YES","NO");
	$def_hw_dtmf = prompt_user_list(@options, $def_hw_dtmf);
	return $def_hw_dtmf;
}

sub prompt_hw_fax {
#HW DTMF not supported in the 3.2 drivers
#       return "NO";
        if( $no_hwdtmf == $TRUE){
                return "NO";
        }
		if ($is_data_api == $TRUE) {
			return "NO";
		}
        print("Would you like to enable hardware fax detection?\n");
        my @options = ("YES","NO");
        $def_hw_fax = prompt_user_list(@options, $def_hw_fax);
        return $def_hw_fax;
}

sub prompt_tdm_law {
	print("Which codec will be used?\n");
	my @options = ("MULAW - North America","ALAW - Europe");
	my @options_val = ("MULAW", "ALAW");
		
	if ($silent == $TRUE) {
		$def_tdm_law=$silent_tdm_law;
		return $def_tdm_law;
	}

	my $res = &prompt_user_list(@options, $def_tdm_law);
	my $i;
	for $i (0..$#options){
		if ( $res eq @options[$i] ){
			$def_tdm_law=@options[$i];
			return @options_val[$i]; 
		}
	}
	print "Internal error: Invalid TDM LAW type\n";
	exit 1;
}
sub prompt_tdm_opemode {
	print("Which Operation Mode will be used?\n");
	my @options = ("FCC","TBR21","AUSTRALIA");
	my $def_tdm_opermode = &prompt_user_list(@options, $def_tdm_opermode);
	return $def_tdm_opermode;
# 	
}

sub get_pri_switchtype {
	my @options = ( "National ISDN 2", "Nortel DMS100", "AT&T 4ESS", "Lucent 5ESS", "EuroISDN", "Old National ISDN 1", "Q.SIG" );
	my @options_val = ( "national", "dms100", "4ess", "5ess", "euroisdn", "ni1", "qsig" );
	my $res = &prompt_user_list(@options,$def_switchtype);
	my $i;
	for $i (0..$#options){
		if ( $res eq @options[$i] ){
			$def_switchtype=@options[$i];
			return @options_val[$i]; 
		}
	}	
	print "Internal error: Invalid PRI switchtype\n";
	exit 1;
}

sub get_boost_t1_switchtype {
	if ($def_switchtype eq "EuroISDN/ETSI"){
		$def_switchtype = "";
	}
	my @options = ( "National", "Nortel DMS100", "Lucent 5ESS", "Lucent 4ESS" );
	my @options_val = ( "national", "dms100", "5ess", "4ess" );

	my $res = &prompt_user_list(@options,$def_switchtype);
	my $i;
	for $i (0..$#options){
		if ( $res eq @options[$i] ){
			$def_switchtype=@options[$i];
			return @options_val[$i]; 
		}
	}	
	print "Internal error: Invalid PRI switchtype\n";
	exit 1;
}

sub get_boost_e1_switchtype {
	#$def_switchtype="EuroISDN/ETSI";
	if ($def_switchtype eq "National" || $def_switchtype eq "Nortel DMS100" || $def_switchtype eq "Lucent 5ESS" || $def_switchtype eq "Lucent 4ESS" ){
	#if ($def_switchtype ne "EuroISDN/ETSI" || $def_switchtype ne "euroisdn"){
		$def_switchtype = "";
	}
	my @options = ( "EuroISDN/ETSI", "QSIG" );
	my @options_val = ( "euroisdn", "qsig" );

        my $res = &prompt_user_list(@options,$def_switchtype);
        my $i;
        for $i (0..$#options){
                if ( $res eq @options[$i] ){
                        $def_switchtype=@options[$i];
                        return @options_val[$i];
                }
        }
        print "Internal error: Invalid PRI switchtype\n";
        exit 1;
}


sub gen_ss7_voicechans{
	my @ss7_array = @_;
	my $T1CHANS = pop(@ss7_array);
	my $count = @ss7_array;
	my $output ='';
	my $chan_in = 1;
	my $chan_de = 0;
	my $flag = 0;
	my $i = 0;
	my $j = 0;      

	while($i < $count){
		$j = $i + 1;
		if ($ss7_array[$i] > 2 && $i == 0){
			$chan_de = $ss7_array[$i] - 1;
			$output .= "1-$chan_de";
			$flag = 1;
		}elsif ($ss7_array[$i] == 2 && $i == 0){
			$output .= "1";
			$flag = 1;
		}	   

		if ($ss7_array[$j] == ($ss7_array[$i] + 1) && $j < $count){
			goto Incrementing;
		}elsif ($ss7_array[$i] == $T1CHANS && $i == ($count-1)){
			goto Incrementing;
		}

		if ($i < ($count-1)){
			$chan_in = $ss7_array[$i]+1;
			if ($chan_in < ($ss7_array[$j]-1)){
				$chan_de = $ss7_array[$j] - 1;
				if ($flag != 0){
					$output .= ".$chan_in-$chan_de";
				}else{
					$output .= "$chan_in-$chan_de";
					$flag = 1;
				}	
			}else{
				if ($flag != 0){
					$output .= ".$chan_in";
				}else{
					$output .= "$chan_in";
					$flag = 1;
				}
			}
		}else{
			$chan_in = $ss7_array[$i]+1;
			if ($chan_in < $T1CHANS){
				$chan_de = $T1CHANS;
				if ($flag != 0){
					$output .= ".$chan_in-$chan_de";
				}else{
					$output .= "$chan_in-$chan_de";
					$flag = 1;
				}
			}else{
				if ($flag != 0){
					$output .= ".$T1CHANS";
				}else{
					$output .= "$T1CHANS";
					$flag = 1;
				}
			}
		}
	
Incrementing:	
		$i++;
	}
	return $output;
}

sub prompt_user_ss7_chans{
	my @ss7_string = @_;
	my $def_ss7_group_chan = '';

	my $ss7_group_chan=&prompt_user("$ss7_string[0]",$def_ss7_group_chan);

CHECK1:	while (length($ss7_group_chan)==0 |!($ss7_group_chan =~ /^\d+$/)){
		print("ERROR: Invalid channel number, input an integer only.\n\n");
		$ss7_group_chan=&prompt_user("$ss7_string[0]",$def_ss7_group_chan);
}
	if ($line_media eq 'T1'){
		while ($ss7_group_chan>24 || $ss7_group_chan<1){
			print("Invalid channel number, there are only 24 channels in T1.\n\n");
			$ss7_group_chan=&prompt_user("$ss7_string[0]",$def_ss7_group_chan);
			goto CHECK1;
}
}elsif ($line_media eq 'E1'){
		while ($ss7_group_chan>31 || $ss7_group_chan<1){
			print("Invalid channel number, there are only 31 channels in E1.\n\n");
			$ss7_group_chan=&prompt_user("$ss7_string[0]",$def_ss7_group_chan);
			goto CHECK1;
}
}
	return $ss7_group_chan;
}






sub config_t1e1{
	if (!$first_cfg && $silent==$FALSE) {
		system('clear');
	}
	print "-------------------------------------------------------\n";
	print "Configuring T1/E1 cards [A101/A102/A104/A108/A116/T116]\n";
	print "-------------------------------------------------------\n";
	
	foreach my $dev (@hwprobe) {

		if ($dev =~ /AFT-T116-SH/) {
			$aft_family_type='T';
		}
		
		if ($cfg_wanpipe_port && $cfg_wanpipe_port ne get_wanpipe_port($dev)) {
			next;	
		}
		
		if (( $dev =~ /A(\d+)(.*):.*SLOT=(\d+).*BUS=(\d+).*CPU=(\w+).*PORT=(\w+).*/) || ($dev =~ /(\d+).(\w+\w+).*SLOT=(\d+).*BUS=(\d+).*(\w+).*PORT=(\d+).*HWEC=(\d+)/) ) {

		  	if ( ! ($1 eq '200' | $1 eq '400' | $1 eq '500' | $1 eq '600') |($1 eq '601' && $6 eq '1') ){
				#do not count analog devices
				$num_digital_devices_total++;
			} elsif ($is_data_api == $TRUE) {
				goto skip_card;
			}



			my $card = eval {new Card(); } or die ($@);
			$card->current_dir($current_dir);
			$card->cfg_dir($cfg_dir);
			$card->device_no($devnum);
			$card->device_no($cfg_wanpipe_port?$cfg_wanpipe_port:$devnum);
			$card->card_model($1);
			$card->card_model_name($dev);
			$card->pci_slot($3);
			$card->pci_bus($4);
			$card->fe_cpu($5);
			
			my $hwec=0;

			if ( $dev =~ /HWEC=(\d+)/){
				if($1 gt 0){
					$hwec=1;
				}
			}

			if($dahdi_installed == $TRUE) {
				$card->dahdi_conf('YES');
				$card->dahdi_echo($dahdi_echo);
				if ($hwec == 0) {
					$card->dahdi_echo('mg2');
				}
			}

			if ( ( $dev =~ /A(\d+)(.*):.*SLOT=(\d+).*BUS=(\d+).*CPU=(\w+).*PORT=(\w+).*/)|| ($dev =~ /(\d+).(\w+\w+).*SLOT=(\d+).*BUS=(\d+).*(\w+).*PORT=(\d+).*HWEC=(\d+)/) ) {
					my $abc=0;
			}
			if ($hwec==0){
				$card->hwec_mode('NO');
			}else{
				$card->hwec_mode('YES');
				
			}
			my $port=$6;
			if ($6 eq 'PRI') {
				$port=$5;
			} 
			if ( $card->card_model eq '101' |  
			     $card->card_model eq '102' |  
			     $card->card_model eq '104' |  
			     $card->card_model eq '108' |
			     $card->card_model eq '116' |
				 ($card->card_model eq '601' && $port eq '2') ){
				if (!$first_cfg && $silent==$FALSE) {
					system('clear');
				}
				if (($6 eq '1' || $6 eq 'PRI') && $5 eq 'A'){
					print "$aft_family_type$1 detected on slot:$3 bus:$4\n";
					$device_has_normal_clock=$FALSE;
					@device_normal_clocks = ("0");
				}
				$first_cfg=0;
				if($silent==$FALSE){
					my $msg ="Configuring port ".$port." on $aft_family_type".$card->card_model." slot:".$card->pci_slot." bus:".$card->pci_bus.".\n";
					print "\n-----------------------------------------------------------\n";
					print "$msg";
					print "-----------------------------------------------------------\n";
				}
				my $fe_media = '';
				if ($silent==$TRUE){
					if($#silent_femedias >= 0){
						$silent_femedia=pop(@silent_femedias);
					}
					
					$fe_media = $silent_femedia;
				} else {
					printf ("\nSelect media type for AFT-%s%s on port %s [slot:%s bus:%s span:$devnum]\n", $aft_family_type,$card->card_model, $port, $card->pci_slot, $card->pci_bus);
					my @options = ("T1", "E1", "Unused","Exit");
					$fe_media = prompt_user_list( @options, $def_femedia);
				}
	
				if ( $fe_media eq 'Exit'){
					print "Exit without applying changes?\n";
					if (&prompt_user_list(("YES","NO","YES")) eq 'YES'){
						print "No changes made to your configuration files.\n\n";
						exit 0;
					}
				}elsif ( $fe_media eq 'Unused'){
					$def_femedia=$fe_media;	
skip_card:
					my $msg= "A$1 on slot:$3 bus:$4 port:$port not configured\n";
					print "$msg";
					prompt_user("Press any key to continue");
				}else{
					if ($card->hwec_mode eq 'YES' && $device_has_hwec==$FALSE){
					$device_has_hwec=$TRUE;
				} 
				
				$def_femedia=$fe_media; 
				$cfg_string.="wanpipe$devnum ";
				my $a10x;
	
				if ($1 !~ m/104/ && $2 !~ m/SH/) {

					if($is_hp_tdm_api == $TRUE){
						#hp_tdm_api uses same templates:)
						$a10x = eval {new A10x(); } or die ($@);
						$a10x->old_a10u('YES');
		
					}else{
						if($is_fs == $TRUE) {
							#don't support old a102/a101 with FS and boost'
							printf("Old a101/102 card not supported in this release!!! with FreeSwitch\n\n");
							goto skip_card;
						}
						if($is_data_api == $TRUE) {
							#don't support old a102/a101 with FS and boost'
							printf("Old a101/102 card not supported in data api mode\n\n");
							goto skip_card;
						}
						$a10x = eval {new A10u(); } or die ($@);
						$a10x->old_a10u('YES');
						#$a10x->card($card);
					}
					$a10x->card($card);
					if ($5 eq "A") { 
						$a10x->fe_line("1");
					} else {
						$a10x->fe_line("2");
					}
				} else {
					$a10x = eval {new A10x(); } or die ($@);
					$a10x->old_a10u('NO');
					if (($dev =~ /A(\d+)(.*):.*SLOT=(\d+).*BUS=(\d+).*CPU=(\w+).*PORT=(\w+).*/) ||($dev =~ /(\d+).(\w+\w+).*SLOT=(\d+).*BUS=(\d+).*(\w+).*PORT=(\d+).*HWEC=(\d+)/)) {
						my $abc=0;
					}
					$a10x->card($card);
					$a10x->fe_line($6);
				}

				$card->first_chan($current_zap_channel);
				$a10x->fe_media($fe_media);
				if ( $fe_media eq 'T1' ){
					$max_chans = 24;
					$line_media = 'T1';
			
					if(!($def_felcode eq 'B8ZS' || $def_felcode eq 'AMI')){
						$def_felcode='B8ZS';
					}
					if(!($def_feframe eq 'ESF' || $def_feframe eq 'D4')){
						$def_feframe='ESF';
					}

		
					if ($silent==$FALSE){
						printf ("Configuring port %s on AFT-%s%s as: %s, coding:%s, framing:%s.\n", 
											$port,
											$aft_family_type,
											$card->card_model, 
											$fe_media,
											$def_felcode,
											$def_feframe,
											$port);
						my @options = ("YES - Keep these settings", "NO  - Configure line coding and framing");
						my $res = &prompt_user_list(@options, "YES");
						if ($res =~ m/NO/){
							printf("Select line coding for port %s on %s\n", $port, $card->card_model);
							my @options = ("B8ZS", "AMI");
							$def_felcode= &prompt_user_list(@options, $def_felcode);
								
							
							printf("Select framing for port %s on %s\n", $port, $card->card_model);
							@options = ("ESF", "D4");
							$def_feframe= &prompt_user_list(@options, $def_feframe);
						}


					} else {
						if($#silent_felcodes >= 0){
							$silent_felcode=pop(@silent_felcodes);
						}					
						$def_felcode=$silent_felcode;

						if(!($def_felcode eq 'B8ZS' || $def_felcode eq 'AMI')){
							$def_felcode='B8ZS';
						}
						
						if($#silent_feframes >= 0){
							$silent_feframe=pop(@silent_feframes);
						}			
						$def_feframe=$silent_feframe;

						if(!($def_feframe eq 'ESF' || $def_feframe eq 'D4')){
							$def_feframe='ESF';
						}
					}


				}else{ #fe_media = E1
					$max_chans = 31;
					$line_media = 'E1';
					if(!($def_felcode eq 'HDB3' || $def_felcode eq 'AMI')){
						$def_felcode='HDB3';
					}
					if(!($def_feframe eq 'CRC4' || $def_feframe eq 'NCRC4' || $def_feframe eq 'UNFRAMED')){
						$def_feframe='CRC4';
					}
					$a10x->rx_slevel('430');#set E1 rx_slevel to 43
					if ($silent==$FALSE){
						printf ("Configuring port %s on AFT-%s%s as %s, line coding:%s, framing:%s \n", 
											$port,
											$aft_family_type,
											$card->card_model, 
											$fe_media,
											$def_felcode,
											$def_feframe);
						my @options = ("YES - Keep these settings", "NO  - Configure line coding and framing");
						my $res = &prompt_user_list(@options, "YES");
						if ($res =~ m/NO/){
							printf("Select line coding for port %s on %s\n", $port, $card->card_model);
							my @options = ("HDB3", "AMI");
							$def_felcode= &prompt_user_list(@options, $def_felcode);
								
							
							printf("Select framing for port %s on %s\n", $port, $card->card_model);
							@options = ("CRC4", "NCRC4","UNFRAMED");
							$def_feframe = &prompt_user_list(@options, $def_feframe);
#							printf("Select signalling mode for port %s on %s\n", $port, $card->card_model);
#							my @options = ("CCS - Clear channel signalling ", "CAS");
#							$def_te_sig_mode = &prompt_user_list(@options, $def_te_sig_mode);
								
						}
					
					} else {
						if($#silent_felcodes >= 0){
							$silent_felcode=pop(@silent_felcodes);
						}					
						$def_felcode=$silent_felcode;
						if(!($def_felcode eq 'HDB3' || $def_felcode eq 'AMI')){
							$def_felcode='HDB3';
						}
						
						if($#silent_feframes >= 0){
							$silent_feframe=pop(@silent_feframes);
						}			
						$def_feframe=$silent_feframe;
						if(!($def_feframe eq 'CRC4' || $def_feframe eq 'NCRC4' || $def_feframe eq 'UNFRAMED')){
							$def_feframe='CRC4';
						}
					}
				}
				$a10x->fe_lcode($def_felcode);
				$a10x->fe_frame($def_feframe);
				if($silent==$FALSE){
					my @options = ("NORMAL", "MASTER");
					printf ("Select clock for AFT-%s on port %s [slot:%s bus:%s span:$devnum]\n", get_card_name($card->card_model), $port, $card->pci_slot, $card->pci_bus);

					$def_feclock=&prompt_user_list(@options, $def_feclock);
				} else {
					if($#silent_feclocks >= 0){
						$silent_feclock=pop(@silent_feclocks);
					}
					$def_feclock=$silent_feclock;
				}
				
				$a10x->fe_clock($def_feclock);
				if ( $def_feclock eq 'NORMAL') {
					$device_has_normal_clock=$TRUE;
					push(@device_normal_clocks, $a10x->fe_line);
				} elsif ( $def_feclock eq 'MASTER' && $device_has_normal_clock == $TRUE ){
					printf("Clock synchronization options for %s on port %s [slot:%s bus:%s span:$devnum] \n", 
							$card->card_model, 
							$port, 
							$card->pci_slot, 
							$card->pci_bus);
					printf(" Free run: Use internal oscillator on card [default] \n");
					printf(" Port N: Sync with clock from port N \n\n");
						
					printf("Select clock source %s on port %s [slot:%s bus:%s span:$devnum]\n", $card->card_model, $port, $card->pci_slot, $card->pci_bus);
					$def_te_ref_clock=&get_te_ref_clock(@device_normal_clocks);
					$a10x->te_ref_clock($def_te_ref_clock);
				}
			     
				if ($card->card_model eq '108') {
					my @options = ("DEFAULT", "LINEAR");
					printf ("Select A108 RJ45 HW Port Mapping Mode\n  DEFAULT [1,5] [2,6] [3,7] [4,8]\n  LINEAR  [1,2] [3,4] [5,6] [7,8] \n  Select LINEAR when connecting A108 to T3Mux\n\n");
					$def_hw_port_map=&prompt_user_list(@options, $def_hw_port_map);
					$a10x->hw_port_map_mode($def_hw_port_map);
				}

				if ($is_data_api == $TRUE || $is_tdm_api == $TRUE) {
					if ($is_fs == $FALSE) {
						my @options = ("NO", "YES");
						printf ("AFT Tapping Mode?\nSelect YES if AFT card will be used for passive tapping\n");
						$def_tapping_mode=&prompt_user_list(@options, $def_tapping_mode);
						$a10x->tapping_mode($def_tapping_mode);

						my @options = ("NO", "8bit", "7bit");
						if ($is_data_api == $TRUE) {
							printf ("HDLC Engine?\nSelect HDLC engine type, NO means transparent mode\n");
							$def_hdlc_mode=&prompt_user_list(@options, $def_hdlc_mode);
							printf ("Setting HDLC mode to %s\n",$def_hdlc_mode);
							$a10x->hw_dchan($def_hdlc_mode);
						}
					}
				}

				my @options="";	
				if ($is_smg==$TRUE && $zaptel_dahdi_installed==$TRUE){
					if($a10x->old_a10u eq 'NO'){
						@options = ("Zaptel/Dahdi - PRI CPE", "Zaptel/Dahdi - PRI NET", "Zaptel/Dahdi - E & M", "Zaptel/Dahdi - E & M Wink", "Zaptel/Dahdi - FXS - Loop Start", "Zaptel/Dahdi - FXS - Ground Start", "Zaptel/Dahdi - FXS - Kewl Start", "Zaptel/Dahdi - FX0 - Loop Start", "Zaptel/Dahdi - FX0 - Ground Start", "Zaptel/Dahdi - FX0 - Kewl Start", "SS7 - Sangoma Signal Media Gateway", "No Signaling (Voice Only)","Sangoma SMG/sangoma_prid - PRI CPE","Sangoma SMG/sangoma_prid - PRI NET");
					}else{
							@options = ("Zaptel/Dahdi - PRI CPE", "Zaptel/Dahdi - PRI NET", "Zaptel/Dahdi - E & M", "Zaptel/Dahdi - E & M Wink", "Zaptel/Dahdi - FXS - Loop Start", "Zaptel/Dahdi - FXS - Ground Start", "Zaptel/Dahdi - FXS - Kewl Start", "Zaptel/Dahdi - FX0 - Loop Start", "Zaptel/Dahdi - FX0 - Ground Start", "Zaptel/Dahdi - FX0 - Kewl Start", "SS7 - Sangoma Signal Media Gateway", "No Signaling (Voice Only)");
						}
				} elsif ($is_smg==$TRUE && $zaptel_dahdi_installed==$FALSE){
					@options = ("SS7 - Sangoma Signal Media Gateway", "No Signaling (Voice Only)");
				} elsif ($is_data_api==$TRUE){
					$def_signalling="DATA API";
					$a10x->is_data_api($TRUE);
					$a10x->is_tdm_api($FALSE);
				} elsif ($is_tdm_api==$TRUE && $is_fs == $FALSE){
					$def_signalling="TDM API";
					$a10x->is_tdm_api($TRUE);
				} elsif ($is_hp_tdm_api==$TRUE){
					$def_signalling="HPTDM API";
				} elsif ($is_fs == $TRUE){
						$def_signalling="pri_cpe";
						$a10x->is_tdm_api($TRUE);
			
				} else {
						if($a10x->old_a10u eq 'NO' && $is_trixbox==$FALSE){			
							@options = ("Zaptel/Dahdi - PRI CPE", "Zaptel/Dahdi - PRI NET", "Zaptel/Dahdi - E & M", "Zaptel/Dahdi - E & M Wink", "Zaptel/Dahdi - FXS - Loop Start", "Zaptel/Dahdi - FXS - Ground Start", "Zaptel/Dahdi - FXS - Kewl Start", "Zaptel/Dahdi - FX0 - Loop Start", "Zaptel/Dahdi - FX0 - Ground Start", "Zaptel/Dahdi - FX0 - Kewl Start","Sangoma SMG/sangoma_prid- PRI CPE","Sangoma SMG/sangoma_prid- PRI NET");
						} else {
							@options = ("Zaptel/Dahdi - PRI CPE", "Zaptel/Dahdi - PRI NET", "Zaptel/Dahdi - E & M", "Zaptel/Dahdi - E & M Wink", "Zaptel/Dahdi - FXS - Loop Start", "Zaptel/Dahdi - FXS - Ground Start", "Zaptel/Dahdi - FXS - Kewl Start", "Zaptel/Dahdi - FX0 - Loop Start", "Zaptel/Dahdi - FX0 - Ground Start", "Zaptel/Dahdi - FX0 - Kewl Start");
						}
				}
		
				if( $is_tdm_api == $FALSE && $is_hp_tdm_api == $FALSE ){
					printf ("Select signalling type for AFT-%s on port %s [slot:%s bus:%s span:$devnum]\n", get_card_name($card->card_model), $port, $card->pci_slot, $card->pci_bus);
					if($def_signalling =~/sangoma_prid/ && $a10x->old_a10u eq 'YES' ){
						$def_signalling='';
					}
					$def_signalling=&prompt_user_list(@options,$def_signalling); 
					
					if($silent == $TRUE) {
						if ($#silent_signallings >= 0){
							$def_signalling=pop(@silent_signallings);
						} else {
							$def_signalling=$silent_signalling;
						}
					}
				}

				$a10x->signalling($def_signalling);
				if ($fe_media eq 'E1'& $is_fs==$FALSE) {
					if ($def_signalling eq 'TDM API' ||$def_signalling eq 'HPTDM API' || $def_signalling eq 'DATA API'){	
						printf("Select signalling mode for port %s on %s\n", $port, $card->card_model);
						my @options=("CCS","CAS");
						$def_te_sig_mode=&prompt_user_list(@options, $def_te_sig_mode);
					} elsif($def_signalling  =~ m/PRI/ |
						$def_signalling eq 'SS7 - Sangoma Signal Media Gateway'|
						$def_signalling eq 'No Signaling (Voice Only)'){
	
						$def_te_sig_mode="CCS";
	
					} else {
						$def_te_sig_mode="CCS";
					}

					if ($silent_te_sig_mode ne '') {
						 $def_te_sig_mode=$silent_te_sig_mode;
					}

					if ($silent==$TRUE && $def_te_sig_mode eq 'CAS') {
						$def_signalling='Zaptel/Dahdi - E & M';
						$a10x->signalling($def_signalling);
						$a10x->hw_dchan('0');
					}
				
					$a10x->te_sig_mode($def_te_sig_mode);
				}
				my $ss7_chan;
				my $ss7_group_start;
				my $ss7_group_end;
				my @ss7_chan_array;
				my @ss7_sorted;
					
				if( $a10x->signalling eq 'SS7 - Sangoma Signal Media Gateway' ){
					$a10x->ss7_option(1); 
					my @options="";	
					print("Select an option below to configure SS7 signalling channels:\n");
					@options =("Configure SS7 XMPT2 Only", 
						      "Configure SS7 XMPT2 + Voice");
					$def_is_ss7_xmpt2_only = &prompt_user_list(@options, "$def_is_ss7_xmpt2_only");
					
					if($def_is_ss7_xmpt2_only=~ m/Only/){
						$is_ss7_xmpt2_only = $TRUE;
					}else{
						$is_ss7_xmpt2_only = $FALSE;
					}
					print("Choose an option below to configure SS7 signalling channels:\n");
					@options =("Configure individual signalling channels(e.g #1,#10)", 
						      "Configure consecutive signalling channels(e.g   #1-#16)");
					my $res = &prompt_user_list(@options, "");
					if ( $res eq 'Configure individual signalling channels(e.g #1,#10)'){
						goto SS7CHAN;
						while (1){
							print("\nAny other SS7 signalling channel to configure?\n");
							if (&prompt_user_list("YES","NO","") eq 'NO'){
								goto ENDSS7CONFIG;
							}else{
SS7CHAN:
								$ss7_chan = prompt_user_ss7_chans('Specify the channel for SS7 signalling(24 for T1? 16 for E1?)');
								push(@ss7_chan_array, $ss7_chan);
								$ss7_array_length = @ss7_chan_array;
								if ($ss7_array_length == $max_chans){
									goto ENDSS7CONFIG;
								}
							}
						}
					}else{
						goto SS7GROUP;
						while(1){
							print("\nAny other SS7 consecutive signalling channels to configure?\n");
							if (&prompt_user_list("YES","NO","") eq 'NO'){
								goto ENDSS7CONFIG;
							}else{
SS7GROUP:					
								$ss7_group_start = prompt_user_ss7_chans('Consecutive signalling channels START FROM channel number');
								$ss7_group_end = prompt_user_ss7_chans('Consecutive signalling channels END AT channel number');						        
								if ($ss7_group_start > $ss7_group_end){
									print("The starting channel number is bigger than the ending one!\n");
									goto SS7GROUP;
								}							
								my $i = 0;	
								for ($i = $ss7_group_start; $i <= $ss7_group_end; $i++){
									push(@ss7_chan_array, $i);
									my @remove_duplicate;
									@ss7_chan_array = grep(!$remove_duplicate[$_]++, @ss7_chan_array);
									$ss7_array_length = @ss7_chan_array;
								
									if ($ss7_array_length > $max_chans){	
										print("\nERROR : You defined more than $max_chans signalling channels in $line_media and please try to configure them again.\n\n");

										@ss7_chan_array = ();
										goto SS7GROUP;
									}
								}
								if ($ss7_array_length == $max_chans){
									goto ENDSS7CONFIG;	
								}	
							}
						}
					}

ENDSS7CONFIG:
					
					@ss7_sorted = sort { $a <=> $b } @ss7_chan_array;

					print("\nYou configured the following SS7 signalling channels: @ss7_sorted\n");
					my $ss7_voicechans = gen_ss7_voicechans(@ss7_sorted,$max_chans);
					$ss7_tdmvoicechans = $ss7_voicechans;
					 if ($is_ss7_xmpt2_only ==$FALSE){

						if ($ss7_voicechans =~ m/(\d+)/){
							$a10x->ss7_tdminterface($1);
						}
		
						$a10x->ss7_tdmchan($ss7_voicechans);
	
						$num_ss7_config++;
						$card->tdmv_span_no($current_tdmapi_span);
	
						#wanrouter start/stop for signalling span is controlled by ss7boxd
						#$startup_string.="wanpipe$devnum "; 
						$current_tdmapi_span++;
					}
				}elsif ( $a10x->signalling eq 'No Signaling (Voice Only)'){
					$a10x->ss7_option(2);
					$num_ss7_config++;   
					$card->tdmv_span_no($current_tdmapi_span);
					$startup_string.="wanpipe$devnum ";
					$current_tdmapi_span++;
				
				}elsif ($a10x->signalling eq 'TDM API' ||$a10x->signalling eq 'pri_cpe' ){
					if ($a10x->te_sig_mode eq 'CAS'){
						$a10x->hw_dchan('0');
					} else {
						if($is_fs == $FALSE && $silent== $FALSE) {
							$def_hw_dchan=prompt_user_hw_dchan($card->card_model, $port, $a10x->fe_media,$def_hw_dchan);
							$a10x->hw_dchan($def_hw_dchan);
						}elsif($is_fs == $TRUE){
							
							if($fe_media eq 'E1') {
								#fs boostpri support PRI only for now #Aug 14 2009
								$a10x->hw_dchan('16');
								$a10x->te_sig_mode('CCS');
							}else{
								$a10x->hw_dchan('24');
							}
						
						}elsif($silent == $TRUE && $is_fs == $FALSE){
						 	if($silent_hw_dchan eq ''){
								if($fe_media eq 'E1' && $a10x->te_sig_mode() == 'CCS'){
									$a10x->hw_dchan('16');				
								} else {
									$a10x->hw_dchan('24');
								}
							}else{
								$a10x->hw_dchan($silent_hw_dchan);
								}
						}
						my $dchan =$a10x->hw_dchan();
					}	
					$card->tdmv_span_no($current_tdmapi_span);
					$startup_string.="wanpipe$devnum ";
					$current_tdmapi_span++;
					
					if($is_fs == $TRUE || $is_ftdm == $TRUE || $is_trillium == $TRUE){
						#smg_pri.conf and openzap.conf
						my $boostspan = eval { new boostspan();} or die ($@);
						my $openzapspan = $current_tdmapi_span-1;
						my $span_type ="t1";
						my $chan_no="1-23";
						my $switchtype = 'national';
						my $chan_set='s'.$openzapspan.'c1-s'.$openzapspan.'c23';
						my $group_no='1';
						my $cardname='';
						my $context="";
						#$def_sigmode='pri_cpe';
						if($silent==$FALSE){
							
							printf ("Select Switchtype for AFT-%s on port %s [slot:%s bus:%s span:$devnum]\n", get_card_name($card->card_model), $port, $card->pci_slot, $card->pci_bus);
							if($a10x->fe_media eq 'T1'){
								$switchtype=get_boost_t1_switchtype();
							}		
							
							if($a10x->fe_media eq 'E1'){
								$span_type="e1";
								
								$chan_no="1-15";
								$chan_set='s'.$openzapspan.'c1-s'.$openzapspan.'c15,s'.$openzapspan.'c17-s'.$openzapspan.'c31';
								#YDBG $switchtype='euroisdn';
								$switchtype=get_boost_e1_switchtype();
							}
	
							printf ("Select signalling type for AFT-%s on port %s [slot:%s bus:%s span:$devnum]\n", get_card_name($card->card_model), $port, $card->pci_slot, $card->pci_bus);
							@options = ("PRI CPE", "PRI NET");
		
							$def_sigmode=&prompt_user_list(@options,$def_sigmode);
							
							$context=get_context(get_card_name($card->card_model),$port);
							$group_no=get_woomera_group();
							#	$boostspan->print(); 
						}
					$boostspan->span_type($span_type);
					$boostspan->span_no($openzapspan);
					$boostspan->chan_no($chan_no);
					$boostspan->group_no($group_no);
					$boostspan->trunk_type($span_type);
					$boostspan->switch_type($switchtype);
					$boostspan->chan_set($chan_set);
					$boostspan->context($context);
					$boostspan->sig_mode($def_sigmode);
					$cardname =get_card_name($card->card_model);
					$pri_conf .= "\n;AFT-$cardname on port $port";#slot:$card->pci_slot Bus:$card->pci_bus";
					
	
					$pri_conf.=$a10x->gen_pri_conf($boostspan->span_no(),$boostspan->span_type(), $boostspan->group_no(), $boostspan->switch_type(),$boostspan->trunk_type(),$boostspan->sig_mode());
#					print"$bri_conf\n";
#					prompt_user('Press any key to continue');
					$num_digital_smg++;
					push(@boostprispan,$boostspan);
		

					}

				}elsif ($a10x->signalling eq 'HPTDM API'){

					$a10x->hp_option(1);
						
					@options = ("160","80","40");
				
					if ($silent==$FALSE){
						printf ("Select Chunk Size in bytes, per timeslot for AFT-%s%s on port %s [slot:%s bus:%s]\n", $aft_family_type,get_card_name($card->card_model), $port, $card->pci_slot, $card->pci_bus);
						$def_chunk_size=&prompt_user_list(@options,$def_chunk_size); 
					}

					my @options="";	

					if ($a10x->te_sig_mode eq "CAS"){
						$a10x->hw_dchan('0');
					} else {
						@ss7_chan_array=&prompt_user_hw_dchan($card->card_model, $port, $a10x->fe_media);
						
					}
				
					@ss7_sorted = sort { $a <=> $b } @ss7_chan_array;

					my $ss7_voicechans = gen_ss7_voicechans(@ss7_sorted,$max_chans);
					$ss7_tdmvoicechans = $ss7_voicechans;
					

					if ($ss7_voicechans =~ m/(\d+)/){
						$a10x->ss7_tdminterface($1);
					}
	
					$a10x->ss7_tdmchan($ss7_voicechans);
						#wanrouter start/stop for signalling span is controlled by ss7boxd
					$startup_string.="wanpipe$devnum "; 
					
					if ($ss7_sorted[0]==0) {
						$def_mtu_mru=$def_chunk_size*$max_chans;
					}else{
						my $no_chans = $max_chans - 1;
						$def_mtu_mru=$def_chunk_size*($no_chans);
			
					}	
					$a10x->mtu_mru($def_mtu_mru);
				
				}elsif( $a10x->signalling =~ m/sangoma_prid/ ){# config TDM API and smp_pri.conf & woomera.confwanr
					my $group;
					my $context;
					my $span =$current_tdmapi_span;
					my $cardname =get_card_name($card->card_model);
					$a10x->is_tdm_api($TRUE);
					$a10x->smg_sig_mode($TRUE);
					
					if($a10x->fe_media eq 'T1'){
						$a10x->hw_dchan('24')
					}else{
						$a10x->hw_dchan('16')
					}
					if ($silent==$FALSE ){
						printf ("Select switchtype for AFT-%s on port %s \n", get_card_name($card->card_model), $port);
							if($a10x->fe_media eq 'T1'){
								$a10x->pri_switchtype(get_boost_t1_switchtype);
							}else{
								$a10x->pri_switchtype(get_boost_e1_switchtype);	
							}
						$group=get_woomera_group($group, $cardname,$a10x->fe_line(),$a10x->fe_media);
						$context=get_woomera_context($group, $cardname,$a10x->fe_line(),$a10x->fe_media);
					} else {
						if($#silent_pri_switchtypes >= 0){
							$silent_pri_switchtype=pop(@silent_pri_switchtypes);
						}			
						$def_feframe=$silent_feframe;
						$a10x->pri_switchtype($silent_pri_switchtype);
						$context="from-zaptel";
						$group=1;
					}

					$pri_conf .= "\n;AFT-$cardname on port $port";
					$pri_conf.=$a10x->gen_pri_conf($span,'e1', $group,$a10x->pri_switchtype(),'e1',$a10x->signalling());
					#woomera context & group
					push(@woomera_groups, $group);
					$woomera_conf.=$a10x->gen_woomera_conf($group, $context);
					$card->tdmv_span_no($current_tdmapi_span);
					$startup_string.="wanpipe$devnum ";
					$current_tdmapi_span++;
					$num_digital_smg++
				}else{
					if($a10x->smg_sig_mode eq "1") {
						$num_zaptel_config++;
						$card->tdmv_span_no($current_zap_span);
						$startup_string.="wanpipe$devnum ";
						$current_zap_span++;
						$current_zap_channel+=$max_chans;
					}
				}
		
				if ( $a10x->signalling =~ m/Dahdi - PRI/ ){    
					if ($silent==$FALSE && $config_zapata==$TRUE){
						printf ("Select switchtype for AFT-%s on port %s \n", get_card_name($card->card_model), $port);
						$a10x->pri_switchtype(get_pri_switchtype());	
					} else {
						if($#silent_pri_switchtypes >= 0){
							$silent_pri_switchtype=pop(@silent_pri_switchtypes);
						}			
						$def_feframe=$silent_feframe;
						$a10x->pri_switchtype($silent_pri_switchtype);
					}
				}
			
				# prompt the user if they would like to enable HW DTMF...HW_DTMF needs to be disabled for SMG
				if (!($a10x->signalling eq 'SS7 - Sangoma Signal Media Gateway'| $a10x->signalling eq 'No Signaling (Voice Only)')){
					if ($silent==$FALSE){
						printf("\n");
						if ($card->hwec_mode eq "YES"){
							$card->hw_dtmf(&prompt_hw_dtmf());
                                                        if ($card->hw_dtmf eq "YES"){
								$card->hw_fax(&prompt_hw_fax());
                                                        }
						} else {
							$card->hw_dtmf("NO");
                                                        $card->hw_fax("NO");

						}
					} else {
						if($#silent_hwdtmfs >= 0){
							$silent_hwdtmf=pop(@silent_hwdtmfs);
						}
						if ($card->hwec_mode eq "YES" && $no_hwdtmf==$FALSE){
							$card->hw_dtmf($silent_hwdtmf);
						} else {
							$card->hw_dtmf("NO");
						}
					}
				}else{
					$card->hw_dtmf("NO");
					$card->hw_fax("NO");
				} 

				#wanpipe gen section		
				if( $a10x->signalling eq 'SS7 - Sangoma Signal Media Gateway' ){
					$a10x->ss7_subinterface(1);
					$a10x->gen_wanpipe_ss7_subinterfaces();
					if ($ss7_tdmvoicechans ne ''){
						$a10x->ss7_subinterface(2);
						$a10x->gen_wanpipe_ss7_subinterfaces();
					}
					my $ss7_element;
					foreach $ss7_element (@ss7_sorted){
						$a10x->ss7_sigchan($ss7_element); 
						$a10x->ss7_subinterface(3);
						$a10x->gen_wanpipe_ss7_subinterfaces();
					}
									
					#$a10x->gen_wanpipe_conf();
					if ($os_type_list =~ m/FreeBSD/){
						$a10x->gen_wanpipe_conf(1);
					} else {
						$a10x->gen_wanpipe_conf(0);
					}
					if ($ss7_tdmvoicechans ne ''){
						$a10x->ss7_subinterface(5);
						$a10x->gen_wanpipe_ss7_subinterfaces();
					}
					foreach $ss7_element (@ss7_sorted){
						$a10x->ss7_sigchan($ss7_element); 
						$a10x->ss7_subinterface(6);
						$a10x->gen_wanpipe_ss7_subinterfaces();
					}
				}elsif ($a10x->signalling =~ m/HPTDM/){
										
					$a10x->ss7_subinterface(1);	
					$a10x->gen_wanpipe_ss7_subinterfaces();
					if ($ss7_tdmvoicechans ne ''){
						$a10x->ss7_subinterface(2);
						$a10x->gen_wanpipe_ss7_subinterfaces();
					}
					my $ss7_element;
				
					foreach $ss7_element (@ss7_sorted){
						if ($ss7_element != 0) {
							$a10x->ss7_sigchan($ss7_element); 
							$a10x->ss7_subinterface(3);
							$a10x->gen_wanpipe_ss7_subinterfaces();
						}
					}
							
					#$a10x->gen_wanpipe_conf();
					if ($os_type_list =~ m/FreeBSD/){
						$a10x->gen_wanpipe_conf(1);
					} else {
						$a10x->gen_wanpipe_conf(0);
					}
					if ($ss7_tdmvoicechans ne ''){
						$a10x->ss7_subinterface(5);
						$a10x->gen_wanpipe_ss7_subinterfaces();
					}
					printf("@ss7_sorted\n");
					prompt_user("Press any key to continue");

					if (@ss7_sorted != 0) {
						foreach $ss7_element (@ss7_sorted){
							if ($ss7_element != 0) {
								$a10x->ss7_sigchan($ss7_element); 
								$a10x->ss7_subinterface(6);
								$a10x->gen_wanpipe_ss7_subinterfaces();
							}
						}
					}
				}else{
					if ($os_type_list =~ m/FreeBSD/){
						$a10x->gen_wanpipe_conf(1);
					} else {
						$a10x->gen_wanpipe_conf(0);
					}
				}
			
				if ($is_smg==$TRUE && $config_zapata==$FALSE && $a10x->smg_sig_mode eq '1'){
					if (!($a10x->signalling eq 'SS7 - Sangoma Signal Media Gateway'
						| $a10x->signalling eq 'No Signaling (Voice Only)')){
						$zaptel_conf.=$a10x->gen_zaptel_conf($dchan_str);
					}
				}elsif ($is_smg==$TRUE && $config_zapata==$TRUE && $a10x->smg_sig_mode eq '1'){
					if (!($a10x->signalling eq 'SS7 - Sangoma Signal Media Gateway'| $a10x->signalling eq 'No Signaling (Voice Only)')){
					
						$zaptel_conf.=$a10x->gen_zaptel_conf($dchan_str);
						$a10x->card->zap_context(&get_zapata_context($a10x->card->card_model,$port));
						$a10x->card->zap_group(&get_zapata_group($a10x->card->card_model,$port,$a10x->card->zap_context));
						$zapata_conf.=$a10x->gen_zapata_conf();
					}
				}elsif (($is_trixbox==$TRUE | $config_zapata==$TRUE) && $a10x->smg_sig_mode eq '1'){
					if($silent==$FALSE){
						printf ("Configuring port %s on AFT-%s as a full %s\n", $a10x->fe_line(), get_card_name($a10x->card->card_model()),$a10x->fe_media());
						my $res=&prompt_user_list("YES - Use all channels", "NO  - Configure for fractional","YES");
						if ($res =~ m/NO/){
							my $max_chan=0;
							if($a10x->fe_media eq 'T1'){
								$max_chan=24;
							} else {
								$max_chan=31;
							}
							my $first_chan = &get_chan_no("first",1,$max_chan-1);
							my $last_chan = &get_chan_no("last",$first_chan,$max_chan);
							
							$a10x->frac_chan_first($first_chan);
							$a10x->frac_chan_last($last_chan);
						}
					} else {
						if($#silent_first_chans >= 0){
							$silent_first_chan = pop(@silent_first_chans);
							$silent_last_chan = pop(@silent_last_chans);
						}
						
						if($silent_first_chan != 0){
							$a10x->frac_chan_first($silent_first_chan);
							$a10x->frac_chan_last($silent_last_chan);
						}	
					}

					$zaptel_conf.=$a10x->gen_zaptel_conf($dchan_str);
					$a10x->card->zap_context(&get_zapata_context($a10x->card->card_model,$port));
					$a10x->card->zap_group(&get_zapata_group($a10x->card->card_model,$port,$a10x->card->zap_context));
					$zapata_conf.=$a10x->gen_zapata_conf();
				}elsif ($is_tdm_api == $FALSE && $is_hp_tdm_api == $FALSE && $a10x->smg_sig_mode eq '1' ){
					$zaptel_conf.=$a10x->gen_zaptel_conf($dchan_str);
				}

				# done with this port time to move on to the next port
				$devnum++;
				$num_digital_devices++;
				my $msg ="\nPort ".$port." on AFT-A".$card->card_model." configuration complete...\n";
				print "$msg";
				if($silent==$FALSE){
					prompt_user("Press any key to continue");
				}

				}				
			} 

		}
	}
	
	if($num_digital_devices_total!=0){
		print("\nT1/E1 card configuration complete.\n");
		if($silent==$FALSE){
			prompt_user("Press any key to continue");
		}
		$first_cfg=0;
	} else {
		print("\nNo Sangoma ISDN T1/E1 cards detected\n\n");
		if($silent==$FALSE){
			prompt_user("Press any key to continue");
		}
	}
}


#------------------------------ANALOG FUNCTIONS------------------------------------#


sub config_analog{

	my $a20x;

	if (!$first_cfg && $silent==$FALSE) {
		system('clear');
	}
	$first_cfg=0;
	print "------------------------------------\n";
	print "Configuring analog cards [A200/A400/B600/B610/B700/B800]\n";
	print "------------------------------------\n";

	my $skip_card=$FALSE;
	if($is_tdm_api == $FALSE && $is_hp_tdm_api == $FALSE && $is_fs == $FALSE) {
		$zaptel_conf.="\n";
		$zapata_conf.="\n";
	}
	foreach my $dev (@hwprobe) {
		
		if ($cfg_wanpipe_port && $cfg_wanpipe_port ne get_wanpipe_port($dev)) {
			next;	
		}
				
		if ( ($dev =~ /(\d+).(\w+\w+).*SLOT=(\d+).*BUS=(\d+).*CPU=(\w+).*PORT=(\w+).*HWEC=(\d+)/) ||
			 ($dev =~ /(\d+).(\w+\w+).*SLOT=(\d+).*BUS=(\d+).*(\w+).*PORT=(\d+).*HWEC=(\d+)/)){

			$skip_card=$FALSE;
			my $card = eval {new Card(); } or die ($@);
			$card->current_dir($current_dir);
			$card->cfg_dir($cfg_dir);
			$card->device_no($devnum);
			$card->device_no($cfg_wanpipe_port?$cfg_wanpipe_port:$devnum);
			$card->card_model($1);
			$card->card_model_name($dev);
			$card->pci_slot($3);
			$card->pci_bus($4);
			$card->fe_cpu($5);
			my $hwec=$7;

			if ($hwec==0){
				$card->hwec_mode('NO');
			} else{
				$card->hwec_mode('YES');
			}

			if($dahdi_installed == $TRUE) {
				$card->dahdi_conf('YES');
				$card->dahdi_echo($dahdi_echo);
				if ($hwec == 0) {
					$card->dahdi_echo('mg2');
				}
			}

			if ($card->card_model eq '200' || $card->card_model eq '400' || $card->card_model eq '600' || $card->card_model eq '610' || $card->card_model eq '800' || ($card->card_model eq '700' && $6 == '5') || ($card->card_model eq '601' && $6 == '1') ){
				$num_analog_devices_total++;
				if($silent==$FALSE) {
					system('clear');
					print "\n-----------------------------------------------------------\n";
					print "A$1 detected on slot:$3 bus:$4\n";
					print "-----------------------------------------------------------\n";
				}
				if($is_trixbox==$FALSE){
					if ($silent==$FALSE){
						printf ("\nWould you like to configure AFT-%s on slot:%d bus:%s\n",
											get_card_name($card->card_model), $3, $4);
						if (&prompt_user_list(("YES","NO","")) eq 'NO'){
							$skip_card=$TRUE;	
						}
					}
				}
				if ($skip_card==$FALSE){
					
					$a20x = eval {new A20x(); } or die ($@);
					$a20x->card($card);
					$card->first_chan($current_zap_channel);
			
					if ( $device_has_hwec==$TRUE && $silent==$FALSE){
						print "Will this AFT-A$1 synchronize with an existing Sangoma T1/E1 card?\n";
						print "\n WARNING: for hardware and firmware requirements, check:\n";
						print "          http://wiki.sangoma.com/t1e1analogfaxing\n";
						
						if (&prompt_user_list(("NO","YES","")) eq 'NO'){
							$a20x->rm_network_sync('NO');
						} else {
							$a20x->rm_network_sync('YES');
						}
					} 
				
					if ($silent==$FALSE){
						if ($card->hwec_mode eq "YES"){
							$card->hw_dtmf(&prompt_hw_dtmf());
                                if ($card->hw_dtmf eq "YES"){
									$card->hw_fax(&prompt_hw_fax());
                               }
						} else {
							$card->hw_dtmf("NO");
							$card->hw_fax("NO");
						}
						if ($card->card_model eq '700'){
							$a20x->tdm_law("ALAW");
						} else {
							$a20x->tdm_law(&prompt_tdm_law());
						}
						$a20x->tdm_opermode(&prompt_tdm_opemode());
		
					} else {
						if($#silent_hwdtmfs >= 0){
							$silent_hwdtmf=pop(@silent_hwdtmfs);
						}
				
						if ($card->hwec_mode eq "YES" && $no_hwdtmf==$FALSE){
							$card->hw_dtmf($silent_hwdtmf);
						} else {
							$card->hw_dtmf("NO");
						}
						
						if($#silent_tdm_laws >= 0){
							$silent_tdm_law=pop(@silent_tdm_laws);
						}						
						
						$a20x->tdm_law($silent_tdm_law);
						
					} 
			
					$startup_string.="wanpipe$devnum ";
					$cfg_string.="wanpipe$devnum ";
					
					if($silent==$FALSE){
						prompt_user("Press any key to continue");
					}
					my $i;
					if( $is_tdm_api == $FALSE) {
						printf ("AFT-%s configured on slot:%d bus:%d span:%s\n", get_card_name($1), $3, $4,$current_zap_span);
						$zaptel_conf.="#Sangoma AFT-".get_card_name($1)." [slot:$3 bus:$4 span:$current_zap_span] <wanpipe".$a20x->card->device_no.">\n";
						$zapata_conf.=";Sangoma AFT-".get_card_name($1)." [slot:$3 bus:$4 span:$current_zap_span]  <wanpipe".$a20x->card->device_no.">\n";
						$current_zap_channel+=24;	
						$num_zaptel_config++;
						$card->tdmv_span_no($current_zap_span);
						$current_zap_span++;
					}else{
						printf ("AFT-%s configured on slot:%s bus:%s span:$current_tdmapi_span\n", 							get_card_name($1), $3, $4);
						$a20x->is_tdm_api($TRUE);
						$card->tdmv_span_no($current_tdmapi_span);
						$current_tdmapi_span++;
					}
						$devnum++;
						$num_analog_devices++;
					if ($os_type_list =~ m/FreeBSD/){
						$a20x->gen_wanpipe_conf(1);
					} else {
						$a20x->gen_wanpipe_conf(0);
					}
					
				}else{
					printf ("AFT-%s on slot:%s bus:%s not configured\n", 
								get_card_name($1), $3, $4);
					prompt_user("Press any key to continue");
				}
			}
	
		}elsif ( $dev =~ /(\d+):FXS/ & $skip_card==$FALSE){
			if( $is_tdm_api==$FALSE) {
				my $zap_pos = $1+$current_zap_channel-25;
				if($silent==$TRUE & $silent_zapata_context_opt == $TRUE){
					if($#silent_zapata_contexts >= 0){
						$silent_zapata_context=pop(@silent_zapata_contexts);
					}
					$a20x->card->zap_context($silent_zapata_context);
				} else {
					$a20x->card->zap_context("from-internal");
				}
				$a20x->card->zap_group("1");
				$zaptel_conf.=$a20x->gen_zaptel_conf($zap_pos,"fxo");
				$zapata_conf.=$a20x->gen_zapata_conf($zap_pos,"fxo");
			}elsif ($is_fs==$TRUE){#do fs conf
					$openzap_pos=$1+10-10;#to remove leading zero
					my $analogspan = eval { new analogspan();} or die ($@);
					my $openzapspan = $current_tdmapi_span-1;
					$analogspan->span_type('FXS');
					$analogspan->span_no("$openzapspan");
					$analogspan->chan_no("$openzap_pos");
					push(@fxsspan,$analogspan);

			}
		}elsif ( $dev =~ /(\d+):FXO/ & $skip_card==$FALSE ){
			if( $is_tdm_api==$FALSE) {
				my $zap_pos = $1+$current_zap_channel-25;
				if($silent==$TRUE & $silent_zapata_context_opt == $TRUE){
					if($#silent_zapata_contexts >= 0){
						$silent_zapata_context=pop(@silent_zapata_contexts);
					}
					$a20x->card->zap_context($silent_zapata_context);
				} else {
					$a20x->card->zap_context("from-zaptel");
				}
				$a20x->card->zap_group("0");
				$zaptel_conf.=$a20x->gen_zaptel_conf($zap_pos,"fxs");
				$zapata_conf.=$a20x->gen_zapata_conf($zap_pos,"fxs");
			}elsif ($is_fs==$TRUE){#do fs conf
					$openzap_pos=$1+10-10;#to remove leading zero
					my $openzapspan = $current_tdmapi_span-1;
					my $analogspan = eval { new analogspan();} or die ($@);
					$analogspan->span_type('FXO');
					$analogspan->span_no("$openzapspan");
					$analogspan->chan_no("$openzap_pos");
					push(@fxospan,$analogspan);

			}
		}
	}
	if($num_analog_devices_total!=0){
		print("\nAnalog card configuration complete\n\n");
		if( $silent==$FALSE){
			prompt_user("Press any key to continue");
		}
	}


}
sub config_tdmv_dummy 
{
	my $command='';
	if( $num_digital_devices == 0 && $num_analog_devices == 0 &&  $num_usb_devices == 0 && $num_bri_devices !=0 && $zaptel_dahdi_installed==$TRUE && $os_type_list =~ m/Linux/  && $silent == $FALSE && $is_fs == $FALSE && $is_smg == $TRUE){
		system('clear');
		print("Would you like to configure BRI board as timing source for $zaptel_string?\n");
		print("(Visit http://wiki.sangoma.com/wanpipe-linux-asterisk-appendix#bri-tdmv for more information)\n");		
		
		if(&prompt_user_list("YES", "NO" ,"") =~ m/YES/){
#			 
		$command=("sed -i -e 's/RM_BRI_CLOCK_MASTER.*/\RM_BRI_CLOCK_MASTER  = YES/' $current_dir/$cfg_dir/wanpipe1.conf");
		
			if ( system($command) == 0){
				printf("TDMV $zaptel_string Timer Enabled\n");
				prompt_user("Press any key to continue");
				
			}else{
				printf("Failed to Enable TDMV $zaptel_string Timer for BRI board\n");
				printf("Please contact Sangoma Tech Support\n");
				exit 1;
			}	
		} 
	}
}

sub config_usbfxo{

	my $u10x;
	if (!$first_cfg && $silent==$FALSE) {
		system('clear');
	}
	$first_cfg=0;
	print "------------------------------------\n";
	print "Configuring USB devices [U100]\n";
	print "------------------------------------\n";

	my $skip_card=$FALSE;
	if($is_tdm_api == $FALSE) {
		$zaptel_conf.="\n";
		$zapata_conf.="\n";
	}
	foreach my $dev (@hwprobe) {
			if (( $dev =~ m/ *.(\D\d+).*BUSID=(\d-\d).*HWEC=(\w+).*/)||
				( $dev =~ m/ *.(\D\d+).*BUSID=(\d-\d.\d).*HWEC=(\w+).*/)||
				( $dev =~ m/ *.(\D\d+).*BUSID=(\d-\d.\d).*/)||
				( $dev =~ m/ *.(\D\d+).*BUSID=(\d-\d).*/)){		
				$skip_card=$FALSE;
				my $card = eval {new Card(); } or die ($@);
				$card->current_dir($current_dir);
				$card->cfg_dir($cfg_dir);
				$card->device_no($devnum);
				$card->device_no($cfg_wanpipe_port?$cfg_wanpipe_port:$devnum);
				$card->card_model($1);
				$card->card_model_name($dev);
				$card->pci_bus($2);
				my $hwec=$3;

				if ($hwec==0){
					$card->hwec_mode('NO');
				}else{
					$card->hwec_mode('YES');
				}
				if($dahdi_installed == $TRUE) {
					$card->dahdi_conf('YES');
					$card->dahdi_echo($dahdi_echo);
					if ($hwec == 0) {
						$card->dahdi_echo('mg2');
					}
				}
				if ($card->card_model eq 'U100'){
					$num_usb_devices_total++;
					
					if($silent==$FALSE) {
						system('clear');
						print "\n-----------------------------------------------------------\n";
						print "USB $1 detected on  bus:$2\n";
						print "-----------------------------------------------------------\n";
					}
					if($is_trixbox==$FALSE){
						if ($silent==$FALSE){
							print "\nWould you like to configure USB $1 on bus:$2\n";
							if (&prompt_user_list(("YES","NO","")) eq 'NO'){
								$skip_card=$TRUE;	
							}
						}
					}
					if ($skip_card==$FALSE){
						
						$u10x = eval {new U10x(); } or die ($@);
						$u10x->card($card);
						$card->first_chan($current_zap_channel);
				
										
						if ($silent==$FALSE){
							$u10x->tdm_law(&prompt_tdm_law());
							$u10x->tdm_opermode(&prompt_tdm_opemode());
						} else {
													
							if($#silent_tdm_laws >= 0){
								$silent_tdm_law=pop(@silent_tdm_laws);
							}						
							
							$u10x->tdm_law($silent_tdm_law);
							
						} 
				
						$startup_string.="wanpipe$devnum ";
						$cfg_string.="wanpipe$devnum ";
						
						if($silent==$FALSE){
							prompt_user("Press any key to continue");
						}
						my $i;
						if( $is_tdm_api == $FALSE) {
							print "$1 configured on  bus:$2 span:$current_zap_span\n";
							$zaptel_conf.="#Sangoma USB $1  [bus:$2 span:$current_zap_span] <wanpipe".$u10x->card->device_no.">\n";
							$zapata_conf.=";Sangoma A$1 [slot:$3 bus:$4 span:$current_zap_span]  <wanpipe".$u10x->card->device_no.">\n";
							$current_zap_channel+=2;	
							$num_zaptel_config++;
							$card->tdmv_span_no($current_zap_span);
							$current_zap_span++;
						}else{
							print "A$1 configured on slot:$3 bus:$4 span:$current_tdmapi_span\n";
							$u10x->is_tdm_api($TRUE);
							$card->tdmv_span_no($current_tdmapi_span);
							$current_tdmapi_span++;
						}
							$devnum++;
							$num_usb_devices++;
						if ($os_type_list =~ m/FreeBSD/){
							$u10x->gen_wanpipe_conf(1);
						} else {
							$u10x->gen_wanpipe_conf(0);
						}
						for (my $mod = 0; $mod <= 1; $mod++){
							my $zap_pos = $mod+$current_zap_channel-2;
							if($silent==$TRUE & $silent_zapata_context_opt == $TRUE){
								if($#silent_zapata_contexts >= 0){
									$silent_zapata_context=pop(@silent_zapata_contexts);
								}
								$u10x->card->zap_context($silent_zapata_context);
							} else {
								$u10x->card->zap_context("from-zaptel");
							}
							$u10x->card->zap_group("0");
							$zaptel_conf.=$u10x->gen_zaptel_conf($zap_pos,"fxs");
							$zapata_conf.=$u10x->gen_zapata_conf($zap_pos,"fxs");
						}
						
					}else{
						print "$1 on  bus:$2 not configured\n";
						prompt_user("Press any key to continue");
					}
				}
			}
	}
	if($num_usb_devices_total!=0){
		print("\nUSB analog card configuration complete\n\n");
		if( $silent==$FALSE){
			prompt_user("Press any key to continue");
		}
	}

}

sub write_openzap_conf{

	if($is_fs==$FALSE){
		return;
	}
	my $openzap='';
	$openzap.="\n";
	
	if(@boostprispan){
		$openzap.="[span wanpipe smg_prid]\n";
		$openzap.="name => smg_prid\n";
		foreach my $span (@boostprispan){
			my $boostprispan=$span;
			$openzap.="trunk_type =>";
		    $openzap.=$boostprispan->span_type()."\n";
			$openzap.="b-channel => ";
			$openzap.=$boostprispan->span_no();
			$openzap.=":";
			$openzap.=$boostprispan->chan_no();
			if($boostprispan->span_type() eq 'e1')
			{
				$openzap.="\n";
				$openzap.="b-channel => ";
				$openzap.=$boostprispan->span_no();
				$openzap.=":";
				$openzap.="17-31";
			}
			$openzap.="\n";
		}
		
	}
	$openzap.="\n\n";

	if(@boostbrispan){
		$openzap.="[span wanpipe smg_brid]\n";
		$openzap.="name => smg_brid\n";
		#$openzap.="trunk_type => bri\n";
		foreach my $span (@boostbrispan){
			my $boostbrispan=$span;
			$openzap.="b-channel => ";
			$openzap.=$boostbrispan->span_no();
			$openzap.=":";
			$openzap.=$boostbrispan->chan_no();
			$openzap.="\n";
		}
		
	}
	$openzap.="\n\n";

	if(@fxsspan){
		$openzap.="[span wanpipe FXS]\n";
		$openzap.="name => OpenZAP\n";
		foreach my $span (@fxsspan){
			my $fxsspan=$span;
			$openzap.="trunk_type => fxs\n";
			$openzap.="fxs-channel => ";
			$openzap.=$fxsspan->span_no();
			$openzap.=":";
			$openzap.=$fxsspan->chan_no();
			$openzap.="\n";
		}
	$openzap.="\n\n";
	}
	if(@fxospan){
		$openzap.="[span wanpipe FXO]\n";
		$openzap.="name => OpenZAP\n";
		foreach my $span (@fxospan){
			my $fxospan=$span;
			$openzap.="trunk_type => fxo\n";
			$openzap.="fxo-channel => ";
			$openzap.=$fxospan->span_no();
			$openzap.=":";
			$openzap.=$fxospan->chan_no();
			$openzap.="\n";
		}
		
	}
	$openzap.="\n\n";
		
	#need to fix properly
	my $openzap_file="";
	open(FH, "$openzap_conf_template") or die "cannot open $openzap_conf_template";
	while (<FH>) {
		$openzap_file .= $_;
	}
	close (FH);
	$openzap_file=$openzap_file.$openzap;	
	open(FH, ">$openzap_conf_file") or die "cannot open $openzap_conf_file";
		print FH $openzap_file;
	close(FH);	
	return ;
}

sub write_freetdm_conf_legacy{

	if($is_fs==$FALSE){
		return;
	}
	my $freetdm='';
	$freetdm.="\n";

	$freetdm.="[span wanpipe boostpri]\n";
	if(@boostprispan){
		foreach my $span (@boostprispan){
			my $boostprispan=$span;
			#$freetdm.="[span wanpipe boostpri]\n";
			if($boostprispan->span_type() eq 't1')
			{
				$freetdm.="trunk_type => t1\n";
			}
			if($boostprispan->span_type() eq 'e1')
			{
				$freetdm.="trunk_type => e1\n";
			}
			$freetdm.="b-channel => ";
			$freetdm.=$boostprispan->span_no();
			$freetdm.=":";
			$freetdm.=$boostprispan->chan_no();
			if($boostprispan->span_type() eq 'e1')
			{
				$freetdm.="\n";
				$freetdm.="b-channel => ";
				$freetdm.=$boostprispan->span_no();
				$freetdm.=":";
				$freetdm.="17-31";
			}
			$freetdm.="\n\n";
		}

	}
	$freetdm.="\n\n";

	if(@boostbrispan){
		$freetdm.="[span wanpipe smg_brid]\n";
		$freetdm.="name => smg_brid\n";
		#$freetdm.="trunk_type => bri\n";
		foreach my $span (@boostbrispan){
			my $boostbrispan=$span;
			$freetdm.="b-channel => ";
			$freetdm.=$boostbrispan->span_no();
			$freetdm.=":";
			$freetdm.=$boostbrispan->chan_no();
			$freetdm.="\n";
		}

	}
	$freetdm.="\n\n";

	if(@fxsspan){
		$freetdm.="[span wanpipe FXS]\n";
		$freetdm.="name => freetdm\n";
		foreach my $span (@fxsspan){
			my $fxsspan=$span;
			$freetdm.="trunk_type => fxs\n";
			$freetdm.="group => grp1\n";
			$freetdm.="fxs-channel => ";
			$freetdm.=$fxsspan->span_no();
			$freetdm.=":";
			$freetdm.=$fxsspan->chan_no();
			$freetdm.="\n\n";
		}
	$freetdm.="\n\n";
	}
	if(@fxospan){
		$freetdm.="[span wanpipe FXO]\n";
		$freetdm.="name => freetdm\n";
		foreach my $span (@fxospan){
			my $fxospan=$span;
			$freetdm.="trunk_type => fxo\n";
			$freetdm.="group => grp2\n";
			$freetdm.="fxo-channel => ";
			$freetdm.=$fxospan->span_no();
			$freetdm.=":";
			$freetdm.=$fxospan->chan_no();
			$freetdm.="\n\n";
		}

	}
	$freetdm.="\n\n";

        #need to fix properly
	my $freetdm_file="";
	open(FH, "$freetdm_conf_template") or die "cannot open $freetdm_conf_template";
	while (<FH>) {
		$freetdm_file .= $_;
	}
	close (FH);
	$freetdm_file=$freetdm_file.$freetdm;
	open(FH, ">$freetdm_conf_file") or die "cannot open $freetdm_conf_file";
		print FH $freetdm_file;
 	close(FH);
	return ;
}



sub write_freetdm_conf{

	if($is_fs==$FALSE){
		return;
	}
	my $freetdm='';
	$freetdm.="\n";
	
	#$freetdm.="[span wanpipe boostpri]\n";
	if(@boostprispan){
		foreach my $span (@boostprispan){
			my $boostprispan=$span;
			#$freetdm.="[span wanpipe boostpri]\n";
			$freetdm.="[span wanpipe wp";
			$freetdm.=$boostprispan->span_no();
			$freetdm.="]\n";
			if($boostprispan->span_type() eq 't1')
			{
				$freetdm.="trunk_type => t1\n";
			}
			if($boostprispan->span_type() eq 'e1')
			{
				$freetdm.="trunk_type => e1\n";
			}
			$freetdm.="group=";
			$freetdm.=$boostprispan->group_no();
			$freetdm.="\n";
			$freetdm.="b-channel => ";
			$freetdm.=$boostprispan->span_no();
			$freetdm.=":";
			$freetdm.=$boostprispan->chan_no();
			if($boostprispan->span_type() eq 'e1')
			{
				$freetdm.="\n";
				$freetdm.="b-channel => ";
				$freetdm.=$boostprispan->span_no();
				$freetdm.=":";
				$freetdm.="17-31";
			}
			$freetdm.="\n";
			if($boostprispan->span_type() eq 't1')
			{
				$freetdm.="d-channel => ";
				$freetdm.=$boostprispan->span_no();
				$freetdm.=":";
				$freetdm.="24";
			}
			if($boostprispan->span_type() eq 'e1')
			{
				$freetdm.="d-channel => ";
				$freetdm.=$boostprispan->span_no();
				$freetdm.=":";
				$freetdm.="16";
                        }
			$freetdm.="\n\n";
		}
		
	}
	$freetdm.="\n\n";

	if(@boostbrispan){
		foreach my $span (@boostbrispan){
			my $boostbrispan=$span;
			$freetdm.="[span wanpipe wp";
			$freetdm.=$boostbrispan->span_no();
			$freetdm.="]\n";
			if($boostbrispan->trunk_type() eq 'point_to_multipoint')
			{
				$freetdm.="trunk_type => bri_ptmp\n";
			}
			if($boostbrispan->trunk_type() eq 'point_to_point')
			{
				$freetdm.="trunk_type => bri\n";
			}
			#$freetdm.='YDBG boostbrispan=trunk_type =>'.$boostbrispan->trunk_type();

			$freetdm.="group=";
			$freetdm.=$boostbrispan->span_no();
			$freetdm.="\n";
			$freetdm.="b-channel => ";
			$freetdm.=$boostbrispan->span_no();
			$freetdm.=":";
			$freetdm.=$boostbrispan->chan_no();
			$freetdm.="\n";
			$freetdm.="d-channel => ";
			$freetdm.=$boostbrispan->span_no();
			$freetdm.=":";
			$freetdm.="3";
			$freetdm.="\n\n";
		}
	}

	if(@fxsspan){
		$freetdm.="[span wanpipe FXS]\n";
		$freetdm.="name => freetdm\n";
		foreach my $span (@fxsspan){
			my $fxsspan=$span;
			$freetdm.="trunk_type => fxs\n";
			$freetdm.="group => grp1\n";
			$freetdm.="fxs-channel => ";
			$freetdm.=$fxsspan->span_no();
			$freetdm.=":";
			$freetdm.=$fxsspan->chan_no();
			$freetdm.="\n\n";
		}
	$freetdm.="\n\n";
	}
	if(@fxospan){
		$freetdm.="[span wanpipe FXO]\n";
		$freetdm.="name => freetdm\n";
		foreach my $span (@fxospan){
			my $fxospan=$span;
			$freetdm.="trunk_type => fxo\n";
			$freetdm.="group => grp2\n";
			$freetdm.="fxo-channel => ";
			$freetdm.=$fxospan->span_no();
			$freetdm.=":";
			$freetdm.=$fxospan->chan_no();
			$freetdm.="\n\n";
		}
		
	}
	$freetdm.="\n\n";
		
	#need to fix properly
	my $freetdm_file="";
	open(FH, "$freetdm_conf_template") or die "cannot open $freetdm_conf_template";
	while (<FH>) {
		$freetdm_file .= $_;
	}
	close (FH);
	$freetdm_file=$freetdm_file.$freetdm;	
	open(FH, ">$freetdm_conf_file") or die "cannot open $freetdm_conf_file";
		print FH $freetdm_file;
	close(FH);	
	return ;
}

sub config_smg_ctrl_boot {
	#smg_ctrl must start after network
	my $network_start_level=10;
	my $smg_ctrl_start_level=11;
	my $res='YES';
	my $script_name='smg_ctrl';
	my $command='';

	my @boot_scripts= ('smg_ctrl','smg_ctrl_safe');
		foreach my $boot_scripts (@boot_scripts) {
	
		print"Removing old $boot_scripts boot";
		if ($have_update_rc_d) {#debian/ubuntu
			$command = "update-rc.d -f $boot_scripts remove >>/dev/null 2>>/dev/null";
			if (system($command) == 0){
				print".....OK\n";
			}else{
				printf".....Failed"
			}
		} else {
			$command="rm -f $rc_dir/rc?.d/*$boot_scripts >/dev/null 2>/dev/null";
			if (system($command) == 0){
				print".....OK\n";
			}else{
				printf".....Failed"
			}
		}
		
	}
	if($num_bri_devices == 0 || $no_smgboot== $TRUE){
                return;
        }
	if( $is_fs == $FALSE  && $is_smg == $FALSE) {
		return;
	}

	if($silent==$FALSE){
		print ("Would you like $script_name to start on system boot?\n");
		$res= &prompt_user_list("YES","NO","");
		if($res eq "NO"){
			return;
		}
	}
	if($safe_mode==$TRUE){
			$script_name='smg_ctrl_safe';
			exec_command("cp -f $current_dir/templates/smg_ctrl_safe $rc_dir/init.d/smg_ctrl_safe");
		
	}

	print "Verifying Network boot scripts...";
	$command="find $rc_dir/rc".$current_run_level.".d/*network >/dev/null 2>/dev/null";
	if (system($command) == 0){
		$command="find $rc_dir/rc".$current_run_level.".d/*network";
		$res=`$command`;
		if ($res =~ /.*S(\d+)network/){
			$network_start_level=$1;
			print "Enabled (level:$network_start_level)\n";
		} else {
			print "\nfailed to parse network boot level, assuming $network_start_level";
		}
	}elsif(system("find $rc_dir/rcS.d/*networking >>/dev/null 2>>/dev/null") == 0){#ubuntu debian fix
			$command="find $rc_dir/rcS.d/*networking ";
			$res=`$command`;
			if ($res =~ /.*S(\d+)network/){
				$network_start_level=$1;
				print "Enabled (level:$network_start_level)\n";
			} else {
				print "\nfailed to parse network boot level, assuming $network_start_level";
			}
 
	}else {
		print "Not installed\n";
		$network_start_level=0;
	}
	if ($network_start_level != 0 && $network_start_level > $zaptel_start_level){
		$smg_ctrl_start_level=$network_start_level+1;	
		print "Enabling smg_ctrl start scripts...(level:$smg_ctrl_start_level)\n";
		#install smg_ctrl in init.d for both safe and regular mode		
		$command="install -D -m 755 /usr/sbin/smg_ctrl $rc_dir/init.d/smg_ctrl";
		if(system($command) !=0){
			print "Failed to install smg_ctrl start scripts to $rc_dir/init.d/smg_ctrl";
			print "smg_ctrl start scripts not installed";
			return 1;
		}
		my $smg_ctrl_start_script='';
		if($smg_ctrl_start_level < 10){
			$smg_ctrl_start_script="S0".$smg_ctrl_start_level."smg_ctrl";
		} else {
			$smg_ctrl_start_script="S".$smg_ctrl_start_level."$script_name";
		}
		print "Enabling $script_name boot scripts ...(level:$smg_ctrl_start_level)\n";

		if ($have_update_rc_d) {
					$command = "update-rc.d $script_name defaults $smg_ctrl_start_level 20";
					if (system($command .' >>/dev/null 2>>/dev/null') !=0 ) {
						print "Command failed: $command\n";
 						return;
 					}
		}else {
			my @run_levels= ("2","3","4","5");
			foreach my $run_level (@run_levels) {
				$command="ln -sf $rc_dir/init.d/$script_name ".$rc_dir."/rc".$run_level.".d/".$smg_ctrl_start_script;
				if(system($command) !=0){
					print "Failed to install $script_name init script to $rc_dir/rc$run_level.d/$smg_ctrl_start_script\n";
					print "$script_name start scripts not installed\n";
					return;
				}
			}	
		}
	}
	
}
sub update_module_load {
	if ($os_type_list =~ m/FreeBSD/){
		return;	
	}

	my @mod_file;
	my $mod_file='';
	if(-e "$asterisk_conf_dir/modules.conf") {

		if(!(-e "$asterisk_conf_dir/modules.conf.bak")){
				exec_command("cp -f $asterisk_conf_dir/modules.conf $asterisk_conf_dir/modules.conf.bak");	
			}
	
		open(FH, "$asterisk_conf_dir/modules.conf") or die "cannot open $asterisk_conf_dir/modules.conf";
		@mod_file=<FH>;
		close (FH);
		foreach my $line (@mod_file){
			if(!($line =~ m/woomera/)){
				$mod_file .= $line;
				if($line =~ m/\[modules]/) {
					if($num_bri_devices !=0 | $num_digital_smg != 0){
						if(-e "/usr/lib/asterisk/modules/chan_woomera.so") {
							$mod_file .= "load=>chan_woomera.so\n";
						}
					}else{
						$mod_file .= "noload=>chan_woomera.so\n";
						
					}
				}
			}
			open (FH,">$asterisk_conf_dir/modules.conf") or die "cannot open $asterisk_conf_dir/modules.conf";;
			print FH $mod_file;
			close (FH);

		}

		if ($is_fs == $FALSE && $is_smg == $FALSE) {
			$mod_file .= "noload=>chan_woomera.so\n";
			open (FH,">$asterisk_conf_dir/modules.conf") or die "cannot open $asterisk_conf_dir/modules.conf";;
			print FH $mod_file;
			close (FH);
		}
	}
	return;
}

sub write_openzap_conf_xml{
	my $openzap_boostpri='';
	my $openzap_boostbri='';
	my $openzap_fxs='';
	my $openzap_fxo='';

	if(@boostprispan){
		#add boost pri conf
		$openzap_boostpri.='<span name="smg_prid">'."\n\t";
		$openzap_boostpri.='<!--<param name="hold-music" value="$${moh_uri}"/>-->'."\n\t";
		$openzap_boostpri.='<param name="dialplan" value="XML"/>'."\n\t";
		$openzap_boostpri.='<param name="context" value="default"/>'."\n\t";;
		$openzap_boostpri.=' <!-- regex to stop dialing when it matches -->'."\n\t";
    	$openzap_boostpri.='<!--<param name="dial-regex" value="5555"/>-->'."\n\t";
    	$openzap_boostpri.='<!-- regex to stop dialing when it does not match -->'."\n\t";
     	$openzap_boostpri.='<!--<param name="fail-dial-regex" value="^5"/>-->'."\n\t";
		$openzap_boostpri.='</span>'."\n";
		
	}
	if(@boostbrispan){
		#add boost bri conf
		$openzap_boostbri.='<span name="smg_brid">'."\n\t";
		$openzap_boostbri.='<!--<param name="hold-music" value="$${moh_uri}"/>-->'."\n\t";
		$openzap_boostbri.='<param name="dialplan" value="XML"/>'."\n\t";
		$openzap_boostbri.='<param name="context" value="default"/>'."\n\t";;
		$openzap_boostbri.=' <!-- regex to stop dialing when it matches -->'."\n\t";
    	$openzap_boostbri.='<!--<param name="dial-regex" value="5555"/>-->'."\n\t";
    	$openzap_boostbri.='<!-- regex to stop dialing when it does not match -->'."\n\t";
     	$openzap_boostbri.='<!--<param name="fail-dial-regex" value="^5"/>-->'."\n";
		$openzap_boostbri.='</span>'."\n";
	}
	
	if(@fxsspan){
		$openzap_fxs.='<span name="FXS">'."\n\t";
		$openzap_fxs.='<!--<param name="hold-music" value="$${moh_uri}"/>-->'."\n\t";
		$openzap_fxs.='<param name="dialplan" value="XML"/>'."\n\t";
		$openzap_fxs.='<param name="context" value="default"/>'."\n\t";;
		$openzap_fxs.=' <!-- regex to stop dialing when it matches -->'."\n\t";
    	$openzap_fxs.='<!--<param name="dial-regex" value="5555"/>-->'."\n\t";
    	$openzap_fxs.='<!-- regex to stop dialing when it does not match -->'."\n\t";
     	$openzap_fxs.='<!--<param name="fail-dial-regex" value="^5"/>-->'."\n";
		$openzap_fxs.='</span>'."\n";
	}
	if(@fxospan){
		$openzap_fxo.='<span name="FXO">'."\n\t";
		$openzap_fxo.='<!--<param name="hold-music" value="$${moh_uri}"/>-->'."\n\t";
		$openzap_fxo.='<param name="dialplan" value="XML"/>'."\n\t";
		$openzap_fxo.='<param name="context" value="default"/>'."\n\t";;
		$openzap_fxo.=' <!-- regex to stop dialing when it matches -->'."\n\t";
    	$openzap_fxo.='<!--<param name="dial-regex" value="5555"/>-->'."\n\t";
    	$openzap_fxo.='<!-- regex to stop dialing when it does not match -->'."\n\t";
     	$openzap_fxo.='<!--<param name="fail-dial-regex" value="^5"/>-->'."\n";
		$openzap_fxo.='</span>'."\n";
	}
	
	my $openzap_xml_file="";
	open(FH, "$openzap_conf_xml_template") or die "cannot open $openzap_conf_xml_template";
	while (<FH>) {
		$openzap_xml_file .= $_;
	}
	close (FH);

	#$openzap_xml_file=$openzap_file.$openzap;	
	$openzap_xml_file=~ s/<!--BOOSTPRI-->/$openzap_boostpri/g;
	$openzap_xml_file=~ s/<!--BOOSTBRI-->/$openzap_boostbri/g;
	$openzap_xml_file=~ s/<!--SANGOMA_FXS-->/$openzap_fxs/g;
	$openzap_xml_file=~ s/<!--SANGOMA_FXO-->/$openzap_fxo/g;
	open(FH, ">$openzap_conf_xml_file") or die "cannot open $openzap_conf_xml_file";
		print FH $openzap_xml_file;
	close(FH);	
	return ;
}

sub write_freetdm_conf_xml_legacy{
	my $freetdm_boostpri='';
	#$freetdm_boostpri.="\n";
	my $freetdm_boostbri='';
	my $freetdm_fxs='';
	my $freetdm_fxo='';
	my $cfg_pri_header='';
	my $cfg_pri_foot='';
	my $cfg_bri_header='';
	my $cfg_bri_foot='';
	my $cfg_analog_header='';
	my $cfg_analog_foot='';
	my $cfg_boost_header='';
	my $cfg_boost_foot='';

	$cfg_boost_header='<boost_spans>';
	$cfg_boost_foot='</boost_spans>';	
	
	if(@boostprispan){
		$cfg_pri_header.="";
		$cfg_pri_foot.="";
		#add boost pri conf
		#$freetdm_boostpri.='<span id="smg_prid">'."\n\t";
                #foreach my $span (@boostprispan){
                #       my $boostprispan=$span;
                        #print "YANNCIK IN WRITE_FREETDM_CONF_XML function\n";
			$freetdm_boostpri.='<span name="boostpri">'."\n\t\t\t";
			$freetdm_boostpri.='<param name="dialplan" value="XML"/>'."\n\t\t\t";
			$freetdm_boostpri.='<param name="context" value="default"/>'."\n\t\t";;
			#$freetdm_boostpri.=$boostprispan->span_no();
			#$freetdm_boostpri.='" sigmod="sangoma_prid">'."\n\t";
			#$freetdm_boostpri.='<param name="signalling" value="';
			#if ($boostprispan->sig_mode() eq "PRI CPE"){
			#       $ftdm_signalling='pri_cpe'
			#}
			#if ($boostprispan->sig_mode() eq "PRI NET"){
			#       $ftdm_signalling = "pri_net";
			#}		
			#$freetdm_boostpri.=$ftdm_signalling;#$boostprispan->sig_mode();
			#$freetdm_boostpri.='"/>'."\n\t";
			#$freetdm_boostpri.='<param name="switchtype" value="';
			#$freetdm_boostpri.=$boostprispan->switch_type();;
			#$freetdm_boostpri.='"/>'."\n\t";
			$freetdm_boostpri.='</span>'."\n\t";
	}


	if(@boostbrispan){
		#add boost bri conf
		$cfg_bri_header.="";
		$cfg_bri_foot.="";
		$freetdm_boostbri.='<span name="smg_brid">'."\n\t\t\t";
		$freetdm_boostbri.='<!--<param name="hold-music" value="$${moh_uri}"/>-->'."\n\t\t\t";
		$freetdm_boostbri.='<param name="dialplan" value="XML"/>'."\n\t\t\t";
		$freetdm_boostbri.='<param name="context" value="default"/>'."\n\t\t\t";;
		$freetdm_boostbri.=' <!-- regex to stop dialing when it matches -->'."\n\t\t\t";
		$freetdm_boostbri.='<!--<param name="dial-regex" value="5555"/>-->'."\n\t\t\t";
		$freetdm_boostbri.='<!-- regex to stop dialing when it does not match -->'."\n\t\t\t";
		$freetdm_boostbri.='<!--<param name="fail-dial-regex" value="^5"/>-->'."\n\t\t";
		$freetdm_boostbri.='</span>'."\n";
	}

	if(@fxsspan){
		#$cfg_analog_header="<analog_spans>";
		#$cfg_analog_foot="</analog_spans>";
		$freetdm_fxs.='<span name="FXS">'."\n\t\t\t";
		$freetdm_fxs.='<!--<param name="hold-music" value="$${moh_uri}"/>-->'."\n\t\t\t";
		$freetdm_fxs.='<param name="dialplan" value="XML"/>'."\n\t\t\t";
		$freetdm_fxs.='<param name="context" value="default"/>'."\n\t\t\t";;
		$freetdm_fxs.=' <!-- regex to stop dialing when it matches -->'."\n\t\t\t";
		$freetdm_fxs.='<!--<param name="dial-regex" value="5555"/>-->'."\n\t\t\t";
		$freetdm_fxs.='<!-- regex to stop dialing when it does not match -->'."\n\t\t\t";
		$freetdm_fxs.='<!--<param name="fail-dial-regex" value="^5"/>-->'."\n\t\t";
		$freetdm_fxs.='</span>'."\n";
	}
	if(@fxospan){
		#$cfg_analog_header="<analog_spans>";
		#$cfg_analog_foot="</analog_spans>";
		$freetdm_fxo.='<span name="FXO">'."\n\t\t\t";
		$freetdm_fxo.='<!--<param name="hold-music" value="$${moh_uri}"/>-->'."\n\t\t\t";
		$freetdm_fxo.='<param name="dialplan" value="XML"/>'."\n\t\t\t";
		$freetdm_fxo.='<param name="context" value="default"/>'."\n\t\t\t";;
		$freetdm_fxo.=' <!-- regex to stop dialing when it matches -->'."\n\t\t\t";
		$freetdm_fxo.='<!--<param name="dial-regex" value="5555"/>-->'."\n\t\t\t";
		$freetdm_fxo.='<!-- regex to stop dialing when it does not match -->'."\n\t\t\t";
		$freetdm_fxo.='<!--<param name="fail-dial-regex" value="^5"/>-->'."\n\t\t";
		$freetdm_fxo.='</span>'."\n";
	}

	my $freetdm_xml_file="";
	open(FH, "$freetdm_conf_xml_template") or die "cannot open $freetdm_conf_xml_template";
	while (<FH>) {
		$freetdm_xml_file .= $_;
	}
	close (FH);

	#$freetdm_xml_file=$freetdm_file.$freetdm;
	$freetdm_xml_file=~ s/<!--BOOST_HEADER-->/$cfg_boost_header/g;
	$freetdm_xml_file=~ s/<!--PRI_HEADER-->/$cfg_pri_header/g;
	$freetdm_xml_file=~ s/<!--BOOSTPRI-->/$freetdm_boostpri/g;
	$freetdm_xml_file=~ s/<!--PRI_FOOT-->/$cfg_pri_foot/g;
	$freetdm_xml_file=~ s/<!--BRI_HEADER-->/$cfg_bri_header/g;
	$freetdm_xml_file=~ s/<!--BOOSTBRI-->/$freetdm_boostbri/g;
	$freetdm_xml_file=~ s/<!--BRI_FOOT-->/$cfg_bri_foot/g;
	$freetdm_xml_file=~ s/<!--BOOST_FOOT-->/$cfg_boost_foot/g;
	#$freetdm_xml_file=~ s/<!--ANALOG_HEADER-->/$cfg_analog_header/g;
	$freetdm_xml_file=~ s/<!--SANGOMA_FXS-->/$freetdm_fxs/g;
	$freetdm_xml_file=~ s/<!--SANGOMA_FXO-->/$freetdm_fxo/g;
	#$freetdm_xml_file=~ s/<!--ANALOG_FOOT-->/$cfg_analog_header/g;
	open(FH, ">$freetdm_conf_xml_file") or die "cannot open $freetdm_conf_xml_file";
	print FH $freetdm_xml_file;
	close(FH);
	return ;
}


sub write_freetdm_conf_xml{
	my $freetdm_boostpri='';
	my $freetdm_boostpri_profile='';
	my $freetdm_boostbri_profile='';
	my $cfg_profile_header='';
	my $cfg_profile_foot='';	
	my $cfg_pri_header='';
	my $cfg_pri_foot='';
	my $cfg_bri_header='';
	my $cfg_bri_foot='';
	my $cfg_analog_header='';
	my $cfg_analog_foot='';
	my $cfg_boost_header='';	
	my $cfg_boost_foot='';
	#$freetdm_boostpri.="\n";
	my $freetdm_boostbri='';
	my $freetdm_fxs='';
	my $freetdm_fxo='';
	my $ftdm_context='public';
	my $num=-1;
	my $count=0;
	my $current_num=-1;
	my $cfg_profile_ni_cpe='';
	my $cfg_profile_dms_cpe='';
	my $cfg_profile_euro_cpe='';
	my $cfg_profile_5ess_cpe='';
	my $cfg_profile_4ess_cpe='';
	my $cfg_profile_ni_net='';
	my $cfg_profile_dms_net='';
	my $cfg_profile_euro_net='';
	my $cfg_profile_5ess_net='';
	my $cfg_profile_4ess_net='';
	my $e1_profile_flag=0;
	my $t1_profile_flag=0;
	$cfg_boost_header.="";
	$cfg_boost_foot.="";
	$cfg_profile_header.="\t";
	$cfg_profile_header.="<config_profiles>";
	$cfg_profile_foot.="\t";
	$cfg_profile_foot.="</config_profiles>";
	$cfg_boost_header.="";
	$cfg_boost_foot.="";

	if(@boostprispan){
		#add boost pri conf
		#$freetdm_boostpri.='<span id="smg_prid">'."\n\t";
		#$cfg_profile_header.="\t";
		#$cfg_profile_header.="<config_profiles>";
		#$cfg_profile_foot.="\t";
		#$cfg_profiile_foot.="</config_profiles>";
		$cfg_pri_header.="<sangoma_pri_spans>";
		$cfg_pri_foot.="</sangoma_pri_spans>";
		#$cfg_bri_header.="<sangoma_bri_spans>";
		#$cfg_bri_foot.="</sangoma_bri_spans>";
		#$cfg_analog_header.="<analog_spans>";
		#$cfg_analog_foot.="</analog_spans>";
		
		foreach my $span (@boostprispan){
				my $boostprispan=$span;
				$cfg_profile='my_pri_te';
				$t1_profile_flag=-1;
				#print "YANNCIK IN WRITE_FREETDM_CONF_XML function\n";
				$num = $num + 1;
				$current_num = $num;

				if ($boostprispan[$num]->sig_mode() eq "PRI CPE"){
					$ftdm_signalling='pri_cpe';
					$ftdm_context='public';
					if($boostprispan->span_type() eq 't1')
					{
						$t1_profile_flag=0;
						$cfg_profile='my_pri_te_';
						#$cfg_profile='my_pri_te';
					}
					if($boostprispan->span_type() eq 'e1')
					{
						$e1_profile_flag=0;
						$cfg_profile='my_pri_te_';
						#$cfg_profile='my_pri_te_e1';
					}
				}

				if ($boostprispan[$num]->sig_mode() eq "PRI NET"){
					$ftdm_signalling = "pri_net";
					$ftdm_context='default';
					if($boostprispan->span_type() eq 't1')
					{
						$t1_profile_flag=1;
						$cfg_profile='my_pri_nt_';
						#$cfg_profile='my_pri_nt';
					}
					if($boostprispan->span_type() eq 'e1')
					{
						$e1_profile_flag=1;
						$cfg_profile='my_pri_nt_';
						#$cfg_profile='my_pri_nt_e1';
					}
				}
					
				if($boostprispan->switch_type() eq 'national')
				{
					for ($count = $current_num; $count > 0; $count--) {
						if($num > 0 && $boostprispan[$current_num]->switch_type() eq $boostprispan[$count-1]->switch_type() && $boostprispan[$current_num]->sig_mode() eq $boostprispan[$count-1]->sig_mode() && ($boostprispan[$current_num]->span_type() eq $boostprispan[$count-1]->span_type())){
							if($boostprispan[$current_num]->sig_mode() eq "PRI NET"){
								$cfg_profile=$cfg_profile_ni_net;
								#$cfg_profile=$cfg_profile.$boostprispan[$count-1]->span_no();
								goto skip;
							}
							if($boostprispan[$current_num]->sig_mode() eq "PRI CPE"){
								$cfg_profile=$cfg_profile_ni_cpe;
								#$cfg_profile=$cfg_profile.$boostprispan[$count-1]->span_no();
								goto skip;
							}

						}
					}
	
					if($boostprispan[$current_num]->sig_mode() eq "PRI NET"){
						$cfg_profile=$cfg_profile_ni_net=$cfg_profile.$boostprispan[$current_num]->span_no();
					}
					if($boostprispan[$current_num]->sig_mode() eq "PRI CPE"){
						$cfg_profile=$cfg_profile_ni_cpe=$cfg_profile.$boostprispan[$current_num]->span_no();
					}

					$freetdm_boostpri_profile.="\n\t\t";
					#$freetdm_boostpri_profile.='<profile name="$cfg_profile.$boostprispan->span_no()"'.'>'."\n\t\t\t";
					$freetdm_boostpri_profile.='<profile name="'.$cfg_profile.'">'."\n\t\t\t";
					#$freetdm_boostpri_profile.='<profile name="'.$cfg_profile.'">'."\n\t\t\t";
					$freetdm_boostpri_profile.='<param name="switchtype" value="ni2" />'."\n\t\t\t";
				}

				if($boostprispan->switch_type() eq 'dms100')
				{
					for ($count = $current_num; $count > 0; $count--) {
						#if($boostprispan[$current_num]->switch_type() eq $boostprispan[$count-1]->switch_type() || ($t1_profile_flag > -1 || $e1_profile_flag > -1)){
							#if($num > 0 && $boostprispan[$current_num]->sig_mode() eq $boostprispan[$count-1]->sig_mode()){
						if($num > 0 && $boostprispan[$current_num]->switch_type() eq $boostprispan[$count-1]->switch_type() && $boostprispan[$current_num]->sig_mode() eq $boostprispan[$count-1]->sig_mode() && ($boostprispan[$current_num]->span_type() eq $boostprispan[$count-1]->span_type())){
								if($boostprispan[$current_num]->sig_mode() eq "PRI NET"){
									$cfg_profile=$cfg_profile_dms_net;
									#$cfg_profile=$cfg_profile.$boostprispan[$count-1]->span_no();
									goto skip;
								}
								if($boostprispan[$current_num]->sig_mode() eq "PRI CPE"){
									$cfg_profile=$cfg_profile_dms_cpe;
									#$cfg_profile=$cfg_profile.$boostprispan[$count-1]->span_no();
							   	 	goto skip;
								}

						}
					}

				    if($boostprispan[$current_num]->sig_mode() eq "PRI NET"){
						$cfg_profile=$cfg_profile_dms_net=$cfg_profile.$boostprispan[$current_num]->span_no();
					}
					if($boostprispan[$current_num]->sig_mode() eq "PRI CPE"){
						$cfg_profile=$cfg_profile_dms_cpe=$cfg_profile.$boostprispan[$current_num]->span_no();
					}

					$freetdm_boostpri_profile.="\n\t\t";			
					$freetdm_boostpri_profile.='<profile name="'.$cfg_profile.'">'."\n\t\t\t"; 	
					$freetdm_boostpri_profile.='<param name="switchtype" value="dms100" />'."\n\t\t\t";
				}

				if($boostprispan->switch_type() eq '5ess')
				{
					for ($count = $current_num; $count > 0; $count--) {
						#if($boostprispan[$current_num]->switch_type() eq $boostprispan[$count-1]->switch_type() || ($t1_profile_flag > -1 || $e1_profile_flag > -1)){
							#if($num > 0 && $boostprispan[$current_num]->sig_mode() eq $boostprispan[$count-1]->sig_mode()){
						if($num > 0 && $boostprispan[$current_num]->switch_type() eq $boostprispan[$count-1]->switch_type() && $boostprispan[$current_num]->sig_mode() eq $boostprispan[$count-1]->sig_mode() && ($boostprispan[$current_num]->span_type() eq $boostprispan[$count-1]->span_type())){
							if($boostprispan[$current_num]->sig_mode() eq "PRI NET"){
								$cfg_profile=$cfg_profile_5ess_net;
								#$cfg_profile=$cfg_profile.$boostprispan[$count-1]->span_no();
								goto skip;
							}
							if($boostprispan[$current_num]->sig_mode() eq "PRI CPE"){
								$cfg_profile=$cfg_profile_5ess_cpe;
								goto skip
							}	
						}
					}

					if($boostprispan[$current_num]->sig_mode() eq "PRI NET"){
						$cfg_profile=$cfg_profile_5ess_net=$cfg_profile.$boostprispan[$current_num]->span_no();
					}
					if($boostprispan[$current_num]->sig_mode() eq "PRI CPE"){
						$cfg_profile=$cfg_profile_5ess_cpe=$cfg_profile.$boostprispan[$current_num]->span_no();
					}

					$freetdm_boostpri_profile.="\n\t\t";
					#$freetdm_boostpri_profile.='<profile name="'.$cfg_profile.$boostprispan->span_no().'">'."\n\t\t\t";
					$freetdm_boostpri_profile.='<profile name="'.$cfg_profile.'">'."\n\t\t\t"; 	
					$freetdm_boostpri_profile.='<param name="switchtype" value="5ess" />'."\n\t\t\t";
				}

				if($boostprispan->switch_type() eq '4ess')
				{
					for ($count = $current_num; $count > 0; $count--) {
						#if($boostprispan[$current_num]->switch_type() eq $boostprispan[$count-1]->switch_type() || ($t1_profile_flag > -1 || $e1_profile_flag > -1)){
							#if($num > 0 && $boostprispan[$current_num]->sig_mode() eq $boostprispan[$count-1]->sig_mode()){
						if($num > 0 && $boostprispan[$current_num]->switch_type() eq $boostprispan[$count-1]->switch_type() && $boostprispan[$current_num]->sig_mode() eq $boostprispan[$count-1]->sig_mode() && ($boostprispan[$current_num]->span_type() eq $boostprispan[$count-1]->span_type())){
							if($boostprispan[$current_num]->sig_mode() eq "PRI NET"){
								$cfg_profile=$cfg_profile_4ess_net;
								#$cfg_profile=$cfg_profile.$boostprispan[$count-1]->span_no();
								goto skip;
							}
							if($boostprispan[$current_num]->sig_mode() eq "PRI CPE"){
								$cfg_profile=$cfg_profile_4ess_cpe;
								goto skip
							}	
						}
					}

					if($boostprispan[$current_num]->sig_mode() eq "PRI NET"){
						$cfg_profile=$cfg_profile_4ess_net=$cfg_profile.$boostprispan[$current_num]->span_no();
					}
					if($boostprispan[$current_num]->sig_mode() eq "PRI CPE"){
						$cfg_profile=$cfg_profile_4ess_cpe=$cfg_profile.$boostprispan[$current_num]->span_no();
					}

					$freetdm_boostpri_profile.="\n\t\t";
					#$freetdm_boostpri_profile.='<profile name="'.$cfg_profile.$boostprispan->span_no().'">'."\n\t\t\t";
					$freetdm_boostpri_profile.='<profile name="'.$cfg_profile.'">'."\n\t\t\t"; 	
					$freetdm_boostpri_profile.='<param name="switchtype" value="4ess" />'."\n\t\t\t";
				}

				if($boostprispan->switch_type() eq 'euroisdn')
				{	
					for ($count = $current_num; $count > 0; $count--) {
						#if($boostprispan[$current_num]->switch_type() eq $boostprispan[$count-1]->switch_type() || ($t1_profile_flag > -1 || $e1_profile_flag > -1)){
							#if($num > 0 && $boostprispan[$current_num]->sig_mode() eq $boostprispan[$count-1]->sig_mode()){
						if($num > 0 && $boostprispan[$current_num]->switch_type() eq $boostprispan[$count-1]->switch_type() && $boostprispan[$current_num]->sig_mode() eq $boostprispan[$count-1]->sig_mode() && ($boostprispan[$current_num]->span_type() eq $boostprispan[$count-1]->span_type())){
								if($boostprispan[$current_num]->sig_mode() eq "PRI NET"){
									$cfg_profile=$cfg_profile_euro_net;
									#$cfg_profile=$cfg_profile.$boostprispan[$count-1]->span_no();
							   		goto skip;
								}
								if($boostprispan[$current_num]->sig_mode() eq "PRI CPE"){
									$cfg_profile=$cfg_profile_euro_cpe;
							   	 goto skip;
								}
						}
					}

					if($boostprispan[$current_num]->sig_mode() eq "PRI NET"){
						$cfg_profile=$cfg_profile_euro_net=$cfg_profile.$boostprispan[$current_num]->span_no();
					}
					if($boostprispan[$current_num]->sig_mode() eq "PRI CPE"){
						$cfg_profile=$cfg_profile_euro_cpe=$cfg_profile.$boostprispan[$current_num]->span_no();
					}

					$freetdm_boostpri_profile.="\n\t\t";
					#$freetdm_boostpri_profile.='<profile name="'.$cfg_profile.$boostprispan->span_no().'">'."\n\t\t\t";
					$freetdm_boostpri_profile.='<profile name="'.$cfg_profile.'">'."\n\t\t\t"; 	
					$freetdm_boostpri_profile.='<param name="switchtype" value="euroisdn" />'."\n\t\t\t";
				}
				
			#	if($boostprispan->switch_type() eq 'qsig')
			#		$freetdm_boostpri_profile.="\n\t\t";
			#		$freetdm_boostpri_profile.='<profile name="'.$cfg_profile.$boostprispan->span_no().'>'."\n\t\t\t";
			#		$freetdm_boostpri_profile.='<param name="switchtype" value="qsig" />'."\n\t\t\t";
			#	}

				
				$freetdm_boostpri_profile.='<param name="interface" value="';
				$freetdm_boostpri_profile.=$ftdm_signalling;
				$freetdm_boostpri_profile.='"/>'."\n\t\t";
				$freetdm_boostpri_profile.='</profile>'."\n\t\t";
			
				#YDBGif($boostprispan[$num]->switch_type() eq $boostprispan[$num-1]->switch_type())
				#{
				#	$freetdm_boostpri.=$num;
				#}
				
skip:					
				$freetdm_boostpri.="\n\t\t";
				#$freetdm_boostpri.='<span name="wp'.$boostprispan->span_no().'"'.' cfgprofile="'.$cfg_profile.$boostprispan->span_no().'">'."\n\t\t\t";
				$freetdm_boostpri.='<span name="wp'.$boostprispan->span_no().'"'.' cfgprofile="'.$cfg_profile.'">'."\n\t\t\t";
        	        	$freetdm_boostpri.='<param name="dialplan" value="XML"/>'."\n\t\t\t";
	                	$freetdm_boostpri.='<param name="context" value="';
				#$freetdm_boostpri.=$ftdm_context;
				$freetdm_boostpri.=$boostprispan->context;
				$freetdm_boostpri.='"/>'."\n\t\t";
				$freetdm_boostpri.='</span>'."\n\t\t";
			#$freetdm_boostpri.=$boostprispan->span_no();
			#$freetdm_boostpri.='" sigmod="sangoma_prid">'."\n\t";
			#$freetdm_boostpri.='<param name="signalling" value="';
			#if ($boostprispan->sig_mode() eq "PRI CPE"){
			#	$ftdm_signalling='pri_cpe';
			#}
			#if ($boostprispan->sig_mode() eq "PRI NET"){
			#	$ftdm_signalling = "pri_net";
			#}
			#$freetdm_boostpri.=$ftdm_signalling;#$boostprispan->sig_mode();
			#$freetdm_boostpri.='"/>'."\n\t";
			#$freetdm_boostpri.='<param name="switchtype" value="';
			#$freetdm_boostpri.=$boostprispan->switch_type();;
			#$freetdm_boostpri.='"/>'."\n\t";
		}
	}
	

	if(@boostbrispan){
		$num=-1;
		$current_num=0;
		$cfg_bri_header.="<sangoma_bri_spans>";
		$cfg_bri_foot.="</sangoma_bri_spans>";
		foreach my $span (@boostbrispan){
                                my $boostbrispan=$span;
				$num = $num + 1; 
				$current_num = $num;
		#add boost bri conf
		#if ($boostbrispan->span_type() eq "TE"){
		#	$ftdm_signalling='cpe';
		#	        printf("boostbrispan->span_type() \n");
			if($boostbrispan->span_type() eq 'TE')
			{
				$ftdm_signalling='cpe';
				$cfg_profile='my_bri_te_';
				$ftdm_context='public';
			}
			if($boostbrispan->span_type() eq 'NT')
			{
				$ftdm_signalling='net';
				$cfg_profile='my_bri_nt_';
				$ftdm_context='default';
			}
		
			for ($count = $current_num; $count > 0; $count--) {
				if($num > 0 && $boostbrispan[$current_num]->span_type() eq $boostbrispan[$count-1]->span_type()){
					$cfg_profile=$cfg_profile.$boostbrispan[$count-1]->span_no();
					goto skip_bri;
				}
			}

			$cfg_profile=$cfg_profile.$boostbrispan[$current_num]->span_no();
			#$freetdm_boostbri_profile.='<profile name="'.$cfg_profile.$boostbrispan->span_no().'">'."\n\t\t\t";
			$freetdm_boostbri_profile.='<profile name="'.$cfg_profile.'">'."\n\t\t\t";
			$freetdm_boostbri_profile.='<param name="switchtype" value="euroisdn" />'."\n\t\t\t";
			$freetdm_boostbri_profile.='<param name="interface" value="';
			$freetdm_boostbri_profile.=$ftdm_signalling;
			$freetdm_boostbri_profile.='"/>'."\n\t\t\t";
			$freetdm_boostbri_profile.='<param name="facility" value="no" />'."\n\t\t";
			$freetdm_boostbri_profile.='</profile>'."\n\t\t";

skip_bri:
			$freetdm_boostbri.="\n\t\t";
			$freetdm_boostbri.='<span name="wp'.$boostbrispan->span_no().'"'.' cfgprofile="'.$cfg_profile.'">'."\n\t\t\t";
			$freetdm_boostbri.='<param name="dialplan" value="XML"/>'."\n\t\t\t";
			#$freetdm_boostbri.='<param name="context" value="$${pstn_context}"/>'."\n\t\t";
			$freetdm_boostbri.='<param name="context" value="';
			$freetdm_boostbri.=$ftdm_context;
			$freetdm_boostbri.='"/>'."\n\t\t";
			$freetdm_boostbri.='</span>'."\n\t\t";

		}
	}
	
	if(@fxsspan){
		#$cfg_analog_header.="<analog_spans>";
		#$cfg_analog_foot.="</analog_spans>";
		$freetdm_fxs.='<span name="FXS">'."\n\t\t\t";
		$freetdm_fxs.='<!--<param name="hold-music" value="$${moh_uri}"/>-->'."\n\t\t\t";
		$freetdm_fxs.='<param name="dialplan" value="XML"/>'."\n\t\t\t";
		$freetdm_fxs.='<param name="context" value="default"/>'."\n\t\t\t";;
		$freetdm_fxs.=' <!-- regex to stop dialing when it matches -->'."\n\t\t\t";
    		$freetdm_fxs.='<!--<param name="dial-regex" value="5555"/>-->'."\n\t\t\t";
    		$freetdm_fxs.='<!-- regex to stop dialing when it does not match -->'."\n\t\t\t";
     		$freetdm_fxs.='<!--<param name="fail-dial-regex" value="^5"/>-->'."\n\t\t";
		$freetdm_fxs.='</span>'."\n";
	}
	if(@fxospan){
		#$cfg_analog_header.="<analog_spans>";
		#$cfg_analog_foot.="</analog_spans>";
		$freetdm_fxo.='<span name="FXO">'."\n\t\t\t";
		$freetdm_fxo.='<!--<param name="hold-music" value="$${moh_uri}"/>-->'."\n\t\t\t";
		$freetdm_fxo.='<param name="dialplan" value="XML"/>'."\n\t\t\t";
		$freetdm_fxo.='<param name="context" value="public"/>'."\n\t\t\t";
		$freetdm_fxo.=' <!-- regex to stop dialing when it matches -->'."\n\t\t\t";
    	$freetdm_fxo.='<!--<param name="dial-regex" value="5555"/>-->'."\n\t\t\t";
    	$freetdm_fxo.='<!-- regex to stop dialing when it does not match -->'."\n\t\t\t";
     	$freetdm_fxo.='<!--<param name="fail-dial-regex" value="^5"/>-->'."\n\t\t";
		$freetdm_fxo.='</span>'."\n";
	}
	
	my $freetdm_xml_file="";
	open(FH, "$freetdm_conf_xml_template") or die "cannot open $freetdm_conf_xml_template";
	while (<FH>) {
		$freetdm_xml_file .= $_;
	}
	close (FH);

	#$freetdm_xml_file=$freetdm_fiile.$freetdm;	
	$freetdm_xml_file=~ s/<!--CONFIG_PROFILE_HEADER-->/$cfg_profile_header/g;
	$freetdm_xml_file=~ s/<!--CONFIG_PRI_PROFILE-->/$freetdm_boostpri_profile/g;
	$freetdm_xml_file=~ s/<!--CONFIG_BRI_PROFILE-->/$freetdm_boostbri_profile/g;
	$freetdm_xml_file=~ s/<!--CONFIG_PROFILE_FOOT-->/$cfg_profile_foot/g;
	$freetdm_xml_file=~ s/<!--BOOST_HEADER-->/$cfg_boost_header/g;
	$freetdm_xml_file=~ s/<!--PRI_HEADER-->/$cfg_pri_header/g;
	$freetdm_xml_file=~ s/<!--BOOSTPRI-->/$freetdm_boostpri/g;
	$freetdm_xml_file=~ s/<!--PRI_FOOT-->/$cfg_pri_foot/g;
	$freetdm_xml_file=~ s/<!--BRI_HEADER-->/$cfg_bri_header/g;
	$freetdm_xml_file=~ s/<!--BOOSTBRI-->/$freetdm_boostbri/g;
	$freetdm_xml_file=~ s/<!--BRI_FOOT-->/$cfg_bri_foot/g;
	$freetdm_xml_file=~ s/<!--BOOST_FOOT-->/$cfg_boost_foot/g;
	#$freetdm_xml_file=~ s/<!--ANALOG_HEADER-->/$cfg_analog_header/g;
	$freetdm_xml_file=~ s/<!--SANGOMA_FXS-->/$freetdm_fxs/g;
	$freetdm_xml_file=~ s/<!--SANGOMA_FXO-->/$freetdm_fxo/g;
	#$freetdm_xml_file=~ s/<!--ANALOG_FOOT-->/$cfg_analog_foot/g;
	open(FH, ">$freetdm_conf_xml_file") or die "cannot open $freetdm_conf_xml_file";
		print FH $freetdm_xml_file;
	close(FH);	
	return ;
}

sub gen_smg_rc{
	#update smg rc 
	printf("Generating smg.cr \n");
	my $rcfile="";
	
	#open smg template
	open(FH, "$smg_rc_template") or die "cannot open $smg_rc_template";
	while (<FH>) {
		$rcfile .= $_;
	}
	close (FH);

	if($num_digital_smg != 0){	
		$rcfile =~ s/SANGOMA_PRID\s*=.*/SANGOMA_PRID="YES"/g;
	}elsif($num_bri_devices !=0){
		$rcfile =~ s/SANGOMA_BRID\s*=.*/SANGOMA_BRID="YES"/g;
	}
	$rcfile=~ s/YES_NO/NO/g;
	$rcfile=~ s/MYDATE/$mdate/g;

	if ($is_fs == $FALSE)
	{
		$rcfile =~ s/SANGOMA_MEDIA_GATEWAY\s*=.*/SANGOMA_MEDIA_GATEWAY="YES"/g;
	}
	open (FH,">$smg_rc_file")or die "cannot open $smg_rc_file";;
	print FH $rcfile;
	close (FH);
}

sub get_wanpipe_port {
	my $local_dev=shift;
	my $local_port=0;

	if ( $local_dev =~ /^(\d+)/) {
		$local_port=$1;
		return $local_port;
	}

	return 0;
}

sub get_current_span_no {

	my $port=shift;
	my $rcfile;
	if (open (FH,"$wanpipe_conf_dir/wanpipe$port.conf")) {
		while (<FH>) {
			$rcfile .= $_;
		}
		close (FH);
		if ($rcfile =~ m/TDMV_SPAN.*=.*(\d+)/) {
			return $1;
		}
		return 1;
	}
}
