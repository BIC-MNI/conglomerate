/* ----------------------------------------------------------------------------
@COPYRIGHT  :
              Copyright 1993,1994,1995 David MacDonald,
              McConnell Brain Imaging Centre,
              Montreal Neurological Institute, McGill University.
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
---------------------------------------------------------------------------- */

#include  <volume_io/internal_volume_io.h>
#include  <vols.h>

#ifndef lint
static char rcsid[] = "$Header: /private-cvsroot/libraries/conglomerate/labels.c,v 1.2 2001-10-22 16:56:19 stever Exp $";
#endif

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_label_volume_real_range
@INPUT      : volume
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets the real range to be equal to the voxel range.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_label_volume_real_range(
    Volume  volume )
{
    if( get_volume_data_type(volume) != FLOAT &&
        get_volume_data_type(volume) != DOUBLE )
    {
        set_volume_real_range( volume,
                               get_volume_voxel_min(volume),
                               get_volume_voxel_max(volume) );
    }
    else
        volume->real_range_set = FALSE;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : create_label_volume
@INPUT      : volume
              type
@OUTPUT     : 
@RETURNS    : a volume
@DESCRIPTION: Creates a label volume with the same tessellation as the volume.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  Volume  create_label_volume(
    Volume  volume,
    nc_type type )
{
    Volume   label_volume;

    if( type == NC_UNSPECIFIED )
        type = NC_BYTE;

    label_volume = copy_volume_definition_no_alloc( volume, type, FALSE,
                                                    0.0, -1.0 );

    set_label_volume_real_range( label_volume );

    return( label_volume );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : check_alloc_label_data
@INPUT      : volume
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Checks if the label data has been allocated.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  void  check_alloc_label_data(
    Volume  volume )
{
    if( !volume_is_alloced( volume ) && !volume_is_cached(volume) )
    {
        alloc_volume_data( volume );
        set_all_volume_label_data( volume, 0 );
    }
}

public  BOOLEAN  is_label_volume_initialized(
    Volume  volume )
{
    return( volume != NULL && 
            ((volume->is_cached_volume &&
             cached_volume_has_been_modified( &volume->cache )) ||
             (!volume->is_cached_volume &&
              multidim_array_is_alloced( &volume->array ))) );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_all_volume_label_data
@INPUT      : volume
              value
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets the label value of all voxels to the value specified.
@CREATED    : Mar   1993           David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_all_volume_label_data(
    Volume    volume,
    int       value )
{
    Data_types      type;
    void            *ptr;
    Real            real_value;
    int             v0, v1, v2, v3, v4;
    unsigned int    n_voxels;

    check_alloc_label_data( volume );

    type = get_volume_data_type( volume );
    if( !volume->is_cached_volume && value == 0 &&
        type != FLOAT && type != DOUBLE )
    {
        GET_VOXEL_PTR( ptr, volume, 0, 0, 0, 0, 0 );
        n_voxels = get_volume_total_n_voxels( volume );
        (void)memset( ptr, 0, (size_t) n_voxels * (size_t) get_type_size(type));
    }
    else
    {
        real_value = (Real) value;
        BEGIN_ALL_VOXELS( volume, v0, v1, v2, v3, v4 )

            set_volume_real_value( volume, v0, v1, v2, v3, v4, real_value );

        END_ALL_VOXELS
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_volume_label_data
@INPUT      : volume
              x
              y
              z
              value
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets the label data of the given voxel to the value.
@CREATED    : Mar   1993           David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_volume_label_data_5d(
    Volume          volume,
    int             v0,
    int             v1,
    int             v2,
    int             v3,
    int             v4,
    int             value )
{
    check_alloc_label_data( volume );

    set_volume_real_value( volume, v0, v1, v2, v3, v4, (Real) value );
}

public  void  set_volume_label_data(
    Volume          volume,
    int             voxel[],
    int             value )
{
    set_volume_label_data_5d( volume,
                              voxel[0], voxel[1], voxel[2], voxel[3], voxel[4],
                              value );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_volume_label_data
@INPUT      : volume
              x
              y
              z
@OUTPUT     : 
@RETURNS    : value
@DESCRIPTION: Returns the label data of the given voxel.
@CREATED    : Mar   1993           David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  int  get_volume_label_data(
    Volume          volume,
    int             voxel[] )
{
    return( get_volume_label_data_5d( volume,
                       voxel[0], voxel[1], voxel[2], voxel[3], voxel[4] ) );
}

public  int  get_volume_label_data_5d(
    Volume          volume,
    int             v0,
    int             v1,
    int             v2,
    int             v3,
    int             v4 )
{
    int    label;

    if( volume == (Volume) NULL || !volume_is_alloced( volume ) )
        return( 0 );
    else
    {
        label = (int) get_volume_real_value( volume, v0, v1, v2, v3, v4 );
        return( label );
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_3D_volume_label_data
@INPUT      : volume
              x
              y
              z
@OUTPUT     : 
@RETURNS    : label
@DESCRIPTION: Gets the label value of a 3D label volume.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  int  get_3D_volume_label_data(
    Volume          volume,
    int             x,
    int             y,
    int             z )
{
    int    label;

    if( volume == (Volume) NULL || !volume_is_alloced( volume ) )
        return( 0 );
    else
    {
        label = (int) get_volume_real_value( volume, x, y, z, 0, 0 );
        return( label );
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_voxel_label_bit
@INPUT      : volume
              voxel
              bit
@OUTPUT     : 
@RETURNS    : TRUE or FALSE
@DESCRIPTION: Returns the label bit.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  BOOLEAN  get_voxel_label_bit(
    Volume          volume,
    int             voxel[],
    int             bit )
{
    return( (get_volume_label_data( volume, voxel ) & bit) == 0 );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_voxel_label_bit
@INPUT      : volume
              voxel
              bit
              value
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets the voxel label bit.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_voxel_label_bit(
    Volume          volume,
    int             voxel[],
    int             bit,
    BOOLEAN         value )
{
    int     i, n_dims, v[MAX_DIMENSIONS], label, anded, new_label;

    check_alloc_label_data( volume );

    n_dims = get_volume_n_dimensions(volume);

    for_less( i, 0, n_dims )
        v[i] = voxel[i];

    label = (int) get_volume_real_value( volume, v[0], v[1], v[2], v[3], v[4]);

    anded = (label & bit);

    if( value )
    {
        if( anded != bit )
        {
            new_label = label | bit;
            set_volume_real_value( volume, v[0], v[1], v[2], v[3], v[4],
                                    (Real) new_label );
        }
    }
    else if( anded != 0 )
    {
        new_label = label & (~bit);
        set_volume_real_value( volume, v[0], v[1], v[2], v[3], v[4],
                                (Real) new_label );
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : set_all_volume_label_data_bit
@INPUT      : volume
              bit
              value - ON or OFF
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Sets just the given bit of all the voxels' label data to the
              given value.
@CREATED    : Mar   1993           David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  set_all_volume_label_data_bit(
    Volume         volume,
    int            bit,
    BOOLEAN        value )
{
    int             v[MAX_DIMENSIONS];

    check_alloc_label_data( volume );

    BEGIN_ALL_VOXELS( volume, v[0], v[1], v[2], v[3], v[4] )

        set_voxel_label_bit( volume, v, bit, value );

    END_ALL_VOXELS
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_volume_voxel_activity
@INPUT      : volume
              voxel
              activity_if_mixed
@OUTPUT     : 
@RETURNS    : TRUE or FALSE
@DESCRIPTION: Returns the voxel activity, by looking at the 2^d corners of the
              voxel.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  BOOLEAN  get_volume_voxel_activity(
    Volume     volume,
    Real       voxel[],
    BOOLEAN    activity_if_mixed )
{
    BOOLEAN  active_found, inactive_found;
    int      c, int_index[MAX_DIMENSIONS], ind[MAX_DIMENSIONS];
    int      n[MAX_DIMENSIONS], sizes[MAX_DIMENSIONS];

    if( volume == (Volume) NULL || !volume_is_alloced( volume ) )
        return( TRUE );

    get_volume_sizes( volume, sizes );

    for_less( c, 0, get_volume_n_dimensions(volume) )
        if( voxel[c] < 0.0 || voxel[c] > (Real) sizes[c]-1.0 )
            return( FALSE );

    for_less( c, 0, get_volume_n_dimensions(volume) )
    {
        int_index[c] = (int) voxel[c];
        if( int_index[c] == sizes[c] - 1 )
            int_index[c] = sizes[c] - 2;
        n[c] = 2;
    }

    for_less( c, get_volume_n_dimensions(volume), MAX_DIMENSIONS )
    {
        n[c] = 1;
        int_index[c] = 0;
    }

    active_found = FALSE;
    inactive_found = FALSE;

    for_less( ind[X], int_index[X], int_index[X] + n[X] )
    for_less( ind[Y], int_index[Y], int_index[Y] + n[Y] )
    for_less( ind[Z], int_index[Z], int_index[Z] + n[Z] )
    for_less( ind[3], int_index[3], int_index[3] + n[3] )
    for_less( ind[4], int_index[4], int_index[4] + n[4] )
    {
        if( get_volume_label_data( volume, ind ) == 0 )
        {
            if( inactive_found )
                return( activity_if_mixed );
            active_found = TRUE;
        }
        else
        {
            if( active_found )
                return( activity_if_mixed );
            inactive_found = TRUE;
        }
    }

    if( active_found && !inactive_found )
        return( TRUE );
    else if( !active_found && inactive_found )
        return( FALSE );
    else
        return( activity_if_mixed );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_input_volume_label_limits
@INPUT      : volume1
              volume2
              slice
@OUTPUT     : limits
@RETURNS    : 
@DESCRIPTION: Computes the range of overlap of volume2 in volume1.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Aug. 1, 1995    David MacDonald
@MODIFIED   : May. 6, 1997    D. MacDonald     -  one slice of 3d volume
---------------------------------------------------------------------------- */

private  void  get_input_volume_label_limits(
    Volume   volume1,
    Volume   volume2,
    int      slice,
    int      limits[2][N_DIMENSIONS] )
{
    int       sizes1[N_DIMENSIONS], sizes2[N_DIMENSIONS];
    int       d, t[N_DIMENSIONS];
    Real      voxel1[N_DIMENSIONS], voxel2[N_DIMENSIONS];
    int       pos;
    Real      xw, yw, zw;
    BOOLEAN   first;

    get_volume_sizes( volume1, sizes1 );
    get_volume_sizes( volume2, sizes2 );

    first = TRUE;

    for_less( t[0], 0, 2 )
    for_less( t[1], 0, 2 )
    for_less( t[2], 0, 2 )
    {
        voxel2[0] = (Real) slice - 0.5 + (Real) t[0];
        voxel2[1] = -0.5 + (Real) t[1] * (Real) sizes2[1];
        voxel2[2] = -0.5 + (Real) t[2] * (Real) sizes2[2];

        convert_voxel_to_world( volume2, voxel2, &xw, &yw, &zw );
        convert_world_to_voxel( volume1, xw, yw, zw, voxel1 );

        for_less( d, 0, N_DIMENSIONS )
        {
            pos = FLOOR( voxel1[d] + 0.5 );

            if( first )
            {
                limits[0][d] = pos;
                limits[1][d] = pos;
            }
            else
            {
                if( pos < limits[0][d] )
                    limits[0][d] = pos;
                else if( pos > limits[1][d] )
                    limits[1][d] = pos;
            }
        }

        first = FALSE;
    }

    for_less( d, 0, N_DIMENSIONS )
    {
        if( limits[0][d] < 0 )
            limits[0][d] = 0;
        if( limits[1][d] >= sizes1[d] )
            limits[1][d] = sizes1[d] - 1;
    }
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : load_label_volume
@INPUT      : Filename
@OUTPUT     : label_volume
@RETURNS    : ERROR or OK
@DESCRIPTION: Loads the label volume.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : Aug. 1, 1995    D. MacDonald  -  loaded volume no longer needs
                                               to be same grid.
@MODIFIED   : May. 6, 1997    D. MacDonald  -  now loads one slice at a time,
                                               and does not overwrite labels
                                               with 0's.
@MODIFIED   : Jun. 6, 1997    D. MacDonald  -  special case, fast, for matching
                                               coordinate systems
---------------------------------------------------------------------------- */

public  Status  load_label_volume(
    STRING   filename,
    Volume   label_volume )
{
    int                   slice, n_slices;
    int                   label_voxel[N_DIMENSIONS];
    int                   int_voxel[N_DIMENSIONS], int_file_value;
    int                   limits[2][N_DIMENSIONS];
    int                   file_sizes[N_DIMENSIONS];
    Real                  file_value, xw, yw, zw, amount_done;
    int                   int_y_voxel[N_DIMENSIONS];
    Real                  y_voxel[N_DIMENSIONS];
    Real                  voxel[N_DIMENSIONS], z_dx, z_dy, z_dz;
    Real                  y_dx, y_dy, y_dz;
    int                   int_z_dx, int_z_dy, int_z_dz;
    int                   int_y_dx, int_y_dy, int_y_dz;
    Real                  start_voxel[N_DIMENSIONS];
    Real                  end_voxel[N_DIMENSIONS];
    Volume                file_volume, file_volume_3d;
    Minc_file             file;
    BOOLEAN               is_linear, is_integer_step;
    progress_struct       progress;

    check_alloc_label_data( label_volume );

    /*--- get 3d transformation in file */

    if( input_volume_header_only( filename, N_DIMENSIONS,
                                  File_order_dimension_names,
                                  &file_volume_3d, NULL ) != OK )
        return( ERROR );

    get_volume_sizes( file_volume_3d, file_sizes );

    file_volume = create_volume( 2, File_order_dimension_names,
                                 NC_UNSPECIFIED, FALSE, 0.0, 0.0 );

    file = initialize_minc_input( filename, file_volume, NULL );

    if( file == NULL )
        return( ERROR );

    n_slices = get_n_input_volumes( file );

    if( n_slices != file_sizes[0] )
    {
        print_error( "load_label_volume(): error in sizes: %d %d\n",
                     n_slices, file_sizes[0] );
    }

    /*--- check if the voxel1-to-voxel2 transform is linear */

    is_linear = get_transform_type(
                get_voxel_to_world_transform(label_volume) ) == LINEAR &&
                get_transform_type(
                get_voxel_to_world_transform(file_volume_3d) ) == LINEAR;
    is_integer_step = FALSE;

    if( is_linear )
    {
        convert_3D_voxel_to_world( label_volume, 0.0, 0.0, 0.0,
                                   &xw, &yw, &zw );
        convert_world_to_voxel( file_volume_3d, xw, yw, zw, start_voxel );

        convert_3D_voxel_to_world( label_volume, 0.0, 0.0, 1.0,
                                   &xw, &yw, &zw );
        convert_world_to_voxel( file_volume_3d, xw, yw, zw, end_voxel );

        z_dx = end_voxel[0] - start_voxel[0];
        z_dy = end_voxel[1] - start_voxel[1];
        z_dz = end_voxel[2] - start_voxel[2];

        convert_3D_voxel_to_world( label_volume, 0.0, 1.0, 0.0,
                                   &xw, &yw, &zw );
        convert_world_to_voxel( file_volume_3d, xw, yw, zw, end_voxel );

        y_dx = end_voxel[0] - start_voxel[0];
        y_dy = end_voxel[1] - start_voxel[1];
        y_dz = end_voxel[2] - start_voxel[2];

        is_integer_step = FALSE;
        if( IS_INT(z_dx) && IS_INT(z_dy) && IS_INT(z_dz) &&
            IS_INT(y_dx) && IS_INT(y_dy) && IS_INT(y_dz) )
        {
            is_integer_step = TRUE;
            int_z_dx = (int) z_dx;
            int_z_dy = (int) z_dy;
            int_z_dz = (int) z_dz;
            int_y_dx = (int) y_dx;
            int_y_dy = (int) y_dy;
            int_y_dz = (int) y_dz;
        }
    }

    /*--- input label slices */

    initialize_progress_report( &progress, FALSE, n_slices,
                                "Reading Labels" );

    for_less( slice, 0, n_slices )
    {
        while( input_more_minc_file( file, &amount_done ) )
        {}

        get_input_volume_label_limits( label_volume, file_volume_3d,
                                       slice, limits );

        for_inclusive( label_voxel[X], limits[0][X], limits[1][X] )
        {
            for_inclusive( label_voxel[Y], limits[0][Y], limits[1][Y] )
            {
                if( is_linear )
                {
                    if( label_voxel[Y] == limits[0][Y] )
                    {
                        convert_3D_voxel_to_world( label_volume,
                                                   (Real) label_voxel[X],
                                                   (Real) label_voxel[Y],
                                                   (Real) limits[0][Z],
                                                   &xw, &yw, &zw );
                        convert_world_to_voxel( file_volume_3d, xw, yw, zw,
                                                y_voxel );
                        y_voxel[X] += 0.5;
                        y_voxel[Y] += 0.5;
                        y_voxel[Z] += 0.5;

                        if( is_integer_step )
                        {
                            int_y_voxel[X] = FLOOR( y_voxel[X] );
                            int_y_voxel[Y] = FLOOR( y_voxel[Y] );
                            int_y_voxel[Z] = FLOOR( y_voxel[Z] );
                        }
                    }
                    else
                    {
                        if( is_integer_step )
                        {
                            int_y_voxel[X] += int_y_dx;
                            int_y_voxel[Y] += int_y_dy;
                            int_y_voxel[Z] += int_y_dz;
                        }
                        else
                        {
                            y_voxel[X] += y_dx;
                            y_voxel[Y] += y_dy;
                            y_voxel[Z] += y_dz;
                        }
                    }

                    if( is_integer_step )
                    {
                        int_voxel[X] = int_y_voxel[X];
                        int_voxel[Y] = int_y_voxel[Y];
                        int_voxel[Z] = int_y_voxel[Z];
                    }
                    else
                    {
                        voxel[X] = y_voxel[X];
                        voxel[Y] = y_voxel[Y];
                        voxel[Z] = y_voxel[Z];
                    }
                }

                for_inclusive( label_voxel[Z], limits[0][Z], limits[1][Z] )
                {
                    if( !is_linear )
                    {
                        convert_3D_voxel_to_world( label_volume,
                                                   (Real) label_voxel[X],
                                                   (Real) label_voxel[Y],
                                                   (Real) label_voxel[Z],
                                                   &xw, &yw, &zw );
                        convert_world_to_voxel( file_volume_3d, xw, yw, zw,
                                                voxel );
                        voxel[X] += 0.5;
                        voxel[Y] += 0.5;
                        voxel[Z] += 0.5;
                    }
                    else if( label_voxel[Z] != limits[0][Z] )
                    {
                        if( is_integer_step )
                        {
                            int_voxel[X] += int_z_dx;
                            int_voxel[Y] += int_z_dy;
                            int_voxel[Z] += int_z_dz;
                        }
                        else
                        {
                            voxel[X] += z_dx;
                            voxel[Y] += z_dy;
                            voxel[Z] += z_dz;
                        }
                    }

                    if( !is_integer_step )
                    {
                        int_voxel[X] = FLOOR( voxel[X] );
                        int_voxel[Y] = FLOOR( voxel[Y] );
                        int_voxel[Z] = FLOOR( voxel[Z] );
                    }

                    if( int_voxel[X] == slice &&
                        int_voxel[Y] >= 0 && int_voxel[Y] < file_sizes[Y] &&
                        int_voxel[Z] >= 0 && int_voxel[Z] < file_sizes[Z] )
                    {
                        file_value = get_volume_real_value( file_volume,
                                        int_voxel[Y], int_voxel[Z], 0, 0, 0 );

                        if( file_value > 0.0 )
                        {
                            int_file_value = ROUND( file_value );
                            set_volume_label_data( label_volume, label_voxel,
                                                   int_file_value );
                        }
                    }
                }
            }
        }

        (void) advance_input_volume( file );

        update_progress_report( &progress, slice+1 );
    }

    terminate_progress_report( &progress );

    delete_volume( file_volume );
    delete_volume( file_volume_3d );

    (void) close_minc_input( file );

    return( OK );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : save_label_volume
@INPUT      : filename
              original_filename
              label_volume
@OUTPUT     : 
@RETURNS    : OK or ERROR
@DESCRIPTION: Saves the label volume.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : Aug. 1, 1995    D. MacDonald    - crops the volume on output
---------------------------------------------------------------------------- */

public  Status  save_label_volume(
    STRING   filename,
    STRING   original_filename,
    Volume   label_volume,
    Real     crop_threshold )
{
    Status   status;
    BOOLEAN  cropping;
    Volume   cropped_volume;
    int      n_voxels, n_voxels_cropped;
    int      c, limits[2][MAX_DIMENSIONS], sizes[MAX_DIMENSIONS];

    check_alloc_label_data( label_volume );

    if( crop_threshold > 0.0 )
    {
        if( find_volume_crop_bounds( label_volume, -1.0, 0.5, limits ) )
        {
            get_volume_sizes( label_volume, sizes );
            n_voxels = 1;
            n_voxels_cropped = 1;
            for_less( c, 0, N_DIMENSIONS )
            {
                n_voxels *= sizes[c];
                n_voxels_cropped *= limits[1][c] - limits[0][c] + 1;
            }

            cropping = ((Real) n_voxels_cropped / (Real) n_voxels <
                        crop_threshold);
        }
        else
        {
            for_less( c, 0, N_DIMENSIONS )
            {
                limits[0][c] = 0;
                limits[1][c] = 0;
            }
            cropping = TRUE;
        }
    }
    else
        cropping = FALSE;

    if( cropping )
        cropped_volume = create_cropped_volume( label_volume, limits );
    else
        cropped_volume = label_volume;

    if( original_filename != NULL )
    {
        status = output_modified_volume( filename,
                                NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                                cropped_volume, original_filename,
                                "Label volume\n", NULL );
    }
    else
    {
        status = output_volume( filename, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                                cropped_volume, "Label volume\n", NULL );
    }

    if( cropping )
        delete_volume( cropped_volume );

    return( status );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : input_tags_as_labels
@INPUT      : file
              volume
@OUTPUT     : label_volume
@RETURNS    : OK or ERROR
@DESCRIPTION: Inputs a tag file into a label volume.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : 1993             David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  Status  input_tags_as_labels(
    FILE    *file,
    Volume  volume,
    Volume  label_volume )
{
    Status          status;
    int             c, label, ind[MAX_DIMENSIONS];
    Real            voxel[MAX_DIMENSIONS];
    int             n_volumes;
    Real            tag1[N_DIMENSIONS];
    int             structure_id;
    Real            min_label, max_label;

    check_alloc_label_data( label_volume );

    get_volume_real_range( label_volume, &min_label, &max_label );

    status = initialize_tag_file_input( file, &n_volumes );

    while( status == OK &&
           input_one_tag( file, n_volumes,
                          tag1, NULL, NULL, &structure_id, NULL, NULL,
                          &status ) )
    {
        convert_world_to_voxel( volume, tag1[X], tag1[Y], tag1[Z], voxel );

        for_less( c, 0, get_volume_n_dimensions(volume) )
        {
            ind[c] = ROUND( voxel[c] );
        }

        label = structure_id;
        if( (Real) label >= min_label && (Real) label <= max_label &&
            int_voxel_is_within_volume( volume, ind ) )
        {
            set_volume_label_data( label_volume, ind, label);
        }
    }

    return( status );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : create_label_volume_from_file
@INPUT      : filename
              volume
@OUTPUT     : label_volume
@RETURNS    : OK or ERROR
@DESCRIPTION: Creates a label volume for the given volume from a tag file or
              a minc file.
@METHOD     :
@GLOBALS    :
@CALLS      :
@CREATED    : Dec.  8, 1995    David MacDonald
@MODIFIED   :
---------------------------------------------------------------------------- */

public  Status  create_label_volume_from_file(
    STRING   filename,
    Volume   volume,
    Volume   *label_volume )
{
    Status  status;
    STRING  *dim_names;
    Volume  file_volume;
    FILE    *file;
    BOOLEAN same_grid;

    status = OK;

    if( filename_extension_matches( filename, "mnc" ) )
    {
        dim_names = get_volume_dimension_names( volume );
        status = input_volume_header_only( filename,
                                           get_volume_n_dimensions(volume),
                                           dim_names, &file_volume, NULL );

        if( status != OK )
        {
            delete_dimension_names( volume, dim_names );
            return( status );
        }

        same_grid = volumes_are_same_grid( volume, file_volume );
        delete_volume( file_volume );

        if( same_grid )
        {
            status = input_volume( filename, get_volume_n_dimensions(volume),
                                   dim_names, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                                   TRUE, label_volume, NULL );
        }
        else
        {
            *label_volume = create_label_volume( volume, NC_UNSPECIFIED );
            status = load_label_volume( filename, *label_volume );
        }

        delete_dimension_names( volume, dim_names );
    }
    else
    {
        *label_volume = create_label_volume( volume, NC_UNSPECIFIED );

        if( open_file( filename, READ_FILE, ASCII_FORMAT, &file )!=OK)
            return( ERROR );

        if( input_tags_as_labels( file, volume, *label_volume ) != OK )
            return( ERROR );

        (void) close_file( file );
    }

    return( status );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : output_labels_as_tags
@INPUT      : file
              volume
              label_volume
              desired_label
              size
              patient_id
@OUTPUT     : 
@RETURNS    : OK or ERROR
@DESCRIPTION: Outputs a set of labels as tags.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : Oct. 19, 1995   D. MacDonald - now writes tags 1 at a time to
                                             be more memory efficient.
---------------------------------------------------------------------------- */

public  Status  output_labels_as_tags(
    FILE    *file,
    Volume  volume,
    Volume  label_volume,
    int     desired_label,
    Real    size,
    int     patient_id )
{
    int             ind[N_DIMENSIONS];
    int             label, sizes[MAX_DIMENSIONS];
    Real            real_ind[N_DIMENSIONS];
    Real            tags[N_DIMENSIONS];
    int             n_tags;

    if( get_volume_n_dimensions(volume) != 3 )
    {
        print_error( "output_labels_as_tags:  volume must be 3D\n" );
        return( ERROR );
    }

    check_alloc_label_data( label_volume );
    get_volume_sizes( label_volume, sizes );

    n_tags = 0;

    for_less( ind[X], 0, sizes[X] )
    {
        real_ind[X] = (Real) ind[X];
        for_less( ind[Y], 0, sizes[Y] )
        {
            real_ind[Y] = (Real) ind[Y];
            for_less( ind[Z], 0, sizes[Z] )
            {
                real_ind[Z] = (Real) ind[Z];
                label = get_volume_label_data( label_volume, ind );

                if( label == desired_label || (desired_label < 0 && label > 0) )
                {
                    convert_voxel_to_world( volume, real_ind,
                                            &tags[X], &tags[Y], &tags[Z] );

                    if( n_tags == 0 &&
                        initialize_tag_file_output( file, NULL, 1 ) != OK )
                        return( ERROR );

                    if( output_one_tag( file, 1, tags, NULL,
                               &size, &label, &patient_id, NULL ) != OK )
                        return( ERROR );

                    ++n_tags;
                }
            }
        }
    }

    if( n_tags > 0 )
        terminate_tag_file_output( file );

    return( OK );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : input_landmarks_as_labels
@INPUT      : file
              volume
@OUTPUT     : label_volume
@RETURNS    : OK or ERROR
@DESCRIPTION: Loads a set of landmarks into the label_volume.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : Oct. 19, 1995   D. MacDonald - now reads tags 1 at a time to
                                             be more memory efficient.
---------------------------------------------------------------------------- */

public  Status  input_landmarks_as_labels(
    FILE    *file,
    Volume  volume,
    Volume  label_volume )
{
    int             c, label, ind[MAX_DIMENSIONS];
    Real            voxel[MAX_DIMENSIONS];
    marker_struct   marker;
    Real            min_label, max_label;

    check_alloc_label_data( label_volume );

    get_volume_real_range( label_volume, &min_label, &max_label );

    while( io_tag_point( file, READ_FILE, volume, 1.0, &marker ) == OK )
    {
        convert_world_to_voxel( volume,
                                (Real) Point_x(marker.position),
                                (Real) Point_y(marker.position),
                                (Real) Point_z(marker.position), voxel );

        for_less( c, 0, get_volume_n_dimensions(volume) )
            ind[c] = ROUND( voxel[c] );

        label = marker.structure_id;
        if( (Real) label >= min_label && (Real) label <= max_label &&
            int_voxel_is_within_volume( volume, ind ) )
        {
            set_volume_label_data( label_volume, ind, label );
        }
    }

    return( OK );
}
