#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void   get_n_tags(
    polygons_struct  *surface,
    polygons_struct  *dest_surface,
    lines_struct     *lines,
    int              n_tags,
    Real             **tags );

private  void  usage(
    STRING   executable )
{
    static  STRING  usage_str = "\n\
Usage: %s  output.tag  n_along surface1.obj surface2.obj A_file1 A_file2 ... B_file1 B_file2 B_file3\n\
\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               output_filename, filename, *filenames;
    STRING               surface1_filename, surface2_filename;
    File_formats         format;
    int                  n_tags, n_tags_per_line, n_objects, poly;
    int                  i, t, n_pairs, n_files, obj_index;
    Real                 **tags1, **tags2;
    object_struct        **objects1, **objects2, **objects;
    lines_struct         *lines1, *lines2;
    polygons_struct      *surface1, *surface2;
    Point                point, point_on_sulcus;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) ||
        !get_int_argument( 0, &n_tags_per_line ) ||
        !get_string_argument( NULL, &surface1_filename ) ||
        !get_string_argument( NULL, &surface2_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( surface1_filename, &format, &n_objects, &objects )
                                 != OK || n_objects != 1 ||
        get_object_type(objects[0]) != POLYGONS )
    {
        print_error( "Surface file must contain polygons.\n" );
        return( 0 );
    }
    surface1 = get_polygons_ptr( objects[0] );

    if( input_graphics_file( surface2_filename, &format, &n_objects, &objects )
                                 != OK || n_objects != 1 ||
        get_object_type(objects[0]) != POLYGONS )
    {
        print_error( "Surface file must contain polygons.\n" );
        return( 0 );
    }

    surface2 = get_polygons_ptr( objects[0] );

    n_files = 0;
    filenames = NULL;
    while( get_string_argument( NULL, &filename ) )
    {
        ADD_ELEMENT_TO_ARRAY( filenames, n_files, filename, DEFAULT_CHUNK_SIZE);
    }

    if( n_files % 2 != 0 )
    {
        print_error( "Must have same number of sulcal files.\n" );
        return( 1 );
    }

    n_pairs = n_files / 2;
    n_tags = 0;
    tags1 = NULL;
    tags2 = NULL;

    create_polygons_bintree( surface1,
                             ROUND( (Real) surface1->n_items * 0.2 ) );
    create_polygons_bintree( surface2,
                             ROUND( (Real) surface2->n_items * 0.2 ) );

    for_less( i, 0, n_pairs )
    {
        if( input_graphics_file( filenames[i], &format, &n_objects, &objects1 )
                                 != OK )
        {
            return( 1 );
        }

        if( n_objects != 1 || get_object_type( objects1[0] ) != LINES )
        {
            delete_object_list( n_objects, objects1 );
            continue;
        }

        if( input_graphics_file( filenames[i+n_pairs], &format, &n_objects,
                                 &objects2 ) != OK )
        {
            return( 1 );
        }

        if( n_objects != 1 || get_object_type( objects2[0] ) != LINES )
        {
            delete_object_list( 1, objects1 );
            delete_object_list( n_objects, objects2 );
            continue;
        }

        print( "Pairs: %s\t%s\n", filenames[i], filenames[i+n_pairs] );
        lines1 = get_lines_ptr( objects1[0] );
        lines2 = get_lines_ptr( objects2[0] );

        create_lines_bintree( lines2, ROUND( (Real) lines2->n_points * 3.0 ) );

        SET_ARRAY_SIZE( tags1, n_tags, n_tags + n_tags_per_line,
                        DEFAULT_CHUNK_SIZE );
        SET_ARRAY_SIZE( tags2, n_tags, n_tags + n_tags_per_line,
                        DEFAULT_CHUNK_SIZE );

        for_less( t, n_tags, n_tags + n_tags_per_line )
        {
            ALLOC( tags1[t], N_DIMENSIONS );
            ALLOC( tags2[t], N_DIMENSIONS );
        }

        get_n_tags( surface1, NULL, lines1, n_tags_per_line, &tags1[n_tags] );

        for_less( t, n_tags, n_tags + n_tags_per_line )
        {
            fill_Point( point, tags1[t][X], tags1[t][Y], tags1[t][Z] );
            (void) find_closest_point_on_object( &point, objects2[0],
                                                 &obj_index, &point_on_sulcus );

            poly = find_closest_polygon_point( &point_on_sulcus, surface2,
                                               &point );

            map_point_between_polygons( surface2, poly, &point,
                                        surface1, &point );

            tags2[t][X] = RPoint_x(point);
            tags2[t][Y] = RPoint_y(point);
            tags2[t][Z] = RPoint_z(point);
        }

        n_tags += n_tags_per_line;

        delete_object_list( 1, objects1 );
        delete_object_list( 1, objects2 );
    }

    (void) output_tag_file( output_filename, "Paired Sulcal homologous points",
                            2, n_tags, tags1, tags2, NULL, NULL, NULL, NULL );

    return( 0 );
}

private  int  get_position(
    Real   distance,
    int    n_intervals,
    Real   int_length[],
    Real   *fraction )
{
    int   index;

    index = 0;
    while( index < n_intervals-1 && distance >= int_length[index] )
    {
        distance -= int_length[index];
        ++index;
    }

    *fraction = distance / int_length[index];

    return( index );
}

private  void   get_n_tags(
    polygons_struct  *surface,
    polygons_struct  *dest_surface,
    lines_struct     *lines,
    int              n_tags,
    Real             **tags )
{
    int      size, i, p0, p1, ind, poly;
    Point    poly_point, point;
    Real     *lengths, total_length, fraction;

    size = GET_OBJECT_SIZE( *lines, 0 );

    ALLOC( lengths, size-1 );
    total_length = 0.0;
    for_less( i, 0, size-1 )
    {
        p0 = lines->indices[i];
        p1 = lines->indices[i+1];
        lengths[i] = distance_between_points( &lines->points[p0],
                                              &lines->points[p1] );
        total_length += lengths[i];
    }

    for_less( i, 0, n_tags )
    {
        ind = get_position( (Real) i / (Real) (n_tags-1) * total_length,
                            size-1, lengths, &fraction );
        p0 = lines->indices[ind];
        p1 = lines->indices[ind+1];
        INTERPOLATE_POINTS( point, lines->points[p0], lines->points[p1],
                            fraction );
        poly = find_closest_polygon_point( &point, surface, &poly_point );

        if( dest_surface != NULL )
        {
            map_point_between_polygons( surface, poly, &poly_point,
                                        dest_surface, &poly_point );
        }

        tags[i][X] = RPoint_x(poly_point);
        tags[i][Y] = RPoint_y(poly_point);
        tags[i][Z] = RPoint_z(poly_point);
    }

    FREE( lengths );
}
