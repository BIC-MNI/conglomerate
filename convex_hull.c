
#include  <internal_volume_io.h>
#include  <bicpl.h>

private  int  get_points_of_region(
    Volume  volume,
    Real    min_value,
    Real    max_value,
    Point   *points[] );

int  main(
    int   argc,
    char  *argv[] )
{
    char           *input_filename, *output_filename;
    int            sizes[N_DIMENSIONS];
    int            n_points;
    Real           min_value, max_value;
    Point          *points;
    Volume         volume;
    object_struct  *object;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        return( 1 );
    }

    (void) get_real_argument( 0.01, &min_value );
    (void) get_real_argument( 1.0e30, &max_value );
 
    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    n_points = get_points_of_region( volume, min_value, max_value, &points );

    object = create_object( LINES );

    {
        int            i;
        lines_struct   *lines;

        lines = get_lines_ptr( object );
        initialize_lines( lines, WHITE );

        lines->n_points = n_points;
        lines->points = points;

        lines->n_items = n_points;
        ALLOC( lines->end_indices, lines->n_items );
        for_less( i, 0, n_points )
            lines->end_indices[i] = i;

        ALLOC( lines->indices, n_points );

        for_less( i, 0, n_points )
            lines->indices[i] = i;
    }

    (void) output_graphics_file( output_filename, BINARY_FORMAT, 1, &object );


    return( 0 );
}

private  int  get_points_of_region(
    Volume  volume,
    Real    min_value,
    Real    max_value,
    Point   *points[] )
{
    int        x, y, z, sizes[N_DIMENSIONS], n_inside;
    int        dx, dy, dz, tx, ty, tz, n_points;
    Real       value, xw, yw, zw, voxel[N_DIMENSIONS];
    Point      point;

    get_volume_sizes( volume, sizes );

    n_points = 0;

    for_less( x, 0, sizes[X] + 1 )
    for_less( y, 0, sizes[Y] + 1 )
    for_less( z, 0, sizes[Z] + 1 )
    {
        n_inside = 0;

        for_less( dx, 0, 2 )
        for_less( dy, 0, 2 )
        for_less( dz, 0, 2 )
        {
            tx = x - dx;
            ty = y - dy;
            tz = z - dz;

            if( tx >= 0 && tx < sizes[X] &&
                ty >= 0 && ty < sizes[Y] &&
                tz >= 0 && tz < sizes[Z] )
            {
                value = get_volume_real_value( volume, tx, ty, tz, 0, 0 );

                if( min_value <= value && value <= max_value )
                    ++n_inside;
            }
        }

        if( n_inside == 1 )
        {
            voxel[X] = (Real) x - 0.5;
            voxel[Y] = (Real) y - 0.5;
            voxel[Z] = (Real) z - 0.5;
            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            fill_Point( point, xw, yw, zw );
            ADD_ELEMENT_TO_ARRAY( *points, n_points, point, DEFAULT_CHUNK_SIZE);
        }
    }

    return( n_points );
}
