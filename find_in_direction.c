#include  <volume_io.h>
#include  <deform.h>

static  VIO_BOOL  voxel_might_contain_boundary(
    VIO_Volume                      volume,
    bitlist_3d_struct           *done_bits,
    bitlist_3d_struct           *surface_bits,
    int                         degrees_continuity,
    int                         voxel_indices[],
    boundary_definition_struct  *boundary_def );

static  VIO_BOOL  check_voxel_for_isovalue(
    voxel_coef_struct   *lookup,
    VIO_Volume              volume,
    VIO_Volume              label_volume,
    int                 degrees_continuity,
    int                 voxel_indices[],
    VIO_Real                line_origin[],
    VIO_Real                line_direction[],
    VIO_BOOL             searching_inwards,
    VIO_Real                min_line_t,
    VIO_Real                max_line_t,
    VIO_Real                isovalue,
    Normal_directions   normal_direction,
    VIO_BOOL             already_found,
    VIO_Real                *dist_found );

static  VIO_BOOL  check_voxel_for_boundary(
    voxel_coef_struct           *lookup,
    VIO_Volume                      volume,
    VIO_Volume                      label_volume,
    int                         degrees_continuity,
    int                         voxel_indices[],
    VIO_Real                        line_origin[],
    VIO_Real                        line_direction[],
    VIO_BOOL                     *inside_surface,
    VIO_BOOL                     searching_inwards,
    VIO_Real                        min_line_t,
    VIO_Real                        max_line_t,
    boundary_definition_struct  *boundary_def,
    VIO_BOOL                     already_found,
    VIO_Real                        *dist_found );

#define  GET_RAY_POINT( result, origin, direction, dist ) \
    { \
        (result)[VIO_X] = (origin)[VIO_X] + (dist) * (direction)[VIO_X]; \
        (result)[VIO_Y] = (origin)[VIO_Y] + (dist) * (direction)[VIO_Y]; \
        (result)[VIO_Z] = (origin)[VIO_Z] + (dist) * (direction)[VIO_Z]; \
    }

static  void  clip_line_to_volume(
    VIO_Volume     volume,
    int        degrees_continuity,
    VIO_Real       line_origin[],
    VIO_Real       line_direction[],
    VIO_Real       *t_min,
    VIO_Real       *t_max )
{
    int    sizes[VIO_N_DIMENSIONS];
    VIO_Point  origin;
    VIO_Vector direction;

    get_volume_sizes( volume, sizes );

    fill_Point( origin, line_origin[VIO_X], line_origin[VIO_Y], line_origin[VIO_Z] );
    fill_Point( direction,
                line_direction[VIO_X], line_direction[VIO_Y], line_direction[VIO_Z] );

    (void) clip_line_to_box( &origin, &direction,
                             0.0 + (VIO_Real) degrees_continuity * 0.5,
                             (VIO_Real) sizes[VIO_X]-1.0 - (VIO_Real)degrees_continuity*0.5,
                             0.0 + (VIO_Real) degrees_continuity * 0.5,
                             (VIO_Real) sizes[VIO_Y]-1.0 - (VIO_Real)degrees_continuity*0.5,
                             0.0 + (VIO_Real) degrees_continuity * 0.5,
                             (VIO_Real) sizes[VIO_Z]-1.0 - (VIO_Real)degrees_continuity*0.5,
                             t_min, t_max );
}

static  void  set_up_to_search_ray(
    VIO_Volume                      volume,
    int                         degrees_continuity,
    VIO_Real                        ray_origin[],
    VIO_Real                        unit_pos_dir[],
    VIO_Real                        unit_neg_dir[],
    VIO_Real                        model_dist,
    VIO_Real                        start_dist,
    VIO_Real                        end_dist,
    VIO_Real                        origin[],
    VIO_Real                        direction[],
    VIO_Real                        *current_distance,
    VIO_Real                        *stop_distance,
    VIO_Real                        next_distance[VIO_N_DIMENSIONS],
    VIO_Real                        delta_distance[VIO_N_DIMENSIONS],
    int                         voxel_index[VIO_N_DIMENSIONS],
    int                         delta_voxel[VIO_N_DIMENSIONS],
    VIO_Real                        *next_closest_distance )
{
    VIO_BOOL       found_exit;
    VIO_Real          voxel_exit, t_min, t_max, voxel_offset;
    VIO_Real          direct;
    VIO_Real          start_voxel[VIO_N_DIMENSIONS], model_point[VIO_N_DIMENSIONS];
    VIO_Real          *dir;

    if( model_dist + start_dist < 0.0 || model_dist + end_dist < 0.0 )
        dir = &unit_neg_dir[0];
    else
        dir = &unit_pos_dir[0];

    GET_RAY_POINT( model_point, ray_origin, dir, model_dist );

    convert_world_to_voxel( volume,
                            model_point[VIO_X], model_point[VIO_Y], model_point[VIO_Z],
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
                            direction[VIO_X], 
                            direction[VIO_Y],
                            direction[VIO_Z], direction );
    direction[VIO_X] -= origin[VIO_X];
    direction[VIO_Y] -= origin[VIO_Y];
    direction[VIO_Z] -= origin[VIO_Z];

    clip_line_to_volume( volume, degrees_continuity,
                         origin, direction, &t_min, &t_max );

    *current_distance = VIO_FABS(start_dist);
    if( t_min > *current_distance )
        *current_distance = t_min;
    *stop_distance = VIO_FABS(end_dist);
    if( t_max < *stop_distance )
        *stop_distance = t_max;

    GET_RAY_POINT( start_voxel, origin, direction, *current_distance );

    found_exit = FALSE;

    if( degrees_continuity % 2 == 1 )
        voxel_offset = 0.5;
    else
        voxel_offset = 0.0;

    /*--- do x loop */

    direct = direction[VIO_X];

    voxel_index[VIO_X] = (int) (start_voxel[VIO_X]+voxel_offset);
    voxel_exit = (VIO_Real) voxel_index[VIO_X] - voxel_offset;

    if( direct == 0.0 )
    {
        delta_voxel[VIO_X] = 1;
        delta_distance[VIO_X] = 0.0;
        next_distance[VIO_X] = *stop_distance;
    }
    else
    {
        direct = 1.0 / direct;
        if( direct < 0.0 )
        {
            delta_voxel[VIO_X] = -1;
            delta_distance[VIO_X] = -direct;
        }
        else
        {
            voxel_exit += 1.0;
            delta_voxel[VIO_X] = 1;
            delta_distance[VIO_X] = direct;
        }

        next_distance[VIO_X] = (voxel_exit - origin[VIO_X]) * direct;

        *next_closest_distance = next_distance[VIO_X];
        found_exit = TRUE;
    }
    /*--- do y loop */

    direct = direction[VIO_Y];

    voxel_index[VIO_Y] = (int) (start_voxel[VIO_Y]+voxel_offset);
    voxel_exit = (VIO_Real) voxel_index[VIO_Y] - voxel_offset;

    if( direct == 0.0 )
    {
        delta_voxel[VIO_Y] = 1;
        delta_distance[VIO_Y] = 0.0;
        next_distance[VIO_Y] = *stop_distance;
    }
    else
    {
        direct = 1.0 / direct;
        if( direct < 0.0 )
        {
            delta_voxel[VIO_Y] = -1;
            delta_distance[VIO_Y] = -direct;
        }
        else
        {
            voxel_exit += 1.0;
            delta_voxel[VIO_Y] = 1;
            delta_distance[VIO_Y] = direct;
        }

        next_distance[VIO_Y] = (voxel_exit - origin[VIO_Y]) * direct;

        if( !found_exit || next_distance[VIO_Y] < *next_closest_distance )
        {
            *next_closest_distance = next_distance[VIO_Y];
            found_exit = TRUE;
        }
    }
    /*--- do z loop */

    direct = direction[VIO_Z];

    voxel_index[VIO_Z] = (int) (start_voxel[VIO_Z]+voxel_offset);
    voxel_exit = (VIO_Real) voxel_index[VIO_Z] - voxel_offset;

    if( direct == 0.0 )
    {
        delta_voxel[VIO_Z] = 1;
        delta_distance[VIO_Z] = 0.0;
        next_distance[VIO_Z] = *stop_distance;
    }
    else
    {
        direct = 1.0 / direct;
        if( direct < 0.0 )
        {
            delta_voxel[VIO_Z] = -1;
            delta_distance[VIO_Z] = -direct;
        }
        else
        {
            voxel_exit += 1.0;
            delta_voxel[VIO_Z] = 1;
            delta_distance[VIO_Z] = direct;
        }

        next_distance[VIO_Z] = (voxel_exit - origin[VIO_Z]) * direct;

        if( !found_exit || next_distance[VIO_Z] < *next_closest_distance )
        {
            *next_closest_distance = next_distance[VIO_Z];
            found_exit = TRUE;
        }
    }
}

#ifdef DEBUGGING
static  int  count = 0;
#endif

  VIO_BOOL  find_boundary_in_direction(
    VIO_Volume                      volume,
    VIO_Volume                      label_volume,
    voxel_coef_struct           *lookup,
    bitlist_3d_struct           *done_bits,
    bitlist_3d_struct           *surface_bits,
    VIO_Real                        model_dist,
    VIO_Point                       *ray_origin,
    VIO_Vector                      *unit_pos_dir,
    VIO_Vector                      *unit_neg_dir,
    VIO_Real                        max_outwards_search_distance,
    VIO_Real                        max_inwards_search_distance,
    int                         degrees_continuity,
    boundary_definition_struct  *boundary_def,
    VIO_Real                        *boundary_distance )
{
    static const VIO_Real  TOLERANCE = 0.0001;
    VIO_BOOL       found, done0, done1, second_leg0, second_leg1, isovalue;
    VIO_Real          next_closest0, next_closest1;
    int           voxel_index0[VIO_N_DIMENSIONS], voxel_index1[VIO_N_DIMENSIONS];
    int           delta_voxel0[VIO_N_DIMENSIONS], delta_voxel1[VIO_N_DIMENSIONS];
    VIO_Real          current_distance0, current_distance1;
    VIO_Real          next_distance0[VIO_N_DIMENSIONS], next_distance1[VIO_N_DIMENSIONS];
    VIO_Real          stop_distance0, stop_distance1;
    VIO_Real          delta_distance0[VIO_N_DIMENSIONS], delta_distance1[VIO_N_DIMENSIONS];
    VIO_Real          max_dist, found_dist;
    VIO_Real          ray_origin_real[VIO_N_DIMENSIONS];
    VIO_Real          unit_pos[VIO_N_DIMENSIONS], unit_neg[VIO_N_DIMENSIONS];
    VIO_BOOL       inside_surface0, inside_surface1, do_first;
    VIO_Real          origin0[VIO_N_DIMENSIONS], origin1[VIO_N_DIMENSIONS];
    VIO_Real          direction0[VIO_N_DIMENSIONS], direction1[VIO_N_DIMENSIONS];

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

    ray_origin_real[VIO_X] = (VIO_Real) Point_x(*ray_origin);
    unit_pos[VIO_X] = (VIO_Real) Vector_x(*unit_pos_dir);
    unit_neg[VIO_X] = (VIO_Real) Vector_x(*unit_neg_dir);

    ray_origin_real[VIO_Y] = (VIO_Real) Point_y(*ray_origin);
    unit_pos[VIO_Y] = (VIO_Real) Vector_y(*unit_pos_dir);
    unit_neg[VIO_Y] = (VIO_Real) Vector_y(*unit_neg_dir);

    ray_origin_real[VIO_Z] = (VIO_Real) Point_z(*ray_origin);
    unit_pos[VIO_Z] = (VIO_Real) Vector_z(*unit_pos_dir);
    unit_neg[VIO_Z] = (VIO_Real) Vector_z(*unit_neg_dir);

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
        VIO_Vector   world_direction;
        VIO_Real     point[VIO_N_DIMENSIONS];
        VIO_Real     vx, vy, vz;

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

                if( next_distance0[VIO_X] <= current_distance0 )
                {
                    voxel_index0[VIO_X] += delta_voxel0[VIO_X];
                    next_distance0[VIO_X] += delta_distance0[VIO_X];
                }
                if( next_distance0[VIO_Y] <= current_distance0 )
                {
                    voxel_index0[VIO_Y] += delta_voxel0[VIO_Y];
                    next_distance0[VIO_Y] += delta_distance0[VIO_Y];
                }
                if( next_distance0[VIO_Z] <= current_distance0 )
                {
                    voxel_index0[VIO_Z] += delta_voxel0[VIO_Z];
                    next_distance0[VIO_Z] += delta_distance0[VIO_Z];
                }

                next_closest0 = next_distance0[VIO_X];
                if( next_distance0[VIO_Y] < next_closest0 )
                    next_closest0 = next_distance0[VIO_Y];
                if( next_distance0[VIO_Z] < next_closest0 )
                    next_closest0 = next_distance0[VIO_Z];
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

                if( next_distance1[VIO_X] <= current_distance1 )
                {
                    voxel_index1[VIO_X] += delta_voxel1[VIO_X];
                    next_distance1[VIO_X] += delta_distance1[VIO_X];
                }
                if( next_distance1[VIO_Y] <= current_distance1 )
                {
                    voxel_index1[VIO_Y] += delta_voxel1[VIO_Y];
                    next_distance1[VIO_Y] += delta_distance1[VIO_Y];
                }
                if( next_distance1[VIO_Z] <= current_distance1 )
                {
                    voxel_index1[VIO_Z] += delta_voxel1[VIO_Z];
                    next_distance1[VIO_Z] += delta_distance1[VIO_Z];
                }

                next_closest1 = next_distance1[VIO_X];
                if( next_distance1[VIO_Y] < next_closest1 )
                    next_closest1 = next_distance1[VIO_Y];
                if( next_distance1[VIO_Z] < next_closest1 )
                    next_closest1 = next_distance1[VIO_Z];
            }
        }

        if( found &&
            (done0 || VIO_FABS(*boundary_distance) <= current_distance0) &&
            (done1 || VIO_FABS(*boundary_distance) <= current_distance1) )
        {
            break;
        }
    }

    return( found );
}

static  void   get_trilinear_gradient(
    VIO_Real   coefs[],
    VIO_Real   u,
    VIO_Real   v,
    VIO_Real   w,
    VIO_Real   derivs[] )
{
    VIO_Real  du00, du01, du10, du11, c00, c01, c10, c11;
    VIO_Real  dv0, dv1, c0, c1, dw, du0, du1;

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

    du0 = VIO_INTERPOLATE( v, du00, du10 );
    du1 = VIO_INTERPOLATE( v, du01, du11 );

    /*--- interpolate the 1D problems in w, or for VIO_Z deriv, just use dw */

    derivs[VIO_X] = VIO_INTERPOLATE( w, du0, du1 );
    derivs[VIO_Y] = VIO_INTERPOLATE( w, dv0, dv1 );
    derivs[VIO_Z] = dw;
}

#define  MAX_DERIVS   2

static  VIO_BOOL  check_voxel_for_isovalue(
    voxel_coef_struct   *lookup,
    VIO_Volume              volume,
    VIO_Volume              label_volume,
    int                 degrees_continuity,
    int                 voxel_indices[],
    VIO_Real                line_origin[],
    VIO_Real                line_direction[],
    VIO_BOOL             searching_inwards,
    VIO_Real                min_line_t,
    VIO_Real                max_line_t,
    VIO_Real                isovalue,
    Normal_directions   normal_direction,
    VIO_BOOL             already_found,
    VIO_Real                *dist_found )
{
    VIO_BOOL         found;
    int             i;
    VIO_Real            value, dot_prod, voxel[VIO_N_DIMENSIONS];
    VIO_Real            first_deriv[VIO_N_DIMENSIONS], *first_deriv_ptr[1];
    VIO_BOOL         active, deriv_dir_correct;
    int             n_boundaries;
    VIO_Real            boundary_positions[3];
    VIO_Real            coefs[(2+MAX_DERIVS)*(2+MAX_DERIVS)*(2+MAX_DERIVS)];

    found = FALSE;

    lookup_volume_coeficients( lookup, volume, degrees_continuity,
                               voxel_indices[VIO_X],
                               voxel_indices[VIO_Y],
                               voxel_indices[VIO_Z], coefs );

    n_boundaries = find_voxel_line_value_intersection( coefs,
                                            degrees_continuity,
                                            voxel_indices[VIO_X],
                                            voxel_indices[VIO_Y],
                                            voxel_indices[VIO_Z],
                                            line_origin,
                                            line_direction,
                                            min_line_t,
                                            max_line_t,
                                            isovalue,
                                            boundary_positions );

    for_less( i, 0, n_boundaries )
    {
        if( (!already_found || boundary_positions[i]< VIO_FABS(*dist_found)) )
        {
            GET_RAY_POINT( voxel, line_origin, line_direction,
                           boundary_positions[i] );
            active = get_volume_voxel_activity( label_volume, voxel, FALSE );
            if( active )
            {
                if( degrees_continuity == 0 )
                {
                    get_trilinear_gradient( coefs,
                                            voxel[VIO_X] - (VIO_Real) voxel_indices[VIO_X],
                                            voxel[VIO_Y] - (VIO_Real) voxel_indices[VIO_Y],
                                            voxel[VIO_Z] - (VIO_Real) voxel_indices[VIO_Z],
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
                    dot_prod = first_deriv[VIO_X] * line_direction[VIO_X] +
                               first_deriv[VIO_Y] * line_direction[VIO_Y] +
                               first_deriv[VIO_Z] * line_direction[VIO_Z];
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

static  VIO_BOOL  does_voxel_contain_value_range(
    VIO_Volume          volume,
    int             degrees_continuity,
    int             voxel[],
    VIO_Real            min_value,
    VIO_Real            max_value )
{
    int      dim, i, n_values, start, end, sizes[VIO_MAX_DIMENSIONS];
    VIO_BOOL  greater, less;
    VIO_Real     values[4*4*4];

    get_volume_sizes( volume, sizes );

    /*--- special case, to be done fast */

    if( degrees_continuity == 0 )
    {
        if( voxel[0] < 0 || voxel[0] >= sizes[0]-1 ||
            voxel[1] < 0 || voxel[1] >= sizes[1]-1 ||
            voxel[2] < 0 || voxel[2] >= sizes[2]-1 )
            return( FALSE );

        get_volume_value_hyperslab_3d( volume, voxel[VIO_X], voxel[VIO_Y], voxel[VIO_Z],
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

    for_less( dim, 0, VIO_N_DIMENSIONS )
    {
        if( voxel[dim] + start < 0 || voxel[dim] + end > sizes[dim] )
            return( FALSE );
    }

    get_volume_value_hyperslab_3d( volume, voxel[VIO_X], voxel[VIO_Y], voxel[VIO_Z],
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

static  VIO_BOOL  voxel_might_contain_boundary(
    VIO_Volume                      volume,
    bitlist_3d_struct           *done_bits,
    bitlist_3d_struct           *surface_bits,
    int                         degrees_continuity,
    int                         voxel_indices[],
    boundary_definition_struct  *boundary_def )
{
    VIO_BOOL         contains;

    if( done_bits != NULL )
    {
        if( !get_bitlist_bit_3d( done_bits,
                                 voxel_indices[VIO_X],
                                 voxel_indices[VIO_Y],
                                 voxel_indices[VIO_Z] ) )
        {
            contains = does_voxel_contain_value_range(
                           volume, degrees_continuity,
                           voxel_indices,
                           boundary_def->min_isovalue,
                           boundary_def->max_isovalue );
 
            set_bitlist_bit_3d( surface_bits,
                                voxel_indices[VIO_X],
                                voxel_indices[VIO_Y],
                                voxel_indices[VIO_Z], contains );
            set_bitlist_bit_3d( done_bits,
                                voxel_indices[VIO_X],
                                voxel_indices[VIO_Y],
                                voxel_indices[VIO_Z], TRUE );
        }
        else
        {
            contains = get_bitlist_bit_3d( surface_bits,
                                           voxel_indices[VIO_X],
                                           voxel_indices[VIO_Y],
                                           voxel_indices[VIO_Z] );
        }

        if( !contains )
            return( FALSE );
    }

    return( TRUE );
}

static  VIO_BOOL  check_voxel_for_boundary(
    voxel_coef_struct           *lookup,
    VIO_Volume                      volume,
    VIO_Volume                      label_volume,
    int                         degrees_continuity,
    int                         voxel_indices[],
    VIO_Real                        line_origin[],
    VIO_Real                        line_direction[],
    VIO_BOOL                     *inside_surface,
    VIO_BOOL                     searching_inwards,
    VIO_Real                        min_line_t,
    VIO_Real                        max_line_t,
    boundary_definition_struct  *boundary_def,
    VIO_BOOL                     already_found,
    VIO_Real                        *dist_found )
{
    VIO_BOOL         found, inside, prev_inside;
    int             degrees;
    VIO_Real            line_poly[10], value, vx, vy, vz;
    VIO_Real            t, increment, t_min, t_max;
    VIO_Vector          direction;
    VIO_Real            surface[VIO_N_DIMENSIONS];
    VIO_Real            coefs[(2+MAX_DERIVS)*(2+MAX_DERIVS)*(2+MAX_DERIVS)];

    found = FALSE;

    lookup_volume_coeficients( lookup, volume, degrees_continuity,
                               voxel_indices[VIO_X],
                               voxel_indices[VIO_Y],
                               voxel_indices[VIO_Z], coefs );

    degrees = find_voxel_line_polynomial( coefs, degrees_continuity,
                                          voxel_indices[VIO_X],
                                          voxel_indices[VIO_Y],
                                          voxel_indices[VIO_Z],
                                          line_origin,
                                          line_direction,
                                          line_poly );

    if( degrees == 0 )
    {
        if( *inside_surface )
        {
            if( !already_found || min_line_t < VIO_FABS(*dist_found) )
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
            if( !already_found || min_line_t < VIO_FABS(*dist_found) )
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
            if( !already_found || t < VIO_FABS(*dist_found) )
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
