#include  <volume_io.h>
#include  <deform.h>

  VIO_BOOL  is_point_inside_surface(
    VIO_Volume                      volume,
    VIO_Volume                      label_volume,
    int                         continuity,
    VIO_Real                        voxel[],
    VIO_Vector                      *direction,
    boundary_definition_struct  *boundary_def )
{
    VIO_BOOL active;
    VIO_Real    value, mag, dx, dy, dz, dot_product;
    VIO_Real    derivs[VIO_MAX_DIMENSIONS], *deriv_ptr[1];
    VIO_Real    min_dot, max_dot;

    active = get_volume_voxel_activity( label_volume, voxel, FALSE );

    if( !active )
        return( FALSE );

    deriv_ptr[0] = derivs;

    evaluate_volume( volume, voxel, NULL, continuity, FALSE,
                     get_volume_real_min(volume),
                     &value, deriv_ptr, NULL );

    if( value < boundary_def->min_isovalue )
        return( FALSE );
    else if( value >= boundary_def->max_isovalue )
        return( TRUE );

    convert_voxel_normal_vector_to_world( volume, derivs,
                                          &dx, &dy, &dz );

    mag = dx * dx + dy * dy + dz * dz;

    if( mag <
        boundary_def->gradient_threshold * boundary_def->gradient_threshold )
        return( FALSE );

    if( mag == 0.0 )
        mag = 1.0;

    dot_product = dx * (VIO_Real) Vector_x(*direction) +
                  dy * (VIO_Real) Vector_y(*direction) +
                  dz * (VIO_Real) Vector_z(*direction);

    min_dot = boundary_def->min_dot_product;
    max_dot = boundary_def->max_dot_product;

    if( min_dot <= -1.0 && max_dot == 0.0 )
        return( dot_product <= 0.0 );
    else if( min_dot == 0.0 && max_dot >= 1.0 )
        return( dot_product >= 0.0 );
    else if( min_dot <= -1.0 && max_dot >= 1.0 )
        return( TRUE );
    else
    {
        dot_product /= sqrt(mag);

        return( dot_product >= min_dot && dot_product <= max_dot );
    }
}

  void   get_centre_of_cube(
    VIO_Point       *cube,
    int         sizes[3],
    VIO_Point       *centre )
{
    int  c;

    for_less( c, 0, 3 )
    {
        if( sizes[c] > 1 )
            Point_coord(*centre,c) = (VIO_Point_coord_type) (
                 ((VIO_Real) Point_coord(cube[0],c) +
                  (VIO_Real) Point_coord(cube[1],c)) / 2.0);
        else
            Point_coord(*centre,c) = Point_coord(cube[0],c);
    }
}

  VIO_BOOL  contains_value(
    VIO_Real  values[2][2][2],
    int   sizes[3] )
{
    VIO_BOOL  under_found, over_found;
    int      x, y, z;

    for_less( x, 0, sizes[VIO_X] )
    {
        for_less( y, 0, sizes[VIO_Y] )
        {
            for_less( z, 0, sizes[VIO_Z] )
            {
                if( values[x][y][z] == 0.0 )
                {
                    return( TRUE );
                }
                else if( x == 0 && y == 0 && z == 0 )
                {
                    under_found = (values[x][y][z] < 0.0);
                    over_found = (values[x][y][z] > 0.0);
                }
                else if( values[x][y][z] < 0.0 )
                {
                    if( over_found )
                        return( TRUE );
                    under_found = TRUE;
                }
                else if( values[x][y][z] > 0.0 )
                {
                    if( under_found )
                        return( TRUE );
                    over_found = TRUE;
                }
            }
        }
    }

    return( FALSE );
}

  VIO_BOOL  cube_is_small_enough(
    VIO_Point     cube[2],
    int       sizes[3],
    VIO_Real      min_cube_size )
{
    VIO_BOOL  small_enough;
    VIO_Real     size_in_dimension;
    int      c;

    small_enough = TRUE;

    for_less( c, 0, 3 )
    {
        size_in_dimension = (VIO_Real) Point_coord(cube[sizes[c]-1],c ) -
                            (VIO_Real) Point_coord(cube[0],c );
        if( size_in_dimension > min_cube_size )
        {
            small_enough = FALSE;
            break;
        }
    }

    return( small_enough );
}

  void  initialize_deform_stats(
    deform_stats  *stats )
{
    int  i;

    stats->average = 0.0;
    stats->maximum = 0.0;

    for_less( i, 0, N_DEFORM_HISTOGRAM )
        stats->n_below[i] = 0;
}

  void  record_error_in_deform_stats(
    deform_stats   *stats,
    VIO_Real           error )
{
    int  i;

    stats->average += error;
    if( error > stats->maximum )
        stats->maximum = error;

    i = N_DEFORM_HISTOGRAM-1;
    while( i >= 0 && error <= (VIO_Real) (i+1) )
    {
        ++stats->n_below[i];
        --i;
    }
}

  void  print_deform_stats(
    deform_stats   *stats,
    int            n_points )
{
    int    i, n_above;

    print( "avg %5.2f  max %6.2f  hist:",
           stats->average / (VIO_Real) n_points, stats->maximum );

    for_less( i, 0, N_DEFORM_HISTOGRAM )
    {
        if( i < N_DEFORM_HISTOGRAM-1 )
            n_above = stats->n_below[i+1] - stats->n_below[i];
        else
            n_above = n_points - stats->n_below[i];

        if( stats->n_below[i] == n_points )
            break;

        if( n_above == 0 )
            print( "     " );
        else if( n_above < 100 )
            print( " %4d", n_above );
        else
            print( " %3.0f%%", 100.0 * (VIO_Real) n_above / (VIO_Real) n_points );
    }
    print( "\n" );
}

  VIO_BOOL   get_max_point_cube_distance(
    VIO_Point   cube[2],
    int     sizes[3],
    VIO_Point   *point,
    VIO_Real    *distance )
{
    int      c;
    VIO_Real     dist_to_low, dist_to_high, dist, max_dist;

    max_dist = 0.0;

    for_less( c, 0, 3 )
    {
        if( sizes[c] > 1 )
        {
            dist_to_low = (VIO_Real) Point_coord(*point,c) -
                          (VIO_Real) Point_coord(cube[0],c);
            dist_to_high = (VIO_Real) Point_coord(cube[1],c) -
                           (VIO_Real) Point_coord(*point,c);

            dist = MAX( dist_to_low, dist_to_high );
            max_dist += dist * dist;
        }
    }

    max_dist = sqrt( (double) max_dist );

    if( max_dist < *distance )
    {
        *distance = max_dist;
        return( TRUE );
    }
    else
        return( FALSE );
}

  void  initialize_deformation_parameters(
    deform_struct  *deform )
{
    deform->deform_data.type = VOLUME_DATA;

    initialize_deformation_model( &deform->deformation_model );
    deform->deformation_model.position_constrained = FALSE;
    deform->fractional_step = 0.3;
    deform->max_step = 0.3;
    deform->stop_threshold = 0.0;
    deform->degrees_continuity = 0;
    deform->boundary_definition.min_isovalue = 90.0;
    deform->boundary_definition.max_isovalue = 90.0;
    deform->boundary_definition.gradient_threshold = 0.0;
    deform->boundary_definition.min_dot_product = -2.0;
    deform->boundary_definition.max_dot_product = 0.0;
    deform->boundary_definition.normal_direction = TOWARDS_LOWER;
    deform->max_iterations = 1000000;
    deform->stop_threshold = 0.0;

    deform->n_movements_alloced = 0;
    deform->movement_threshold = 0.0;
}

  void  delete_deformation_parameters(
    deform_struct  *deform )
{
    delete_deformation_model( &deform->deformation_model );

    if( deform->n_movements_alloced > 0 )
        FREE( deform->prev_movements );
}

  void  set_boundary_definition(
    boundary_definition_struct  *boundary_def,
    VIO_Real                        min_value,
    VIO_Real                        max_value,
    VIO_Real                        grad_threshold,
    VIO_Real                        angle,
    char                        direction,
    VIO_Real                        tolerance )
{
    VIO_Real   cosine;

    boundary_def->min_isovalue = MIN( min_value, max_value );
    boundary_def->max_isovalue = MAX( min_value, max_value );
    boundary_def->gradient_threshold = grad_threshold;
    boundary_def->tolerance = tolerance;

    if( angle == 90.0 )
        cosine = 0.0;
    else
        cosine = cos( angle * VIO_DEG_TO_RAD );

    if( direction == '-' )
    {
        boundary_def->normal_direction = TOWARDS_LOWER;
        boundary_def->min_dot_product = -2.0;
        boundary_def->max_dot_product = -cosine;
    }
    else if( direction == '+' )
    {
        boundary_def->normal_direction = TOWARDS_HIGHER;
        boundary_def->min_dot_product = cosine;
        boundary_def->max_dot_product = 2.0;
    }
    else
    {
        boundary_def->normal_direction = ANY_DIRECTION;
        boundary_def->min_dot_product = -2.0;
        boundary_def->max_dot_product = 2.0;
    }
}

  void  initialize_lookup_volume_coeficients(
    voxel_coef_struct  *lookup )
{
    lookup->n_in_hash = 0;
}

  void  lookup_volume_coeficients(
    voxel_coef_struct  *lookup,
    VIO_Volume             volume,
    int                degrees_continuity,
    int                x,
    int                y,
    int                z,
    VIO_Real               c[] )
{
    int                    key, i, offset, n, sizes[VIO_N_DIMENSIONS];
    voxel_lin_coef_struct  *data;

    offset = -(degrees_continuity + 1) / 2;
    n = degrees_continuity + 2;
    get_volume_sizes( volume, sizes );

    if( x + offset < 0 || x + offset + n >= sizes[VIO_X] ||
        y + offset < 0 || y + offset + n >= sizes[VIO_Y] ||
        z + offset < 0 || z + offset + n >= sizes[VIO_Z] )
    {
        for_less( i, 0, n * n * n )
            c[i] = 0.0;
        return;
    }

    if( lookup == NULL || degrees_continuity != 0 )
    {
        get_volume_value_hyperslab_3d( volume, x+offset, y+offset, z+offset,
                                       n, n, n, c );
        return;
    }

    if( lookup->n_in_hash == 0 )
    {
        initialize_hash_table( &lookup->hash, MAX_IN_VOXEL_COEF_LOOKUP * 10,
                               sizeof(voxel_lin_coef_struct *),
                               0.5, 0.25 );
        lookup->head = NULL;
        lookup->tail = NULL;
    }

    key = VIO_IJK( x, y, z, sizes[VIO_Y], sizes[VIO_Z] );

    data = NULL;

    if( !lookup_in_hash_table( &lookup->hash, key, (void *) &data ) )
    {
        if( lookup->n_in_hash >= MAX_IN_VOXEL_COEF_LOOKUP )
        {
            if( !remove_from_hash_table( &lookup->hash, lookup->tail->hash_key,
                                         (void *) &data))
                handle_internal_error( "lookup_volume_coeficients" );

            lookup->tail = data->prev;
            if( lookup->tail == NULL )
                lookup->head = NULL;
            else
                lookup->tail->next = NULL;
        }
        else
        {
            ALLOC( data, 1 );
            ++lookup->n_in_hash;
        }

        data->hash_key = key;
        get_volume_value_hyperslab_3d( volume, x, y, z, 2, 2, 2, data->coefs );

        data->next = lookup->head;
        data->prev = NULL;
        if( lookup->head != NULL )
            lookup->head->prev = data;
        else
            lookup->tail = data;
        lookup->head = data;

        insert_in_hash_table( &lookup->hash, key, (void *) &data );
    }
    
    for_less( i, 0, 8 )
        c[i] = data->coefs[i];
}

  void  delete_lookup_volume_coeficients(
    voxel_coef_struct  *lookup )
{
    voxel_lin_coef_struct  *ptr, *next;

    ptr = lookup->head;
    while( ptr != NULL )
    {
        next = ptr->next;
        FREE( ptr );
        ptr = next;
    }

    if( lookup->n_in_hash > 0 )
        delete_hash_table( &lookup->hash );
}
