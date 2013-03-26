#include  <volume_io.h>
#include  <bicpl.h>

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s  input.tag  output.obj  radius  [optional_n_triangles]\n\
\n\
     Creates a set of spheres with the given radius to correspond with the\n\
     given tags.  If the radius is less than or equal to 0, then the weights\n\
     stored in the tag file are used as the radii.  The default n_triangles \n\
     is 80.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR              input_filename, output_filename;
    int                 i, n_triangles;
    int                 n_volumes, n_tags;
    VIO_Real                **tags, *weights, radius, this_radius;
    object_struct       **object_list;
    VIO_Point               centre;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &radius ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 80, &n_triangles );

    if( input_tag_file( input_filename, &n_volumes, &n_tags,
                        &tags, NULL, &weights,
                        NULL, NULL, NULL ) != OK )
        return( 1 );

    ALLOC( object_list, n_tags );

    for_less( i, 0, n_tags )
    {
        object_list[i] = create_object( POLYGONS );

        fill_Point( centre, tags[i][0],
                            tags[i][1],
                            tags[i][2] );

        if( radius <= 0.0 )
            this_radius = weights[i];
        else
            this_radius = radius;

        create_tetrahedral_sphere( &centre,
                                   this_radius, this_radius, this_radius,
                                   n_triangles,
                                   get_polygons_ptr(object_list[i]));
    }

    (void) output_graphics_file( output_filename, BINARY_FORMAT, n_tags,
                                 object_list );

    free_tag_points( n_volumes, n_tags, tags, NULL, weights,
                     NULL, NULL, NULL );

    return( 0 );
}
