#include <internal_volume_io.h>
#include <bicpl.h>

typedef struct tri_node_struct
{
    int                      nodes[3];
    struct tri_node_struct   *children[4];
} tri_node_struct;

typedef struct
{
    int                n_points;
    Point              *points;
    int                n_triangles;
    tri_node_struct    *triangles;
} tri_mesh_struct;

private   int   get_tri_mesh_n_triangles(
    tri_mesh_struct   *mesh );

private  void   convert_polygons_to_mesh(
    polygons_struct  *polygons,
    tri_mesh_struct  *mesh );

private   void   convert_mesh_to_polygons(
    tri_mesh_struct   *mesh,
    polygons_struct   *polygons );

private  Status  output_triangular_mesh(
    STRING         filename,
    File_formats   format,
    tri_mesh_struct  *mesh );

private  Status  input_triangular_mesh(
    STRING           filename,
    File_formats     format,
    tri_mesh_struct  *mesh );

private   void   resample_mesh(
    tri_mesh_struct   *mesh,
    Real              min_size,
    Real              max_size );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj|input.msh  output.obj  output.msh min_size max_size\n\
\n\
     Subdivides the triangular mesh.\n\h";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING           input_filename, output_filename;
    STRING           output_mesh_filename;
    int              n_objects, prev_n_polys, new_n_polys;
    File_formats     format;
    object_struct    **object_list, *object;
    polygons_struct  *polygons;
    Real             min_size, max_size;
    BOOLEAN          input_mesh_specified;
    tri_mesh_struct  mesh;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &output_mesh_filename ) ||
        !get_real_argument( 0.0, &min_size ) ||
        !get_real_argument( 0.0, &max_size ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    input_mesh_specified = filename_extension_matches( input_filename, "msh" );

    if( input_mesh_specified )
    {
        if( input_triangular_mesh( input_filename, BINARY_FORMAT,
                                   &mesh ) != OK )
            return( 1 );
    }
    else
    {
        if( input_graphics_file( input_filename, &format, &n_objects,
                                 &object_list ) != OK ||
            n_objects < 1 || get_object_type(object_list[0]) != POLYGONS )
        {
            print_error( "Error in input file.\n" );
            return( 1 );
        }

        polygons = get_polygons_ptr( object_list[0] );

        convert_polygons_to_mesh( polygons, &mesh );
        delete_polygons( polygons );
    }

    prev_n_polys = get_tri_mesh_n_triangles( &mesh );
    
    resample_mesh( &mesh, min_size, max_size );

    object = create_object( POLYGONS );
    polygons = get_polygons_ptr( object );
    convert_mesh_to_polygons( &mesh, polygons );

    new_n_polys = polygons->n_items;

    print( "Resampled %d polygons into %d polygons.\n", prev_n_polys,
           new_n_polys );

    (void) output_graphics_file( output_filename, format, 1, &object );

    (void) output_triangular_mesh( output_mesh_filename, BINARY_FORMAT, &mesh );

    delete_object_list( n_objects, object_list );

    return( 0 );
}

private  void  initialize_tri_mesh(
    tri_mesh_struct  *mesh )
{
    mesh->n_triangles = 0;
    mesh->n_points = 0;
}

private  void  tri_mesh_insert_triangle(
    tri_mesh_struct  *mesh,
    int              p0,
    int              p1,
    int              p2 )
{
    tri_node_struct  node;

    node.nodes[0] = p0;
    node.nodes[1] = p1;
    node.nodes[2] = p2;
    node.children[0] = NULL;
    node.children[1] = NULL;
    node.children[2] = NULL;
    node.children[3] = NULL;

    ADD_ELEMENT_TO_ARRAY( mesh->triangles, mesh->n_triangles, node,
                          DEFAULT_CHUNK_SIZE );
}

private  int  tri_mesh_insert_point(
    tri_mesh_struct  *mesh,
    Point            *point )
{
    int  ind;

    ind = mesh->n_points;

    ADD_ELEMENT_TO_ARRAY( mesh->points, mesh->n_points, *point,
                          DEFAULT_CHUNK_SIZE )

    return( ind );
}

private  Status  output_tri_node(
    FILE             *file,
    File_formats     format,
    tri_node_struct  *node )
{
    int      child, *list;
    BOOLEAN  leaf_flag;

    leaf_flag = (node->children[0] == NULL);

    list = node->nodes;
    if( io_ints( file, WRITE_FILE, format, 3, &list ) ||
        io_boolean( file, WRITE_FILE, format, &leaf_flag ) )
        return( ERROR );

    if( !leaf_flag )
    {
        for_less( child, 0, 4 )
        {
            if( output_tri_node( file, format, node->children[child] ) != OK )
                return( ERROR );
        }
    }

    return( OK );
}

private  Status  output_triangular_mesh(
    STRING         filename,
    File_formats   format,
    tri_mesh_struct  *mesh )
{
    FILE   *file;
    int    tri;

    if( open_file( filename, WRITE_FILE, format, &file ) != OK )
        return( ERROR );

    if( io_int( file, WRITE_FILE, format, &mesh->n_points ) != OK )
        return( ERROR );

    if( io_points( file, WRITE_FILE, format, mesh->n_points, &mesh->points )
                                              != OK )
        return( ERROR );

    if( io_int( file, WRITE_FILE, format, &mesh->n_triangles ) != OK )
        return( ERROR );

    for_less( tri, 0, mesh->n_triangles )
    {
        if( output_tri_node( file, format, &mesh->triangles[tri] ) != OK )
            return( ERROR );
    }

    (void) close_file( file );

    return( OK );
}

private  Status  input_tri_node(
    FILE             *file,
    File_formats     format,
    tri_mesh_struct  *mesh,
    tri_node_struct  *node )
{
    int      child, *list;
    BOOLEAN  leaf_flag;

    if( io_ints( file, READ_FILE, format, 3, &list ) ||
        io_boolean( file, READ_FILE, format, &leaf_flag ) )
        return( ERROR );

    node->nodes[0] = list[0];
    node->nodes[1] = list[1];
    node->nodes[2] = list[2];

    FREE( list );

    if( leaf_flag )
    {
        node->children[0] = NULL;
        node->children[1] = NULL;
        node->children[2] = NULL;
        node->children[3] = NULL;
    }
    else
    {
        for_less( child, 0, 4 )
        {
            ALLOC( node->children[child], 1 );

            if( input_tri_node( file, format, mesh, node->children[child] )
                != OK )
                return( ERROR );
        }
    }

    return( OK );
}

private  Status  input_triangular_mesh(
    STRING           filename,
    File_formats     format,
    tri_mesh_struct  *mesh )
{
    FILE   *file;
    int    tri;

    initialize_tri_mesh( mesh );

    if( open_file( filename, READ_FILE, format, &file ) != OK )
        return( ERROR );

    if( io_int( file, READ_FILE, format, &mesh->n_points ) != OK )
        return( ERROR );

    SET_ARRAY_SIZE( mesh->points, 0, mesh->n_points, DEFAULT_CHUNK_SIZE );

    if( io_points( file, READ_FILE, format, mesh->n_points, &mesh->points )
                                              != OK )
        return( ERROR );

    if( io_int( file, READ_FILE, format, &mesh->n_triangles ) != OK )
        return( ERROR );

    SET_ARRAY_SIZE( mesh->triangles, 0, mesh->n_triangles, DEFAULT_CHUNK_SIZE );

    for_less( tri, 0, mesh->n_triangles )
    {
        if( input_tri_node( file, format, mesh, &mesh->triangles[tri] ) != OK )
            return( ERROR );
    }

    (void) close_file( file );

    return( OK );
}

private  void   mark_points_used(
    tri_node_struct  *node,
    int              used[] )
{
    ++used[node->nodes[0]];
    ++used[node->nodes[1]];
    ++used[node->nodes[2]];

    if( node->children[0] != NULL )
    {
        mark_points_used( node->children[0], used );
        mark_points_used( node->children[1], used );
        mark_points_used( node->children[2], used );
        mark_points_used( node->children[3], used );
    }
}

private  void   renumber_points(
    tri_node_struct  *node,
    int              new_id[] )
{
    node->nodes[0] = new_id[node->nodes[0]];
    node->nodes[1] = new_id[node->nodes[1]];
    node->nodes[2] = new_id[node->nodes[2]];

    if( node->children[0] != NULL )
    {
        renumber_points( node->children[0], new_id );
        renumber_points( node->children[1], new_id );
        renumber_points( node->children[2], new_id );
        renumber_points( node->children[3], new_id );
    }
}

private  void   delete_unused_nodes(
    tri_mesh_struct  *mesh )
{
    int   *new_id, point, tri, new_n_points;

    ALLOC( new_id, mesh->n_points );

    for_less( point, 0, mesh->n_points )
        new_id[point] = 0;

    for_less( tri, 0, mesh->n_triangles )
        mark_points_used( &mesh->triangles[tri], new_id );

    new_n_points = 0;

    for_less( point, 0, mesh->n_points )
    {
        if( new_id[point] > 0 )
        {
            new_id[point] = new_n_points;
            ++new_n_points;
        }
    }

    if( new_n_points == mesh->n_points )
    {
        FREE( new_id );
        return;
    }

    for_less( point, 0, mesh->n_points )
        mesh->points[new_id[point]] = mesh->points[point];

    SET_ARRAY_SIZE( mesh->points, mesh->n_points, new_n_points,
                    DEFAULT_CHUNK_SIZE );
    mesh->n_points = new_n_points;

    for_less( tri, 0, mesh->n_triangles )
        renumber_points( &mesh->triangles[tri], new_id );
}

private  void   convert_polygons_to_mesh(
    polygons_struct  *polygons,
    tri_mesh_struct  *mesh )
{
    int    poly, size, point;

    initialize_tri_mesh( mesh );

    for_less( point, 0, polygons->n_points )
        (void) tri_mesh_insert_point( mesh, &polygons->points[point] );

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        if( size != 3 )
        {
            print_error( "Ignoring non-triangular polygon.\n" );
            continue;
        }

        tri_mesh_insert_triangle( mesh,
                 polygons->indices[POINT_INDEX(polygons->end_indices,poly,0)],
                 polygons->indices[POINT_INDEX(polygons->end_indices,poly,1)],
                 polygons->indices[POINT_INDEX(polygons->end_indices,poly,2)] );
    }
}

private  int  get_key(
    int  p0,
    int  p1 )
{
    int   k0, k1;

    if( p0 >= (1<<16) || p1 >= (1<<16) )
        print( "Warning: get_key, too many points.\n" );

    k0 = MIN( p0, p1 );
    k1 = MAX( p0, p1 );

    return( (k0 << 16) + k1 );
}

private  void  insert_edge_midpoint(
    hash_table_struct       *edge_lookup,
    int                     p0,
    int                     p1,
    int                     midpoint )
{
    int     key;

    key = get_key( p0, p1 );

    if( lookup_in_hash_table( edge_lookup, key, (void *) &midpoint ) )
        return;

    insert_in_hash_table( edge_lookup, key, (void *) &midpoint );
}

private  BOOLEAN  lookup_edge_midpoint(
    hash_table_struct    *edge_lookup,
    int                  p0,
    int                  p1,
    int                  *midpoint )
{
    int     key;

    key = get_key( p0, p1 );

    return( lookup_in_hash_table( edge_lookup, key, (void *) midpoint ) );
}

private  int  get_edge_midpoint(
    tri_mesh_struct      *mesh,
    hash_table_struct    *edge_lookup,
    int                  p0,
    int                  p1 )
{
    int     key, midpoint;
    Point   mid;

    midpoint = 0;  /* to avoid compiler message*/

    if( lookup_edge_midpoint( edge_lookup, p0, p1, (void *) &midpoint ) )
        return( midpoint );

    INTERPOLATE_POINTS( mid, mesh->points[p0], mesh->points[p1], 0.5 );

    midpoint = tri_mesh_insert_point( mesh, &mid );

    key = get_key( p0, p1 );
    insert_in_hash_table( edge_lookup, key, (void *) &midpoint );

    return( midpoint );
}

private  void   insert_edge_points(
    hash_table_struct       *edge_lookup,
    tri_node_struct         *node )
{
    if( node->children[0] == NULL )
        return;

    insert_edge_midpoint( edge_lookup, node->nodes[0], node->nodes[1],
                          node->children[2]->nodes[0] );
    insert_edge_midpoint( edge_lookup, node->nodes[1], node->nodes[2],
                          node->children[2]->nodes[1] );
    insert_edge_midpoint( edge_lookup, node->nodes[2], node->nodes[0],
                          node->children[2]->nodes[2] );

    insert_edge_points( edge_lookup, node->children[0] );
    insert_edge_points( edge_lookup, node->children[1] );
    insert_edge_points( edge_lookup, node->children[2] );
    insert_edge_points( edge_lookup, node->children[3] );
}

private  void  create_edge_lookup(
    tri_mesh_struct    *mesh,
    hash_table_struct  *lookup )
{
    int   tri;

    initialize_hash_table( lookup, 10 * mesh->n_points,
                           sizeof(int), 0.5, 0.25 );

    for_less( tri, 0, mesh->n_triangles )
        insert_edge_points( lookup, &mesh->triangles[tri] );
}

private  void  delete_edge_lookup(
    hash_table_struct  *lookup )
{
    delete_hash_table( lookup );
}

private  void  subdivide_tri_node(
    tri_mesh_struct   *mesh,
    hash_table_struct *edge_lookup,
    tri_node_struct   *node )
{
    int  midpoints[3];

    midpoints[0] = get_edge_midpoint( mesh, edge_lookup, node->nodes[0],
                                                         node->nodes[1] );
    midpoints[1] = get_edge_midpoint( mesh, edge_lookup, node->nodes[1],
                                                         node->nodes[2] );
    midpoints[2] = get_edge_midpoint( mesh, edge_lookup, node->nodes[2],
                                                         node->nodes[0] );

    tri_mesh_insert_triangle( mesh, node->nodes[0], midpoints[0], midpoints[2]);
    tri_mesh_insert_triangle( mesh, midpoints[0], node->nodes[1], midpoints[1]);
    tri_mesh_insert_triangle( mesh, midpoints[0], midpoints[1], midpoints[2]);
    tri_mesh_insert_triangle( mesh, midpoints[2], midpoints[1], node->nodes[2]);
}

private  int  count_triangles(
    tri_node_struct  *node )
{
    int   n_triangles;

    n_triangles = 1;

    if( node->children[0] != NULL )
    {
        n_triangles += count_triangles( node->children[0] );
        n_triangles += count_triangles( node->children[1] );
        n_triangles += count_triangles( node->children[2] );
        n_triangles += count_triangles( node->children[3] );
    }

    return( n_triangles );
}

private   int   get_tri_mesh_n_triangles(
    tri_mesh_struct   *mesh )
{
    int    tri, n_triangles;

    n_triangles = 0;

    for_less( tri, 0, mesh->n_triangles )
        n_triangles += count_triangles( &mesh->triangles[tri] );

    return( n_triangles );
}

private  void  add_subdivided_edge(
    hash_table_struct      *edge_lookup,
    int                    *indices[],
    int                    *n_indices,
    int                    p0,
    int                    p1 )
{
    int   midpoint;

    if( lookup_edge_midpoint( edge_lookup, p0, p1, &midpoint ) )
    {
        add_subdivided_edge( edge_lookup, indices, n_indices, p0, midpoint );
        add_subdivided_edge( edge_lookup, indices, n_indices, midpoint, p1 );
    }
    else
    {
        ADD_ELEMENT_TO_ARRAY( *indices, *n_indices, p0, DEFAULT_CHUNK_SIZE );
    }
}

private  void   add_to_polygons(
    hash_table_struct       *edge_lookup,
    int                     end_indices[],
    int                     *current_poly,
    int                     *indices[],
    int                     *n_indices,
    tri_node_struct         *node )
{
    if( node->children[0] == NULL )
    {
        add_subdivided_edge( edge_lookup, indices, n_indices,
                             node->nodes[0], node->nodes[1] );
        add_subdivided_edge( edge_lookup, indices, n_indices,
                             node->nodes[1], node->nodes[2] );
        add_subdivided_edge( edge_lookup, indices, n_indices,
                             node->nodes[2], node->nodes[0] );
        end_indices[*current_poly] = *n_indices;
        ++(*current_poly);
    }
    else
    {
        add_to_polygons( edge_lookup, end_indices, current_poly,
                         indices, n_indices, node->children[0] );
        add_to_polygons( edge_lookup, end_indices, current_poly,
                         indices, n_indices, node->children[1] );
        add_to_polygons( edge_lookup, end_indices, current_poly,
                         indices, n_indices, node->children[2] );
        add_to_polygons( edge_lookup, end_indices, current_poly,
                         indices, n_indices, node->children[3] );
    }
}

private   void   convert_mesh_to_polygons(
    tri_mesh_struct   *mesh,
    polygons_struct   *polygons )
{
    int                point, poly, tri, n_indices;
    hash_table_struct  edge_lookup;

    initialize_polygons( polygons, WHITE, NULL );

    ALLOC( polygons->points, mesh->n_points );
    ALLOC( polygons->normals, mesh->n_points );
    polygons->n_points = mesh->n_points;

    for_less( point, 0, mesh->n_points )
        polygons->points[point] = mesh->points[point];

    polygons->n_items = get_tri_mesh_n_triangles( mesh );

    ALLOC( polygons->end_indices, polygons->n_items );

    polygons->indices = NULL;
    n_indices = 0;

    poly = 0;

    create_edge_lookup( mesh, &edge_lookup );

    for_less( tri, 0, mesh->n_triangles )
    {
        add_to_polygons( &edge_lookup, polygons->end_indices, &poly,
                         &polygons->indices, &n_indices,
                         &mesh->triangles[tri] );
    }

    delete_edge_lookup( &edge_lookup );

    compute_polygon_normals( polygons );
}

private   void   resample_mesh(
    tri_mesh_struct   *mesh,
    Real              min_size,
    Real              max_size )
{
    delete_unused_nodes( mesh );
}
