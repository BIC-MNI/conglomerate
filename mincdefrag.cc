/**
 * Remove disconnected islands of voxels of a given label.
 * Author: Claude Lepage, 2006.
 **/

#include <iostream>
#include <iomanip>
extern "C" { 
#include <volume_io.h>
#include <time_stamp.h> 
}

using namespace std;

//
// Try to remove dangling pieces of matter of this color.
//
int clean_color( int sizes[MAX_DIMENSIONS], short * val, short color,
                 short stencil, int max_connect ) {

    int     i, j, k, ii, jj;
    int     changed, num_color, n_voxels;
    short * old_im1, * old_i, * old_ip1;
    int     num_pieces = 0;
    int     total_kept = 0;
    int     total_removed = 0;
    int     di, dj, dk;

    n_voxels = sizes[0] * sizes[1] * sizes[2];

    num_color = 0;
    for( ii = 0;  ii < n_voxels;  ii++ ) {
      if( val[ii] == color ) num_color++;
    }
    // cout << num_color << " voxels of color " << color
    //      << " in original volume" << endl;

    // specified size of at least 5% of total size otherwise
    if( max_connect <= 0 ) {
      max_connect = (int)(0.05*num_color);
    }

    // Try to propagate color voxels to eliminate disjoint pieces.

    short * vflag = new short[n_voxels];
    int * vstack = new int[n_voxels];

    for( ii = 0; ii < n_voxels; ii++ ) {
      vflag[ii] = -1;
    }
    for( ii = 0; ii < n_voxels; ii++ ) {
      if( val[ii] == color && vflag[ii] == -1 ) {
        int   stack_size = 0;
        int   count = 0;
        vstack[stack_size] = ii;
        vflag[ii] = num_pieces;
        stack_size++;
        while( stack_size > 0 ) {
          stack_size--;
          jj = vstack[stack_size];
          count++;
          i = jj / ( sizes[1] * sizes[2] );
          j = ( jj - i * sizes[1] * sizes[2] ) / sizes[2];
          k = jj - i * sizes[1] * sizes[2] - j * sizes[2];

          for( di = -1; di <= 1; di++ ) {
            for( dj = -1; dj <= 1; dj++ ) {
              for( dk = -1; dk <= 1; dk++ ) {
                if( ABS(di) + ABS(dj) + ABS(dk) <= stencil ) {
                  jj = (i+di) * sizes[1] * sizes[2] + (j+dj) * sizes[2] + k+dk;
                  if( val[jj] == color && vflag[jj] == -1 ) {
                    vstack[stack_size] = jj;
                    vflag[jj] = num_pieces;
                    stack_size++;
                  }
                }
              }
            }
          }
        }

        if( count <= max_connect ) {
          total_removed += count;
          // cout << "remove piece " << num_pieces << " with "
          //      << count << " voxels of color " << color << endl;
        } else {
          total_kept += count;
          // cout << "keep piece " << num_pieces << " with "
          //      << count << " voxels of color " << color << endl;
          // Everything to keep has vflag = -2.
          for( jj = 0; jj < n_voxels; jj++ ) {
            if( vflag[jj] == num_pieces ) vflag[jj] = -2;
          }
        }

        num_pieces++;
      }
    }

    cout << "Kept total of " << total_kept << " voxels of label "
         << color << endl;
    cout << "Removed total of " << total_removed << " voxels of label "
         << color << endl;

    // Try to decide new color to switch deleted voxels to. Take minimum.

    short * ngh_color = new short[num_pieces];
    for( ii = 0; ii < num_pieces; ii++ ) {
      ngh_color[ii] = 1000;
    }

    for( ii = 0; ii < n_voxels; ii++ ) {
      if( vflag[ii] >= 0 ) {
        i = ii / ( sizes[1] * sizes[2] );
        j = ( ii - i * sizes[1] * sizes[2] ) / sizes[2];
        k = ii - i * sizes[1] * sizes[2] - j * sizes[2];
        short min_color = ngh_color[vflag[ii]];

        for( di = -1; di <= 1; di++ ) {
          for( dj = -1; dj <= 1; dj++ ) {
            for( dk = -1; dk <= 1; dk++ ) {
              if( ABS(di) + ABS(dj) + ABS(dk) <= stencil ) {
                jj = (i+di) * sizes[1] * sizes[2] + (j+dj) * sizes[2] + k+dk;
                if( val[jj] != color ) min_color = min( min_color, val[jj] );
              }
            }
          }
        }
        ngh_color[vflag[ii]] = min_color;
      }
    }

    // Keep only the main piece(s) of this color, while keeping the 
    // other voxels at the same color.
 
    for( ii = 0; ii < n_voxels; ii++ ) {
      if( val[ii] == color && vflag[ii] >= 0 ) {
        val[ii] = ngh_color[vflag[ii]];
      }
    }

    delete[] ngh_color;
    delete[] vflag;
    delete[] vstack;

    return( OK );
}

int extract_color( int sizes[MAX_DIMENSIONS], short color,
                   short stencil, int max_connect, Volume volume ) {

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
          val[count] = (short)(0.2 + fval );  // rounding to nearest integer
          count++;
        }
      }
    }

    // Try to remove dangling pieces of color matter.

    int ret = OK;
    ret = clean_color( sizes, val, color, stencil, max_connect );

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
  
    if( ac < 5 ) {
      cerr << "Usage: " << av[0] << " input.mnc output.mnc label stencil [max_connect]" 
           << endl;
      cerr << "       label = integer label for voxel intensity" << endl;
      cerr << "       stencil = number of neighbours (6, 19, 27)" << endl;
      cerr << "       max_connect = threshold for number of connected voxels" 
           << endl;
      return 1;
    }

    // Read the volume. 
    Volume in_volume;
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

    int sizes[MAX_DIMENSIONS];
    get_volume_sizes( in_volume, sizes );

    short color = (short)atoi( av[3] );
 
    short stencil = (short)atoi( av[4] );
    if( stencil == 27 ) {
      stencil = 3;
    } else if( stencil == 19 ) {
      stencil = 2;
    } else {
      stencil = 1;
    }

    int max_connect = -1;
    if( ac == 6 ) {
      max_connect = (int)atoi( av[5] );
    }

    int ret = extract_color( sizes, color, stencil, max_connect, in_volume );
    if( ret != OK ) return( 1 );

    int rv = output_modified_volume( av[2], MI_ORIGINAL_TYPE,
                                     0, 0, 0, in_volume, av[1],
                                     time_stamp( ac, av ), NULL );
    return ( rv != OK );

}

