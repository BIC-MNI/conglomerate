#include <internal_volume_io.h>
#include <bicpl.h>
#include <special_geometry.h>

/*------------------------ basic stuff -----------*/

public  void  tri_mesh_initialize(
    tri_mesh_struct  *mesh )
{
    mesh->n_triangles = 0;
    mesh->triangles = NULL;
    mesh->n_points = 0;
    mesh->edge_lookup_initialized = FALSE;
}

public  int  tri_mesh_get_n_points(
    tri_mesh_struct  *mesh )
{
    return( mesh->n_points );
}

private  tri_node_struct  *create_tri_leaf(
    int              p0,
    int              p1,
    int              p2 )
{
    tri_node_struct  *node;

    ALLOC( node, 1 );

    node->nodes[0] = p0;
    node->nodes[1] = p1;
    node->nodes[2] = p2;
    node->children[0] = NULL;
    node->children[1] = NULL;
    node->children[2] = NULL;
    node->children[3] = NULL;

    return( node );
}

private  void  delete_tri_node(
    tri_node_struct  *node )
{
    if( node->children[0] != NULL )
    {
        delete_tri_node( node->children[0] );
        delete_tri_node( node->children[1] );
        delete_tri_node( node->children[2] );
        delete_tri_node( node->children[3] );
    }

    FREE( node );
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
    Point            *point,
    BOOLEAN          active_flag )
{
    ADD_ELEMENT_TO_ARRAY( mesh->points, mesh->n_points, *point,
                          DEFAULT_CHUNK_SIZE )
    --mesh->n_points;
    ADD_ELEMENT_TO_ARRAY( mesh->active_flags, mesh->n_points,
                          (Smallest_int) active_flag, DEFAULT_CHUNK_SIZE )

    return( mesh->n_points-1 );
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
    if( io_ints( file, WRITE_FILE, format, 3, &list ) != OK ||
        io_boolean( file, WRITE_FILE, format, &leaf_flag ) != OK )
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

public  Status  tri_mesh_output(
    STRING         filename,
    File_formats   format,
    tri_mesh_struct  *mesh )
{
    FILE     *file;
    int      tri, point;
    BOOLEAN  flag;

    if( open_file( filename, WRITE_FILE, format, &file ) != OK )
        return( ERROR );

    if( io_int( file, WRITE_FILE, format, &mesh->n_points ) != OK )
        return( ERROR );

    if( io_int( file, WRITE_FILE, format, &mesh->n_triangles ) != OK )
        return( ERROR );

    for_less( point, 0, mesh->n_points )
    {
        if( io_point( file, WRITE_FILE, format, &mesh->points[point] ) != OK )
            return( ERROR );
    }

    for_less( point, 0, mesh->n_points )
    {
        flag = (BOOLEAN) mesh->active_flags[point];
        if( io_boolean( file, WRITE_FILE, format, &flag ) != OK )
            return( ERROR );
    }

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

    if( io_ints( file, READ_FILE, format, 3, &list ) != OK ||
        io_boolean( file, READ_FILE, format, &leaf_flag ) != OK )
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

public  BOOLEAN  tri_mesh_set_points(
    tri_mesh_struct  *mesh,
    int              n_points,
    Point            points[] )
{
    int   point;

    if( n_points != mesh->n_points )
    {
        print_error( "Mismatched number of points.\n" );
        return( FALSE );
    }

    for_less( point, 0, n_points )
        mesh->points[point] = points[point];

    return( TRUE );
}

private  Status  input_tri_mesh_format(
    STRING           filename,
    File_formats     format,
    tri_mesh_struct  *mesh )
{
    FILE     *file;
    int      tri, point;
    BOOLEAN  flag;

    tri_mesh_initialize( mesh );

    if( open_file( filename, READ_FILE, format, &file ) != OK )
        return( ERROR );

    if( io_int( file, READ_FILE, format, &mesh->n_points ) != OK )
        return( ERROR );

    if( io_int( file, READ_FILE, format, &mesh->n_triangles ) != OK )
        return( ERROR );

    SET_ARRAY_SIZE( mesh->points, 0, mesh->n_points, DEFAULT_CHUNK_SIZE );

    for_less( point, 0, mesh->n_points )
    {
        if( io_point( file, READ_FILE, format, &mesh->points[point] ) != OK )
            return( ERROR );
    }

    SET_ARRAY_SIZE( mesh->active_flags, 0, mesh->n_points, DEFAULT_CHUNK_SIZE );

    for_less( point, 0, mesh->n_points )
    {
        if( io_boolean( file, READ_FILE, format, &flag ) != OK )
            return( ERROR );

        mesh->active_flags[point] = (Smallest_int) flag;
    }

    SET_ARRAY_SIZE( mesh->triangles, 0, mesh->n_triangles, DEFAULT_CHUNK_SIZE );

    for_less( tri, 0, mesh->n_triangles )
    {
        if( input_tri_node( file, format, mesh, &mesh->triangles[tri] ) != OK )
            return( ERROR );
    }

    (void) close_file( file );

    return( OK );
}

public  Status  tri_mesh_input(
    STRING           filename,
    File_formats     format,
    tri_mesh_struct  *mesh )
{
    Status         status;
    int            n_objects;
    object_struct  **objects;

    if( filename_extension_matches( filename, "obj" ) )
    {
        if( input_graphics_file( filename, &format, &n_objects, &objects ) != OK
            || n_objects != 1 ||
            get_object_type( objects[0] ) != POLYGONS )
        {
            print_error( "Error reading polygons as tri mesh.\n" );
            return( ERROR );
        }

        tri_mesh_convert_from_polygons( get_polygons_ptr(objects[0]),
                                        mesh );

        delete_object_list( n_objects, objects );
        status = OK; 
    }
    else
        status = input_tri_mesh_format( filename, format, mesh );

    return( status );
}

/*--------------- edge point lookup ------------------------------- */

private  void  insert_edge_midpoint(
    hash2_table_struct      *edge_lookup,
    int                     p0,
    int                     p1,
    int                     midpoint )
{
    int     k0, k1;

    k0 = MIN( p0, p1 );
    k1 = MAX( p0, p1 );

    if( lookup_in_hash2_table( edge_lookup, k0, k1, (void *) &midpoint ) )
        return;

    insert_in_hash2_table( edge_lookup, k0, k1, (void *) &midpoint );
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

private  void   insert_edge_points(
    hash2_table_struct       *edge_lookup,
    tri_node_struct          *node )
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

public  void  tri_mesh_delete_edge_lookup(
    tri_mesh_struct     *mesh )
{
    if( mesh->edge_lookup_initialized )
        delete_hash2_table( &mesh->edge_lookup );

    mesh->edge_lookup_initialized = FALSE;
}

public  void  tri_mesh_create_edge_lookup(
    tri_mesh_struct     *mesh )
{
    int   tri;

    tri_mesh_delete_edge_lookup( mesh );

    initialize_hash2_table( &mesh->edge_lookup, 10 * mesh->n_points,
                            sizeof(int), 0.25, 0.125 );

    for_less( tri, 0, mesh->n_triangles )
        insert_edge_points( &mesh->edge_lookup, &mesh->triangles[tri] );

    mesh->edge_lookup_initialized = TRUE;
}

private  void  recursive_delete_tri_mesh(
    tri_node_struct  *node )
{
    if( node == NULL )
        return;

    recursive_delete_tri_mesh( node->children[0] );
    recursive_delete_tri_mesh( node->children[1] );
    recursive_delete_tri_mesh( node->children[2] );
    recursive_delete_tri_mesh( node->children[3] );

    FREE( node );
}

public  void  tri_mesh_delete(
    tri_mesh_struct  *mesh )
{
    int   tri;

    for_less( tri, 0, mesh->n_triangles )
        recursive_delete_tri_mesh( &mesh->triangles[tri] );

    tri_mesh_delete_edge_lookup( mesh );

    if( mesh->n_points > 0 )
    {
        FREE( mesh->points );
        FREE( mesh->active_flags );
    }

    if( mesh->n_triangles > 0 )
    {
        FREE( mesh->triangles );
    }
}

private  void  check_edge_lookup_created(
    tri_mesh_struct     *mesh )
{
    if( !mesh->edge_lookup_initialized )
        tri_mesh_create_edge_lookup( mesh );
}

private  int  get_edge_midpoint(
    tri_mesh_struct      *mesh,
    int                  p0,
    int                  p1,
    BOOLEAN              active_flag )
{
    int     midpoint;
    Point   mid;
    int     k0, k1;

    check_edge_lookup_created( mesh );

    midpoint = 0;  /* to avoid compiler message*/

    if( lookup_edge_midpoint( &mesh->edge_lookup, p0, p1, (void *) &midpoint ) )
    {
        mesh->active_flags[midpoint] = (Smallest_int) active_flag;
        return( midpoint );
    }

    INTERPOLATE_POINTS( mid, mesh->points[p0], mesh->points[p1], 0.5 );

    midpoint = tri_mesh_insert_point( mesh, &mid, active_flag );

    k0 = MIN( p0, p1 );
    k1 = MAX( p0, p1 );

    insert_in_hash2_table( &mesh->edge_lookup, k0, k1, (void *) &midpoint );

    return( midpoint );
}

/*------------------------------- subdivide ----------------------------- */

private  void  subdivide_tri_node(
    tri_mesh_struct    *mesh,
    tri_node_struct    *node,
    BOOLEAN            active_flag )
{
    int  midpoints[3];

    midpoints[0] = get_edge_midpoint( mesh, node->nodes[0], node->nodes[1],
                                      active_flag );
    midpoints[1] = get_edge_midpoint( mesh, node->nodes[1], node->nodes[2],
                                      active_flag );
    midpoints[2] = get_edge_midpoint( mesh, node->nodes[2], node->nodes[0],
                                      active_flag );

    node->children[0] = create_tri_leaf( node->nodes[0], midpoints[0],
                                         midpoints[2]);
    node->children[1] = create_tri_leaf( midpoints[0], node->nodes[1],
                                         midpoints[1]);
    node->children[2] = create_tri_leaf( midpoints[0], midpoints[1],
                                         midpoints[2]);
    node->children[3] = create_tri_leaf( midpoints[2], midpoints[1],
                                         node->nodes[2]);
}

/*----------------- removing unused nodes ------------------------ */

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

public  void   tri_mesh_delete_unused_nodes(
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
        else
            new_id[point] = -1;
    }

    if( new_n_points == mesh->n_points )
    {
        FREE( new_id );
        return;
    }

    for_less( point, 0, mesh->n_points )
    {
        if( new_id[point] >= 0 )
        {
            mesh->points[new_id[point]] = mesh->points[point];
            mesh->active_flags[new_id[point]] = mesh->active_flags[point];
        }
    }

    SET_ARRAY_SIZE( mesh->points, mesh->n_points, new_n_points,
                    DEFAULT_CHUNK_SIZE );
    SET_ARRAY_SIZE( mesh->active_flags, mesh->n_points, new_n_points,
                    DEFAULT_CHUNK_SIZE );
    mesh->n_points = new_n_points;

    for_less( tri, 0, mesh->n_triangles )
        renumber_points( &mesh->triangles[tri], new_id );

    if( mesh->edge_lookup_initialized )
        tri_mesh_delete_edge_lookup( mesh );

}

/*------------------------ reorder triangles ------------------------- */

private  void  reorder_triangles(
    tri_node_struct  *node,
    int              index )
{
    int              p0, p1, p2;
    tri_node_struct  *c0, *c1, *c2, *c3;

    if( node == NULL )
        return;

    p0 = node->nodes[0];
    p1 = node->nodes[1];
    p2 = node->nodes[2];

    c0 = node->children[0];
    c1 = node->children[1];
    c2 = node->children[2];
    c3 = node->children[3];

    switch( index )
    {
    case 1:
        node->nodes[0] = p1;
        node->nodes[1] = p2;
        node->nodes[2] = p0;

        if( c0 != NULL )
        {
            node->children[0] = c1;
            node->children[1] = c3;
            node->children[3] = c0;
            reorder_triangles( c0, 1 );
            reorder_triangles( c1, 1 );
            reorder_triangles( c2, 1 );
            reorder_triangles( c3, 1 );
        }
        break;

    case 2:
        node->nodes[0] = p2;
        node->nodes[1] = p0;
        node->nodes[2] = p1;

        if( c0 != NULL )
        {
            node->children[0] = c3;
            node->children[1] = c0;
            node->children[3] = c1;
            reorder_triangles( c0, 2 );
            reorder_triangles( c1, 2 );
            reorder_triangles( c2, 2 );
            reorder_triangles( c3, 2 );
        }
        break;

    default:
        break;
    }
}

public  void   tri_mesh_convert_from_polygons(
    polygons_struct  *polygons,
    tri_mesh_struct  *mesh )
{
    int              poly, size, tri, n_nodes, subtri, node, p, p_index, point;
    int              nodes[6], counts[6];
    int              which_tri[4], indices[4], mid0, n_done;
    tri_node_struct  new_tri;

    tri_mesh_initialize( mesh );

    SET_ARRAY_SIZE( mesh->points, 0, polygons->n_points, DEFAULT_CHUNK_SIZE );
    SET_ARRAY_SIZE( mesh->active_flags, 0, polygons->n_points,
                    DEFAULT_CHUNK_SIZE );

    mesh->n_points = polygons->n_points;

    for_less( point, 0, polygons->n_points )
    {
        mesh->points[point] = polygons->points[point];
        mesh->active_flags[point] = TRUE;
    }

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

    if( is_this_tetrahedral_topology(polygons) )
    {
        while( mesh->n_triangles > 8 && mesh->n_triangles != 20 )
        {
            for( tri = 0;  tri < mesh->n_triangles;  tri += 4 )
            {
                n_nodes = 0;
                for_less( subtri, 0, 4 )
                {
                    for_less( node, 0, 3 )
                    {
                        p = mesh->triangles[tri+subtri].nodes[node];
                        for_less( p_index, 0, n_nodes )
                            if( nodes[p_index] == p ) break;
                        if( p_index >= n_nodes )
                        {
                            nodes[p_index] = p;
                            counts[p_index] = 0;
                            ++n_nodes;
                        }
                        ++counts[p_index];
                    }
                }

                if( n_nodes != 6 )
                    handle_internal_error( "tetra" );

                n_done = 0;
                for_less( subtri, 0, 4 )
                {
                    for_less( node, 0, 3 )
                    {
                        p = mesh->triangles[tri+subtri].nodes[node];
                        for_less( p_index, 0, n_nodes )
                            if( nodes[p_index] == p ) break;

                        if( counts[p_index] == 1 )
                        {
                            which_tri[n_done] = subtri;
                            indices[n_done] = node;
                            ++n_done;
                            if( n_done == 2 )
                                ++n_done;
                            break;
                        }
                    }

                    if( node == 3 )
                        which_tri[2] = subtri;
                }

                mid0 =mesh->triangles[tri+which_tri[0]].nodes[(indices[0]+1)%3];
                for_less( node, 0, 3 )
                    if( mesh->triangles[tri+which_tri[2]].nodes[node] == mid0 )
                        break;
                indices[2] = node;

                reorder_triangles( &mesh->triangles[tri+which_tri[0]],
                                   indices[0] );
                reorder_triangles( &mesh->triangles[tri+which_tri[1]],
                                   (indices[1]+2) % 3 );
                reorder_triangles( &mesh->triangles[tri+which_tri[2]],
                                   indices[2] );
                reorder_triangles( &mesh->triangles[tri+which_tri[3]],
                                   (indices[3]+1) % 3 );
                for_less( subtri, 0, 4 )
                {
                    ALLOC( new_tri.children[subtri], 1 );
                    *(new_tri.children[subtri]) = mesh->triangles
                                                [tri+which_tri[subtri]];
                }

                new_tri.nodes[0] = 
                    mesh->triangles[tri+which_tri[0]].nodes[indices[0]];
                new_tri.nodes[1] = 
                    mesh->triangles[tri+which_tri[1]].nodes[indices[1]];
                new_tri.nodes[2] = 
                    mesh->triangles[tri+which_tri[3]].nodes[indices[3]];

                mesh->triangles[tri/4] = new_tri;
            }
            mesh->n_triangles /= 4;
        }
        SET_ARRAY_SIZE( mesh->triangles, polygons->n_items, mesh->n_triangles,
                        DEFAULT_CHUNK_SIZE );
    }
}

/* ------------------------------------------------------ */

private  void   add_to_polygons(
    tri_mesh_struct          *mesh,
    int                      *end_indices[],
    int                      *poly,
    int                      *indices[],
    int                      *n_indices,
    int                      p0,
    int                      p1,
    int                      p2 )
{
    int      mid0, mid1, mid2;
    BOOLEAN  mid0_exists, mid1_exists, mid2_exists;

    mid0_exists = lookup_edge_midpoint( &mesh->edge_lookup, p0, p1, &mid0 );
    mid1_exists = lookup_edge_midpoint( &mesh->edge_lookup, p1, p2, &mid1 );
    mid2_exists = lookup_edge_midpoint( &mesh->edge_lookup, p2, p0, &mid2 );

    if( !mid0_exists && !mid1_exists && !mid2_exists )
    {
        ADD_ELEMENT_TO_ARRAY( *indices, *n_indices, p0, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *indices, *n_indices, p1, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *indices, *n_indices, p2, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *end_indices, *poly, *n_indices,
                              DEFAULT_CHUNK_SIZE );
    }
    else if( mid0_exists && !mid1_exists && !mid2_exists )
    {
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         p0, mid0, p2 );
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         mid0, p1, p2 );
    }
    else if( !mid0_exists && mid1_exists && !mid2_exists )
    {
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         p0, p1, mid1 );
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         mid1, p2, p0 );
    }
    else if( !mid0_exists && !mid1_exists && mid2_exists )
    {
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         p0, p1, mid2 );
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         p1, p2, mid2 );
    }
    else if( mid0_exists && mid1_exists && !mid2_exists )
    {
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         p0, mid0, p2 );
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         mid0, mid1, p2 );
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         mid0, p1, mid1 );
    }
    else if( !mid0_exists && mid1_exists && mid2_exists )
    {
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         p0, p1, mid1 );
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         p0, mid1, mid2 );
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         mid1, p2, mid2 );
    }
    else if( mid0_exists && !mid1_exists && mid2_exists )
    {
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         p0, mid0, mid2 );
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         mid0, p1, p2 );
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         mid0, p2, mid2 );
    }
    else
    {
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         p0, mid0, mid2 );
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         mid0, p1, mid1 );
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         mid0, mid1, mid2 );
        add_to_polygons( mesh, end_indices, poly, indices, n_indices,
                         mid1, p2, mid2 );
    }
}

public   void   tri_mesh_convert_to_polygons(
    tri_mesh_struct   *mesh,
    polygons_struct   *polygons )
{
    int                 point, tri, n_indices;

    initialize_polygons( polygons, WHITE, NULL );

    ALLOC( polygons->points, mesh->n_points );
    ALLOC( polygons->normals, mesh->n_points );
    polygons->n_points = mesh->n_points;

    for_less( point, 0, mesh->n_points )
        polygons->points[point] = mesh->points[point];

    polygons->end_indices = NULL;
    polygons->n_items = 0;

    polygons->indices = NULL;
    n_indices = 0;

    check_edge_lookup_created( mesh );

    for_less( tri, 0, mesh->n_triangles )
    {
        add_to_polygons( mesh, &polygons->end_indices,
                         &polygons->n_items,
                         &polygons->indices, &n_indices,
                         mesh->triangles[tri].nodes[0],
                         mesh->triangles[tri].nodes[1],
                         mesh->triangles[tri].nodes[2] );
    }

    compute_polygon_normals( polygons );
}

/*----------------------------------------------------------- */

private  Real  get_triangle_size(
    Point   *p1,
    Point   *p2,
    Point   *p3 )
{
    Real  dist1, dist2, dist3;

    dist1 = sq_distance_between_points( p1, p2 );
    dist2 = sq_distance_between_points( p2, p3 );
    dist3 = sq_distance_between_points( p3, p1 );

    return( MAX3( dist1, dist2, dist3 ) );
}

private  void   coalesce_on_node_values(
    tri_mesh_struct    *mesh,
    tri_node_struct    *node,
    Real               min_value,
    Real               max_value,
    int                n_values,
    Real               values[],
    Real               min_size_sq,
    Real               max_size_sq )
{
    int       i, list[6], child;
    Real      size;
    BOOLEAN   coalesce;

    if( node->children[0] != NULL )
    {
        coalesce_on_node_values( mesh, node->children[0],
                                 min_value, max_value, n_values, values,
                                 min_size_sq, max_size_sq );
        coalesce_on_node_values( mesh, node->children[1],
                                 min_value, max_value, n_values, values,
                                 min_size_sq, max_size_sq );
        coalesce_on_node_values( mesh, node->children[2],
                                 min_value, max_value, n_values, values,
                                 min_size_sq, max_size_sq );
        coalesce_on_node_values( mesh, node->children[3],
                                 min_value, max_value, n_values, values,
                                 min_size_sq, max_size_sq );
    }

    if( node->children[0] != NULL &&
        node->children[0]->children[0] == NULL &&
        node->children[1]->children[0] == NULL &&
        node->children[2]->children[0] == NULL &&
        node->children[3]->children[0] == NULL )
    {
        coalesce = TRUE;

        if( values != NULL )
        {
            list[0] = node->nodes[0];
            list[1] = node->nodes[1];
            list[2] = node->nodes[2];
            (void) lookup_edge_midpoint( &mesh->edge_lookup,
                                         list[0], list[1], &list[3] );
            (void) lookup_edge_midpoint( &mesh->edge_lookup,
                                         list[1], list[2], &list[4] );
            (void) lookup_edge_midpoint( &mesh->edge_lookup,
                                         list[2], list[0], &list[5] );

            for_less( i, 0, 6 )
            {
                if( list[i] >= n_values || 
                    values[list[i]] < min_value ||
                    values[list[i]] > max_value )
                {
                    coalesce = FALSE;
                }
            }
        }

        if( coalesce )
        {
            for_less( child, 0, 4 )
            {
                size = get_triangle_size(
                       &mesh->points[node->children[child]->nodes[0]],
                       &mesh->points[node->children[child]->nodes[1]],
                       &mesh->points[node->children[child]->nodes[2]] );

                if( min_size_sq > 0.0 && size < min_size_sq ||
                    max_size_sq > 0.0 && size > max_size_sq )
                    coalesce = FALSE;
            }
        }

        if( coalesce )
        {
            delete_tri_node( node->children[0] );
            delete_tri_node( node->children[1] );
            delete_tri_node( node->children[2] );
            delete_tri_node( node->children[3] );
            node->children[0] = NULL;
            node->children[1] = NULL;
            node->children[2] = NULL;
            node->children[3] = NULL;
        }
    }
}

public   void   tri_mesh_coalesce_triangles(
    tri_mesh_struct    *mesh,
    Real               min_value,
    Real               max_value,
    int                n_values,
    Real               values[],
    Real               min_size,
    Real               max_size )
{
    int                tri;

    if( min_size > 0.0 )
        min_size = min_size * min_size;

    if( max_size > 0.0 )
        max_size = max_size * max_size;

    check_edge_lookup_created( mesh );

    for_less( tri, 0, mesh->n_triangles )
    {
        (void) coalesce_on_node_values( mesh, &mesh->triangles[tri],
                                        min_value, max_value,
                                        n_values, values, min_size, max_size );
    }
}

/*------------------------ subdivide ---------------------------------- */

private  void   subdivide_on_node_values(
    tri_mesh_struct    *mesh,
    tri_node_struct    *node,
    Real               min_value,
    Real               max_value,
    int                n_values,
    Real               values[],
    Real               min_size_sq,
    Real               max_size_sq,
    int                max_subdivisions )
{
    Real     size;
    int      child, vertex;
    BOOLEAN  subdivide_flag;

    if( max_subdivisions != 0 && node->children[0] == NULL )
    {
        size = get_triangle_size( &mesh->points[node->nodes[0]],
                                  &mesh->points[node->nodes[1]],
                                  &mesh->points[node->nodes[2]] );

        subdivide_flag = (min_size_sq <= 0.0 || size >= min_size_sq) &&
                         (max_size_sq <= 0.0 || size <= max_size_sq);

        if( values != NULL )
        {
            for_less( vertex, 0, 3 )
            {
                if( node->nodes[vertex] >= n_values ||
                    values[node->nodes[vertex]] < min_value ||
                    values[node->nodes[vertex]] > max_value )
                {
                    subdivide_flag = FALSE;
                }
            }
        }

        if( subdivide_flag )
        {
            subdivide_tri_node( mesh, node, TRUE );
            --max_subdivisions;
        }
    }

    if( node->children[0] != NULL )
    {
        for_less( child, 0, 4 )
        {
            subdivide_on_node_values( mesh,
                                      node->children[child],
                                      min_value, max_value, n_values, values,
                                      min_size_sq, max_size_sq,
                                      max_subdivisions );
        }
    }
}

public   void   tri_mesh_subdivide_triangles(
    tri_mesh_struct    *mesh,
    Real               min_value,
    Real               max_value,
    int                n_values,
    Real               values[],
    Real               min_size,
    Real               max_size,
    int                max_subdivisions )
{
    int                tri;

    if( min_size > 0.0 )
        min_size = min_size * min_size;

    if( max_size > 0.0 )
        max_size = max_size * max_size;

    check_edge_lookup_created( mesh );

    for_less( tri, 0, mesh->n_triangles )
    {
        subdivide_on_node_values( mesh,
                                  &mesh->triangles[tri], min_value, max_value,
                                  n_values, values, min_size, max_size,
                                  max_subdivisions );
    }
}

/*------------------------ subdivide ---------------------------------- */

private  void   subdivide_bordering_triangles(
    tri_mesh_struct    *mesh,
    tri_node_struct    *node )
{
    int      child, edge, p1, p2, midpoint;
    BOOLEAN  subdivide_flag;

    if( node->children[0] == NULL )
    {
        subdivide_flag = FALSE;
        for_less( edge, 0, 3 )
        {
            p1 = node->nodes[edge];
            p2 = node->nodes[(edge+1)%3];
            if( lookup_edge_midpoint( &mesh->edge_lookup, p1, p2, &midpoint ) &&
                mesh->active_flags[midpoint] )
            {
                subdivide_flag = TRUE;
            }

        }

        if( subdivide_flag )
            subdivide_tri_node( mesh, node, FALSE );
    }

    if( node->children[0] != NULL )
    {
        for_less( child, 0, 4 )
            subdivide_bordering_triangles( mesh, node->children[child] );
    }
}

public   void   tri_mesh_subdivide_bordering_triangles(
    tri_mesh_struct    *mesh )
{
    int                tri;

    check_edge_lookup_created( mesh );

    for_less( tri, 0, mesh->n_triangles )
    {
        subdivide_bordering_triangles( mesh,
                                       &mesh->triangles[tri] );
    }
}

/* ---------------------------------------------------------------- */

private  void   count_on_level(
    tri_mesh_struct   *mesh,
    tri_node_struct   *node,
    int               *n_levels,
    int               *n_in_level[],
    int               level )
{
    int   l;

    if( node->children[0] == NULL )
    {
        if( level >= *n_levels )
        {
            SET_ARRAY_SIZE( *n_in_level, *n_levels, level+1, 100 );
            for_less( l, *n_levels, level+1 )
                (*n_in_level)[l] = 0;
            *n_levels = level+1;
        }
        ++(*n_in_level)[level];
    }
    else
    {
        count_on_level( mesh, node->children[0], n_levels, n_in_level, level+1);
        count_on_level( mesh, node->children[1], n_levels, n_in_level, level+1);
        count_on_level( mesh, node->children[2], n_levels, n_in_level, level+1);
        count_on_level( mesh, node->children[3], n_levels, n_in_level, level+1);
    }
}

public  void  tri_mesh_print_levels(
    tri_mesh_struct  *mesh )
{
    int   tri, n_levels, *n_in_level, level;

    n_levels = 0;
    n_in_level = NULL;

    for_less( tri, 0, mesh->n_triangles )
        count_on_level( mesh, &mesh->triangles[tri], &n_levels, &n_in_level,
                        0 );

    for_less( level, 0, n_levels )
        print( " %d", n_in_level[level] );
    print( "\n" );

    if( n_levels > 0 )
        FREE( n_in_level );
}

/* ---------------------------------------------------------------- */

private  Status  output_fixed_midpoints(
    FILE                 *file,
    hash2_table_struct   *edge_lookup,
    Smallest_int         active_flags[],
    Smallest_int         visited_flags[],
    tri_node_struct      *node )
{
    int   edge, p1, p2, midpoint, child;

    if( node->children[0] != NULL )
    {
        for_less( edge, 0, 3 )
        {
            p1 = node->nodes[edge];
            p2 = node->nodes[(edge+1)%3];
            if( lookup_edge_midpoint( edge_lookup, p1, p2, &midpoint ) &&
                !visited_flags[midpoint] && !active_flags[midpoint] )
            {
                visited_flags[midpoint] = TRUE;

                if( output_int( file, p1 ) != OK ||
                    output_int( file, p2 ) != OK ||
                    output_int( file, midpoint ) != OK ||
                    output_newline( file ) != OK )
                    return( ERROR );
            }
        }

        for_less( child, 0, 4 )
        {
            if( output_fixed_midpoints( file, edge_lookup,
                                        active_flags, visited_flags,
                                        node->children[child] ) != OK )
                return( ERROR );
        }
    }

    return( OK );
}

public  Status  output_mesh_fixed_midpoints(
    STRING           filename,
    tri_mesh_struct  *mesh )
{
    int           tri, point;
    Smallest_int  *visited_flags;
    FILE          *file;

    if( open_file( filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( ERROR );

    ALLOC( visited_flags, mesh->n_points );

    for_less( point, 0, mesh->n_points )
        visited_flags[point] = FALSE;

    for_less( tri, 0, mesh->n_triangles )
    {
        if( output_fixed_midpoints( file, &mesh->edge_lookup,
                                    mesh->active_flags, visited_flags,
                                    &mesh->triangles[tri] ) != OK )
            return( ERROR );
    }

    FREE( visited_flags );

    (void) close_file( file );

    return( OK );
}

private   void  make_meshes_same(
    tri_mesh_struct      *dest_mesh,
    tri_mesh_struct      *src_mesh,
    tri_node_struct      *dest_node,
    tri_node_struct      *src_node )
{
    if( dest_node->children[0] == NULL &&
        src_node->children[0] != NULL )
    {
        delete_tri_node( src_node->children[0] );
        delete_tri_node( src_node->children[1] );
        delete_tri_node( src_node->children[2] );
        delete_tri_node( src_node->children[3] );
        src_node->children[0] = NULL;
        src_node->children[1] = NULL;
        src_node->children[2] = NULL;
        src_node->children[3] = NULL;
    }

    if( dest_node->children[0] != NULL &&
        src_node->children[0] == NULL )
    {
        subdivide_tri_node( src_mesh, src_node, TRUE );
    }

    if( dest_node->children[0] != NULL )
    {
        make_meshes_same( dest_mesh, src_mesh, dest_node->children[0],
                          src_node->children[0] );   
        make_meshes_same( dest_mesh, src_mesh, dest_node->children[1],
                          src_node->children[1] );   
        make_meshes_same( dest_mesh, src_mesh, dest_node->children[2],
                          src_node->children[2] );   
        make_meshes_same( dest_mesh, src_mesh, dest_node->children[3],
                          src_node->children[3] );   
    }
}

private  void  tri_mesh_make_meshes_same(
    tri_mesh_struct  *dest_mesh,
    tri_mesh_struct  *src_mesh )
{
    int   tri;

    if( dest_mesh->n_triangles != src_mesh->n_triangles )
    {
        print_error( "tri_mesh_make_meshes_same():  error in n_triangles\n" );
        return;
    }

    for_less( tri, 0, dest_mesh->n_triangles )
    {
        make_meshes_same( dest_mesh, src_mesh,
                          &dest_mesh->triangles[tri],
                          &src_mesh->triangles[tri] );
    }
}

private   void  recursive_reconcile_points(
    tri_mesh_struct      *dest_mesh,
    tri_mesh_struct      *src_mesh,
    tri_node_struct      *dest_node,
    tri_node_struct      *src_node )
{
    int   node, child;

    if( dest_node == NULL )
        return;

    for_less( node, 0, 3 )
    {
        dest_mesh->points[dest_node->nodes[node]] =
                     src_mesh->points[src_node->nodes[node]];
    }

    for_less( child, 0, 4 )
        recursive_reconcile_points( dest_mesh, src_mesh,
                                    dest_node->children[child],
                                    src_node->children[child] );
}

public  void  tri_mesh_reconcile_points(
    tri_mesh_struct  *dest_mesh,
    tri_mesh_struct  *src_mesh )
{
    int   tri;

    tri_mesh_make_meshes_same( dest_mesh, src_mesh );

    for_less( tri, 0, dest_mesh->n_triangles )
    {
        recursive_reconcile_points( dest_mesh, src_mesh,
                                    &dest_mesh->triangles[tri],
                                    &src_mesh->triangles[tri] );
    }
}
