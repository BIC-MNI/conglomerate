#!/bin/csh -f

    set input_file = /avgbrain/brain/autoreg/kennedy_sharyl_hres.mni
    set output_prefix = /scratch/david/kennedy

    set  low = 75
    set  high = 90

    foreach  low ( 70 75 80 )
        foreach  high ( 90 95 )
            dilate $input_file     ${output_prefix}_${low}_${high}.mnc $low $high 100
        end
    end
