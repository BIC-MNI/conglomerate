#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int    argc,
    char   *argv[] )
{
    FILE           *file;
    STRING         filename1, filename2, output_filename;
    int            i, p, n_objects1, n_objects2, n_points1, n_points2;
    Point          *points1, *points2;
    Real           dist;
    File_formats   format;
    object_struct  **object_list1, **object_list2;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &filename1 ) ||
        !get_string_argument( "", &filename2 ) ||
        !get_string_argument( "", &output_filename ) )
    {
        print_error( "Usage: %s  input1.obj input2.obj output.txt\n",
                     argv[0] );
        return( 1 );
    }

    if( input_graphics_file( filename1, &format, &n_objects1, &object_list1 )
                              != OK )
        return( 1 );

    if( input_graphics_file( filename2, &format, &n_objects2, &object_list2 )
                              != OK )
        return( 1 );

    if( n_objects1 != n_objects2 )
    {
        print_error( "Number of objects do not match.\n" );
        return( 1 );
    }

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    for_less( i, 0, n_objects1 )
    {
        n_points1 = get_object_points( object_list1[i], &points1 );
        n_points2 = get_object_points( object_list2[i], &points2 );

        if( n_points1 != n_points2 )
        {
            print_error( "Number of points do not match.\n" );
            return( 1 );
        }

        for_less( p, 0, n_points1 )
        {
            dist = distance_between_points( &points1[p], &points2[p] );

            (void) output_real( file, dist );
            (void) output_newline( file );
        }
    }

    (void) close_file( file );

    delete_object_list( n_objects1, object_list1 );
    delete_object_list( n_objects2, object_list2 );

    return( 0 );
}
