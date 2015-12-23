#class boostbrispan
#for freeswitch isdn bri/pri boost span configuraton 

package boostspan;
#use warnings;
#use diagnostics;
use strict;

#constructor
sub new	{
	my ($class) = @_;
	my $self = {
		#openzap conf
		_span_type => undef,
		_span_name => 'OpenZap',
		_span_no   => undef,
		_chan_no   => undef,
		#signalling conf
		_trunk_type => undef,
		_group_no => undef,
		_chan_set => undef,
		_sig_mode => undef,		
		_context => undef,
	};			
	bless $self, $class;
    	return $self;
}

sub span_type {
	            my ( $self, $span_type ) = @_;
		                    $self->{_span_type} = $span_type if defined($span_type);
				                        return $self->{_span_type};
}

sub span_no {
	            my ( $self, $span_no ) = @_;
		                    $self->{_span_no} = $span_no if defined($span_no);
				                        return $self->{_span_no};
}
sub span_name {
	            my ( $self, $span_name ) = @_;
		                    $self->{_span_name} = $span_name if defined($span_name);
				                        return $self->{_span_name};
}
sub chan_no {
	            my ( $self, $chan_no ) = @_;
		                    $self->{_chan_no} = $chan_no if defined($chan_no);
				                        return $self->{_chan_no};
}
sub trunk_type {
	            my ( $self, $trunk_type ) = @_;
		                    $self->{_trunk_type} = $trunk_type if defined($trunk_type);
				                        return $self->{_trunk_type};
}
sub group_no {
	            my ( $self, $group_no ) = @_;
		                    $self->{_group_no} = $group_no if defined($group_no);
				                        return $self->{_group_no};
}
sub switch_type {
	            my ( $self, $switch_type ) = @_;
		                    $self->{_switch_type} = $switch_type if defined($switch_type);
				                        return $self->{_switch_type};
}

sub chan_set {
	            my ( $self, $chan_set ) = @_;
		                    $self->{_chan_set} = $chan_set if defined($chan_set);
				                        return $self->{_chan_set};
}

sub sig_mode {
	            my ( $self, $sig_mode ) = @_;
		                    $self->{_sig_mode} = $sig_mode if defined($sig_mode);
				                        return $self->{_sig_mode};
}

sub context {
	            my ( $self, $context ) = @_;
		                    $self->{_context} = $context if defined($context);
				                        return $self->{_context};
}

sub print {
    my ($self) = @_;
    printf (" span_name: %s\n span_type: %s\n span_no: %s\n chan_no:  %s \ntrunk type: %s \ngroup_no:  %s\n switchtype: %s\n chan_set: %s\n sig_mode:%s\n context:%s\n", $self->span_name, $self->span_type, $self->span_no, $self->chan_no, $self->trunk_type,$self->group_no,$self->switch_type,$self->chan_set,$self->sig_mode,$self->context);

}

1;
