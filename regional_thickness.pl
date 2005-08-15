#! /usr/bin/env perl

# a script to get regional averages of a subjects cortical thickness.

use strict;

my $usage = "$0 input_thickness.txt input_labels.txt output.txt\n";

my $thickness = shift @ARGV or die $usage;
my $labels = shift @ARGV or die $usage;
my $output = shift @ARGV or die $usage;

my %region_thickness;
my %region_counts;

open THICKNESS, $thickness or die "Error opening $thickness: $!\n";
open LABELS, $labels or die "Error opening $labels: $!\n";
open OUT, ">$output" or die "Error opening $output for writing: $!\n";

while (<THICKNESS>) {
    my $current_thickness = $_;
    my $current_label = <LABELS>;
    chomp $current_thickness;
    chomp $current_label;

    if ($region_thickness{$current_label}) {
	$region_thickness{$current_label} = $region_thickness{$current_label} + 
	    $current_thickness;
	$region_counts{$current_label} = $region_counts{$current_label} + 1;
    }
    else {
	$region_thickness{$current_label} = $current_thickness;
	$region_counts{$current_label} = 1;
    }
}

foreach my $region (keys %region_thickness) {
    my $avg = $region_thickness{$region} / (1 + $region_counts{$region});
    print OUT "$region $avg\n";
}

