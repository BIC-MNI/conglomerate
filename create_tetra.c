#include  <def_mni.h>
#include  <def_module.h>

int  main(
    int    argc,
    char   *argv[] )
{
    Status          status;
    char            *output_filename;
    object_struct   *object;
    polygons_struct *polygons;
    int             i, n_indices;
    Surfprop        spr;
    Real            cx, cy, cz, rx, ry, rz;
    Real            angle;

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

    object = create_object( POLYGONS );
    polygons = get_polygons_ptr(object);
    fill_Surfprop( spr, 0.3, 0.6, 0.6, 60.0, 1.0 );
    initialize_polygons( polygons, WHITE, &spr );
    polygons->n_points = 5;
    ALLOC( polygons->points, polygons->n_points );
    ALLOC( polygons->normals, polygons->n_points );

    fill_Point( polygons->points[0], cx, cy, cz + rz );
    fill_Point( polygons->points[4], cx, cy, cz - rz );

    for_less( i, 0, 3 )
    {
        angle = (Real) i / 3.0 * 2.0 * PI;
        fill_Point( polygons->points[i+1],
                    cx + rx * cos( angle ),
                    cy + ry * sin( angle ),
                    cz );
    }

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

    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 4, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 2, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 1, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->end_indices, polygons->n_items,
                          n_indices, DEFAULT_CHUNK_SIZE );

    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 4, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 3, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 2, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->end_indices, polygons->n_items,
                          n_indices, DEFAULT_CHUNK_SIZE );

    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 4, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 1, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->indices, n_indices, 3, DEFAULT_CHUNK_SIZE );
    ADD_ELEMENT_TO_ARRAY( polygons->end_indices, polygons->n_items,
                          n_indices, DEFAULT_CHUNK_SIZE );

    compute_polygon_normals( polygons );

    status = output_graphics_file( output_filename, BINARY_FORMAT, 1, &object );

    if( status == OK )
        delete_object( object );

    if( status == OK )
        print( "Tetrahedron output.\n" );

    return( status != OK );
}
