#include  <bicpl.h>
#include  <internal_volume_io.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Real           x, y, z, dist, best_dist;
    Point          position, *points;
    Status         status;
    STRING         input_filename;
    int            i, n_objects, ind, n_points;
    File_formats   format;
    object_struct  **object_list;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_real_argument( 0.0, &x ) ||
        !get_real_argument( 0.0, &y ) ||
        !get_real_argument( 0.0, &z ) )
    {
        (void) fprintf( stderr, "Must have a filename argument.\n" );
        return( 1 );
    }

    status = input_graphics_file( input_filename, &format, &n_objects,
                                  &object_list );

    fill_Point( position, x, y, z );

    n_points = get_object_points( object_list[0], &points );

    best_dist = 0.0;
    for_less( i, 0, n_points )
    {
        dist = distance_between_points( &position, &points[i] );
        if( i == 0 || dist < best_dist )
        {
            ind = i;
            best_dist = dist;
        }
    }

    print( "%d: %g %g %g    -- distance = %g\n", ind,
            Point_x(points[ind]),
            Point_y(points[ind]),
            Point_z(points[ind]), best_dist );

    return( status != OK );
}
