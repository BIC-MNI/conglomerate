#!/usr/bin/env perl

# stereotaxic registration for the VU Amsterdam protocol images

use strict;
use MNI::Spawn;
use MNI::Startup;
use Getopt::Tabular;

# global variables
my $model = undef;
my $premodel= undef;
my $modelDir = undef;
my $premodelDir= undef;
my $t1suppressed= undef;
my $threshold = 1;

# argument handling
my @leftOverArgs;
my @argTbl = 
  (
   @DefaultArgs,
   ["-model", "string", 1, \$model,
    "set the base name of the main fit model files"],
   ["-premodel", "string", 1, \$premodel,
    "set the base name of the initial fit model files in two-stage registration"],
   ["-modeldir", "string", 1, \$modelDir,
    "set the default directory to search for main fit model files"],
   ["-premodeldir", "string", 1, \$premodelDir,
    "set the default directory to search for initial fit model files in two-stage registration"],
	["-t1suppressed", "string", 1, \$t1_suppressed,
	 "set the suppressed file name in two-stage registration"],
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
### in case of two-stage registration
if ($premodel) {
	RegisterPrograms(qw/mritotal_suppress)
}


if ($Clobber) {
  AddDefaultArgs("mritotal", ["-clobber"]);
  AddDefaultArgs("mritoself", ["-clobber"]);
}

if (! $threshold) { 
    AddDefaultArgs("mritoself", ["-nothreshold"]);
}

if ($premodel) {
	AddDefaultArgs("mritotal_suppress", ["-premodel"], ["-premodelDir"], ["-keepsuppressed"])
}

# step 1: T1 registration to talairach space
### single-stage registration:
unless ($premodel) {
my @mritotalOptions;
push @mritotalOptions, ["-modeldir", $modelDir] if $model;
push @mritotalOptions, ["-model", $model] if $model;

Spawn(["mritotal", @mritotalOptions, $nativeT1, $T1total]);
}

### two-stage registration:
if ($premodel) {
my @mritotal_suppressOptions;	
push @mritotal_suppressOptions, ["-premodeldir", $premodelDir] if $premodel;
push @mritotal_suppressOptions, ["-premodel", $premodel] if $premodel;
push @mritotal_suppressOptions, ["-modeldir", $modelDir] if $model;
push @mritotal_suppressOptions, ["-model", $model] if $model;

Spawn(["mritotal_suppress", @mritotal_suppressOptions, $t1suppressed, $nativeT1, $T1total])
}

# step 2: registration of T2/PD to native T1
my $T2toT1 = "${TmpDir}/t2_to_t1.xfm";
Spawn(["mritoself", "-mi", "-lsq6", $nativeT2, $nativeT1, $T2toT1]);

# step 3: concatenate transforms to get T2 to tal transform
Spawn(["xfmconcat", $T2toT1, $T1total, $T2total]);
