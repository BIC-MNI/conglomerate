#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR  0.3

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *file;
    STRING           input1_filename, input2_filename, output_filename;
    int              i, n_objects, which;
    File_formats     format;
    object_struct    **object_list[2];
    polygons_struct  *polygons[2];
    Point            point;
    Real             dist, max_d_A[2], d_A_B;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input1_filename ) ||
        !get_string_argument( NULL, &input2_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error(
          "Usage: %s  input1.obj  input2.obj\n", argv[0] );
        return( 1 );
    }

    if( input_graphics_file( input1_filename, &format, &n_objects,
                             &object_list[0] ) != OK || n_objects != 1 ||
        get_object_type(object_list[0][0]) != POLYGONS )
    {
        print_error( "Error reading %s.\n", input1_filename );
        return( 1 );
    }

    polygons[0] = get_polygons_ptr( object_list[0][0] );

    if( input_graphics_file( input2_filename, &format, &n_objects,
                             &object_list[1] ) != OK || n_objects != 1 ||
        get_object_type(object_list[1][0]) != POLYGONS )
    {
        print_error( "Error reading %s.\n", input2_filename );
        return( 1 );
    }

    polygons[1] = get_polygons_ptr( object_list[1][0] );

    for_less( which, 0, 2 )
    {
        create_polygons_bintree( polygons[1-which],
                                 ROUND( (Real) polygons[1-which]->n_items *
                                        BINTREE_FACTOR ) );

        max_d_A[which] = -1.0;

        for_less( i, 0, polygons[which]->n_points )
        {
            (void) find_closest_polygon_point( &polygons[which]->points[i],
                                               polygons[1-which],
                                               &point );

            dist = distance_between_points( &polygons[which]->points[i], &point );
            if( i == 0 || dist > max_d_A[which] )
                max_d_A[which] = dist;
        }

        delete_bintree_if_any( &polygons[1-which]->bintree );
    }

    d_A_B = MIN( max_d_A[0], max_d_A[1] );

    file = fopen( output_filename, "w" );
    (void) fprintf( file, "%g\n", d_A_B );
    fclose( file );

    return( 0 );
}
