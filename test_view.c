#include  <volume_io.h>
#include  <bicpl.h>

private  Real  x_scale = 1.0;
private  Real  y_scale = 1.0;
private  Real  x_trans = 0.0;
private  Real  y_trans = 0.0;

private  Real  current_x_axis[MAX_DIMENSIONS];
private  Real  current_y_axis[MAX_DIMENSIONS];
private  Real  current_voxel[MAX_DIMENSIONS];
private   Volume     volume;

private  void  check_mapping(
    Real            x_start,
    Real            y_start,
    Real            voxel[] );
private  void  set_slice_plane_perp_axis(
    Real      voxel_perp[] );
private  void  convert_voxel_to_pixel(
    Real              voxel[],
    Real              *x_pixel,
    Real              *y_pixel );
private  void  get_slice_plane(
    Real             origin[],
    Real             x_axis[],
    Real             y_axis[] );

int  main(
    int   argc,
    char  *argv[] )
{
    char       *input_filename;
    Real       perp[MAX_DIMENSIONS];
    Real       x_axis[MAX_DIMENSIONS];
    Real       y_axis[MAX_DIMENSIONS];
    Real       origin[MAX_DIMENSIONS];
    Real       x, y;
    int        sizes[MAX_DIMENSIONS];

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) )
    {
        return( 1 );
    }

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    perp[0] = 1.0;
    perp[1] = 1.0;
    perp[2] = 1.0;

    get_volume_sizes( volume, sizes );

    current_voxel[0] = (sizes[0] - 1) / 2.0;
    current_voxel[1] = (sizes[1] - 1) / 2.0;
    current_voxel[2] = (sizes[2] - 1) / 2.0;

    volume->separations[0] = 1.0;
    volume->separations[1] = 5.0;
    volume->separations[2] = 10.0;

    current_voxel[0] = 10.0;
    current_voxel[1] = 10.0;
    current_voxel[2] = 10.0;

    set_slice_plane_perp_axis( perp );

    get_slice_plane( origin, x_axis, y_axis );

    current_voxel[0] += 100.0 * x_axis[0];
    current_voxel[1] += 100.0 * x_axis[1];
    current_voxel[2] += 100.0 * x_axis[2];

    convert_voxel_to_pixel( current_voxel, &x, &y );

    return( 0 );
}

private  BOOLEAN  convert_pixel_to_voxel(
    int               x_pixel,
    int               y_pixel,
    Real              voxel[] )
{
    BOOLEAN           found;
    Real              origin[MAX_DIMENSIONS];
    Real              x_axis[MAX_DIMENSIONS], y_axis[MAX_DIMENSIONS];

    found = FALSE;

    get_slice_plane( origin, x_axis, y_axis );

    found = convert_slice_pixel_to_voxel( volume, x_pixel, y_pixel,
         origin, x_axis, y_axis,
         x_trans, y_trans, x_scale, y_scale, voxel );

    return( found );
}

private  void  convert_voxel_to_pixel(
    Real              voxel[],
    Real              *x_pixel,
    Real              *y_pixel )
{
    Real              x_real_pixel, y_real_pixel;
    Real              origin[MAX_DIMENSIONS];
    Real              x_axis[MAX_DIMENSIONS], y_axis[MAX_DIMENSIONS];

    get_slice_plane( origin, x_axis, y_axis );

    convert_voxel_to_slice_pixel( volume, voxel, origin, x_axis, y_axis,
          x_trans, y_trans, x_scale, y_scale, &x_real_pixel, &y_real_pixel );
    check_mapping( x_real_pixel, y_real_pixel, voxel );

    *x_pixel = x_real_pixel;
    *y_pixel = y_real_pixel;
}

private  void  get_voxel_axis_perpendicular(
    Real     x_axis[],
    Real     y_axis[],
    Real     perp_axis[] )
{
    int      c, a1, a2;
    Real     len, separations[MAX_DIMENSIONS];

    get_volume_separations( volume, separations );

    len = 0.0;
    for_less( c, 0, N_DIMENSIONS )
    {
        a1 = (c + 1) % N_DIMENSIONS;
        a2 = (c + 2) % N_DIMENSIONS;
        perp_axis[c] = x_axis[a1] * y_axis[a2] - x_axis[a2] * y_axis[a1];

        perp_axis[c] *= ABS( separations[a1] * separations[a2] /
                             separations[c] );

        len += perp_axis[c] * perp_axis[c];
    }

    if( len != 0.0 )
    {
        len = sqrt( len );
        for_less( c, 0, N_DIMENSIONS )
            perp_axis[c] /= len;
    }
}

private  void  get_slice_perp_axis(
    Real             perp_axis[N_DIMENSIONS] )
{
    get_voxel_axis_perpendicular( current_x_axis, current_y_axis, perp_axis );
}

private  void  set_slice_plane_perp_axis(
    Real      voxel_perp[] )
{
    Real     max_value, len, sign;
    Real     len_x_axis, len_y_axis, factor;
    Real     used_x_axis[MAX_DIMENSIONS];
    Real     used_y_axis[MAX_DIMENSIONS];
    Real     separations[MAX_DIMENSIONS];
    Real     perp[MAX_DIMENSIONS];
    int      x_index, y_index;
    int      c, max_axis;

    get_volume_separations( volume, separations );

    for_less( c, 0, N_DIMENSIONS )
        separations[c] = ABS( separations[c] );

    for_less( c, 0, N_DIMENSIONS )
        perp[c] = voxel_perp[c] * separations[c];

    max_value = 0.0;
    for_less( c, 0, N_DIMENSIONS )
    {
        if( c == 0 || ABS(perp[c]) > max_value )
        {
            max_value = ABS(perp[c]);
            max_axis = c;
        }
    }

    switch( max_axis )
    {
    case X: x_index = Y;   y_index = Z;  break;
    case Y: x_index = X;   y_index = Z;  break;
    case Z: x_index = X;   y_index = Y;  break;
    }

    used_x_axis[X] = 0.0;
    used_x_axis[Y] = 0.0;
    used_x_axis[Z] = 0.0;
    used_x_axis[x_index] = 1.0;

    len = perp[X] * perp[X] + perp[Y] * perp[Y] + perp[Z] * perp[Z];
    if( len == 0.0 )
        return;

    factor = used_x_axis[x_index] * perp[x_index] / len;

    for_less( c, 0, N_DIMENSIONS )
        used_x_axis[c] -= factor * perp[c];

    used_y_axis[X] = perp[Y] * used_x_axis[Z] - used_x_axis[Y] * perp[Z];
    used_y_axis[Y] = perp[Z] * used_x_axis[X] - used_x_axis[Z] * perp[X];
    used_y_axis[Z] = perp[X] * used_x_axis[Y] - used_x_axis[X] * perp[Y];

    len_x_axis = 0.0;
    len_y_axis = 0.0;
    for_less( c, 0, N_DIMENSIONS )
    {
        used_x_axis[c] /= separations[c];
        used_y_axis[c] /= separations[c];
        len_x_axis += used_x_axis[c] * used_x_axis[c];
        len_y_axis += used_y_axis[c] * used_y_axis[c];
    }

    if( len_x_axis == 0.0 || len_y_axis == 0.0 )
        return;

    len_x_axis = sqrt( len_x_axis );
    len_y_axis = sqrt( len_y_axis );

    if( used_y_axis[y_index] < 0.0 )
        sign = -1.0;
    else
        sign = 1.0;

    for_less( c, 0, N_DIMENSIONS )
    {
        used_x_axis[c] /= len_x_axis;
        used_y_axis[c] /= sign * len_y_axis;
    }

    for_less( c, 0, N_DIMENSIONS )
    {
        current_x_axis[c] = used_x_axis[c];
        current_y_axis[c] = used_y_axis[c];
    }
}

private  void  check_mapping(
    Real            x_start,
    Real            y_start,
    Real            voxel[] )
{
    Real   test_voxel[MAX_DIMENSIONS];
    Real   origin[MAX_DIMENSIONS];
    Real   x_axis[MAX_DIMENSIONS], y_axis[MAX_DIMENSIONS];

    get_slice_plane( origin, x_axis, y_axis );

    (void)  convert_slice_pixel_to_voxel( volume, x_start, y_start,
         origin, x_axis, y_axis,
         x_trans, y_trans, x_scale, y_scale, test_voxel );

    if( !numerically_close( voxel[0], test_voxel[0], 1.0e-2 ) ||
        !numerically_close( voxel[1], test_voxel[1], 1.0e-2 ) ||
        !numerically_close( voxel[2], test_voxel[2], 1.0e-2 ) )
        handle_internal_error( "Dang it all." );
}

private  void  get_slice_plane(
    Real             origin[],
    Real             x_axis[],
    Real             y_axis[] )
{
    int    a1, a2, c;
    Real   separations[MAX_DIMENSIONS];
    Real   perp_axis[MAX_DIMENSIONS];
    Real   voxel_dot_perp, perp_dot_perp, factor;

    get_volume_separations( volume, separations );

    get_slice_perp_axis( perp_axis );

    for_less( c, 0, N_DIMENSIONS )
    {
        separations[c] = ABS( separations[c] );
        x_axis[c] = current_x_axis[c];
        y_axis[c] = current_y_axis[c];
    }

    for_less( c, 0, N_DIMENSIONS )
    {
        a1 = (c + 1) % N_DIMENSIONS;
        a2 = (c + 2) % N_DIMENSIONS;
        perp_axis[c] = x_axis[a1] * y_axis[a2] - x_axis[a2] * y_axis[a1];
    }

    voxel_dot_perp = 0.0;
    for_less( c, 0, N_DIMENSIONS )
        voxel_dot_perp += current_voxel[c] * perp_axis[c];

    perp_dot_perp = 0.0;
    for_less( c, 0, N_DIMENSIONS )
       perp_dot_perp += perp_axis[c] * perp_axis[c];

    if( perp_dot_perp == 0.0 )
    {
        for_less( c, 0, N_DIMENSIONS )
            origin[c] = 0.0;
    }
    else
    {
        factor = voxel_dot_perp / perp_dot_perp;
        for_less( c, 0, N_DIMENSIONS )
            origin[c] = factor * perp_axis[c];
    }
}
