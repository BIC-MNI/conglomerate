#include  <volume_io.h>
#include  <bicpl.h>

private  Real  compute_cross_correlation(
    Volume     volume1,
    Volume     volume2,
    Transform  *transform,
    int        nx,
    int        ny,
    int        nz );

private  void  usage(
    STRING  executable )
{
    STRING  usage_str = "\n\
Usage: %s input1.mnc input2.mnc  output.obj xs ys zs  ns  xr yr zr  xt yt zt\n\
\n\
     Evaluates the cross correlation of the two volumes, with the set of\n\
     offsets.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING         input_filename1, input_filename2, output_filename;
    Volume         volume1, volume2;
    Real           x_sampling, y_sampling, z_sampling, x_rot, y_rot, z_rot;
    Real           x_trans, y_trans, z_trans, xr, yr, zr, xt, yt, zt, alpha;
    Real           value, separations[3], min_value, max_value;
    Transform      transform, x_rot_trans, y_rot_trans, z_rot_trans;
    Transform      translation;
    int            n_samples, sample, sizes[3], nx, ny, nz;
    object_struct  *objects[2];
    lines_struct   *lines;
    progress_struct  progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename1 ) ||
        !get_string_argument( NULL, &input_filename2 ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &x_sampling ) ||
        !get_real_argument( 0.0, &y_sampling ) ||
        !get_real_argument( 0.0, &z_sampling ) ||
        !get_int_argument( 0, &n_samples ) ||
        !get_real_argument( 0.0, &x_rot ) ||
        !get_real_argument( 0.0, &y_rot ) ||
        !get_real_argument( 0.0, &z_rot ) ||
        !get_real_argument( 0.0, &x_trans ) ||
        !get_real_argument( 0.0, &y_trans ) ||
        !get_real_argument( 0.0, &z_trans ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume( input_filename1, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume1, NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume1, sizes );
    get_volume_separations( volume1, separations );

    nx = ROUND( (Real) sizes[X] * FABS(separations[X]) / x_sampling );
    ny = ROUND( (Real) sizes[Y] * FABS(separations[Y]) / y_sampling );
    nz = ROUND( (Real) sizes[Z] * FABS(separations[Z]) / z_sampling );

    if( equal_strings( input_filename1, input_filename2 ) )
    {
        volume2 = volume1;
    }
    else
    {
        if( input_volume( input_filename2, 3, File_order_dimension_names,
                          NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          TRUE, &volume2, NULL ) != OK )
            return( 1 );
    }

    objects[0] = create_object( LINES );
    lines = get_lines_ptr( objects[0] );

    initialize_lines_with_size( lines, GREEN, n_samples, FALSE );

    initialize_progress_report( &progress, FALSE, n_samples,
                                "Computing cross correlation" );

    min_value = 0.0;
    max_value = 0.0;

    for_less( sample, 0, n_samples )
    {
        alpha = 2.0 * (Real) sample / (Real) (n_samples-1) - 1.0;

        xr = alpha * x_rot;
        yr = alpha * y_rot;
        zr = alpha * z_rot;
        xt = alpha * x_trans;
        yt = alpha * y_trans;
        zt = alpha * z_trans;

        make_rotation_transform( DEG_TO_RAD * xr, X, &x_rot_trans );
        make_rotation_transform( DEG_TO_RAD * yr, Y, &y_rot_trans );
        make_rotation_transform( DEG_TO_RAD * zr, Z, &z_rot_trans );
        make_translation_transform( xt, yt, zt, &translation );

        concat_transforms( &transform, &x_rot_trans, &y_rot_trans );
        concat_transforms( &transform, &transform, &z_rot_trans );
        concat_transforms( &transform, &transform, &translation );

        value = compute_cross_correlation( volume1, volume2, &transform,
                                           nx, ny, nz );

        if( sample == 0 || value < min_value )
            min_value = value;
        if( sample == 0 || value > max_value )
            max_value = value;

        fill_Point( lines->points[sample], alpha, value, 0.0 );

        update_progress_report( &progress, sample+1 );
    }

    for_less( sample, 0, n_samples )
    {
        fill_Point( lines->points[sample],
                    Point_x(lines->points[sample]),
                    (Point_y(lines->points[sample]) - min_value) /
                    (max_value - min_value),
                    0.0 );
    }

    terminate_progress_report( &progress );

    objects[1] = create_object( LINES );
    lines = get_lines_ptr( objects[1] );
    initialize_lines_with_size( lines, RED, 2, FALSE );
    fill_Point( lines->points[0], 0.0, 0.0, 0.0 );
    fill_Point( lines->points[1], 0.0, 1.0, 0.0 );

    (void) output_graphics_file( output_filename, ASCII_FORMAT, 2, objects );

    delete_object( objects[0] );
    delete_object( objects[1] );

    if( volume1 != volume2 )
        delete_volume( volume2 );

    delete_volume( volume1 );

    return( 0 );
}

private  Real  compute_cross_correlation(
    Volume     volume1,
    Volume     volume2,
    Transform  *transform,
    int        nx,
    int        ny,
    int        nz )
{
    Real  voxel[MAX_DIMENSIONS];
    Real  voxel2[MAX_DIMENSIONS];
    Real  delta[MAX_DIMENSIONS];
    int   sizes[MAX_DIMENSIONS], grid[MAX_DIMENSIONS];
    int   i, j, k, dim;
    Real  value1, value2, corr, xw, yw, zw, diff;

    corr = 0.0;

    get_volume_sizes( volume1, sizes );

    voxel[0] = 0.0;
    voxel[1] = 0.0;
    voxel[2] = INTERPOLATE( ((Real) 0 + 0.5) / (Real) nz,
                                        -0.5, (Real) sizes[Z] - 0.5 );
    convert_voxel_to_world( volume1, voxel, &xw, &yw, &zw );
    transform_point( transform, xw, yw, zw, &xw, &yw, &zw );
    convert_world_to_voxel( volume2, xw, yw, zw, voxel2 );

    voxel[2] = INTERPOLATE( ((Real) 1 + 0.5) / (Real) nz,
                                        -0.5, (Real) sizes[Z] - 0.5 );
    convert_voxel_to_world( volume1, voxel, &xw, &yw, &zw );
    transform_point( transform, xw, yw, zw, &xw, &yw, &zw );
    convert_world_to_voxel( volume2, xw, yw, zw, delta );

    delta[0] -= voxel2[0];
    delta[1] -= voxel2[1];
    delta[2] -= voxel2[2];

    for_less( i, 0, nx )
    {
        voxel[X] = INTERPOLATE( ((Real) i + 0.5) / (Real) nx,
                                -0.5, (Real) sizes[X] - 0.5 );

        for_less( j, 0, ny )
        {
            voxel[Y] = INTERPOLATE( ((Real) j + 0.5) / (Real) ny,
                                    -0.5, (Real) sizes[Y] - 0.5 );

            for_less( k, 0, nz )
            {
                voxel[Z] = INTERPOLATE( ((Real) k + 0.5) / (Real) nz,
                                        -0.5, (Real) sizes[Z] - 0.5 );

                if( k == 0 )
                {
                    convert_voxel_to_world( volume1, voxel, &xw, &yw, &zw );
                    transform_point( transform, xw, yw, zw, &xw, &yw, &zw );
                    convert_world_to_voxel( volume2, xw, yw, zw, voxel2 );
                }
                else
                {
                    voxel2[0] += delta[0];
                    voxel2[1] += delta[1];
                    voxel2[2] += delta[2];
                }

                (void) evaluate_volume( volume1, voxel, NULL, 0, FALSE, 0.0,
                                        &value1, NULL, NULL );
                (void) evaluate_volume( volume2, voxel2, NULL, 0, FALSE, 0.0,
                                        &value2, NULL, NULL );

                diff = value1 - value2;
                corr += diff * diff;
            }
        }
    }

    corr /= (Real) (nx * ny * nz);

    return( corr );
}
