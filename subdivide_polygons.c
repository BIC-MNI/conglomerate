#include <volume_io/internal_volume_io.h>
#include <bicpl.h>

#define  BINTREE_FACTOR  0.3

private  void  resample_polygons(
    polygons_struct    *polygons,
    int                new_n_polygons );

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  input.obj  [output.obj]  [new_n_polygons]\n\
\n\
     Subdivides any polygons in the file, placing output in the original file\n\
     or in a different output file.   If a new_n_polygons is specified\n\
     then the polygons will be resampled to this size.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING           input_filename, output_filename;
    int              i, n_objects;
    File_formats     format;
    object_struct    **object_list;
    polygons_struct  *polygons;
    int              new_n_polygons;
    BOOLEAN          sampling_specified;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_string_argument( input_filename, &output_filename );
    sampling_specified = get_int_argument( 0, &new_n_polygons );

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        if( get_object_type(object_list[i]) == POLYGONS )
        {
            polygons = get_polygons_ptr( object_list[i] );

            if( sampling_specified && new_n_polygons != polygons->n_items * 4 )
            {
                if( !is_this_tetrahedral_topology( polygons ) )
                {
                    print_error( "Polygons are not tetrahedral topology.\n" );
                    return( 1 );
                }

                resample_polygons( polygons, new_n_polygons );
            }
            else
                subdivide_polygons( get_polygons_ptr(object_list[i]) );
            compute_polygon_normals( get_polygons_ptr(object_list[i]) );
        }
    }

    (void) output_graphics_file( output_filename, format,
                                 n_objects, object_list );

    delete_object_list( n_objects, object_list );

    return( 0 );
}

private  void  resample_polygons(
    polygons_struct    *polygons,
    int                new_n_polygons )
{
    int               p, poly;
    Point             centre, *points, src_point;
    polygons_struct   src_unit_sphere, dest_unit_sphere;

    fill_Point( centre, 0.0, 0.0, 0.0 );

    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0,
                               polygons->n_items, &src_unit_sphere );

    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0,
                               new_n_polygons, &dest_unit_sphere );

    create_polygons_bintree( &src_unit_sphere,
                             ROUND( (Real) src_unit_sphere.n_items *
                                    BINTREE_FACTOR ) );

    ALLOC( points, dest_unit_sphere.n_points );

    for_less( p, 0, dest_unit_sphere.n_points )
    {
        poly = find_closest_polygon_point( &dest_unit_sphere.points[p],
                                           &src_unit_sphere,
                                           &src_point );

        
        map_point_between_polygons( &src_unit_sphere, poly,
                                    &src_point, polygons,
                                    &points[p] );
    }

    delete_polygons( &src_unit_sphere );
    delete_polygons( polygons );

    *polygons = dest_unit_sphere;
    FREE( polygons->points );
    polygons->points = points;
}
