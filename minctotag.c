#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR  executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s  volume  output.tag\n\
       %s  volume  output.tag  id\n\
       %s  volume  output.tag  min_id  max_id\n\
\n\
     Creates a tag file from a volume.  The three forms create tags from:\n\
           all non-zero voxels,\n\
           all voxels with values exactly equal to id, and\n\
           all voxels with values from min_id to max_id, respectively.\n\n";

    print_error( usage_str, executable, executable, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               volume_filename, tag_filename;
    VIO_Volume               volume;
    FILE                 *file;
    VIO_Real                 min_id, max_id, value;
    int                  x, y, z;
    int                  sizes[VIO_N_DIMENSIONS];
    int                  n_tags, structure_id;
    VIO_Real                 tag[VIO_N_DIMENSIONS];
    VIO_progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &tag_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    if( get_real_argument( 0.5, &min_id ) )
    {
        (void) get_real_argument( min_id, &max_id );
    }
    else
    {
        min_id = 0.0;
        max_id = -1.0;
    }

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    /* --- create the output tags */

    get_volume_sizes( volume, sizes );

    if( open_file_with_default_suffix( tag_filename,
                                       get_default_tag_file_suffix(),
                                       WRITE_FILE, ASCII_FORMAT,
                                       &file ) != VIO_OK ||
        initialize_tag_file_output( file,
                "Created by converting volume to tags (minctotag).", 1 ) != VIO_OK )
    {
        return( 1 );
    }

    initialize_progress_report( &progress, FALSE, sizes[VIO_X] * sizes[VIO_Y],
                                "Creating tags" );

    n_tags = 0;
    for_less( x, 0, sizes[VIO_X] )
    {
        for_less( y, 0, sizes[VIO_Y] )
        {
            for_less( z, 0, sizes[VIO_Z] )
            {
                value = get_volume_real_value( volume, x, y, z, 0, 0 );
                if( min_id > max_id && value != 0.0 ||
                    min_id <= value && value <= max_id )
                {
                    convert_3D_voxel_to_world( volume,
                                               (VIO_Real) x, (VIO_Real) y, (VIO_Real) z,
                                               &tag[VIO_X], &tag[VIO_Y], &tag[VIO_Z] );

                    structure_id = VIO_ROUND( value );

                    if( output_one_tag( file, 1, tag, NULL, &value,
                                        &structure_id, NULL, NULL ) != VIO_OK )
                    {
                        return( 1 );
                    }

                    ++n_tags;
                }
            }

            update_progress_report( &progress, x * sizes[VIO_Y] + y + 1 );
        }
    }

    terminate_progress_report( &progress );

    terminate_tag_file_output( file );

    print( "Created %d tags.\n", n_tags );

    delete_volume( volume );

    return( 0 );
}
