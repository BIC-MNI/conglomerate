#include  <def_mni.h>
#include  <def_module.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Status          status;
    char            *output_filename;
    int             resolution, p;
    object_struct   *object;
    polygons_struct *polygons;
    int             i, n_indices;
    Surfprop        spr;
    Real            cx, cy, cz, rx, ry, rz, dx, dy, dz, scale;
    Real            x, z, c;

    status = OK;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &cx ) ||
        !get_real_argument( 0.0, &cy ) ||
        !get_real_argument( 0.0, &cz ) ||
        !get_real_argument( 0.0, &rx ) ||
        !get_real_argument( 0.0, &ry ) ||
        !get_real_argument( 0.0, &rz ) )
    {
        (void) fprintf( stderr, "Must have a filename and resolution.\n" );
        return( 1 );
    }

    (void) get_int_argument( 1, &resolution );

    object = create_object( POLYGONS );
    polygons = get_polygons_ptr(object);
    fill_Surfprop( spr, 0.3, 0.6, 0.6, 60.0, 1.0 );
    initialize_polygons( polygons, WHITE, &spr );
    polygons->n_points = 4;
    ALLOC( polygons->points, polygons->n_points );
    ALLOC( polygons->normals, polygons->n_points );

    x = sqrt( 8.0 / 9.0 );
    z = - 1.0 / 3.0;
    c = sqrt( 0.75 );
    fill_Point( polygons->points[0], cx, cy, cz + rz );
    fill_Point( polygons->points[1], cx + x * rx, cy, cz + z * rz );
    fill_Point( polygons->points[2], cx - 0.5 * x * rx,
                                     cy + c * ry,
                                     cz + z * rz );
    fill_Point( polygons->points[3], cx - 0.5 * x * rx,
                                     cy - c * ry,
                                     cz + z * rz );

    polygons->n_items = 0;
    n_indices = 0;

    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 0, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 1, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 2, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->end_indices, polygons->n_items,
                          n_indices, DEFAULT_CHUNK_SIZE );

    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 0, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 2, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 3, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->end_indices, polygons->n_items,
                          n_indices, DEFAULT_CHUNK_SIZE );

    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 0, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 3, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 1, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->end_indices, polygons->n_items,
                          n_indices, DEFAULT_CHUNK_SIZE );

    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 1, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 3, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 2, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->end_indices, polygons->n_items,
                          n_indices, DEFAULT_CHUNK_SIZE );

    for_less( i, 1, resolution )
    {
        subdivide_polygons( polygons );

        for_less( p, 0, polygons->n_points )
        {
            dx = Point_x(polygons->points[p]) - cx;
            dy = Point_y(polygons->points[p]) - cy;
            dz = Point_z(polygons->points[p]) - cz;
            scale = dx * dx / rx / rx + dy * dy / ry / ry + dz * dz / rz / rz;
            scale = 1.0 / sqrt( scale );
            dx *= scale;
            dy *= scale;
            dz *= scale;
            fill_Point( polygons->points[p], cx + dx, cy + dy, cz + dz );
        }
    }

    compute_polygon_normals( polygons );

    status = output_graphics_file( output_filename, BINARY_FORMAT, 1, &object );

    if( status == OK )
        delete_object( object );

    if( status == OK )
        print( "Tetrahedron output.\n" );

    return( status != OK );
}
