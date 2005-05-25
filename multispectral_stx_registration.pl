#! /usr/bin/env perl

# stereotaxic registration for the VU Amsterdam protocol images

use strict;
use MNI::Spawn;
use MNI::Startup;
use Getopt::Tabular;

# global variables
my $model = undef;
my $modelDir = undef;
my $threshold = 1;

# argument handling
my @leftOverArgs;
my @argTbl = 
  (
   @DefaultArgs,
   ["-model", "string", 1, \$model,
    "set the base name of the fit model files"],
   ["-modeldir", "string", 1, \$modelDir,
    "set the default directory to search for model files"],
   ["-threshold", "boolean", undef, \$threshold,
    "Enable thresholding in mritoself"],
  );
GetOptions(\@argTbl, \@ARGV, \@leftOverArgs) or die "\n";

my $usage = "$0 [options] T1.mnc T2.mnc PD.mnc T1_to_tal.xfm T2PD_to_tal.xfm\n";

my $nativeT1 = shift @leftOverArgs or die $usage;
my $nativeT2 = shift @leftOverArgs or die $usage;
my $nativePD = shift @leftOverArgs or die $usage;
my $T1total = shift @leftOverArgs or die $usage;
my $T2total = shift @leftOverArgs or die $usage;

# create the tempdir if necessary
system("mkdir -p $TmpDir") unless (-d $TmpDir);

# register the programs
RegisterPrograms([qw/mritotal
                  mritoself
                  xfmconcat/]);

if ($Clobber) {
  AddDefaultArgs("mritotal", ["-clobber"]);
  AddDefaultArgs("mritoself", ["-clobber"]);
}

if (! $threshold) { 
    AddDefaultArgs("mritoself", ["-nothreshold"]);
}

# step 1: T1 registration to talairach space
my @mritotalOptions;
push @mritotalOptions, ["-modeldir", $modelDir] if $model;
push @mritotalOptions, ["-model", $model] if $model;

Spawn(["mritotal", @mritotalOptions, $nativeT1, $T1total]);

# step 2: registration of T2/PD to native T1
my $T2toT1 = "${TmpDir}/t2_to_t1.xfm";
Spawn(["mritoself", "-mi", "-lsq6", $nativeT2, $nativeT1, $T2toT1]);

# step 3: concatenate transforms to get T2 to tal transform
Spawn(["xfmconcat", $T2toT1, $T1total, $T2total]);
