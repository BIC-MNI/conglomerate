#include  <internal_volume_io.h>
#include  <bicpl.h>


private  Real  evaluate_error(
    Volume   volume,
    Real     max_diff );

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_filename, *output_filename;
    Volume               volume;
    Real                 max_diff;
    Real                 before, after;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &max_diff ) )
    {
        print( "%s  input.mnc  output.mnc  max_diff\n", argv[0] );
        return( 1 );
    }

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    before = evaluate_error( volume, max_diff );

    print( "Before: %g\n", before );

/*
    normalize_intensities( volume, max_diff );

    after = evaluate_error( volume, max_diff );

    print( "After : %g\n", after );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED,
                                   FALSE, 0.0, 0.0, volume, input_filename,
                                   "flip_volume", (minc_output_options *) NULL);

    delete_volume( volume );
*/

    return( 0 );
}

private  Real  evaluate_error(
    Volume   volume,
    Real     max_diff )
{
    int   tx, ty, tz, dx, dy, dz;
    int   x, y, z, sizes[N_DIMENSIONS];
    Real  error, value1, value2, diff;

    get_volume_sizes( volume, sizes );

    error = 0.0;

    for_less( x, 0, sizes[0] )
    {
        if( x < sizes[0]-1 )
            tx = 1;
        else
            tx = 0;
        for_less( y, 0, sizes[1] )
        {
            if( y < sizes[1]-1 )
                ty = 1;
            else
                ty = 0;
            for_less( z, 0, sizes[2] )
            {
                if( z < sizes[2]-1 )
                    tz = 1;
                else
                    tz = 0;

                GET_VALUE_3D( value1, volume, x, y, z );

                for_less( dx, 0, tx )
                for_less( dy, 0, ty )
                for_less( dz, 0, tz )
                {
                    if( dx == 0 && dy == 0 && dz == 0 )
                        continue;

                    GET_VALUE_3D( value2, volume, x + dx, y + dy, z + dz );

                    diff = value1 - value2;
                    if( diff < 0.0 )
                        diff = -diff;

                    if( diff <= max_diff )
                        error += (value1 - value2) * (value1 - value2);
                }
            }
        }
    }

    return( error );
}
