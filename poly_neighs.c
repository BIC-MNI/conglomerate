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
#include  <objects.h>

#ifndef lint
static char rcsid[] = "$Header: /private-cvsroot/libraries/conglomerate/poly_neighs.c,v 1.1 2001-05-11 08:11:21 stever Exp $";
#endif

#define  INVALID_ID       -1

#define  INITIAL_HASH_TABLE_SIZE   2.0   /* times number of polygons */
#define  ENLARGE_THRESHOLD         0.25
#define  NEW_DENSITY               0.125

#define  DEBUG

private   void   create_polygon_neighbours(
    polygons_struct  *polygons,
    int              neighbours[] );

/* ----------------------------- MNI Header -----------------------------------
@NAME       : check_polygons_neighbours_computed
@INPUT      : polygons
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Creates the polygons neighbours (neighbouring polygon index for
              each polygon edge), if necessary.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  check_polygons_neighbours_computed(
    polygons_struct   *polygons )
{
    if( polygons->neighbours == NULL && polygons->n_items > 0 )
    {
        ALLOC( polygons->neighbours,polygons->end_indices[polygons->n_items-1]);
        create_polygon_neighbours( polygons, polygons->neighbours );
    }
}

public  void  delete_polygon_point_neighbours(
    int              n_point_neighbours[],
    int              *point_neighbours[],
    int              *point_polygons[] )
{
    FREE( n_point_neighbours );
    FREE( point_neighbours[0] );

    if( point_polygons != NULL )
    {
        FREE( point_polygons[0] );
    }
}

public   void   create_polygon_point_neighbours(
    polygons_struct  *polygons,
    int              *n_point_neighbours_ptr[],
    int              **point_neighbours_ptr[],
    int              **point_polygons_ptr[] )
{
    int                 edge, i0, i1, size, poly, total_neighbours;
    int                 *n_point_neighbours, **point_neighbours;
    int                 **point_polygons, point, index0, index1;
    progress_struct     progress;

    ALLOC( n_point_neighbours, polygons->n_points );
    for_less( point, 0, polygons->n_points )
        n_point_neighbours[point] = 0;

    total_neighbours = 0;

    initialize_progress_report( &progress, FALSE, polygons->n_items,
                                "Neighbour-finding step 1" );

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( edge, 0, size )
        {
            i0 = polygons->indices[
                  POINT_INDEX(polygons->end_indices,poly,edge)];
            i1 = polygons->indices[
                  POINT_INDEX(polygons->end_indices,poly,(edge+1)%size)];

            ++n_point_neighbours[i0];
            ++n_point_neighbours[i1];
            total_neighbours += 2;
        }

        update_progress_report( &progress, poly+1 );
    }

    terminate_progress_report( &progress );

    ALLOC( point_neighbours, polygons->n_points );
    ALLOC( point_neighbours[0], total_neighbours );

    if( point_polygons_ptr != NULL )
    {
        ALLOC( point_polygons, polygons->n_points );
        ALLOC( point_polygons[0], total_neighbours );

        for_less( point, 0, total_neighbours )
            point_polygons[0][point] = -1;
    }

    for_less( point, 1, polygons->n_points )
    {
        point_neighbours[point] = &point_neighbours[point-1]
                                               [n_point_neighbours[point-1]];
        if( point_polygons_ptr != NULL )
        {
            point_polygons[point] = &point_polygons[point-1]
                                               [n_point_neighbours[point-1]];
        }
    }

    for_less( point, 0, polygons->n_points )
        n_point_neighbours[point] = 0;

    initialize_progress_report( &progress, FALSE, polygons->n_items,
                                "Neighbour-finding step 2" );

    for_less( poly, 0, polygons->n_items )
    {
        size = GET_OBJECT_SIZE( *polygons, poly );

        for_less( edge, 0, size )
        {
            i0 = polygons->indices[
                  POINT_INDEX(polygons->end_indices,poly,edge)];
            i1 = polygons->indices[
                  POINT_INDEX(polygons->end_indices,poly,(edge+1)%size)];

            for_less( index0, 0, n_point_neighbours[i0] )
            {
                if( point_neighbours[i0][index0] == i1 )
                    break;
            }

            if( index0 >= n_point_neighbours[i0] )
            {
                point_neighbours[i0][index0] = i1;
                ++n_point_neighbours[i0];
            }

            for_less( index1, 0, n_point_neighbours[i1] )
            {
                if( point_neighbours[i1][index1] == i0 )
                    break;
            }

            if( index1 >= n_point_neighbours[i1] )
            {
                point_neighbours[i1][index1] = i0;
                ++n_point_neighbours[i1];
            }

            if( point_polygons_ptr != NULL )
            {
                if( point_polygons[i0][index0] < 0 )
                    point_polygons[i0][index0] = poly;
                else
                    point_polygons[i1][index1] = poly;
            }
        }

        update_progress_report( &progress, poly+1 );
    }

    terminate_progress_report( &progress );

    *n_point_neighbours_ptr = n_point_neighbours;   
    *point_neighbours_ptr = point_neighbours;   
    if( point_polygons_ptr != NULL )
        *point_polygons_ptr = point_polygons;   
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : create_polygon_neighbours
@INPUT      : n_polygons
              indices
              end_indices
@OUTPUT     : neighbours
@RETURNS    : 
@DESCRIPTION: Computes the neighbours for each edge of the polygons
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    :         1993    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private   void   create_polygon_neighbours(
    polygons_struct  *polygons,
    int              neighbours[] )
{
    int                 i0, i1, size1, size2, n1, n2;
    int                 poly1, poly2, point1, point2, edge1, edge2;
    int                 *n_point_neighbours, **point_neighbours;
    int                 **point_polygons;
    progress_struct     progress;

    for_less( i0, 0, polygons->end_indices[polygons->n_items-1] )
        neighbours[i0] = -1;

    create_polygon_point_neighbours( polygons, &n_point_neighbours,
                                     &point_neighbours, &point_polygons );

    initialize_progress_report( &progress, FALSE, polygons->n_items,
                                "Neighbour-finding step 2" );

    for_less( point1, 0, polygons->n_points )
    {
        for_less( n1, 0, n_point_neighbours[point1] )
        {
            point2 = point_neighbours[point1][n1];

            if( point2 <= point1 )
                continue;

            poly1 = point_polygons[point1][n1];
            if( poly1 < 0 )
            {
                handle_internal_error( "create_polygon_neighbours: poly1" );
                continue;
            }

            for_less( n2, 0, n_point_neighbours[point2] )
            {
                if( point_neighbours[point2][n2] == point1 )
                    break;
            }

            if( n2 >= n_point_neighbours[point2] )
                handle_internal_error( "create_polygon_neighbours" );

            poly2 = point_polygons[point2][n2];
            if( poly2 < 0 )
            {
                handle_internal_error( "create_polygon_neighbours: poly2" );
                continue;
            }

            size1 = GET_OBJECT_SIZE( *polygons, poly1 );
            for_less( edge1, 0, size1 )
            {
                i0 = polygons->indices[POINT_INDEX(polygons->end_indices,
                                                   poly1,edge1)];
                i1 = polygons->indices[POINT_INDEX(polygons->end_indices,
                                                   poly1,(edge1+1)%size1)];

                if( i0 == point1 && i1 == point2 ||
                    i1 == point1 && i0 == point2 )
                    break;
            }

            if( edge1 >= size1 )
                handle_internal_error( "create_polygon_neighbours" );

            size2 = GET_OBJECT_SIZE( *polygons, poly2 );
            for_less( edge2, 0, size2 )
            {
                i0 = polygons->indices[POINT_INDEX(polygons->end_indices,
                                                   poly2,edge2)];
                i1 = polygons->indices[POINT_INDEX(polygons->end_indices,
                                                   poly2,(edge2+1)%size2)];

                if( i0 == point1 && i1 == point2 ||
                    i1 == point1 && i0 == point2 )
                    break;
            }

            if( edge2 >= size2 )
                handle_internal_error( "create_polygon_neighbours" );

            neighbours[POINT_INDEX( polygons->end_indices, poly1, edge1 )] =
                                                               poly2;
            neighbours[POINT_INDEX( polygons->end_indices, poly2, edge2 )] =
                                                               poly1;
        }

        update_progress_report( &progress, point1+1 );
    }

    terminate_progress_report( &progress );

    delete_polygon_point_neighbours( n_point_neighbours, point_neighbours,
                                     point_polygons );

#ifdef  DEBUG
    for_less( i0, 0, polygons->end_indices[polygons->n_items-1] )
    {
        if( neighbours[i0] < 0 )
            break;
    }

    if( i0 < polygons->end_indices[polygons->n_items-1] )
        handle_internal_error( "create_polygon_neighbours: topology\n" );
#endif
}
