#!/bin/csh -f

deform_surface  ../big_data/sphere.fre none 0 0 0 \
       /scratch/david/test /scratch/david/test_out \
       .5 parametric none 0 1 -1 .3 .3 30 0 130 131 + 5 90 0.01 5 0
