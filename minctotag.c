#include  <mni.h>

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *volume_filename, *tag_filename;
    Volume               volume;
    Real                 min_id, max_id, value, xw, yw, zw;
    int                  x, y, z;
    int                  sizes[N_DIMENSIONS];
    int                  n_tags, *structure_ids, *patient_ids;
    Real                 **tags, *weights;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &tag_filename ) )
    {
        print( "%s  volume  output.tag\n", argv[0] );
        print( "%s  volume  output.tag  id\n", argv[0] );
        print( "%s  volume  output.tag  min_id  max_id\n", argv[0] );
        print( "\n" );
        print( "     creates a tag file from a volume.\n" );
        return( 1 );
    }

    if( get_real_argument( 0.5, &min_id ) )
    {
        (void) get_real_argument( min_id, &max_id );
    }
    else
    {
        min_id = 0.0;
        max_id = 0.0;
    }

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    n_tags = 0;

    /* --- create the output tags */

    get_volume_sizes( volume, sizes );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            for_less( z, 0, sizes[Z] )
            {
                GET_VALUE_3D( value, volume, x, y, z );
                if( min_id >= max_id || min_id <= value && value <= max_id )
                {
                    /*--- increase the memory allocation of the tag points */

                    SET_ARRAY_SIZE( tags, n_tags, n_tags+1, 10 );
                    ALLOC( tags[n_tags], 3 );

                    SET_ARRAY_SIZE( weights, n_tags, n_tags+1, 10 );
                    SET_ARRAY_SIZE( structure_ids, n_tags, n_tags+1, 10 );
                    SET_ARRAY_SIZE( patient_ids, n_tags, n_tags+1, 10 );

                    convert_3D_voxel_to_world( volume,
                            (Real) x, (Real) y, (Real) z, &xw, &yw, &zw );

                    tags[n_tags][0] = xw;
                    tags[n_tags][1] = yw;
                    tags[n_tags][2] = zw;

                    weights[n_tags] = 1.0;
                    structure_ids[n_tags] = ROUND( value );
                    patient_ids[n_tags] = 1;

                    ++n_tags;
                }
            }
        }
    }

    if( output_tag_file( tag_filename, "Created by converting volume to tags.",
                         1, n_tags, tags, NULL, weights, structure_ids,
                         patient_ids, NULL ) != OK )
        return( 1 );

    free_tag_points( 1, n_tags, tags, NULL, weights, structure_ids, patient_ids,
                     NULL );

    delete_volume( volume );

    return( 0 );
}
