#include  <volume_io.h>
#include  <bicpl.h>

static  int  get_stats_for_one_file(
    int             n_objects,
    object_struct   *object_list[],
    int             structure_id,
    VIO_Real            y_min_range,
    VIO_Real            y_max_range,
    VIO_Real            *x_min,
    VIO_Real            *x_max,
    VIO_Real            *x_mean,
    VIO_Real            *y_min,
    VIO_Real            *y_max,
    VIO_Real            *y_mean,
    VIO_Real            *z_min,
    VIO_Real            *z_max,
    VIO_Real            *z_mean,
    int             *total_in_file );

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_Real                 x_min, x_max, y_min, y_max, z_min, z_max;
    VIO_Real                 x_mean, y_mean, z_mean;
    VIO_STR               filename;
    int                  n_objects, total_in_file;
    object_struct        **object_list;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &filename ) )
    {
        print( "Usage:  %s  tag_file\n", argv[0] );
        return( 1 );
    }

    if( input_objects_any_format( (VIO_Volume) NULL, filename,
                                  WHITE, 1.0, BOX_MARKER,
                                  &n_objects, &object_list ) != VIO_OK )
        return( 1 );

    (void) get_stats_for_one_file( n_objects, object_list,
                                   -1,
                                   1.0, 0.0,
                                   &x_min, &x_max, &x_mean,
                                   &y_min, &y_max, &y_mean,
                                   &z_min, &z_max, &z_mean,
                                   &total_in_file );

    print( "X   min: %g    mean: %g   max: %g\n", x_min, x_mean, x_max );
    print( "Y   min: %g    mean: %g   max: %g\n", y_min, y_mean, y_max );
    print( "Z   min: %g    mean: %g   max: %g\n", z_min, z_mean, z_max );

    delete_object_list( n_objects, object_list );

    return( 0 );
}

static  int  get_stats_for_one_file(
    int             n_objects,
    object_struct   *object_list[],
    int             structure_id,
    VIO_Real            y_min_range,
    VIO_Real            y_max_range,
    VIO_Real            *x_min,
    VIO_Real            *x_max,
    VIO_Real            *x_mean,
    VIO_Real            *y_min,
    VIO_Real            *y_max,
    VIO_Real            *y_mean,
    VIO_Real            *z_min,
    VIO_Real            *z_max,
    VIO_Real            *z_mean,
    int             *total_in_file )
{
    int             i, n_samples;
    VIO_Real            *x_positions, *y_positions, *z_positions;
    VIO_Real            std_dev, median;
    marker_struct   *marker;

    *total_in_file = 0;
    x_positions = NULL;
    y_positions = NULL;
    z_positions = NULL;
    n_samples = 0;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            ++(*total_in_file);
            marker = get_marker_ptr( object_list[i] );

            if( (structure_id < 0 ||
                 marker->structure_id == structure_id ||
                 marker->structure_id == structure_id + 1000) &&
                (y_min_range >= y_max_range ||
                 (VIO_Real) Point_y(marker->position) >= y_min_range &&
                 (VIO_Real) Point_y(marker->position) <= y_max_range ) )
            {
                SET_ARRAY_SIZE( x_positions, n_samples, n_samples+1,
                                DEFAULT_CHUNK_SIZE );
                SET_ARRAY_SIZE( y_positions, n_samples, n_samples+1,
                                DEFAULT_CHUNK_SIZE );
                SET_ARRAY_SIZE( z_positions, n_samples, n_samples+1,
                                DEFAULT_CHUNK_SIZE );

                x_positions[n_samples] = (VIO_Real) Point_x(marker->position);
                y_positions[n_samples] = (VIO_Real) Point_y(marker->position);
                z_positions[n_samples] = (VIO_Real) Point_z(marker->position);

                ++n_samples;
            }
        }
    }

    if( n_samples > 0 )
    {
        compute_statistics( n_samples, x_positions, x_min, x_max,
                            x_mean, &std_dev, &median );
        compute_statistics( n_samples, y_positions, y_min, y_max,
                            y_mean, &std_dev, &median );
        compute_statistics( n_samples, z_positions, z_min, z_max,
                            z_mean, &std_dev, &median );

        FREE( x_positions );
        FREE( y_positions );
        FREE( z_positions );
    }

    return( n_samples );
}
