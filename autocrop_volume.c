
#include  <volume_io.h>
#include  <bicpl.h>

private  void  usage(
    char   executable[] )
{
    STRING   usage_str = "\n\
Usage: %s   input.mnc  output.mnc   [min_threshold]  [n_boundary_voxels]\n\
\n\
     Crops a volume by removing the outer voxels which are less than or equal\n\
     to min_threshold, or if none specified, zero.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING     input_filename, output_filename;
    Volume     volume, cropped_volume;
    Real       min_threshold, size_factor;
    int        dim, limits[2][MAX_DIMENSIONS];
    int        sizes[MAX_DIMENSIONS], n_boundary_voxels;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &min_threshold );
    (void) get_int_argument( 0, &n_boundary_voxels );
 
    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( !find_volume_crop_bounds( volume, -1.0e30, min_threshold, limits ) )
    {
        for_less( dim, 0, N_DIMENSIONS )
        {
            limits[0][dim] = 0;
            limits[1][dim] = 0;
        }
    }

    size_factor = 1.0;
    get_volume_sizes( volume, sizes );

    for_less( dim, 0, N_DIMENSIONS )
    {
        limits[0][dim] = MAX( 0, limits[0][dim] - n_boundary_voxels );
        limits[1][dim] = MIN( sizes[dim]-1, limits[1][dim] + n_boundary_voxels );
        size_factor *= (Real) (limits[1][dim] - limits[0][dim] + 1) /
                       (Real) sizes[dim];
    }

    print( "Cropping volume to %3.0f %% size.\n", size_factor * 100.0 + 0.49 );

    cropped_volume = create_cropped_volume( volume, limits );

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, cropped_volume, "Autocropped", NULL );
    
    return( 0 );
}
