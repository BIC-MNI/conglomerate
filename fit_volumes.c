#include  <bicpl.h>
#include  <internal_volume_io.h>
#include  <conjugate_min.h>

typedef  struct
{
    int       n_parameters;
    Volume    volume1;
    Volume    volume2;
    int       n_samples[N_DIMENSIONS];
} function_data_struct;

private  Real  compute_cross_correlation(
    Volume     volume1,
    Volume     volume2,
    Transform  *transform,
    int        nx,
    int        ny,
    int        nz );

private  Real  function(
    Real   parameters[],
    void   *function_data )
{
    Transform              transform;
    function_data_struct   *func_data;

    func_data = (function_data_struct *) function_data;

    make_rotation_transform( parameters[0] * DEG_TO_RAD, X,
                             &transform );

    return( compute_cross_correlation( func_data->volume1,
                                       func_data->volume2,
                                       &transform,
                                       func_data->n_samples[0],
                                       func_data->n_samples[1],
                                       func_data->n_samples[2] ) );
}

private  void  function_deriv(
    Real   parameters[],
    void   *function_data,
    Real   deriv[] )
{
    Transform              transform;
    function_data_struct   *func_data;
    Real                   transform_deriv[4][4];

    func_data = (function_data_struct *) function_data;

    make_rotation_transform( parameters[0] * DEG_TO_RAD, X,
                             &transform );

    compute_cross_correlation_deriv( func_data->volume1,
                                     func_data->volume2,
                                     &transform,
                                     func_data->n_samples[0],
                                     func_data->n_samples[1],
                                     func_data->n_samples[2],
                                     transform_deriv );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING                 input_filename1, input_filename2;
    Real                   *initial, *solution, value;
    Real                   range_tolerance, domain_tolerance;
    Real                   sampling[N_DIMENSIONS];
    Real                   separations[N_DIMENSIONS];
    int                    n_iterations, dim, parm;
    int                    sizes[N_DIMENSIONS];
    function_data_struct   func_data;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename1 ) ||
        !get_string_argument( NULL, &input_filename2 ) ||
        !get_real_argument( 0.0, &sampling[0] ) ||
        !get_real_argument( 0.0, &sampling[1] ) ||
        !get_real_argument( 0.0, &sampling[2] ) )
    {
        return( 1 );
    }

    (void) get_real_argument( 0.01, &range_tolerance );
    (void) get_real_argument( 0.01, &domain_tolerance );
    (void) get_int_argument( 100, &n_iterations );
    (void) get_int_argument( 6, &func_data.n_parameters );

    if( input_volume( input_filename1, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &func_data.volume1, NULL ) != OK )
        return( 1 );

    get_volume_sizes( func_data.volume1, sizes );
    get_volume_separations( func_data.volume1, separations );

    if( equal_strings( input_filename1, input_filename2 ) )
    {
        func_data.volume2 = func_data.volume1;
    }
    else
    {
        if( input_volume( input_filename2, 3, File_order_dimension_names,
                          NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          TRUE, &func_data.volume2, NULL ) != OK )
            return( 1 );
    }

    ALLOC( initial, func_data.n_parameters );
    ALLOC( solution, func_data.n_parameters );

    for_less( parm, 0, func_data.n_parameters )
        initial[parm] = 0.0;

    for_less( dim, 0, N_DIMENSIONS )
    {
        func_data.n_samples[dim] = ROUND( (Real) sizes[dim] *
                                          FABS(separations[dim]) /
                                          sampling[dim] );
    }

    value = conjugate_minimize_function( func_data.n_parameters,
                                         initial, function, function_deriv,
                                         (void *) &func_data, range_tolerance,
                                         domain_tolerance, n_iterations, 2,
                                         solution );

    print( "Solution: " );

    for_less( parm, 0, func_data.n_parameters )
        print( " %g", solution[parm] );

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
    int   sizes[MAX_DIMENSIONS];
    int   i, j, k;
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
