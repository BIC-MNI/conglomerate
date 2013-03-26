/**
 * Create a rough skeleton of the white matter.
 * By: Claude Lepage, Dec 2005.
 **/

#include <iostream>
#include <iomanip>
extern "C" { 
#include <volume_io.h>
#include <time_stamp.h> 
}

using namespace std;

//
// Make a thin skeleton of the image.
//
int make_skel( int sizes[VIO_MAX_DIMENSIONS], short * val ) {

    int     i, j, k, ii, jj;
    int     changed;
    int     di, dj, dk;

    int     stencil = 1;
    int     n_voxels = sizes[0] * sizes[1] * sizes[2];
 
    // Try to propagate color voxels to eliminate disjoint pieces.

    for( ii = 0; ii < n_voxels; ii++ ) {
      if( val[ii] > 0 ) {
        val[ii] = -1;
      } else {
        val[ii] = 0;
      }
    }

    int nlevel = 0;
    do {
      changed = 0;
      for( ii = 0; ii < n_voxels; ii++ ) {
    
        if( val[ii] == -1 ) {
          short found = 0;
          i = ii / ( sizes[1] * sizes[2] );
          j = ( ii - i * sizes[1] * sizes[2] ) / sizes[2];
          k = ii - i * sizes[1] * sizes[2] - j * sizes[2];

          for( di = -1; di <= 1; di++ ) {
            if( i + di < 0 || i + di >= sizes[0] ) continue;
            for( dj = -1; dj <= 1; dj++ ) {
              if( j + dj < 0 || j + dj >= sizes[1] ) continue;
              for( dk = -1; dk <= 1; dk++ ) {
                if( k + dk < 0 || k + dk >= sizes[2] ) continue;
                if( ABS(di) + ABS(dj) + ABS(dk) <= stencil ) {
                  jj = (i+di) * sizes[1] * sizes[2] + (j+dj) * sizes[2] + k+dk;
                  if( val[jj] == nlevel ) found++;
                }
              }
            }
          }
          if( found ) {
            val[ii] = nlevel+1;
            changed++;
          }
        }
      }
      // cout << "Layer " << nlevel << " with " << changed << "voxels" << endl;
      nlevel++;
    } while( changed > 0 );

    // Create the skeleton.
    // Step 1: Find local max.

    short * flag = new short[n_voxels];

    for( ii = 0; ii < n_voxels; ii++ ) {
      flag[ii] = 0;
    }
    for( ii = 0; ii < n_voxels; ii++ ) {
      if( val[ii] > 0 ) {
        short found = 0;
        i = ii / ( sizes[1] * sizes[2] );
        j = ( ii - i * sizes[1] * sizes[2] ) / sizes[2];
        k = ii - i * sizes[1] * sizes[2] - j * sizes[2];

        for( di = -1; di <= 1; di++ ) {
          if( i + di < 0 || i + di >= sizes[0] ) continue;
          for( dj = -1; dj <= 1; dj++ ) {
            if( j + dj < 0 || j + dj >= sizes[1] ) continue;
            for( dk = -1; dk <= 1; dk++ ) {
              if( k + dk < 0 || k + dk >= sizes[2] ) continue;
              if( ABS(di) + ABS(dj) + ABS(dk) <= stencil ) {
                jj = (i+di) * sizes[1] * sizes[2] + (j+dj) * sizes[2] + k+dk;
                if( val[jj] > val[ii] ) found++;
              }
            }
          }
        }
        if( !found ) {
          flag[ii] = 2;
        }
      }
    }

    // Step 2: Add a connectivity layer to fill up gaps.
    stencil = 1;
    for( ii = 0; ii < n_voxels; ii++ ) {
      if( flag[ii] == 2 ) {
        i = ii / ( sizes[1] * sizes[2] );
        j = ( ii - i * sizes[1] * sizes[2] ) / sizes[2];
        k = ii - i * sizes[1] * sizes[2] - j * sizes[2];

        for( di = -1; di <= 1; di++ ) {
          if( i + di < 0 || i + di >= sizes[0] ) continue;
          for( dj = -1; dj <= 1; dj++ ) {
            if( j + dj < 0 || j + dj >= sizes[1] ) continue;
            for( dk = -1; dk <= 1; dk++ ) {
              if( k + dk < 0 || k + dk >= sizes[2] ) continue;
              if( ABS(di) + ABS(dj) + ABS(dk) <= stencil ) {
                jj = (i+di) * sizes[1] * sizes[2] + (j+dj) * sizes[2] + k+dk;
                if( val[jj] > 0 && flag[jj] == 0 ) flag[jj] = 1;
              }
            }
          }
        }
      }
    }

    // Step 3: Thin out skeleton a little bit. Remove newly added voxels
    //         that have only one connection.
    stencil = 1;
    for( ii = 0; ii < n_voxels; ii++ ) {
      if( flag[ii] == 1 ) {
        short count = 0;
        i = ii / ( sizes[1] * sizes[2] );
        j = ( ii - i * sizes[1] * sizes[2] ) / sizes[2];
        k = ii - i * sizes[1] * sizes[2] - j * sizes[2];

        for( di = -1; di <= 1; di++ ) {
          if( i + di < 0 || i + di >= sizes[0] ) continue;
          for( dj = -1; dj <= 1; dj++ ) {
            if( j + dj < 0 || j + dj >= sizes[1] ) continue;
            for( dk = -1; dk <= 1; dk++ ) {
              if( k + dk < 0 || k + dk >= sizes[2] ) continue;
              if( ABS(di) + ABS(dj) + ABS(dk) <= stencil ) {
                jj = (i+di) * sizes[1] * sizes[2] + (j+dj) * sizes[2] + k+dk;
                if( flag[jj] > 0 ) count++;
              }
            }
          }
        }
        if( count <= 2 ) {   // count includes itself ii==jj
          flag[ii] = 0;
          changed++;
        }
      }
    }

    // Copy skeleton back to volume.
    for( ii = 0; ii < n_voxels; ii++ ) {
      if( flag[ii] > 0 ) {
        val[ii] = 1;
      } else {
        val[ii] = 0;
      }
    }

    delete [] flag;

    return( OK );
}

int skel( int sizes[VIO_MAX_DIMENSIONS], VIO_Volume volume ) {

    int    i, j, k, ii;
    float  fval;

    int count;

    short * val = NULL;
    int * nodes = NULL;

    short * old_im1, * old_i, * old_ip1;

    // Copy the volume data in temporary arrays, for speed.
    // This is faster than calling get_volume_real_value
    // many times.

    val = new short[sizes[0]*sizes[1]*sizes[2]];

    count = 0;
    for( int i = 0;  i < sizes[0];  ++i ) {
      for( int j = 0;  j < sizes[1];  ++j ) {
        for( int k = 0;  k < sizes[2];  ++k ) {
          fval = get_volume_real_value( volume, i, j, k, 0, 0 );
          if( fval > 0.0 ) {
            val[count] = 1;        // binary mask
          } else {
            val[count] = 0;
          }
          count++;
        }
      }
    }

    // Make the skeleton.

    int ret = OK;
    ret = make_skel( sizes, val );

    // Save back to volume.

    count = 0;
    for( int i = 0;  i < sizes[0];  ++i ) {
      for( int j = 0;  j < sizes[1];  ++j ) {
        for( int k = 0;  k < sizes[2];  ++k ) {
          set_volume_real_value( volume, i, j, k, 0, 0, (float)val[count] );
          count++;
        }
      }
    }

    delete[] val;
    return( ret );
}



int  main( int ac, char* av[] ) {
  
    if( ac < 3 ) {
      cerr << "Usage: " << av[0] << " pve_wm.mnc skel_wm.mnc " 
           << endl;
      return 1;
    }

    // Read the volume. 
    VIO_Volume in_volume;
    if ( input_volume( av[1], 3, NULL, 
                       MI_ORIGINAL_TYPE, 0, 0, 0,
                       TRUE, &in_volume, NULL ) != OK ) {
      cerr << "Error: cannot read volume file " << av[1] << endl;
      return 1;
    }

    if ( get_volume_n_dimensions( in_volume ) != 3 ) {
      cerr << "Error: volume in " << av[1] 
           << " does not have three dimensions." << endl;
      return 1;
    }

    int sizes[VIO_MAX_DIMENSIONS];
    get_volume_sizes( in_volume, sizes );

    int ret = skel( sizes, in_volume );
    if( ret != OK ) return( 1 );

    int rv = output_modified_volume( av[2], MI_ORIGINAL_TYPE,
                                     0, 0, 0, in_volume, av[1],
                                     time_stamp( ac, av ), NULL );
    return ( rv != OK );

}

