#!/bin/csh -f

    set surface = $1:r

    create_tetra ${surface}_sphere 0 0 0 60 70 50 8192
    colour_curvature ${surface} ${surface}_sphere .1
