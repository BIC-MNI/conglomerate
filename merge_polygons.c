#include <internal_volume_io.h>
#include <bicpl.h>

private  void   merge_polygons(
    polygons_struct    *in1,
    polygons_struct    *in2,
    int                translation1[],
    int                translation2[],
    polygons_struct    *out2 );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input1.obj  input2.obj input.trans output.obj\n\
\n\
     Merges two polygons into one.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    FILE             *file;
    STRING           input1_filename, input2_filename, output_filename;
    STRING           translation_filename;
    int              n_objects, *translation1, *translation2;
    int              p;
    File_formats     format;
    object_struct    **object_list, *object;
    polygons_struct  *polygons1, *polygons2;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input1_filename ) ||
        !get_string_argument( NULL, &input2_filename ) ||
        !get_string_argument( NULL, &translation_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( input1_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects < 1 ||
        get_object_type( object_list[0] ) != POLYGONS )
    {
        print_error( "File must have a polygons structure.\n" );
        return( 1 );
    }

    polygons1 = get_polygons_ptr( object_list[0] );

    if( input_graphics_file( input2_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects < 1 ||
        get_object_type( object_list[0] ) != POLYGONS )
    {
        print_error( "File must have a polygons structure.\n" );
        return( 1 );
    }

    polygons2 = get_polygons_ptr( object_list[0] );

    object = create_object( POLYGONS );
    initialize_polygons( get_polygons_ptr(object), WHITE, NULL );

    ALLOC( translation1, polygons1->n_points );
    ALLOC( translation2, polygons2->n_points );

    if( open_file( translation_filename, READ_FILE, BINARY_FORMAT,
                   &file ) != OK )
        return( 1 );

    for_less( p, 0, polygons1->n_points )
    {
        if( io_int( file, READ_FILE, BINARY_FORMAT, &translation1[p] ) !=
                    OK )
            return( 1 );
    }

    for_less( p, 0, polygons2->n_points )
    {
        if( io_int( file, READ_FILE, BINARY_FORMAT, &translation2[p] ) !=
                    OK )
            return( 1 );
    }

    (void) close_file( file );

    merge_polygons( polygons1, polygons2, translation1, translation2,
                    get_polygons_ptr( object ) );

    (void) output_graphics_file( output_filename, format, 1, &object );

    return( 0 );
}

private  void   merge_polygons(
    polygons_struct    *in1,
    polygons_struct    *in2,
    int                translation1[],
    int                translation2[],
    polygons_struct    *out )
{
    int               n_points, *translation[2], p, poly, size, vertex;
    int               n_indices, ind, new_index, point_index;
    int               n_pairs;
    Real              *x1, *y1, *x2, *y2, x_trans, y_trans;
    Point             point;
    polygons_struct   in[2];
    int               *pair_index;
    Transform_2d      transform_2d;

    in[0] = *in1;
    in[1] = *in2;
    translation[0] = translation1;
    translation[1] = translation2;

    n_points = -1;
    for_less( p, 0, in1->n_points )
        n_points = MAX( n_points, translation1[p] );
    for_less( p, 0, in2->n_points )
        n_points = MAX( n_points, translation2[p] );
    ++n_points;

    out->n_points = n_points;
    ALLOC( out->points, n_points );
    ALLOC( out->normals, n_points );
    ALLOC( pair_index, n_points );

    for_less( p, 0, n_points )
        pair_index[p] = 0;

    for_less( ind, 0, 2 )
    {
        for_less( p, 0, in[ind].n_points )
        {
            if( translation[ind][p] >= 0 )
                ++pair_index[translation[ind][p]];
        }
    }

    n_pairs = 0;
    for_less( p, 0, n_points )
    {
        if( pair_index[p] == 2 )
        {
            pair_index[p] = n_pairs;
            ++n_pairs;
        }
        else
            pair_index[p] = -1;
    }

    ALLOC( x1, n_pairs );
    ALLOC( y1, n_pairs );
    ALLOC( x2, n_pairs );
    ALLOC( y2, n_pairs );

    for_less( p, 0, in[0].n_points )
    {
        if( translation[0][p] >= 0 && pair_index[translation[0][p]] >= 0 )
        {
            x1[pair_index[translation[0][p]]] = RPoint_x(in[0].points[p]);
            y1[pair_index[translation[0][p]]] = RPoint_y(in[0].points[p]);
        }
    }

    for_less( p, 0, in[1].n_points )
    {
        if( translation[1][p] >= 0 && pair_index[translation[1][p]] >= 0 )
        {
            x2[pair_index[translation[1][p]]] = RPoint_x(in[1].points[p]);
            y2[pair_index[translation[1][p]]] = RPoint_y(in[1].points[p]);
        }
    }

    get_least_squares_transform_2d( n_pairs, x2, y2, x1, y1, &transform_2d );

    FREE( x1 );
    FREE( y1 );
    FREE( x2 );
    FREE( y2 );

    for_less( p, 0, n_points )
    {
        fill_Point( out->points[p], 0.0, 0.0, 0.0 );
        fill_Point( out->normals[p], 0.0, 0.0, 0.0 );
    }

    n_indices = 0;

    for_less( ind, 0, 2 )
    {
        for_less( point_index, 0, in[ind].n_points )
        {
            point = in[ind].points[point_index];
            new_index = translation[ind][point_index];

            if( ind == 1 )
            {
                transform_point_2d( &transform_2d,
                                    RPoint_x(point), RPoint_y(point),
                                    &x_trans, &y_trans );

                fill_Point( point, x_trans, y_trans, RPoint_z(point) );
            }

            ADD_POINTS( out->points[new_index],
                        out->points[new_index],
                        point );
            ADD_VECTORS( out->normals[new_index],
                         out->normals[new_index],
                         in[ind].normals[point_index] );
        }

        for_less( poly, 0, in[ind].n_items )
        {
            size = GET_OBJECT_SIZE( in[ind], poly );
            for_less( vertex, 0, size )
            {
                point_index = in[ind].indices[POINT_INDEX(in[ind].end_indices,
                                               poly,vertex)];

                new_index = translation[ind][point_index];

                ADD_ELEMENT_TO_ARRAY( out->indices, n_indices,
                                      new_index, DEFAULT_CHUNK_SIZE );
            }

            ADD_ELEMENT_TO_ARRAY( out->end_indices, out->n_items,
                                  n_indices, DEFAULT_CHUNK_SIZE );
        }
    }

    for_less( p, 0, n_points )
    {
        if( pair_index[p] >= 0 )
            SCALE_POINT( out->points[p], out->points[p], 0.5 );
        NORMALIZE_VECTOR( out->normals[p], out->normals[p] );
    }

    FREE( pair_index );
}
