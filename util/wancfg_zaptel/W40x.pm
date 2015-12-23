#class W240x
#for W400 series cards

package W40x;
use Card;
use strict;

#constructor
sub new	{
	my ($class) = @_;
	my $self = {
		_is_tdm_api => undef,
		_card      => undef,
		_tdm_law => 'ALAW',
		_fe_line   => undef,
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

sub tdm_law {
	my ( $self, $tdm_law ) = @_;
	$self->{_tdm_law} = $tdm_law if defined($tdm_law);
	return $self->{_tdm_law};
}

sub is_tdm_api {
	my ( $self, $is_tdm_api ) = @_;
	$self->{_is_tdm_api} = $is_tdm_api if defined($is_tdm_api);
	return $self->{_is_tdm_api};
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

sub gen_wanpipe_conf{
	my ($self, $is_freebsd) = @_;
	my $wanpipe_conf_template = $self->card->current_dir."/templates/wanpipe.tdm.w400";
	my $wanpipe_conf_file = $self->card->current_dir."/".$self->card->cfg_dir."/wanpipe".$self->card->device_no.".conf";
		
	my $device_no = $self->card->device_no;
	my $tdmv_span_no = $self->card->tdmv_span_no;

	my $pci_slot = $self->card->pci_slot;
	my $pci_bus = $self->card->pci_bus;
	my $tdm_law = $self->tdm_law;
	my $is_tdm_api = $self->is_tdm_api;
	my $tdm_voice_op_mode = "TDM_VOICE";
	my $fe_line = $self->fe_line;
	my $tdmv_span_no = $self->card->tdmv_span_no;

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

	$wp_file =~ s/IFNUM/$device_no/g;

	$wp_file =~ s/FELINE/$fe_line/g;

	$wp_file =~ s/TDM_VOICE_OP_MODE/$tdm_voice_op_mode/g;
	$wp_file =~ s/SLOTNUM/$pci_slot/g;
	$wp_file =~ s/BUSNUM/$pci_bus/g;
	$wp_file =~ s/TDM_LAW/$tdm_law/g;
	$wp_file =~ s/TDMVSPANNO/$tdmv_span_no/g;

	print FH $wp_file;
	close (FH);

# print "\n created $fname for A$card_model $devnum SLOT $slot BUS $bus HWEC $hwec_mode\n";
}

sub gen_zaptel_conf
{
	my ($self) = @_;
	my $zp_file='';
	my $tmpstr='';
	my $tmpchan=1;

	$zp_file.="\n\#Sangoma AFT-W".$self->card->card_model." port ".$self->fe_line." [slot:".$self->card->pci_slot." bus:".$self->card->pci_bus." span:".$self->card->tdmv_span_no."] <wanpipe".$self->card->device_no.">\n";
	
	$zp_file.="bchan=".$self->card->first_chan."\n";

	$tmpstr.="echocanceller=mg2,".$self->card->first_chan."\n";
	if ($self->card->dahdi_echo eq 'NO') {
		$zp_file.="#";
	}
	$zp_file.=$tmpstr;
	$tmpchan=$self->card->first_chan+1;
	$zp_file.="hardhdlc=".$tmpchan."\n";
	return $zp_file;
}

sub gen_zapata_conf
{
	my ($self) = @_;
	my $zp_file='';
	my $tmpchan=1;
	$zp_file.="\n\;Sangoma AFT-W".$self->card->card_model." port ".$self->fe_line." [slot:".$self->card->pci_slot." bus:".$self->card->pci_bus." span:".$self->card->tdmv_span_no."] <wanpipe".$self->card->device_no.">\n";

	$zp_file.="context = ".$self->card->zap_context."\n";
	$zp_file.="group = ".$self->card->zap_group."\n";

	if ($self->card->dahdi_echo eq 'NO') {
		$zp_file.="echocancel = no\n";
		$zp_file.="echocancelwhenbridged = no\n";
	} else {
		$zp_file.="echocancel = yes\n";
		$zp_file.="echocancelwhenbridged = yes\n";
	}

	$zp_file.="signalling = gsm\n";
	$zp_file.="wat_moduletype = telit\n";

	$zp_file.="channel => ".$self->card->first_chan."\n";

	return $zp_file;
}

1;
