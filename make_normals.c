
#include <mni.h>
#include <module.h>

public  Status  process_object(
    object_struct  *object )
{
    if( object->object_type == POLYGONS )
        compute_polygon_normals( get_polygons_ptr(object) );

    return( OK );
}
