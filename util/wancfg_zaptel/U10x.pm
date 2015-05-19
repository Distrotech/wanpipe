#class A10x
#for U100 series cards

package U10x;
use Card;
use strict;

#constructor
sub new	{
	my ($class) = @_;
	my $self = {
		_is_tdm_api => undef,
		_card      => undef,		
		_tdm_opermode => 'FCC',
		_tdm_law => 'MULAW',
		_rm_network_sync => 'NO',
		_analog_modules => undef,
	};			
	bless $self, $class;
    	return $self;
}

sub card {
	    my ( $self, $card ) = @_;
	        $self->{_card} = $card if defined($card);
		return $self->{_card};
}

sub tdm_opermode {
	            my ( $self, $tdm_opermode ) = @_;
		                    $self->{_tdm_opermode} = $tdm_opermode if defined($tdm_opermode);
				                        return $self->{_tdm_opermode};
}

sub tdm_law {
	            my ( $self, $tdm_law ) = @_;
		                    $self->{_tdm_law} = $tdm_law if defined($tdm_law);
				                        return $self->{_tdm_law};
}

sub rm_network_sync {
	            my ( $self, $rm_network_sync ) = @_;
		                    $self->{_rm_network_sync} = $rm_network_sync if defined($rm_network_sync);
				                        return $self->{_rm_network_sync};
}

sub is_tdm_api {
	            my ( $self, $is_tdm_api ) = @_;
		                    $self->{_is_tdm_api} = $is_tdm_api if defined($is_tdm_api);
				                        return $self->{_is_tdm_api};
}

sub analog_modules {
	            my ( $self, $analog_modules ) = @_;
		                    $self->{_analog_modules} = $analog_modules if defined($analog_modules);
				                        return $self->{_analog_modules};
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

sub gen_wanpipe_conf{
	my ($self, $is_freebsd) = @_;
	my $wanpipe_conf_template = $self->card->current_dir."/templates/wanpipe.tdm.u100";
	my $wanpipe_conf_file = $self->card->current_dir."/".$self->card->cfg_dir."/wanpipe".$self->card->device_no.".conf";
		
	my $device_no = $self->card->device_no;
	my $tdmv_span_no = $self->card->tdmv_span_no;

	my $pci_slot = $self->card->pci_slot;
	my $pci_bus = $self->card->pci_bus;
	my $tdm_law = $self->tdm_law;
	my $tdm_opermode = $self->tdm_opermode;
	my $rm_network_sync = $self->rm_network_sync;
	my $hwec_mode = $self->card->hwec_mode;
	my $hw_dtmf = $self->card->hw_dtmf;
	my $is_tdm_api = $self->is_tdm_api;
	my $tdm_voice_op_mode = "TDM_VOICE";
	

	my $device_alpha = &get_alpha_from_num($device_no);

	if($self->is_tdm_api eq '0') {
		$tdm_voice_op_mode = "TDM_VOICE_API";
	}
	
	open(FH, $wanpipe_conf_template) or die "Can open $wanpipe_conf_template";
	my $wp_file='';
       	while (<FH>) {
       		$wp_file .= $_;
	}
	close (FH);
	open(FH, ">$wanpipe_conf_file") or die "Cant open $wanpipe_conf_file";
        $wp_file =~ s/DEVNUM/$device_no/g;

	if ( $is_freebsd ) {
	        $wp_file =~ s/IFNUM/$device_alpha/g;
	} else {
		$wp_file =~ s/IFNUM/$device_no/g;
	}

	$wp_file =~ s/TDM_VOICE_OP_MODE/$tdm_voice_op_mode/g;
	$wp_file =~ s/SLOTNUM/$pci_slot/g;
	$wp_file =~ s/BUSNUM/$pci_bus/g;
	$wp_file =~ s/TDM_LAW/$tdm_law/g;
	$wp_file =~ s/RMNETSYNC/$rm_network_sync/g;
	$wp_file =~ s/TDM_OPERMODE/$tdm_opermode/g;
	$wp_file =~ s/TDMVSPANNO/$tdmv_span_no/g;
	$wp_file =~ s/HWECMODE/$hwec_mode/g;
	$wp_file =~ s/HWDTMF/$hw_dtmf/g;

	print FH $wp_file;
	close (FH);

# print "\n created $fname for A$card_model $devnum SLOT $slot BUS $bus HWEC $hwec_mode\n";
}
sub gen_zaptel_conf{
	my ($self, $channel, $type) = @_;
	my $dahdi_conf = $self->card->dahdi_conf;
	my $dahdi_echo = $self->card->dahdi_echo;
	my $zp_file='';
	#For USB device type is hardcoded to fxo
	if ( 1 ){
		$zp_file.="fxsks=$channel\n";
	}
	if($dahdi_conf eq 'YES') {
			$zp_file.="echocanceller=" .$dahdi_echo.",".$channel."\n"; 
	}
	return $zp_file;	
}
sub gen_zapata_conf{
	my ($self, $channel, $type) = @_;
	my $dahdi_conf = $self->card->dahdi_conf;
	my $dahdi_echo = $self->card->dahdi_echo;
	my $zp_file='';
	$zp_file.="context=".$self->card->zap_context."\n";
	$zp_file.="group=".$self->card->zap_group."\n";
	if($dahdi_conf eq 'YES') {
			$zp_file.="echocancel=yes\n";
	}
		
	if (1){
	      $zp_file.="signalling = fxs_ks\n";
        }
	$zp_file.="channel => $channel\n\n";
        return $zp_file;
}

1;
