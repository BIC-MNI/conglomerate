#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR   executable )
{
    static  VIO_STR  usage_str = "\n\
Usage: %s  input.mnc  labels.mnc|labels.tag  min_value  max_value  label_value\n\
         [out.mnc] | [min_value2 max_value2] | [min_value...] | [-crop] ...\n\
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
    VIO_STR               volume_filename, arg;
    VIO_STR               labels_filename, output_filename;
    VIO_BOOL              modifying_flag;
    VIO_Real                 min_value, max_value;
    VIO_Real                 *min_values, *max_values, value;
    VIO_BOOL              crop_flag, in_range;
    int                  v[VIO_MAX_DIMENSIONS], label_value_to_set;
    int                  i, n_ranges;
    VIO_Volume               volume, label_volume, out_volume;

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

    n_ranges = 0;
    min_values = NULL;
    max_values = NULL;
    SET_ARRAY_SIZE( min_values, n_ranges, n_ranges+1, DEFAULT_CHUNK_SIZE );
    SET_ARRAY_SIZE( max_values, n_ranges, n_ranges+1, DEFAULT_CHUNK_SIZE );
    min_values[n_ranges] = min_value;
    max_values[n_ranges] = max_value;
    ++n_ranges;

    modifying_flag = FALSE;
    crop_flag = FALSE;
    output_filename = labels_filename;

    while( get_string_argument( NULL, &arg ) )
    {
        if( filename_extension_matches( arg, "mnc" ) )
        {
            if( modifying_flag )
            {
                usage( argv[0] );
                return( 1 );
            }
            output_filename = arg;
            modifying_flag = TRUE;
        }
        else if( equal_strings( arg, "-crop" ) )
            crop_flag = TRUE;
        else
        {
            if( sscanf( arg, "%lg", &min_value ) != 1 ||
                !get_real_argument( 0.0, &max_value ) )
            {
                usage( argv[0] );
                return( 1 );
            }

            SET_ARRAY_SIZE( min_values, n_ranges, n_ranges+1,
                            DEFAULT_CHUNK_SIZE );
            SET_ARRAY_SIZE( max_values, n_ranges, n_ranges+1,
                            DEFAULT_CHUNK_SIZE );
            min_values[n_ranges] = min_value;
            max_values[n_ranges] = max_value;
            ++n_ranges;
        }
    }

    if( input_volume( volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    if( modifying_flag )
    {
        if( create_label_volume_from_file( labels_filename, volume,
                                           &label_volume ) != VIO_OK )
            return( VIO_ERROR );
    }
    else
    {
        label_volume = create_label_volume( volume, NC_UNSPECIFIED );
    }

    BEGIN_ALL_VOXELS( volume, v[0], v[1], v[2], v[3], v[4] )

        value = get_volume_real_value( volume, v[0], v[1], v[2], v[3], v[4] );

        in_range = FALSE;
        for_less( i, 0, n_ranges )
        {
            if( value >= min_values[i] && value <= max_values[i] )
            {
                in_range = TRUE;
                break;
            }
        }

        if( in_range )
            set_volume_label_data( label_volume, v, label_value_to_set );
        else if( !modifying_flag )
            set_volume_label_data( label_volume, v, 0 );


    END_ALL_VOXELS

    if( crop_flag )
        out_volume = autocrop_volume( label_volume );
    else
        out_volume = label_volume;

    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, out_volume, "Thresholded\n", NULL );

    return( 0 );
}
