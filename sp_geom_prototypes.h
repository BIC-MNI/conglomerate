#ifndef  DEF_sp_geom_prototypes
#define  DEF_sp_geom_prototypes

public  int  clip_polygons_to_plane(
    polygons_struct   *polygons,
    Vector            *plane_normal,
    Real              plane_constant,
    polygons_struct   *clipped );

public  Real  get_closed_polyhedron_volume(
    polygons_struct  *polygons );

public  void  tri_mesh_initialize(
    tri_mesh_struct  *mesh );

public  int  tri_mesh_get_n_points(
    tri_mesh_struct  *mesh );

public  Status  tri_mesh_output(
    STRING         filename,
    File_formats   format,
    tri_mesh_struct  *mesh );

public  BOOLEAN  tri_mesh_set_points(
    tri_mesh_struct  *mesh,
    int              n_points,
    Point            points[] );

public  Status  tri_mesh_input(
    STRING           filename,
    File_formats     format,
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
    Real               min_value,
    Real               max_value,
    int                n_values,
    Real               values[],
    Real               min_size,
    Real               max_size );

public   void   tri_mesh_subdivide_triangles(
    tri_mesh_struct    *mesh,
    Real               min_value,
    Real               max_value,
    int                n_values,
    Real               values[],
    Real               min_size,
    Real               max_size,
    int                max_subdivisions );

public   void   tri_mesh_subdivide_bordering_triangles(
    tri_mesh_struct    *mesh );

public  void  tri_mesh_print_levels(
    tri_mesh_struct  *mesh );

public  Status  output_mesh_fixed_midpoints(
    STRING           filename,
    tri_mesh_struct  *mesh );

public  void  tri_mesh_reconcile_points(
    tri_mesh_struct  *dest_mesh,
    tri_mesh_struct  *src_mesh );
#endif
