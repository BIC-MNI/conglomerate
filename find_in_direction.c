#include  <internal_volume_io.h>
#include  <deform.h>

private  BOOLEAN  voxel_might_contain_boundary(
    Volume                      volume,
    bitlist_3d_struct           *done_bits,
    bitlist_3d_struct           *surface_bits,
    int                         degrees_continuity,
    int                         voxel_indices[],
    boundary_definition_struct  *boundary_def );

private  BOOLEAN  check_voxel_for_isovalue(
    voxel_coef_struct   *lookup,
    Volume              volume,
    Volume              label_volume,
    int                 degrees_continuity,
    int                 voxel_indices[],
    Real                line_origin[],
    Real                line_direction[],
    BOOLEAN             searching_inwards,
    Real                min_line_t,
    Real                max_line_t,
    Real                isovalue,
    Normal_directions   normal_direction,
    BOOLEAN             already_found,
    Real                *dist_found );

private  BOOLEAN  check_voxel_for_boundary(
    voxel_coef_struct           *lookup,
    Volume                      volume,
    Volume                      label_volume,
    int                         degrees_continuity,
    int                         voxel_indices[],
    Real                        line_origin[],
    Real                        line_direction[],
    BOOLEAN                     *inside_surface,
    BOOLEAN                     searching_inwards,
    Real                        min_line_t,
    Real                        max_line_t,
    boundary_definition_struct  *boundary_def,
    BOOLEAN                     already_found,
    Real                        *dist_found );

#define  GET_RAY_POINT( result, origin, direction, dist ) \
    { \
        (result)[X] = (origin)[X] + (dist) * (direction)[X]; \
        (result)[Y] = (origin)[Y] + (dist) * (direction)[Y]; \
        (result)[Z] = (origin)[Z] + (dist) * (direction)[Z]; \
    }

private  void  clip_line_to_volume(
    Volume     volume,
    int        degrees_continuity,
    Real       line_origin[],
    Real       line_direction[],
    Real       *t_min,
    Real       *t_max )
{
    int    sizes[N_DIMENSIONS];
    Point  origin;
    Vector direction;

    get_volume_sizes( volume, sizes );

    fill_Point( origin, line_origin[X], line_origin[Y], line_origin[Z] );
    fill_Point( direction,
                line_direction[X], line_direction[Y], line_direction[Z] );

    (void) clip_line_to_box( &origin, &direction,
                             0.0 + (Real) degrees_continuity * 0.5,
                             (Real) sizes[X]-1.0 - (Real)degrees_continuity*0.5,
                             0.0 + (Real) degrees_continuity * 0.5,
                             (Real) sizes[Y]-1.0 - (Real)degrees_continuity*0.5,
                             0.0 + (Real) degrees_continuity * 0.5,
                             (Real) sizes[Z]-1.0 - (Real)degrees_continuity*0.5,
                             t_min, t_max );
}

private  void  set_up_to_search_ray(
    Volume                      volume,
    int                         degrees_continuity,
    Real                        ray_origin[],
    Real                        unit_pos_dir[],
    Real                        unit_neg_dir[],
    Real                        model_dist,
    Real                        start_dist,
    Real                        end_dist,
    Real                        origin[],
    Real                        direction[],
    Real                        *current_distance,
    Real                        *stop_distance,
    Real                        next_distance[N_DIMENSIONS],
    Real                        delta_distance[N_DIMENSIONS],
    int                         voxel_index[N_DIMENSIONS],
    int                         delta_voxel[N_DIMENSIONS],
    Real                        *next_closest_distance )
{
    BOOLEAN       found_exit;
    Real          voxel_exit, t_min, t_max, voxel_offset;
    Real          direct;
    Real          start_voxel[N_DIMENSIONS], model_point[N_DIMENSIONS];
    Real          *dir;

    if( model_dist + start_dist < 0.0 || model_dist + end_dist < 0.0 )
        dir = &unit_neg_dir[0];
    else
        dir = &unit_pos_dir[0];

    GET_RAY_POINT( model_point, ray_origin, dir, model_dist );

    convert_world_to_voxel( volume,
                            model_point[X], model_point[Y], model_point[Z],
                            origin );

    if( end_dist >= start_dist )
    {
        GET_RAY_POINT( direction, model_point, dir, 1.0 );
    }
    else
    {
        GET_RAY_POINT( direction, model_point, dir, -1.0 );
    }

    convert_world_to_voxel( volume,
                            direction[X], 
                            direction[Y],
                            direction[Z], direction );
    direction[X] -= origin[X];
    direction[Y] -= origin[Y];
    direction[Z] -= origin[Z];

    clip_line_to_volume( volume, degrees_continuity,
                         origin, direction, &t_min, &t_max );

    *current_distance = FABS(start_dist);
    if( t_min > *current_distance )
        *current_distance = t_min;
    *stop_distance = FABS(end_dist);
    if( t_max < *stop_distance )
        *stop_distance = t_max;

    GET_RAY_POINT( start_voxel, origin, direction, *current_distance );

    found_exit = FALSE;

    if( degrees_continuity % 2 == 1 )
        voxel_offset = 0.5;
    else
        voxel_offset = 0.0;

    /*--- do x loop */

    direct = direction[X];

    voxel_index[X] = (int) (start_voxel[X]+voxel_offset);
    voxel_exit = (Real) voxel_index[X] - voxel_offset;

    if( direct == 0.0 )
    {
        delta_voxel[X] = 1;
        delta_distance[X] = 0.0;
        next_distance[X] = *stop_distance;
    }
    else
    {
        direct = 1.0 / direct;
        if( direct < 0.0 )
        {
            delta_voxel[X] = -1;
            delta_distance[X] = -direct;
        }
        else
        {
            voxel_exit += 1.0;
            delta_voxel[X] = 1;
            delta_distance[X] = direct;
        }

        next_distance[X] = (voxel_exit - origin[X]) * direct;

        *next_closest_distance = next_distance[X];
        found_exit = TRUE;
    }
    /*--- do y loop */

    direct = direction[Y];

    voxel_index[Y] = (int) (start_voxel[Y]+voxel_offset);
    voxel_exit = (Real) voxel_index[Y] - voxel_offset;

    if( direct == 0.0 )
    {
        delta_voxel[Y] = 1;
        delta_distance[Y] = 0.0;
        next_distance[Y] = *stop_distance;
    }
    else
    {
        direct = 1.0 / direct;
        if( direct < 0.0 )
        {
            delta_voxel[Y] = -1;
            delta_distance[Y] = -direct;
        }
        else
        {
            voxel_exit += 1.0;
            delta_voxel[Y] = 1;
            delta_distance[Y] = direct;
        }

        next_distance[Y] = (voxel_exit - origin[Y]) * direct;

        if( !found_exit || next_distance[Y] < *next_closest_distance )
        {
            *next_closest_distance = next_distance[Y];
            found_exit = TRUE;
        }
    }
    /*--- do z loop */

    direct = direction[Z];

    voxel_index[Z] = (int) (start_voxel[Z]+voxel_offset);
    voxel_exit = (Real) voxel_index[Z] - voxel_offset;

    if( direct == 0.0 )
    {
        delta_voxel[Z] = 1;
        delta_distance[Z] = 0.0;
        next_distance[Z] = *stop_distance;
    }
    else
    {
        direct = 1.0 / direct;
        if( direct < 0.0 )
        {
            delta_voxel[Z] = -1;
            delta_distance[Z] = -direct;
        }
        else
        {
            voxel_exit += 1.0;
            delta_voxel[Z] = 1;
            delta_distance[Z] = direct;
        }

        next_distance[Z] = (voxel_exit - origin[Z]) * direct;

        if( !found_exit || next_distance[Z] < *next_closest_distance )
        {
            *next_closest_distance = next_distance[Z];
            found_exit = TRUE;
        }
    }
}

#ifdef DEBUGGING
static  int  count = 0;
#endif

public  BOOLEAN  find_boundary_in_direction(
    Volume                      volume,
    Volume                      label_volume,
    voxel_coef_struct           *lookup,
    bitlist_3d_struct           *done_bits,
    bitlist_3d_struct           *surface_bits,
    Real                        model_dist,
    Point                       *ray_origin,
    Vector                      *unit_pos_dir,
    Vector                      *unit_neg_dir,
    Real                        max_outwards_search_distance,
    Real                        max_inwards_search_distance,
    int                         degrees_continuity,
    boundary_definition_struct  *boundary_def,
    Real                        *boundary_distance )
{
    private const Real  TOLERANCE = 0.0001;
    BOOLEAN       found, done0, done1, second_leg0, second_leg1, isovalue;
    Real          next_closest0, next_closest1;
    int           voxel_index0[N_DIMENSIONS], voxel_index1[N_DIMENSIONS];
    int           delta_voxel0[N_DIMENSIONS], delta_voxel1[N_DIMENSIONS];
    Real          current_distance0, current_distance1;
    Real          next_distance0[N_DIMENSIONS], next_distance1[N_DIMENSIONS];
    Real          stop_distance0, stop_distance1;
    Real          delta_distance0[N_DIMENSIONS], delta_distance1[N_DIMENSIONS];
    Real          max_dist, found_dist;
    Real          ray_origin_real[N_DIMENSIONS];
    Real          unit_pos[N_DIMENSIONS], unit_neg[N_DIMENSIONS];
    BOOLEAN       inside_surface0, inside_surface1, do_first;
    Real          origin0[N_DIMENSIONS], origin1[N_DIMENSIONS];
    Real          direction0[N_DIMENSIONS], direction1[N_DIMENSIONS];

#ifdef DEBUGGING
    {
        static  int stop_at = -2;

        if( stop_at == -2 )
        {
            if( getenv("STOP") == NULL ||
                sscanf( getenv("STOP"), "%d", &stop_at ) != 1)
                stop_at = -1;
        }

        ++count;

        if( count == stop_at )
        {
            (void) printf( "Stop: %d\n", stop_at );
            (void) getchar();
        }
    }
#endif

    ray_origin_real[X] = (Real) Point_x(*ray_origin);
    unit_pos[X] = (Real) Vector_x(*unit_pos_dir);
    unit_neg[X] = (Real) Vector_x(*unit_neg_dir);

    ray_origin_real[Y] = (Real) Point_y(*ray_origin);
    unit_pos[Y] = (Real) Vector_y(*unit_pos_dir);
    unit_neg[Y] = (Real) Vector_y(*unit_neg_dir);

    ray_origin_real[Z] = (Real) Point_z(*ray_origin);
    unit_pos[Z] = (Real) Vector_z(*unit_pos_dir);
    unit_neg[Z] = (Real) Vector_z(*unit_neg_dir);

    second_leg0 = FALSE;
    second_leg1 = FALSE;

    if( model_dist >= 0.0 )
    {
        set_up_to_search_ray( volume, degrees_continuity,
                              ray_origin_real, unit_pos, unit_neg,
                              model_dist, 0.0, max_outwards_search_distance,
                              origin0, direction0, &current_distance0,
                              &stop_distance0, next_distance0,
                              delta_distance0, voxel_index0, delta_voxel0,
                              &next_closest0 );
    }
    else
    {
        set_up_to_search_ray( volume, degrees_continuity,
                              ray_origin_real, unit_pos, unit_neg,
                              model_dist,
                              0.0,
                              MIN( max_outwards_search_distance, -model_dist),
                              origin0, direction0, &current_distance0,
                              &stop_distance0, next_distance0,
                              delta_distance0, voxel_index0, delta_voxel0,
                              &next_closest0 );
        if( -model_dist < max_outwards_search_distance )
            second_leg0 = TRUE;
    }

    if( model_dist <= 0.0 )
    {
        set_up_to_search_ray( volume, degrees_continuity,
                              ray_origin_real, unit_pos, unit_neg,
                              model_dist, 0.0, -max_inwards_search_distance,
                              origin1, direction1, &current_distance1,
                              &stop_distance1, next_distance1,
                              delta_distance1, voxel_index1, delta_voxel1,
                              &next_closest1 );
    }
    else
    {
        set_up_to_search_ray( volume, degrees_continuity,
                              ray_origin_real, unit_pos, unit_neg,
                              model_dist,
                              0.0,
                              - MIN( max_inwards_search_distance, model_dist ),
                              origin1, direction1, &current_distance1,
                              &stop_distance1, next_distance1,
                              delta_distance1, voxel_index1, delta_voxel1,
                              &next_closest1 );
        if( model_dist < max_inwards_search_distance )
            second_leg1 = TRUE;
    }

    if( boundary_def->min_isovalue != boundary_def->max_isovalue )
    {
        Vector   world_direction;
        Real     point[N_DIMENSIONS];
        Real     vx, vy, vz;

        isovalue = FALSE;

        convert_voxel_normal_vector_to_world( volume, direction0,
                                              &vx, &vy, &vz );
        fill_Vector( world_direction, vx, vy, vz );
        NORMALIZE_VECTOR( world_direction, world_direction );
        GET_RAY_POINT( point, origin0, direction0, current_distance0 );
        inside_surface0 = is_point_inside_surface( volume, label_volume,
                                                   degrees_continuity,
                                                   point, &world_direction,
                                                   boundary_def );

        convert_voxel_normal_vector_to_world( volume, direction1,
                                              &vx, &vy, &vz );
        fill_Vector( world_direction, vx, vy, vz );
        NORMALIZE_VECTOR( world_direction, world_direction );
        SCALE_VECTOR( world_direction, world_direction, -1.0 );

        GET_RAY_POINT( point, origin1, direction1, current_distance1 );

        inside_surface1 = is_point_inside_surface( volume, label_volume,
                                                   degrees_continuity,
                                                   point, &world_direction,
                                                   boundary_def );
    }
    else
        isovalue = TRUE;

    found = FALSE;
    done0 = current_distance0 >= stop_distance0 - TOLERANCE;
    done1 = current_distance1 >= stop_distance1 - TOLERANCE;

    do_first = ( done1 || (!done0 && current_distance0 <= current_distance1));

    while( !done0 || !done1 )
    {
        if( do_first )
        {
            max_dist = next_closest0;
            if( stop_distance0 < max_dist )
                max_dist = stop_distance0;

            if( voxel_might_contain_boundary( volume, done_bits, surface_bits,
                                              degrees_continuity, voxel_index0,
                                              boundary_def ) &&
                (isovalue &&
                 check_voxel_for_isovalue( lookup, volume, label_volume,
                                           degrees_continuity,
                                           voxel_index0,
                                           origin0, direction0,
                                           FALSE,
                                           current_distance0, max_dist,
                                           boundary_def->min_isovalue,
                                           boundary_def->normal_direction,
                                           found, &found_dist ) ||
                !isovalue &&
                check_voxel_for_boundary( lookup, volume, label_volume,
                                          degrees_continuity,
                                          voxel_index0,
                                          origin0, direction0,
                                          &inside_surface0,
                                          FALSE,
                                          current_distance0, max_dist,
                                          boundary_def, found, &found_dist ) ) )
            {
                found = TRUE;
                *boundary_distance = found_dist;
            }

            if( next_closest0 >= stop_distance0 - TOLERANCE )
            {
                if( second_leg0 )
                {
                    second_leg0 = FALSE;
                    set_up_to_search_ray(
                              volume, degrees_continuity, ray_origin_real,
                              unit_pos, unit_neg, model_dist,
                              -model_dist, max_outwards_search_distance,
                              origin0, direction0, &current_distance0,
                              &stop_distance0, next_distance0,
                              delta_distance0, voxel_index0, delta_voxel0,
                              &next_closest0 );
                    done0 = current_distance0 >= stop_distance0 - TOLERANCE;
                }
                else
                    done0 = TRUE;

                do_first = (done1 ||
                            (!done0 && current_distance0 <= current_distance1));
            }
            else
            {
                current_distance0 = next_closest0;
                do_first = (done1 || current_distance0 <= current_distance1);

                if( next_distance0[X] <= current_distance0 )
                {
                    voxel_index0[X] += delta_voxel0[X];
                    next_distance0[X] += delta_distance0[X];
                }
                if( next_distance0[Y] <= current_distance0 )
                {
                    voxel_index0[Y] += delta_voxel0[Y];
                    next_distance0[Y] += delta_distance0[Y];
                }
                if( next_distance0[Z] <= current_distance0 )
                {
                    voxel_index0[Z] += delta_voxel0[Z];
                    next_distance0[Z] += delta_distance0[Z];
                }

                next_closest0 = next_distance0[X];
                if( next_distance0[Y] < next_closest0 )
                    next_closest0 = next_distance0[Y];
                if( next_distance0[Z] < next_closest0 )
                    next_closest0 = next_distance0[Z];
            }
        }
        else
        {
            max_dist = next_closest1;
            if( stop_distance1 < max_dist )
                max_dist = stop_distance1;

            if( voxel_might_contain_boundary( volume, done_bits, surface_bits,
                                              degrees_continuity, voxel_index1,
                                              boundary_def ) &&
                (isovalue &&
                 check_voxel_for_isovalue( lookup, volume, label_volume,
                                           degrees_continuity,
                                           voxel_index1,
                                           origin1, direction1,
                                           TRUE,
                                           current_distance1, max_dist,
                                           boundary_def->min_isovalue,
                                           boundary_def->normal_direction,
                                           found, &found_dist ) ||
                !isovalue &&
                check_voxel_for_boundary( lookup, volume, label_volume,
                                          degrees_continuity,
                                          voxel_index1,
                                          origin1, direction1,
                                          &inside_surface1,
                                          TRUE,
                                          current_distance1, max_dist,
                                          boundary_def, found, &found_dist ) ) )
            {
                found = TRUE;
                *boundary_distance = found_dist;
            }

            if( next_closest1 >= stop_distance1 - TOLERANCE )
            {
                if( second_leg1 )
                {
                    second_leg1 = FALSE;
                    set_up_to_search_ray(
                              volume, degrees_continuity, ray_origin_real,
                              unit_pos, unit_neg, model_dist,
                              -model_dist, -max_inwards_search_distance,
                              origin1, direction1, &current_distance1,
                              &stop_distance1, next_distance1,
                              delta_distance1, voxel_index1, delta_voxel1,
                              &next_closest1 );
                    done1 = current_distance1 >= stop_distance1 - TOLERANCE;
                }
                else
                    done1 = TRUE;

                do_first = !(done0 ||
                            (!done1 && current_distance1 <= current_distance0));
            }
            else
            {
                current_distance1 = next_closest1;
                do_first = !(done0 || current_distance1 <= current_distance0);

                if( next_distance1[X] <= current_distance1 )
                {
                    voxel_index1[X] += delta_voxel1[X];
                    next_distance1[X] += delta_distance1[X];
                }
                if( next_distance1[Y] <= current_distance1 )
                {
                    voxel_index1[Y] += delta_voxel1[Y];
                    next_distance1[Y] += delta_distance1[Y];
                }
                if( next_distance1[Z] <= current_distance1 )
                {
                    voxel_index1[Z] += delta_voxel1[Z];
                    next_distance1[Z] += delta_distance1[Z];
                }

                next_closest1 = next_distance1[X];
                if( next_distance1[Y] < next_closest1 )
                    next_closest1 = next_distance1[Y];
                if( next_distance1[Z] < next_closest1 )
                    next_closest1 = next_distance1[Z];
            }
        }

        if( found &&
            (done0 || FABS(*boundary_distance) <= current_distance0) &&
            (done1 || FABS(*boundary_distance) <= current_distance1) )
        {
            break;
        }
    }

    return( found );
}

private  void   get_trilinear_gradient(
    Real   coefs[],
    Real   u,
    Real   v,
    Real   w,
    Real   derivs[] )
{
    Real  du00, du01, du10, du11, c00, c01, c10, c11;
    Real  dv0, dv1, c0, c1, dw, du0, du1;

    /*--- get the 4 differences in the u direction */

    du00 = coefs[4] - coefs[0];
    du01 = coefs[5] - coefs[1];
    du10 = coefs[6] - coefs[2];
    du11 = coefs[7] - coefs[3];

    /*--- reduce to a 2D problem, by interpolating in the u direction */

    c00 = coefs[0] + u * du00;
    c01 = coefs[1] + u * du01;
    c10 = coefs[2] + u * du10;
    c11 = coefs[3] + u * du11;

    /*--- get the 2 differences in the v direction for the 2D problem */

    dv0 = c10 - c00;
    dv1 = c11 - c01;
    /*--- reduce 2D to a 1D problem, by interpolating in the v direction */

    c0 = c00 + v * dv0;
    c1 = c01 + v * dv1;

    /*--- get the 1 difference in the w direction for the 1D problem */

    dw = c1 - c0;

    /*--- reduce the 2D u derivs to 1D */

    du0 = INTERPOLATE( v, du00, du10 );
    du1 = INTERPOLATE( v, du01, du11 );

    /*--- interpolate the 1D problems in w, or for Z deriv, just use dw */

    derivs[X] = INTERPOLATE( w, du0, du1 );
    derivs[Y] = INTERPOLATE( w, dv0, dv1 );
    derivs[Z] = dw;
}

#define  MAX_DERIVS   2

private  BOOLEAN  check_voxel_for_isovalue(
    voxel_coef_struct   *lookup,
    Volume              volume,
    Volume              label_volume,
    int                 degrees_continuity,
    int                 voxel_indices[],
    Real                line_origin[],
    Real                line_direction[],
    BOOLEAN             searching_inwards,
    Real                min_line_t,
    Real                max_line_t,
    Real                isovalue,
    Normal_directions   normal_direction,
    BOOLEAN             already_found,
    Real                *dist_found )
{
    BOOLEAN         found;
    int             i;
    Real            value, dot_prod, voxel[N_DIMENSIONS];
    Real            first_deriv[N_DIMENSIONS], *first_deriv_ptr[1];
    BOOLEAN         active, deriv_dir_correct;
    int             n_boundaries;
    Real            boundary_positions[3];
    Real            coefs[(2+MAX_DERIVS)*(2+MAX_DERIVS)*(2+MAX_DERIVS)];

    found = FALSE;

    lookup_volume_coeficients( lookup, volume, degrees_continuity,
                               voxel_indices[X],
                               voxel_indices[Y],
                               voxel_indices[Z], coefs );

    n_boundaries = find_voxel_line_value_intersection( coefs,
                                            degrees_continuity,
                                            voxel_indices[X],
                                            voxel_indices[Y],
                                            voxel_indices[Z],
                                            line_origin,
                                            line_direction,
                                            min_line_t,
                                            max_line_t,
                                            isovalue,
                                            boundary_positions );

    for_less( i, 0, n_boundaries )
    {
        if( (!already_found || boundary_positions[i]< FABS(*dist_found)) )
        {
            GET_RAY_POINT( voxel, line_origin, line_direction,
                           boundary_positions[i] );
            active = get_volume_voxel_activity( label_volume, voxel, FALSE );
            if( active )
            {
                if( degrees_continuity == 0 )
                {
                    get_trilinear_gradient( coefs,
                                            voxel[X] - (Real) voxel_indices[X],
                                            voxel[Y] - (Real) voxel_indices[Y],
                                            voxel[Z] - (Real) voxel_indices[Z],
                                            first_deriv );
                }
                else
                {
                    first_deriv_ptr[0] = first_deriv;
                    (void) evaluate_volume( volume, voxel, NULL,
                                            degrees_continuity, FALSE,
                                            get_volume_real_min(volume), &value,
                                            first_deriv_ptr, NULL );
                }

                deriv_dir_correct = TRUE;
                if( normal_direction != ANY_DIRECTION )
                {
                    dot_prod = first_deriv[X] * line_direction[X] +
                               first_deriv[Y] * line_direction[Y] +
                               first_deriv[Z] * line_direction[Z];
                    if( searching_inwards )
                        dot_prod = -dot_prod;
                    if( normal_direction == TOWARDS_LOWER && dot_prod > 0.0 ||
                        normal_direction == TOWARDS_HIGHER && dot_prod < 0.0 )
                    {
                        deriv_dir_correct = FALSE;
                    }
                }

                if( deriv_dir_correct )
                {
                    if( searching_inwards )
                        *dist_found = -boundary_positions[i];
                    else
                        *dist_found = boundary_positions[i];
                    found = TRUE;
                }
            }
        }
    }

    return( found );
}

private  BOOLEAN  does_voxel_contain_value_range(
    Volume          volume,
    int             degrees_continuity,
    int             voxel[],
    Real            min_value,
    Real            max_value )
{
    int      dim, i, n_values, start, end, sizes[MAX_DIMENSIONS];
    BOOLEAN  greater, less;
    Real     values[4*4*4];

    get_volume_sizes( volume, sizes );

    /*--- special case, to be done fast */

    if( degrees_continuity == 0 )
    {
        if( voxel[0] < 0 || voxel[0] >= sizes[0]-1 ||
            voxel[1] < 0 || voxel[1] >= sizes[1]-1 ||
            voxel[2] < 0 || voxel[2] >= sizes[2]-1 )
            return( FALSE );

        get_volume_value_hyperslab_3d( volume, voxel[X], voxel[Y], voxel[Z],
                                       2, 2, 2, values );

        if( min_value <= values[0] && values[0] <= max_value )
            return( TRUE );

        if( values[0] < min_value )
        {
            return( values[1] >= min_value ||
                    values[2] >= min_value ||
                    values[3] >= min_value ||
                    values[4] >= min_value ||
                    values[5] >= min_value ||
                    values[6] >= min_value ||
                    values[7] >= min_value );
        }
        else
        {
            return( values[1] <= max_value ||
                    values[2] <= max_value ||
                    values[3] <= max_value ||
                    values[4] <= max_value ||
                    values[5] <= max_value ||
                    values[6] <= max_value ||
                    values[7] <= max_value );
        }
    }

    start = -(degrees_continuity + 1) / 2;
    end = start + degrees_continuity + 2;

    for_less( dim, 0, N_DIMENSIONS )
    {
        if( voxel[dim] + start < 0 || voxel[dim] + end > sizes[dim] )
            return( FALSE );
    }

    get_volume_value_hyperslab_3d( volume, voxel[X], voxel[Y], voxel[Z],
                                   degrees_continuity + 2,
                                   degrees_continuity + 2,
                                   degrees_continuity + 2, values );

    n_values = (degrees_continuity + 2) *
               (degrees_continuity + 2) *
               (degrees_continuity + 2);

    less = FALSE;
    greater = FALSE;

    for_less( i, 0, n_values )
    {
        if( values[i] < min_value )
        {
            if( greater )
                return( TRUE );
            less = TRUE;
        }
        else if( values[i] > max_value )
        {
            if( less )
                return( TRUE );
            greater = TRUE;
        }
        else
            return( TRUE );
    }

    return( FALSE );
}

private  BOOLEAN  voxel_might_contain_boundary(
    Volume                      volume,
    bitlist_3d_struct           *done_bits,
    bitlist_3d_struct           *surface_bits,
    int                         degrees_continuity,
    int                         voxel_indices[],
    boundary_definition_struct  *boundary_def )
{
    BOOLEAN         contains;

    if( done_bits != NULL )
    {
        if( !get_bitlist_bit_3d( done_bits,
                                 voxel_indices[X],
                                 voxel_indices[Y],
                                 voxel_indices[Z] ) )
        {
            contains = does_voxel_contain_value_range(
                           volume, degrees_continuity,
                           voxel_indices,
                           boundary_def->min_isovalue,
                           boundary_def->max_isovalue );
 
            set_bitlist_bit_3d( surface_bits,
                                voxel_indices[X],
                                voxel_indices[Y],
                                voxel_indices[Z], contains );
            set_bitlist_bit_3d( done_bits,
                                voxel_indices[X],
                                voxel_indices[Y],
                                voxel_indices[Z], TRUE );
        }
        else
        {
            contains = get_bitlist_bit_3d( surface_bits,
                                           voxel_indices[X],
                                           voxel_indices[Y],
                                           voxel_indices[Z] );
        }

        if( !contains )
            return( FALSE );
    }

    return( TRUE );
}

private  BOOLEAN  check_voxel_for_boundary(
    voxel_coef_struct           *lookup,
    Volume                      volume,
    Volume                      label_volume,
    int                         degrees_continuity,
    int                         voxel_indices[],
    Real                        line_origin[],
    Real                        line_direction[],
    BOOLEAN                     *inside_surface,
    BOOLEAN                     searching_inwards,
    Real                        min_line_t,
    Real                        max_line_t,
    boundary_definition_struct  *boundary_def,
    BOOLEAN                     already_found,
    Real                        *dist_found )
{
    BOOLEAN         found, inside, prev_inside;
    int             degrees;
    Real            line_poly[10], value, vx, vy, vz;
    Real            t, increment, t_min, t_max;
    Vector          direction;
    Real            surface[N_DIMENSIONS];
    Real            coefs[(2+MAX_DERIVS)*(2+MAX_DERIVS)*(2+MAX_DERIVS)];

    found = FALSE;

    lookup_volume_coeficients( lookup, volume, degrees_continuity,
                               voxel_indices[X],
                               voxel_indices[Y],
                               voxel_indices[Z], coefs );

    degrees = find_voxel_line_polynomial( coefs, degrees_continuity,
                                          voxel_indices[X],
                                          voxel_indices[Y],
                                          voxel_indices[Z],
                                          line_origin,
                                          line_direction,
                                          line_poly );

    if( degrees == 0 )
    {
        if( *inside_surface )
        {
            if( !already_found || min_line_t < FABS(*dist_found) )
            {
                if( searching_inwards )
                    *dist_found = -min_line_t;
                else
                    *dist_found = min_line_t;
                found = TRUE;
            }
        }
        *inside_surface = FALSE;
        return( found );
    }

    convert_voxel_normal_vector_to_world( volume, line_direction,
                                          &vx, &vy, &vz );
    fill_Vector( direction, vx, vy, vz );
    NORMALIZE_VECTOR( direction, direction );
    if( searching_inwards )
        SCALE_VECTOR( direction, direction, -1.0 );

    if( !get_range_of_polynomial( degrees, line_poly, min_line_t, max_line_t,
                                  boundary_def->min_isovalue,
                                  boundary_def->max_isovalue,
                                  boundary_def->tolerance, &t_min, &t_max ) )
    {

        GET_RAY_POINT( surface, line_origin, line_direction,
                          (min_line_t + max_line_t) / 2.0 );

        inside = is_point_inside_surface( volume, label_volume,
                                          degrees_continuity,
                                          surface, &direction,
                                          boundary_def );

        if( inside != *inside_surface )
        {
            if( !already_found || min_line_t < FABS(*dist_found) )
            {
                if( searching_inwards )
                    *dist_found = -min_line_t;
                else
                    *dist_found = min_line_t;
                found = TRUE;
            }
        }

        *inside_surface = inside;

        return( found );
    }

    found = FALSE;

    increment = boundary_def->tolerance;

    if( t_min > min_line_t )
        inside = FALSE;
    else
        inside = *inside_surface;

    for( t = t_min;  t <= t_max;  t += increment )
    {
        prev_inside = inside;

        GET_RAY_POINT( surface, line_origin, line_direction, t );

        value = evaluate_polynomial( degrees, line_poly, t );

        if( value < boundary_def->min_isovalue )
            inside = FALSE;
        else if( value > boundary_def->max_isovalue )
            inside = TRUE;
        else
            inside = is_point_inside_surface( volume, label_volume,
                                              degrees_continuity,
                                              surface, &direction,
                                              boundary_def );

        if( inside != prev_inside )
        {
            if( !already_found || t < FABS(*dist_found) )
            {
                if( searching_inwards )
                    *dist_found = -t;
                else
                    *dist_found = t;
                found = TRUE;
                break;
            }
        }
    }

    if( t_max != max_line_t )
        *inside_surface = FALSE;
    else
        *inside_surface = inside;

    return( found );
}
