#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

private  void  chamfer_volume(
    Volume   volume );
private  void  peel_volume(
    Volume   volume,
    Real     distance );

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: %s input.mnc output.mnc  distance\n\
\n\
\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING        input_filename;
    Volume        volume;
    int           x, y, z, sizes[N_DIMENSIONS], n_steps, step, v_step;
    Real          *deriv[1], deriv_info[3], pos;
    Real          x_step, y_step, z_step, *sum, min_value, max_value;
    Real          voxel[N_DIMENSIONS], mag, value, hist;
    Real          upper_limit;
    int           *count, n_x_steps, n_y_steps, n_z_steps;
    int           ignore_threshold;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_real_argument( 0, &x_step ) ||
        !get_real_argument( 0, &y_step ) ||
        !get_real_argument( 0, &z_step ) ||
        !get_int_argument( 0, &n_steps ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 1e30, &upper_limit );
    (void) get_int_argument( 0, &ignore_threshold );

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &volume,
                      NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );
    get_volume_real_range( volume, &min_value, &max_value );

    ALLOC( count, n_steps );
    ALLOC( sum, n_steps );

    for_less( step, 0, n_steps )
    {
        count[step] = 0;
        sum[step] = 0.0;
    }

    n_x_steps = sizes[X] / x_step;
    n_y_steps = sizes[Y] / y_step;
    n_z_steps = sizes[Z] / z_step;

    for_less( x, 0, n_x_steps )
    for_less( y, 0, n_y_steps )
    for_less( z, 0, n_z_steps )
    {
        voxel[X] = (Real) x * x_step;
        voxel[Y] = (Real) y * y_step;
        voxel[Z] = (Real) z * z_step;

        deriv[0] = deriv_info;
        (void) evaluate_volume( volume, voxel, NULL, 0, FALSE, 0.0,
                                &value, deriv, NULL );

        mag = deriv_info[0] * deriv_info[0] +
              deriv_info[1] * deriv_info[1] +
              deriv_info[2] * deriv_info[2];

/*
        if( mag > 0.0 )
            mag = sqrt( mag );
*/

        v_step = (int)
            ((value - min_value) / (max_value - min_value) * (Real) n_steps);
        if( v_step == n_steps )
            v_step = n_steps-1;

        ++count[v_step];
        sum[v_step] += mag;
    }

    for_less( step, 0, n_steps )
    {
        pos = min_value + ((Real) step + 0.5) * (max_value - min_value) /
                   (Real) n_steps;

        if( count[step] <= ignore_threshold )
            hist = 0.0;
        else
            hist = sum[step] / (Real) count[step];

        if( pos < upper_limit )
            print( "%g %g\n", pos, hist );
    }

    FREE( count );
    FREE( sum );

    return( 0 );
}
