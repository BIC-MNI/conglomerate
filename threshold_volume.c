#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    static  STRING  usage_str = "\n\
Usage: %s  input.mnc  labels.mnc  min_value  max_value  label_value  [out.mnc]\n\
\n\
     Creates a label volume which has the value of label_value where the input\n\
     voxels are between min_value and max_value.\n\
     If out.mnc is specified, it modifies the labels.mnc, otherwise, it just\n\
     creates labels.mnc\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               volume_filename;
    STRING               labels_filename, output_filename;
    BOOLEAN              modifying_flag;
    Real                 min_value, max_value, value;
    int                  v[MAX_DIMENSIONS], label_value_to_set;
    Volume               volume, label_volume;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &labels_filename ) ||
        !get_real_argument( 0.0, &min_value ) ||
        !get_real_argument( 0.0, &max_value ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 1, &label_value_to_set );
    modifying_flag = get_string_argument( labels_filename, &output_filename );

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    if( modifying_flag )
    {
        if( input_volume( labels_filename, 3, XYZ_dimension_names,
                          NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          TRUE, &label_volume, (minc_input_options *) NULL )
                          != OK )
            return( 1 );

        if( !volumes_are_same_grid( volume, label_volume ) )
        {
            print_error( "The label volume must match the intensity volume.\n");
            return( 1 );
        }
    }
    else
    {
        label_volume = create_label_volume( volume, NC_UNSPECIFIED );
    }

    BEGIN_ALL_VOXELS( volume, v[0], v[1], v[2], v[3], v[4] )

        value = get_volume_real_value( volume, v[0], v[1], v[2], v[3], v[4] );

        if( value >= min_value && value <= max_value )
            set_volume_label_data( label_volume, v, label_value_to_set );
        else if( !modifying_flag )
            set_volume_label_data( label_volume, v, 0 );


    END_ALL_VOXELS

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED, FALSE,
                                   0.0, 0.0, label_volume, volume_filename,
                                   "Thresholded", NULL );

    return( 0 );
}
