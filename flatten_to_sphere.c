#include  <internal_volume_io.h>
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
    int              counts[MAX_POINTS_PER_POLYGON];
    Point            neigh_points[MAX_POINTS_PER_POLYGON];
    Point            neigh_neigh_points[MAX_POINTS_PER_POLYGON];
    Real             x_sphere1[MAX_POINTS_PER_POLYGON], x, y, z;
    Real             y_sphere1[MAX_POINTS_PER_POLYGON];
    Real             z_sphere1[MAX_POINTS_PER_POLYGON], len, factor;
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

    return( n_neighbours[node] + n_nn );
}

#define SPHERE

private  int  create_coefficients(
    polygons_struct  *polygons,
    int              n_neighbours[],
    int              **neighbours,
    int              n_fixed,
    int              fixed_indices[],
    Real             *fixed_pos[3],
    int              to_parameters[],
    int              to_fixed_index[],
    int              *n_nodes_involved[],
    int              **node_list[],
    Real             *constants[],
    Real             **node_weights[] )
{
    int              node, p, eq, dim, max_neighbours;
    int              neigh, ind, dim1, n_neighs;
    Real             x_sphere[100];
    Real             y_sphere[100];
    Real             z_sphere[100];
    int              neigh_indices[100];
    Real             *weights[3][3];
    Real             radius, con;
    int              nodes[100];
    Real             eq_weights[100];
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

    *n_nodes_involved = NULL;
    *constants = NULL;
    *node_weights = NULL;
    *node_list = NULL;

    eq = 0;

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
                nodes[ind] = 3 * to_parameters[node] + dim;
                eq_weights[ind] = 1.0;
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
                            nodes[ind] = 3 * to_parameters[neigh] + dim1;
                            eq_weights[ind] = -weights[dim][dim1][p];
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

            if( ind > 0 )
            {
                SET_ARRAY_SIZE( *n_nodes_involved, eq,eq+1,DEFAULT_CHUNK_SIZE );
                SET_ARRAY_SIZE( *node_list, eq,eq+1,DEFAULT_CHUNK_SIZE );
                SET_ARRAY_SIZE( *node_weights, eq,eq+1,DEFAULT_CHUNK_SIZE );
                SET_ARRAY_SIZE( *constants, eq,eq+1,DEFAULT_CHUNK_SIZE );
                (*n_nodes_involved)[eq] = ind;
                (*constants)[eq] = con;
                ALLOC( (*node_list)[eq], (*n_nodes_involved)[eq] );
                ALLOC( (*node_weights)[eq], (*n_nodes_involved)[eq] );
                for_less( ind, 0, (*n_nodes_involved)[eq] )
                {
                    (*node_list)[eq][ind] = nodes[ind];
                    (*node_weights)[eq][ind] = eq_weights[ind];
                }
                ++eq;
            }
        }

        update_progress_report( &progress, node + 1 );
    }

    terminate_progress_report( &progress );

#ifdef DEBUG
#define DEBUG
    {
        for_less( ind, 0, eq )
        {
            print( "%d: %g : ", ind, (*constants)[ind] );
            for_less( p, 0, (*n_nodes_involved)[ind] )
                print( " %d:%g", (*node_list)[ind][p], (*node_weights)[ind][p] );
            print( "\n" );
        }
    }

#endif

    for_less( dim, 0, N_DIMENSIONS )
    {
        FREE( weights[0][dim] );
        FREE( weights[1][dim] );
        FREE( weights[2][dim] );
    }

    return( eq );
}

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Point            init_points[],
    int              n_iters )
{
    int              i, point, *n_neighbours, **neighbours;
    int              n_equations, *n_nodes_per_equation, **node_list;
    int              n_fixed, *fixed_indices, size, dim;
    Real             *fixed_pos[3];
    Real             *constants, **node_weights;
    Real             *parameters;
    int              *to_parameters, *to_fixed_index, ind;

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    size = GET_OBJECT_SIZE( *polygons, 0 );
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


    n_equations = create_coefficients( polygons,
                                       n_neighbours, neighbours,
                                       n_fixed, fixed_indices, fixed_pos,
                                       to_parameters, to_fixed_index,
                                       &n_nodes_per_equation,
                                       &node_list, &constants, &node_weights );

    delete_polygon_point_neighbours( polygons, n_neighbours, neighbours,
                                     NULL, NULL );

    ALLOC( parameters, 3 * (polygons->n_points - n_fixed) );

    for_less( point, 0, polygons->n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            parameters[3*to_parameters[point]+0] = RPoint_x(init_points[point]);
            parameters[3*to_parameters[point]+1] = RPoint_y(init_points[point]);
            parameters[3*to_parameters[point]+2] = RPoint_z(init_points[point]);
        }
    }

#ifdef DEBUG
#define DEBUG
#undef DEBUG
    {
    int eq;
    Real value;
        for_less( eq, 0, n_equations )
        {
            value = constants[eq];
            for_less( point, 0, n_nodes_per_equation[eq] )
            {
                value += node_weights[eq][point] *
                         parameters[node_list[eq][point]];
            }

            if( value > 1e-6 )
                print( "%d: %g\n", eq, value );
        }
    }
#endif


    (void) minimize_lsq( 3 * (polygons->n_points - n_fixed), n_equations,
                         n_nodes_per_equation, node_list, constants,
                         node_weights, n_iters, parameters );

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