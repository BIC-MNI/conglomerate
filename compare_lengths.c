#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               polygons_filename1, polygons_filename2;
    VIO_File_formats         format;
    int                  poly, size, p1, p2, edge, n_edges;
    int                  n_objects1, n_objects2;
    object_struct        **objects1, **objects2;
    VIO_Real                 sum_x, sum_xx, min_x, max_x, avg, var;
    VIO_Real                 relative, l1, l2, sum1, sum2, scale;
    polygons_struct      *polygons1, *polygons2;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &polygons_filename1 ) ||
        !get_string_argument( NULL, &polygons_filename2 ) )
    {
        print_error(
             "Usage: %s  src_polygons  dest_polygons\n", argv[0] );
        return( 1 );
    }

    if( input_graphics_file( polygons_filename1,
                             &format, &n_objects1, &objects1 ) != VIO_OK )
        return( 1 );

    if( input_graphics_file( polygons_filename2,
                             &format, &n_objects2, &objects2 ) != VIO_OK )
        return( 1 );

    if( n_objects1 != 1 || get_object_type( objects1[0] ) != POLYGONS ||
        n_objects2 != 1 || get_object_type( objects2[0] ) != POLYGONS )
    {
        print_error( "Must specify polygons files.\n" );
        return( 1 );
    }

    polygons1 = get_polygons_ptr( objects1[0] );
    polygons2 = get_polygons_ptr( objects2[0] );

    if( !polygons_are_same_topology( polygons1, polygons2 ) )
    {
        print_error( "Polygons are not same topology.\n" );
        return( 1 );
    }

    sum1 = 0.0;
    sum2 = 0.0;
    n_edges = 0;

    for_less( poly, 0, polygons1->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons1, poly );

        for_less( edge, 0, size )
        {
            p1 = polygons1->indices[POINT_INDEX(polygons1->end_indices,
                                          poly, edge)];
            p2 = polygons1->indices[POINT_INDEX(polygons1->end_indices,
                                          poly, (edge+1) % size)];

            if( p1 < p2 )
            {
                l1 = distance_between_points( &polygons1->points[p1],
                                              &polygons1->points[p2] );
                l2 = distance_between_points( &polygons2->points[p1],
                                              &polygons2->points[p2] );

                sum1 += l1;
                sum2 += l2;
                ++n_edges;
            }
        }
    }

    scale = sum2 / sum1;

    print( "Length scale: %g\n", scale );

    min_x = 0.0;
    max_x = 0.0;
    sum_x = 0.0;
    sum_xx = 0.0;

    for_less( poly, 0, polygons1->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons1, poly );

        for_less( edge, 0, size )
        {
            p1 = polygons1->indices[POINT_INDEX(polygons1->end_indices,
                                          poly, edge)];
            p2 = polygons1->indices[POINT_INDEX(polygons1->end_indices,
                                          poly, (edge+1) % size)];

            if( p1 < p2 )
            {
                l1 = distance_between_points( &polygons1->points[p1],
                                              &polygons1->points[p2] );
                l2 = distance_between_points( &polygons2->points[p1],
                                              &polygons2->points[p2] );

                l2 /= scale;

                relative = (l2 - l1) / l1;

                if( min_x > max_x )
                {
                    min_x = relative;
                    max_x = relative;
                }
                else if( relative < min_x )
                    min_x = relative;
                else if( relative > max_x )
                    max_x = relative;
                                              

                sum_x += relative;
                sum_xx += relative * relative;
            }
        }
    }

    avg = sum_x / (VIO_Real) n_edges;

    var = (sum_xx - sum_x * sum_x / (VIO_Real) n_edges) / (VIO_Real) (n_edges-1);

    print( "Min: %g\n", min_x );
    print( "Max: %g\n", max_x );
    print( "Avg: %g\n", avg );
    print( "Sd : %g\n", sqrt( var ) );

    return( 0 );
}
