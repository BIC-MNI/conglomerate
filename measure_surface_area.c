#include  <internal_volume_io.h>
#include  <bicpl.h>

private  Real  get_area_of_values(
    polygons_struct   *polygons,
    Real              values[],
    Real              low,
    Real              high,
    BOOLEAN           clip_flag,
    int               clip_axis,
    Real              clip_sign,
    Real              plane_pos );

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
    STRING               axis_name, sign_name;
    Real                 plane_pos, clip_sign;
    int                  clip_axis;
    BOOLEAN              clip_flag;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) )
    {
        print_error( "Usage: %s  src.obj values_file  low high [x|y|z +|- pos]\n",
                     argv[0] );
        return( 1 );
    }

    (void) get_string_argument( NULL, &values_filename );
    (void) get_real_argument( 0.0, &low );
    (void) get_real_argument( 0.0, &high );

    if( get_string_argument( NULL, &axis_name ) &&
        axis_name[0] >= 'x' && axis_name[0] <= 'z' &&
        get_string_argument( NULL, &sign_name ) &&
        (sign_name[0] == '-' || sign_name[0] == '+') &&
        get_real_argument( 0.0, &plane_pos ) )
    {
        clip_flag = TRUE;
        if( sign_name[0] == '-' )
            clip_sign = -1.0;
        else
            clip_sign = 1.0;
        clip_axis = (int) (axis_name[0] - 'x');
    }
    else
        clip_flag = FALSE;

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    if( n_objects != 1 || get_object_type(object_list[0]) != POLYGONS )
    {
        print_error( "Must contain exactly one polygons object.\n" );
        return( 1 );
    }

    polygons = get_polygons_ptr( object_list[0] );
    n_points = polygons->n_points;

    if( values_filename != NULL )
    {
        if( open_file( values_filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
            return( 1 );

        ALLOC( values, n_points );

        for_less( p, 0, n_points )
        {
            if( input_real( file, &values[p] ) != OK )
            {
                print_error( "Could not read %d'th value from file.\n", p );
                return( 1 );
            }
        }

        close_file( file );
    }
    else
        values = NULL;

    area = get_area_of_values( polygons, values, low, high,
                               clip_flag, clip_axis, clip_sign, plane_pos );

    print( "Area: %g\n", area );

    return( 0 );
}

private  Real  get_area_of_values(
    polygons_struct   *polygons,
    Real              values[],
    Real              low,
    Real              high,
    BOOLEAN           clip_flag,
    int               clip_axis,
    Real              clip_sign,
    Real              plane_pos )
{
    int    n_points, i1, i2, size;
    int    poly, v, indices[MAX_POINTS_PER_POLYGON], ind;
    Point  points[2][1000];
    Real   area, alpha, clip, prev_clip;

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
            if( values == NULL || low <= values[i1] && values[i1] <= high )
            {
                points[0][n_points] = polygons->points[i1];
                ++n_points;
            }

            i2 = indices[(v+1)%size];

            if( values != NULL &&
                (values[i1] <= low && values[i2] >= low && values[i2] <= high ||
                values[i2] <= low && values[i1] >= low && values[i1] <= high ||
                values[i1] <= high && values[i2] >= high && values[i1] >= low ||
                values[i2] <= high && values[i1] >= high && values[i2] >= low) )
            {
                if( values[i1] <= low && values[i2] >= low ||
                    values[i2] <= low && values[i1] >= low )
                    alpha = (low - values[i1]) / (values[i2] - values[i1]);
                else
                    alpha = (high - values[i1]) / (values[i2] - values[i1]);

                if( alpha > 0.0 && alpha < 1.0 )
                {
                    INTERPOLATE_POINTS( points[0][n_points],
                                        polygons->points[i1],
                                        polygons->points[i2], alpha );
                    ++n_points;
                }
            }
        }

        if( clip_flag && n_points > 2 )
        {
            size = n_points;

            n_points = 0;
            clip = clip_sign *
                   (RPoint_coord(points[0][size-1],clip_axis) - plane_pos);

            for_less( v, 0, size )
            {
                prev_clip = clip;
                clip = clip_sign *
                          (RPoint_coord(points[0][v],clip_axis) - plane_pos);

                if( clip * prev_clip < 0.0 )
                {
                    alpha = prev_clip / (prev_clip - clip);
                    if( alpha > 0.0 && alpha < 1.0 )
                    {
                        INTERPOLATE_POINTS( points[1][n_points],
                                            points[0][(v-1+size)%size],
                                            points[0][v], alpha );
                        ++n_points;
                    }
                }

                if( clip >= 0.0 )
                {
                    points[1][n_points] = points[0][v];
                    ++n_points;
                }
            }

            ind = 1;
        }
        else
            ind = 0;

        if( n_points >= 3 )
            area += get_polygon_surface_area( n_points, points[ind] );
    }

    return( area );
}
