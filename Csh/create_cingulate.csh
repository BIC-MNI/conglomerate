#!/bin/csh -f

    set   doubles = ~david/Tomas/cing_sorted.lst
    set   list = ~david/Tomas/cing.lst

    ~/Batch_processing/create_landmark_full_volume \
              /avgbrain/brain/autoreg/kennedy_sharyl_hres.mni \
              /scratch/david/CING_TOTAL.mnc -1 3 $doubles

    foreach id ( 10 20 11 21 12 22 13 23 30 40 )

        echo "############## doing  $id #######################"

        ~/Batch_processing/create_landmark_full_volume \
              /avgbrain/brain/autoreg/kennedy_sharyl_hres.mni \
              /scratch/david/CING_${id}.mnc $id 3 $list
    end
