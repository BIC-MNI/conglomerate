#!/bin/csh -f

    set error_file = /scratch/david/error

    foreach id ( -1 10 20 11 21 12 22 13 23 30 40 -2 -3 )

        set y = ( 1 -1 )
        if( $id == -2 ) then
            set id = 30
            set y = (12 30 )
        endif
        if( $id == -3 ) then
            set id = 40
            set y = (12 30 )
        endif

        if( $id == 10 )  echo ""
        if( $id == 11 )  echo ""
        if( $id == 12 )  echo ""
        if( $id == 13 )  echo ""
        if( $id == 20 )  echo ""
        if( $id == 30 )  echo ""
        echo ""
        echo "###################### $id #########################"
        echo ""

        ~david/Batch_processing/tag_statistics $id $y $error_file $*
    end
