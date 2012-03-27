#include  <volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    static  STRING  usage_str = "\n\
Usage: %s  labels.mnc [min_value max_value]\n\
\n\
     Displays a count of the number of voxels of the different labels in the\n\
     volume.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               volume_filename;
    Real                 min_volume, max_volume;
    Real                 min_value, max_value, label;
    int                  v0, v1, v2, v3, v4, n_labels;
    int                  i, *counts;
    Volume               volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &min_value );
    (void) get_real_argument( min_value - 1.0, &max_value );

    if( input_volume( volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    get_volume_real_range( volume, &min_volume, &max_volume );

    n_labels = ROUND( max_volume ) - ROUND( min_volume ) + 1;

    ALLOC( counts, n_labels );

    for_less( i, 0, n_labels )
        counts[i] = 0;

    BEGIN_ALL_VOXELS( volume, v0, v1, v2, v3, v4 )
        label = get_volume_real_value( volume, v0, v1, v2, v3, v4 );
        ++counts[ROUND(label) - ROUND(min_volume)];
    END_ALL_VOXELS

    for_inclusive( i, ROUND(min_volume), ROUND(max_volume) )
    {
        if( i == 0 || counts[i-ROUND(min_volume)] == 0 )
            continue;

        print( "Label: %d %d\n", i, counts[i-ROUND(min_volume)] );
    }

    FREE( counts );

    return( 0 );
}
