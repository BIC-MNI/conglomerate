#include  <bicpl.h>
#include  <internal_volume_io.h>

private  void  get_random_unit_vector(
    Vector   *v );

int  main(
    int    argc,
    char   *argv[] )
{
    char                 comment[EXTREMELY_LARGE_STRING_SIZE];
    STRING               src_filename, output_filename, tag_filename;
    int                  n_points, dim, delta[N_DIMENSIONS];
    int                  n_objects, ind, i, initial_seed;
    Point                *points, centroid, min_corner, max_corner, point;
    Vector               offset;
    File_formats         format;
    object_struct        **object_list, **tag_objects;
    marker_struct        *marker;
    Real                 domain_factor, warp_distance;
    Real                 **tags1, **tags2;
    BOOLEAN              dump_tags;
    General_transform    transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_real_argument( 0.0, &domain_factor ) ||
        !get_real_argument( 0.0, &warp_distance ) )
    {
        print_error( "Usage: %s  src.obj dest.xfm\n",
                     argv[0] );
        print_error( "           [domain_factor] [warp_distance] [seed]\n" );
        return( 1 );
    }

    if( get_int_argument( 0, &initial_seed ) )
        set_random_seed( initial_seed );

    dump_tags = get_string_argument( NULL, &tag_filename );

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    if( n_objects != 1 )
    {
        print_error( "Must contain exactly one object.\n" );
        return( 1 );
    }

    n_points = get_object_points( object_list[0], &points );

    get_points_centroid( n_points, points, &centroid );

    get_range_points( n_points, points, &min_corner, &max_corner );

    ALLOC( tags1, 28 );
    ALLOC( tags2, 28 );

    for_less( i, 0, 28 )
    {
        ALLOC( tags1[i], 3 );
        ALLOC( tags2[i], 3 );
    }

    ind = 0;
    for_inclusive( delta[X], -1, 1 )
    for_inclusive( delta[Y], -1, 1 )
    for_inclusive( delta[Z], -1, 1 )
    {
        for_less( dim, 0, N_DIMENSIONS )
        {
            tags1[ind][dim] = (Real) Point_coord(centroid,dim) +
                              (Real) delta[dim] * domain_factor *
                              ((Real) Point_coord(max_corner,dim) -
                               (Real) Point_coord(min_corner,dim));
            tags2[ind][dim] = tags1[ind][dim];
        }

        ++ind;
    }

    point = points[get_random_int(n_points)];

    tags1[27][X] = (Real) Point_x(point);
    tags1[27][Y] = (Real) Point_y(point);
    tags1[27][Z] = (Real) Point_z(point);

    get_random_unit_vector( &offset );

    for_less( dim, 0, N_DIMENSIONS )
        tags2[27][dim] = tags1[27][dim] +
                         warp_distance * (Real) Vector_coord(offset,dim );

    safe_compute_transform_from_tags( 28, tags2, tags1, TRANS_TPS,
                                      &transform );

    (void) sprintf( comment, " Random warp: %g %g %g --> %g %g %g\n",
                    tags1[27][X], tags1[27][Y], tags1[27][Z],
                    tags2[27][X], tags2[27][Y], tags2[27][Z] );

    (void) output_transform_file( output_filename, comment, &transform );

    if( dump_tags )
    {
        ALLOC( tag_objects, 56 );
        for_less( i, 0, 56 )
            tag_objects[i] = create_object( MARKER );

        for_less( i, 0, 28 )
        {
            marker = get_marker_ptr(tag_objects[i]);
            initialize_marker( marker, SPHERE_MARKER, WHITE );
            marker->size = 1.0;
            marker->label = create_string( "Source" );
            marker->structure_id = 0;
            marker->patient_id = 0;
            fill_Point( marker->position,
                        tags1[i][X], tags1[i][Y], tags1[i][Z] );

            marker = get_marker_ptr(tag_objects[i+28]);
            initialize_marker( marker, SPHERE_MARKER, WHITE );
            marker->size = 1.0;
            marker->label = create_string( "Dest" );
            marker->structure_id = 0;
            marker->patient_id = 0;
            fill_Point( marker->position,
                        tags2[i][X], tags2[i][Y], tags2[i][Z] );
        }

        (void) output_graphics_file( tag_filename, ASCII_FORMAT,
                                     56, tag_objects );
    }

#ifdef DEBUG

#define   TOLERANCE  1.0e-3
{
    Real   tx, ty, tz;

    for_less( i, 0, 28 )
    {
        general_transform_point( &transform,
                                 tags1[i][X], tags1[i][Y], tags1[i][Z],
                                 &tx, &ty, &tz );

        if( !numerically_close( tags2[i][X], tx, TOLERANCE ) ||
            !numerically_close( tags2[i][Y], ty, TOLERANCE ) ||
            !numerically_close( tags2[i][Z], tz, TOLERANCE ) )
        {
            print( "%g %g %g   !=   %g %g %g\n",
                   tags2[i][X], tags2[i][Y], tags2[i][Z],
                   tx, ty, tz );
        }
    }
}
#endif

    return( 0 );
}

private  void  get_random_unit_vector(
    Vector   *v )
{
    Real  x, y, z, len_squared, len;

    do
    {
        x = get_random_0_to_1();
        y = get_random_0_to_1();
        z = get_random_0_to_1();
        len_squared = x * x + y * y + z * z;
    } while( len_squared <= 1.0 && len_squared != 0.0 );

    len = sqrt( len_squared );

    fill_Vector( *v, x / len, y / len, z / len );
}
