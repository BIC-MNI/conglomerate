#!/bin/csh -f

    foreach file ( callosum.tag cing_l.tag cing_r.tag )
        /tmp/convert.csh  /scratch/david/adolph.xfm \
            /scratch/tomas/talairach/adolph_marc_hres.xfm \
            /@/yorick/disk2/tomas/landmarks/frenky/newnaim/adolph_026817/$file \
            /scratch/david/adolph_$file

        /tmp/convert.csh  /scratch/david/burnett.xfm \
            /scratch/tomas/talairach/burnett_scott_hres.xfm \
            /@/yorick/disk2/tomas/landmarks/frenky/newnaim/burnett_026014/$file\
            /scratch/david/burnett_$file

        /tmp/convert.csh  /scratch/david/pecknold.xfm \
            /scratch/tomas/talairach/pecknold_sean_hres.xfm \
            /@/yorick/disk2/tomas/landmarks/frenky/newnaim/pecknold_025958/$file \
            /scratch/david/pecknold_$file

        /tmp/convert.csh  /scratch/david/slater.xfm \
            /scratch/tomas/talairach/slater_mike_hres.xfm \
            /@/yorick/disk2/tomas/landmarks/frenky/newnaim/slater_026721/$file \
            /scratch/david/slater_$file

        /tmp/convert.csh  /scratch/david/uribe.xfm \
            /scratch/tomas/talairach/uribe_hector_hres.xfm \
            /@/yorick/disk2/tomas/landmarks/frenky/newnaim/uribe_025951/$file \
            /scratch/david/uribe_$file

        /tmp/convert.csh  /scratch/david/velican.xfm \
            /scratch/tomas/talairach/velican_marc_hres.xfm \
            /@/yorick/disk2/tomas/landmarks/frenky/newnaim/velican_025943/$file \
            /scratch/david/velican_$file
    end
