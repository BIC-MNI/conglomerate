#include  <bicpl.h>

private  Status  add_polygons(
    polygons_struct   *summed_polygons,
    Real              weight,
    polygons_struct   *polygons,
    BOOLEAN           first_flag )
{
    int     i, n_colours;
    Point   scaled;
    Colour  scaled_colour;

    if( first_flag )
    {
        copy_polygons( polygons, summed_polygons );
        for_less( i, 0, summed_polygons->n_points )
            fill_Point( summed_polygons->points[i], 0.0, 0.0, 0.0 );

        n_colours = get_n_colours( polygons->colour_flag,
                                   polygons->n_points,
                                   polygons->n_items );

        for_less( i, 0, n_colours )
            summed_polygons->colours[i] = BLACK;
    }
    else if( !polygons_are_same_topology( summed_polygons, polygons ) )
    {
        print( "Polygons different topology\n" );
        return( ERROR );
    }

    for_less( i, 0, summed_polygons->n_points )
    {
        SCALE_POINT( scaled, polygons->points[i], weight );
        ADD_POINTS( summed_polygons->points[i], summed_polygons->points[i],
                    scaled );
    }

    if( summed_polygons->colour_flag == polygons->colour_flag )
    {
        n_colours = get_n_colours( polygons->colour_flag,
                                   polygons->n_points,
                                   polygons->n_items );

        for_less( i, 0, n_colours )
        {
            scaled_colour = SCALE_COLOUR( polygons->colours[i], weight );
            ADD_COLOURS( summed_polygons->colours[i],
                         summed_polygons->colours[i], scaled_colour );
        }
    }
    else
        print( "Warning:  Polygons colour flags do not match. Colours will not be added properly\n" );

    return( OK );
}

int  main(
    int    argc,
    char   *argv[] )
{
    Status         status;
    char           *filename, *output_filename;
    Real           weight, new_weight;
    int            n_objects, n_polygons;
    File_formats   format;
    object_struct  *out_object;
    object_struct  **object_list;
    BOOLEAN        done;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &output_filename ) )
    {
        print( "Need output_file  argument.\n" );
        return( 1 );
    }

    done = FALSE;
    n_polygons = 0;
    weight = 1.0;

    out_object = create_object( POLYGONS );

    while( !done )
    {
        if( get_real_argument( 0.0, &new_weight ) )
            weight = new_weight;
        else if( !get_string_argument( "", &filename ) )
            done = TRUE;
        else if( input_graphics_file( filename, &format, &n_objects,
                                      &object_list ) != OK )
        {
            print( "Couldn't read %s.\n", filename );
            return( 1 );
        }
        else
        {
            if( n_objects >= 1 && object_list[0]->object_type == POLYGONS )
            {
                print( "%s: Weighted %g\n", filename, weight );
                if( add_polygons( get_polygons_ptr(out_object), weight,
                                  get_polygons_ptr(object_list[0]),
                                  n_polygons == 0) != OK )
                {
                    print( "File %s.\n", filename );
                    return( 1 );
                }
                ++n_polygons;
            }

            delete_object_list( n_objects, object_list );
        }
    }

    compute_polygon_normals( get_polygons_ptr(out_object) );

    if( status == OK )
        status = output_graphics_file( output_filename, format,
                                       1, &out_object );

    if( status == OK )
        print( "Objects output.\n" );

    return( status != OK );
}
