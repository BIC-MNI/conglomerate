#include  <internal_volume_io.h>
#include  <bicpl.h>

private  Status  add_polygons(
    polygons_struct   *summed_polygons,
    Real              weight,
    polygons_struct   *polygons,
    Real              **colours,
    BOOLEAN           first_flag )
{
    int     i, c, n_colours;
    Point   scaled;

    if( first_flag )
    {
        copy_polygons( polygons, summed_polygons );
        for_less( i, 0, summed_polygons->n_points )
            fill_Point( summed_polygons->points[i], 0.0, 0.0, 0.0 );

        n_colours = get_n_colours( polygons->colour_flag,
                                   polygons->n_points,
                                   polygons->n_items );

        for_less( i, 0, n_colours )
            for_less( c, 0, 4 )
                colours[i][c] = 0.0;
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
            colours[i][0] += weight * get_Colour_r_0_1( polygons->colours[i] );
            colours[i][1] += weight * get_Colour_g_0_1( polygons->colours[i] );
            colours[i][2] += weight * get_Colour_b_0_1( polygons->colours[i] );
            colours[i][3] += weight * get_Colour_a_0_1( polygons->colours[i] );
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
    Status           status;
    STRING           filename, output_filename;
    Real             weight, new_weight;
    int              i, n_objects, n_polygons, n_colours;
    Real             scale_colours;
    Real             **colours;
    File_formats     format;
    object_struct    *out_object;
    object_struct    **object_list;
    polygons_struct  *polygons;
    BOOLEAN          done;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &output_filename ) )
    {
        print_error(
          "Usage: %s output.obj [weight|input1.obj] [weight|input2.obj] ...\n",
          argv[0] );
        return( 1 );
    }

    scale_colours = 1.0;
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
        else if( equal_strings( filename, "-scale_colours" ) )
        {
            if( !get_real_argument( 0.0, &scale_colours ) )
            {
                print( "Error in -scale_colours argument.\n" );
                return( 1 );
            }
        }
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
                polygons = get_polygons_ptr( object_list[0] );

                if( n_polygons == 0 )
                {
                    n_colours = get_n_colours( polygons->colour_flag,
                                               polygons->n_points,
                                               polygons->n_items );
                    ALLOC2D( colours, n_colours, 4 );
                }

                print( "%s: Weighted %g\n", filename, weight );
                if( add_polygons( get_polygons_ptr(out_object), weight,
                                  polygons,
                                  colours, n_polygons == 0) !=OK)
                {
                    print( "File %s.\n", filename );
                    return( 1 );
                }
                ++n_polygons;
            }

            delete_object_list( n_objects, object_list );
        }
    }

    {
        int   c;

        for_less( i, 0, n_colours )
        {
            for_less( c, 0, 4 )
            {
                colours[i][c] = 0.5 + scale_colours * (colours[i][c] - 0.5);
                if( colours[i][c] < 0.0 )
                    colours[i][c] = 0.0;
                else if( colours[i][c] > 1.0 )
                    colours[i][c] = 1.0;
            }

            get_polygons_ptr(out_object)->colours[i] = make_rgba_Colour_0_1(
                       colours[i][0], colours[i][1], colours[i][2],
                       colours[i][3] );
        }
    }

    if( n_colours > 0 )
        FREE2D( colours );

    compute_polygon_normals( get_polygons_ptr(out_object) );

    if( status == OK )
        status = output_graphics_file( output_filename, format,
                                       1, &out_object );

    if( status == OK )
        print( "Objects output.\n" );

    return( status != OK );
}
