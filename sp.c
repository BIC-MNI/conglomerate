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

#include  <internal_volume_io.h>
#include  <geom.h>

#ifndef lint
static char rcsid[] = "$Header: /private-cvsroot/libraries/conglomerate/sp.c,v 1.1 2001-05-11 08:11:23 stever Exp $";
#endif

private  void  subdivide_polygon(
    polygons_struct   *polygons,
    int               poly,
    int               *new_n_points,
    Point             *new_points[],
    int               *new_n_polygons,
    int               *new_end_indices[],
    int               *new_n_indices,
    int               *new_indices[],
    int               n_neighbours[],
    int               *neighbours[],
    int               *midpoints[] );

/* ----------------------------- MNI Header -----------------------------------
@NAME       : subdivide_polygons
@INPUT      : polygons
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Subdivides the polygons, each polygon become 4 polygons.
              Can only handle polygons that have 3 or 4 vertices.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  delete_polygon_point_neighbours(
    int              n_point_neighbours[],
    int              *point_neighbours[],
    int              *point_polygons[] );
public   void   create_polygon_point_neighbours(
    polygons_struct  *polygons,
    int              *n_point_neighbours_ptr[],
    int              **point_neighbours_ptr[],
    int              **point_polygons_ptr[] );

public  void  subdivide_polygons(
    polygons_struct  *polygons )
{
    int                i, new_n_points, new_n_indices, new_n_polygons;
    int                *new_indices, *new_end_indices;
    int                size, *n_point_neighbours, **point_neighbours;
    int                **midpoints;
    int                total_n_point_neighbours;
    Point              *new_points;
    Point              dummy;
    Colour             save_colour;
    progress_struct    progress;

    new_points = NULL;

    if( is_this_sphere_topology( polygons ) )
    {
        print( "Subdividing assuming sphere topology.\n" );
        create_polygons_sphere( &dummy, 0.0, 0.0, 0.0, 0, 0, TRUE, polygons );
        return;
    }

    save_colour = polygons->colours[0];

    new_n_points = polygons->n_points;
    new_n_polygons = 0;
    new_n_indices = 0;

    size = 0;
    SET_ARRAY_SIZE( new_points, size, new_n_points, DEFAULT_CHUNK_SIZE );

    for_less( i, 0, new_n_points )
        new_points[i] = polygons->points[i];

    create_polygon_point_neighbours( polygons, &n_point_neighbours,
                                     &point_neighbours, NULL );

    total_n_point_neighbours = 0;

    for_less( i, 0, new_n_points )
        total_n_point_neighbours += n_point_neighbours[i];

    ALLOC( midpoints, new_n_points );
    ALLOC( midpoints[0], total_n_point_neighbours );

    for_less( i, 1, new_n_points )
        midpoints[i] = &midpoints[i-1][n_point_neighbours[i-1]];

    for_less( i, 0, total_n_point_neighbours )
        midpoints[0][i] = -1;

    initialize_progress_report( &progress, TRUE, polygons->n_items,
                                "Subdividing Polygon" );

    for_less( i, 0, polygons->n_items )
    {
        subdivide_polygon( polygons, i,
                           &new_n_points, &new_points,
                           &new_n_polygons, &new_end_indices,
                           &new_n_indices, &new_indices,
                           n_point_neighbours, point_neighbours, midpoints );
        update_progress_report( &progress, i+1 );
    }

    terminate_progress_report( &progress );

    delete_polygon_point_neighbours( n_point_neighbours,
                                     point_neighbours, NULL );

    delete_polygons( polygons );

    ALLOC( polygons->colours, 1 );
    polygons->colour_flag = ONE_COLOUR;
    polygons->colours[0] = save_colour;

    ALLOC( polygons->normals, new_n_points );

    polygons->n_points = new_n_points;
    polygons->points = new_points;
    polygons->n_items = new_n_polygons;
    polygons->end_indices = new_end_indices;
    polygons->indices = new_indices;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : subdivide_polygon
@INPUT      : polygons
              poly
@OUTPUT     : new_n_points
              new_points
              new_n_polygons
              new_end_indices
              new_n_indices
              new_indices
              midpoint_table
@RETURNS    : 
@DESCRIPTION: Subdivides the poly'th polygon into 4 new polygons.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  void  subdivide_polygon(
    polygons_struct   *polygons,
    int               poly,
    int               *new_n_points,
    Point             *new_points[],
    int               *new_n_polygons,
    int               *new_end_indices[],
    int               *new_n_indices,
    int               *new_indices[],
    int               n_neighbours[],
    int               *neighbours[],
    int               *midpoints[] )
{
    int     edge, size, i0, i1, point_indices[4], p;
    int     midpoint_indices[4], centre_point_index, p1, p2;
    Point   midpoint;

    size = GET_OBJECT_SIZE( *polygons, poly );

    if( size < 3 || size > 4 )
    {
        print_error( "Polygon size %d in subdivide polygons\n", size );
        return;
    }

    for_less( edge, 0, size )
    {
        point_indices[edge] = polygons->indices[
                      POINT_INDEX(polygons->end_indices,poly,edge)];
    }

    for_less( edge, 0, size )
    {
        p1 = point_indices[edge];
        p2 = point_indices[(edge+1)%size];
        i0 = MIN( p1, p2 );
        i1 = MAX( p1, p2 );

        for_less( p, 0, n_neighbours[i0] )
        {
            if( neighbours[i0][p] == i1 )
                break;
        }

        if( p >= n_neighbours[i0] )
            handle_internal_error( "subdivide_polygon" );

        midpoint_indices[edge] = midpoints[i0][p];

        if( midpoint_indices[edge] < 0 )
        {
            midpoint_indices[edge] = *new_n_points;
            midpoints[i0][p] = *new_n_points;
            INTERPOLATE_POINTS( midpoint, polygons->points[i0],
                                polygons->points[i1], 0.5 );
            ADD_ELEMENT_TO_ARRAY( *new_points, *new_n_points,
                                  midpoint, DEFAULT_CHUNK_SIZE );
        }
    }

    if( size == 3 )
    {

        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              point_indices[0], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[0], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[2], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_end_indices, *new_n_polygons,
                              *new_n_indices, DEFAULT_CHUNK_SIZE );

        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[0], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              point_indices[1], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[1], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_end_indices, *new_n_polygons,
                              *new_n_indices, DEFAULT_CHUNK_SIZE );

        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[2], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[0], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[1], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_end_indices, *new_n_polygons,
                              *new_n_indices, DEFAULT_CHUNK_SIZE );

        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[2], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[1], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              point_indices[2], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_end_indices, *new_n_polygons,
                              *new_n_indices, DEFAULT_CHUNK_SIZE );
    }
    else
    {
        centre_point_index = *new_n_points;

        ADD_POINTS( midpoint, polygons->points[point_indices[0]],
                              polygons->points[point_indices[1]] );
        ADD_POINTS( midpoint, midpoint, polygons->points[point_indices[2]] );
        ADD_POINTS( midpoint, midpoint, polygons->points[point_indices[3]] );
        SCALE_POINT( midpoint, midpoint, 0.25 );

        ADD_ELEMENT_TO_ARRAY( *new_points, *new_n_points,
                              midpoint, DEFAULT_CHUNK_SIZE );

        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              point_indices[0], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[0], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              centre_point_index, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[3], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_end_indices, *new_n_polygons,
                              *new_n_indices, DEFAULT_CHUNK_SIZE );

        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[0], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              point_indices[1], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[1], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              centre_point_index, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_end_indices, *new_n_polygons,
                              *new_n_indices, DEFAULT_CHUNK_SIZE );

        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[3], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              centre_point_index, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[2], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              point_indices[3], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_end_indices, *new_n_polygons,
                              *new_n_indices, DEFAULT_CHUNK_SIZE );

        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              centre_point_index, DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[1], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              point_indices[2], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_indices, *new_n_indices,
                              midpoint_indices[2], DEFAULT_CHUNK_SIZE );
        ADD_ELEMENT_TO_ARRAY( *new_end_indices, *new_n_polygons,
                              *new_n_indices, DEFAULT_CHUNK_SIZE );
    }
}
