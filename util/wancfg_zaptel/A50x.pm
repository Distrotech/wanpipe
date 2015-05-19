#class A50x
#for A50x-sh series cards

package A50x;
use Card;
use strict;

#constructor
sub new	{
	my ($class) = @_;
	my $self = {
		_current_dir => undef,
		_card      => undef,		
 		_fe_line   => undef,
		_fe_media  => 'BRI',
		_bri_switchtype => 'etsi',
		_bri_country => 'europe',
		_signalling => 'NT',
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

sub fe_media {
	    my ( $self, $fe_media ) = @_;
	        $self->{_fe_media} = $fe_media if defined($fe_media);
		    return $self->{_fe_media};
}


sub bri_switchtype {
	   my ( $self, $bri_switchtype ) = @_;
	        $self->{_bri_switchtype} = $bri_switchtype if defined($bri_switchtype);
		    return $self->{_bri_switchtype};
}


sub current_dir {
           my ( $self, $current_dir ) = @_;
                $self->{_current_dir} = $current_dir if defined($current_dir);
                 return $self->{_current_dir};
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
	my $wanpipe_conf_template = $self->card->current_dir."/templates/wanpipe.tdm_api.a500";
	my $wanpipe_conf_file = $self->card->current_dir."/".$self->card->cfg_dir."/wanpipe".$self->card->device_no.".conf";

	if ($self->card->dahdi_conf eq 'YES') {
		$wanpipe_conf_template = $self->card->current_dir."/templates/wanpipe.tdm.a500";
	}
	my $device_no = $self->card->device_no;
	my $tdmv_span_no = $self->card->tdmv_span_no;
	my $pci_slot = $self->card->pci_slot;
	my $pci_bus = $self->card->pci_bus;
	my $fe_media = $self->fe_media;
	my $fe_line = $self->fe_line;
	my $hwec_mode = $self->card->hwec_mode;
	my $hw_dtmf = $self->card->hw_dtmf;
	my $hw_fax = $self->card->hw_fax;

	my $device_alpha = &get_alpha_from_num($device_no);

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

        $wp_file =~ s/SLOTNUM/$pci_slot/g;
        $wp_file =~ s/BUSNUM/$pci_bus/g;
        $wp_file =~ s/FEMEDIA/$fe_media/g;
        $wp_file =~ s/FELINE/$fe_line/g;
		$wp_file =~ s/TDMVSPANNO/$tdmv_span_no/g;
        $wp_file =~ s/HWECMODE/$hwec_mode/g;
		$wp_file =~ s/HWDTMF/$hw_dtmf/g;
		$wp_file =~ s/HWFAX/$hw_fax/g;
	
	print FH $wp_file;
	close (FH);

}


sub gen_bri_conf{
	my ($self, $span, $type, $group, $country, $operator, $conn_type, $default_tei) = @_;
	my $bri_file='';
	
	$bri_file.="\n";
	$bri_file.="group=$group\n";
	$bri_file.="country=$country\n";
	$bri_file.="operator=$operator\n";
	$bri_file.="connection_type=$conn_type\n";
	

	if ( $type eq 'bri_nt') {
		$bri_file.="signalling=bri_nt\n";
	} else {
		$bri_file.="signalling=bri_te\n";
	}

	if ( ! $default_tei eq ''){
		$bri_file.="default_tei=$default_tei\n";
		
	}
	$bri_file.="spans=$span\n";

	return $bri_file;	
}


sub gen_woomera_conf{
	my ($self, $group, $context) = @_;
	my $woomera_file='';
	
	$woomera_file.="\n";
	$woomera_file.="context=$context\n";
	$woomera_file.="group=$group\n";
	return $woomera_file;	
}

sub gen_zaptel_conf{
	my ($self) = @_;
	my $zp_file='';
	my $tmpchan=1;
	my $tmpstr='';
	my $hwec_mode = $self->card->hwec_mode;

	$zp_file.="\n\#Sangoma AFT-B".$self->card->card_model." port ".$self->fe_line." [slot:".$self->card->pci_slot." bus:".$self->card->pci_bus." span:".$self->card->tdmv_span_no."] <wanpipe".$self->card->device_no.">\n";
        $zp_file.="span=".$self->card->tdmv_span_no.",0,0,ccs,ami\n";
	
	$tmpchan = $self->card->first_chan+1;
	$zp_file.="bchan=".$self->card->first_chan.",".$tmpchan."\n";

	$tmpchan = $self->card->first_chan+1;
	$tmpstr.="echocanceller=".$self->card->dahdi_echo.",".$self->card->first_chan."-".$tmpchan."\n";
	if ($self->card->dahdi_echo eq 'NO') {
		$zp_file.="#";
	}
	$zp_file.=$tmpstr;
	$tmpchan=$self->card->first_chan+2;
	$zp_file.="hardhdlc=".$tmpchan."\n";
	return $zp_file;
}

sub gen_zapata_conf{
	my ($self) = @_;
	my $zp_file='';
	my $tmpchan=1;

	$zp_file.="\n\;Sangoma AFT-B".$self->card->card_model." port ".$self->fe_line." [slot:".$self->card->pci_slot." bus:".$self->card->pci_bus." span:".$self->card->tdmv_span_no."] <wanpipe".$self->card->device_no.">\n";

#        $zp_file.="switchtype=".$self->bri_switchtype."\n";
        $zp_file.="context=".$self->card->zap_context."\n";
        $zp_file.="group=".$self->card->zap_group."\n";

        if ($self->card->dahdi_echo eq 'NO') {
                $zp_file.="echocancel=no\n";
        } else {
                $zp_file.="echocancel=yes\n";
	}

        if ( $self->signalling eq 'NT' ){
                $zp_file.="signalling=bri_net\n";
        } elsif ( $self->signalling eq 'TEM' ){
                $zp_file.="signalling=bri_cpe_ptmp\n";
        } else {
                $zp_file.="signalling=bri_cpe\n";
        }

        $tmpchan = $self->card->first_chan+1;
        $zp_file.="channel =>".$self->card->first_chan."-".$tmpchan."\n";

	return $zp_file;
}
sub signalling {
            my ( $self, $signalling ) = @_;
            $self->{_signalling} = $signalling if defined($signalling);
            return $self->{_signalling};
}


1;
