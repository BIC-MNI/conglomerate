#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    static  STRING  usage_str = "\n\
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
    int                  x_start, x_end, y_start, y_end, sizes[N_DIMENSIONS];
    int                  col;
    STRING               input_filename, output_filename;
    Volume               volume, labels, cropped;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( input_volume_header_only( input_filename, 3, XYZ_dimension_names,
                                  &volume, NULL ) != OK )
        return( 1 );

    labels = copy_volume_definition( volume, NC_BYTE, FALSE, 0.0, 255.0 );
    set_volume_real_range( labels, 0.0, 255.0 );

    get_volume_sizes( labels, sizes );

    for_less( x, 0, sizes[X] )
    for_less( y, 0, sizes[Y] )
    for_less( z, 0, sizes[Z] )
    {
        set_volume_real_value( labels, x, y, z, 0, 0, 0.0 );
    }

    n_labels = (int) get_volume_real_max(labels);
    n_up = (n_labels + 9) / 10;

    for_less( col, 1, n_labels )
    {
        hor = col % 10;
        vert = col / 10;

        x_start = ROUND( (Real) hor / 10.0 * (Real) sizes[X] ) + 1;
        x_end = ROUND( (Real) (hor+1) / 10.0 * (Real) sizes[X] ) - 1;
        y_start = ROUND( (Real) vert / (Real) n_up * (Real) sizes[Y] ) + 1;
        y_end = ROUND( (Real) (vert+1) / (Real) n_up * (Real) sizes[Y] ) - 1;

        for_inclusive( x, x_start, x_end )
        for_inclusive( y, y_start, y_end )
            set_volume_real_value( labels, x, y, 0, 0, 0, (Real) col );
    }

    cropped = autocrop_volume( labels );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, cropped, "Label colour map\n", NULL );

    return( 0 );
}
