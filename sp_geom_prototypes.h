#ifndef  DEF_sp_geom_prototypes
#define  DEF_sp_geom_prototypes

public  int  clip_polygons_to_plane(
    polygons_struct   *polygons,
    VIO_Vector            *plane_normal,
    VIO_Real              plane_constant,
    polygons_struct   *clipped );

public  VIO_Real  get_closed_polyhedron_volume(
    polygons_struct  *polygons );

public  void  tri_mesh_initialize(
    tri_mesh_struct  *mesh );

public  int  tri_mesh_get_n_points(
    tri_mesh_struct  *mesh );

public  VIO_Status  tri_mesh_output(
    VIO_STR         filename,
    VIO_File_formats   format,
    tri_mesh_struct  *mesh );

public  VIO_BOOL  tri_mesh_set_points(
    tri_mesh_struct  *mesh,
    int              n_points,
    VIO_Point            points[] );

public  VIO_Status  tri_mesh_input(
    VIO_STR           filename,
    VIO_File_formats     format,
    tri_mesh_struct  *mesh );

public  void  tri_mesh_delete_edge_lookup(
    tri_mesh_struct     *mesh );

public  void  tri_mesh_create_edge_lookup(
    tri_mesh_struct     *mesh );

public  void  tri_mesh_delete(
    tri_mesh_struct  *mesh );

public  void   tri_mesh_delete_unused_nodes(
    tri_mesh_struct  *mesh );

public  void   tri_mesh_convert_from_polygons(
    polygons_struct  *polygons,
    tri_mesh_struct  *mesh );

public   void   tri_mesh_convert_to_polygons(
    tri_mesh_struct   *mesh,
    polygons_struct   *polygons );

public   void   tri_mesh_coalesce_triangles(
    tri_mesh_struct    *mesh,
    VIO_Real               min_value,
    VIO_Real               max_value,
    int                n_values,
    VIO_Real               values[],
    VIO_Real               min_size,
    VIO_Real               max_size );

public   void   tri_mesh_subdivide_triangles(
    tri_mesh_struct    *mesh,
    VIO_Real               min_value,
    VIO_Real               max_value,
    int                n_values,
    VIO_Real               values[],
    VIO_Real               min_size,
    VIO_Real               max_size,
    int                max_subdivisions );

public   void   tri_mesh_subdivide_bordering_triangles(
    tri_mesh_struct    *mesh );

public  void  tri_mesh_print_levels(
    tri_mesh_struct  *mesh );

public  VIO_Status  output_mesh_fixed_midpoints(
    VIO_STR           filename,
    tri_mesh_struct  *mesh );

public  void  tri_mesh_reconcile_points(
    tri_mesh_struct  *dest_mesh,
    tri_mesh_struct  *src_mesh );
#endif
