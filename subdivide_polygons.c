#include <internal_volume_io.h>
#include <bicpl.h>

public  Status  process_object(
    object_struct  *object )
{
    if( object->object_type == POLYGONS )
    {
        subdivide_polygons( get_polygons_ptr(object) );
        compute_polygon_normals( get_polygons_ptr(object) );
    }

    return( OK );
}
