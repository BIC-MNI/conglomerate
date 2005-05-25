#!/usr/local/bin/perl5 -w
 
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
#$RCSfile: mritotal_suppress.pl,v $
#$Revision: 1.1 $
#$Author: bert $
#$Date: 2005-05-25 20:54:28 $
#$State: Exp $
#---------------------------------------------------------------------------

use Startup;
use JobControl;
use ParseArgs;
use MNI::DataDir;

require "file_utilities.pl";
require "path_utilities.pl";
require "minc_utilities.pl";
require "numeric_utilities.pl";
require "volume_heuristics.pl";

# ------------------------------ start here
&Startup;
&Initialize;

if (defined($MaxTE)) {
    my($TE) = &get_volume_echo_time(defined($NativeVolume) ? $NativeVolume : $inputfile);
    $ForceSuppress = ($TE < $MaxTE) ? 1 : 0;
}

if ($ForceSuppress) {
    my ($filebase);

    $filebase = (&SplitPath ($inputfile))[1];

				# 1st step: get .xfm to bright model
    if (!defined($InitialXfm)) {
	$InitialXfm = "${TmpDir}/${filebase}_bright.xfm";

	&Spawn ("$MriToTal $inputfile $InitialXfm -modeldir $ModelDir ".
		"-model $ModelBase -nocrop");
    }
				# 2nd - suppress fat in $inputfile volume
    if (-e $InitialXfm) {
	$MriToTal .= " -transformation $InitialXfm";

	my($tmpmnc) = "${TmpDir}/${filebase}_lowfat.mnc";

	&Spawn ("$SuppressFat $inputfile $InitialXfm $tmpmnc");

	if (-e $tmpmnc) {
	    if (defined($Suppressed_file)) {
		print "Storing the fat_suppressed file to $Suppressed_file" if $Debug;
		&Spawn("cp $tmpmnc $Suppressed_file");
	    }
	    &Spawn ("$MriToTal -nocrop $tmpmnc $xfmfile");
	}
	else {
	    &Fatal("I can't find $tmpmnc,\neven though I just tried to create it.");	
	}

	if (defined($SaveXfm) && ($SaveXfm ne $InitialXfm)) {
	    &Spawn("cp $InitialXfm $SaveXfm");
	}
    }
    else {
	&Fatal("I can't find $InitialXfm,\neven though I just tried to create it.");	
    }

}
else {
    if (defined($InitialXfm)) {
	$MriToTal .= " -transformation $InitialXfm";
    }
			       
    $MriToTal .= " -nocrop $inputfile $xfmfile";

    &Spawn ("$MriToTal");
}


&Shutdown (1);

# ------------------------------ end here!

sub get_volume_echo_time{
    my($file) = @_;
    
    my ($echo_time, $result);
    
    ($result, $echo_time) = Spawn("mincinfo -attval acquisition:echo_time ${file}","-");

				# since ErrorAction is probably set to FATAL, I 
                                # don't have to worry about checking the value of $result
    ( $echo_time );	       
}


# --------------------------------------------
sub Initialize
{
    $Version = "0.1";
    $LongVersion = "version ${Version}: slightly tested perl code. Beware!";
    $Verbose = 1;
    $Execute = 1;
    $Clobber = 0;
    $Debug   = 0;
    $KeepTmp = 0;
    
    sub print_version {
	print "Program $ProgramName, built from:\n$LongVersion\n";
	exit;
    }

    &SelfAnnounce ("STDOUT") if $Verbose && ! -t "STDOUT";

    &JobControl::SetOptions ("ErrorAction", "fatal");

    my $usage = <<USAGE;
Usage: $ProgramName [options] <orig_t1.mnc> <stx_total.xfm> 
       $ProgramName -help for details

USAGE

    my $help = <<HELP;

$ProgramName is a wrapper around mritotal which will, depending on 
the supplied options, either execute the default mritotal, or use
a two-stage registration. The first stage of the two-stage process
uses a fat-suppressed volume and possibly an alternate model.

See also mritotal and suppress_fat.
HELP

   $ForceSuppress   = 1;
   $MaxTE           = undef;
   $NativeVolume    = undef;
   $Suppressed_file = undef;
   $InitialXfm      = undef;
   $SaveXfm         = undef;
   $ModelDir        = 'bright_scalp_model';
   $ModelBase       = 'stx_bright_scalp2';

   @ArgInfo =
      (@DefaultArgs,
       ["Specific options", "section"],
       ["-version", "call", undef, \&print_version,
        "print version and quit"],
       ["-force_suppress|-noforce_suppress", "boolean", 1, \$ForceSuppress,
	"force two-stage registration with fat suppression [default; opposite is -noforce_suppress]"],
       ["-maxTE", "float", 1, \$MaxTE,
	"force two-stage registration with fat suppression only if the TE of the source volume is less than <TE> (overrides -force_suppress flag)", "<TE>"],
       ["-native_volume", "string", 1, \$NativeVolume,
	"volume to extract TE from [default: use <orig_t1.mnc>]", "<volume.mnc>"],
       ["-modeldir", "string", 1, \$ModelDir,
	"directory containing the model for the first fitting stage. If this directory does not exist as such, the supplied string is assumed to be the name of a standard MNI data directory [default: $ModelDir]", "<modeldir>"],
       ["-model", "string", 1, \$ModelBase,
	"base name of the model found in <modeldir> [default: $ModelBase]", 
	"<basename>"],
       ["-keep_suppressed", "string", 1, \$Suppressed_file,
	"store the fat-suppressed volume in <lite.mnc>", "<lite.mnc>"],
       ["-transformation", "string", 1, \$InitialXfm,
	"initial transformation. This will cause the first stages to be skipped (see mritotal)", "<transform.xfm>"],
       ["-save_transformation", "string", 1, \$SaveXfm,
	"save the intermediate transformation", "<transform.xfm>"]
       );

   &ParseArgs::SetHelpText ($help, $usage);

   my (@argv) = @ARGV;
   &ParseArgs::Parse (\@ArgInfo, \@argv) || &Fatal ();

   if (@argv != 2)
   {
      warn $usage;
      die "Incorrect number of arguments\n";
   }

   # Look for required programs
   $MriToTal     = &FindProgram ("mritotal");
   $SuppressFat  = &FindProgram ("suppress_fat");

   exit 1 unless ($MriToTal && $SuppressFat );

   # They were found, so add options according to
   # $Debug, $Verbose, $Clobber flags

   my ($debug, $verbose, $clobber);
   $debug   = ($Debug)   ? " -debug"   : "";
   $verbose = ($Verbose) ? ""          : " -quiet";
   $clobber = ($Clobber) ? " -clobber" : "";

   $MriToTal     .= "$debug$verbose$clobber";

   if (! -d $ModelDir) {
       $ModelDir = MNI::DataDir::dir($ModelDir);
   }

   ($inputfile, $xfmfile) = @argv;

   &CheckOutputDirs ($TmpDir);
}
