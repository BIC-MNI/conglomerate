#include  <internal_volume_io.h>
#include  <bicpl.h>
#include  <deform.h>

private  void  deform_line(
    int                n_points,
    Point              points[],
    Point              model_points[],
    Real               model_weight,
    Volume             volume,
    Real               threshold,
    char               normal_direction,
    Real               max_outward,
    Real               max_inward,
    int                n_iters,
    int                n_iters_recompute );

int  main(
    int    argc,
    char   *argv[] )
{
    STRING               input_filename, output_filename, model_filename;
    STRING               surface_direction, volume_filename;
    int                  n_objects, n_m_objects;
    int                  n_iters, n_iters_recompute;
    File_formats         format;
    object_struct        **object_list, **m_object_list;
    lines_struct         *lines, *model_lines;
    Volume               volume;
    Real                 threshold, model_weight;
    Real                 max_outward, max_inward;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &model_filename ) ||
        !get_real_argument( 0.0, &model_weight ) ||
        !get_string_argument( NULL, &volume_filename ) ||
        !get_real_argument( 0.0, &threshold ) ||
        !get_string_argument( NULL, &surface_direction ) ||
        !get_real_argument( 0.0, &max_outward ) ||
        !get_real_argument( 0.0, &max_inward ) )
    {
        print_error( "Usage: %s  input.obj output.obj etc. [n_iters] [n_between]\n",
                     argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 100, &n_iters );
    (void) get_int_argument( 1, &n_iters_recompute );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK || n_objects != 1 ||
        get_object_type(object_list[0]) != LINES )
        return( 1 );

    lines = get_lines_ptr( object_list[0] );

    if( input_graphics_file( model_filename, &format, &n_m_objects,
                             &m_object_list ) != OK || n_m_objects != 1 ||
        get_object_type(m_object_list[0]) != LINES )
        return( 1 );

    model_lines = get_lines_ptr( m_object_list[0] );

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    deform_line( lines->n_points, lines->points, model_lines->points,
                 model_weight, volume, threshold,
                 surface_direction[0], max_outward, max_inward,
                 n_iters, n_iters_recompute );

    (void) output_graphics_file( output_filename, format, 1, object_list );

    return( 0 );
}

private  void  create_model_coefficients(
    int              n_nodes,
    Point            model_points[],
    int              n_parms_involved[],
    int              *parm_list[],
    Real             constants[],
    Real             *node_weights[] )
{
    int              node, eq, dim;
    int              prev, next;
    Real             xs[2], ys[2];
    Real             *weights[2][2], cons[2];
    BOOLEAN          ignoring;

    ALLOC( weights[0][0], 2 );
    ALLOC( weights[0][1], 2 );
    ALLOC( weights[1][0], 2 );
    ALLOC( weights[1][1], 2 );

    eq = 0;
    for_less( node, 0, n_nodes )
    {
        prev = (node - 1 + n_nodes) % n_nodes;
        next = (node + 1 + n_nodes) % n_nodes;

        xs[0] = RPoint_x(model_points[prev]);
        ys[0] = RPoint_y(model_points[prev]);
        xs[1] = RPoint_x(model_points[next]);
        ys[1] = RPoint_y(model_points[next]);

        ignoring = FALSE;

        if( !get_prediction_weights_2d( RPoint_x(model_points[node]),
                                        RPoint_y(model_points[node]),
                                        2, xs, ys,
                                        weights[0], &cons[0],
                                        weights[1], &cons[1] ) )
        {
            print_error( "Error in interpolation weights, ignoring..\n" );
            ignoring = TRUE;
        }

#ifdef DEBUG
        print( "%d: %g %g %g %g %g    %g %g %g %g %g\n",
                node, cons[0], weights[0][0][0], weights[0][0][1],
                               weights[0][1][0], weights[0][1][1],
                      cons[1], weights[1][0][0], weights[1][0][1],
                               weights[1][1][0], weights[1][1][1] );
#endif

        if( ignoring )
        {
            n_parms_involved[eq] = 0;
            constants[eq] = 0.0;
            ++eq;
            n_parms_involved[eq] = 0;
            constants[eq] = 0.0;
            ++eq;
            continue;
        }

        for_less( dim, 0, 2 )
        {
            n_parms_involved[eq] = 5;
            ALLOC( parm_list[eq], n_parms_involved[eq] );
            ALLOC( node_weights[eq], n_parms_involved[eq] );

            parm_list[eq][0] = 2 * node + dim;
            node_weights[eq][0] = 1.0;
            constants[eq] = -cons[dim];

            parm_list[eq][1] = 2 * prev + 0;
            node_weights[eq][1] = -weights[dim][0][0];
            parm_list[eq][2] = 2 * prev + 1;
            node_weights[eq][2] = -weights[dim][1][0];

            parm_list[eq][3] = 2 * next + 0;
            node_weights[eq][3] = -weights[dim][0][1];
            parm_list[eq][4] = 2 * next + 1;
            node_weights[eq][4] = -weights[dim][1][1];

            ++eq;
        }
    }

#ifdef DEBUG
#define DEBUG
#undef DEBUG
    for_less( eq, 0, 2 * n_nodes )
    {
        int  p;
        print( "%3d: %g : ", eq, constants[eq] );
        for_less( p, 0, n_parms_involved[eq] )
        {
            print( " %d:%g ", parm_list[eq][p], node_weights[eq][p] );
        }
        print( "\n" );
    }
#endif

    FREE( weights[0][0] );
    FREE( weights[0][1] );
    FREE( weights[1][0] );
    FREE( weights[1][1] );
}

private  void  create_image_coefficients(
    Volume                      volume,
    boundary_definition_struct  *boundary,
    Real                        max_outward,
    Real                        max_inward,
    Real                        z,
    int                         n_nodes,
    Real                        parameters[],
    int                         *parm_list[],
    Real                        constants[],
    Real                        *node_weights[] )
{
    int     eq, node, prev, next;
    Real    dist;
    Point   prev_point, next_point, origin, p;
    Vector  offset, normal;

    eq = 0;

    for_less( node, 0, n_nodes )
    {
        prev = (node - 1 + n_nodes) % n_nodes;
        next = (node + 1 + n_nodes) % n_nodes;

        fill_Point( origin, parameters[2*node+X], parameters[2*node+Y], z );
        fill_Point( prev_point, parameters[2*prev+X], parameters[2*prev+Y], z );
        fill_Point( next_point, parameters[2*next+X], parameters[2*next+Y], z );

        SUB_POINTS( offset, next_point, prev_point );
        fill_Vector( normal, RVector_y(offset), -RVector_x(offset), 0.0 );
        NORMALIZE_VECTOR( normal, normal );

        if( !find_boundary_in_direction( volume, NULL, NULL, NULL, NULL,
                                         0.0, &origin, &normal, &normal,
                                         max_outward, max_inward, 0,
                                         boundary, &dist ) )
        {
            dist = MAX( max_outward, max_inward );
            node_weights[eq][0] = 0.0;
            constants[eq] = dist * dist;
            ++eq;
            node_weights[eq][0] = 0.0;
            constants[eq] = dist * dist;
            ++eq;
            continue;
        }

        GET_POINT_ON_RAY( p, origin, normal, dist );

        node_weights[eq][0] = 1.0;
        constants[eq] = -RPoint_x(p);
        ++eq;

        node_weights[eq][0] = 1.0;
        constants[eq] = -RPoint_y(p);
        ++eq;
    }
}

private  void  deform_line(
    int                n_points,
    Point              points[],
    Point              model_points[],
    Real               model_weight,
    Volume             volume,
    Real               threshold,
    char               normal_direction,
    Real               max_outward,
    Real               max_inward,
    int                n_iters,
    int                n_iters_recompute )
{
    int              eq, point, n, iter;
    int              n_model_equations, n_image_equations;
    int              n_equations, *n_parms_involved, **parm_list;
    Real             *constants, **node_weights;
    Real             *parameters;
    boundary_definition_struct  boundary;

    set_boundary_definition( &boundary, threshold, threshold, -1.0, 90.0,
                             normal_direction, 1.0e-4 );

    n_model_equations = 2 * n_points;
    n_image_equations = 2 * n_points;
    n_equations = n_model_equations + n_image_equations;

    ALLOC( n_parms_involved, n_equations );
    ALLOC( constants, n_equations );
    ALLOC( node_weights, n_equations );
    ALLOC( parm_list, n_equations );

    create_model_coefficients( n_points, model_points, n_parms_involved,
                               parm_list, constants, node_weights );

    model_weight = sqrt( model_weight );
    for_less( eq, 0, n_model_equations )
    {
        constants[eq] *= model_weight;
        for_less( n, 0, n_parms_involved[eq] )
            node_weights[eq][n] *= model_weight;
    }

    for_less( point, 0, n_points )
    {
        n_parms_involved[n_model_equations+2*point+X] = 1;
        ALLOC( node_weights[n_model_equations+2*point+X], 1 );
        ALLOC( parm_list[n_model_equations+2*point+X], 1 );
        parm_list[n_model_equations+2*point+X][0] = 2 * point + X;

        n_parms_involved[n_model_equations+2*point+Y] = 1;
        ALLOC( node_weights[n_model_equations+2*point+Y], 1 );
        ALLOC( parm_list[n_model_equations+2*point+Y], 1 );
        parm_list[n_model_equations+2*point+Y][0] = 2 * point + Y;
    }

    ALLOC( parameters, 2 * n_points );

    for_less( point, 0, n_points )
    {
        parameters[2*point+X] = RPoint_x(points[point]);
        parameters[2*point+Y] = RPoint_y(points[point]);
    }

    iter = 0;
    while( iter < n_iters )
    {
        create_image_coefficients( volume, &boundary,
                                   max_outward, max_inward,
                                   RPoint_z(points[0]),
                                   n_points, parameters,
                                   &parm_list[n_model_equations],
                                   &constants[n_model_equations],
                                   &node_weights[n_model_equations] );
#ifdef DEBUG
#define DEBUG
#undef DEBUG
{
    int  eq, n;
    for_less( eq, 0, n_equations )
    {
        print( "%d: %g : ", eq, constants[eq] );
        for_less( n, 0, n_parms_involved[eq] )
        {
            print( " %d:%g", parm_list[eq][n], node_weights[eq][n] );
        }
        print( "\n" );
    }
}
#endif

        (void) minimize_lsq( 2 * n_points, n_equations,
                             n_parms_involved, parm_list, constants,
                             node_weights, -1.0, n_iters_recompute,
                             parameters );

        iter += n_iters_recompute;
        print( "########### %d:\n", iter );
    }

    for_less( point, 0, n_points )
    {
        Point_x(points[point]) = (Point_coord_type) parameters[2*point+X];
        Point_y(points[point]) = (Point_coord_type) parameters[2*point+Y];
    }

    FREE( parameters );

    for_less( eq, 0, n_equations )
    {
        FREE( parm_list[eq] );
        FREE( node_weights[eq] );
    }
    FREE( n_parms_involved );
    FREE( constants );
    FREE( node_weights );
    FREE( parm_list );
}
