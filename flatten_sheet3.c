#include  <volume_io.h>
#include  <bicpl.h>

#define  USING_FLOAT
#undef   USING_FLOAT

#ifdef USING_FLOAT

typedef  float  ftype;
#define  MINIMIZE_LSQ    minimize_lsq_float
#define  INITIALIZE_LSQ  initialize_lsq_terms_float
#define  ADD_TO_LSQ      add_to_lsq_terms_float
#define  REALLOC_LSQ     realloc_lsq_terms_float
#define  DELETE_LSQ      delete_lsq_terms_float

#else

typedef  VIO_Real   ftype;
#define  MINIMIZE_LSQ    minimize_lsq
#define  MINIMIZE_LSQ    minimize_lsq
#define  INITIALIZE_LSQ  initialize_lsq_terms
#define  ADD_TO_LSQ      add_to_lsq_terms
#define  REALLOC_LSQ     realloc_lsq_terms
#define  DELETE_LSQ      delete_lsq_terms

#endif

static  void  flatten_polygons(
    VIO_BOOL          use_prediction_method,
    int              n_points,
    VIO_Point            points[],
    int              n_neighbours[],
    int              *neighbours[],
    VIO_SCHAR     interior_flags[],
    VIO_Point            init_points[],
    int              n_fixed,
    int              fixed_indices[],
    int              n_iters );

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR               src_filename, dest_filename, initial_filename;
    VIO_STR               fixed_filename, method_name;
    int                  n_objects, n_i_objects, n_iters;
    int                  n_fixed, *fixed_indices, ind, p, n_points;
    VIO_File_formats         format;
    object_struct        **object_list, **i_object_list;
    polygons_struct      *polygons;
    VIO_Point                *init_points, *points;
    int                  *n_neighbours, **neighbours;
    VIO_SCHAR         *interior_flags;
    FILE                 *file;
    VIO_BOOL              use_prediction_method;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &dest_filename ) ||
        !get_string_argument( NULL, &method_name ) )
    {
        print_error( "Usage: %s  input.obj output.obj t|p [n_iters]\n", argv[0] );
        print_error( "          [initfile] [fixedfile]\n" );
        return( 1 );
    }

    (void) get_int_argument( 100, &n_iters );

    if( method_name[0] == 'p' )
        use_prediction_method = TRUE;
    else if( method_name[0] == 't' )
        use_prediction_method = FALSE;
    else
    {
        print_error( "Usage: %s  input.obj output.obj t|p [n_iters]\n", argv[0] );
        print_error( "          [initfile] [fixedfile]\n" );
        return( 1 );
    }

    if( get_string_argument( NULL, &initial_filename ) )
    {
        if( input_graphics_file( initial_filename, &format, &n_i_objects,
                                 &i_object_list ) != VIO_OK || n_i_objects != 1 ||
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

    n_fixed = 0;
    fixed_indices = NULL;
    if( get_string_argument( NULL, &fixed_filename ) )
    {
        if( open_file( fixed_filename, READ_FILE, ASCII_FORMAT, &file ) != VIO_OK )
            return( 1 );

        while( input_int( file, &ind ) == VIO_OK )
        {
            ADD_ELEMENT_TO_ARRAY( fixed_indices, n_fixed, ind,
                                  DEFAULT_CHUNK_SIZE );
        }

        (void) close_file( file );
    }

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != VIO_OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );

    for_less( p, 0, polygons->n_items )
    {
        if( GET_OBJECT_SIZE(*polygons,p) != 3 )
        {
            print_error( "Must be a triangulation.\n" );
            return( 1 );
        }
    }

    polygons = get_polygons_ptr( object_list[0] );

    create_polygon_point_neighbours( polygons, FALSE, &n_neighbours,
                                     &neighbours, &interior_flags, NULL );

    n_points = polygons->n_points;
    points = polygons->points;
    ALLOC( polygons->points, 1 );
    delete_object_list( n_objects, object_list );

    flatten_polygons( use_prediction_method, n_points, points,
                      n_neighbours, neighbours, interior_flags,
                      init_points, n_fixed, fixed_indices, n_iters );

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != VIO_OK || n_objects != 1 ||
        get_object_type(object_list[0]) != POLYGONS )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );
    FREE( polygons->points );
    polygons->points = points;

    (void) output_graphics_file( dest_filename, format, n_objects, object_list);

    return( 0 );
}

static  void  flatten_around_point(
    VIO_Point            points[],
    int              point,
    int              n_neighbours[],
    int              **neighbours,
    VIO_SCHAR     interior_flags[],
    VIO_Point            flat[] )
{
    int              n;
    VIO_Real             *x_flat, *y_flat;

    ALLOC( x_flat, n_neighbours[point] );
    ALLOC( y_flat, n_neighbours[point] );

    for_less( n, 0, n_neighbours[point] )
        flat[n] = points[neighbours[point][n]];

    flatten_around_vertex( &points[point],
                           n_neighbours[point], flat,
                           (VIO_BOOL) interior_flags[point],
                           x_flat, y_flat );

    for_less( n, 0, n_neighbours[point] )
        fill_Point( flat[n], x_flat[n], y_flat[n], 0.0 );

    FREE( x_flat );
    FREE( y_flat );
}

static  void  create_coefficients_prediction_method(
    int              n_points,
    VIO_Point            points[],
    int              n_neighbours[],
    int              **neighbours,
    VIO_SCHAR     interior_flags[],
    int              n_fixed,
    int              fixed_indices[],
    VIO_Real             *fixed_pos[2],
    int              to_parameters[],
    int              to_fixed_index[],
    VIO_Real             *constant,
    ftype            *linear_terms[],
    ftype            *square_terms[],
    int              *n_cross_terms[],
    int              **cross_parms[],
    ftype            **cross_terms[] )
{
    int              node, p, dim, n_nodes_in, n_parameters;
    int              neigh, *indices, parm;
    VIO_Point            neigh_points[MAX_POINTS_PER_POLYGON];
    VIO_Real             flat[2][MAX_POINTS_PER_POLYGON];
    VIO_Real             *weights[2][2], cons[2], con;
    VIO_Real             *node_weights;
    VIO_BOOL          found, ignoring;
    VIO_progress_struct  progress;

    n_parameters = 2 * (n_points - n_fixed);

    INITIALIZE_LSQ( n_parameters, constant, linear_terms, square_terms,
                    n_cross_terms, cross_parms, cross_terms );

    ALLOC( weights[0][0], MAX_POINTS_PER_POLYGON );
    ALLOC( weights[0][1], MAX_POINTS_PER_POLYGON );
    ALLOC( weights[1][0], MAX_POINTS_PER_POLYGON );
    ALLOC( weights[1][1], MAX_POINTS_PER_POLYGON );

    initialize_progress_report( &progress, FALSE, n_points,
                                "Creating coefficients" );

    ALLOC( node_weights, n_parameters );
    ALLOC( indices, n_parameters );

    for_less( node, 0, n_points )
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
            neigh_points[p] = points[neighbours[node][p]];

        flatten_around_vertex( &points[node],
                               n_neighbours[node], neigh_points,
                               (VIO_BOOL) interior_flags[node],
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
                indices[n_nodes_in] = VIO_IJ(to_parameters[node],dim,2);
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
                    indices[n_nodes_in] = VIO_IJ(to_parameters[neigh],0,2);
                    node_weights[n_nodes_in] = -weights[dim][0][p];
                    ++n_nodes_in;

                    indices[n_nodes_in] = VIO_IJ(to_parameters[neigh],1,2);
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

static  void  create_coefficients(
    int              n_points,
    VIO_Point            points[],
    int              n_neighbours[],
    int              **neighbours,
    VIO_SCHAR     interior_flags[],
    int              n_fixed,
    int              fixed_indices[],
    VIO_Real             *fixed_pos[2],
    int              to_parameters[],
    int              to_fixed_index[],
    VIO_Real             *constant,
    ftype            *linear_terms[],
    ftype            *square_terms[],
    int              *n_cross_terms[],
    int              **cross_parms[],
    ftype            **cross_terms[] )
{
    int              dim, n_nodes_in, n_parameters, n, nn, point;
    int              max_neighbours, next_n, offset, tmp;
    int              indices[6], nodes[3], dim2, n_to_do;
    VIO_Real             con, node_weights[6], x, y;
    VIO_Real             weights[2][3][2], len;
    VIO_Point            *flat, flat_point[3], tmp_point;
    VIO_Vector           v12, v13, v12_rotated;
    VIO_BOOL          found;
    VIO_progress_struct  progress;

    n_parameters = 2 * (n_points - n_fixed);

    INITIALIZE_LSQ( n_parameters, constant, linear_terms, square_terms,
                    n_cross_terms, cross_parms, cross_terms );

    max_neighbours = 0;
    for_less( point, 0, n_points )
        max_neighbours = MAX( max_neighbours, n_neighbours[point] );

    ALLOC( flat, max_neighbours );

    initialize_progress_report( &progress, FALSE, n_points,
                                "Creating coefficients" );

    for_less( point, 0, n_points )
    {
        found = (to_parameters[point] >= 0);

        for_less( n, 0, n_neighbours[point] )
        {
            if( to_parameters[neighbours[point][n]] >= 0 )
            {
                found = TRUE;
                break;
            }
        }

        if( !found || n_neighbours[point] < 2 )
            continue;

        flatten_around_point( points, point, n_neighbours, neighbours,
                              interior_flags, flat );

        if( interior_flags[point] )
            n_to_do = n_neighbours[point];
        else
            n_to_do = n_neighbours[point] - 1;

        for_less( nn, 0, n_to_do )
        {
            next_n = (nn+1) % n_neighbours[point];
            nodes[0] = point;
            nodes[1] = neighbours[point][nn];
            nodes[2] = neighbours[point][next_n];
            fill_Point( flat_point[0], 0.0, 0.0, 0.0 );
            flat_point[1] = flat[nn];
            flat_point[2] = flat[next_n];

            for_less( offset, 0, 3 )
            {
            tmp = nodes[0];
            nodes[0] = nodes[1];
            nodes[1] = nodes[2];
            nodes[2] = tmp;

            tmp_point = flat_point[0];
            flat_point[0] = flat_point[1];
            flat_point[1] = flat_point[2];
            flat_point[2] = tmp_point;

            SUB_POINTS( v12, flat_point[1], flat_point[0] );
            SUB_POINTS( v13, flat_point[2], flat_point[0] );

            len = DOT_VECTORS( v12, v12 );

            if( len == 0.0 )
                continue;

            x = DOT_VECTORS( v12, v13 ) / len;

            fill_Vector( v12_rotated, -Vector_y(v12), Vector_x(v12), 0.0 );
            y = DOT_VECTORS( v13, v12_rotated ) / len;

            weights[0][0][0] = -1.0 + x;
            weights[0][0][1] = -y;
            weights[0][1][0] = -x;
            weights[0][1][1] = y;
            weights[0][2][0] = 1.0;
            weights[0][2][1] = 0.0;

            weights[1][0][0] = y;
            weights[1][0][1] = -1.0 + x;
            weights[1][1][0] = -y;
            weights[1][1][1] = -x;
            weights[1][2][0] = 0.0;
            weights[1][2][1] = 1.0;

#define DEBUG
#ifdef DEBUG
{
    VIO_Real  sum;

    for_less( dim, 0, 2 )
    {
        sum = 0.0;

        for_less( n, 0, 3 )
        for_less( dim2, 0, 2 )
        {
            sum += RPoint_coord( flat_point[n], dim2 ) * weights[dim][n][dim2];
        }

        if( sum > 1.0e-3 || sum < -1.0e-3 )
        {
            print( "%d %d: %g\n", point, nn, sum );
        }
    }
}
#endif

            if( y < 0.0 )
                print_error( "Y = %g\n", y );

            for_less( dim, 0, 2 )
            {
                con = 0.0;
                n_nodes_in = 0;
                for_less( n, 0, 3 )
                {
                    for_less( dim2, 0, 2 )
                    {
                        if( weights[dim][n][dim2] != 0.0 )
                        {
                            if( to_parameters[nodes[n]] >= 0 )
                            {
                                indices[n_nodes_in] =
                                           VIO_IJ(to_parameters[nodes[n]],dim2,2);
                                node_weights[n_nodes_in] = weights[dim][n][dim2];
                                ++n_nodes_in;
                            }
                            else
                                con += weights[dim][n][dim2] *
                                   fixed_pos[dim2][to_fixed_index[nodes[n]]];
                        }
                    }
                }

                ADD_TO_LSQ( n_parameters, constant, *linear_terms, *square_terms,
                            *n_cross_terms, *cross_parms, *cross_terms,
                            n_nodes_in, indices, node_weights, con, 5 );
            }
            }
        }

        update_progress_report( &progress, point + 1 );
    }

    terminate_progress_report( &progress );

    FREE( flat );

    REALLOC_LSQ( n_parameters, *n_cross_terms, *cross_parms, *cross_terms );
}

static  int  choose_static_polygon(
    int              n_points,
    int              n_neighbours[],
    int              *neighbours[],
    VIO_SCHAR     interior_flags[] )
{
    int                n_done, which, point, n;
    VIO_SCHAR       *used_flags;
    QUEUE_STRUCT(int)  queue;

    INITIALIZE_QUEUE( queue );

    n_done = 0;
    ALLOC( used_flags, n_points );
    for_less( point, 0, n_points )
    {
        used_flags[point] = (VIO_SCHAR) (!interior_flags[point]);
        if( used_flags[point] )
        {
            INSERT_IN_QUEUE( queue, point );
            ++n_done;
        }
    }

    while( n_done < n_points-1 )
    {
        REMOVE_FROM_QUEUE( queue, point );

        for_less( n, 0, n_neighbours[point] )
        {
            if( n_done < n_points-1 && !used_flags[neighbours[point][n]] )
            {
                INSERT_IN_QUEUE( queue, neighbours[point][n] );
                used_flags[neighbours[point][n]] = TRUE;
                ++n_done;
            }
        }
    }

    DELETE_QUEUE( queue );

    if( n_done < n_points-1 )
    {
        print( "Could not find interior point for start\n" );
        which = 0;
    }
    else
    {
        for_less( which, 0, n_points )
            if( !used_flags[which] )
                break;

        if( which >= n_points )
            which = n_points;

        print( "Using point %d for start\n", which );
    }

    FREE( used_flags );

    return( which );
}

static  void  flatten_polygons(
    VIO_BOOL          use_prediction_method,
    int              n_points,
    VIO_Point            points[],
    int              n_neighbours[],
    int              *neighbours[],
    VIO_SCHAR     interior_flags[],
    VIO_Point            init_points[],
    int              n_fixed,
    int              fixed_indices[],
    int              n_iters )
{
    int              i, p, point, which, n;
    VIO_Real             *fixed_pos[2];
    VIO_Real             constant;
    int              *n_cross_terms, **cross_parms, w_offset;
    ftype            *linear_terms, *square_terms, **cross_terms;
    VIO_Real             *parameters;
    int              *to_parameters, *to_fixed_index, ind;
    VIO_Vector           x_dir, y_dir, offset;
    VIO_Point            *flat;
    polygons_struct  tmp_polygons;
    VIO_BOOL          alloced_fixed;

    if( n_fixed <= 0 || init_points == NULL )
    {
        if( getenv( "FLATTEN_OFFSET" ) != NULL &&
            sscanf( getenv("FLATTEN_OFFSET"), "%d", &w_offset ) == 1 )
        {
            for_less( i, 0, n_points )
            {
                which = (i + w_offset) % n_points;
                if( n_neighbours[which] > 1 )
                    break;
            }
        }
        else
            which = choose_static_polygon( n_points, n_neighbours, neighbours,
                                           interior_flags );

        ALLOC( flat, n_neighbours[which] );

        flatten_around_point( points, which, n_neighbours, neighbours,
                              interior_flags, flat );

        n_fixed = n_neighbours[which]+1;

        if( getenv( "TRIANGLE" ) != NULL )
            n_fixed = 2;

        ALLOC( fixed_indices, n_fixed );
        ALLOC( fixed_pos[0], n_fixed );
        ALLOC( fixed_pos[1], n_fixed );

        fixed_indices[0] = which;
        for_less( n, 0, n_fixed-1 )
            fixed_indices[n+1] = neighbours[which][n];

        fixed_pos[0][0] = 0.0;
        fixed_pos[1][0] = 0.0;

        if( getenv( "TRIANGLE" ) != NULL )
        {
            fixed_pos[0][1] = distance_between_points(
                                                &points[fixed_indices[1]],
                                                &points[fixed_indices[0]] );
            fixed_pos[1][1] = 0.0;
        }
        else
        {
            for_less( n, 0, n_fixed-1 )
            {
                fixed_pos[0][1+n] = RPoint_x(flat[n]);
                fixed_pos[1][1+n] = RPoint_y(flat[n]);
            }
        }

        FREE( flat );

        alloced_fixed = TRUE;
    }
    else
    {
        alloced_fixed = FALSE;
        ALLOC( fixed_pos[0], n_fixed );
        ALLOC( fixed_pos[1], n_fixed );

        for_less( i, 0, n_fixed )
        {
            fixed_pos[0][i] = RPoint_x(init_points[fixed_indices[i]]);
            fixed_pos[1][i] = RPoint_y(init_points[fixed_indices[i]]);
        }
    }

    ALLOC( to_parameters, n_points );
    ALLOC( to_fixed_index, n_points );
    ind = 0;
    for_less( point, 0, n_points )
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

    if( use_prediction_method )
    {
        create_coefficients_prediction_method( n_points, points,
                             n_neighbours, neighbours,
                             interior_flags, n_fixed, fixed_indices, fixed_pos,
                             to_parameters, to_fixed_index,
                             &constant, &linear_terms, &square_terms,
                             &n_cross_terms, &cross_parms, &cross_terms );
    }
    else
    {
        create_coefficients( n_points, points, n_neighbours, neighbours,
                             interior_flags, n_fixed, fixed_indices, fixed_pos,
                             to_parameters, to_fixed_index,
                             &constant, &linear_terms, &square_terms,
                             &n_cross_terms, &cross_parms, &cross_terms );
    }

    ALLOC( parameters, 2 * (n_points - n_fixed) );

    if( init_points == NULL )
    {
        SUB_POINTS( x_dir, points[neighbours[which][0]], points[which] );
        NORMALIZE_VECTOR( x_dir, x_dir );
        fill_Point( y_dir, -RVector_y(x_dir), RVector_x(x_dir), 0.0 );
    }

    tmp_polygons.n_points = n_points;
    delete_polygon_point_neighbours( &tmp_polygons, n_neighbours, neighbours,
                                     interior_flags, NULL );

    for_less( point, 0, n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            if( init_points == NULL )
            {
if( getenv( "ZERO" ) != NULL )
{
    parameters[2*to_parameters[point]] = 0.0f;
    parameters[2*to_parameters[point]+1] = 0.0f;
}
else
{
                SUB_POINTS( offset, points[point], points[which] );
                parameters[2*to_parameters[point]] =
                               DOT_VECTORS( offset, x_dir );
                parameters[2*to_parameters[point]+1] =
                               DOT_VECTORS( offset, y_dir );
}
            }
            else
            {
                parameters[2*to_parameters[point]] =
                                      RPoint_x(init_points[point]);
                parameters[2*to_parameters[point]+1] =
                                      RPoint_y(init_points[point]);
            }
        }
    }

    (void) MINIMIZE_LSQ( 2 * (n_points - n_fixed),
                         constant, linear_terms, square_terms,
                         n_cross_terms, cross_parms, cross_terms,
                         -1.0, n_iters, parameters );

    FREE( linear_terms );
    FREE( square_terms );
    for_less( p, 0, 2 * (n_points - n_fixed) )
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

    for_less( point, 0, n_points )
    {
        if( to_parameters[point] >= 0 )
        {
            fill_Point( points[point], parameters[2*to_parameters[point]],
                        parameters[2*to_parameters[point]+1], 0.0 );
        }
        else
        {
            fill_Point( points[point], fixed_pos[0][to_fixed_index[point]],
                        fixed_pos[1][to_fixed_index[point]], 0.0 );
        }
    }

    FREE( parameters );

    FREE( to_parameters );
    FREE( to_fixed_index );

    if( alloced_fixed )
    {
        FREE( fixed_indices );
        FREE( fixed_pos[0] );
        FREE( fixed_pos[1] );
    }
}
