#ifndef  DEF_TRIMESH_H
#define  DEF_TRIMESH_H

typedef struct tri_node_struct
{
    int                      nodes[3];
    struct tri_node_struct   *children[4];
} tri_node_struct;

typedef struct
{
    int                  n_points;
    Point                *points;
    Smallest_int         *active_flags;
    int                  n_triangles;
    tri_node_struct      *triangles;
    BOOLEAN              edge_lookup_initialized;
    hash2_table_struct   edge_lookup;
} tri_mesh_struct;

#endif
