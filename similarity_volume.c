#include  <bicpl.h>
#include  <volume_io.h>

private  void  usage(
    STRING   executable_name )
{
    STRING  usage_str = "\n\
Usage: %s  output.mnc w1 w2 w3  [w input.mnc] [w input.mnc]\n\
\n\
     \n\n";

     print_error( usage_str, executable_name );

}

int  main(
    int    argc,
    char   *argv[] )
{
    int                  v0, v1, v2, v3, v4;
    Real                 weight, value_weight, deriv1_weight, deriv2_weight;
    Real                 xw, yw, zw, add;
    Real                 test_deriv1_mag, test_deriv2_mag;
    Real                 deriv1_mag, deriv2_mag;
    Real                 v[MAX_DIMENSIONS], xv, yv, zv;
    Real                 diff, value, test_value;
    Real                 deriv1[3], test_deriv1[3];
    Real                 deriv2[6], test_deriv2[6], sim;
    Real                 min_value, max_value;
    int                  degrees_continuity, sizes[MAX_DIMENSIONS];
    progress_struct      progress;
    STRING               input_filename;
    STRING               output_filename;
    STRING               history;
    Volume               volume, similarity_volume;
    BOOLEAN              first;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &value_weight ) ||
        !get_real_argument( 0.0, &deriv1_weight ) ||
        !get_real_argument( 0.0, &deriv2_weight ) ||
        !get_real_argument( 0.0, &xv ) ||
        !get_real_argument( 0.0, &yv ) ||
        !get_real_argument( 0.0, &zv ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( deriv2_weight != 0.0 )
        degrees_continuity = 2;
    else
        degrees_continuity = 0;

    first = TRUE;

    while( get_real_argument( 0.0, &weight ) &&
           get_string_argument( NULL, &input_filename ) )
    {
        if( weight == 0.0 )
            continue;

        /* read the input volume */

        if( input_volume( input_filename, 3, XYZ_dimension_names,
                          NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          TRUE, &volume, (minc_input_options *) NULL ) != OK )
            return( 1 );

        /* --- create the similarity volume */

        if( first )
        {
            first = FALSE;

            similarity_volume = copy_volume_definition( volume, NC_FLOAT, FALSE,
                                                        0.0, 0.0 );

            BEGIN_ALL_VOXELS( similarity_volume, v0, v1, v2, v3, v4 )
                set_volume_real_value( similarity_volume, v0, v1, v2, v3, v4,
                                       0.0 );
            END_ALL_VOXELS
        }

        v[X] = xv;
        v[Y] = yv;
        v[Z] = zv;
        convert_voxel_to_world( similarity_volume, v, &xw, &yw, &zw );
        evaluate_volume_in_world( volume, xw, yw, zw, degrees_continuity,
                                  FALSE, 0.0, &test_value,
                          &test_deriv1[0], &test_deriv1[1], &test_deriv1[2],
                          &test_deriv2[0], &test_deriv2[1], &test_deriv2[2],
                          &test_deriv2[3], &test_deriv2[4], &test_deriv2[5] );

        test_deriv1_mag = sqrt( test_deriv1[0] * test_deriv1[0] +
                                test_deriv1[1] * test_deriv1[1] +
                                test_deriv1[2] * test_deriv1[2] );

        test_deriv2_mag = sqrt( test_deriv2[0] * test_deriv2[0] +
                                test_deriv2[1] * test_deriv2[1] +
                                test_deriv2[2] * test_deriv2[2] +
                                test_deriv2[3] * test_deriv2[3] +
                                test_deriv2[4] * test_deriv2[4] +
                                test_deriv2[5] * test_deriv2[5] );

        get_volume_sizes( volume, sizes );

        initialize_progress_report( &progress, FALSE,
                                sizes[X] * sizes[Y] * sizes[Z], "Similarity:" );

        BEGIN_ALL_VOXELS( similarity_volume, v0, v1, v2, v3, v4 )
            v[0] = (Real) v0;
            v[1] = (Real) v1;
            v[2] = (Real) v2;
            v[3] = (Real) v3;
            v[4] = (Real) v4;
            convert_voxel_to_world( similarity_volume, v, &xw, &yw, &zw );
            evaluate_volume_in_world( volume, xw, yw, zw, degrees_continuity,
                                      FALSE, 0.0, &value,
                                      &deriv1[0], &deriv1[1], &deriv1[2],
                                      &deriv2[0], &deriv2[1], &deriv2[2],
                                      &deriv2[3], &deriv2[4], &deriv2[5] );

            add = 0.0;

            if( value_weight != 0.0 )
            {
                diff = value - test_value;
                add += value_weight * diff * diff;
            }

            if( deriv1_weight != 0.0 )
            {
                deriv1_mag = sqrt( deriv1[0] * deriv1[0] +
                                   deriv1[1] * deriv1[1] +
                                   deriv1[2] * deriv1[2] );

                diff = test_deriv1_mag - deriv1_mag;
                add += deriv1_weight * diff * diff;
            }

            if( deriv2_weight != 0.0 )
            {
                deriv2_mag = sqrt( deriv2[0] * deriv2[0] +
                                   deriv2[1] * deriv2[1] +
                                   deriv2[2] * deriv2[2] +
                                   deriv2[3] * deriv2[3] +
                                   deriv2[4] * deriv2[4] +
                                   deriv2[5] * deriv2[5] );

                diff = test_deriv2_mag - deriv2_mag;
                add += deriv2_weight * diff * diff;
            }

            sim = get_volume_real_value( similarity_volume,
                                         v[0], v[1], v[2], v[3], v[4] );
            set_volume_real_value( similarity_volume,
                                   v[0], v[1], v[2], v[3], v[4],
                                   sim + weight * add );

            update_progress_report( &progress, v0 * sizes[Y] * sizes[Z] +
                                               v1 * sizes[Z] + v2 + 1 );

        END_ALL_VOXELS

        terminate_progress_report( &progress );

        delete_volume( volume );
    }

    min_value = get_volume_real_value( similarity_volume, 0, 0, 0, 0, 0 );
    max_value = min_value;

    BEGIN_ALL_VOXELS( similarity_volume, v0, v1, v2, v3, v4 )
        sim = get_volume_real_value( similarity_volume, v0, v1, v2, v3, v4 );
        if( sim < min_value )
            min_value = sim;
        else if( sim > max_value )
            max_value = sim;
    END_ALL_VOXELS

    print( "Range: %g %g\n", min_value, max_value );

    set_volume_voxel_range( similarity_volume, min_value, max_value );
    set_volume_real_range( similarity_volume, min_value, max_value );

    history = create_string( "similarity_volume\n" );

    (void) output_modified_volume( output_filename, NC_SHORT, FALSE,
                           0.0, 0.0, similarity_volume, input_filename,
                           history, (minc_output_options *) NULL );


    return( 0 );
}
