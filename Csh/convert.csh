#!/bin/csh -f

    set pet = $1
    set mri = $2

    set tag_in = $3
    set tag_out = $4

    set tmp_file = /tmp/t.tag

    ~/Batch_processing/transform_tags $3 $pet $tmp_file invert
    ~/Batch_processing/transform_tags $tmp_file $mri $tag_out
