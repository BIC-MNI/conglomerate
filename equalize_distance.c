#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  reparameterize(
    polygons_struct   *original,
    polygons_struct   *model,
    Point             new_points[],
    Real              movement_threshold,
    int               n_iters );

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               model_filename;
    STRING               input_filename, output_filename;
    File_formats         src_format, model_format;
    int                  n_src_objects, n_model_objects;
    Point                *new_points;
    object_struct        **model_objects, **src_objects;
    polygons_struct      *original, *model;
    Real                 movement_threshold;
    int                  n_iters;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &model_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s  input.obj  model.obj output.obj\n", argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 1, &n_iters );
    (void) get_real_argument( 0.0, &movement_threshold );

    if( input_graphics_file( input_filename, &src_format,
                             &n_src_objects, &src_objects ) != OK ||
        n_src_objects != 1 || get_object_type(src_objects[0]) != POLYGONS )
    {
        print_error( "Error in %s\n", input_filename );
        return( 1 );
    }

    if( input_graphics_file( model_filename, &model_format,
                             &n_model_objects, &model_objects ) != OK||
        n_model_objects != 1 || get_object_type(model_objects[0]) != POLYGONS )
    {
        print_error( "Error in %s\n", model_filename );
        return( 1 );
    }

    original = get_polygons_ptr( src_objects[0] );
    model = get_polygons_ptr( model_objects[0] );

    if( !objects_are_same_topology( original->n_points,
                                    original->n_items,
                                    original->end_indices,
                                    original->indices,
                                    model->n_points,
                                    model->n_items,
                                    model->end_indices,
                                    model->indices ) )
    {
        print_error( "Mismatched topology.\n" );
        return( 1 );
    }

    ALLOC( new_points, original->n_points );

    reparameterize( original, model, new_points, movement_threshold, n_iters );

    FREE( original->points );
    original->points = new_points;

    if( output_graphics_file( output_filename, src_format, n_src_objects,
                              src_objects ) != OK )
        return( 1 );

    return( 0 );
}

private  void  reparameterize(
    polygons_struct   *original,
    polygons_struct   *model,
    Point             new_points[],
    Real              movement_threshold,
    int               n_iters )
{
    int   total_neighbours, ind, p1, n, *n_neighbours, **neighbours;
    int   *which_triangle, size, vertex, poly, iter;
    Real  *model_lengths, movement;

    create_polygon_point_neighbours( original, FALSE, &n_neighbours,
                                     &neighbours, NULL, NULL );

    check_polygons_neighbours_computed( original );

    total_neighbours = 0;
    for_less( p1, 0, original->n_points )
        total_neighbours += n_neighbours[p1];

    ALLOC( model_lengths, total_neighbours );
    ind = 0;

    for_less( p1, 0, original->n_points )
    {
        for_less( n, 0, n_neighbours[p1] )
        {
            model_lengths[ind] = distance_between_points( &model->points[p1],
                                   &model->points[neighbours[p1][n]] );
            ++ind;
        }
    }

    ALLOC( which_triangle, original->n_points );
    for_less( p1, 0, original->n_points )
        which_triangle[p1] = -1;

    for_less( poly, 0, original->n_items )
    {
        size = GET_OBJECT_SIZE( *original, poly );
        for_less( vertex, 0, size )
        {
            p1 = original->indices[
                   POINT_INDEX(original->end_indices,poly,vertex)];
            if( which_triangle[p1] < 0 )
                which_triangle[p1] = poly;
        }
    }

    iter = 0;
    movement = -1.0;
    while( iter < n_iters && movement > movement_threshold )
    {
/*
        movement = perturb_vertices( original, model_lengths,
                                     new_points, which_triangle );
*/
        ++iter;
        print( "%3d: %g\n", iter, movement );
    }

    delete_polygon_point_neighbours( original, n_neighbours,
                                     neighbours, NULL, NULL );

    FREE( model_lengths );
}
