#include  <bicpl.h>
#include  <volume_io/internal_volume_io.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Volume               volume, image;
    STRING               input_filename, output_filename, axis_name;
    int                  degree, x_size, y_size, axis_index, a1, a2;
    int                  x, y, v[N_DIMENSIONS];
    Real                 axis_pos, voxel[N_DIMENSIONS], slice_pos, grad;
    float                **grad_mag;
    Real                 dx, dy, dz, value, max_value, max_value2;
    Real                 world[N_DIMENSIONS];
    Real                 separations[N_DIMENSIONS];
    Real                 separations_2d[N_DIMENSIONS];
    Real                 avg_value, max;
    int                  sizes[N_DIMENSIONS];
    int                  sizes_2d[N_DIMENSIONS];
    int                  dir, n_dirs, *x_dirs, *y_dirs, tx, ty;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s  volume.mnc  input.obj\n", argv[0]);
        return( 1 );
    }

    (void) get_int_argument( 2, &degree );
    (void) get_int_argument( 200, &x_size );
    (void) get_int_argument( 200, &y_size );
    (void) get_string_argument( "x", &axis_name );
    (void) get_real_argument( 0.0, &axis_pos );

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    axis_index = (int) (axis_name[0] - 'x');
    a1 = (axis_index + 1) % N_DIMENSIONS;
    a2 = (axis_index + 2) % N_DIMENSIONS;

    get_volume_sizes( volume, sizes );
    get_volume_separations( volume, separations );

    sizes_2d[a1] = x_size;
    sizes_2d[a2] = y_size;
    sizes_2d[axis_index] = 2;

    separations_2d[a1] = separations[a1] *
                         (Real) sizes[a1] / (Real) sizes_2d[a1];
    separations_2d[a2] = separations[a2] *
                         (Real) sizes[a2] / (Real) sizes_2d[a2];
    separations_2d[axis_index] = 1.0;

    world[a1] = 0.0;
    world[a2] = 0.0;
    world[axis_index] = axis_pos;

    convert_world_to_voxel( volume, world[X], world[Y], world[Z], voxel );

    slice_pos = voxel[axis_index];

    print( "Slice pos: %g\n", slice_pos );

    image = create_volume( 3, XYZ_dimension_names, NC_BYTE, FALSE, 0.0, 0.0 );
    set_volume_sizes( image, sizes_2d );
    alloc_volume_data( image );
    set_volume_real_range( image, 0.0, 100.0 );

    set_volume_separations( image, separations_2d );

    voxel[a1] = -0.5;
    voxel[a2] = -0.5;
    voxel[axis_index] = slice_pos;
    convert_voxel_to_world( volume, voxel, &world[X], &world[Y], &world[Z] );

    voxel[axis_index] = 0.0;

    set_volume_translation( image, voxel, world );

    ALLOC2D( grad_mag, x_size, y_size );

    max = 0.0;
    for_less( x, 0, x_size )
    for_less( y, 0, y_size )
    {
        voxel[a1] = (Real) x;
        voxel[a2] = (Real) y;
        voxel[axis_index] = 0.0;
        convert_voxel_to_world( image, voxel, &world[X], &world[Y], &world[Z] );
        convert_world_to_voxel( volume, world[X], world[Y], world[Z], voxel );

/*
        print( "%d %d: %g %g %g  %g %g %g\n", x, y, voxel[X], voxel[Y], voxel[Z], world[X], world[Y], world[Z] );
*/

        evaluate_volume_in_world( volume,
                                  world[X],
                                  world[Y],
                                  world[Z],
                                  degree, FALSE, 0.0, &value,
                                  &dx, &dy, &dz,
                                  NULL, NULL, NULL, NULL, NULL, NULL );

        grad_mag[x][y] = (float) (dx * dx + dy * dy + dz * dz);
        max = MAX( max, (Real) grad_mag[x][y] );
    }

    n_dirs = get_neighbour_directions( EIGHT_NEIGHBOURS, &x_dirs, &y_dirs );

    avg_value = 0.0;
    for_less( x, 0, x_size )
    for_less( y, 0, y_size )
    {
        grad = (Real) grad_mag[x][y];

        max_value = 0.0;
        for_less( dir, 0, n_dirs )
        {
            tx = x + x_dirs[dir];
            ty = y + y_dirs[dir];
            if( tx >= 0 && tx < x_size && ty >= 0 && ty < y_size &&
                (Real) grad_mag[tx][ty] > max_value )
            {
                max_value = (Real) grad_mag[tx][ty];
            }
        }

        max_value2 = 0.0;
        for_less( dir, 0, n_dirs )
        {
            tx = x + x_dirs[dir];
            ty = y + y_dirs[dir];
            if( tx >= 0 && tx < x_size && ty >= 0 && ty < y_size &&
                (Real) grad_mag[tx][ty] < max_value &&
                (Real) grad_mag[tx][ty] > max_value2 )
            {
                max_value2 = (Real) grad_mag[tx][ty];
            }
        }

        if( grad >= max_value2 )
            value = 100.0;
        else
            value = 0.0;

        v[a1] = x;
        v[a2] = y;
        v[axis_index] = 0;
        set_volume_real_value( image, v[X], v[Y], v[Z], 0, 0, value );
        v[axis_index] = 1;
        set_volume_real_value( image, v[X], v[Y], v[Z], 0, 0, value );

        avg_value += value;
    }

    avg_value /= (Real) x_size * (Real) y_size;

    print( "Avg: %g\n", avg_value );

    FREE2D( grad_mag );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED,
                                   FALSE, 0.0, 0.0, image, input_filename,
                                   "make_grad_mag\n", NULL );

    return( 0 );
}
