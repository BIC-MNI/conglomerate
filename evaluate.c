#include  <bicpl.h>
#include  <volume_io/internal_volume_io.h>


/*! \brief Look up volume value for each point of object.
 *
 * Typically used for vertex colouring of objects.  
 * For each vertex of the geometrical object, the point is located in
 * the volume.  The volume values are written to the output text file
 * in the order that the vertices are visited.  Each object in the
 * BIC obj file is processed in sequence.
 */


int  main(
    int    argc,
    char   *argv[] )
{
    Volume               volume;
    STRING               input_filename, output_filename;
    STRING               volume_filename;
    int                  i, n_objects, p, n_points, degree, n_values;
    int                  dim, sizes[MAX_DIMENSIONS];
    Real                 *values, *evaluate_values;
    Real                 xyz_voxel[N_DIMENSIONS], voxel[MAX_DIMENSIONS];
    Point                *points;
    File_formats         format, output_format;
    object_struct        **object_list;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error( "Usage: %s  volume.mnc  input.obj  output.txt\n", argv[0]);
        return( 1 );
    }

    (void) get_int_argument( 0, &degree );

    if( input_volume( volume_filename, -1, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    xyz_voxel[X] = 1.0;
    xyz_voxel[Y] = 2.0;
    xyz_voxel[Z] = 3.0;

    reorder_xyz_to_voxel( volume, xyz_voxel, voxel );

    n_values = 1;
    get_volume_sizes( volume, sizes );
    for_less( dim, 0, get_volume_n_dimensions(volume) )
    {
        if( voxel[dim] == 0.0 )
            n_values *= sizes[dim];
    }

    ALLOC( evaluate_values, n_values );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    if( filename_extension_matches( output_filename, MNC_ENDING ) )
        output_format = BINARY_FORMAT;
    else
        output_format = ASCII_FORMAT;

    n_values = 0;
    values = NULL;
    
    for_less( i, 0, n_objects )
    {
        n_points = get_object_points( object_list[i], &points );

        for_less( p, 0, n_points )
        {
            evaluate_volume_in_world( volume,
                                      (Real) Point_x(points[p]),
                                      (Real) Point_y(points[p]),
                                      (Real) Point_z(points[p]),
                                      degree, FALSE, 0.0, evaluate_values,
                                      NULL, NULL, NULL,
                                      NULL, NULL, NULL, NULL, NULL, NULL );

            ADD_ELEMENT_TO_ARRAY( values, n_values, evaluate_values[0],
                                  DEFAULT_CHUNK_SIZE );
        }
    }

    FREE( evaluate_values );

    (void) output_texture_values( output_filename, output_format, n_values,
                                  values );

    delete_object_list( n_objects, object_list );

    return( 0 );
}
