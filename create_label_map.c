#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR   executable )
{
    static  VIO_STR  usage_str = "\n\
Usage: %s  template.mnc  output.mnc\n\
\n\
     Creates a label volume which is at the first slice of the template\n\
     and has all the label values in a square grid\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    int                  x, y, z, n_labels, n_up, hor, vert;
    int                  x_start, x_end, y_start, y_end, sizes[VIO_N_DIMENSIONS];
    int                  col;
    VIO_STR               input_filename, output_filename;
    VIO_Volume               volume, labels, cropped;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume_header_only( input_filename, 3, XYZ_dimension_names,
                                  &volume, NULL ) != VIO_OK )
        return( 1 );

    labels = copy_volume_definition( volume, NC_BYTE, FALSE, 0.0, 255.0 );
    set_volume_real_range( labels, 0.0, 255.0 );

    get_volume_sizes( labels, sizes );

    for_less( x, 0, sizes[VIO_X] )
    for_less( y, 0, sizes[VIO_Y] )
    for_less( z, 0, sizes[VIO_Z] )
    {
        set_volume_real_value( labels, x, y, z, 0, 0, 0.0 );
    }

    n_labels = (int) get_volume_real_max(labels);
    n_up = (n_labels + 9) / 10;

    for_less( col, 1, n_labels )
    {
        hor = col % 10;
        vert = col / 10;

        x_start = VIO_ROUND( (VIO_Real) hor / 10.0 * (VIO_Real) sizes[VIO_X] ) + 1;
        x_end = VIO_ROUND( (VIO_Real) (hor+1) / 10.0 * (VIO_Real) sizes[VIO_X] ) - 1;
        y_start = VIO_ROUND( (VIO_Real) vert / (VIO_Real) n_up * (VIO_Real) sizes[VIO_Y] ) + 1;
        y_end = VIO_ROUND( (VIO_Real) (vert+1) / (VIO_Real) n_up * (VIO_Real) sizes[VIO_Y] ) - 1;

        for_inclusive( x, x_start, x_end )
        for_inclusive( y, y_start, y_end )
            set_volume_real_value( labels, x, y, 0, 0, 0, (VIO_Real) col );
    }

    cropped = autocrop_volume( labels );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, cropped, "Label colour map\n", NULL );

    return( 0 );
}
