use strict;

# memory objects (operator new) {$address} = [$line, $size]
my %objmap;

sub opnew($$)
{	my ($ptr, $size) = @_;
	exists $objmap{$ptr} and warn "duplicate address $ptr returned by new at $., previous new at $objmap{$ptr}[0]\n";
	$objmap{$ptr} = [$., $size];
}

sub opdel($)
{	my ($ptr) = @_;
	delete $objmap{$ptr} or warn "delete of unknown object $ptr at $.\n";
}

while (<>)
{	/operator new(?:\[\])?\((\d+)\)\s*:\s*(?:0x)?(\w+)/ and opnew $2, $1;
	/operator delete(?:\[\])?\((?:0x)?(\w+)/ and opdel $1; 
}

my $count;
my $size;

while (my ($ptr, $data) = each %objmap)
{	++$count;
	$size += $$data[1];
	print "$ptr : $$data[1] @ $$data[0]\n";
}
print "summary : size = $size, count = $count\n";

