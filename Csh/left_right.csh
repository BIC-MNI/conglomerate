#!/bin/csh -f

    set in_dir = ~france/Frequency
    set out_dir = /scratch/david

left_minus_right $in_dir/CING_10.mnc $in_dir/CING_20.mnc $out_dir/CING_10_20.mnc
left_minus_right $in_dir/CING_11.mnc $in_dir/CING_21.mnc $out_dir/CING_11_21.mnc
left_minus_right $in_dir/CING_12.mnc $in_dir/CING_22.mnc $out_dir/CING_12_22.mnc
left_minus_right $in_dir/CING_13.mnc $in_dir/CING_23.mnc $out_dir/CING_13_23.mnc
left_minus_right $in_dir/CING_30.mnc $in_dir/CING_40.mnc $out_dir/CING_30_40.mnc
