#include  <volume_io/internal_volume_io.h>
#include  <deform.h>

public  void  get_model_shape_point(
    Point    *origin,
    Vector   *pos_model_dir,
    Vector   *neg_model_dir,
    Real     dist,
    Point    *point )
{
    Vector   offset;

    if( dist < 0.0 )
    {
        SCALE_VECTOR( offset, *neg_model_dir, dist );
    }
    else
    {
        SCALE_VECTOR( offset, *pos_model_dir, dist );
    }

    ADD_POINT_VECTOR( *point, *origin, offset );
}

public  void  compute_equilibrium_point(
    int                       point_index,
    BOOLEAN                   boundary_exists,
    Real                      boundary_dist,
    Real                      base_length,
    Real                      model_dist,
    Vector                    *pos_model_dir,
    Vector                    *neg_model_dir,
    Point                     *centroid,
    deformation_model_struct  *deformation_model,
    Point                     *equilibrium_point )
{
    Real                  curvature_offset, equil_dist;
    deform_model_struct   *model;

    model = find_relevent_model( deformation_model, point_index );

    /* find equilibrium between model point and boundary point */

    if( !boundary_exists )
        boundary_dist = model_dist;

    equil_dist = INTERPOLATE( model->model_weight, boundary_dist, model_dist );

    if( model->min_curvature_offset <= model->max_curvature_offset )
    {
        curvature_offset = (equil_dist - model_dist) / base_length;

        if( curvature_offset < model->min_curvature_offset ||
            curvature_offset > model->max_curvature_offset )
        {
            if( curvature_offset < model->min_curvature_offset )
                curvature_offset = model->min_curvature_offset;
            else
                curvature_offset = model->max_curvature_offset;

            equil_dist = model_dist + curvature_offset * base_length;
        }
    }

    get_model_shape_point( centroid, pos_model_dir, neg_model_dir,
                           equil_dist, equilibrium_point );
}

public  void  compute_model_dirs(
    Point      *centroid,
    Vector     *normal,
    Real       base_length,
    Point      *model_point,
    Real       *model_dist,
    Point      *search_origin,
    Vector     *pos_model_dir,
    Vector     *neg_model_dir )
{
    Real     to_model_dot_normal, normal_dot_normal, len;
    Real     ratio, left_length, right_length, offset;
    Vector   centroid_to_model, offset_vec, tmp, base_vector, perp;

    if( EQUAL_POINTS( *centroid, *model_point ) )
    {
        *model_dist = 0.0;
        *search_origin = *centroid;
        *pos_model_dir = *normal;
        *neg_model_dir = *normal;
        return;
    }

    SUB_POINTS( centroid_to_model, *model_point, *centroid );
    CROSS_VECTORS( perp, *normal, centroid_to_model );
    CROSS_VECTORS( base_vector, perp, *normal );
    len = MAGNITUDE( base_vector );

    if( len == 0.0 )
    {
        *model_dist = DOT_VECTORS( centroid_to_model, *normal );
        *search_origin = *centroid;
        *pos_model_dir = *normal;
        *neg_model_dir = *normal;
        return;
    }

    SCALE_VECTOR( base_vector, base_vector, base_length / 2.0 / len );

    SUB_VECTORS( offset_vec, centroid_to_model, base_vector );
    left_length = MAGNITUDE( offset_vec );
    ADD_VECTORS( offset_vec, centroid_to_model, base_vector );
    right_length = MAGNITUDE( offset_vec );

    if( left_length + right_length == 0.0 )
    {
        HANDLE_INTERNAL_ERROR( "compute model dirs" );
        *model_dist = 0.0;
        *search_origin = *centroid;
        *pos_model_dir = *normal;
        *neg_model_dir = *normal;
        return;
    }

    ratio = left_length / (left_length + right_length);
    ratio = 1.0 - 2.0 * ratio;

    if( ratio < -0.0001 || ratio > 1.0001 )
    {
        HANDLE_INTERNAL_ERROR( "compute model dirs ratio" );
        *model_dist = 0.0;
        *search_origin = *centroid;
        *pos_model_dir = *normal;
        *neg_model_dir = *normal;
        return;
    }

    SCALE_VECTOR( offset_vec, base_vector, ratio );
    ADD_POINT_VECTOR( *search_origin, *centroid, offset_vec );
    SUB_POINT_VECTOR( *pos_model_dir, *model_point, *search_origin );
    *model_dist = MAGNITUDE( *pos_model_dir );
    NORMALIZE_VECTOR( *pos_model_dir, *pos_model_dir );

    to_model_dot_normal = DOT_VECTORS( *pos_model_dir, *normal );
    normal_dot_normal = DOT_VECTORS( *normal, *normal );
    if( normal_dot_normal == 0.0 )
        normal_dot_normal = 1.0;
    offset = -2.0 * to_model_dot_normal / normal_dot_normal;

    SCALE_VECTOR( offset_vec, *normal, offset );
    ADD_VECTORS( *neg_model_dir, *pos_model_dir, offset_vec );
    NORMALIZE_VECTOR( *neg_model_dir, *neg_model_dir );

    if( DOT_VECTORS( *pos_model_dir, *normal ) < 0.0 )
    {
        tmp = *pos_model_dir;
        *pos_model_dir = *neg_model_dir;
        *neg_model_dir = tmp;
        *model_dist = -(*model_dist);
    }

    SCALE_VECTOR( *neg_model_dir, *neg_model_dir, -1.0 );
}

private  void  get_coords_in_nonortho_system(
    Vector   *v,
    Vector   *v1,
    Vector   *v2,
    Vector   *normalized_v3,
    Real     *d1,
    Real     *d2,
    Real     *d3 )
{
    Real      mag_v1, mag_c, fact;
    Vector    offset, v2d, c;

    *d3 = DOT_VECTORS( *v, *normalized_v3 );

    SCALE_VECTOR( offset, *normalized_v3, *d3 );
    SUB_VECTORS( v2d, *v, offset );

    mag_v1 = MAGNITUDE( *v1 );
    if( mag_v1 == 0.0 )
        mag_v1 = 1.0;

    if( MAGNITUDE( v2d ) / mag_v1 < 1.0e-5 )
    {
        *d2 = 0.0;
        *d1 = 0.0;
        return;
    }

    fact = DOT_VECTORS( v2d, *v1 ) / mag_v1;
    SCALE_VECTOR( offset, *v1, fact/mag_v1 );
    SUB_VECTORS( c, v2d, offset );
    mag_c = MAGNITUDE( c );

    if( mag_c / mag_v1 < 1.0e-5 )
        *d2 = 0.0;
    else
    {
        fact = DOT_VECTORS( *v2, c ) / mag_c;
        if( fact == 0.0 )
            *d2 = 0.0;
        else
            *d2 = mag_c / fact;
    }

    SCALE_VECTOR( offset, *v2, *d2 );
    SUB_VECTORS( c, v2d, offset );
    mag_c = DOT_VECTORS( c, *v1 ) / mag_v1;
    *d1 = mag_c / mag_v1;
}

#define  MAX_MODEL_NEIGHS   2000

private  void  compute_model_point(
    Point   points[],
    int     point_index,
    int     n_neighbours,
    int     neighbours[],
    Point   *centroid,
    Vector  *normal,
    Point   model_points[],
    Point   model_centroids[],
    Vector  model_normals[],
    Point   *model_point )
{
    int      i;
    Real     model_len_to_neighbour, len_to_neighbour, scale_factor;
    Real     d1, d2, d3;
    Vector   to_model_neighbour;
    Vector   to_neighbour, model_offset;
    Vector   to_model_point, third_model_vector, third_vector;
    Vector   offset;

    fill_Point( model_offset, 0.0, 0.0, 0.0 );

    SUB_POINTS( to_model_point, model_points[point_index],
                model_centroids[point_index] );

    for_less( i, 0, n_neighbours )
    {
        SUB_POINTS( to_model_neighbour,
                    model_points[neighbours[i]], model_centroids[point_index] );

        CROSS_VECTORS( third_model_vector,
                       to_model_neighbour, model_normals[point_index] );
        NORMALIZE_VECTOR( third_model_vector, third_model_vector );

        get_coords_in_nonortho_system( &to_model_point,
                                       &to_model_neighbour,
                                       &model_normals[point_index],
                                       &third_model_vector,
                                       &d1, &d2, &d3 );

        model_len_to_neighbour = MAGNITUDE( to_model_neighbour );
        if( model_len_to_neighbour == 0.0 )
            model_len_to_neighbour = 1.0;

        SUB_POINTS( to_neighbour, points[neighbours[i]], *centroid );
        CROSS_VECTORS( third_vector, to_neighbour, *normal );
        NORMALIZE_VECTOR( third_vector, third_vector );

        len_to_neighbour = MAGNITUDE( to_neighbour );
        scale_factor = len_to_neighbour / model_len_to_neighbour;
        d2 *= scale_factor;
        d3 *= scale_factor;

        SCALE_VECTOR( offset, to_neighbour, d1 );
        ADD_VECTORS( model_offset, model_offset, offset );
        SCALE_VECTOR( offset, *normal, d2 );
        ADD_VECTORS( model_offset, model_offset, offset );
        SCALE_VECTOR( offset, third_vector, d3 );
        ADD_VECTORS( model_offset, model_offset, offset );
    }

    SCALE_VECTOR( model_offset, model_offset, 1.0 / (Real) n_neighbours );

    ADD_POINT_VECTOR( *model_point, *centroid, model_offset );

#ifdef DEBUG
    if( distance_between_points( model_point, &points[point_index] ) > 0.0001 )
    {
        print( "%d: %g %g %g  %g %g %g\n", point_index,
               Point_x(*model_point ),
               Point_y(*model_point ),
               Point_z(*model_point ),
               Point_x(points[point_index]),
               Point_y(points[point_index]),
               Point_z(points[point_index]) );
        HANDLE_INTERNAL_ERROR( "dang" );
    }
#endif
}

public  void  get_model_point(
    deformation_model_struct  *deformation_model,
    Point                     points[],
    int                       point_index,
    int                       n_neighbours,
    int                       neighbours[],
    Real                      curvatures[],
    Point                     *centroid,
    Vector                    *normal,
    Real                      base_length,
    Point                     *model_point )
{
    int                       i;
    Real                      curvature, dist_from_centroid, normal_mag;
    Vector                    offset;
    deform_model_struct       *model;
    Deformation_model_types   model_type;

    model = find_relevent_model( deformation_model, point_index );

    model_type = model->model_type;

    if( model_type == PARAMETRIC_MODEL && model->n_model_points == 0 )
        model_type = FLAT_MODEL;

    switch( model_type )
    {
    case FLAT_MODEL:
        *model_point = *centroid;
        break;

    case AVERAGE_MODEL:
        curvature = 0.0;
        for_less( i, 0, n_neighbours )
            curvature += curvatures[neighbours[i]];
        curvature /= (Real) n_neighbours;

        dist_from_centroid = curvature * base_length;
        normal_mag = MAGNITUDE( *normal );
        if( normal_mag == 0.0 )  normal_mag = 1.0;

        SCALE_VECTOR( offset, *normal, dist_from_centroid / normal_mag );

        ADD_POINT_VECTOR( *model_point, *centroid, offset );
        break;

    case PARAMETRIC_MODEL:
    case GENERAL_MODEL:
        compute_model_point( points, point_index, n_neighbours, neighbours,
                             centroid, normal,
                             model->model_points,
                             model->model_centroids,
                             model->model_normals,
                             model_point );
        break;

    default:
        HANDLE_INTERNAL_ERROR( "get_model_point" );
        break;
    }
}

public  void  get_neighbours_of_line_vertex(
    lines_struct    *lines,
    int             vertex_index,
    int             neighbours[2] )
{
    int    prev_index, next_index;

    if( vertex_index == 0 )
        prev_index = lines->n_points-1;
    else
        prev_index = (vertex_index - 1 + lines->n_points) %
                     lines->n_points;

    next_index = (vertex_index + 1 + lines->n_points) % lines->n_points;

    neighbours[0] = lines->indices[prev_index];
    neighbours[1] = lines->indices[next_index];
}

public  BOOLEAN  deformation_model_includes_average(
    deformation_model_struct   *model )
{
    int   i;

    for_less( i, 0, model->n_models )
        if( model->models[i].model_type == AVERAGE_MODEL )
            return( TRUE );

    return( FALSE );
}

public  Real  compute_line_curvature(
    lines_struct    *lines,
    int             axis,
    int             point_index,
    int             prev_point_index,
    int             next_point_index )
{
    Real             base_length, curvature;
    Point            centroid;
    Vector           normal, dir_to_point;

    compute_line_centroid_and_normal( lines, axis, prev_point_index,
                                      next_point_index, &centroid, &normal,
                                      &base_length );

    SUB_POINTS( dir_to_point, lines->points[point_index], centroid );

    curvature = MAGNITUDE( dir_to_point ) / base_length;

    if( DOT_VECTORS( dir_to_point, normal ) < 0.0 )
        curvature = -curvature;

    return( curvature );
}

public  Real  deform_point(
    int                        point_index,
    Point                      points[],
    Point                      *equilibrium_point,
    Real                       fractional_step,
    Real                       max_step,
    BOOLEAN                    position_constrained,
    Real                       max_position_offset,
    Point                      original_positions[],
    Point                      *new_point )
{
    Vector    toward_equil;
    Real      dist_to_equil, dist;
    Vector    diff;
    Real      factor;

    SUB_POINTS( toward_equil, *equilibrium_point, points[point_index] );
    dist_to_equil = MAGNITUDE( toward_equil );

    if( dist_to_equil * fractional_step > max_step )
        fractional_step = max_step / dist_to_equil;

    SCALE_VECTOR( toward_equil, toward_equil, fractional_step );

    ADD_POINT_VECTOR( *new_point, points[point_index], toward_equil );

    if( position_constrained )
    {
        SUB_POINTS( diff, *new_point, original_positions[point_index] );
        dist = MAGNITUDE( diff );
        if( dist > max_position_offset )
        {
            factor = max_position_offset / dist;
            SCALE_VECTOR( diff, diff, factor );
            ADD_POINT_VECTOR( *new_point, original_positions[point_index],
                              diff );
        }
    }

    return( dist_to_equil );
}

public  void  compute_line_centroid_and_normal(
    lines_struct     *lines,
    int              axis,
    int              prev_point_index,
    int              next_point_index,
    Point            *centroid,
    Vector           *normal,
    Real             *base_length )
{
    int       a1, a2;
    Vector    dir;

    SUB_POINTS( dir, lines->points[next_point_index],
                     lines->points[prev_point_index] );

    *base_length = MAGNITUDE( dir );
    if( *base_length == 0.0 )
        *base_length = 1.0;

    a1 = (axis + 1) % 3;
    a2 = (axis + 2) % 3;

    Point_coord(*normal,axis) = Point_coord(dir,axis);
    Point_coord(*normal,a1) = Point_coord(dir,a2);
    Point_coord(*normal,a2) = -Point_coord(dir,a1);
    NORMALIZE_VECTOR( *normal, *normal );

    INTERPOLATE_POINTS( *centroid,
                        lines->points[next_point_index],
                        lines->points[prev_point_index], 0.5 );
}

public  int  get_subsampled_neighbours_of_point(
    deformation_model_struct  *deformation_model,
    polygons_struct           *polygons,
    int                       poly,
    int                       vertex_index,
    int                       neighbours[],
    int                       max_neighbours,
    BOOLEAN                   *interior_flag )
{
    int                  point_index, model_vertex_index, model_poly;
    polygons_struct      *model_polygons;
    deform_model_struct  *model;

    point_index = polygons->indices[
                POINT_INDEX(polygons->end_indices,poly,vertex_index)];

    model = find_relevent_model( deformation_model, point_index );

    if( polygons->n_points <= model->up_to_n_points )
    {
        model_polygons = polygons;
        model_poly = poly;
        model_vertex_index = vertex_index;
    }
    else
    {
        model_polygons = get_polygons_ptr( model->model_object );

        if( !find_polygon_with_vertex( model_polygons, point_index,
                                       &model_poly, &model_vertex_index ) )
        {
            HANDLE_INTERNAL_ERROR( "get_subsampled_neighbours_of_point" );
        }
    }

    return( get_neighbours_of_point( model_polygons, model_poly,
                                     model_vertex_index, neighbours,
                                     max_neighbours, interior_flag ) );
}
