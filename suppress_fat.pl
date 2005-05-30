#! /usr/bin/env perl
 
#---------------------------------------------------------------------------
#@COPYRIGHT :
#             Copyright 1996, D. Louis Collins
#             McConnell Brain Imaging Centre,
#             Montreal Neurological Institute, McGill University.
#             Permission to use, copy, modify, and distribute this
#             software and its documentation for any purpose and without
#             fee is hereby granted, provided that the above copyright
#             notice appear in all copies.  The author and McGill University
#             make no representations about the suitability of this
#             software for any purpose.  It is provided "as is" without
#             express or implied warranty.
#---------------------------------------------------------------------------- 
#$RCSfile: suppress_fat.pl,v $
#$Revision: 1.1 $
#$Author: bert $
#$Date: 2005-05-30 17:01:06 $
#$State: Exp $
#---------------------------------------------------------------------------

use Startup;
use JobControl;
use Getopt::Tabular;
use MNI::DataDir;

require "file_utilities.pl";
require "path_utilities.pl";
require "minc_utilities.pl";
require "numeric_utilities.pl";
require "volume_heuristics.pl";

# ------------------------------ start here
&Startup;

&Initialize;
  
# ------------------------------ get the input file

($originputfile) = &uncompress ($TmpDir, $inputfile);

# ------------------------------ build a brain mask on the native data volume

$Native_Mask = &build_native_brain_mask($originputfile, $Transform);

# ------------------------------ mask brain and non-brain voxels

$Brain    = &mask_data($originputfile, $Native_Mask, 0, "brain");
$NonBrain = &mask_data($originputfile, $Native_Mask, 1, "nonbrain");

# ------------------------------ decrease the bright (fat) voxels in nonbrain
#                                and increase dynamic range for brain, if possible

($Brain, $NonBrain) = &remap_intensities($Brain, $NonBrain, $Native_Mask);

# ------------------------------ put them back together, to produce result

&Spawn ("$MincMath -clob -add $Brain $NonBrain $outputfile");


&Shutdown (1);

# ------------------------------ end here!


# --------------------------------------------
sub Initialize
{
    $Version = "@VERSION@";
    $LongVersion = "version ${Version}: slightly tested perl code. Beware!";
    
    &SelfAnnounce ("STDOUT") if $Verbose && ! -t "STDOUT";

    $Verbose = 1;
    $Execute = 1;
    $Clobber = 0;
    $Debug = 0;
    
    $KeepTmp = 0;
    &JobControl::SetOptions ("ErrorAction", "fatal");
    
    $Transform = undef;
    $ModelDir  = 'avg305';
    $ModelBase = undef;
    $BrainMask = MNI::DataDir::dir($ModelDir) . "average_305_mask_1mm.mnc";
    
    my $usage = <<USAGE;
Usage: $ProgramName [options] <fat.mnc> <lite.mnc>
       $ProgramName [options] <fat.mnc> <transform.xfm> <lite.mnc>
       $ProgramName -help for details

USAGE

   my $help = <<HELP;

$ProgramName attempts to automatically reduce the intensity of voxels
outside the brain (scalp, fat). The definition of -outside of brain-
is all voxels that lie outside a predefined brain mask. The procedure
depends on a pre-calculated stereotaxic transformation to this model,
and the inverse of this transform is used to map the brain mask onto
the native data.  The program minclookup is then used to remap the
intensities of the voxels outside of the brain.  
HELP

    @ArgInfo =
      (@DefaultArgs,
       ["Specific options", "section"],
       ["-version", "call", undef, \&print_version, "print version and quit"], 
       ["-transform", "string", 1, \$Transform,
	"transformation from the source volume to the brain mask (preferred over the three-argument invocation)", "<transform.xfm>"],
       ["-mask", "string", 1, \$BrainMask,
	"use this brain mask [default: $BrainMask]", 
	"<brainmask.mnc>"],
       ["-modeldir", "string", 1, \$ModelDir,
	"set the default directory to search for model files (use with -model). If this directory does not exist as such, it is assumed to be the name of a standard MNI datadir [default: $ModelDir]",
	"<modeldir>"],
       ["-modelbase", "string", 1, \$ModelBase,
	"set the base name of the model files (use with -modeldir; overrides -mask)", 
	"<model_basename>"]
       );

    &Getopt::Tabular::SetHelp ($help, $usage);
    
    my (@argv) = @ARGV;
    &GetOptions (\@ArgInfo, \@argv) || &Fatal ();

    if (@argv == 2) {
	($inputfile, $outputfile) = @argv;
    }
    elsif (@argv == 3) {
	($inputfile, $Transform, $outputfile) = @argv;
    }
    else {
	die "Incorrect number of arguments\n";
    }
    
    # Look for required programs
    $MincMath     = &FindProgram ("mincmath");
    $XfmInvert    = &FindProgram ("xfminvert");
    $MincResample = &FindProgram ("mincresample");
    $MincLookup   = &FindProgram ("minclookup");
    $VolumeStats  = &FindProgram ("volume_stats");
    $VolumeHist   = &FindProgram ("volume_hist");
    
    die unless ($MincMath && $XfmInvert && $MincResample && $VolumeStats && 
		$VolumeHist && $MincLookup);
    
    # They were found, so add options according to
    # $Debug, $Verbose, $Clobber flags
    
    my ($debug, $verbose, $clobber);
    $debug   = ($Debug)   ? " -debug"   : "";
    $verbose = ($Verbose) ? ""          : " -quiet";
    $clobber = ($Clobber) ? " -clobber" : "";
    
    $MincResample .= "$verbose$clobber";
    $MincMath     .= "$debug$verbose$clobber";

    if (! -d $ModelDir) {
	$ModelDir  = MNI::DataDir::dir($ModelDir);
    }

    if (defined($ModelBase)) {
	$BrainMask = "${ModelDir}${ModelBase}_mask.mnc";
    }
    
    if (-e $outputfile && !$Clobber) {
	die "$outputfile already exists (use -clobber to overwrite)\n";
    }

    die if (defined($Transform) && !CheckFiles($Transform));
    die if (!CheckFiles($inputfile, $BrainMask));

    &CheckOutputDirs ($TmpDir) if $Execute;
}

# ------------------------------
#@NAME       : remap_intensities
#@INPUT      : three minc volumes: one for brain, one for non-brain, one for brain mask
#@OUTPUT     : two intensity corrected minc volumes: 
#              one for brain, one for non-brain
#@RETURNS    : names of brain and non-brain volumes
#@DESCRIPTION: 
#@METHOD     : invert the .xfm, and resample the brain mask into the 
#	       native space.
#@GLOBALS    : $Mask 
#@CALLS      : 
#@CREATED    : originally: Dec.11.96 Louis
#@MODIFIED   : 

sub remap_intensities {
    my ($brain, $nonbrain, $mask) = @_;

    my ($new_brain, $new_nonbrain, 
	$min, $max, $mean, $std,
	$nb_min, $nb_max, $nb_mean, $nb_std,
	$f_min, $f_max, $f_mean, $f_std, $f1, $t1, $t2,
	$lut, $lower, $upper, $min_range, $max_range);

    $filebase   = (&SplitPath ($brain))[1];
    $new_brain  = "${TmpDir}/${filebase}_remapped.mnc";
    $filebase   = (&SplitPath ($nonbrain))[1];
    $new_nonbrain  = "${TmpDir}/${filebase}_remapped.mnc";

				# Get brain and nonbrain intensity stats

    ($min_range, $max_range) = volume_minmax($brain);
    ($min, $max, $mean, $std) = get_masked_volume_stats( $brain, $mask );
    print "Brain:     min = $min,  max = $max, mean = $mean, std = $std\n" if ($Debug) ;
    print "Brain:   range  min = $min_range,  max = $max_range\n" if ($Debug);

#    ($nb_min, $nb_max, $nb_mean, $nb_std) = get_masked_volume_stats( $nonbrain, $mask );
#    print "Non-Brain: min = $min,  max = $max, mean = $mean, std = $std\n" if ($Debug) ;

    $f_mean = $mean / $max_range ;
    $f1 = ($mean + 5 * $std) / $max_range;
    $f1 = 1.0 if ($f1 > 1.0);
    $t1 = $mean + $std;
    $t2 = $mean + $std + $std;
    $lut = "-lut_string '0 0; $f_mean $mean; $f1 $t1; 1.0 $t2'";

    if (-e "$new_nonbrain") {
	print ("file $new_nonbrain already exists\n");
    }				
    else {
	&Spawn ("$MincLookup $lut $nonbrain $new_nonbrain");
    }


    ($lower, $upper) = &compute_thresholds($brain, $mask, 0.0, 0.001);
    print "brain thresholds: $lower $upper\n" if ($Debug);

    if ( ($upper + $std) < $max_range ) {
	$upper = $upper + $std;

	$f1 = $upper/$max_range; 

	$lut = "-lut_string '0 0;  $f1 $upper'";
	print $lut if ($Debug);

	if (-e "$new_brain") {
	    print ("file $new_brain already exists\n");
	}				
	else {
	    &Spawn ("$MincLookup $lut $brain $new_brain");
	}
    }
    else {
	$upper = max_range;
	$new_brain = $brain;
    }



    ($new_brain, $new_nonbrain);
}



sub get_masked_volume_stats{
    my($file, $mask) = @_;

    my($output, $result);

    ($result, $output) = &Spawn ("$VolumeStats -min -max -mean -stddev -quiet -mask $mask ${file}","-");

    split("\n",$output);
}

# ------------------------------
#@NAME       : compute_thresholds
#@INPUT      : -the name of the input minc file (containing a masked 
#               t1-weighted brain)
#              -the brain mask 
#              low_lim is the % of max to return for lower threshold
#              up_lim is the % of area under histogram computed from the 
#              right tail to return for upper threshold
#@OUTPUT     : 
#@RETURNS    : upper and lower thresholds to maximize dynamic range
#@DESCRIPTION: 
#@METHOD     : 
#@GLOBALS    : 
#@CALLS      : 
#@CREATED    : originally: Dec.11.96 Louis
#@MODIFIED   : 


sub compute_thresholds{
    my ($file, $mask, $low_lim, $up_lim) = @_;
    my ($filebase, $upper, $lower, $out_histo, $int, $cnt, $s_int, $s_cnt, $i, $s);

    $filebase = (&SplitPath ($file))[1];

    $out_histo = "${TmpDir}/${filebase}.histo.txt";
    
    &Spawn("$VolumeHist -mask $mask $file $out_histo -bins 512 -w -text -clob");

    $s_int = 0.0;
    $s_cnt = 0.0;
    open (OBJ, $out_histo) || die "can't open $out_histo: $!\n";

    while (<OBJ>) {
	if (/^# histogram for class 1/) {
	    last;
	}
	next;	       
    }

    while (<OBJ>) {
	if ($_ =~ /#/)  {
	    print $_ if ($Debug);
	}
	else {
	    chop;
	    ($int, $cnt) = split ;
	    $s_int = $s_int + $int;
	    $s_cnt = $s_cnt + $cnt;
	    push (@intensities, $int);
	    push (@counts, $cnt);
	}
    }

    $s = 0.0;
    for($i= $#counts; $i >=0; $i--) {


#	$ratio  = $s / $s_cnt;
#	print (" $i : $intensities[$i] $counts[$i] ( $s  $s_cnt ) $ratio\n") if ($Debug);

	$s = $s + $counts[$i];

	$upper = $intensities[$i];
	
	last if ( ($s / $s_cnt) > $up_lim ) ;
    }

    
    $lower = $low_lim * $intensities[ $#intensities ];

    ($lower, $upper);
}



# ------------------------------
#@NAME       : build_native_brain_mask
#@INPUT      : transformation file to bright model
#@OUTPUT     : native space brain mask volume
#@RETURNS    : name of brain mask to be applied to native data
#@DESCRIPTION: 
#@METHOD     : invert the .xfm, and resample the brain mask into the 
#	       native space.
#@GLOBALS    : $BrainMask 
#@CALLS      : 
#@CREATED    : originally: Dec.11.96 Louis
#@MODIFIED   : 

sub build_native_brain_mask {
    my ($native_mnc, $xfm_file) = @_;

    my ($result, $inverse);
				# get the file basename, without directory
    $filebase = (&SplitPath ($native_mnc))[1];

    $result  = "${TmpDir}/${filebase}_native_brain_mask.mnc";
    $inverse = "${TmpDir}/${filebase}_inv.xfm";

				# build the native  brain mask
    my($ResampleCmd) = 
	"$MincResample -nearest $BrainMask $result -like $native_mnc";

    if (defined($xfm_file)) {
	&Spawn("$XfmInvert $Transform $inverse");
	$ResampleCmd .= " -transformation $inverse";
    }

    &Spawn($ResampleCmd);
    
    ($result);
}

# -----------------------------------------------------------------
# subroutine: mask_data
# 
# inputs:     $file   - the name of the input minc file
#             $mask   - the name of the mask file
#             $invert_flag - used to invert the mask
#             $label  - character string used to identify the masked volume
# output:     a masked volume with the name $result
# returns:    $result
#

sub mask_data {
    my ($file, $mask, $invert_flag, $label) = @_;

    my($filebase) = (&SplitPath ($file))[1];
    my($result)   = "${TmpDir}/${filebase}_${label}.mnc";

    if (-e "$result") {
	print ("file $result already exists\n");
    }				
    else {
	if ($invert_flag) {
	    my($inverted_mask) = &ReplaceDir($TmpDir, $mask);
	    $inverted_mask .= "_inv";

	    if (-e $inverted_mask) {
		warn "$inverted_mask already exists\n";
	    }
	    else {
		&Spawn("$MincMath -lt -const 0.5 $mask $inverted_mask");
	    }

	    $mask = $inverted_mask;
	}
	    
	&Spawn("$MincMath -mult $file $mask $result");
    }

    ($result);
}

sub print_version {
    print "Program $ProgramName, built from:\n$LongVersion\n";
    exit;
}

