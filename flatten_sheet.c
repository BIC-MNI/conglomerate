#include  <internal_volume_io.h>
#include  <bicpl.h>

#undef   USING_FLOAT
#define  USING_FLOAT

#ifdef USING_FLOAT

typedef  float  ftype;
#define  MINIMIZE_LSQ    minimize_lsq_float
#define  INITIALIZE_LSQ  initialize_lsq_terms_float
#define  ADD_TO_LSQ      add_to_lsq_terms_float
#define  REALLOC_LSQ     realloc_lsq_terms_float
#define  DELETE_LSQ      delete_lsq_terms_float

#else

typedef  Real   ftype;
#define  MINIMIZE_LSQ    minimize_lsq
#define  MINIMIZE_LSQ    minimize_lsq
#define  INITIALIZE_LSQ  initialize_lsq_terms
#define  ADD_TO_LSQ      add_to_lsq_terms
#define  REALLOC_LSQ     realloc_lsq_terms
#define  DELETE_LSQ      delete_lsq_terms

#endif

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
    polygons_struct      *polygons;
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

    if( get_string_argument( NULL, &initial_filename ) )
    {
        if( input_graphics_file( initial_filename, &format, &n_i_objects,
                                 &i_object_list ) != OK || n_i_objects != 1 ||
            get_object_type(i_object_list[0]) != POLYGONS )
            return( 1 );

        polygons = get_polygons_ptr( i_object_list[0] );
        init_points = polygons->points;
        ALLOC( polygons->points, 1 );
        delete_object_list( n_i_objects, i_object_list );
    }
    else
    {
        init_points = NULL;
    }

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    flatten_polygons( polygons, init_points, n_iters );

    (void) output_graphics_file( dest_filename, format, 1, object_list );

    return( 0 );
}

private  void  create_coefficients(
    polygons_struct  *polygons,
    int              n_neighbours[],
    int              **neighbours,
    Smallest_int     interior_flags[],
    int              n_fixed,
    int              fixed_indices[],
    Real             *fixed_pos[2],
    int              to_parameters[],
    int              to_fixed_index[],
    Real             *constant,
    ftype            *linear_terms[],
    ftype            *square_terms[],
    int              *n_cross_terms[],
    int              **cross_parms[],
    ftype            **cross_terms[] )
{
    int              node, p, dim, n_nodes_in, n_parameters;
    int              neigh, *indices, parm;
    Point            neigh_points[MAX_POINTS_PER_POLYGON];
    Real             flat[2][MAX_POINTS_PER_POLYGON];
    Real             *weights[2][2], cons[2], con;
    Real             *node_weights;
    BOOLEAN          found, ignoring;
    progress_struct  progress;

    n_parameters = 2 * (polygons->n_points - n_fixed);

    INITIALIZE_LSQ( n_parameters, constant, linear_terms, square_terms,
                    n_cross_terms, cross_parms, cross_terms );

    ALLOC( weights[0][0], MAX_POINTS_PER_POLYGON );
    ALLOC( weights[0][1], MAX_POINTS_PER_POLYGON );
    ALLOC( weights[1][0], MAX_POINTS_PER_POLYGON );
    ALLOC( weights[1][1], MAX_POINTS_PER_POLYGON );

    initialize_progress_report( &progress, FALSE, polygons->n_points,
                                "Creating coefficients" );

    ALLOC( node_weights, n_parameters );
    ALLOC( indices, n_parameters );

    for_less( node, 0, polygons->n_points )
    {
        parm = to_parameters[node];
        found = (parm >= 0);

        for_less( p, 0, n_neighbours[node] )
        {
            if( to_parameters[neighbours[node][p]] >= 0 )
            {
                found = TRUE;
                break;
            }
        }

        if( !found || n_neighbours[node] < 2 )
            continue;

        for_less( p, 0, n_neighbours[node] )
            neigh_points[p] = polygons->points[neighbours[node][p]];

        flatten_around_vertex( &polygons->points[node],
                               n_neighbours[node], neigh_points,
                               (BOOLEAN) interior_flags[node],
                               flat[0], flat[1] );

        ignoring = FALSE;
        if( !get_prediction_weights_2d( 0.0, 0.0, n_neighbours[node],
                                         flat[0], flat[1],
                                         weights[0], &cons[0],
                                         weights[1], &cons[1] ) )
        {
            print_error( "Error in interpolation weights, ignoring..\n" );
            ignoring = TRUE;
        }

        if( ignoring )
            continue;

        for_less( dim, 0, 2 )
        {
            con = -cons[dim];
            n_nodes_in = 0;
            if( to_parameters[node] >= 0 )
            {
                indices[n_nodes_in] = IJ(to_parameters[node],dim,2);
                node_weights[n_nodes_in] = 1.0;
                ++n_nodes_in;
            }
            else
                con += fixed_pos[dim][to_fixed_index[node]];

            for_less( p, 0, n_neighbours[node] )
            {
                neigh = neighbours[node][p];
                if( to_parameters[neigh] >= 0 )
                {
                    indices[n_nodes_in] = IJ(to_parameters[neigh],0,2);
                    node_weights[n_nodes_in] = -weights[dim][0][p];
                    ++n_nodes_in;

                    indices[n_nodes_in] = IJ(to_parameters[neigh],1,2);
                    node_weights[n_nodes_in] = -weights[dim][1][p];
                    ++n_nodes_in;
                }
                else
                {
                    con += -weights[dim][0][p] *
                            fixed_pos[0][to_fixed_index[neigh]]
                            -weights[dim][1][p] *
                            fixed_pos[1][to_fixed_index[neigh]];
                }
            }

            ADD_TO_LSQ( n_parameters, constant, *linear_terms, *square_terms,
                        *n_cross_terms, *cross_parms, *cross_terms,
                        n_nodes_in, indices, node_weights, con, 5 );
        }

        update_progress_report( &progress, node + 1 );
    }

    terminate_progress_report( &progress );

    REALLOC_LSQ( n_parameters, *n_cross_terms, *cross_parms, *cross_terms );

    FREE( node_weights );
    FREE( indices );

    FREE( weights[0][0] );
    FREE( weights[0][1] );
    FREE( weights[1][0] );
    FREE( weights[1][1] );
}

private  void  flatten_polygons(
    polygons_struct  *polygons,
    Point            init_points[],
    int              n_iters )
{
    int              i, p, point, *n_neighbours, **neighbours, n, neigh, which;
    int              n_fixed, *fixed_indices;
    Real             *fixed_pos[2], scale, dist1, dist2;
    Real             sum_xx, sum_xy;
    Real             constant;
    int              *n_cross_terms, **cross_parms;
    ftype            *linear_terms, *square_terms, **cross_terms;
    Point            neigh_points[MAX_POINTS_PER_POLYGON], *new_points;
    Real             *parameters;
    int              *to_parameters, *to_fixed_index, ind;
    Vector           x_dir, y_dir, offset;
    Smallest_int     *interior_flags;

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, &interior_flags, NULL );

    for_less( which, 0, polygons->n_points )
    {
        if( n_neighbours[which] > 1 )
            break;
    }

    n_fixed = 1 + n_neighbours[which];
    ALLOC( fixed_indices, n_fixed );
    ALLOC( fixed_pos[0], n_fixed );
    ALLOC( fixed_pos[1], n_fixed );

    fixed_indices[0] = which;
    for_less( i, 0, n_neighbours[which] )
        fixed_indices[i+1] = neighbours[which][i];

    for_less( i, 0, n_neighbours[which] )
        neigh_points[i] = polygons->points[neighbours[which][i]];

    fixed_pos[0][0] = 0.0;
    fixed_pos[1][0] = 0.0;

    flatten_around_vertex( &polygons->points[which], n_neighbours[which],
                           neigh_points, (BOOLEAN) interior_flags[which],
                           &fixed_pos[0][1], &fixed_pos[1][1] );

    n_fixed = 3;

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

    create_coefficients( polygons, n_neighbours, neighbours, interior_flags,
                         n_fixed, fixed_indices, fixed_pos,
                         to_parameters, to_fixed_index,
                         &constant, &linear_terms, &square_terms,
                         &n_cross_terms, &cross_parms, &cross_terms );

    ALLOC( parameters, 2 * (polygons->n_points - n_fixed) );

    if( init_points == NULL )
        init_points = polygons->points;

    SUB_POINTS( x_dir, init_points[neighbours[which][0]],
                init_points[which] );
    NORMALIZE_VECTOR( x_dir, x_dir );
    fill_Point( y_dir, -RVector_y(x_dir), RVector_x(x_dir), 0.0 );
    for_less( point, 0, polygons->n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            SUB_POINTS( offset, init_points[point], init_points[which] );
            parameters[2*to_parameters[point]] =
                           DOT_VECTORS( offset, x_dir );
            parameters[2*to_parameters[point]+1] =
                           DOT_VECTORS( offset, y_dir );
        }
    }

    (void) MINIMIZE_LSQ( 2 * (polygons->n_points - n_fixed),
                         constant, linear_terms, square_terms,
                         n_cross_terms, cross_parms, cross_terms,
                         -1.0, n_iters, parameters );

    FREE( linear_terms );
    FREE( square_terms );
    for_less( p, 0, 2 * (polygons->n_points - n_fixed) )
    {
        if( n_cross_terms[p] > 0 )
        {
            FREE( cross_parms[p] );
            FREE( cross_terms[p] );
        }
    }
    FREE( n_cross_terms );

    FREE( cross_parms );
    FREE( cross_terms );

    ALLOC( new_points, polygons->n_points );

    for_less( point, 0, polygons->n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            fill_Point( new_points[point],
                        parameters[2*to_parameters[point]],
                        parameters[2*to_parameters[point]+1], 0.0 );
        }
        else
        {
            fill_Point( new_points[point],
                        fixed_pos[0][to_fixed_index[point]],
                        fixed_pos[1][to_fixed_index[point]], 0.0 );
        }
    }

    FREE( parameters );

    sum_xx = 0.0;
    sum_xy = 0.0;
    for_less( point, 0, polygons->n_points )
    {
        for_less( n, 0, n_neighbours[point] )
        {
            neigh = neighbours[point][n];
            dist1 = distance_between_points( &polygons->points[point],
                                             &polygons->points[neigh] );
            dist2 = distance_between_points( &new_points[point],
                                             &new_points[neigh] );

            sum_xx += dist2 * dist2;
            sum_xy += dist1 * dist2;
        }
    }

    delete_polygon_point_neighbours( polygons, n_neighbours, neighbours,
                                     interior_flags, NULL );

    if( sum_xx == 0.0 )
    {
        print_error( "sum_xx = 0.0" );
        scale = 1.0;
    }
    else
    {
        scale = sum_xy / sum_xx;
    }

    scale = 1.0;
    for_less( point, 0, polygons->n_points )
    {
        SCALE_POINT( polygons->points[point], new_points[point], scale );
    }

    FREE( new_points );
    FREE( to_parameters );
    FREE( to_fixed_index );
    FREE( fixed_pos[0] );
    FREE( fixed_pos[1] );
}
