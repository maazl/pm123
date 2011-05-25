# recursive playlist simulation
use strict;

# TEST CASES
# Example
#   A => [qw/B a C B/]
# is read as A contains B then C then B (again). 
my %lists =
( A => [qw/B a C B/]
, B => [qw/b A C D/]
, C => [qw/D c/]
, D => [qw/d A A b/]
);


sub contains($@)
{ my $search = shift;
  foreach (@_)
  { return 1 if $_ eq $search;
  }
}

# enumerate(callback(index, item, callstack), list, callstack)
# Call callback for each item in the list.
sub enumerate(&$;$@)
{ my ($cb, $list, $idx, @stack) = @_;
  &$cb($idx, $list, @stack);
  unshift @stack, $list;
  # get list content
  my $content = $lists{$list};
  #print "enumerate $list, stack = @stack\n";
  # enumerate
  return unless $content;
  undef $idx;
  foreach (@$content)
  { enumerate($cb, $_, ++$idx, @stack) unless contains $_, @stack;
  }
}

my @all = @ARGV;
@all = sort keys %lists unless @all;

print "*** enumerated lists\n";
enumerate { print "@_\n"; } $_ foreach @all;

print "*** aggregates required at list level\n";
my %agg;
enumerate { $agg{"$_[1] without ".join(' ',sort @_[2..$#_])} = undef if exists $lists{$_[1]}; } $_ foreach @all;
print "$_\n" foreach sort keys %agg;

#print "*** aggregates required at reference level\n";
#my %agg;
#enumerate { $agg{"$_[2] -> $_[1]$_[0] without ".join(' ',sort @_[2..$#_])} = undef if exists $lists{$_[1]}; } $_ foreach @all;
#print "$_\n" foreach sort keys %agg;

print "*** offsets per list item\n";
my %off;
foreach (@all)
{ my $offset;
  my @lens;
  enumerate
  { ++$off{"$_[2] -> $_[1]$_[0] without ".join(' ',sort @_[2..$#_])." offset ".substr($offset,$lens[@_-2])};
    #$#lens = @_-2;
    $offset .= $_[1] unless exists $lists{$_[1]};
    $lens[@_-1] = length $offset;
  } $_;
} 
print "($off{$_}) $_\n" foreach sort keys %off;

