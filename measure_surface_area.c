#include  <internal_volume_io.h>
#include  <bicpl.h>

private  Real  get_area_of_values(
    polygons_struct   *polygons,
    Real              values[],
    Real              low,
    Real              high );

int  main(
    int    argc,
    char   *argv[] )
{
    FILE                 *file;
    Real                 *values, low, high, area;
    STRING               src_filename, values_filename;
    int                  p, n_objects, n_points;
    File_formats         format;
    object_struct        **object_list;
    polygons_struct      *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &values_filename ) ||
        !get_real_argument( 0.0, &low ) ||
        !get_real_argument( 0.0, &high ) )
    {
        print_error( "Usage: %s  src.obj values_file  low high\n",
                     argv[0] );
        return( 1 );
    }

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    if( n_objects != 1 || get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Must contain exactly one polygons object.\n" );
        return( 1 );
    }

    if( open_file( values_filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    polygons = get_polygons_ptr( object_list[0] );
    n_points = polygons->n_points;
    ALLOC( values, n_points );

    for_less( p, 0, n_points )
    {
        if( input_real( file, &values[p] ) != OK )
        {
            print_error( "Could not read %d'th value from file.\n", p );
            return( 1 );
        }
    }

    area = get_area_of_values( polygons, values, low, high );

    print( "%g\n", area );

    close_file( file );

    return( 0 );
}

private  Real  get_area_of_values(
    polygons_struct   *polygons,
    Real              values[],
    Real              low,
    Real              high )
{
    int    n_points, i1, i2, size;
    int    poly, v, indices[MAX_POINTS_PER_POLYGON];
    Point  points[10000];
    Real   area, alpha;

    area = 0.0;

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( v, 0, size )
        {
            indices[v] = polygons->indices[
                           POINT_INDEX(polygons->end_indices,poly,v)];
        }

        n_points = 0;

        for_less( v, 0, size )
        {
            i1 = indices[v];
            if( low <= values[i1] && values[i1] <= high )
            {
                points[n_points] = polygons->points[i1];
                ++n_points;
            }

            i2 = indices[(v+1)%size];

            if( values[i1] * values[i2] < 0.0 )
            {
                alpha = values[i1] / (values[i1] - values[i2]);
                if( alpha > 0.0 && alpha < 1.0 )
                {
                    INTERPOLATE_POINTS( points[n_points], polygons->points[i1],
                                        polygons->points[i2], alpha );
                    ++n_points;
                }
            }
        }

        if( n_points == 1 || n_points == 2 )
        {
            print_error( "get_area_of_values: n_points %d\n", n_points );
        }

        if( n_points >= 3 )
            area += get_polygon_surface_area( n_points, points );
    }

    return( area );
}
