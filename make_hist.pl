#!/usr/local/bin/perl5 -w

    $volume = shift;
    $output = shift;

    $s = 1;
    $out = `/nil/david/Source/Batch_processing/gradient_histogram $volume $s $s $s 200 1000 | grep -v Reading`;

    @out = split( /\s+/, $out );

    $n_points = @out / 2;

    print( "N points: $n_points\n" );

    for( $i = 0;  $i < $n_points;  ++$i )
    {
        $x = $out[2*$i];
        $y = $out[2*$i+1];
        if( $y == 0 )
            { next; }
        if( $i == 0 )
        {
            $x_min = $x;
            $x_max = $x;
            $y_min = $y;
            $y_max = $y;
        }
        else
        {
            if( $x < $x_min )
            {
                $x_min = $x;
            }
            elsif( $x > $x_max )
            {
                $x_max = $x;
            }

            if( $y < $y_min )
            {
                $y_min = $y;
            }
            elsif( $y > $y_max )
            {
                $y_max = $y;
            }
        }
    }

    $tmp_out = "/tmp/out_${$}.values";

    open( OUT, ">$tmp_out" ) || die;

    for( $i = 0;  $i < $n_points;  ++$i )
    {
        $x = $out[2*$i];
        $y = $out[2*$i+1];

        if( $y > 0 )
        {
            print( OUT "$x $y\n" );
        }
    }

    close( OUT );

    system( "plot.pl $output $tmp_out 0 $x_max 0 $y_max Value Avg-GradMag Avg-GradMag-vs-Value 1 2 -" );

    unlink( $tmp_out );

