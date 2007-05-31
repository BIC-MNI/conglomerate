/*
 * Code adapted from David MacDonald's surface_mask2
 * Basic logic:
 *   1) Read input file and input object
 *   2) Create a binary volume of all voxels inside object
 *   3) Force inclusion of object boundaries (this is the key difference
 *      between this version and the old David MacDonald version).
 *   4) Keep all original voxels inside binary mask, set the rest to 0
 *
 * Author: Jason Lerch <jason@bic.mni.mcgill.ca>
 * Last changed: Sep 2003.
 */

#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>
#include  <ParseArgv.h>

#define  BINTREE_FACTOR   0.1

#define  OFFSET   1000.0

/* argument parsing options */
int    output_binary_mask = 0;

/* argument parsing table */
ArgvInfo argTable[] = {
  { NULL, ARGV_HELP, (char *)NULL, (char *)NULL,
    "\nOutput options:" },
  { "-binary_mask", ARGV_CONSTANT, (char *) 1, (char *) &output_binary_mask,
    "Create a binary output." },

  { NULL, ARGV_END, NULL, NULL, NULL }
};

int binary_object_mask( STRING input_surface_filename,
                        Real set_value,
                        Real inside_value,
                        Volume volume ) 
{

    Real                 separations[MAX_DIMENSIONS];
    Real                 xw, yw, zw, value_to_set;
    Real                 sign_before, sign_after;
    Point                origin, dest;
    Real                 min_real_value, max_real_value;
    Real                 *directions;
    Vector               direction;
    File_formats         format;
    int                  x, y, z, n_objects, k;
    int                  sizes[MAX_DIMENSIONS];
    int                  obj_index;
    int                  i, j, best, n_intersections, ind;
    Real                 dist, *distances, tmp, voxel[MAX_DIMENSIONS];
    object_struct        **objects;
    BOOLEAN              inside, erase_flag;
    progress_struct      progress;

    if( input_graphics_file( input_surface_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    if( n_objects < 1 )
    {
        print( "No objects in %s.\n", input_surface_filename);
        return( 1 );
    }

    get_volume_separations( volume, separations );
    get_volume_sizes( volume, sizes );

    min_real_value = get_volume_real_min( volume );
    max_real_value = get_volume_real_max( volume );

    if( BINTREE_FACTOR > 0.0 )
    {
        create_polygons_bintree( get_polygons_ptr(objects[0]),
                     ROUND( (Real) get_polygons_ptr(objects[0])->n_items *
                                                   BINTREE_FACTOR ) );
    }

    initialize_progress_report( &progress, FALSE, sizes[X] * sizes[Y],
                                "Masking" );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        {
            voxel[X] = (Real) x;
            voxel[Y] = (Real) y;
            voxel[Z] = -OFFSET;

            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            fill_Point( origin, xw, yw, zw );

            voxel[Z] = (Real) sizes[Z] + OFFSET;
            convert_voxel_to_world( volume, voxel, &xw, &yw, &zw );
            fill_Point( dest, xw, yw, zw );
            SUB_POINTS( direction, dest, origin );
            NORMALIZE_VECTOR( direction, direction );

            initialize_intersect_directions();

            n_intersections = intersect_ray_with_object( &origin, &direction,
                                       objects[0], &obj_index, &dist,
                                       &distances );

            directions = get_intersect_directions();

            for_less( i, 0, n_intersections )
                distances[i] = distances[i] / FABS( separations[Z] ) - OFFSET;

            for_less( i, 0, n_intersections-1 )
            {
                best = i;
                for_less( j, i+1, n_intersections )
                {
                    if( distances[j] < distances[best] )
                        best = j;
                }

                tmp = distances[i];
                distances[i] = distances[best];
                distances[best] = tmp;
                tmp = directions[i];
                directions[i] = directions[best];
                directions[best] = tmp;
            }

            j = 0;
            i = 0;
            while( i < n_intersections )
            {
                if( j == 0 || distances[i] - distances[j-1] > 1e-10 ||
                    directions[i] != directions[j-1] )
                {
                    distances[j] = distances[i];
                    directions[j] = directions[i];
                    ++j;
                }
                ++i;
            }
            n_intersections = j;

            while( TRUE )
            {
                for_less( i, 0, n_intersections-1 )
                {
                    if( distances[i+1] - distances[i] < 1e-10 )
                        break;
                }

                if( i >= n_intersections-1 )
                    break;

                j = i+1;
                while( j < n_intersections-1 &&
                       distances[j+1] - distances[i] < 1e-10 )
                    ++j;

                if( i == 0 )
                    sign_before = 1.0;
                else
                    sign_before = directions[i-1];

                if( j == n_intersections-1 )
                    sign_after = -1.0;
                else
                    sign_after = directions[j+1];

                if( sign_before == sign_after )
                {
                    for_less( k, j, n_intersections )
                    {
                        distances[i+k-j] = distances[k];
                        directions[i+k-j] = directions[k];
                    }
                    directions[j] = -sign_before;
                    n_intersections -= j - i;
                }
                else
                {
                    for_less( k, j+1, n_intersections )
                    {
                        distances[i+k-j-1] = distances[k];
                        directions[i+k-j-1] = directions[k];
                    }
                    n_intersections -= j - i + 1;
                }
            }

            if( n_intersections % 2 == 0 )
            {
                ind = 0;
                inside = FALSE;
                for_less( z, 0, sizes[Z] )
                {
                    while( ind <= n_intersections-1 &&
                           (Real) z >= distances[ind] )
                    {
                        inside = !inside;
                        ++ind;
                    }

                    erase_flag = FALSE;
                    if( !inside )
                    {
                      erase_flag = TRUE;
                    }
                    if( erase_flag )
                    {
                      value_to_set = set_value;
                      set_volume_real_value( volume, x, y, z, 0, 0,
                                             value_to_set );
                    }
                    else {
                      set_volume_real_value( volume, x, y, z, 0, 0, 
                                             inside_value );
                    }
                }
            }
            else {
                print( "N intersections: %d\n", n_intersections );
            }

            if( n_intersections > 0 )
            {
                FREE( distances );
                FREE( directions );
            }

            update_progress_report( &progress, x * sizes[Y] + y + 1 );
        }
    }
    
    terminate_progress_report( &progress );

    /* force intersection to be part of mask */
    scan_object_to_volume(objects[0], volume, volume, inside_value, 1);
    
    delete_object_list( n_objects, objects );
    return( 0 );
}



int main ( int argc, char *argv[] )
{
  STRING               input_volume_filename, output_file_name;
  STRING               surface_filename;
  Real                 outside_value, inside_value;
  Volume               binary_volume, out_volume;
  int                  sizes[MAX_DIMENSIONS];
  int                  x,y,z;
  Real                 original_value, binary_value;
  minc_output_options  output_options;
  volume_input_struct  input_struct;
  STRING               *original_dimnames;


  outside_value = 0;
  inside_value = 1;

  if (ParseArgv(&argc, argv, argTable, 0) ) {
    fprintf(stderr, "\nUsage: %s in_volume.mnc surface.obj output.mnc\n",
            argv[0]);
    return(1);
  }
  
  initialize_argument_processing( argc, argv );

  if ( !get_string_argument( NULL, &input_volume_filename ) ||
       !get_string_argument( NULL, &surface_filename ) ||
       !get_string_argument( NULL, &output_file_name )
       ) {
    print_error("Usage: %s in_volume.mnc surface.obj output.mnc \n", argv[0] );
    return( 1 );
  }

  /*
   * open the volume without reading in the voxel data. This will
   * allow us to get the original dimensions names so that we can
   * write the volume out again in the same orientation (we have to
   * read it in XYZ order for the data processing. 
   */

  start_volume_input( input_volume_filename, 3, NULL,
		      MI_ORIGINAL_TYPE, FALSE, 0.0, 0.0, TRUE,
		      &binary_volume, NULL, &input_struct );

  original_dimnames = create_output_dim_names( binary_volume, input_volume_filename,
                                               NULL, sizes );

  delete_volume_input( &input_struct );
  /*
   * create two copies of the volume - one to use for the binary mask,
   * the other to store the original volume in.
   */

  if( input_volume( input_volume_filename, 3, XYZ_dimension_names,
                    MI_ORIGINAL_TYPE, FALSE, 0.0, 0.0, TRUE, 
                    &binary_volume, NULL ) != OK )
    return( 1 );

  out_volume = copy_volume(binary_volume);
  set_volume_real_range(binary_volume, 0, 1);


  /* create the binary mask */
  (void) binary_object_mask( surface_filename,
                             outside_value,
                             inside_value,
                             binary_volume );

  get_volume_sizes( out_volume, sizes );

  /* now test each voxel for inclusion */
  for_less( x, 0, sizes[0] ) {
    for_less( y, 0, sizes[1] ) {
      for_less( z, 0, sizes[2] ) {
        original_value = get_volume_real_value(out_volume, x, y, z, 0, 0);
        binary_value = get_volume_real_value(binary_volume, x, y, z, 0, 0);
        if (binary_value > 0.5) {
          /* keep original value */
          set_volume_real_value(out_volume, x, y, z, 0, 0, original_value);
        }
        else {
          /* erase */
          set_volume_real_value(out_volume, x, y, z, 0, 0, 0);
        }
      }
    }
  }

  /* use the original dimension order rather than the XYZ that we loade */
  set_default_minc_output_options( &output_options );
  set_minc_output_dimensions_order( &output_options, 3, original_dimnames );

  if (output_binary_mask == 1) {
    output_volume(output_file_name, NC_BYTE, FALSE, 0.0, 1.0,
                  binary_volume, "", &output_options );
  }
  else {
    output_volume(output_file_name, MI_ORIGINAL_TYPE, FALSE, 0.0, 0.0,
                  out_volume, "", &output_options );
  }

  return 0;
}


