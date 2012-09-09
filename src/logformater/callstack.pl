# Analyze call stack of each thread from a log file.
use strict;

my @thread;

sub dostack($$$)
{ my ($tid, $stack, $fn) = @_;
  $stack = sprintf "%04x", 0xffff-length $stack if $stack =~ /^ *$/;
  my $r = $thread[$tid];
  unless ($r)
  { $thread[$tid] = [[$stack, $fn]];
    return;
  }
  shift @$r while @$r && $$r[0]->[0] le $stack;
  unshift @$r, [$stack, $fn];
}

while (<>)
{ /[: ](\d{4}) ([ 0-9a-fA-F]*) (.*)/ or next;
  dostack($1, $2, "($.) $3");
}

for (my $tid = 0; $tid < @thread; ++$tid)
{ my $r = $thread[$tid] or next;
  print "**** Thread $tid:\n";
  foreach (@$r)
  { print "$$_[0] -> $$_[1]\n";
  }
  print "\n";
}
