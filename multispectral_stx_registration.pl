#! /usr/bin/env perl

###############################################################################
###############################################################################
###  Run multispectral linear registration in either a single- or two-stage 
###  fashion
###
###	Authors:
###			Original versions: Jason Lerch
###			Current modifications: Yasser Ad-Dab'bagh
###			Date: May 26, 2005
###############################################################################
###############################################################################

use strict;
use MNI::DataDir;
use MNI::Spawn;
use MNI::Startup;
use Getopt::Tabular;

my $version = 0.1.1;
# version date: May 26, 2005
# last person to modify: Yasser Ad-Dab'bagh

##############################################################
### global variables
##############################################################

my $twoStage= 0;
my $modelDir = MNI::DataDir::dir("mni_autoreg");
my $kiddieModel = "${modelDir}/nih_chp_avg";
my $Template = "${modelDir}/icbm_template_1.00mm.mnc";
my $model = "${Template}";
my $t1suppressed= undef;
my $threshold = 1;
my $premodel = undef;
my $premodelDir = undef;

if ($twoStage) {
    $premodel= "${kiddieModel}";
    $premodelDir= MNI::DataDir::dir("mni_autoreg");
}

##############################################################
### argument handling
##############################################################
my @leftOverArgs;
my @argTbl = 
    (
     @DefaultArgs,
     ["-two-stage|-single-stage", "boolean", undef,\$twoStage,
      "Use a two-stage registration process"],
     ["-model", "string", 1, \$model,
      "set the base name of the main fit model files"],
     ["-premodel", "string", 1, \$premodel,
      "set the base name of the initial fit model files in two-stage registration"],
     ["-modeldir", "string", 1, \$modelDir,
      "set the default directory to search for main fit model files"],
     ["-premodeldir", "string", 1, \$premodelDir,
      "set the default directory to search for initial fit model files in two-stage registration"],
     ["-t1suppressed", "string", 1, \$t1suppressed,
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

my $help = <<HELP;

$ProgramName version $version

Takes multispectral minc image files and registers them to 
MNI-Talairach space in a linear fashion. This can be done 
either directly to a standard model (single-stage 
registration), or to an intermediary model first and then 
to the standard model (two-stage registration). 

HELP
Getopt::Tabular::SetHelp($help, $usage);


##############################################################
### create the tempdir if necessary
##############################################################
system("mkdir -p $TmpDir") unless (-d $TmpDir);

##############################################################
### register the programs
##############################################################
RegisterPrograms([qw/mritotal
						mritoself
						xfmconcat/]);

### in case of two-stage registration
if ($twoStage) {
    RegisterPrograms([qw/mritotal_suppress/]);
}


if ($Clobber) {
    AddDefaultArgs("mritotal", ["-clobber"]);
    AddDefaultArgs("mritoself", ["-clobber"]);
}

if (! $threshold) { 
    AddDefaultArgs("mritoself", ["-nothreshold"]);
}

if ($twoStage and $Clobber) {
    AddDefaultArgs("mritotal_suppress", ["-clobber"]);
}

##############################################################
### step 1: T1 registration to talairach space
##############################################################
		### two-stage registration:
if ($twoStage) {
    my @mritotal_suppressOptions;	
    push @mritotal_suppressOptions, ["-premodeldir", $premodelDir] if $premodel;
    push @mritotal_suppressOptions, ["-premodel", $premodel] if $premodel;
    push @mritotal_suppressOptions, ["-modeldir", $modelDir] if $model;
    push @mritotal_suppressOptions, ["-model", $model] if $model;
    push @mritotal_suppressOptions, ["-keep_suppressed", $t1suppressed] if $t1suppressed;
    Spawn(["mritotal_suppress", @mritotal_suppressOptions, $nativeT1, $T1total]);
}
		### single-stage registration:
else {
    my @mritotalOptions;
    push @mritotalOptions, ["-modeldir", $modelDir] if $model;
    push @mritotalOptions, ["-model", $model] if $model;
    Spawn(["mritotal", @mritotalOptions, $nativeT1, $T1total]);
}

##############################################################	
### step 2: registration of T2/PD to native T1
##############################################################
my $T2toT1 = "${TmpDir}/t2_to_t1.xfm";
Spawn(["mritoself", "-mi", "-lsq6", $nativeT2, $nativeT1, $T2toT1]);

##############################################################
###step 3: concatenate transforms to get T2 to tal transform
##############################################################
Spawn(["xfmconcat", $T2toT1, $T1total, $T2total]);



########################### done! ############################
