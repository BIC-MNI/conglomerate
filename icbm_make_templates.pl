#! /usr/bin/env perl

# Generate templates for a given model (inspired by icbm_make_templates 
# from Neelin).
#
# Claude Lepage - claude@bic.mni.mcgill.ca
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted,
# provided that the above copyright notice appear in all copies.  The
# author and the University of Queensland make no representations about the
# suitability of this software for any purpose.  It is provided "as is"
# without express or implied warranty.

use strict;
use warnings "all";
use File::Temp qw/ tempdir /;

# Output prefix
my $PREFIX = "icbm_nlin_template";

# Dimensions
my $XSTART = -96;
my $YSTART = -132;
my $ZSTART = -78;

my $XCENTER = 0;
my $YCENTER = -18;
my $ZCENTER = 18;

my $XLEN = 192;
my $YLEN = 228;
my $ZLEN = 192;

my @spacings = ( "0.50", "0.75", "1.00", "1.50", "2.00", "3.00", "4.00", "6.00" );

my $date = `date`;
chomp( $date );

# make tmpfile
my $tmpdir = &tempdir( "template-XXXX", TMPDIR => 1, CLEANUP => 1 );

# Generate the templates at each size

for( my $i = 0;  $i < @spacings;  $i++ ) {

  my $dx = $XLEN / $spacings[$i] + 1;
  my $dy = $YLEN / $spacings[$i] + 1;
  my $dz = $ZLEN / $spacings[$i] + 1;

  my $list = "$dx $dy $dz -xstart $XSTART -ystart $YSTART -zstart $ZSTART " .
             "-xstep $spacings[$i] -ystep $spacings[$i] -zstep $spacings[$i] " .
             "-xdircos 1 0 0 -ydircos 0 1 0 -zdircos 0 0 1";
  print "creating template ${PREFIX}_${spacings[$i]}mm.mnc...\n";

  open PIPE, ">${tmpdir}/template_${spacings[$i]}.sh";
  print PIPE "#! /bin/csh -f\n";
  print PIPE "limit filesize 4k\n";
  print PIPE "rawtominc -clobber -byte -input /dev/zero ${PREFIX}_${spacings[$i]}mm.mnc " .
             "$list -transverse -sattribute xspace:spacetype=talairach_ " .
             "-sattribute yspace:spacetype=talairach_ -sattribute zspace:spacetype=talairach_ " .
             "-sattribute xspace:units=mm -sattribute yspace:units=mm -sattribute zspace:units=mm " .
             "-sattribute \':history=$date >>> icbm_make_templates\'\n";
  close PIPE;
  system( "chmod u+x ${tmpdir}/template_${spacings[$i]}.sh" );
  system( "${tmpdir}/template_${spacings[$i]}.sh" );
  unlink( "${tmpdir}/template_${spacings[$i]}.sh" );
}


sub run { 
  system(@_) == 0 or die;
}
       
