#!/opt/perl5/bin/perl
use strict;
use Socket;
use FileHandle;
my ($remote, $port, $iaddr, $paddr, $proto, $line, $done, $cmd);

$remote = 'localhost';
$port = 22763;

$iaddr = inet_aton($remote) or die "no host: $remote";
$paddr = sockaddr_in($port, $iaddr);

$proto = getprotobyname('tcp');

socket(SOCK, PF_INET, SOCK_STREAM, $proto) or die "socket: $!";
connect(SOCK, $paddr)  or die "connect: $!";

autoflush SOCK 1;

while(!$done) {
    print "Cmd: ";
    $cmd = <>;
    chomp $cmd;
    print SOCK $cmd;
    while ( ($line = <SOCK>) && !($line =~ /\<\<\<DONE\>\>\>/ )) {
	print $line;
    }
}

close (SOCK) or die "close: $!";
exit;
