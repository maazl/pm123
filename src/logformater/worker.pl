use strict;

# human readable object names by address
my %omap;

# current pending RequestInfo calls per TID
my @reqstack;

# current request state by object address and priority
my %reqflags;

# required workers by object address and priority
my %worker;

# dependency workers by address
my %dependencyworker;

# dependecy workers by dependant object adress
my %dependencyindex;

sub pri_ix($)
{	return ($_[0] & 3) < 2;
}

# splits dependency set dump into a dictionary object -> flags
sub parse_set($)
{	my @parts = split /,\s+/, shift;
	return {} if @parts <= 1 && $parts[0] !~ /\S/;
	return { map
	{	/0x([0-9a-fA-F]+)(?:\{[^\}]*\})?\s*:\s*(\d+)/;
		($1, hex $2);
	} @parts};
}

sub dump_set(\%)
{	my @parts;
	while (my ($o, $flags) = each %{$_[0]})
	{	push @parts, sprintf '%s:%x', $o, $flags;
	}
	return join ', ', @parts;
}

# 00001376 01e7:0005 0284fba0 APlayable(0x3b1648{test2})::RequestInfo(418, 0, 2)
#               $tid                      --$o--                      $wha $p $r
sub req($$$$$)
{	my ($tid, $o, $what, $pri, $rel) = @_;
	$reqstack[$tid] = [] unless exists $reqstack[$tid];
	push @{$reqstack[$tid]}, [$o, $what, $pri, $rel];
}

# 00001376 01e7:0005 0284fb94 APlayable::RequestInfo rq = 18, async = 0
#               $tid                                      $rq         $async
sub req2($$$)
{	my ($tid, $rq, $async) = @_;
	exists $reqstack[$tid] && @{$reqstack[$tid]} or warn "$. req2 without req in thread $tid\nnn", return;
	my ($o, $what, $pri, $rel) = @{pop @{$reqstack[$tid]}};
	$o or warn "$. Invalid request $tid, $rq, $async\n";
	printf "$. Req: %s, what=%x pri=%u rel=%u rq=%x async=%x\n", $o, $what, $pri, $rel, $rq, $async;
	return unless $rq;
	return unless $pri;
	my $ix = pri_ix $pri;
	exists $reqflags{$o} && ($reqflags{$o}[$ix] || $reqflags{$o}[1]) || $async or
 		warn sprintf "$. first request for %s at priority %i did not cause an asynchronous request (%x,%x)\n", $o, $pri, $reqflags{$o}[0], $reqflags{$o}[1];
	$reqflags{$o}[$ix] |= $rq;
	$reqflags{$o}[0] &= ~$rq if $ix;
	#$reqflags{$o}[2+$ix] |= $rq;
	#$reqflags{$o}[2] &= ~$rq if $ix;
	$worker{$o}[$ix] .= "$. " if $async;
}

# 00000045 0073:0006 0289fd58 APlayable(0x3a5508{Shock.flac})::HandleRequest(6)
#                                         --$o--                             $pri
sub handle($$)
{	my ($o, $pri) = @_;
	printf "$. Handle: %s, pri=%u isclow=%x ischigh=%x\n", $o, $pri, $reqflags{$o}[2], $reqflags{$o}[3];            

	my $ix = pri_ix $pri;
	#$worker{$o}[$ix] || $worker{$o}[0] or
	#	warn sprintf "$. Unrequested handler for %s at priority %u\n", $o, $pri;
	$worker{$o}[0] = ' '; # That's logically zero but does not cause the above check to fail.
	$worker{$o}[$ix] = undef;

	#my $rq = $reqflags{$o}[2+(pri_ix $pri)];
	#$reqflags{$o}[2+(pri_ix $pri)] = 0;
}

# 00000029 0073:0004 0284fd24 APlayable(0x3a8bc8)::RaiseInfoChange({&0x3a8bc8, 0x3a8bc8, 8f,  87, 0})
#                                         --$o--                                         $load$chg$inval                
sub event($$$$)
{	my ($o, $load, $chg, $inval) = @_;
	printf "$. Event: %s, %x, %x, %x - %x, %x\n", $o, $load, $chg, $inval, $reqflags{$o}[0], $reqflags{$o}[1];
	$reqflags{$o}[0] &= ~$load;
	$reqflags{$o}[1] &= ~$load;

	# check dependecies
	return unless $load;
	my $deps = $dependencyindex{$o} or return;
	my @remove;
	while (my ($w, $flags) = each %$deps)
	{	$$deps{$w} &= ~$load and next;
		push @remove, $w;
	}
	foreach my $w (@remove)
	{	delete $$deps{$w};
		my $wk = $dependencyworker{$w} or
			warn(sprintf "$. Internal dependency index inconsistent worker=%s\n", $w), next;
		delete $$wk[0]{$o} & ~$load and
			warn sprintf "$. Internal dependency index inconsistent worker=%s obj=%s\n", $w, $o;  
		next unless exists $$wk[1]{$o};
		# discard optional set
		delete $dependencyworker{$w}{$_} foreach keys %{$$wk[1]};
		$$wk[1] = {};
	}
}

# 00000089 0073:0004 0284fec4 RescheduleWorker(0x3ab0c8)::RescheduleWorker(&0x284ff4c{MandatorySet = 0x3aa728{@1:Shock} : 300, 0x3aaea8{@2:Shock} : 300; OptionalSet = 0x3ab088{@3:Shock} : 300}, &0x3a5508{Shock.flac}, 1)
#                                                --$w--                                              $mand---------------------------------------------                $opt--------------------      --$o--              $pri
sub reschedule($$$$$)
{	my ($w, $mand, $opt, $o, $pri) = @_;
	$mand = parse_set $mand;
	$opt = parse_set $opt;
	printf "$. Dependency %s: %s (%u): M = {%s}, O = {%s}\n", $w, $o, $pri, dump_set(%$mand), dump_set(%$opt);
	$dependencyworker{$w} = [$mand, $opt, $o, $pri];
	while (my ($dep, $flags) = each %$mand)
	{	exists $dependencyindex{$dep}{$w} and
			warn sprintf "$. Duplicate mandatory dependenncy of worker %s on %s (%x)\n", $w, $dep, $flags;
		$dependencyindex{$dep}{$w} = $flags;
	} 
	while (my ($dep, $flags) = each %$opt)
	{	exists $dependencyindex{$dep}{$w} and
			warn sprintf "$. Duplicate optional dependenncy of worker %s on %s (%x)\n", $w, $dep, $flags;
		$dependencyindex{$dep}{$w} = $flags;
	} 
}

# 284fa10 RescheduleWorker(0x3adfe8)::~RescheduleWorker()
#                            --$w--
sub noreschedule($)
{	my ($w) = @_;
	my $val = delete $dependencyworker{$w}
		or warn sprintf "$. Deleting inexistent dependency worker %s\n", $w;
	printf "$. Dependency %s done: %s (%u)\n", $w, $$val[2], $$val[3];
	$worker{$$val[2]}[pri_ix $$val[3]] .= "$. ";
}


while (<>)
{	chomp;
	/0x([0-9a-fA-F]+)\{([^}]+)\}/ and $omap{$1} = $2; 
	/[ :](\d{4}) .*APlayable\(0x([0-9a-fA-F]+).*\)::Request(?:Aggregate)?Info\(([0-9a-fA-F]+),\s*(\d),\s*(\d)\)/ and req $1, $2, hex $3, $4, $5;
	/[ :](\d{4}) .*APlayable::Request(?:Aggregate)?Info.*rq\s*=\s*([0-9a-fA-F]+),\s*async\s*=\s*([0-9a-fA-F]+)/ and req2 $1, hex $2, hex $3;
	/APlayable\(0x([0-9a-fA-F]+).*\)::RaiseInfoChange\(&?\{&?0x([0-9a-fA-F]+),[^,]*,\s*([0-9a-fA-F]+),\s*([0-9a-fA-F]+),\s*([0-9a-fA-F]+)\s*\}\)/ and event $1, hex $3, hex $4, hex $5;
	/APlayable\(0x([0-9a-fA-F]+).*\)::HandleRequest\((\d)\)/ and handle $1, $2;
	/RescheduleWorker\(0x([0-9a-fA-F]+)\)::RescheduleWorker\(.*{MandatorySet\s*=\s*(.*);\s*OptionalSet\s*=\s*(.*)},\s*&0x([0-9a-fA-F]+)(?:{[^}]*})?,\s*(\d)/ and reschedule $1, $2, $3, $4, $5; 
	/RescheduleWorker\(0x([0-9a-fA-F]+)\)::~RescheduleWorker/ and noreschedule $1; 
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

while (my ($o, $d) = each %worker)
{	$$d[0] =~ /\S/ and warn sprintf "Missing worker for %s high priority requested at %s\n", $o, $$d[0];
	$$d[1] =~ /\S/ and warn sprintf "Missing worker for %s low priority requested at %s\n", $o, $$d[1];
}

while (my ($w, $dep) = each %dependencyworker)
{	my ($mand, $opt, $o, $pri) = @$dep;
	if (%$mand || %$opt)
	{	warn sprintf "Uncompleted dependency worker %s for %s pri=%u: M = {%s}, O = {%s}\n", $w, $o, $pri, dump_set(%$mand), dump_set(%$opt);
	} else
	{	warn sprintf "Completed dependency worker %s for %s pri=%u has not been discarded\n", $w, $o, $pri;
	}
}

