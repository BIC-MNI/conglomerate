
#include  <mni.h>

private  BOOLEAN  recursive_find_polygon(
    bintree_node_struct   *node,
    polygons_struct       *polygons,
    Point                 *point,
    int                   *poly_index );

public  BOOLEAN  find_polygon_containing_point(
    bintree_struct  *bintree,
    polygons_struct *polygons,
    Point           *point,
    int             *poly_index )
{
    Real           closest;
    range_struct   limits;

    get_bintree_limits( bintree, &limits );

    if( !point_within_range( point, &limits ) )
        return( FALSE );

    return( recursive_find_polygon( bintree->root, polygons,
                                    point, poly_index ) );
}

#define  MAX_POLYGON_SIZE   2100

private  BOOLEAN  recursive_find_polygon(
    bintree_node_struct   *node,
    polygons_struct       *polygons,
    Point                 *point,
    int                   *poly_index )
{
    BOOLEAN               test_child, searching_left;
    int                   poly, n_points;
    Real                  dist;
    Point                 points[MAX_POLYGON_SIZE];
    int                   i, n_objects, *object_list;
    bintree_node_struct   *left_child, *right_child;

    if( bintree_node_is_leaf( node ) )
    {
        n_objects = get_bintree_leaf_objects( node, &object_list );

        for_less( i, 0, n_objects )
        {
            poly = object_list[i];
            n_points = get_polygon_points( polygons, poly, points );

            if( point_is_in_polygon( n_points, points, point ) )
            {
                *poly_index = poly;
                return( TRUE );
            }
        }
    }
    else
    {
        axis_index = get_node_split_axis( node );

        if( get_bintree_left_child( node, &left_child ) &&
            Point_coord(*point,axis_index) <=
                       get_node_split_position(left_child) )
        {
            if( recursive_find_polygon( left_child, polygons, point,
                                        poly_index ) )
                return( TRUE );
        }

        if( get_bintree_right_child( node, &right_child ) &&
            Point_coord(*point,axis_index) >=
                       get_node_split_position(right_child) )
        {
            if( recursive_find_polygon( right_child, polygons, point,
                                        poly_index ) )
                return( TRUE );
        }
    }

    return( FALSE );
}
