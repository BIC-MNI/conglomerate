/* ----------------------------- MNI Header -----------------------------------
@NAME       : find_peaks.c
@INPUT      : argc, argv - command line arguments
@OUTPUT     : (nothing)
@RETURNS    : status
@DESCRIPTION: Program to find the peaks in a volume. The handling of plateaus
              is fairly simplistic: any voxel that is greater or equal to all
              of its neighbours is considered a peak if it is greater than
              at least one of them (ie. not equal to all of them). This
              means that all voxels on the edge of a plateau are considered 
              peaks, but the plateau itself is not.
@CREATED    : February 1, 1999 (Peter Neelin)
@MODIFIED   : $Log: find_peaks.c,v $
@MODIFIED   : Revision 1.1  2004-04-22 13:15:01  bert
@MODIFIED   : Initial addition to conglomerate
@MODIFIED   :
 * Revision 1.11  2001/04/18  13:27:00  neelin
 * Addition of -min_distance option.
 *
 * Revision 1.10  2000/11/30  15:59:55  neelin
 * Fixed handling of invalid values. As neighbours, these are treated as
 * edges and behaviour is affected by the -include_edges / -exclude_edges
 * options.
 *
 * Revision 1.9  2000/10/19  17:05:54  neelin
 * Fixed up logging of point information.
 *
 * Revision 1.8  2000/10/19  16:48:21  neelin
 * More re-arrangements to do max and min search at the same time
 * (faster) and to put global variables in a structure (other than
 * logging level).
 *
 * Revision 1.7  2000/10/19  13:50:10  neelin
 * Some small speed improvements and code rationalization.
 *
 * Revision 1.6  2000/10/18  18:54:13  neelin
 * Fixed bug in plateau searching that marked all plateaus as false.
 *
 * Revision 1.5  2000/10/18  15:36:07  neelin
 * Minor fix to -help description.
 *
 * Revision 1.4  2000/10/18  15:07:45  neelin
 * Added code to handle peak plateaus and to calculate centroid.
 * Also added verbosity options and sensible default thresholds.
 *
 * Revision 1.3  1999/02/04  19:28:04  neelin
 * Added check for doing only positive or negative peaks.
 *
 * Revision 1.2  1999/02/03  15:12:50  neelin
 * Added rcsid string.
 *
 * Revision 1.1  1999/02/03  15:11:03  neelin
 * Initial revision
 *
---------------------------------------------------------------------------- */

#ifndef lint
static char rcsid[]="$Header: /private-cvsroot/libraries/conglomerate/find_peaks.c,v 1.1 2004-04-22 13:15:01 bert Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <limits.h>
#include <volume_io.h>
#include <ParseArgv.h>
#include <time_stamp.h>

/* Constants */
#define NUM_EULER_TERMS 4
#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif

#define START_ALLOC 100
#define ALLOC_FACTOR 1.4
#define MAX_ALLOC 10000
#define START_INDEX (-1)
#define END_INDEX 1
#define NUM_NEIGHBOURS (END_INDEX - START_INDEX + 1)
#define NUM_OFFSETS (NUM_NEIGHBOURS * NUM_NEIGHBOURS * NUM_NEIGHBOURS)
#define VQUEUE_BLOCK_LENGTH 64
#define DEFAULT_THRESHOLD_FRACTION 0.8

/* Flags for peaks */
#define CHECKED_FLAG 1

/* Types */
typedef enum {NO_LOGGING, LOW_LOGGING, HIGH_LOGGING} LogLevelType;
typedef enum {SLC, ROW, COL, VOXEL_NDIMS} Voxel_Index;
typedef enum {XW, YW, ZW, WORLD_NDIMS} World_Index;
typedef struct {
   Real coord[WORLD_NDIMS];
   Real value;
   int valid;
} Tag;
typedef enum {MAXIMUM, MINIMUM, NEXTREMES, NOT_A_PEAK} Extreme_Index;

typedef struct {
   double real_threshold[NEXTREMES];
   double threshold[NEXTREMES];
   double minimum_distance;
   int include_edges;
   int positive_only;
   int negative_only;
   int false_plateaus;
   int true_plateaus;
   int true_points;
} PeakParams;

/* Types for the voxel index queue. The queue is a ring of blocks */
typedef struct VQueueBlock *VQueueBlock;
struct VQueueBlock {
  VQueueBlock next;
  int coord[VQUEUE_BLOCK_LENGTH][VOXEL_NDIMS];
};
typedef struct {
   VQueueBlock head;
   VQueueBlock tail;
   int head_index;
   int tail_index;
#ifdef VQUEUE_DEBUG
   int size;
   int max_size;
   int num_ring_blocks;
#endif
} *VoxelQueue;

/* Macros */
#ifdef MALLOC
#  undef MALLOC
#endif
#ifdef FREE
#  undef FREE
#endif
#ifdef REALLOC
#  undef REALLOC
#endif
#ifdef CALLOC
#  undef CALLOC
#endif

#define MALLOC(size) ((void *) malloc(size))

#define FREE(ptr) free(ptr)

#define REALLOC(ptr, size) ((void *) realloc(ptr, size))

#define CALLOC(nelem, elsize) ((void *) calloc(nelem, elsize))

/* Functions */
int sort_ascending(const void *val1, const void *val2);
int sort_descending(const void *val1, const void *val2);
void find_peaks(Volume volume, PeakParams *peak_params,
                int *nmin, Tag **min_peaks, 
                int *nmax, Tag **max_peaks);
int setup_neighbours(int offsets[][VOXEL_NDIMS]);
Extreme_Index check_for_peak(Volume volume, int index[], int sizes[],
                             int noffsets, int offsets[][VOXEL_NDIMS],
                             PeakParams *peak_params, int include_plateau, 
                             Real *value, 
                             int *num_equal_neighbours, int *equal_neighbours);
Extreme_Index find_peak_centroid(Volume volume, 
                                 Volume flag_volume, int *flags_alloced,
                                 int index[], int sizes[],
                                 int noffsets, int offsets[][VOXEL_NDIMS],
                                 PeakParams *peak_params, 
                                 Real *value, Real centroid[]);
int check_and_mark_flag(Volume flag_volume, int *index);
void allocate_flag_volume(Volume flag_volume, int *current);
void reset_vqueue(VoxelQueue *voxel_queue);
void delete_vqueue(VoxelQueue queue);
void add_vqueue_tail(VoxelQueue queue, int coord[]);
int remove_vqueue_head(VoxelQueue queue, int coord[]);
void do_not_print(char *string);
void reset_peak_counters(PeakParams *peak_params);
void increment_peak_counter(PeakParams *peak_params, 
                            int have_a_peak, int is_a_plateau);
void print_peak_counters(PeakParams *peak_params);
void log_peak_info(Volume volume, char *type, Real value, Real centroid[],
                   int num_centroid_voxels);

/* Globals for argument parsing */
PeakParams Peak_parameters = {
   {0.0, 0.0}, {0.0, 0.0},          /* thresholds */
   0.0,                             /* minimum distance */
   FALSE,                           /* include_edges */
   FALSE,                           /* positive_only */
   FALSE,                           /* negative_only */
   0, 0, 0                          /* counters */
};
LogLevelType LogLevel = LOW_LOGGING;

/* Argument table */
ArgvInfo argTable[] = {
   {"-quiet", ARGV_CONSTANT, (char *) NO_LOGGING, (char *) &LogLevel,
       "Do not print out log messages."},
   {"-verbose", ARGV_CONSTANT, (char *) LOW_LOGGING, (char *) &LogLevel,
       "Print out log messages as processing is being done (default)."},
   {"-veryverbose", ARGV_CONSTANT, (char *) HIGH_LOGGING, (char *) &LogLevel,
       "Print out a lot of log messages as processing is being done."},
   {"-exclude_edges", ARGV_CONSTANT, (char *) FALSE, 
       (char *) &Peak_parameters.include_edges,
       "Exclude edges of volume from search for peaks (default)."},
   {"-include_edges", ARGV_CONSTANT, (char *) TRUE, 
       (char *) &Peak_parameters.include_edges,
       "Include edges of volume in search for peaks."},
   {"-pos_threshold", ARGV_FLOAT, (char *) 1, 
       (char *) &Peak_parameters.real_threshold[MAXIMUM],
       "Threshold for positive peaks (default = 80% full range)"},
   {"-neg_threshold", ARGV_FLOAT, (char *) 1,
       (char *) &Peak_parameters.real_threshold[MINIMUM],
       "Threshold for negative peaks (default = 20% full range)"},
   {"-pos_only", ARGV_CONSTANT, (char *) TRUE, 
       (char *) &Peak_parameters.positive_only,
       "Write out only positive peaks."},
   {"-neg_only", ARGV_CONSTANT, (char *) TRUE, 
       (char *) &Peak_parameters.negative_only,
       "Write out only negative peaks."},
   {"-min_distance", ARGV_FLOAT, (char *) 1, 
       (char *) &Peak_parameters.minimum_distance,
       "Specify minimum distance between peaks."},
   {(char *) NULL, ARGV_END, (char *) NULL, (char *) NULL,
       (char *) NULL}
};

/* Main program */

int main(int argc, char *argv[])
{
   char *pname;
   char *infile, *outfile;
   Volume volume;
   char *history;
   minc_input_options options;
   int nmax_peaks, nmin_peaks;
   Tag *max_peaks, *min_peaks, *this_tag;
   int total_ntags, itag;
   Real volume_min, volume_max;
   Real **output_tags;
   char **output_labels;
   char string[20];

   /* Set default thresholds */
   Peak_parameters.real_threshold[MAXIMUM] = DBL_MAX;
   Peak_parameters.real_threshold[MINIMUM] = -DBL_MAX;

   /* Save history */
   history = time_stamp(argc, argv);

   /* Check arguments */
   pname = argv[0];
   if (ParseArgv(&argc, argv, argTable, 0) || (argc != 3)) {
      (void) fprintf(stderr, 
         "Usage: %s [options] <in>.mnc <out>.tag\n", pname);
      (void) fprintf(stderr, 
         "       %s -help\n", pname);
      return EXIT_FAILURE;
   }
   infile = argv[1];
   outfile = argv[2];

   /* Check for inconsistent options */
   if (Peak_parameters.negative_only && Peak_parameters.positive_only) {
      (void) fprintf(stderr, 
                     "Please do not specify both -neg_only and -pos_only\n");
      return EXIT_FAILURE;
   }

   /* Check for no logging */
   if (LogLevel == NO_LOGGING) {
      set_print_function(do_not_print);
   }

   /* Set input options */
   set_default_minc_input_options(&options);
   set_minc_input_promote_invalid_to_min_flag(&options, FALSE);
   set_minc_input_vector_to_scalar_flag(&options, FALSE);

   /* Read in the volume */
   if (input_volume(infile, VOXEL_NDIMS, NULL, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                    TRUE, &volume, &options) != OK) {
      (void) fprintf(stderr, "Error loading volume %s\n", infile);
      return EXIT_FAILURE;
   }

   /* Set default thresholds if needed */
   get_volume_real_range(volume, &volume_min, &volume_max);
   if (Peak_parameters.real_threshold[MAXIMUM] == DBL_MAX) {
      Peak_parameters.real_threshold[MAXIMUM] = 
         (volume_max - volume_min) * DEFAULT_THRESHOLD_FRACTION + volume_min;
      if (LogLevel >= LOW_LOGGING && 
          !Peak_parameters.negative_only) {
         print("Using positive peak threshold %.6g\n", 
               Peak_parameters.real_threshold[MAXIMUM]);
      }
   }
   if (Peak_parameters.real_threshold[MINIMUM] == -DBL_MAX) {
      Peak_parameters.real_threshold[MINIMUM] = 
         (volume_min - volume_max) * DEFAULT_THRESHOLD_FRACTION + volume_max;
      if (LogLevel >= LOW_LOGGING && 
          !Peak_parameters.positive_only) {
         print("Using negative peak threshold %.6g\n", 
               Peak_parameters.real_threshold[MINIMUM]);
      }
   }

   /* Find the peaks */
   find_peaks(volume, &Peak_parameters,
              &nmin_peaks, &min_peaks, &nmax_peaks, &max_peaks);

   /* Write out a combined tag file */
   if (Peak_parameters.positive_only) total_ntags = nmax_peaks;
   else if (Peak_parameters.negative_only) total_ntags = nmin_peaks;
   else total_ntags = nmax_peaks + nmin_peaks;
   output_tags = MALLOC(total_ntags * sizeof(*output_tags));
   output_labels = MALLOC(total_ntags * sizeof(*output_labels));
   for (itag=0; itag < total_ntags; itag++) {
      if (Peak_parameters.positive_only) this_tag = &max_peaks[itag];
      else if (Peak_parameters.negative_only) this_tag = &min_peaks[itag];
      else if (itag < nmax_peaks) this_tag = &max_peaks[itag];
      else this_tag = &min_peaks[itag - nmax_peaks];
      output_tags[itag] = this_tag->coord;
      (void) sprintf(string, "%.4g", this_tag->value);
      output_labels[itag] = strdup(string);
   }
   if (output_tag_file(outfile, history, 2, total_ntags, 
                       output_tags, output_tags, 
                       NULL, NULL, NULL, output_labels) != OK) {
      (void) fprintf(stderr, "Error writing out labels\n");
      return EXIT_FAILURE;
   }

   /* Free things */
   FREE(output_tags);
   FREE(min_peaks);
   FREE(max_peaks);
   delete_volume(volume);

   return EXIT_SUCCESS;

}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : find_peaks
@INPUT      : volume
              peak_params - parameters for peak search
@OUTPUT     : nmin, nmax - number of min and max peaks
              min_peaks, max_peaks - list of min and max peaks
@RETURNS    : (nothing)
@DESCRIPTION: Routine to find the positive and negative peaks in a volume
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 13, 2000
@MODIFIED   : 
---------------------------------------------------------------------------- */
void find_peaks(Volume volume, PeakParams *peak_params,
                int *nmin, Tag **min_peaks, 
                int *nmax, Tag **max_peaks)
{
   int sizes[VOXEL_NDIMS];
   int start[VOXEL_NDIMS], end[VOXEL_NDIMS], index[VOXEL_NDIMS];
   int offsets[NUM_OFFSETS][VOXEL_NDIMS];
   int noffsets;
   Voxel_Index idim;
   World_Index iworld;
   Extreme_Index iextreme;
   Tag *tags[NEXTREMES];
   int ntags[NEXTREMES];
   int ntags_alloc[NEXTREMES];
   int ntags_increm[NEXTREMES];
   int itag, jtag;
   Real x_world, y_world, z_world;
   Real value;
   Volume flag_volume;
   int flags_alloced;
   Real centroid[VOXEL_NDIMS];
   progress_struct progress;
   double distance2, diff, mindist2;

   /* Reset peak counters */
   reset_peak_counters(peak_params);

   /* Get volume dimensions */
   get_volume_sizes(volume, sizes);

   /* Convert thresholds to voxel units */
   peak_params->threshold[MAXIMUM] = 
      convert_value_to_voxel(volume, peak_params->real_threshold[MAXIMUM]);
   peak_params->threshold[MINIMUM] = 
      convert_value_to_voxel(volume, peak_params->real_threshold[MINIMUM]);

   /* Set up array of offsets to find neighbours */
   noffsets = setup_neighbours(offsets);

   /* Allocate tag lists */
   for (iextreme = 0; iextreme < NEXTREMES; iextreme++) {
      ntags_increm[iextreme] = START_ALLOC;
      ntags_alloc[iextreme] = ntags_increm[iextreme];;
      ntags[iextreme] = 0;
      tags[iextreme] = MALLOC(sizeof(tags[0][0]) * ntags_alloc[iextreme]);
      if (tags[iextreme] == NULL) {
         (void) fprintf(stderr, "Memory allocation error\n");
         exit(EXIT_FAILURE);
      }
   }

   /* Set up looping arrays */
   for (idim=0; idim < VOXEL_NDIMS; idim++) {
      if (peak_params->include_edges) {
         start[idim] = 0;
         end[idim] = sizes[idim] - 1;
      }
      else {
         start[idim] = 1;
         end[idim] = sizes[idim] - 2;
      }
   }

   /* Set up progress report */
   initialize_progress_report(&progress, LogLevel <= LOW_LOGGING, 
                              end[SLC]-start[SLC]+1,
                              "Processing");

   /* Create but do not allocate the flag volume */
   flag_volume = copy_volume_definition_no_alloc(volume, NC_BYTE, FALSE, 
                                                 0.0, 255.0);
   set_volume_real_range(flag_volume, (Real) 0.0, (Real) 255.0);
   flags_alloced = FALSE;

   /* Look for peaks, looping over all voxels */
   for (index[SLC]=start[SLC]; index[SLC] <= end[SLC]; index[SLC]++) {
      for (index[ROW]=start[ROW]; index[ROW] <= end[ROW]; index[ROW]++) {
         for (index[COL]=start[COL]; index[COL] <= end[COL]; index[COL]++) {

            /* Check for peak and save the info if one is found */
            iextreme = find_peak_centroid(volume, flag_volume, &flags_alloced,
                                          index, sizes, noffsets, offsets, 
                                          peak_params, &value, centroid);

            /* Did we find a peak? */
            if (iextreme != NOT_A_PEAK) {

               /* Check whether we need to look for this type of extreme */
               if (peak_params->positive_only && iextreme == MINIMUM) continue;
               if (peak_params->negative_only && iextreme == MAXIMUM) continue;

               /* Check whether we need more space */
               if (ntags[iextreme] >= ntags_alloc[iextreme]) {
                  ntags_alloc[iextreme] += ntags_increm[iextreme];
                  tags[iextreme] = 
                     REALLOC(tags[iextreme], 
                             sizeof(tags[0][0]) * ntags_alloc[iextreme]);
                  if (tags[iextreme] == NULL) {
                     (void) fprintf(stderr, "Memory allocation error\n");
                     exit(EXIT_FAILURE);
                  }
                  ntags_increm[iextreme] *= ALLOC_FACTOR;
                  if (ntags_increm[iextreme] > MAX_ALLOC)
                     ntags_increm[iextreme] = MAX_ALLOC;
               }

               /* Put the tag on the list */
               convert_3D_voxel_to_world(volume, 
                                 centroid[SLC], centroid[ROW], centroid[COL],
                                 &x_world, &y_world, &z_world);
               tags[iextreme][ntags[iextreme]].coord[XW] = x_world;
               tags[iextreme][ntags[iextreme]].coord[YW] = y_world;
               tags[iextreme][ntags[iextreme]].coord[ZW] = z_world;
               tags[iextreme][ntags[iextreme]].value = 
                  convert_voxel_to_value(volume, value);
               tags[iextreme][ntags[iextreme]].valid = TRUE;

               ntags[iextreme]++;

            }         /* Endif peak */

         }         /* End of loop over voxels */
      }

      /* Print progress report */
      update_progress_report(&progress, index[SLC] - start[SLC] + 1);

   }

   /* Finish up progress report */
   terminate_progress_report(&progress);

   /* Sort the tags and remove any that are too close to a bigger one */
   for (iextreme=0; iextreme < NEXTREMES; iextreme++) {

      /* Check whether we need to look for this type of extreme */
      if (peak_params->positive_only && iextreme == MINIMUM) continue;
      if (peak_params->negative_only && iextreme == MAXIMUM) continue;

      /* Make sure that thee are some peaks */
      if (ntags[iextreme] <= 0) continue;

      /* Sort the peaks */
      qsort(tags[iextreme], (size_t) ntags[iextreme], sizeof(tags[0][0]), 
            (iextreme == MAXIMUM ? sort_descending : sort_ascending));

      /* Remove peaks that are too close to a bigger one. This code 
         depends on the peaks being sorted in descending order of 
         absolute value. */
      if (peak_params->minimum_distance > 0.0) {
         mindist2 = peak_params->minimum_distance;
         mindist2 = mindist2 * mindist2;

         /* First, loop over peaks, comparing smaller ones to this one.
            We do not need to test the smallest peak. */
         for (itag=0; itag < ntags[iextreme]-1; itag++) {

            /* Check for a valid peak */
            if (!tags[iextreme][itag].valid) continue;

            /* Loop over smaller peaks testing for proximity */
            for (jtag=itag+1; jtag < ntags[iextreme]; jtag++) {

               /* Check for a valid peak */
               if (!tags[iextreme][jtag].valid) continue;

               /* How far away is this peak? */
               distance2 = 0.0;
               for (iworld=0; iworld < WORLD_NDIMS; iworld++) {
                  diff = 
                     tags[iextreme][jtag].coord[iworld] -
                     tags[iextreme][itag].coord[iworld];
                  distance2 += diff * diff;
               }

               /* If it is too close then mark it invalid */
               if (distance2 < mindist2) {
                  tags[iextreme][jtag].valid = FALSE;
               }

            }    /* End of inner loop over tags */

         }    /* End of outer loop over tags */

         /* Then, remove the invalid peaks. 
            jtag is the old index, itag the new. */
         for (itag=jtag=0; jtag < ntags[iextreme]; jtag++) {

            /* Skip invalid peaks */
            if (!tags[iextreme][jtag].valid) continue;

            /* Copy valid peaks if the old and new indices differ */
            if (itag != jtag) {
               tags[iextreme][itag] = tags[iextreme][jtag];
            }

            /* Increment the new index */
            itag++;

         }

         /* Correct the number of peaks */
         ntags[iextreme] = itag;

      }    /* End of loop over extrema */

   }    /* Endif minimum_distance > 0 */

   /* Return the peaks */
   *nmin = ntags[MINIMUM];
   *nmax = ntags[MAXIMUM];
   *min_peaks = tags[MINIMUM];
   *max_peaks = tags[MAXIMUM];

   /* Print out peak_counters */
   if (LogLevel >= HIGH_LOGGING) {
      print_peak_counters(peak_params);
   }

}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : setup_neighbours
@INPUT      : (none)
@OUTPUT     : offsets - list of offsets to find neighbours
@RETURNS    : number of neighbours
@DESCRIPTION: Routine to set up a list of neighbours
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 13, 2000
@MODIFIED   : 
---------------------------------------------------------------------------- */
int setup_neighbours(int offsets[][VOXEL_NDIMS])
{
   int start[VOXEL_NDIMS], end[VOXEL_NDIMS], index[VOXEL_NDIMS];
   Voxel_Index idim;
   int noffsets;

   /* Set up array of offsets to find neighbours */
   for (idim=0; idim < VOXEL_NDIMS; idim++) {
      start[idim] = START_INDEX;
      end[idim] = END_INDEX;
   }
   noffsets = 0;
   for (index[SLC]=start[SLC]; index[SLC] <= end[SLC]; index[SLC]++) {
      for (index[ROW]=start[ROW]; index[ROW] <= end[ROW]; index[ROW]++) {
         for (index[COL]=start[COL]; index[COL] <= end[COL]; index[COL]++) {

            /* Exclude the point itself - we just want neighbours */
            if (!((index[SLC] == 0) && (index[ROW] == 0) &&
                  (index[COL] == 0))) {
               for (idim=0; idim < VOXEL_NDIMS; idim++) {
                  offsets[noffsets][idim] = index[idim];
               }
               noffsets++;
            }

         }
      }
   }

   return noffsets;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : check_for_peak
@INPUT      : volume
              index - volume index array
              noffsets - number of neighbours to check
              offsets - list of neighbour offsets
              peak_params - parameters describing peak search
              include_plateau - TRUE if a voxel should be reported as
                 a peak if it is equal to all of its neighbours.
@OUTPUT     : value - real value of this voxel if it has not been checked
                 before, otherwise value is not changed.
              num_equal_neighbours - number of equal neighbours.
              equal_neighbours - list of indices into offsets array
                 indicating which neighbours are equal to this one. 
                 Can be NULL if this info is not needed.
@RETURNS    : Type of peak: MAXIMUM, MINIMUM or NOT_A_PEAK.
@DESCRIPTION: Routine to check whether this voxel is a peak and return
              the type of peak.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 13, 2000
@MODIFIED   : 
---------------------------------------------------------------------------- */
Extreme_Index check_for_peak(Volume volume, int index[], int sizes[],
                             int noffsets, int offsets[][VOXEL_NDIMS],
                             PeakParams *peak_params, int include_plateau, 
                             Real *value, 
                             int *num_equal_neighbours, int *equal_neighbours)
{
   Extreme_Index extreme;
   Real voxel_min, voxel_max;
   Real diff, neighbour_value;
   Voxel_Index idim;
   int ioffset;
   int neighbour[VOXEL_NDIMS];
   int found_peak, outside;

   /* Make sure that we indicate that there are no neighbours */
   *num_equal_neighbours = 0;

   /* Get the volume valid voxel range */
   get_volume_voxel_range(volume, &voxel_min, &voxel_max);

   /* Get the current value */
   *value = get_volume_voxel_value(volume, index[SLC], index[ROW], index[COL], 
                                  0, 0);

   /* Check for an invalid value */
   if ((*value < voxel_min) || (*value > voxel_max)) {
      return NOT_A_PEAK;
   }

   /* Check whether we are out of range. Because we are checking both max
      and min, failing one test is okay, but forces a particular type
      of peak. This determines the first guess as to the type of peak. */
   extreme = NOT_A_PEAK;
   if (*value < peak_params->threshold[MAXIMUM]) 
      extreme = MINIMUM;
   if (*value > peak_params->threshold[MINIMUM]) {
      if (extreme == MINIMUM) return NOT_A_PEAK;
      extreme = MAXIMUM;
   }

   /* Check whether we are on the edge - this could happen if an edge voxel
      is equal to a non-edge peak voxel. By returning FALSE we force the
      exclusion of that peak. We do not have to test if this is the first
      voxel of a peak.
   */
   if (include_plateau && !peak_params->include_edges) {
      for (idim=0; idim < VOXEL_NDIMS; idim++) {
         if ((index[idim] == 0) || (index[idim] == sizes[idim]-1)) {
            return NOT_A_PEAK;
         }
      }
   }

   /* Look for a peak:
      If we are not including plateau points (voxel equal to all
      neighbours), then a peak is a voxel that is >= to all its
      neighbours and > at least one. Thus we start off assuming that
      we have not found a peak. If the voxel is < a neighbour, then it
      is not a peak and we stop checking, if it is greater then we
      flag it as a peak and if it is equal we don't change anything.
      If we are including plateau points, the we just start by
      assuming that we have a peak. As above, if the voxel is < a
      neighbour, then it is not a peak and we stop checking.
   */

   found_peak = include_plateau;
   for (ioffset=0; ioffset < noffsets; ioffset++) {

      /* Get the neighbour index */
      for (idim=0; idim < VOXEL_NDIMS; idim++) {
         neighbour[idim] = index[idim] + offsets[ioffset][idim];
      }

      /* Check whether we are outside the volume. This is only necessary
         if we are looking at plateau voxels or the edges are included. */
      if (peak_params->include_edges || include_plateau) {
         outside = FALSE;
         for (idim=0; idim < VOXEL_NDIMS; idim++) {
            if ((neighbour[idim] < 0) || 
                (neighbour[idim] >= sizes[idim])) {
               outside = TRUE;
               break;
            }
         }
         if (outside) continue;      /* Go to next offset */
      }

      /* Get the neighbour value */
      neighbour_value = 
         get_volume_voxel_value(volume, neighbour[SLC], neighbour[ROW],
                                neighbour[COL], 0, 0);

      /* Check for an invalid value */
      if ((neighbour_value < voxel_min) || (neighbour_value > voxel_max)) {
         if (!peak_params->include_edges) {
            return NOT_A_PEAK;
         }
         continue;           /* Go to next offset if we are including edges */
      }

      /* Test against this neighbour (do it with voxel values) */
      diff = *value - neighbour_value;
      if (diff < 0.0) {
         switch (extreme) {
            case NOT_A_PEAK: extreme = MINIMUM;   /* Fall through */
            case MINIMUM: found_peak = TRUE; break;
            case MAXIMUM: return NOT_A_PEAK;
         }
      }
      else if (diff > 0.0) {
         switch (extreme) {
            case NOT_A_PEAK: extreme = MAXIMUM;   /* Fall through */
            case MAXIMUM: found_peak = TRUE; break;
            case MINIMUM: return NOT_A_PEAK;
         }
      }
      else {
         if (equal_neighbours != NULL)
            equal_neighbours[*num_equal_neighbours] = ioffset;
         (*num_equal_neighbours)++;
      }

   }        /* End of loop over neighbours */

   /* extreme may be set to indicate type of peak, so test whether a 
      peak was really found */
   if (!found_peak) extreme = NOT_A_PEAK;

   return extreme;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : find_peak_centroid
@INPUT      : volume
              flag_volume - flags indicating whether voxels have been checked
              flags_alloced - indicates whether flag volume has been allocated
              index - volume index array
              noffsets - number of neighbours to check
              offsets - list of neighbour offsets
              peak_params - parameters describing peak search
@OUTPUT     : value - volume value at peak
              centroid - voxel coordinate of centroid of peak
@RETURNS    : Type of peak: MAXIMUM, MINIMUM or NOT_A_PEAK.
@DESCRIPTION: Routine to find the centroid of a peak plateau. After the
              first voxel, we know that all voxels checked have the same
              value, since they are in the equal_neighbour list, so if they 
              are reported as not being peaks, that means that this peak
              is not one either (we also make sure that they have not been 
              previously checked). Even if we invalidate this peak, we 
              continue to search in order to mark all voxels in it.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 15, 2000
@MODIFIED   : 
--------------------------------------------------------------------------- */
Extreme_Index find_peak_centroid(Volume volume, 
                                 Volume flag_volume, int *flags_alloced,
                                 int index[], int sizes[],
                                 int noffsets, int offsets[][VOXEL_NDIMS],
                                 PeakParams *peak_params, 
                                 Real *value, Real centroid[])
{
   Extreme_Index extreme, this_extreme;
   int this_index[VOXEL_NDIMS];
   int equal_neighbours[NUM_OFFSETS];
   int num_equal_neighbours;
   int new_index[VOXEL_NDIMS];
   Voxel_Index idim;
   int ioffset;
   Real this_value;
   int num_centroid_voxels;
   int have_a_peak;
   int first_voxel;
   int is_a_plateau;

   /* We re-use the voxel_queue */
   static VoxelQueue voxel_queue = NULL;

   /* First check whether we need to look at this voxel */
   if (*flags_alloced) {
      if (check_and_mark_flag(flag_volume, index))
         return NOT_A_PEAK;
   }

   /* Check the first point */
   extreme = 
      check_for_peak(volume, index, sizes,
                     noffsets, offsets, peak_params, FALSE, value,
                     &num_equal_neighbours, equal_neighbours);
   have_a_peak = (extreme != NOT_A_PEAK);

   /* Return if there is no peak */
   if (!have_a_peak) {
      return NOT_A_PEAK;
   }

   /* Or if there is a peak but no plateau. Remember to get centroid and
      increment counters. */
   else if (num_equal_neighbours == 0) {
      for (idim=0; idim < VOXEL_NDIMS; idim++) {
         centroid[idim] = (Real) index[idim];
      }
      is_a_plateau = FALSE;
      increment_peak_counter(peak_params, have_a_peak, is_a_plateau);
      if (LogLevel >= HIGH_LOGGING) {
         log_peak_info(volume, 
                       (have_a_peak ? "Point" : "False plateau"),
                       *value, centroid, -1);
      }
      return extreme;
   }

   /* Otherwise we have a plateau peak. */
   is_a_plateau = TRUE;

   /* Allocate flag volume if it is needed. */
   if (!*flags_alloced) {
      allocate_flag_volume(flag_volume, index);
      *flags_alloced = TRUE;
   }

   /* Set up voxel coordinate queue and initialize centroid calculation */
   reset_vqueue(&voxel_queue);
   num_centroid_voxels = 0;
   for (idim=0; idim < VOXEL_NDIMS; idim++) {
      this_index[idim] = index[idim];
      centroid[idim] = 0.0;
   }

   /* Set flag to indicate first voxel */
   first_voxel = TRUE;

   /* Loop over queue entries until queue is empty */
   while (TRUE) {

      /* Check next peak, unless this is the first time through */
      if (first_voxel) {
         first_voxel = FALSE;
         this_extreme = extreme;
      }
      else if (!remove_vqueue_head(voxel_queue, this_index)) {
         break;
      }
      else {
         this_extreme = 
            check_for_peak(volume, this_index, sizes,
                           noffsets, offsets, peak_params, TRUE, &this_value,
                           &num_equal_neighbours, equal_neighbours);
      }

      /* Add to centroid counters */
      for (idim=0; idim < VOXEL_NDIMS; idim++)
         centroid[idim] += this_index[idim];
      num_centroid_voxels++;

      if (this_extreme != extreme)
         have_a_peak = FALSE;

      /* Loop over the list of equal neighbours */
      for (ioffset=0; ioffset < num_equal_neighbours; ioffset++) {

         /* Get the neighbour index */
         for (idim=0; idim < VOXEL_NDIMS; idim++) {
            new_index[idim] = this_index[idim] + 
               offsets[equal_neighbours[ioffset]][idim];
         }

         /* Add the index to the queue if it has not previously 
            been checked. We mark the voxel as checked as soon
            as it goes on the queue. */
         if (!check_and_mark_flag(flag_volume, new_index)) {
            add_vqueue_tail(voxel_queue, new_index);
         }
      }

   }         /* Loop over queue */

   /* Calculate centroid */
   if (num_centroid_voxels > 0) {
      for (idim=0; idim < VOXEL_NDIMS; idim++) {
         centroid[idim] /= (Real) num_centroid_voxels;
      }
   }

   /* Increment counters */
   increment_peak_counter(peak_params, have_a_peak, is_a_plateau);

   if (LogLevel >= HIGH_LOGGING) {
      log_peak_info(volume, (have_a_peak ? "True plateau" : "False plateau"),
                    *value, centroid, num_centroid_voxels);
   }

   /* Return the extreme */
   if (!have_a_peak) extreme = NOT_A_PEAK;
   return extreme;
}
                               
/* ----------------------------- MNI Header -----------------------------------
@NAME       : check_and_mark_flag
@INPUT      : flag_volume
              index - index to check
@OUTPUT     : flag_volume
@RETURNS    : TRUE if the flag was previously set.
@DESCRIPTION: Function to check whether a flag is set for a given
              voxel. If not, the flag is set.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 19, 2000
@MODIFIED   : 
--------------------------------------------------------------------------- */
int check_and_mark_flag(Volume flag_volume, int *index)
{
   int flag;

   flag = (int) get_volume_voxel_value(flag_volume, 
                                       index[SLC], 
                                       index[ROW], 
                                       index[COL], 
                                       0, 0);

   /* Check whether the flag is set */
   if (flag) return TRUE;

   /* Mark the voxel as checked */
   flag |= CHECKED_FLAG;
   set_volume_voxel_value(flag_volume, 
                          index[SLC], 
                          index[ROW], 
                          index[COL], 
                          0, 0, (Real) flag);

   /* If we are here, then the flag was not set */
   return FALSE;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : allocate_flag_volume
@INPUT      : current - index of the current voxel. 
                 All voxels before this are initialized.
@OUTPUT     : flag_volume
@RETURNS    : (nothing)
@DESCRIPTION: Routine to allocate and initialize the flag volume.
              Flags must be set before this voxel (voxels are scanned
              slice-by-slice), and this voxel's flag must be set according
              to max and min search order (using checked_flag).
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 15, 2000
@MODIFIED   : 
--------------------------------------------------------------------------- */
void allocate_flag_volume(Volume flag_volume, int *current)
{
   Real *flagbuf;
   Real flag;
   int sizes[VOXEL_NDIMS];
   int index[VOXEL_NDIMS];
   int ivox;

   /* Print log message */
   if (LogLevel >= HIGH_LOGGING) {
      print("\nSetting up flag volume for plateau searches\n");
   }

   /* Allocate the volume */
   alloc_volume_data(flag_volume);

   /* Get volume dimensions */
   get_volume_sizes(flag_volume, sizes);

   /* Get a buffer for each line and initialize it */
   flagbuf = malloc(sizeof(*flagbuf) * sizes[COL]);
   flag = (Real) CHECKED_FLAG;
   for (ivox=0; ivox < sizes[COL]; ivox++) 
      flagbuf[ivox] = flag;

   /* Loop over rows up to this column */
   for (index[SLC]=0; index[SLC] <= current[SLC]; index[SLC]++) {
      for (index[ROW]=0; index[ROW] < sizes[ROW]; index[ROW]++) {
         if (index[SLC] == current[SLC] && 
             index[ROW] == current[ROW]) {
            break;
         }
         set_volume_voxel_hyperslab(flag_volume,
                                    index[SLC], index[ROW], 0, 0, 0,
                                    1, 1, sizes[COL], 1, 1, flagbuf);
      }
   }

   /* Set this row. The flag changes to zero after the current voxel,
      since it is being checked. */
   index[SLC] = current[SLC];
   index[ROW] = current[ROW];
   for (index[COL]=0; index[COL] < sizes[COL]; index[COL]++) {
      set_volume_voxel_value(flag_volume, index[SLC], index[ROW], index[COL],
                             0, 0, flag);
      if (index[COL] == current[COL])
         flag = 0.0;
   }

   /* Change the buffer to the new flag (0) */
   for (ivox=0; ivox < sizes[COL]; ivox++) 
      flagbuf[ivox] = flag;

   /* Set remaining rows */
   for (index[SLC]=current[SLC]; index[SLC] < sizes[SLC]; index[SLC]++) {
      for (index[ROW]=0; index[ROW] < sizes[ROW]; index[ROW]++) {
         if (index[SLC] == current[SLC] && 
             index[ROW] <= current[ROW]) {
            continue;
         }
         set_volume_voxel_hyperslab(flag_volume,
                                    index[SLC], index[ROW], 0, 0, 0,
                                    1, 1, sizes[COL], 1, 1, flagbuf);
      }
   }

   /* Free buffer */
   free(flagbuf);

   /* Print log message */
   if (LogLevel >= HIGH_LOGGING) {
      print("Done setting up flag volume\n");
   }

}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : reset_vqueue
@INPUT      : (none)
@OUTPUT     : voxel_queue - pointer to queue to be reset. If the queue
                 pointed to is NULL, then allocate a new queue.
@RETURNS    : (nothing)
@DESCRIPTION: Routine to create or reset a voxel queue.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 15, 2000
@MODIFIED   : 
--------------------------------------------------------------------------- */
void reset_vqueue(VoxelQueue *voxel_queue)
{
   VoxelQueue queue;

   /* Allocate a new queue if it is needed. The queue starts off as a ring
      with one element. */
   if (*voxel_queue == NULL) {
      queue = malloc(sizeof(*queue));
      if (queue == NULL) {
         (void) fprintf(stderr, "Memory allocation error\n");
         exit(EXIT_FAILURE);
      }
      queue->head = malloc(sizeof(*(queue->head)));
      if (queue->head == NULL) {
         (void) fprintf(stderr, "Memory allocation error\n");
         exit(EXIT_FAILURE);
      }
      queue->head->next = queue->head;
      *voxel_queue = queue;
#ifdef VQUEUE_DEBUG
      queue->num_ring_blocks = 1;
#endif
   }
   else {
      queue = *voxel_queue;
   }

   /* Reset the queue */
   queue->tail = queue->head;
   queue->head_index = 0;
   queue->tail_index = 0;

#ifdef VQUEUE_DEBUG
   /* Accounting info */
   queue->size = 0;
   queue->max_size = 0;
#endif

}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : delete_vqueue
@INPUT      : queue
@OUTPUT     : (none)
@RETURNS    : (nothing)
@DESCRIPTION: Routine to create or reset a voxel queue.
@METHOD     : This is not actually used, since the queue ring is kept between
              uses, but it is here for completeness.
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 15, 2000
@MODIFIED   : 
--------------------------------------------------------------------------- */
void delete_vqueue(VoxelQueue queue)
{
   VQueueBlock this, next;

   if (queue == NULL) return;

   this = queue->head;
   do {

      next = this->next;
      free(this);
      this = next;

   } while (this != queue->head);

   free(queue);

}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : add_vqueue_tail
@INPUT      : queue
              coord - coordinate to add to tail of queue
@OUTPUT     : (none)
@RETURNS    : (nothing)
@DESCRIPTION: Routine to push a coordinate onto the tail of the queue.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 15, 2000
@MODIFIED   : 
--------------------------------------------------------------------------- */
void add_vqueue_tail(VoxelQueue queue, int coord[])
{
   Voxel_Index idim;

   /* Check whether there is room on the last block. If not, go to the next
      block unless it is the head. In that case add a new block to the ring. */
   if (queue->tail_index >= VQUEUE_BLOCK_LENGTH) {

      /* Add a new block if we are at the end of the ring */
      if (queue->tail->next == queue->head) {

         queue->tail->next = malloc(sizeof(*(queue->head)));
         if (queue->tail->next == NULL) {
            (void) fprintf(stderr, "Memory allocation error\n");
            exit(EXIT_FAILURE);
         }
         queue->tail->next->next = queue->head;
#ifdef VQUEUE_DEBUG
         queue->num_ring_blocks++;
#endif

      }

      /* Move to the next block */
      queue->tail = queue->tail->next;
      queue->tail_index = 0;
      
   }

   /* Put the index in the queue */
   for (idim=0; idim < VOXEL_NDIMS; idim++) {
      queue->tail->coord[queue->tail_index][idim] = coord[idim];
   }
   queue->tail_index++;

#ifdef VQUEUE_DEBUG
   queue->size++;
   if (queue->size > queue->max_size) queue->max_size = queue->size;
#endif
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : remove_vqueue_head
@INPUT      : queue
@OUTPUT     : coord - coordinate from head of queue
@RETURNS    : TRUE if an entry was found, FALSE otherwise.
@DESCRIPTION: Routine to pull a coordinate off of the head of a queue.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 15, 2000
@MODIFIED   : 
--------------------------------------------------------------------------- */
int remove_vqueue_head(VoxelQueue queue, int coord[])
{
   Voxel_Index idim;

   /* Check if we need to wrap the head_index */
   if ((queue->head != queue->tail) && 
       (queue->head_index >= VQUEUE_BLOCK_LENGTH)) {
      queue->head = queue->head->next;
      queue->head_index = 0;
   }

   /* Check if anything is on the queue */
   if ((queue->head == queue->tail) && 
       (queue->head_index >= queue->tail_index))
      return FALSE;


   /* Get the coordinate index */
   for (idim=0; idim < VOXEL_NDIMS; idim++) {
      coord[idim] = queue->head->coord[queue->head_index][idim];
   }

   /* Move to the next entry */
   queue->head_index++;

#ifdef VQUEUE_DEBUG
   queue->size--;
#endif

   return TRUE;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : sort_ascending
@INPUT      : val1, val2 - values to compare for qsort
@OUTPUT     : (none)
@RETURNS    : -1, 0 or 1 for qsort
@DESCRIPTION: Routine to sort tags in ascending order
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : February 2, 1999
@MODIFIED   : 
---------------------------------------------------------------------------- */
int sort_ascending(const void *val1, const void *val2)
{
   Tag *tag1, *tag2;

   tag1 = (Tag *) val1;
   tag2 = (Tag *) val2;
   if (tag1->value > tag2->value)
      return 1;
   else if (tag1->value < tag2->value)
      return -1;
   else
      return 0;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : sort_descending
@INPUT      : val1, val2 - values to compare for qsort
@OUTPUT     : (none)
@RETURNS    : -1, 0 or 1 for qsort
@DESCRIPTION: Routine to sort tags in descending order
@METHOD     : 
@GLOBALS    : 
@CALLS      : sort_ascending
@CREATED    : February 2, 1999
@MODIFIED   : 
---------------------------------------------------------------------------- */
int sort_descending(const void *val1, const void *val2)
{
   return -sort_ascending(val1, val2);
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : do_not_print
@INPUT      : string
@OUTPUT     : (none)
@RETURNS    : (nothing)
@DESCRIPTION: Routine to throw away output
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 18, 2000
@MODIFIED   : 
---------------------------------------------------------------------------- */
void do_not_print(char *string)
/* ARGSUSED */
{
   return;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : reset_peak_counters
@INPUT      : peak_params
@OUTPUT     : (none)
@RETURNS    : (nothing)
@DESCRIPTION: Reset counters for peak types
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 18, 2000
@MODIFIED   : 
---------------------------------------------------------------------------- */
void reset_peak_counters(PeakParams *peak_params)
{
   peak_params->true_plateaus = 0;
   peak_params->false_plateaus = 0;
   peak_params->true_points = 0;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : increment_peak_counter
@INPUT      : peak_params
              have_a_peak - flag to indicate a peak
              is_a_plateau - flag to indicate a peak plateau
@OUTPUT     : (none)
@RETURNS    : (nothing)
@DESCRIPTION: Increments the appropriate peak counter
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 18, 2000
@MODIFIED   : 
---------------------------------------------------------------------------- */
void increment_peak_counter(PeakParams *peak_params, 
                            int have_a_peak, int is_a_plateau)
{
   if (is_a_plateau) {
      if (have_a_peak)
         peak_params->true_plateaus++;
      else
         peak_params->false_plateaus++;
   }
   else if (have_a_peak)
      peak_params->true_points++;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : print_peak_counters
@INPUT      : peak_params
@OUTPUT     : (none)
@RETURNS    : (nothing)
@DESCRIPTION: Prints out peaks counters
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 18, 2000
@MODIFIED   : 
---------------------------------------------------------------------------- */
void print_peak_counters(PeakParams *peak_params)
{
   print("\n");
   print("Number of point peaks         = %d\n", peak_params->true_points);
   print("Number of true plateau peaks  = %d\n", peak_params->true_plateaus);
   print("Number of false plateau peaks = %d\n", peak_params->false_plateaus);
   print("\n");
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : log_peak_info
@INPUT      : volume
              type
              value
              centroid 
              num_centroid_voxels - if < 0, then do not print this
@OUTPUT     : (none)
@RETURNS    : (nothing)
@DESCRIPTION: Prints out peak information
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : October 18, 2000
@MODIFIED   : 
---------------------------------------------------------------------------- */
void log_peak_info(Volume volume, char *type, Real value, Real centroid[],
                   int num_centroid_voxels)
{
   Real real_value;
   Real x_world, y_world, z_world;

   real_value = convert_voxel_to_value(volume, value);
   convert_3D_voxel_to_world(volume, 
                             centroid[SLC], centroid[ROW], centroid[COL],
                             &x_world, &y_world, &z_world);
   print("\n%s peak: (%.6g, %.6g, %.6g), value = %.6g",
         type, x_world, y_world, z_world, real_value);
   if (num_centroid_voxels >= 0) {
      print(", %d voxels", num_centroid_voxels);
   }
   print("\n");

}

