#!/usr/bin/perl
use strict;

# Script to patch libmpg123 from Thomas Orgis to compile with 64 bit file support.

# get all source files
opendir my $D, '.' or die;
my @files = readdir $D;
closedir $D;

# process files
foreach my $file (@files)
{
  next unless $file =~ /\.[chS]$/i;

  # load file
  open F, "<$file" or die;
  my @lines = <F>;
  close F;
  # apply patches
  my $mod;
  if ($file =~ /S$/)
  { # Assembler
    s/^#ifndef\s+__APPLE__/#ifdef OS_IS_OS2\n\t.text\n#elif !defined(__APPLE__)/ and $mod=1 foreach @lines;
  } else
  { # C
    s/(^|\W)off_t/$1mpg123_off_t/g and $mod=1 foreach @lines;
  }
  next unless $mod;
  # write
  open F, ">$file" or die;
  binmode F;
  print F @lines;
  close F;
}
