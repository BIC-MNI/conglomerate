#ifndef  DEF_DEFORM
#define  DEF_DEFORM

#include  <bicpl.h>

typedef  enum  { TOWARDS_LOWER, TOWARDS_HIGHER, ANY_DIRECTION }
               Normal_directions;

typedef  struct
{
    Real               min_isovalue;
    Real               max_isovalue;
    Real               gradient_threshold;
    Real               min_dot_product;
    Real               max_dot_product;
    Normal_directions  normal_direction;
    Real               tolerance;
} boundary_definition_struct;

typedef  enum  { VOLUME_DATA }  Deform_data_types;

typedef  struct
{
    Deform_data_types   type;
    Volume              volume;
    Volume              label_volume;
} deform_data_struct;

typedef  enum { FLAT_MODEL, AVERAGE_MODEL,
                PARAMETRIC_MODEL, GENERAL_MODEL }
                Deformation_model_types;

typedef  struct
{
    int                       up_to_n_points;

    Deformation_model_types   model_type;
    Real                      model_weight;
    object_struct             *model_object;

    int                       n_model_points;
    Point                     *model_centroids;
    Vector                    *model_normals;
    Point                     *model_points;

    Real                      min_curvature_offset;
    Real                      max_curvature_offset;
} deform_model_struct;

typedef  struct
{
    int                       n_models;
    deform_model_struct       *models;
    BOOLEAN                   position_constrained;
    Real                      max_position_offset;
    Point                     *original_positions;
} deformation_model_struct;

typedef  struct
{
    deform_data_struct            deform_data;
    deformation_model_struct      deformation_model;
    Real                          fractional_step;
    Real                          max_step;
    Real                          max_search_distance;
    int                           degrees_continuity;
    boundary_definition_struct    boundary_definition;
    int                           max_iterations;
    Real                          stop_threshold;

    int                           n_movements_alloced;
    float                         *prev_movements;
    Real                          movement_threshold;
} deform_struct;

#define  MAX_IN_VOXEL_COEF_LOOKUP  10000

typedef  struct  voxel_lin_coef_struct
{
    int                               hash_key;
    Real                              coefs[8];
    struct     voxel_lin_coef_struct  *prev;
    struct     voxel_lin_coef_struct  *next;
}
voxel_lin_coef_struct;

typedef struct
{
    hash_table_struct      hash;
    int                    n_in_hash;
    voxel_lin_coef_struct  *head;
    voxel_lin_coef_struct  *tail;
} voxel_coef_struct;

#define  N_DEFORM_HISTOGRAM   7

typedef struct
{
    Real    average;
    Real    maximum;
    int     n_below[N_DEFORM_HISTOGRAM];
} deform_stats;

typedef struct
{
    int                  axis;
    Point                *save_points;
    Real                 *curvature_factors;
    Point                *equilibrium_points;
    Point                *new_equilibrium_points;
    Point                *boundary_points;
    Point                *new_boundary_points;
    Real                 temperature;
    Real                 temperature_factor;
    int                  temperature_step;
    int                  min_n_to_move;
    int                  max_n_to_move;
    Real                 max_translation;
    Real                 max_angle_rotation;
    Real                 max_scale_offset;
    int                  stop_criteria;
    int                  try;
    int                  max_tries;
    int                  max_successes;
    int                  n_successes;
    int                  n_pos_successes;
    int                  n_no_moves;
    Real                 min_delta_energy;
    Real                 max_delta_energy;
    Real                 energy;
} anneal_struct;

#ifndef  public
#define       public   extern
#define       public_was_defined_here
#endif

#include  <deform_prototypes.h>

#ifdef  public_was_defined_here
#undef       public
#undef       public_was_defined_here
#endif

#endif
