#class A10x
#for A10x-sh series cards

package A10x;
use Card;
use strict;

#constructor
sub new	{
	my ($class) = @_;
	my $self = {
		_is_tdm_api => undef,
		_is_data_api => undef,
		_current_dir => undef,
		_card      => undef,		
 		_fe_line   => undef,
		_fe_media  => 'T1',
		_fe_lcode  => 'B8ZS',
		_fe_frame  => 'ESF',
		_fe_clock  => 'NORMAL',
		_te_sig_mode => undef,
		_te_ref_clock  => '0',
		_signalling => 'PRI_CPE',
		_pri_switchtype => 'national',
		_rx_slevel => '360',
		_hw_dchan  => '0',
		_frac_chanfirst => '0',
		_frac_chanlast => '0',
		_ss7_sigchan => undef,
		_ss7_option => undef,
		_ss7_tdmchan => undef,
		_ss7_subinterface => undef,
		_ss7_tdminterface => undef,
		_hp_option => undef,
		_mtu_mru => undef,
		_old_a10u => undef,
		_smg_sig_mode => '1',
		_hw_port_map_mode => 'DEFAULT',
		_tapping_mode => undef,
	};			
	bless $self, $class;
    	return $self;
}

sub card {
	    my ( $self, $card ) = @_;
	        $self->{_card} = $card if defined($card);
		return $self->{_card};
}
sub fe_line {
	    my ( $self, $fe_line ) = @_;
	        $self->{_fe_line} = $fe_line if defined($fe_line);
		    return $self->{_fe_line};
}

sub frac_chan_first {
	    my ( $self, $frac_chan_first ) = @_;
	        $self->{_frac_chan_first} = $frac_chan_first if defined($frac_chan_first);
		    return $self->{_frac_chan_first};
}

sub frac_chan_last {
	    my ( $self, $frac_chan_last ) = @_;
	        $self->{_frac_chan_last} = $frac_chan_last if defined($frac_chan_last);
		    return $self->{_frac_chan_last};
}


sub te_ref_clock {
	    my ( $self, $te_ref_clock ) = @_;
	        $self->{_te_ref_clock} = $te_ref_clock if defined($te_ref_clock);
		    return $self->{_te_ref_clock};
}
sub signalling {
	    my ( $self, $signalling ) = @_;
	    $self->{_signalling} = $signalling if defined($signalling);
	    return $self->{_signalling};
}

sub fe_media {
	    my ( $self, $fe_media ) = @_;
	        $self->{_fe_media} = $fe_media if defined($fe_media);
		    return $self->{_fe_media};
}

sub fe_lcode {
	    my ( $self, $fe_lcode) = @_;
	        $self->{_fe_lcode} = $fe_lcode if defined($fe_lcode);
		    return $self->{_fe_lcode};
}

sub fe_frame {
	    my ( $self, $fe_frame ) = @_;
	        $self->{_fe_frame} = $fe_frame if defined($fe_frame);
		    return $self->{_fe_frame};
}

sub hw_dchan {
	    my ( $self, $hw_dchan ) = @_;
	        $self->{_hw_dchan} = $hw_dchan if defined($hw_dchan);
		    return $self->{_hw_dchan};
}

sub fe_clock {
	   my ( $self, $fe_clock ) = @_;
	        $self->{_fe_clock} = $fe_clock if defined($fe_clock);
		    return $self->{_fe_clock};
}

sub te_sig_mode {
	   my ( $self, $te_sig_mode ) = @_;
	        $self->{_te_sig_mode} = $te_sig_mode if defined($te_sig_mode);
		    return $self->{_te_sig_mode};
}

sub pri_switchtype {
	   my ( $self, $pri_switchtype ) = @_;
	        $self->{_pri_switchtype} = $pri_switchtype if defined($pri_switchtype);
		    return $self->{_pri_switchtype};
}

sub current_dir {
           my ( $self, $current_dir ) = @_;
                $self->{_current_dir} = $current_dir if defined($current_dir);
                 return $self->{_current_dir};
}

sub ss7_option {
          my ( $self, $ss7_option ) = @_;
                $self->{_ss7_option} = $ss7_option if defined($ss7_option);
                 return $self->{_ss7_option};     
}

sub hp_option {
          my ( $self, $hp_option ) = @_;
                $self->{_hp_option} = $hp_option if defined($hp_option);
                 return $self->{_hp_option};     
}

sub ss7_sigchan {
          my ( $self, $ss7_sigchan ) = @_;
                $self->{_ss7_sigchan} = $ss7_sigchan if defined($ss7_sigchan);		
	        return $self->{_ss7_sigchan};     
}

sub ss7_tdmchan {
          my ( $self, $ss7_tdmchan ) = @_;
                $self->{_ss7_tdmchan} = $ss7_tdmchan if defined($ss7_tdmchan);		
	        return $self->{_ss7_tdmchan};     
}

sub ss7_subinterface {
          my ( $self, $ss7_subinterface ) = @_;
                $self->{_ss7_subinterface} = $ss7_subinterface if defined($ss7_subinterface);		
	        return $self->{_ss7_subinterface};     
}

sub ss7_tdminterface {
	  my ( $self, $ss7_tdminterface ) = @_;
                $self->{_ss7_tdminterface} = $ss7_tdminterface if defined($ss7_tdminterface);		
	        return $self->{_ss7_tdminterface};   
}

sub mtu_mru {
          my ( $self, $mtu_mru ) = @_;
                $self->{_mtu_mru} = $mtu_mru if defined($mtu_mru);		
	        return $self->{_mtu_mru};     
}
sub is_tdm_api {
	            my ( $self, $is_tdm_api ) = @_;
		                    $self->{_is_tdm_api} = $is_tdm_api if defined($is_tdm_api);
				                        return $self->{_is_tdm_api};
}
sub is_data_api {
	            my ( $self, $is_data_api ) = @_;
		                    $self->{_is_data_api} = $is_data_api if defined($is_data_api);
				                        return $self->{_is_data_api};
}

sub old_a10u {
          my ( $self, $old_a10u ) = @_;
                $self->{_old_a10u} = $old_a10u if defined($old_a10u);		
	        return $self->{_old_a10u};     
}

sub rx_slevel {
	            my ( $self, $rx_slevel ) = @_;
		                    $self->{_rx_slevel} = $rx_slevel if defined($rx_slevel);
				                        return $self->{_rx_slevel};
}

sub hw_port_map_mode {
	my ( $self, $hw_port_map_mode ) = @_;
	$self->{_hw_port_map_mode} = $hw_port_map_mode if defined($hw_port_map_mode);
	return $self->{_hw_port_map_mode};
}

sub tapping_mode {
	my ( $self, $tapping_mode ) = @_;
	$self->{_tapping_mode} = $tapping_mode if defined($tapping_mode);
	return $self->{_tapping_mode};
}


sub prompt_user{
	my($promptString, $defaultValue) = @_;
	if ($defaultValue) {
		print $promptString, "[", $defaultValue, "]: ";
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

sub smg_sig_mode {
	   my ( $self, $smg_sig_mode ) = @_;
	        $self->{_smg_sig_mode} = $smg_sig_mode if defined($smg_sig_mode);
		    return $self->{_smg_sig_mode};
}
sub prompt_user_list{
	my @list = @_;
	my $i;
	my $valid = 0;
	for $i (0..$#list) {
		printf(" %s\. %s\n",$i+1, @list[$i]);
	}
	while ($valid == 0){
		$| = 1;               # force a flush after our print
		$_ = <STDIN>;         # get the input from STDIN (presumably the keyboard)
		chomp;
			if ( $_ =~ /(\d+)/ ){
			if ( $1 > $#list+1) {
				print "Invalid option: Value out of range \n";
			} else {
			return $1-1 ;
			}
		} else {
			print "Invalid option: Input an integer\n";
		}
	}
}

sub print {
	my ($self) = @_;
	$self->card->print();    
	printf (" fe_line: %s\n fe_media: %s\n fe_lcode: %s\n fe_frame: %s \n fe_clock:%s\n ", $self->fe_line, $self->fe_media, $self->fe_lcode, $self->fe_frame, $self->fe_clock);

}

sub get_alpha_from_num {
	my ($num) = @_;
	my $alpha_str="";	
	my $alpha_char="";
	my $i;
	my @chars = split(//, $num);
	for $i (0..$#chars) {
		if ( $i == 0 ) {
			$alpha_char=chr(ord(@chars[$i])+48);	
		} else {
			$alpha_char=chr(ord(@chars[$i])+49);	
		}
		$alpha_str=$alpha_str."".$alpha_char;
	}
	return $alpha_str;
}


sub gen_wanpipe_ss7_subinterfaces{
        my ($self) = @_;
	my $wanpipe_ss7_conf_file = $self->card->current_dir."/".$self->card->cfg_dir."/wanpipe".$self->card->device_no.".conf";
	my $ss7_sigchan = $self->ss7_sigchan;
	my $tdmv_span_no = $self->card->tdmv_span_no;
	my $device_no = $self->card->device_no;
	my $ss7_subinterface = $self->ss7_subinterface;
	my $ss7_tdmchan = $self->ss7_tdmchan;
	my $mtu_mru = $self->mtu_mru;
	my $hwec_mode = $self->card->hwec_mode;
	my $ss7_tdminterface = $self->ss7_tdminterface;
	my $hp_option	= $self->hp_option;
	my $card_type="WAN_AFT_TE1";
	my $old_a10u =  $self->old_a10u;
	my $rx_slevel= $self->rx_slevel;
	my $wanpipe_ss7_interfaces_template = $self->card->current_dir."/templates/ss7_a100/wanpipe.ss7.$ss7_subinterface";
	if (!($hp_option == ' ')) {
		#for hp tdmapi change templates 
		$wanpipe_ss7_interfaces_template = $self->card->current_dir."/templates/hp_a100/wanpipe.hp.$ss7_subinterface";
	}
	
	if(($old_a10u =~ m/YES/)){
		$card_type="WAN_AFT";
	}
	
	my $hw_dtmf = $self->card->hw_dtmf;
	my $hw_fax = $self->card->hw_fax;
	
	open(FH, $wanpipe_ss7_interfaces_template) or die "Can't open $wanpipe_ss7_interfaces_template";
	my $wp_file='';

       	while (<FH>) {
		if (!(($ss7_tdminterface == ' ') && (($wanpipe_ss7_interfaces_template =~ m/2/) || ($wanpipe_ss7_interfaces_template =~ m/5/)))){
			$wp_file .= $_;
		}
	}
	close (FH);

	open(FH, ">>$wanpipe_ss7_conf_file") or die "Cant open $wanpipe_ss7_conf_file";
	$wp_file =~ s/WAN_AFT_TYPE/$card_type/g;
	$wp_file =~ s/DEVNUM/$device_no/g;
	$wp_file =~ s/SS7SIGCHAN/$ss7_sigchan/g;
	$wp_file =~ s/TDMVOICECHAN/$ss7_tdmchan/g;
	$wp_file =~ s/HWECMODE/$hwec_mode/g;
	$wp_file =~ s/RXSLEVEL/$rx_slevel/g;
	$wp_file =~ s/CHUNKTIME/$mtu_mru/g;
	$wp_file =~ s/VOICEINTERFACE/$ss7_tdminterface/g;
	$wp_file =~ s/TDMVSPANNO/$tdmv_span_no/g;
	$wp_file =~ s/HWDTMF/$hw_dtmf/g;
	$wp_file =~ s/HWFAX/$hw_fax/g;
	print FH $wp_file;
	close (FH);
}

sub gen_wanpipe_conf{
	my ($self, $is_freebsd) = @_;
	my $wanpipe_conf_template = $self->card->current_dir."/templates/wanpipe.tdm.a100";
	my $wanpipe_conf_file = $self->card->current_dir."/".$self->card->cfg_dir."/wanpipe".$self->card->device_no.".conf";
	my $device_no = $self->card->device_no;
	my $tdmv_span_no = $self->card->tdmv_span_no;
	my $pci_slot = $self->card->pci_slot;
	my $pci_bus = $self->card->pci_bus;
	my $fe_media = $self->fe_media;
	my $fe_lcode = $self->fe_lcode;
	my $fe_frame = $self->fe_frame;
	my $fe_line = $self->fe_line;
	my $fe_clock = $self->fe_clock;
	my $te_ref_clock = $self->te_ref_clock;
	my $te_sig_mode = $self->te_sig_mode;
	my $hwec_mode = $self->card->hwec_mode;
	my $hw_dtmf = $self->card->hw_dtmf;
        my $hw_fax = $self->card->hw_fax;
	my $ss7_option = $self->ss7_option;
	my $hp_option	= $self->hp_option;
	my $dchan = $self->hw_dchan;
	my $fe_lbo;
	my $old_a10u =  $self->old_a10u;
	my $fe_cpu = "A";
	my $card_type="WAN_AFT_TE1";
	my $tdm_voice_op_mode = "TDM_VOICE";
	my $mtu_size='8';
	my $device_alpha = &get_alpha_from_num($device_no);

	my $te_sig_mode_line='';
	my $card_model = $self->card->card_model;
	my $rx_slevel= $self->rx_slevel;
	my $hw_port_map_mode = $self->hw_port_map_mode;
	my $tapping_mode = $self->tapping_mode;
	if ($ss7_option == 1){
	        $wanpipe_conf_template = $self->card->current_dir."/templates/ss7_a100/wanpipe.ss7.4";
	} elsif ($ss7_option == 2){
                $wanpipe_conf_template = $self->card->current_dir."/templates/ss7_a100/wanpipe.tdmvoiceapi.a100";
	} elsif ($hp_option == 1){
	        $wanpipe_conf_template = $self->card->current_dir."/templates/hp_a100/wanpipe.hp.4";
	}
	
	if(($old_a10u =~ m/YES/)){
		#Set card type and CPU for old card in for hpdmapi
		$card_type="WAN_AFT";
		if ($self->fe_line eq '1'){
			$fe_cpu='A';
		}elsif($self->fe_line eq '2'){
			$fe_cpu='B';
		}else{
			print "Error: Invalid port on A101-2u\n";
			exit 1;
       		}
	}
	if($is_freebsd){
        $dchan = 0;
	}
	if(!$is_freebsd){
		if ($self->signalling  =~ m/PRI/ | $self->signalling  =~ m/SS7/ ){
			if(($self->fe_media eq 'T1')){
				$dchan=24;
			}else{ 
				$dchan=16;
			}
		}
	}

   	if($self->fe_media eq 'T1'){
		$te_sig_mode_line='';
		$fe_lbo='0DB';
	}else{
		$fe_lbo='120OH';
		$te_sig_mode_line= 'TE_SIG_MODE     = '.$te_sig_mode;
	}

	if($self->is_data_api eq '0') {
		$tdm_voice_op_mode = "DATA_API";
		$mtu_size='80';
		$dchan = $self->hw_dchan;
	} elsif($self->is_tdm_api eq '0') {
		$tdm_voice_op_mode = "TDM_VOICE_API";
		#for tdm_api hw_dchan is set by user
		$dchan = $self->hw_dchan;
		$mtu_size='80';
	}

	
	open(FH, $wanpipe_conf_template) or die "Can't open $wanpipe_conf_template";
	my $wp_file='';
       	while (<FH>) {
       		$wp_file .= $_;
	}
	close (FH);

	open(FH, ">>$wanpipe_conf_file") or die "Cant open $wanpipe_conf_file";
        $wp_file =~ s/DEVNUM/$device_no/g;

	if ( $is_freebsd ) {
	        $wp_file =~ s/IFNUM/$device_alpha/g;
	} else {
		$wp_file =~ s/IFNUM/$device_no/g;
	}
	$wp_file =~ s/TDM_VOICE_OP_MODE/$tdm_voice_op_mode/g;
	$wp_file =~ s/FECPU/$fe_cpu/g;
	$wp_file =~ s/WAN_AFT_TYPE/$card_type/g;
	$wp_file =~ s/SLOTNUM/$pci_slot/g;
	$wp_file =~ s/BUSNUM/$pci_bus/g;
	$wp_file =~ s/FEMEDIA/$fe_media/g;
	$wp_file =~ s/FELCODE/$fe_lcode/g;
	$wp_file =~ s/FEFRAME/$fe_frame/g;
	$wp_file =~ s/FELINE/$fe_line/g;
	$wp_file =~ s/TESIGMODE/$te_sig_mode_line/g;
    $wp_file =~ s/FECLOCK/$fe_clock/g;
	$wp_file =~ s/TEREFCLOCK/$te_ref_clock/g;
	$wp_file =~ s/RXSLEVEL/$rx_slevel/g;
    $wp_file =~ s/FELBO/$fe_lbo/g;

	if ($fe_lcode eq "AMI" && $fe_frame eq "D4" ){
		print "Setting DCHAN $dchan to zero\n";
		$dchan=0;
	}

	if($self->is_data_api eq '0') {
    	$wp_file =~ s/TDMVDCHAN/0/g;
		$hw_dtmf="NO";
		$hw_fax="NO";
		$hwec_mode="NO";
	} else {
    	$wp_file =~ s/TDMVDCHAN/$dchan/g;
	}
	$wp_file =~ s/TDMVSPANNO/$tdmv_span_no/g;
    $wp_file =~ s/HWECMODE/$hwec_mode/g;
    $wp_file =~ s/HWDTMF/$hw_dtmf/g;

	if ($tapping_mode eq 'YES') {
		$wp_file =~ s/TE_HIGHIMPEDANCE.*/TE_HIGHIMPEDANCE\t= YES/g;
		$wp_file =~ s/TE_RX_SLEVEL.*/TE_RX_SLEVEL\t= 120/g;
	}

	if($self->is_data_api eq '0') {
		if ($dchan == 'NO') {
			$wp_file =~ s/ACTIVE_CH(.*)/HDLC_STREAMING\t= NO\nACTIVE_CH$1/g;
		} elsif ($dchan == '7bit') {
			$wp_file =~ s/ACTIVE_CH(.*)/HDLC_STREAMING\t= YES\nSEVEN_BIT_HDLC\t= YES\nACTIVE_CH$1/g;
		} else {
			$wp_file =~ s/ACTIVE_CH(.*)/HDLC_STREAMING\t= YES\nACTIVE_CH$1/g;
			$mtu_size=0;
		}
	}
	if($card_model eq '108'){
		$wp_file =~ s/HWRJ45PORTMAP/$hw_port_map_mode/g;
	} else {
		$wp_file =~ s/HWRJ45PORTMAP/DEFAULT/g;
	}

    $wp_file =~ s/HWFAX/$hw_fax/g;

	$wp_file =~ s/MTUSIZE/$mtu_size/g;
	       
	print FH $wp_file;
	close (FH);
# print "\n created $fname for A$card_model $devnum SLOT $slot BUS $bus HWEC $hwec_mode\n";
}


sub gen_zaptel_conf{
	my ($self, $dchan_str) = @_;
	my $zap_lcode;
	my $zap_frame;
	my $zap_crc4;
 	my $zp_file="";
	my $dahdi_conf = $self->card->dahdi_conf;
	my $hwec_mode = $self->card->hwec_mode;
	my $dahdi_echo = $self->card->dahdi_echo;
	my $card_model = $self->card->card_model;
			

    if ( $self->fe_lcode eq 'B8ZS' ){
		$zap_lcode='b8zs';
	} elsif ( $self->fe_lcode eq 'AMI' ){
		$zap_lcode='ami';
	} elsif ( $self->fe_lcode eq 'HDB3' ){
		$zap_lcode='hdb3';
	} else {
        	printf("Error: invalid line coding %s\n", $self->fe_lcode);
		exit;
	}

        if ( $self->fe_frame eq 'ESF' ){
		$zap_frame='esf';
	} elsif ( $self->fe_frame eq 'D4' ){
		$zap_frame='d4';
	} elsif ( $self->fe_frame eq 'CRC4' ){
		$zap_frame='ccs';
		$zap_crc4=',crc4';
	} elsif ( $self->fe_frame eq 'NCRC4' ){
		$zap_frame='ccs';
	} else {
        	printf("Error: invalid line framing %s\n", $self->fe_frame);
	   	exit;
	}

        $zp_file.="\n\#Sangoma A".$self->card->card_model." port ".$self->fe_line." [slot:".$self->card->pci_slot." bus:".$self->card->pci_bus." span:".$self->card->tdmv_span_no."] <wanpipe".$self->card->device_no.">\n";
	$zp_file.="span=".$self->card->tdmv_span_no.",".$self->card->tdmv_span_no.",0,".$zap_frame.",".$zap_lcode.$zap_crc4."\n";	
	
	if ( $self->signalling =~ /PRI/ ){
        	if ( $self->fe_media eq 'T1' ){
			if($self->frac_chan_first() != 0){
				if ($self->frac_chan_first == $self->frac_chan_last) {
					my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
		                	$zp_file.="bchan=".$first_ch."\n"; 
					if($dahdi_conf eq 'YES') {
						if($hwec_mode eq 'NO' ) {
							$zp_file.="echocanceller=" .$dahdi_echo.",".$first_ch."\n"; 
						}

					}
					$zp_file.=$dchan_str."=".($self->card->first_chan+23)."\n";
				} else {
					my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
					if($self->frac_chan_last == 24){
						$self->frac_chan_last(23);
					}
					my $last_ch=$self->card->first_chan + $self->frac_chan_last-1;
		                	$zp_file.="bchan=".$first_ch."-".$last_ch."\n"; 
					#$zp_file.="echocanceller=mg2".$first_ch."-".$last_ch."\n"; 
					#dahdi software echo canellation conf
					if($dahdi_conf eq 'YES') {
							$zp_file.="echocanceller=" .$dahdi_echo.",".$first_ch."-".$last_ch."\n"; 

					}

					$zp_file.=$dchan_str."=".($self->card->first_chan+23)."\n";
				}
			} else {
	                	$zp_file.="bchan=".$self->card->first_chan."-".($self->card->first_chan+22)."\n"; 

				if($dahdi_conf eq 'YES') {
							$zp_file.="echocanceller=".$dahdi_echo.",".$self->card->first_chan."-".($self->card->first_chan+22)."\n";  

				}

				$zp_file.=$dchan_str."=".($self->card->first_chan+23)."\n";
			}
		} else {
			if($self->frac_chan_first() != 0){
				if ($self->frac_chan_first == $self->frac_chan_last) {
					if ($self->frac_chan_first == 16) {
						print "Warning: You will not have any b-channels configured for this port";
						print "Would you like to continue?";
						if (&prompt_user_list(("YES","EXIT","")) eq 'EXIT'){
							exit(1);	
						}
					} else {
						my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
		                		$zp_file.="bchan=".$first_ch."\n"; 
						if($dahdi_conf eq 'YES') {
							$zp_file.="echocanceller=".$dahdi_echo.",".$first_ch."\n";  

						}

						$zp_file.=$dchan_str."=".($self->card->first_chan+15)."\n";
					}
				} else {
					if($self->frac_chan_last() == 16){
						$self->frac_chan_last(15);
					}
					if($self->frac_chan_first() == 16){
						$self->frac_chan_first(17);
					}
					if($self->frac_chan_last() > 15){
						my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
						my $last_ch=$self->card->first_chan + $self->frac_chan_last-1;
						my $mid_ch1=$self->card->first_chan + 14;
						my $mid_ch2=$self->card->first_chan + 16;
					
		                		$zp_file.="bchan=".$first_ch."-".$mid_ch1.",".$mid_ch2."-".$last_ch."\n"; 
						if($dahdi_conf eq 'YES') {
							$zp_file.="echocanceller=".$dahdi_echo.",".$first_ch."-".$mid_ch1.",".$mid_ch2."-".$last_ch."\n";  

						}
						
						$zp_file.=$dchan_str."=".($self->card->first_chan+15)."\n";
					
					} else {
						my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
						my $last_ch=$self->card->first_chan + $self->frac_chan_last-1;
		                		$zp_file.="bchan=".$first_ch."-".$last_ch."\n"; 
						if($dahdi_conf eq 'YES') {
							$zp_file.="echocanceller=".$dahdi_echo.",".$first_ch."-".$last_ch."\n";  

						}

						$zp_file.=$dchan_str."=".($self->card->first_chan+15)."\n";
					}
				}
			} else {
                		$zp_file.="bchan=".$self->card->first_chan."-".($self->card->first_chan+14).",".($self->card->first_chan+16)."-".($self->card->first_chan+30)."\n"; 

				if($dahdi_conf eq 'YES') {
						$zp_file.="echocanceller=".$dahdi_echo.",".$self->card->first_chan."-".($self->card->first_chan+14).",".($self->card->first_chan+16)."-".($self->card->first_chan+30)."\n";   

				}
				

				$zp_file.=$dchan_str."=".($self->card->first_chan+15)."\n";
			}
		}  
	} else {
		my $zap_signal;
        	if ( $self->signalling eq 'Zaptel/Dahdi - E & M' | $self->signalling eq 'Zaptel/Dahdi - E & M Wink' ){
			$zap_signal='e&m';
		} elsif ( $self->signalling eq 'Zaptel/Dahdi - FXS - Loop Start' ){
			$zap_signal='fxsls';
		} elsif ( $self->signalling eq 'Zaptel/Dahdi - FXS - Ground Start' ){
			$zap_signal='fxsgs';
		} elsif ( $self->signalling eq 'Zaptel/Dahdi - FXS - Kewl Start' ){
			$zap_signal='fxsks';
		} elsif ( $self->signalling eq 'Zaptel/Dahdi - FX0 - Loop Start' ){
			$zap_signal='fxols';
		} elsif ( $self->signalling eq 'Zaptel/Dahdi - FX0 - Ground Start' ){
			$zap_signal='fxogs';
		} elsif ( $self->signalling eq 'Zaptel/Dahdi - FX0 - Kewl Start' ){
			$zap_signal='fxoks';
		} else {
                 	printf("Error: invalid signalling %s\n", $self->card->signalling);
		}
		
        	if ( $self->fe_media eq 'T1' ){
			if($self->frac_chan_first() != 0){
				if ($self->frac_chan_first == $self->frac_chan_last) {
					my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
	        	        	$zp_file.=$zap_signal."=".$first_ch."\n"; 
					if($dahdi_conf eq 'YES') {
						$zp_file.="echocanceller=".$dahdi_echo.",".$first_ch."\n";   

					}


				} else {
					my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
					my $last_ch=$self->card->first_chan + $self->frac_chan_last-1;
	        	        	$zp_file.=$zap_signal."=".$first_ch."-".$last_ch."\n";
					if($dahdi_conf eq 'YES') {
						$zp_file.="echocanceller=".$dahdi_echo.",".$first_ch."-".$last_ch."\n";

					}

 
				}
			} else {
                		$zp_file.=$zap_signal."=".$self->card->first_chan."-".($self->card->first_chan+23)."\n"; 
				if($dahdi_conf eq 'YES') {
						$zp_file.="echocanceller=".$dahdi_echo.",".$self->card->first_chan."-".($self->card->first_chan+23)."\n";

					}
				
			}
		} else {
			if($self->frac_chan_first() != 0){
				if ($self->frac_chan_first == $self->frac_chan_last) {
					my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
		                	$zp_file.=$zap_signal."=".$first_ch."\n"; 
				} else {
					my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
					my $last_ch=$self->card->first_chan + $self->frac_chan_last-1;
		                	$zp_file.=$zap_signal."=".$first_ch."-".$last_ch."\n"; 
				}
			} else {
                		$zp_file.=$zap_signal."=".$self->card->first_chan."-".($self->card->first_chan+30)."\n"; 
			}
		}
	}
	return $zp_file;	
	
}

sub gen_zapata_conf{
	my ($self) = @_;
        
 	my $zp_file='';
	my $dahdi_conf = $self->card->dahdi_conf;
	my $hwec_mode = $self->card->hwec_mode;
	my $fax_detect = $self->card->hw_fax;
       $zp_file.="\n\;Sangoma A".$self->card->card_model." port ".$self->fe_line." [slot:".$self->card->pci_slot." bus:".$self->card->pci_bus." span:".$self->card->tdmv_span_no."] <wanpipe".$self->card->device_no.">\n";

	if ( $self->signalling =~ m/PRI/ ){
		$zp_file.="switchtype=".$self->pri_switchtype."\n";
	}	
       
	$zp_file.="context=".$self->card->zap_context."\n";
       	$zp_file.="group=".$self->card->zap_group."\n";
	if($dahdi_conf eq 'YES') {
			$zp_file.="echocancel=yes\n";

	}

	if($fax_detect eq 'YES'){
			$zp_file.="faxdetect=incoming\n";
	}
       	
	if ( $self->signalling eq 'Zaptel/Dahdi - PRI NET' ){
		$zp_file.="signalling=pri_net\n";
	} elsif ( $self->signalling eq 'Zaptel/Dahdi - PRI CPE' ){
		$zp_file.="signalling=pri_cpe\n";
	} elsif ( $self->signalling eq 'Zaptel/Dahdi - E & M' ){
		$zp_file.="signalling=em\n";
       	} elsif ($self->signalling eq 'Zaptel/Dahdi - E & M Wink' ){
		$zp_file.="signalling=em_w\n";
	} elsif ( $self->signalling eq 'Zaptel/Dahdi - FXS - Loop Start' ){
		$zp_file.="signalling=fxs_ls\n";
	} elsif ( $self->signalling eq 'Zaptel/Dahdi - FXS - Ground Start' ){
		$zp_file.="signalling=fxs_gs\n";
	} elsif ( $self->signalling eq 'Zaptel/Dahdi - FXS - Kewl Start' ){
		$zp_file.="signalling=fxs_ks\n";
	} elsif ( $self->signalling eq 'Zaptel/Dahdi - FX0 - Loop Start' ){
		$zp_file.="signalling=fxo_ls\n";
	} elsif ( $self->signalling eq 'Zaptel/Dahdi - FX0 - Ground Start' ){
		$zp_file.="signalling=fxo_gs\n";
	} elsif ( $self->signalling eq 'Zaptel/Dahdi - FX0 - Kewl Start' ){
		$zp_file.="signalling=fxo_ks\n";
	} else {
               	printf("Error: invalid signalling %s\n", $self->signalling);
	}
		
       	if ( $self->signalling eq 'Zaptel/Dahdi - PRI NET' | $self->signalling eq 'Zaptel/Dahdi - PRI CPE' ){
              	if ( $self->fe_media eq 'T1' ){
			if($self->frac_chan_first() != 0){
				if ($self->frac_chan_first == $self->frac_chan_last) {
					my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
		                	$zp_file.="channel =>".$first_ch."\n"; 
					
				} else {
					my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
					if($self->frac_chan_last == 24){
						$self->frac_chan_last(23);
					}
					my $last_ch=$self->card->first_chan + $self->frac_chan_last-1;
	                		$zp_file.="channel =>".$first_ch."-".$last_ch."\n"; 
					
				}
			} else {
	                	$zp_file.="channel =>".$self->card->first_chan."-".($self->card->first_chan+22)."\n"; 
			}
		}else{
			if($self->frac_chan_first() != 0){
				if ($self->frac_chan_first == $self->frac_chan_last) {
					#already sent warning on gen_zaptel_conf
					if ($self->frac_chan_first != 16) {
						my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
	                			$zp_file.="channel =>".$first_ch."\n"; 
					}
				} else {
					if($self->frac_chan_last() == 16){
						$self->frac_chan_last(15);
					}
					if($self->frac_chan_first() == 16){
						$self->frac_chan_first(17);
					}
					if($self->frac_chan_last() > 15){
						my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
						my $last_ch=$self->card->first_chan + $self->frac_chan_last-1;
						my $mid_ch1=$self->card->first_chan + 14;
						my $mid_ch2=$self->card->first_chan + 16;
					
		                		$zp_file.="channel =>".$first_ch."-".$mid_ch1.",".$mid_ch2."-".$last_ch."\n"; 
					} else {
						my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
						my $last_ch=$self->card->first_chan + $self->frac_chan_last-1;
	                			$zp_file.="channel =>".$first_ch."-".$last_ch."\n"; 
					}
				}
			} else {
                		$zp_file.="channel =>".$self->card->first_chan."-".($self->card->first_chan+14).",".($self->card->first_chan+16)."-".($self->card->first_chan+30)."\n"; 
			}
		}
	} else {
		if ( $self->fe_media eq 'T1' ){
			if($self->frac_chan_first() != 0){
				if ($self->frac_chan_first == $self->frac_chan_last) {
					my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
	        	        	$zp_file.="channel =>".$first_ch."\n"; 
				} else {
					my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
					my $last_ch=$self->card->first_chan + $self->frac_chan_last-1;
	        	        	$zp_file.="channel =>".$first_ch."-".$last_ch."\n"; 
				}
			} else {
               			$zp_file.="channel => ".$self->card->first_chan."-".($self->card->first_chan+23)."\n"; 
			}
		} else {
			if($self->frac_chan_first() != 0){
				if ($self->frac_chan_first == $self->frac_chan_last) {
					my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
		                	$zp_file.="channel =>".$first_ch."\n"; 
				} else {
					my $first_ch=$self->card->first_chan + $self->frac_chan_first-1;
					my $last_ch=$self->card->first_chan + $self->frac_chan_last-1;
			                $zp_file.="channel =>".$first_ch."-".$last_ch."\n"; 
				}
			} else {
               			$zp_file.="channel => ".$self->card->first_chan."-".($self->card->first_chan+30)."\n"; 
			}
		}
	}
	return $zp_file;	
	
}
sub gen_pri_conf{
	my ($self ,$span_no,$span_type, $group_no, $switch_type,$trunk_type,$sig_mode) = @_;
	my $pri_file='';
	
	$pri_file.="\n";
	#$pri_file.="link=$trunk_type\n";
	if ($sig_mode =~ m/CPE/ ){
		$pri_file.="signalling=pri_cpe\n";
	} elsif ( $sig_mode =~ m/NET/ ){
			$pri_file.="signalling=pri_net\n";
	}
	$pri_file.="switchtype=$switch_type\n";	
	$pri_file.="group=$group_no\n";
	$pri_file.="spans=$span_no\n";
	#$pri_file.="operator=$operator\n";

	return $pri_file;	
}
sub gen_woomera_conf{
	my ($self, $group, $context) = @_;
	my $woomera_file='';
	
	$woomera_file.="\n";
	$woomera_file.="context=$context\n";
	$woomera_file.="group=$group\n";
	return $woomera_file;	
}
1;
