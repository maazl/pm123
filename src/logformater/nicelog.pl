#!perl
use strict;

my @stack;
my @indent;

while(<>)
{ if (my ($tim,$tid,$stk,$val) = /(\d+)\s+[0-9A-Fa-f]+:(\d+)\s+([0-9A-Fa-f]+)(.*)/)
  { #print STDERR "$tim-$tid-$stk-$val-\n";
    $indent[$tid] .= ' ';
    while ($stack[$tid][0] && ($stk ge $stack[$tid][0]))
    { #print STDERR "$indent[$tid].@{$stack[$tid]}.\n";
      $indent[$tid] = substr $indent[$tid], 1;
      shift @{$stack[$tid]};
    }
    unshift @{$stack[$tid]}, $stk;
    #print STDERR "STK:@{$stack[$tid]}.\n";
    print "$tim $tid$indent[$tid]$val\n";
  } else
  { print $_;
  }
}
