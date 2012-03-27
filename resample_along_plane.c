#include  <volume_io.h>
#include  <bicpl.h>

private  void    find_plane(
    Point      points[],
    Vector     *normal );

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               three_tags_filename, output_filename;
    STRING               volume_filename;
    Point                points[3], origin, point, centroid;
    Vector               normal, plane_normal;
    object_struct        *object, **object_list;
    Vector               princ_normal, normals[4];
    int                  i, n_tag_points, n_volumes, *patient_ids, *structure_ids;
    Real                 **tags1, **tags2, *weights;
    STRING               *labels;
    Real                 plane_constants[4], dir[3], separations[3];
    Real                 voxel[3], xyz[3];
    Volume               volume;
    Vector               x_axis, y_axis, z_axis;
    char                 command[10000];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &three_tags_filename ) ||
        !get_string_argument( NULL, &volume_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print( "Usage:  %s  3_tags_file.tag  input.mnc output.xfm  output.mnc\n" );
        return( 1 );
    }

    if( input_volume_header_only( volume_filename, 3, File_order_dimension_names,
                                  &volume, NULL ) != OK )
        return( 1 );

    if( input_tag_file( three_tags_filename, &n_volumes, &n_tag_points,
                        &tags1, &tags2, &weights, &structure_ids,
                        &patient_ids, &labels ) != OK )
        return( 1 );

    if( n_volumes != 1 || n_tag_points != 3 )
    {
        print( "First file must contain exactly 3 tags.\n" );
        return( 1 );
    }

    for_less( i, 0, 3 )
        fill_Point( points[i], tags1[i][X], tags1[i][Y], tags1[i][Z] );

    find_plane( points, &x_axis );

    if( RPoint_x(x_axis) < 0.0 )
    {
        SCALE_VECTOR( x_axis, x_axis, -1.0 );
    }

    voxel[0] = 0.0;
    voxel[1] = 1.0;
    voxel[2] = 2.0;

    reorder_voxel_to_xyz( volume, voxel, xyz );

    get_volume_direction_cosine( volume, (int) xyz[Y], dir );

    fill_Vector( y_axis, dir[0], dir[1], dir[2] );
    CROSS_VECTORS( z_axis, x_axis, y_axis );

    if( null_Vector( &z_axis ) )
    {
        create_orthogonal_vector( &x_axis, &y_axis );
        CROSS_VECTORS( z_axis, x_axis, y_axis );
    }

    CROSS_VECTORS( y_axis, z_axis, x_axis );

    NORMALIZE_VECTOR( x_axis, x_axis );
    NORMALIZE_VECTOR( y_axis, y_axis );
    NORMALIZE_VECTOR( z_axis, z_axis );

    /*----------- now for translation */

    get_volume_separations( volume, separations );

    (void) sprintf( command, "mincresample -trilinear -xdircos %g %g %g -ydircos %g %g %g \
-zdircos %g %g %g %s %s",
           RPoint_x(x_axis), RPoint_y(x_axis), RPoint_z(x_axis),
           RPoint_x(y_axis), RPoint_y(y_axis), RPoint_z(y_axis),
           RPoint_x(z_axis), RPoint_y(z_axis), RPoint_z(z_axis), volume_filename,
           output_filename );

    print( "%s\n", command );

    system( command );

    return( 0 );
}

private  void    find_plane(
    Point      points[],
    Vector     *normal )
{
    Vector   v1, v2;

    SUB_POINTS( v1, points[1], points[0] );
    SUB_POINTS( v2, points[2], points[0] );
    CROSS_VECTORS( *normal, v1, v2 );
}
