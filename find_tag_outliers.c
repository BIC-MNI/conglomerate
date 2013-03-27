#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR               volume_filename, input_tags;
    VIO_Volume               volume;
    int                  i, n_inside;
    int                  n_volumes, n_tag_points;
    VIO_Real                 **tags_volume1, **tags_volume2, value;
    VIO_Real                 voxel[VIO_N_DIMENSIONS], avg_threshold, tag_threshold;
    int                  int_voxel[VIO_N_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_real_argument( 0.0, &avg_threshold ) ||
        !get_real_argument( 0.0, &tag_threshold ) )
    {
        print( "%s  avg_tags.mnc  avg_threshold tag_threshold input_tags.tag\n",
               argv[0] );
        return( 1 );
    }

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != VIO_OK )
        return( 1 );

    while( get_string_argument( "", &input_tags ) )
    {
        if( input_tag_file( input_tags, &n_volumes, &n_tag_points,
                            &tags_volume1, &tags_volume2,
                            NULL, NULL, NULL, NULL ) != VIO_OK )
            return( 1 );

        n_inside = 0;

        for_less( i, 0, n_tag_points )
        {
            convert_world_to_voxel( volume, tags_volume1[i][VIO_X],
                                    tags_volume1[i][VIO_Y],
                                    tags_volume1[i][VIO_Z],
                                    voxel );

            convert_real_to_int_voxel( VIO_N_DIMENSIONS, voxel, int_voxel );

            if( int_voxel_is_within_volume( volume, int_voxel ) )
            {
                value = get_volume_real_value( volume, int_voxel[VIO_X],
                                         int_voxel[VIO_Y], int_voxel[VIO_Z], 0, 0 );

                if( value >= avg_threshold )
                    ++n_inside;
            }
        }

        if( (VIO_Real) n_inside / (VIO_Real) n_tag_points < tag_threshold )
            print( "POSSIBLE TAG OUTLIER: %s\n", input_tags );

        free_tag_points( n_volumes, n_tag_points, tags_volume1, tags_volume2,
                         NULL, NULL, NULL, NULL );
    }

    return( 0 );
}
