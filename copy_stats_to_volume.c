#include  <volume_io.h>
#include  <bicpl.h>
#include <float.h>

VIO_Status
input_stats_file( VIO_STR filename, int *n_stats, VIO_Real **stats )
{
  FILE *fp = fopen(filename, "r");
  double d;
  if (fp == NULL)
    return VIO_ERROR;

  *n_stats = 0;
  while (fscanf(fp, "%lf", &d) == 1)
  {
    ADD_ELEMENT_TO_ARRAY(*stats, *n_stats, d, 100 );
  }
  fclose(fp);
  return VIO_OK;
}

int
main( int argc, char *argv[] )
{
  VIO_STR              src_filename;
  VIO_STR              obj_filename;
  VIO_STR              dst_filename;
  VIO_STR              stats_filename;
  VIO_Volume           src_vol;
  VIO_Volume           dst_vol;
  VIO_File_formats     format;
  int                  n_objects;
  object_struct        **objects;
  int                  sizes[VIO_MAX_DIMENSIONS];
  int                  i, j, k;
  int                  n_points;
  VIO_Point            *points;
  VIO_Real             stat_min, stat_max;
  int                  n_stats;
  VIO_Real             *stats;
  polygons_struct      *polygons;
  VIO_Real             fillvalue = 0.0;
    
  initialize_argument_processing( argc, argv );

  if( !get_string_argument( NULL, &src_filename ) ||
      !get_string_argument( NULL, &obj_filename ) ||
      !get_string_argument( NULL, &stats_filename ) ||
      !get_string_argument( NULL, &dst_filename ) )
  {
    char *usage = "Usage: %s volume.mnc object.obj stats.txt result.mnc [fillvalue]\n";
    print_error(usage, argv[0]);
    return( 1 );
  }

  get_real_argument(0.0, &fillvalue);

  if( input_volume( src_filename, 3, NULL,
                    NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE,
                    &src_vol, NULL) != VIO_OK )
    return( 1 );

  if( input_graphics_file( obj_filename,
                           &format, &n_objects, &objects ) != VIO_OK )
    return( 1 );

  if ( input_stats_file( stats_filename, &n_stats, &stats ) != VIO_OK )
    return( 1 );

  stat_min = DBL_MAX;
  stat_max = -DBL_MAX;

  for (i = 0; i < n_stats; i++)
  {
    if (stats[i] < stat_min)
      stat_min = stats[i];
    if (stats[i] > stat_max)
      stat_max = stats[i];
  }
  if (stat_min > fillvalue)
    stat_min = fillvalue;
  if (stat_max < fillvalue)
    stat_max = fillvalue;
  
  print( "Volume will range from %f to %f\n", stat_min, stat_max );
  get_volume_sizes( src_vol, sizes );
  dst_vol = copy_volume_definition( src_vol, NC_FLOAT, FALSE,
                                    stat_min, stat_max );
  set_volume_real_range( dst_vol, stat_min, stat_max );

  polygons = get_polygons_ptr( objects[0] );

  n_points = get_object_points( objects[0], &points );

  print("Building bintree: "); fflush(stdout);
  create_polygons_bintree( polygons,
                           VIO_ROUND( (VIO_Real) polygons->n_items * 0.3 ) );
  print("Done.\n");
  
  if( n_points != n_stats )
  {
    print_error("Size mismatch.\n");
    return 1;
  }

  print( "Copying: " );

  for_less(i, 0, sizes[0])
  {
    print(".");
    fflush(stdout);
    for_less(j, 0, sizes[1])
    {
      for_less(k, 0, sizes[2])
      {
        VIO_Real value = get_volume_voxel_value( src_vol, i, j, k, 0, 0 );
        if (value != 0)
        {
          VIO_Real  wx, wy, wz;
          VIO_Real  voxel[VIO_MAX_DIMENSIONS];
          VIO_Point pt;
          int       min_p;
            
          voxel[0] = i;
          voxel[1] = j;
          voxel[2] = k;
          voxel[3] = 0;
          voxel[4] = 0;

          convert_voxel_to_world( src_vol, voxel, &wx, &wy, &wz );
          fill_Point( pt, wx, wy, wz );

          find_closest_vertex_on_object( &pt, objects[0], &min_p );
          set_volume_voxel_value( dst_vol, i, j, k, 0, 0, stats[min_p] );
        }
        else
        {
          set_volume_voxel_value( dst_vol, i, j, k, 0, 0, fillvalue );
        }
      }
    }
  }

  print( "\nDone.\n" );

  output_volume( dst_filename, NC_UNSPECIFIED, FALSE,
                 0.0, 0.0, dst_vol, "Scanned from .obj\n", NULL );

  FREE(stats);
  delete_volume( src_vol );
  delete_volume( dst_vol );
  delete_object( objects[0] );
  FREE(objects);
  return( 0 );
}
