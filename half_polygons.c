#include <mni.h>
#include <module.h>

public  Status  process_object(
    object_struct  *object )
{
    polygons_struct  *polygons, half;

    if( object->object_type == POLYGONS )
    {
        polygons = get_polygons_ptr( object );

        if( is_this_sphere_topology( polygons ) )
            half_sample_sphere_tessellation( polygons, &half );
        else if( is_this_tetrahedral_topology( polygons ) )
            half_sample_tetrahedral_tessellation( polygons, &half );

        compute_polygon_normals( &half );

        delete_polygons( polygons );
        *polygons = half;
    }

    return( OK );
}
