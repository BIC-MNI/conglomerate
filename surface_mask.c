#include  <mni.h>

typedef  enum  { LESS, GREATER } Sign_types;

private  void  mask_volume(
    Volume            volume,
    polygons_struct   *polygons,
    Sign_types        sign,
    Real              distance,
    Real              value_to_set );

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *input_volume_filename, *input_surface_filename;
    char                 *output_volume_filename, *sign_str;
    Real                 value_to_set;
    STRING               history;
    File_formats         format;
    Sign_types           sign;
    Volume               volume;
    Real                 distance;
    int                  n_objects;
    object_struct        **objects;
    polygons_struct      *polygons;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_volume_filename ) ||
        !get_string_argument( "", &input_surface_filename ) ||
        !get_string_argument( "", &output_volume_filename ) ||
        !get_string_argument( "", &sign_str ) ||
        !get_real_argument( 0.0, &distance ) )
    {
        print( "Usage: %s  in_volume  in_surface  out_volume  <|>  dist\n",
               argv[0] );
        return( 1 );
    }

    (void) get_real_argument( 0.0, &value_to_set );

    status = OK;

    if( strcmp( sign_str, "<" ) == 0 )
        sign = LESS;
    else if( strcmp( sign_str, ">" ) == 0 )
        sign = GREATER;
    else
    {
        print( "Usage: %s  in_volume  in_surface  out_volume  <|>  dist\n",
               argv[0] );
        return( 1 );
    }

    status = input_volume( input_volume_filename, 3, XYZ_dimension_names,
                           NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                           TRUE, &volume, (minc_input_options *) NULL );

    if( status == OK )
        status = input_graphics_file( input_surface_filename,
                                      &format, &n_objects, &objects );

    if( status == OK && n_objects < 1 || objects[0]->object_type != POLYGONS )
    {
        print( "First object in %s is not polygons.\n", input_surface_filename);
        status = ERROR;
    }

    if( status == OK )
    {
        polygons = get_polygons_ptr(objects[0]);

        create_polygons_bintree( polygons, polygons->n_items * 2 );

        mask_volume( volume, polygons, sign, distance, value_to_set );

        delete_object_list( n_objects, objects );

        (void) strcpy( history, "Surface masked." );

        status = output_volume( output_volume_filename, NC_UNSPECIFIED, FALSE,
                                0.0, 0.0, volume, history,
                                (minc_output_options *) NULL );
    }

    return( status != OK );
}

typedef  struct
{
    int        n_intersections;
    short      *intersections;
} intersection_struct;

private  void  create_intersections(
    Point             *ray_origin,
    Vector            *ray_direction,
    polygons_struct   *polygons,
    Sign_types        sign,
    Real              distance,
    intersection_struct  *intersect )
{
    int   n_intersections;

    n_intersections = intersect_ray_with_bintree( ray_origin, ray_direction,
                                polygons->bintree, polygons,
                                (int *) 0,
}

private  BOOLEAN  should_zero(
    intersection_struct  *intersect,
    int                  pos )
{
    int   i;

    while( i < intersect->n_intersections &&
           pos >= intersect->intersections[i] )
    {
        ++i;
    }

    return( i % 1 == 0 );
}

private  void  mask_volume(
    Volume            volume,
    polygons_struct   *polygons,
    Sign_types        sign,
    Real              distance,
    Real              value_to_set )
{
    intersection_struct     **intersections[N_DIMENSIONS];
    int                     c, a1, a2, i, j, sizes[N_DIMENSIONS];
    int                     x, y, z, vox[N_DIMENSIONS];
    Real                    voxel[N_DIMENSIONS], xw, yw, zw;
    Point                   ray_origin, ray_dest;
    Vector                  ray_direction;

    get_volume_sizes( volume, sizes );

    for_less( c, 0, N_DIMENSIONS )
    {
        a1 = (c + 1) % N_DIMENSIONS;
        a2 = (c + 2) % N_DIMENSIONS;

        ALLOC2D( intersections[c], sizes[a1], sizes[a2] );

        for_less( i, 0, sizes[a1] )
        {
            for_less( j, 0, sizes[a2] )
            {
                voxel[c] = -3.0 * sizes[c];
                voxel[a1] = (Real) i;
                voxel[a2] = (Real) j;
                convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
                fill_Point( ray_origin, xw, yw, zw );

                voxel[c] = 3.0 * sizes[c];
                convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
                fill_Point( ray_dest, xw, yw, zw );

                SUB_POINTS( ray_direction, ray_dest, ray_origin );
                NORMALIZE_VECTOR( ray_direction, ray_direction );

                create_intersections( &ray_origin, &ray_direction,
                                      polygons, sign, distance,
                                      &intersections[c][i][j] );
            }
        }
    }

    for_less( vox[X], 0, sizes[X] )
    {
        for_less( vox[Y], 0, sizes[Y] )
        {
            for_less( vox[Z], 0, sizes[Z] )
            {
                for_less( c, 0, N_DIMENSIONS )
                {
                    a1 = (c + 1) % N_DIMENSIONS;
                    a2 = (c + 2) % N_DIMENSIONS;

                    if( !should_zero( &intersections[c][vox[a1]][vox[a2]],
                                      vox[c] ) )
                        break;
                }

                if( c == N_DIMENSIONS )
                    SET_VOXEL_3D( volume, vox[X], vox[Y], vox[Z], value_to_set);
            }
        }
    }
}
