
#include <mni.h>
#include <module.h>

public  Status  process_object(
    object_struct  *object )
{
    if( object->object_type == POLYGONS )
        average_polygon_normals( get_polygons_ptr(object), 5, 0.9 );

    return( OK );
}
