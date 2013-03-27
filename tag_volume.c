#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               volume_filename, input_tags;
    VIO_Volume               volume;
    VIO_Real                 y_min, y_max, y_step;
    int                  i;
    int                  min_step, max_step, n_steps, *counts;
    int                  n_volumes, n_tag_points, step;
    VIO_Real                 **tags_volume1, **tags_volume2;
    VIO_Real                 voxel_volume;
    VIO_Real                 separations[VIO_N_DIMENSIONS];
    volume_input_struct  volume_input;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &input_tags ) ||
        !get_real_argument( 0.0, &y_min ) ||
        !get_real_argument( 0.0, &y_max ) ||
        !get_real_argument( 0.0, &y_step ) )
    {
        print( "%s  example.mnc  input.tags y_min y_max y_step\n", argv[0] );
        return( 1 );
    }

    if( start_volume_input( volume_filename, 3, XYZ_dimension_names,
                            NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                            TRUE, &volume, (minc_input_options *) NULL,
                            &volume_input ) != VIO_OK )
    {
        return( 1 );
    }

    if( input_tag_file( input_tags, &n_volumes, &n_tag_points,

                        &tags_volume1, &tags_volume2,
                        NULL, NULL, NULL, NULL ) != VIO_OK )
        return( 1 );

    min_step = VIO_FLOOR( y_min / y_step );
    max_step = VIO_FLOOR( y_max / y_step );
    if( (VIO_Real) max_step * y_step == y_max )
        --max_step;

    n_steps = max_step - min_step + 1;

    ALLOC( counts, n_steps );
    for_less( step, 0, n_steps )
        counts[step] = 0;

    get_volume_separations( volume, separations );

    for_less( i, 0, n_tag_points )
    {
        step = VIO_FLOOR( tags_volume1[i][VIO_Y] / y_step ) - min_step;
        if( step < 0 || step >= n_steps )
            print_error( "Error in file: %s  at %d'th point\n", input_tags, i );
        else
            ++counts[step];
    }

    voxel_volume = separations[VIO_X] * separations[VIO_Y] * separations[VIO_Z];

    for_less( step, 0, n_steps )
        print( " %g", voxel_volume * (VIO_Real) counts[step] );
    print( "\n" );

    return( 0 );
}
