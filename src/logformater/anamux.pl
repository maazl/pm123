use strict;

my %mux;

while (<>)
{ /[ :](\d{4}) .* Mutex\((?:0x)?([0-9a-fA-F]+).*\)::(.*)/ or next;
  my $tid = $1;
  my $mtx = $2;
  $_ = $3;
  if (/^Mutex\(/)
  { # constructor
    warn "Mutex $mtx constructed twice at line $.\n" if exists $mux{$mtx};
    $mux{$mtx} = [0,0,undef]; # requests, owned, owned by 
    next;
  }
  # any other member function
  unless (exists $mux{$mtx})
  { warn "Access to non-existing Mutex $mtx at line $.\n";
    next;
  }
  my $r = $mux{$mtx};
  if (/^Request\(/)
  { ++$$r[0];
  } elsif (/^Request /)
  { warn "No outstanding request to $mtx at line $.???\n" unless $$r[0];
    warn "Mutex $mtx owned by other thread at line $.???\n" if $$r[1] && $$r[2] != $tid;
    --$$r[0];
    ++$$r[1];
    $$r[2] = $tid;
  } elsif (/^Release\(/)
  { unless ($$r[1])
    { warn "Released Mutex $mtx not owned at line $.\n";
    } else
    { --$$r[1];
    }  
  } elsif (/^~Mutex/)
  { warn "Destroying Mutex $mtx with $$r[0] outstanding requests at line $.\n" if $$r[0];
    warn "Destroying Mutex $mtx owned by thread $$r[2] at line $.\n" if $$r[1];
    delete $mux{$mtx}; 
  }
}

# Print short summary first
while (my ($mtx, $r) = each %mux)
{ next unless $$r[0] || $$r[1];
  print "$mtx -> [@$r]\n";
}

# print all existing mutexes
print "\n";
print join "\t", sort keys %mux;
print "\n";

