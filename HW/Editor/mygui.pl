#!/opt/perl5/bin/perl

use Tk;
#use Tk ':eventtypes';
use Socket;
use FileHandle;
my ($remote, $port, $iaddr, $paddr, $proto, $line, $done, $cmd);
my $buttonDown = 0;
use subs qw/file_menuitems edit_menuitems create_menuitems help_menuitems/;
use subs qw/CreateObject my_exit SendMsg DisplayProps ModifyProp ResizeWindow /;
use subs qw/CheckAsyncEvents UpdateObjectList ObjectSelected UpdateMouseMode/;
my $winH=1, $winW=1, $lastW=0, $lastH=0;


sub mysleep
{
  my($t) = @_;
#  printf("Sleeping for: %f\n", $t);
  select undef, undef, undef, $t;
}


sub ButtonPress
{
    return;
    my($widget) = @_;
    my $e = $widget->XEvent;
    printf("Button %d Pressed: %d %d \n", $e->b, $e->x, $e->y);
    $buttonDown = $e->b;
    $x = $e->x;
    $y = $e->y;
    $b = $e->b;
    SendMsg "ButtonDown $x $y $b 0\n\0";
}

sub ButtonRelease
{
    return;
    my($widget) = @_;
    my $e = $widget->XEvent;
    printf("Button %d Released: %d %d \n", $e->b, $e->x, $e->y);
    $buttonDown = 0;
    $x = $e->x;
    $y = $e->y;
    $b = $e->b;
    SendMsg "ButtonUp $x $y $b 0\n\0";
}

sub Motion
{
    return;
    my($widget) = @_;
    my $e = $widget->XEvent;
    if( $buttonDown) {
	$x = $e->x;
	$y = $e->y;
	SendMsg "Motion $x $y 0\n\0";
	printf("Button %d Motion: %d %d \n", $e->b, $e->x, $e->y);
    }
}

sub ResizeWindow
{
    my($widget) = @_;
    if( $widget =~ /Canvas/ )  {
	my $e = $widget->XEvent;
	if( ($e->h != $lastH) || ($e->w != $lastW) )
	{
	    $winH = $e->h;
	    $winW = $e->w;
	    $needToSendResize = 1;
	}
    }
}

sub SetupSocket
{
    $remote = 'localhost';
    $port = 22763;

    $iaddr = inet_aton($remote) or die "no host: $remote";
    $paddr = sockaddr_in($port, $iaddr);

    $proto = getprotobyname('tcp');

    socket(SOCK, PF_INET, SOCK_STREAM, $proto) or die "socket: $!";
    connect(SOCK, $paddr)  or die "connect: $!";

    autoflush SOCK 1;
    print "SetupSocket complete. Sock = ", SOCK, "\n";
}

sub SendMsg
{
    my($msg) = @_;
    $mw->update;
    print "Sending: $msg\n";
    print SOCK $msg;
    if( !( $msg =~ /Quit/ )) {
#	mysleep(0.1);
	$reply = <SOCK>;
	while (! ($reply =~ /DONE/) ) {
	    $reply .= <SOCK>;
	    $reply =~ s/\n/ /g;
	}
    }
    $reply =~ s/\0//g;
    $reply =~ s/<<<DONE>>>//g;
    print(" Reply: $reply\n");
    return $reply;
}

$mw = MainWindow->new;
$mw->title("MainWindow");
$mw->geometry("500x500+0+0");

$grCanvas = $mw->Canvas()->form( -left=>205, -right=>'%100', -top=>'%40', -bottom=>'%100');

#printf("Frame WindowID: %s\n", $grCanvas->id);

$id = $grCanvas->id;
#printf("id = %s\n", $id);

$mw->configure(-menu=> my $menubar = $mw->Menu);

map {$menubar->cascade( -label =>'~'.$_->[0], -menuitems=> $_->[1])}
    ['File', file_menuitems],
    ['Edit', edit_menuitems],
    ['Create', create_menuitems],
    ['Help', edit_menuitems];

$mw->update;
mysleep(1.0);

$os = $^O;
printf("Running on: %s\n", $os);

$pid = fork;
if($pid == 0) {  #Child 
    if( $os =~ /MSWin32/ ) {
	print "Windows!\n";
        `./hwedit.exe -windowid $id -width 512 -height 512`;
    }
    else {
	print "HP-UX!\n";
        `./hwedit -windowid $id > /tmp/hwedit.log 2>&1 `;
    }
    exit;
}
printf("Running hwedit -windowid $id \n");
#printf("Quick! Run hwedit -windowid $id \n");

#mysleep(10);
mysleep(1);

SetupSocket();

mysleep(1);

$mw->bind('<Configure>' => \&ResizeWindow );


$mw->repeat( 2000 => \&CheckAsyncEvents);

$mw->Label(-text=>"Property")->place(-x=>20, -y=>0);
$mw->Label(-text=>"Value")->place(-x=>100, -y=>0);
$mw->Label(-text=>"Objects")->place(-x=>250, -y=>0);

$objectList = $mw->Scrolled("Listbox", -width => 20 )->place(
    -x=>205, -y=>20, -width=>150, -relheight=>0.30);

$objectList->bind('<ButtonRelease-1>' => \&ObjectSelected );

$mouseView = $mw->Button(-text=>"Mouse View", 
     -command=>[ \&UpdateMouseMode, "MouseView"])->place( -x=>405, -y=>20);

$mouseSelect = $mw->Button(-text=>"Mouse Select", 
     -command=>[ \&UpdateMouseMode, "MouseSelect"])->place( -x=>485, -y=>20);

$mw->update;
DisplayProps("Nothing");

#printf("Now running the MainLoop\n");
MainLoop;


sub file_menuitems
{
 [
   ['command', 'Exit', -command => \&my_exit ],
 ];
}

sub edit_menuitems
{
 [
   ['command', 'Exit', -command => \&my_exit ],
 ];
}

sub CreateObject
{
    my($object) =  @_;
    
    $objectnum++;

    SendMsg "NewNamedObject $object $object$objectnum\n";
$mw->update;
    SendMsg "SetProp Color {1,0,0}\n";
$mw->update;
    SendMsg "SetProp Radius 40\n";
$mw->update;
    SendMsg "MouseView\n";
$mw->update;
    SendMsg "SetProp Shininess 1.0\n";
$mw->update;
    SendMsg "SetProp GraphM 40\n";
$mw->update;
    SendMsg "SetProp GraphN 40\n";
$mw->update;
    $selected = SendMsg "ListSelected\n";
    @newobj = split /\s/, $selected;
    print "newobj: $newobj[0]";
    $newobj[0] =~ s/\0//g;
    $newProps = (SendMsg "ListProps $newobj[0]\0");
    DisplayProps( $newProps);
    UpdateObjectList;
    $currentlySelected = $newobj[0];
}

sub create_menuitems
{
    [
      map ['command', $_, -command => [ \&CreateObject, $_] ],
	qw/hwSphere hwTorus hwPolygon/,
    ];
}

sub help_menuitems
{
 [
   ['command', 'Exit', -command => \&my_exit ],
 ];
}

sub my_exit
{
    if( $^O =~ /MSWin32/ ) {
	$mw->destroy();
	print "Detroyed main window\n";
    }
    mysleep(1);
    SendMsg "Quit\n";
    mysleep(1);
    print "Quitting!\n";
    exit;
}

sub DisplayProps 
{
   my($in) = @_;

   $in =~ s/\0//g;
   print "DisplayProps: $in";
   @props = split '\s', $in;

   if( defined $t ) {
       $t->destroy;
   }

   $t = $mw->Scrolled("Text", -width => 40, -wrap => 'none' )->place(
	-x=>0, -y=>20, -width=>200, -relheight=>0.80);

   $i=1;
   foreach ( @props ) {
     $prop = $_;
     print "$prop \n";
     $w = $t->Label(-text=>"$_ :", -relief => 'groove', -width=> 20);
     $w = $t->Label(-text=>'$prop' , -relief => 'groove', -width=> 12) ;
     $w->configure(-text=>" $prop");
     $t->windowCreate('end', -window=>$w);
     $w = $t->Entry(-width=>20, -textvariable=>\$props{$_}, 
           -validatecommand => [\&ModifyProp, $_], -validate=>'key');
     $propVal = SendMsg("GetProp $_\n");
     $propVal =~ s/\0//g;
     $props{$_} = $propVal;
     $t->windowCreate('end', -window=>$w);
     $t->insert('end', "\n");
     print '$props{$prop} = ', "$props{$prop}";
   }
}


sub ModifyProp
{
    my($prop, $proposed, $change, $current, $indx, $type) = @_;

    #print "Parameters: @_  \n";
    #print "\$prop: $prop  \$proposed: $proposed  \$change: $change \n";
    #print "\$current: $current  \$indx: $indx  \$type: $type\n";
    # print "Modifying $prop to $props{$prop} \n";
    if( $type != -1 ) {
	SendMsg("SetProp $prop $proposed\n");
    }
    return(1);
}

sub CheckAsyncEvents
{
    #print "CheckAsyncEvents \n";
    if($needToSendResize)  {
         print "Need to resize: winW $winW winH $winH lastW $lastW lastH $lastH";
	 $lastW = $winW;
	 $lastH = $winH;
	 $needToSendResize = 0;
	 SendMsg "WindowResize $winW $winH\n";
    }
    $mw->update;
    if($mouseMode =~ /MouseSelect/ ) {
        $selected = SendMsg("ListSelected\n");
	$selected =~ s/\W//g;
	if( $selected ne $currentlySelected ) {
	    print "\$selected = $selected  \$currentlySelected = $currentlySelected \n";
	    print "Why don't they match?\n";
	    $currentlySelected = $selected;
	    $newProps = (SendMsg "ListProps $selected\0");
	    DisplayProps( $newProps);
	}
    }
}

sub UpdateObjectList
{
    $numObjects = $objectList->size();
    $objectList->delete(0,'end');
    @objs = split '\s', SendMsg("ListObjects\n");
    $objectList->insert('end', @objs);
}

sub ObjectSelected
{
    $sel = $objectList->get('active');
    SendMsg("UnselectAll\n");
    SendMsg("SelectObject $sel\n");
    $newProps = (SendMsg "ListProps $sel\0");
    DisplayProps( $newProps);
    $currentlySelected = $sel;
}

sub UpdateMouseMode
{
    print "UpdateMouseMode Args:  @_ \n";
    my($mode) = @_;
    print "UpdateMouseMode: $mode\n";
    SendMsg("$mode");
    $mouseMode = $mode;
}
