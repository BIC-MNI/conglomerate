#include  <bicpl.h>

#define  GRAY_STRING       "gray"
#define  HOT_STRING        "hot"
#define  SPECTRAL_STRING   "spectral"

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_Status               status;
    VIO_Real                 low_threshold, smoothing_distance;
    VIO_Real                 *curvatures;
    VIO_STR               src_filename, dest_filename;
    int                  i, p, n_src_objects, n_dest_objects;
    File_formats         format;
    object_struct        **src_object_list, **dest_object_list;
    polygons_struct      *polygons1, *polygons2;
    Colour_coding_types  coding_type;
    colour_coding_struct colour_coding;
    VIO_STR               coding_type_string;
    VIO_Real                 low, high, min_curvature, max_curvature;
    VIO_BOOL              low_present, high_present;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &src_filename ) ||
        !get_string_argument( "", &dest_filename ) )
    {
        print_error( "Usage: %s  in.obj out.obj [dist] [gray|hot] [lo] [hi]\n",
                     argv[0] );
        print_error( "      [low_threshold]\n" );
        return( 1 );
    }

    (void)  get_real_argument( 0.0, &smoothing_distance );

    (void) get_string_argument( GRAY_STRING, &coding_type_string );
    low_present = get_real_argument( -0.2, &low );
    high_present = get_real_argument( 0.2, &high );

    if( equal_strings( coding_type_string, GRAY_STRING ) )
        coding_type = GRAY_SCALE;
    else if( equal_strings( coding_type_string, HOT_STRING ) )
        coding_type = HOT_METAL;
    else if( equal_strings( coding_type_string, SPECTRAL_STRING ) )
        coding_type = SPECTRAL;
    else
    {
        print( "Invalid coding type: %s\n", coding_type_string );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &low_threshold );

    if( input_graphics_file( src_filename, &format, &n_src_objects,
                                  &src_object_list ) != VIO_OK )
        return( 1 );

    if( input_graphics_file( dest_filename, &format, &n_dest_objects,
                                      &dest_object_list ) != VIO_OK )
        return( 1 );

    print( "%d Objects input.\n", n_src_objects );

    if( n_src_objects != n_dest_objects )
    {
        print( "Different number of objects in the two files.\n" );
        return( 1 );
    }

    for_less( i, 0, n_src_objects )
    {
        if( src_object_list[i]->object_type == POLYGONS &&
            dest_object_list[i]->object_type == POLYGONS &&
            polygons_are_same_topology(
                    get_polygons_ptr(src_object_list[i]),
                    get_polygons_ptr(dest_object_list[i]) ) )
        {
            polygons1 = get_polygons_ptr(src_object_list[i]);
            polygons2 = get_polygons_ptr(dest_object_list[i]);
            ALLOC( curvatures, polygons1->n_points );

            if( smoothing_distance > 0.0 )
                compute_polygon_normals( polygons1 );

            get_polygon_vertex_curvatures( polygons1, smoothing_distance,
                                           low_threshold, curvatures );

            if( polygons2->colour_flag != PER_VERTEX_COLOURS )
            {
                polygons2->colour_flag = PER_VERTEX_COLOURS;
                REALLOC( polygons2->colours, polygons2->n_points );
            }

            if( !low_present || !high_present )
            {
                min_curvature = 0.0;
                max_curvature = 0.0;
                for_less( p, 0, polygons1->n_points )
                {
                    if( p == 0 || curvatures[p] < min_curvature )
                        min_curvature = curvatures[p];
                    if( p == 0 || curvatures[p] > max_curvature )
                        max_curvature = curvatures[p];
                }

                if( !low_present )
                    low = min_curvature;
                if( !high_present )
                    high = max_curvature;
            }

            initialize_colour_coding( &colour_coding, coding_type,
                                      BLACK, WHITE, low, high );

            for_less( p, 0, polygons1->n_points )
            {
                polygons2->colours[p] = get_colour_code( &colour_coding,
                                                         curvatures[p] );
            }

            FREE( curvatures );
        }
        else
            print( "Objects don't match.\n" );
    }

    print( "Objects processed.\n" );

    status = output_graphics_file( dest_filename, format,
                                   n_dest_objects, dest_object_list );

    delete_object_list( n_src_objects, src_object_list );

    delete_object_list( n_dest_objects, dest_object_list );

    if( status == VIO_OK )
        print( "Objects output.\n" );

    return( status != VIO_OK );
}
