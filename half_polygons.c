#include <def_mni.h>
#include <def_module.h>

public  Status  process_object(
    object_struct  *object )
{
    polygons_struct  half;

    if( object->object_type == POLYGONS )
    {
        half_sample_sphere_tessellation( get_polygons_ptr(object), &half );

        delete_polygons( get_polygons_ptr(object) );
        *(get_polygons_ptr(object)) = half;
    }

    return( OK );
}
