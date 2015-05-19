#!/usr/bin/perl -w
use Socket;
#use strict;

#Global Variables
my $debug = 0;
my $localip= '192.168.1.10';
my $remoteip= '192.168.1.10';
my $numCalls = 0;

sub debug_event{
	if ($debug == 1){
		my ($debugs) = @_;
		print "$debugs";
	} 
}

#make_call("1","100","101","from-pstn","4741992","Sangoma");
sub make_call{
	my($group, $remote_extension, $local_extension, $context, $callerid, $callerid_name ) = @_;	
#	if ($debug == 1){ 
#		print "group = $group\n";
#		print "remote_extension = $remote_extension\n";
#		print "local_extension = $local_extension\n";
#		print "context = $context\n";
#		print "callerid = $callerid\n";
#		print "callerid_name = $callerid_name\n";
#	}

	my $wp_file="";
	open(FH, 'sample.call.template') or die "Can't open call template";
	while (<FH>) {
		$wp_file .= $_;
	}
	close (FH);

	my $fname = "/var/spool/asterisk/outgoing/call-g".$group."-".$remote_extension.".call";
	open (FH, ">$fname") or die "Can't open $fname";

	$wp_file =~ s/GROUP/g1/g;
	$wp_file =~ s/REMOTE_EXT/$remote_extension/g;
	$wp_file =~ s/LOCAL_EXT/$local_extension/g;
	$wp_file =~ s/CALLERIDNAME/$callerid_name/g;
	$wp_file =~ s/CALLERID/$callerid/g;
	$wp_file =~ s/CONTEXT/$context/g;
	$wp_file =~ s/CALLERIDNAME/$callerid_name/g;

	print FH $wp_file;
	close (FH);
#	sleep 1;
#	unlink $fname;
		
}

sub check_call{
	 
	my($group, $remote_extension, $local_extension, $context, $callerid, $callerid_name ) = @_;	
	if ($debug == 1){
		print "group = $group\n";
		print "remote_extension = $remote_extension\n";
		print "local_extension = $local_extension\n";
		print "context = $context\n";
		print "callerid = $callerid\n";
		print "callerid_name = $callerid_name\n";
	}

}


#open socket and wait for acknowledgement from other side
sub wait_ack {
	my($port, $group) = @_;

	socket(my $SERVER, PF_INET, SOCK_STREAM, getprotobyname('tcp')) or die "socket: $!";
	setsockopt($SERVER, SOL_SOCKET, SO_REUSEADDR, 1) or die "setsock: $!";

	my $paddr = sockaddr_in($port, INADDR_ANY);

	bind( $SERVER, $paddr) or die "bind: $!";
	listen($SERVER, SOMAXCONN) or die "listen: $!";
	debug_event("SERVER started on port $port\n");

	my $client_addr =  accept(CLIENT,$SERVER); 
	debug_event("waiting for ack from hostR\n");
	my $answer = <CLIENT>; 
	if ($answer == $group){
		debug_event("All tests on channel $group successful\n");
		return 1; 
	}else{
		print "Error: call sent on channel $group , received on channel $answer \n";	
		return -1;
	}
	debug_event("Closing connection\n");
	close CLIENT;
}

#make_call("1","999","100","from-analog","4741992","Sangoma");
#$rnd=rand()&0xff00;

my $j = 0;
while ($j == 0){
	for (my $i = 1; $i < 2; $i++){
		make_call($i,"100",100+$i,"sangoma","4741990","Sangoma"); 
		$numCalls++;
		print "number of calls: $numCalls \n";
	}
	sleep 5;
	system("rm -rf /var/spool/asterisk/outgoing/*");
	last;
#	for (my $i = 1; $i < 5; $i++){
#		wait_ack("9000","$i");
#	}
}
exit;

