#include <volume_io/internal_volume_io.h>
#include <bicpl.h>

private  int  refine_mesh(
    Point              *length_points[],
    polygons_struct    *polygons,
    Real               max_length,
    polygons_struct    *new_polygons );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  lengths.obj  input.obj output.obj  max_length\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING             input_filename, output_filename;
    STRING             length_filename;
    int                n_objects, n_len_objects, n_done, i;
    File_formats       format;
    object_struct      **object_list, **len_object_list;
    polygons_struct    *polygons, *length, new_polygons;
    Point              *length_points;
    Real               max_length;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &length_filename ) ||
        !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &max_length ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_graphics_file( length_filename, &format, &n_len_objects,
                             &len_object_list ) != OK ||
        n_len_objects < 1 || get_object_type(len_object_list[0]) != POLYGONS )
    {
        print_error( "Error in input file.\n" );
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK ||
        n_objects < 1 || get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Error in input file.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );
    length = get_polygons_ptr( len_object_list[0] );

    if( !objects_are_same_topology( polygons->n_points,
                                    polygons->n_items,
                                    polygons->end_indices,
                                    polygons->indices,
                                    length->n_points,
                                    length->n_items,
                                    length->end_indices,
                                    length->indices ) )
    {
        print_error( "Mismatching topology.\n" );
        return( 1 );
    }

    SET_ARRAY_SIZE( length_points, 0, length->n_points, DEFAULT_CHUNK_SIZE );
    for_less( i, 0, length->n_points )
        length_points[i] = length->points[i];

    do
    {
        n_done = refine_mesh( &length_points, polygons, max_length,
                              &new_polygons );

        delete_polygons( polygons );
        *polygons = new_polygons;
    }
    while( n_done > 0 );

    print( "Resampled into %d polygons.\n", polygons->n_items );

    (void) output_graphics_file( output_filename, format, n_objects,
                                 object_list );

    return( 0 );
}

private  BOOLEAN  lookup_edge_midpoint(
    hash2_table_struct    *edge_lookup,
    int                   p0,
    int                   p1,
    int                   *midpoint )
{
    int     k0, k1;

    k0 = MIN( p0, p1 );
    k1 = MAX( p0, p1 );

    return( lookup_in_hash2_table( edge_lookup, k0, k1, (void *) midpoint ) );
}

private  void  subdivide_edge(
    hash2_table_struct *edge_lookup,
    Real               normalized_length,
    int                p1,
    int                p2,
    polygons_struct    *new_polygons,
    Point              *length_points[] )
{
    int    midpoint;
    Point  mid;

    midpoint = new_polygons->n_points;
    INTERPOLATE_POINTS( mid, new_polygons->points[p1],
                             new_polygons->points[p2], 0.5 );
    ADD_ELEMENT_TO_ARRAY( new_polygons->points, new_polygons->n_points,
                          mid, DEFAULT_CHUNK_SIZE );

    INTERPOLATE_POINTS( mid, (*length_points)[p1], (*length_points)[p2], 0.5 );
    --new_polygons->n_points;
    ADD_ELEMENT_TO_ARRAY( *length_points, new_polygons->n_points,
                          mid, DEFAULT_CHUNK_SIZE );

    insert_in_hash2_table( edge_lookup, MIN(p1,p2), MAX(p1,p2),
                           (void *) &midpoint );
}

public  void  add_polygons(
    int                p0,
    int                p1,
    int                p2,
    Point              length_points[],
    hash2_table_struct *edge_lookup,
    polygons_struct    *new_polygons,
    int                *n_indices )
{
    int      tri, edge, mid[3], n_present, indices[3], i0, i1, i2;
    int      n_new_triangles, new_indices[4][3], offset;
    BOOLEAN  mid_present[3];

    indices[0] = p0;
    indices[1] = p1;
    indices[2] = p2;

    n_present = 0;
    for_less( edge, 0, 3 )
    {
        mid_present[edge] = lookup_edge_midpoint( edge_lookup, indices[edge],
                                             indices[(edge+1)%3], &mid[edge] );

        if( mid_present[edge] )
            ++n_present;
    }

    if( n_present == 3 )
    {
        n_new_triangles = 4;
        new_indices[0][0] = p0;
        new_indices[0][1] = mid[0];
        new_indices[0][2] = mid[2];
        new_indices[1][0] = mid[0];
        new_indices[1][1] = p1;
        new_indices[1][2] = mid[1];
        new_indices[2][0] = mid[0];
        new_indices[2][1] = mid[1];
        new_indices[2][2] = mid[2];
        new_indices[3][0] = mid[2];
        new_indices[3][1] = mid[1];
        new_indices[3][2] = p2;
    }
    else if( n_present == 2 )
    {
        offset = 0;
        while( mid_present[offset] )
            ++offset;

        n_new_triangles = 3;

        i0 = offset;
        i1 = (offset+1) % 3;
        i2 = (offset+2) % 3;

        if( sq_distance_between_points( &length_points[indices[i0]],
                                        &length_points[mid[i1]] ) <
            sq_distance_between_points( &length_points[indices[i1]],
                                        &length_points[mid[i2]] ) )
        {
            new_indices[0][0] = indices[i0];
            new_indices[0][1] = indices[i1];
            new_indices[0][2] = mid[i1];
            new_indices[1][0] = indices[i0];
            new_indices[1][1] = mid[i1];
            new_indices[1][2] = mid[i2];
        }
        else
        {
            new_indices[0][0] = indices[i0];
            new_indices[0][1] = indices[i1];
            new_indices[0][2] = mid[i2];
            new_indices[1][0] = indices[i1];
            new_indices[1][1] = mid[i1];
            new_indices[1][2] = mid[i2];
        }
        new_indices[2][0] = mid[i2];
        new_indices[2][1] = mid[i1];
        new_indices[2][2] = indices[i2];
    }
    else if( n_present == 1 )
    {
        offset = 0;
        while( !mid_present[offset] )
            ++offset;

        n_new_triangles = 2;

        i0 = offset;
        i1 = (offset+1) % 3;
        i2 = (offset+2) % 3;

        new_indices[0][0] = indices[i0];
        new_indices[0][1] = mid[i0];
        new_indices[0][2] = indices[i2];
        new_indices[1][0] = mid[i0];
        new_indices[1][1] = indices[i1];
        new_indices[1][2] = indices[i2];
    }
    else if( n_present == 0 )
    {
        n_new_triangles = 0;

        ADD_ELEMENT_TO_ARRAY( new_polygons->indices, *n_indices,
                              p0, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( new_polygons->indices, *n_indices,
                              p1, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( new_polygons->indices, *n_indices,
                              p2, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( new_polygons->end_indices, new_polygons->n_items,
                              *n_indices, DEFAULT_CHUNK_SIZE );
    }

    for_less( tri, 0, n_new_triangles )
    {
        add_polygons( new_indices[tri][0],
                      new_indices[tri][1],
                      new_indices[tri][2],
                      length_points, edge_lookup, new_polygons, n_indices );
    }
}

private  int  refine_mesh(
    Point              *length_points[],
    polygons_struct    *polygons,
    Real               max_length,
    polygons_struct    *new_polygons )
{
    int                  n_indices, p1, p2, size, point, edge, midpoint, poly;
    Real                 normalized_length;
    hash2_table_struct   edge_lookup;

    initialize_polygons( new_polygons, WHITE, NULL );

    SET_ARRAY_SIZE( new_polygons->points, 0, polygons->n_points,
                    DEFAULT_CHUNK_SIZE );

    new_polygons->n_points = polygons->n_points;

    for_less( point, 0, polygons->n_points )
        new_polygons->points[point] = polygons->points[point];

    initialize_hash2_table( &edge_lookup, 10 * polygons->n_points,
                            sizeof(int), 0.25, 0.125 );

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );
        if( size != 3 )
        {
            print_error( "Cannot handle non-triangles\n" );
        }

        for_less( edge, 0, size )
        {
            p1 =polygons->indices[POINT_INDEX(polygons->end_indices,poly,edge)];
            p2 =polygons->indices[POINT_INDEX(polygons->end_indices,poly,
                                 (edge+1)%size)];

            normalized_length = distance_between_points( &(*length_points)[p1],
                                                         &(*length_points)[p2])/
                                  max_length;

            if( normalized_length > 1.0 &&
                !lookup_edge_midpoint( &edge_lookup, p1, p2, &midpoint ) )
            {
                subdivide_edge( &edge_lookup, normalized_length,
                                p1, p2, new_polygons, length_points );
            }
        }
    }

    n_indices = 0;
    for_less( poly, 0, polygons->n_items )
    {
        add_polygons( polygons->indices[
                               POINT_INDEX(polygons->end_indices,poly,0)],
                      polygons->indices[
                               POINT_INDEX(polygons->end_indices,poly,1)],
                      polygons->indices[
                               POINT_INDEX(polygons->end_indices,poly,2)],
                      *length_points, &edge_lookup, new_polygons, &n_indices );
    }

    ALLOC( new_polygons->normals, new_polygons->n_points );

    compute_polygon_normals( new_polygons );

    delete_hash2_table( &edge_lookup );

    return( new_polygons->n_items - polygons->n_items );
}
