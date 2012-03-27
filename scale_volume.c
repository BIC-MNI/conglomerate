#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               volume_filename;
    STRING               output_filename;
    Real                 x1, y1, x2, y2, m, b, value_at_centre, centre;
    Real                 value;
    int                  v[MAX_DIMENSIONS], axis, sizes[MAX_DIMENSIONS];
    Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_int_argument( 0, &axis ) ||
        !get_real_argument( 0.0, &x1 ) ||
        !get_real_argument( 0.0, &y1 ) ||
        !get_real_argument( 0.0, &x2 ) ||
        !get_real_argument( 0.0, &y2 ) )
    {
        return( 1 );
    }

    if( input_volume( volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    centre = sizes[axis] / 2;

    m = (y2 - y1) / (x2 - x1);
    b = y1 - m * x1;

    value_at_centre = m * centre + b;

    print( "%g %g %g %g\n", m, b, centre, value_at_centre );

    m = (y2/value_at_centre - y1/value_at_centre) / (x2 - x1);
    b = y1/value_at_centre - m * x1;

    print( "%g %g\n", m, b );

    BEGIN_ALL_VOXELS( volume, v[0], v[1], v[2], v[3], v[4] )

        value = get_volume_real_value( volume, v[0], v[1], v[2], v[3], v[4] );
        value = value / (m*v[axis] + b);
        set_volume_real_value( volume, v[0], v[1], v[2], v[3], v[4], value );

    END_ALL_VOXELS

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, volume, volume_filename,
                          "Scaled\n", NULL );

    return( 0 );
}
