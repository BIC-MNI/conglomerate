
/*
   Clean up the surface labels obtained using volume_object_evaluate when
   intersecting a cortical surface with the volumetric ANIMAL segmentation.
   Sometimes, there are vertices on the surface that get tagged as CSF but
   these should be projected to the nearest non-CSF tissue label.

   fix_surface_labels surface.obj labels.txt new_labels.txt

   Values: surface.obj = the surface
           labels.txt = input labels (range 0 to 255)
           new_labels.txt = corrected labels

   By: Claude Lepage, February 2007.

   COPYRIGHT: McConnell Brain Imaging Center, 
              Montreal Neurological Institute,
              Department of Psychology,
              McGill University, Montreal, Quebec, Canada. 
  
              Permission to use, copy, modify, and distribute this
              software and its documentation for any purpose and without
              fee is hereby granted, provided that the above copyright
              notice appear in all copies.  The author and McGill University
              make no representations about the suitability of this
              software for any purpose.  It is provided "as is" without
              express or implied warranty.
*/

#include <stdio.h>
#include <volume_io.h>
#include <bicpl.h>

#define BG 0
#define CSF 255

// Prototypes of functions in this file.

static void usage( char * );
private Status read_surface_obj( STRING, int *, Point *[],
                                 Vector *[], int *[], int **[] );
private Status get_surface_neighbours( polygons_struct *, int *[],
                                       int ** [] );
private void read_scalar( int n_points, int scalar[], char * in_file );
private void save_scalar( int n_points, int scalar[], char * out_file );


// Main program.

int main( int argc, char * argv[] ) {

  int      i, j, nn, count, min_lbl, max_lbl, min_count, max_count, modified;
  int      n_points;           // number of grid points 
  Point  * coords;             // coordinates
  Vector * normals;            // normal vectors
  int    * n_ngh = NULL;       // node neighbours (inverse connectivity)
  int   ** ngh = NULL;

  FILE   * fp;

  if( argc < 3 ) {
    usage( argv[0] );
    return( ERROR );
  }

  // Parse the command line arguments for the file names.

  // Read the surface file.
  if( read_surface_obj( argv[1], &n_points, &coords, &normals,
                        &n_ngh, &ngh ) != OK ) {
    return( ERROR );
  }

  int * labels = (int *)malloc( n_points * sizeof( int ) );
  read_scalar( n_points, labels, argv[2] );

  do {
    count = 0;
    modified = 0;
    for( i = 0; i < n_points; i++ ) {
      min_lbl = 999999;
      max_lbl = -1;
      for( j = 0; j < n_ngh[i]; j++ ) {
        nn = ngh[i][j];
        if( labels[nn] < min_lbl && labels[nn] != BG ) min_lbl = labels[nn];
        if( labels[nn] > max_lbl && labels[nn] != CSF ) max_lbl = labels[nn];
      }
      if( min_lbl == max_lbl ) {
        if( labels[i] != min_lbl ) {
          if( min_lbl != CSF && min_lbl != BG ) {
            labels[i] = min_lbl;
            modified++;
          }
        }
#if 1
      } else {
        if( labels[i] != min_lbl && labels[i] != max_lbl ) {
          min_count = 0;
          max_count = 0;
          for( j = 0; j < n_ngh[i]; j++ ) {
            nn = ngh[i][j];
            if( labels[nn] == min_lbl ) min_count++;
            if( labels[nn] == max_lbl ) max_count++;
          }
          if( min_count + max_count == n_ngh[i] ) {
            modified++;
            if( min_count >= max_count ) {
              labels[i] = min_lbl;
            } else {
              labels[i] = max_lbl;
            }
          }
        }
#endif
      }

      if( labels[i] == CSF ) {
        count++;
        for( j = 0; j < n_ngh[i]; j++ ) {
          nn = ngh[i][j];
          if( labels[nn] != BG && labels[nn] != CSF ) {
            labels[i] = labels[nn];
            modified++;
            break;
          }
        }
      }
    }
    printf( "modified %d labels\n", modified );
  } while( modified > 0 );
  if( count > 0 ) printf( "There are %d CSF labels left\n", count );

  save_scalar( n_points, labels, argv[3] );

  free( labels );

  if( coords ) FREE( coords );
  if( normals ) FREE( normals );
  if( n_ngh ) FREE( n_ngh );
  if( ngh ) {
    FREE( ngh[0] );   // this is ngh_array
    FREE( ngh );
  }

  return 0;
}


// -------------------------------------------------------------------
// Help message on how to use this module.
//
static void usage( char * executable_name ) {

  STRING  usage_format = "\
Usage: %s surface.obj labels.txt new_labels.txt\n\
Values: surface.obj = the surface\n\
        labels.txt = input labels (range 0 to 255)\n\
        new_labels.txt = corrected labels\n\n";

  print_error( usage_format, executable_name );
}


// -------------------------------------------------------------------
// Load the cortical surface.
//
// filename: name of the .obj file
// n_points: the number of the vertices
// points: (x,y,z) coordinates
// normals: normal vectors
// n_neighbours: number of vertices around each node
// neighbours: the set of ordered triangle consisting of the vertices
//
private Status read_surface_obj( STRING filename,
                                 int * n_points,
                                 Point * points[],
                                 Vector * normals[],
                                 int * n_neighbours[],
                                 int ** neighbours[] ) {

  int               i, n_objects;
  object_struct  ** object_list;
  polygons_struct * surface;
  File_formats      format;
  STRING            expanded;

  expanded = expand_filename( filename );   // why?????

  int err = input_graphics_file( expanded, &format, &n_objects,
                                 &object_list );

  if( err != OK ) {
    print_error( "Error reading file %s\n", expanded );
    return( ERROR );
  }

  if( n_objects != 1 ||
      ( n_objects == 1 && get_object_type(object_list[0]) != POLYGONS ) ) {
    print_error( "Error in contents of file %s\n", expanded );
    return( ERROR );
  }

  delete_string( expanded );

  surface = get_polygons_ptr( object_list[0] );

  // Make a copy of the coordinates and the normals, since
  // delete_object_list will destroy them.

  *n_points = surface->n_points;
  ALLOC( *points, surface->n_points );
  ALLOC( *normals, surface->n_points );
  for( i = 0; i < *n_points; i++ ) {
    (*points)[i].coords[0] = surface->points[i].coords[0];
    (*points)[i].coords[1] = surface->points[i].coords[1];
    (*points)[i].coords[2] = surface->points[i].coords[2];
    (*normals)[i].coords[0] = surface->normals[i].coords[0];
    (*normals)[i].coords[1] = surface->normals[i].coords[1];
    (*normals)[i].coords[2] = surface->normals[i].coords[2];
  }

  get_surface_neighbours( surface, n_neighbours, neighbours );

  delete_object_list( n_objects, object_list );

  return( OK );
}

// -------------------------------------------------------------------
// Construct the edges around each node. The edges are sorted to
// make an ordered closed loop.
//
private Status get_surface_neighbours( polygons_struct * surface,
                                       int * n_neighbours_return[],
                                       int ** neighbours_return[] ) {

  int    i, j, k, jj;
  int  * tri;
  int  * n_ngh;
  int ** ngh;
  int  * ngh_array;

  // Check if all polygons are triangles.

  if( 3 * surface->n_items != surface->end_indices[surface->n_items-1] ) {
    printf( "Surface must contain only triangular polygons.\n" );
    return ERROR;
  }

  // Check if the node numbering starts at 0 or 1.

  int min_idx, max_idx;

  min_idx = 100*surface->n_points;  // anything big
  max_idx = 0;                      // anything small

  for( i = 0; i < 3*surface->n_items; i++ ) {
    if( surface->indices[i] < min_idx ) min_idx = surface->indices[i];
    if( surface->indices[i] > max_idx ) max_idx = surface->indices[i];
  }

  // Shift numbering to start at zero, for array indexing. Note
  // that we don't care if surface->indices array is modified.

  if( min_idx != 0 ) {
    for( i = 0; i < 3*surface->n_items; i++ ) {
      surface->indices[i] -= min_idx;
    }
  }

  // Count number of triangles attached to each node.

  ALLOC( n_ngh, surface->n_points );
  ALLOC( ngh, surface->n_points );
  ALLOC( ngh_array, 3*surface->n_items );

  for( i = 0; i < surface->n_points; i++ ) {
    n_ngh[i] = 0;
  }

  for( i = 0; i < 3*surface->n_items; i++ ) {
    n_ngh[surface->indices[i]]++;
    ngh_array[i] = -1;
  }

  int max_ngh = 0;
  int sum_ngh = 0;
  for( i = 0; i < surface->n_points; i++ ) {
    ngh[i] = &(ngh_array[sum_ngh]);
    sum_ngh += n_ngh[i];
    max_ngh = MAX( max_ngh, n_ngh[i] );
  }

  // At first, store the indices of the triangles in the neighbours.
  for( i = 0; i < surface->n_items; i++ ) {
    for( j = 0; j < 3; j++ ) {
      jj = surface->indices[3*i+j];
      for( k = 0; k < n_ngh[jj]; k++ ) {
        if( ngh[jj][k] == -1 ) {
          ngh[jj][k] = i;
          break;
        }
      }
    }
  }

  // Now create a sort closed loop of the node neighbours.
  // This is needed by the parametric=0 FEM algorithm.
  //
  //         1 ----- 2
  //          /\   /\
  //         /  \ /  \
  //       0 ----P---- 3
  //         \  / \  /
  //          \/   \/
  //         5 ----- 4
  //

  int * tmp;
  ALLOC( tmp, 2*max_ngh );

  for( i = 0; i < surface->n_points; i++ ) {
    for( k = 0; k < n_ngh[i]; k++ ) {
      tri = &(surface->indices[3*ngh[i][k]]);
      for( j = 0; j < 3; j++ ) {
        if( tri[j] == i ) break;
      }
      tmp[2*k+0] = tri[(j+1)%3];
      tmp[2*k+1] = tri[(j+2)%3];
    }

    ngh[i][0] = tmp[0];
    ngh[i][1] = tmp[1];
    for( k = 2; k < n_ngh[i]; k++ ) {
      for( j = 1; j < n_ngh[i]; j++ ) {
        if( tmp[2*j] == ngh[i][k-1] || tmp[2*j+1] == ngh[i][k-1] ) {
          if( tmp[2*j] == ngh[i][k-1] ) {
            ngh[i][k] = tmp[2*j+1];
          } else {
            ngh[i][k] = tmp[2*j];
          }
          tmp[2*j] = -1;
          tmp[2*j+1] = -1;
          break;
        }
      }
    }
  }

  *n_neighbours_return = n_ngh;
  *neighbours_return = ngh;

  FREE( tmp );

  return OK;

}

// -------------------------------------------------------------------
// Read a scalar from a file.
//
private void read_scalar( int n_points, int scalar[], char * in_file ) {

  int i;
  float val;

  FILE * fp = fopen( in_file, "r" );
  for( i = 0; i < n_points; i++ ) {
    fscanf( fp, "%f", &val );
    scalar[i] = (int)( val + 0.25 );  // truncate to nearest if noise from minc ranges.
  }
  fclose( fp );
}

// -------------------------------------------------------------------
// Save a scalar to a file.
//
private void save_scalar( int n_points, int scalar[], char * out_file ) {

  int i;

  FILE * fp = fopen( out_file, "w" );
  for( i = 0; i < n_points; i++ ) {
    fprintf( fp, "%d\n", scalar[i] );
  }
  fclose( fp );
}


