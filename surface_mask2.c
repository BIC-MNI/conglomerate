#include  <internal_volume_io.h>
#include  <bicpl.h>

#define  BINTREE_FACTOR   0.1

#define  OFFSET   1000.0

private  BOOLEAN  get_extrapolated_value(
    Volume         volume,
    int            x,
    int            y,
    int            z,
    object_struct  *surface,
    Real           threshold,
    Real           *value_to_set );

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_volume_filename, input_surface_filename;
    STRING               output_volume_filename;
    Real                 set_value, separations[MAX_DIMENSIONS];
    Real                 min_value, max_value, value;
    Real                 xw, yw, zw, threshold, value_to_set;
    Point                origin, dest;
    Real                 min_real_value, max_real_value;
    Vector               direction;
    STRING               history;
    File_formats         format;
    Volume               volume;
    int                  x, y, z, n_objects;
    int                  sizes[MAX_DIMENSIONS];
    int                  obj_index;
    int                  i, j, best, n_intersections, ind;
    Real                 dist, *distances, tmp, voxel[MAX_DIMENSIONS];
    object_struct        **objects;
    BOOLEAN              set_value_specified, inside, erase_flag;
    BOOLEAN              threshold_specified;
    progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_volume_filename ) ||
        !get_string_argument( NULL, &input_surface_filename ) ||
        !get_string_argument( NULL, &output_volume_filename ) )
    {
        print_error(
            "Usage: %s  in_volume.mnc  in_surface.obj  out_volume.mnc\n",
               argv[0] );
        print_error( "      [min_value max_value] [set_value] [threshold]\n" );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &min_value );
    (void) get_real_argument( min_value - 1.0, &max_value );
    set_value_specified = get_real_argument( 0.0, &set_value );
    threshold_specified = get_real_argument( 0.0, &threshold );

    if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, NULL ) != OK )
        return( 1 );

    if( input_graphics_file( input_surface_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    if( n_objects < 1 )
    {
        print( "No objects in %s.\n", input_surface_filename);
        return( 1 );
    }

    get_volume_separations( volume, separations );
    get_volume_sizes( volume, sizes );

    min_real_value = get_volume_real_min( volume );
    max_real_value = get_volume_real_max( volume );

    if( !set_value_specified )
        set_value = min_real_value;
    else if( set_value < min_real_value )
        set_value = min_real_value;
    else if( set_value > max_real_value )
        set_value = max_real_value;

    create_polygons_bintree( get_polygons_ptr(objects[0]),
                  ROUND( (Real) get_polygons_ptr(objects[0])->n_items *
                                                BINTREE_FACTOR ) );

    initialize_progress_report( &progress, FALSE, sizes[X] * sizes[Y],
                                "Masking" );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            voxel[X] = (Real) x;
            voxel[Y] = (Real) y;
            voxel[Z] = -OFFSET;

            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            fill_Point( origin, xw, yw, zw );

            voxel[Z] = (Real) sizes[Z] + OFFSET;
            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            fill_Point( dest, xw, yw, zw );
            SUB_POINTS( direction, dest, origin );
            NORMALIZE_VECTOR( direction, direction );

            n_intersections = intersect_ray_with_object( &origin, &direction,
                                       objects[0], &obj_index, &dist,
                                       &distances );

            for_less( i, 0, n_intersections )
                distances[i] = distances[i] / FABS( separations[Z] ) - OFFSET;

            for_less( i, 0, n_intersections-1 )
            {
                best = i;
                for_less( j, i+1, n_intersections )
                {
                    if( distances[j] < distances[best] )
                        best = j;
                }

                tmp = distances[i];
                distances[i] = distances[best];
                distances[best] = tmp;
            }

            if( n_intersections % 2 == 0 )
            {
                ind = 0;
                inside = FALSE;
                for_less( z, 0, sizes[Z] )
                {
                    while( ind <= n_intersections-1 &&
                           (Real) z >= distances[ind] )
                    {
                        inside = !inside;
                        ++ind;
                    }

                    erase_flag = FALSE;
                    if( !inside )
                    {
                        if( min_value <= max_value )
                        {
                            value = get_volume_real_value( volume, x, y, z,0,0);
                            erase_flag = min_value <= value &&
                                         value <= max_value;
                        }
                        else
                            erase_flag = TRUE;
                    }

                    if( erase_flag )
                    {
                        if( !threshold_specified ||
                            !get_extrapolated_value( volume, x, y, z,
                                        objects[0], threshold, &value_to_set ) )
                        {
                            value_to_set = set_value;
                        }
                        else if( value_to_set < min_real_value )
                            value_to_set = min_real_value;
                        else if( value_to_set > max_real_value )
                            value_to_set = max_real_value;

                        set_volume_real_value( volume, x, y, z, 0, 0,
                                               value_to_set );
                    }
                }
            }
            else
                print( "N intersections: %d\n", n_intersections );

            if( n_intersections > 0 )
                FREE( distances );

            update_progress_report( &progress, x * sizes[Y] + y + 1 );
        }
    }

    terminate_progress_report( &progress );
    
    delete_object_list( n_objects, objects );

    history = create_string( "Surface masked.\n" );

    (void) output_volume( output_volume_filename, NC_UNSPECIFIED,
                            FALSE, 0.0, 0.0, volume, history,
                            NULL );

    delete_string( history );

    delete_volume( volume );

    return( 0 );
}

private  BOOLEAN  get_one_extrapolated_value(
    Volume         volume,
    int            x1,
    int            y1,
    int            z1,
    int            x2,
    int            y2,
    int            z2,
    object_struct  *surface,
    Real           threshold,
    Real           *value_to_set )
{
    int      sizes[N_DIMENSIONS];
    int      obj_index, n_int, n_intersections, i;
    Real     value2, dist, *distances, xw, yw, zw;
    Real     voxel[N_DIMENSIONS];
    Point    p1, p2;
    Vector   direction;

    get_volume_sizes( volume, sizes );

    if( x2 < 0 || x1 >= sizes[X] ||
        y2 < 0 || y1 >= sizes[Y] ||
        z2 < 0 || z1 >= sizes[Z] )
        return( FALSE );

    voxel[X] = (Real) x1;
    voxel[Y] = (Real) y1;
    voxel[Z] = (Real) z1;
    convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
    fill_Point( p1, xw, yw, zw );

    voxel[X] = (Real) x2;
    voxel[Y] = (Real) y2;
    voxel[Z] = (Real) z2;
    convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
    fill_Point( p2, xw, yw, zw );
    SUB_POINTS( direction, p2, p1 );

    n_intersections = intersect_ray_with_object( &p1, &direction,
                                                 surface, &obj_index, &dist,
                                                 &distances );

    n_int = 0;
    for_less( i, 0, n_intersections )
    {
        if( distances[i] > 0.0 && distances[i] < 1.0 )
        {
            ++n_int;
            dist = distances[i];
        }
    }

    if( n_intersections > 0 )
        FREE( distances );

    if( n_int != 1 )
        return( FALSE );

    value2 = get_volume_real_value( volume, x2, y2, z2, 0, 0 );

    if( value2 <= threshold )
        return( FALSE );

    *value_to_set = (threshold - value2 * dist) / (1.0 - dist);

    return( TRUE );
}

private  BOOLEAN  get_extrapolated_value(
    Volume         volume,
    int            x,
    int            y,
    int            z,
    object_struct  *surface,
    Real           threshold,
    Real           *value_to_set )
{
    Real     value;
    int      c, dir, n_bounds, voxel[N_DIMENSIONS];

    *value_to_set = 0.0;
    n_bounds = 0;
    for_less( c, 0, N_DIMENSIONS )
    {
        for( dir = -1;  dir <= 1;  dir += 2 )
        {
            voxel[X] = x;
            voxel[Y] = y;
            voxel[Z] = z;
            voxel[c] += dir;
            if( get_one_extrapolated_value( volume, x, y, z,
                                            voxel[X], voxel[Y], voxel[Z],
                                            surface, threshold, &value ) )
            {
                *value_to_set += value;
                ++n_bounds;
            }
        }
    }

    if( n_bounds > 0 )
        *value_to_set /= (Real) n_bounds;

    return( n_bounds > 0 );
}
