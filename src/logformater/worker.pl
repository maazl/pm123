use strict;

# human readable object names by address
my %omap;

# current pending RequestInfo calls per TID
my @reqstack;

# current request state by priority
my %reqflags;

# 00001376 01e7:0005 0284fba0 APlayable(0x3b1648{test2})::RequestInfo(418, 0, 2)
#               $tid-                     --$o--                      $wha $p $r
sub req($$$$$)
{	my ($tid, $o, $what, $pri, $rel) = @_;
	$reqstack[$tid] = [] unless exists $reqstack[$tid];
	push @{$reqstack[$tid]}, [$o, $what, $pri, $rel];
}

# 00001376 01e7:0005 0284fb94 APlayable::RequestInfo rq = 18, async = 0
#               $tid                                      $rq         $async
sub req2($$$)
{	my ($tid, $rq, $async) = @_;
	exists $reqstack[$tid] && @{$reqstack[$tid]} or warn "req2 without req in thread $tid", return;
	my ($o, $what, $pri, $rel) = @{pop @{$reqstack[$tid]}};
	printf "Req: %s, what=%x pri=%u rel=%u, rq=%x async=%x\n", $o, $what, $pri, $rel, $rq, $async;
	return unless $rq;
	return unless $pri;
	exists $reqflags{$o} && $reqflags{$o}[$pri] || $async
		or warn sprintf "first request at priority %i did not cause an asynchronous request (%x,%x)", $reqflags{$o}[0], $reqflags{$o}[1];
	$reqflags{$o}[$pri>=2] |= $rq;
	$reqflags{$o}[0] &= ~$rq if $pri >= 2;
	#$reqflags{$o}[2+($pri>=2)] |= $rq;
	#$reqflags{$o}[2] &= ~$rq if $pri >= 2;
}

# 00001376 01e7:0005 0284fd58 Playable(0x398ca8{URLMRU})::DoLoadInfo({2,})
#                                        --$o--                       $pri
sub load($$)
{	my ($o, $pri) = @_;
	my $rq = $reqflags{$o}[2+($pri>=2)];
	printf "Load: %s, %i - %x, %x\n", $o, $pri, $reqflags{$o}[2], $reqflags{$o}[3];
	$reqflags{$o}[2+($pri>=2)] = 0;
}

sub event($$$$)
{	my ($o, $load, $chg, $inval) = @_;
	printf "Event: %s, %x, %x, %x - %x, %x\n", $o, $load, $chg, $inval, $reqflags{$o}[0], $reqflags{$o}[1];
	$reqflags{$o}[0] &= ~$load;
	$reqflags{$o}[1] &= ~$load;
}

while (<>)
{	chomp;
	/0x([0-9a-fA-F]+)\{([^}]+)\}/ and $omap{$1} = $2; 
	/\(0x([0-9a-fA-F]+).*\)::DoLoadInfo\(\{(\d),/ and load $1, $2;
	/[ :](\d{4}) .*APlayable\(0x([0-9a-fA-F]+).*\)::Request(?:Aggregate)?Info\(([0-9a-fA-F]+),\s*(\d),\s*(\d)\)/ and req $1, $2, hex $3, $4, $5;
	/[ :](\d{4}) .*APlayable::Request(?:Aggregate)?Info.*rq\s*=\s*([0-9a-fA-F]+),\s*async\s*=\s*([0-9a-fA-F]+)/ and req2 $1, hex $2, hex $3;
	/APlayable\(0x([0-9a-fA-F]+).*\)::RaiseInfoChange\(&?\{&?0x([0-9a-fA-F]+),[^,]*,\s*([0-9a-fA-F]+),\s*([0-9a-fA-F]+),\s*([0-9a-fA-F]+)\s*\}\)/ and event $1, hex $3, hex $4, hex $5; 
}

while (my ($o, $d) = each %reqflags)
{	my ($lreq, $hreq, $ljob, $hjob) = @$d;
	if ($hjob)
	{	warn sprintf "Object %s\{%s\} has outstanding high priority worker %x\n", $o, $omap{$o}, $hjob;
	} elsif ($hreq)
	{	warn sprintf "Object %s\{%s\} has incomplete high priority request %x\n", $o, $omap{$o}, $hreq; 
	}
	if ($ljob)
	{	warn sprintf "Object %s\{%s\} has outstanding low priority worker %x\n", $o, $omap{$o}, $ljob;
	} elsif ($lreq) 
	{	warn sprintf "Object %s\{%s\} has incomplete low priority request %x\n", $o, $omap{$o}, $lreq;
	} 
}

