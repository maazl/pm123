#!usr/bin/perl
use strict;
use File::Find;

my %files;
my $lastfile;
my $cont;
my @row;

while (<>)
{ # CSV Parser
  $_ = '"'.$_ if $cont;
  while (/"((?:""|[^"])*)(",|"\n$|$)|([^,]*)(,|\n$)/g)
  { push @row, undef unless $cont;
    my $data;
    if (defined $1)
    { $data = $1;
      $data =~ s/""/"/g;
      $cont = !$2;
    } else
    { $data = $3;
      undef $cont;
    }
    $row[$#row] .= $data;
    #print "DATA: $cont.$#row.$data.\n";
  }
  next if $cont;

  # 0       1          2       3      4             5          6     7   8    9      10   11     12     13        14
  # urlname,parentname,baseref,result,warningstring,infostring,valid,url,line,column,name,dltime,dlsize,checktime,cached
  #print "LINE: $row[7].", join("\n\t", @row), "\n\n";

  my $relurl = $row[7];
  if ($relurl =~ s/file:.*\/doc\///i)
  { $files{$relurl} = undef;
    my $relurl = $row[1] || $row[7];
    $relurl =~ s/file:.*\/doc\///i;
    my $valid = $row[6] eq 'True';
    if (!$valid || $row[4])
    { if ($lastfile ne $relurl)
      { $lastfile = $relurl;
        print "\nFILE:\t$relurl\n";
      }
      map s/\n/\n\t/g, @row;
      print "Line:\t${row[8]}\n" if $row[8];
      print "Result:\t${row[3]}\n" if !$valid;
      print "Warn:\t${row[4]}\n" if $row[4];
      print "Info:\t${row[5]}\n" if $row[5];
    }
  }
  undef @row;
}

# look for orphaned files
sub wanted
{ next if /\/\./;
  s/^\.\///;
  next unless /\//;
  next unless -f;
  next unless /\.(jpg|html|gif|css)$/;
  next if exists $files{$_};
  print "\nOrphan:\t$_";
}

find({no_chdir=>1, wanted=>\&wanted}, '.');

