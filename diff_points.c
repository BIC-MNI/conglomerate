#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Status         status;
    char           *filename1, *filename2;
    int            i, p, n_objects1, n_objects2, n_points1, n_points2;
    Point          *points1, *points2;
    Real           dist, max_dist;
    File_formats   format;
    object_struct  **object_list1, **object_list2;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &filename1 ) ||
        !get_string_argument( "", &filename2 ) )
    {
        (void) fprintf( stderr, "Must have two filename argument.\n" );
        return( 1 );
    }

    status = input_graphics_file( filename1, &format, &n_objects1,
                                  &object_list1 );

    if( status == OK )
        status = input_graphics_file( filename2, &format, &n_objects2,
                                      &object_list2 );

    if( n_objects1 != n_objects2 )
    {
        print( "Number of objects do not match.\n" );
        return( 1 );
    }

    if( status == OK )
    {
        for_less( i, 0, n_objects1 )
        {
            n_points1 = get_object_points( object_list1[i], &points1 );
            n_points2 = get_object_points( object_list2[i], &points2 );
            if( n_points1 != n_points2 )
            {
                print( "Number of points do not match.\n" );
                return( 1 );
            }
            max_dist = 0.0;

            for_less( p, 0, n_points1 )
            {
                dist = distance_between_points( &points1[p], &points2[p] );
                if( p == 0 || dist > max_dist )
                    max_dist = dist;
            }

            print( "Max Dist: %g\n", max_dist );
        }
    }

    if( status == OK )
        delete_object_list( n_objects1, object_list1 );

    if( status == OK )
        delete_object_list( n_objects2, object_list2 );

    return( status != OK );
}
