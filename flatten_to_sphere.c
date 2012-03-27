#include  <volume_io.h>
#include  <bicpl.h>

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Point            init_points[],
    int              n_iters );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               src_filename, dest_filename, initial_filename;
    int                  n_objects, n_i_objects, n_iters;
    File_formats         format;
    object_struct        **object_list, **i_object_list;
    polygons_struct      *polygons, *init_polygons;
    Point                *init_points;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) )
    {
        print_error( "Usage: %s  input.obj output.obj [n_iters]\n",
                     argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 100, &n_iters );

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    if( get_string_argument( NULL, &initial_filename ) )
    {
        if( input_graphics_file( initial_filename, &format, &n_i_objects,
                                 &i_object_list ) != OK || n_i_objects != 1 ||
            get_object_type(i_object_list[0]) != POLYGONS )
            return( 1 );

        init_polygons = get_polygons_ptr( i_object_list[0] );
        init_points = init_polygons->points;
        ALLOC( init_polygons->points, 1 );
        delete_object_list( n_i_objects, i_object_list );
    }
    else
    {
        init_points = polygons->points;
    }

    flatten_polygons( polygons, init_points, n_iters );

    if( output_graphics_file( dest_filename, format, 1, object_list ) != OK )
        print_error( "Error outputting: %s\n", dest_filename );

    return( 0 );
}

private  int   get_neighbours_neighbours(
    int    node,
    int    n_neighbours[],
    int    *neighbours[],
    int    neigh_indices[] )
{
    int   n, n_nn;
    int   i, nn, neigh, neigh_neigh;

    n_nn = 0;

    for_less( n, 0, n_neighbours[node] )
    {
        neigh = neighbours[node][n];
        for_less( nn, 0, n_neighbours[neigh] )
        {
            neigh_neigh = neighbours[neigh][nn];
            if( neigh_neigh == node )
                continue;

            for_less( i, 0, n_neighbours[node] )
            {
                if( neighbours[node][i] == neigh_neigh )
                    break;
            }

            if( i < n_neighbours[node] )
                continue;

            for_less( i, 0, n_nn )
            {
                if( neigh_indices[i] == neigh_neigh )
                    break;
            }

            if( i >= n_nn )
            {
                neigh_indices[n_nn] = neigh_neigh;
                ++n_nn;
            }
        }
    }

    return( n_nn );
}

private  void  create_mesh(
    int            node,
    int            n_neighbours[],
    int            *neighbours[],
    int            total_neighbours,
    int            neigh_indices[],
    Real           x_sphere[],
    Real           y_sphere[],
    Real           z_sphere[] )
{
    object_struct  *object;
    lines_struct   *lines;
    int            t, n, nn, neigh, current;
    Point          point1, point2;
    char           name[EXTREMELY_LARGE_STRING_SIZE];

    object = create_object( LINES );
    lines = get_lines_ptr( object );

    initialize_lines( lines, WHITE );

    for_less( n, -1, n_neighbours[node] )
    {
        if( n == -1 )
        {
            current = node;
            fill_Point( point1, 0.0, 0.0, 0.0 );
        }
        else
        {
            current = neighbours[node][n];
            fill_Point( point1, x_sphere[n], y_sphere[n], z_sphere[n] );
        }

        for_less( nn, 0, n_neighbours[current] )
        {
            neigh = neighbours[current][nn];
            if( neigh == node )
            {
                fill_Point( point2, 0.0, 0.0, 0.0 );
            }
            else
            {
                for_less( t, 0, total_neighbours )
                    if( neigh == neigh_indices[t] )
                        break;

                if( t >= total_neighbours )
                    handle_internal_error( "t >= neighbours" );

                fill_Point( point2, x_sphere[t], y_sphere[t], z_sphere[t] );
            }

            start_new_line( lines );
            add_point_to_line( lines, &point1 );
            add_point_to_line( lines, &point2 );
        }
    }

    (void) sprintf( name, "lines_%d.obj", node );
    (void) output_graphics_file( name, BINARY_FORMAT, 1, &object );

    delete_object( object );
}

#define  MAX_NN 1000

private  int   flatten_patches_to_sphere(
    Real             radius,
    polygons_struct  *polygons,
    int              node,
    int              n_neighbours[],
    int              **neighbours,
    int              neigh_indices[],
    Real             x_sphere[],
    Real             y_sphere[],
    Real             z_sphere[] )
{
    int              n_nn, neigh, neigh_neigh, i, p, n, which;
    int              desired_node;
    int              counts[MAX_NN];
    Point            neigh_points[MAX_NN];
    Point            neigh_neigh_points[MAX_NN];
    Real             x_sphere1[MAX_NN], x, y, z;
    Real             y_sphere1[MAX_NN];
    Real             z_sphere1[MAX_NN], len, factor;
    Vector           x_axis, y_axis, z_axis;
    Point            origin;
    Transform        transform, from, to;
    
    for_less( n, 0, n_neighbours[node] )
    {
        neigh_indices[n] = neighbours[node][n];
        neigh_points[n] = polygons->points[neighbours[node][n]];
    }

    flatten_around_vertex_to_sphere( radius, &polygons->points[node],
                                     n_neighbours[node], neigh_points,
                                     x_sphere, y_sphere, z_sphere );

    n_nn = get_neighbours_neighbours( node, n_neighbours, neighbours,
                                      &neigh_indices[n_neighbours[node]] );

    if( n_neighbours[node] + n_nn > MAX_NN )
        handle_internal_error( "Must rewrite code to handle large NN" );

    for_less( i, 0, n_nn )
    {
        x_sphere[n_neighbours[node]+i] = 0.0;
        y_sphere[n_neighbours[node]+i] = 0.0;
        z_sphere[n_neighbours[node]+i] = 0.0;
        counts[i] = 0;
    }

    for_less( n, 0, n_neighbours[node] )
    {
        neigh = neighbours[node][n];
        for_less( p, 0, n_neighbours[neigh] )
            neigh_neigh_points[p] = polygons->points[neighbours[neigh][p]];

        flatten_around_vertex_to_sphere( radius, &polygons->points[neigh],
                                         n_neighbours[neigh],
                                         neigh_neigh_points,
                                         x_sphere1, y_sphere1, z_sphere1 );

        for_less( which, 0, n_neighbours[neigh] )
            if( neighbours[neigh][which] == node )
                break;

        fill_Vector( z_axis, 0.0, 0.0, 1.0 );
        fill_Point( origin, 0.0, 0.0, 0.0 );
        fill_Vector( x_axis, x_sphere1[which], y_sphere1[which],
                     z_sphere1[which] );
        CROSS_VECTORS( y_axis, z_axis, x_axis );
        CROSS_VECTORS( z_axis, x_axis, y_axis );
        NORMALIZE_VECTOR( x_axis, x_axis );
        NORMALIZE_VECTOR( y_axis, y_axis );
        NORMALIZE_VECTOR( z_axis, z_axis );

        make_change_from_bases_transform( &origin, &x_axis, &y_axis, &z_axis,
                                          &from );

        fill_Vector( z_axis, 0.0, 0.0, 1.0 );
        fill_Point( origin, x_sphere[n], y_sphere[n], z_sphere[n] );
        fill_Vector( x_axis, -x_sphere[n], -y_sphere[n], -z_sphere[n] );
        CROSS_VECTORS( y_axis, z_axis, x_axis );
        CROSS_VECTORS( z_axis, x_axis, y_axis );
        NORMALIZE_VECTOR( x_axis, x_axis );
        NORMALIZE_VECTOR( y_axis, y_axis );
        NORMALIZE_VECTOR( z_axis, z_axis );

        make_change_to_bases_transform( &origin, &x_axis, &y_axis, &z_axis,
                                        &to );

        concat_transforms( &transform, &from, &to );

        for_less( p, 0, n_neighbours[neigh] )
        {
            neigh_neigh = neighbours[neigh][p];

            for_less( which, 0, n_nn )
            {
                if( neigh_neigh == neigh_indices[n_neighbours[node]+which] )
                    break;
            }

            if( which >= n_nn )
                continue;

            transform_point( &transform, x_sphere1[p], y_sphere1[p],
                             z_sphere1[p], &x, &y, &z );

            x_sphere[n_neighbours[node]+which] += x;
            y_sphere[n_neighbours[node]+which] += y;
            z_sphere[n_neighbours[node]+which] += z;
            ++counts[which];

            z += radius;
            len = x * x + y * y + z * z;
            if( !numerically_close( len, radius * radius, 1.0e-4 ) )
                print( "Not close: %g %g %g\n", x, y, z );
        }
    }

    for_less( i, 0, n_nn )
    {
        x_sphere[n_neighbours[node]+i] /= (Real) counts[i];
        y_sphere[n_neighbours[node]+i] /= (Real) counts[i];
        z_sphere[n_neighbours[node]+i] /= (Real) counts[i];

        len = x_sphere[n_neighbours[node]+i] * x_sphere[n_neighbours[node]+i] +
              y_sphere[n_neighbours[node]+i] * y_sphere[n_neighbours[node]+i] +
              (z_sphere[n_neighbours[node]+i] + radius) *
              (z_sphere[n_neighbours[node]+i] + radius);

        if( len == 0.0 )
            handle_internal_error( "len = 0.0" );

        factor = radius / sqrt( len );
        x_sphere[n_neighbours[node]+i] *= factor;
        y_sphere[n_neighbours[node]+i] *= factor;
        z_sphere[n_neighbours[node]+i] = (z_sphere[n_neighbours[node]+i] +
                                          radius) * factor - radius;
    }

    if( getenv("NODE") != NULL &&
        sscanf( getenv("NODE"), "%d", &desired_node ) == 1 &&
        desired_node == node )
    {
        print( "Creating mesh for node %d\n", node );
        create_mesh( node, n_neighbours, neighbours, n_neighbours[node] + n_nn,
                     neigh_indices, x_sphere, y_sphere, z_sphere );
    }

    return( n_neighbours[node] + n_nn );
}

private  void  create_coefficients(
    polygons_struct  *polygons,
    int              n_neighbours[],
    int              **neighbours,
    int              n_fixed,
    int              fixed_indices[],
    Real             *fixed_pos[3],
    int              to_parameters[],
    int              to_fixed_index[],
    Real             *constant,
    float            *linear_terms[],
    float            *square_terms[],
    int              *n_cross_terms[],
    int              **cross_parms[],
    float            **cross_terms[] )
{
    int              node, p, dim, max_neighbours;
    int              neigh, ind, dim1, n_neighs, n_parameters;
    Real             x_sphere[100];
    Real             y_sphere[100];
    Real             z_sphere[100];
    int              neigh_indices[100];
    Real             *weights[3][3];
    Real             radius, con, *node_weights;
    int              *indices;
    BOOLEAN          found, ignoring;
    progress_struct  progress;
#ifdef SPHERE
    polygons_struct  unit_sphere;
    static  Point            centre = { 0.0f, 0.0f, 0.0f };
#endif

    radius = sqrt( get_polygons_surface_area( polygons ) / 4.0 / PI );

#ifdef SPHERE
    radius = 10.0;
    create_tetrahedral_sphere( &centre, radius, radius, radius, polygons->n_items, &unit_sphere );
#endif

    print( "Radius: %g\n", radius );

    n_parameters = 3 * (polygons->n_points - n_fixed);
    ALLOC( node_weights, n_parameters );
    ALLOC( indices, n_parameters );

    max_neighbours = 0;
    for_less( node, 0, polygons->n_points )
        max_neighbours = MAX( max_neighbours, n_neighbours[node] );

    max_neighbours = MIN((1+max_neighbours)*max_neighbours,polygons->n_points );

    for_less( dim, 0, N_DIMENSIONS )
    {
        ALLOC( weights[0][dim], max_neighbours );
        ALLOC( weights[1][dim], max_neighbours );
        ALLOC( weights[2][dim], max_neighbours );
    }

    initialize_lsq_terms_float( n_parameters, constant, linear_terms,
                                square_terms, n_cross_terms, cross_parms,
                                cross_terms );

    initialize_progress_report( &progress, FALSE, polygons->n_points,
                                "Creating Coefficients" );

    for_less( node, 0, polygons->n_points )
    {
        found = to_parameters[node] >= 0;

        for_less( p, 0, n_neighbours[node] )
        {
            if( to_parameters[neighbours[node][p]] >= 0 )
                found = TRUE;
        }

        if( !found )
            continue;

        n_neighs = flatten_patches_to_sphere(
                                    radius, polygons, node,
                                    n_neighbours, neighbours,
                                    neigh_indices,
                                    x_sphere, y_sphere, z_sphere );
#ifdef SPHERE
        for_less( p, 0, n_neighs )
        {
            x_sphere[p] = RPoint_x(unit_sphere.points[neigh_indices[p]]) -
                          RPoint_x(unit_sphere.points[node]);
            y_sphere[p] = RPoint_y(unit_sphere.points[neigh_indices[p]]) -
                          RPoint_y(unit_sphere.points[node]);
            z_sphere[p] = RPoint_z(unit_sphere.points[neigh_indices[p]]) -
                          RPoint_z(unit_sphere.points[node]);
        }
#endif

        ignoring = FALSE;
        if( !get_prediction_weights_3d( 0.0, 0.0, 0.0,
                                        n_neighs,
                                        x_sphere, y_sphere, z_sphere,
                                        weights[0], weights[1], weights[2] ) )
                                         
        {
            print_error( "Error in interpolation weights, ignoring..\n" );
            ignoring = TRUE;
        }

        if( !ignoring )
        {
            for_less( dim, 0, N_DIMENSIONS )
            for_less( dim1, 0, N_DIMENSIONS )
            for_less( p, 0, n_neighs )
            {
                if( FABS( weights[dim][dim1][p] ) > 100.0 )
                {
                    print_error( "Interpolation weights too high: %d %d %d %g\n",
                                 p, dim, dim1, weights[dim][dim1][p] );
                    ignoring = TRUE;
                }
            }
        }

        if( ignoring )
            continue;

        for_less( dim, 0, N_DIMENSIONS )
        {
            ind = 0;

            if( to_parameters[node] >= 0 )
            {
                indices[ind] = 3 * to_parameters[node] + dim;
                node_weights[ind] = 1.0;
                ++ind;
                con = 0.0;
            }
            else
            {
                con = fixed_pos[dim][to_fixed_index[node]];
            }

            for_less( p, 0, n_neighs )
            {
                neigh = neigh_indices[p];
                if( to_parameters[neigh] >= 0 )
                {
                    for_less( dim1, 0, N_DIMENSIONS )
                    {
                        if( weights[dim][dim1][p] != 0.0 )
                        {
                            indices[ind] = 3 * to_parameters[neigh] + dim1;
                            node_weights[ind] = -weights[dim][dim1][p];
                            ++ind;
                        }
                    }
                }
                else
                {
                    for_less( dim1, 0, N_DIMENSIONS )
                    {
                        con += -weights[dim][dim1][p] *
                              fixed_pos[dim1][to_fixed_index[neigh]];
                    }
                }
            }

            add_to_lsq_terms_float( n_parameters, constant, *linear_terms,
                                    *square_terms, *n_cross_terms,
                                    *cross_parms, *cross_terms, ind,
                                    indices, node_weights, con, 5 );
        }

        update_progress_report( &progress, node + 1 );
    }

    terminate_progress_report( &progress );

    for_less( dim, 0, N_DIMENSIONS )
    {
        FREE( weights[0][dim] );
        FREE( weights[1][dim] );
        FREE( weights[2][dim] );
    }

    FREE( node_weights );
    FREE( indices );

    realloc_lsq_terms_float( n_parameters, *n_cross_terms, *cross_parms,
                             *cross_terms );
}

private  void   get_transform(
    int         n_points,
    Point       src_points[],
    Point       dest_points[],
    Transform   *transform )
{
    int                p, dim;
    Real               **src_tags, **dest_tags;
    Real               src_area, dest_area, scale;
    Transform          from, to, scale_transform;
    Point              origin;
    Vector             hor, vert, normal;
    General_transform  gen_transform;

    if( n_points == 3 )
    {
        origin = src_points[0];
        SUB_VECTORS( hor, src_points[1], src_points[0] );
        SUB_VECTORS( vert, src_points[2], src_points[0] );
        CROSS_VECTORS( normal, hor, vert );
        src_area = MAGNITUDE( normal );
        CROSS_VECTORS( vert, normal, hor );
        NORMALIZE_VECTOR( normal, normal );
        NORMALIZE_VECTOR( hor, hor );
        NORMALIZE_VECTOR( vert, vert );
        make_change_from_bases_transform( &origin, &hor, &vert, &normal, &from);

        origin = dest_points[0];
        SUB_VECTORS( hor, dest_points[1], dest_points[0] );
        SUB_VECTORS( vert, dest_points[2], dest_points[0] );
        CROSS_VECTORS( normal, hor, vert );
        dest_area = MAGNITUDE( normal );
        CROSS_VECTORS( vert, normal, hor );
        NORMALIZE_VECTOR( normal, normal );
        NORMALIZE_VECTOR( hor, hor );
        NORMALIZE_VECTOR( vert, vert );
        make_change_to_bases_transform( &origin, &hor, &vert, &normal, &to );

        scale = sqrt( dest_area / src_area );
        make_scale_transform( scale, scale, scale, &scale_transform );

        concat_transforms( transform, &from, &scale_transform );
        concat_transforms( transform, transform, &to );
    }
    else
    {
        ALLOC2D( src_tags, n_points, N_DIMENSIONS );
        ALLOC2D( dest_tags, n_points, N_DIMENSIONS );

        for_less( p, 0, n_points )
        {
            for_less( dim, 0, N_DIMENSIONS )
            {
                src_tags[p][dim] = RPoint_coord(src_points[p],dim);
                dest_tags[p][dim] = RPoint_coord(dest_points[p],dim);
            }
        }

        compute_transform_from_tags( n_points, dest_tags, src_tags, TRANS_LSQ7,
                                     &gen_transform );

        FREE2D( src_tags );
        FREE2D( dest_tags );
        *transform = *get_linear_transform_ptr( &gen_transform );
        delete_general_transform( &gen_transform );
    }
}

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Point            init_points[],
    int              n_iters )
{
    int              i, point, *n_neighbours, **neighbours;
    int              n_fixed, *fixed_indices, size, dim;
    int              n_parameters;
    Real             *fixed_pos[3], x, y, z;
    Real             constant;
    float            *linear_terms, *square_terms, **cross_terms;
    int              *n_cross_terms, **cross_parms;
    Real             *parameters;
    int              *to_parameters, *to_fixed_index, ind;
    Point            first_points[MAX_POINTS_PER_POLYGON];
    Point            init_first_points[MAX_POINTS_PER_POLYGON];
    Transform        transform;

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    size = GET_OBJECT_SIZE( *polygons, 0 );
    for_less( i, 0, size )
    {
        first_points[i] = polygons->points[polygons->indices[
                           POINT_INDEX(polygons->end_indices,0,i)]];
        init_first_points[i] = init_points[polygons->indices[
                                  POINT_INDEX(polygons->end_indices,0,i)]];
    }

    get_transform( size, init_first_points, first_points, &transform );

    n_fixed = size;
    ALLOC( fixed_indices, n_fixed );
    ALLOC( fixed_pos[0], n_fixed );
    ALLOC( fixed_pos[1], n_fixed );
    ALLOC( fixed_pos[2], n_fixed );

    for_less( i, 0, size )
    {
        fixed_indices[i] = polygons->indices[
                           POINT_INDEX(polygons->end_indices,0,i)];
        for_less( dim, 0, N_DIMENSIONS )
            fixed_pos[dim][i] = RPoint_coord(
                                   polygons->points[fixed_indices[i]], dim );
    }

    ALLOC( to_parameters, polygons->n_points );
    ALLOC( to_fixed_index, polygons->n_points );
    ind = 0;
    for_less( point, 0, polygons->n_points )
    {
        for_less( i, 0, n_fixed )
        {
            if( point == fixed_indices[i] )
                break;
        }
        if( i < n_fixed )
        {
            to_fixed_index[point] = i;
            to_parameters[point] = -1;
        }
        else
        {
            to_fixed_index[point] = -1;
            to_parameters[point] = ind;
            ++ind;
        }
    }


    create_coefficients( polygons, n_neighbours, neighbours,
                         n_fixed, fixed_indices, fixed_pos,
                         to_parameters, to_fixed_index,
                         &constant, &linear_terms, &square_terms,
                         &n_cross_terms, &cross_parms,
                         &cross_terms );

    delete_polygon_point_neighbours( polygons, n_neighbours, neighbours,
                                     NULL, NULL );

    n_parameters = 3 * (polygons->n_points - n_fixed);
    ALLOC( parameters, n_parameters );

    for_less( point, 0, polygons->n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            transform_point( &transform, 
                             RPoint_x(init_points[point]),
                             RPoint_y(init_points[point]),
                             RPoint_z(init_points[point]),
                             &x, &y, &z );
            parameters[3*to_parameters[point]+0] = x;
            parameters[3*to_parameters[point]+1] = y;
            parameters[3*to_parameters[point]+2] = z;
        }
    }

#ifdef DEBUG
    {
    int   p, n;
    print( "Constant: %g\n", constant );
    for_less( p, 0, n_parameters )
    {
        print( "%3d: %g %g %g : ", p, linear_terms[p], square_terms[p] );
        for_less( n, 0, n_cross_terms[p] )
            print( " %g", cross_terms[p][n] );
        print( "\n" );
    }
    }
#endif

    (void) minimize_lsq_float( n_parameters, constant, linear_terms,
                               square_terms, n_cross_terms, cross_parms,
                               cross_terms, -1.0, n_iters, parameters );

    delete_lsq_terms_float( n_parameters, linear_terms, square_terms,
                            n_cross_terms, cross_parms, cross_terms );

    for_less( point, 0, polygons->n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            fill_Point( polygons->points[point],
                        parameters[3*to_parameters[point]],
                        parameters[3*to_parameters[point]+1],
                        parameters[3*to_parameters[point]+2] );
        }
        else
        {
            fill_Point( polygons->points[point],
                        fixed_pos[0][to_fixed_index[point]],
                        fixed_pos[1][to_fixed_index[point]],
                        fixed_pos[2][to_fixed_index[point]] );
        }
    }

    FREE( parameters );

    FREE( to_parameters );
    FREE( to_fixed_index );
    FREE( fixed_pos[0] );
    FREE( fixed_pos[1] );
    FREE( fixed_pos[2] );
}
