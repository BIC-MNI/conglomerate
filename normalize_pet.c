/* ----------------------------- MNI Header -----------------------------------
@NAME       : normalize_pet.c
@INPUT      : argc, argv - command line arguments
@OUTPUT     : (nothing)
@RETURNS    : status
@DESCRIPTION: Program to normalize a volume to a standard intensity.
@CREATED    : October 27, 1993 (Peter Neelin)
@MODIFIED   : $Log: normalize_pet.c,v $
@MODIFIED   : Revision 1.1  2005/03/01 17:21:54  bert
@MODIFIED   : Added normalize_pet to the conglomerate
@MODIFIED   :
 * Revision 1.9  1998/06/25  16:23:57  neelin
 * Added check for error on writing output volume.
 *
 * Revision 1.8  1996/05/06  11:57:59  neelin
 * Changed name to normalize_pet/
 *
 * Revision 1.7  1995/04/21  13:23:12  neelin
 * Added options to use mask file and normalize to a value other than 100.
 *
 * Revision 1.6  1995/04/11  12:36:50  neelin
 * *** empty log message ***
 *
 * Revision 1.5  93/10/29  15:31:40  neelin
 * Delete norm volume before loading input volume (so that type and range
 * don't get changed).
 * 
 * Revision 1.4  93/10/29  10:44:24  neelin
 * Pass options to input_volume.
 * 
 * Revision 1.3  93/10/28  16:53:00  neelin
 * Removed extra GET_VOXEL_3D call.
 * 
 * Revision 1.2  93/10/28  16:12:14  neelin
 * Modified usage line.
 * 
 * Revision 1.1  93/10/28  15:21:14  neelin
 * Initial revision
 * 
---------------------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <volume_io.h>
#include <time_stamp.h>
#include <ParseArgv.h>

/* Constants */
#define RANGE_EPSILON (10.0 * FLT_EPSILON)

/* Function prototypes */
int match_volume_dimensions(Volume vol1, Volume vol2);
int match_reals(Real value1, Real value2);

/* Variables for arguments */
char *maskfile = NULL;
double normalized_mean = 100.0;

/* Argument table */
ArgvInfo argTable[] = {
   {"-mask", ARGV_STRING, (char *) 1, (char *) &maskfile,
       "Use the specified file as a mask for normalization."},
   {"-normalized_mean", ARGV_FLOAT, (char *) 1, (char *) &normalized_mean,
       "Specify a target value for the normalized mean."},
   {NULL, ARGV_END, NULL, NULL, NULL}
};

int main(int argc, char *argv[])
{
   char *normfile, *infile, *outfile;
   Volume volume;
   Volume mask;
   int sizes[3];
   int ix, iy, iz;
   Real sum1, sumv, value, mean, threshold, norm_factor, mask_value;
   Real maximum, minimum, mask_min, mask_max;
   char *history, *pname;
   minc_input_options options;
   int need_threshold;

   /* Check arguments */
   history = time_stamp(argc, argv);
   pname=argv[0];
   if (ParseArgv(&argc, argv, argTable, 0)) {
      (void) fprintf(stderr, 
         "\nUsage: %s [<options>] <norm.mnc> [<in.mnc>] <out.mnc>\n",
                     pname);
      return EXIT_FAILURE;
   }
   if (argc == 3) {
      normfile = argv[1];
      infile = NULL;
      outfile = argv[2];
   }
   else if (argc == 4) {
      normfile = argv[1];
      infile = argv[2];
      outfile = argv[3];
   }
   else {
      (void) fprintf(stderr, 
         "\nUsage: %s [<options>] <norm.mnc> [<in.mnc>] <out.mnc>\n",
                     pname);
      return EXIT_FAILURE;
   }
   need_threshold = (maskfile == NULL);

   /* Set input options */
   set_default_minc_input_options(&options);
   set_minc_input_promote_invalid_to_min_flag(&options, FALSE);
   set_minc_input_vector_to_scalar_flag(&options, FALSE);

   /* Read in the normalization file */
   if (input_volume(normfile, 3, NULL, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                    TRUE, &volume, &options) != OK) {
      (void) fprintf(stderr, "Error loading volume %s\n", normfile);
      return EXIT_FAILURE;
   }
   get_volume_real_range(volume, &minimum, &maximum);
   minimum -= ABS(minimum) * RANGE_EPSILON;
   maximum += ABS(maximum) * RANGE_EPSILON;
   get_volume_sizes(volume, sizes);

   /* Read in the mask file and check that the dimensions match */
   if (maskfile != NULL) {
      if (input_volume(maskfile, 3, NULL, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                       TRUE, &mask, &options) != OK) {
         (void) fprintf(stderr, "Error loading volume %s\n", maskfile);
         return EXIT_FAILURE;
      }
      if (!match_volume_dimensions(volume, mask)) {
         (void) fprintf(stderr, 
            "Mask volume dimensions do not match normalization volume.\n");
         return EXIT_FAILURE;
      }
      get_volume_real_range(mask, &mask_min, &mask_max);
      mask_min -= ABS(mask_min) * RANGE_EPSILON;
      mask_max += ABS(mask_max) * RANGE_EPSILON;
   }

   /* Calculate the threshold if needed */
   if (need_threshold) {
      sum1 = 0.0;
      sumv = 0.0;
      for (iz=0; iz<sizes[0]; iz++) {
         for (iy=0; iy<sizes[1]; iy++) {
            for (ix=0; ix<sizes[2]; ix++) {
               GET_VALUE_3D(value, volume, iz, iy, ix);
               if ((value >= minimum) && (value <= maximum)) {
                  sum1 += 1.0;
                  sumv += value;
               }
            }
         }
      }
      if (sum1 > 0.0)
         mean = sumv / sum1;
      else 
         mean = 0.0;
      threshold = 1.5 * mean;
   }
   else {
      threshold = 0.0;
   }

   /* Calculate the normalization factor */
   sum1 = 0.0;
   sumv = 0.0;
   mask_value = 1.0;
   for (iz=0; iz<sizes[0]; iz++) {
      for (iy=0; iy<sizes[1]; iy++) {
         for (ix=0; ix<sizes[2]; ix++) {
            GET_VALUE_3D(value, volume, iz, iy, ix);
            if (maskfile != NULL) {
               GET_VALUE_3D(mask_value, mask, iz, iy, ix);
               if ((mask_value < mask_min) || (mask_value > mask_max))
                  mask_value = 0.0;
            }
            if ((!need_threshold || (value > threshold)) &&
                (value >= minimum) && (value <= maximum)) {
               sum1 += mask_value;
               sumv += value * mask_value;
            }
         }
      }
   }
   if (sum1 > 0.0)
      mean = sumv / sum1;
   else 
      mean = 0.0;
   if (mean <= 0.0) {
      (void) fprintf(stderr, "Thresholded mean <= 0.0\n");
      return EXIT_FAILURE;
   }
   norm_factor = normalized_mean / mean;

   /* Get rid of the mask volume */
   if (maskfile != NULL) {
      delete_volume(mask);
   }

   /* Write out the numbers */
   (void) printf("File        = %s\n", normfile);
   (void) printf("Threshold   = %.20g\n", (double) threshold);
   (void) printf("Mean        = %.20g\n", (double) mean);
   (void) printf("Norm factor = %.20g\n", (double) norm_factor);

   /* Load file to normalize if needed */
   if (infile != NULL) {
      delete_volume(volume);
      if (input_volume(infile, 3, NULL, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                       TRUE, &volume, &options) != OK) {
         (void) fprintf(stderr, "Error loading volume %s\n", infile);
         return EXIT_FAILURE;
      }
      get_volume_sizes(volume, sizes);
   }
   else {
      infile = normfile;
   }

   /* Normalize volume */
   get_volume_real_range(volume, &minimum, &maximum);
   minimum *= norm_factor;
   maximum *= norm_factor;
   set_volume_real_range(volume, minimum, maximum);
   if ((volume->nc_data_type == NC_FLOAT) ||
       (volume->nc_data_type == NC_DOUBLE)) {
      for (iz=0; iz<sizes[0]; iz++) {
         for (iy=0; iy<sizes[1]; iy++) {
            for (ix=0; ix<sizes[2]; ix++) {
               GET_VOXEL_3D(value, volume, iz, iy, ix);
               value *= norm_factor;
               SET_VOXEL_3D(volume, iz, iy, ix, value);
            }
         }
      }
   }

   /* Save the normalized volume */
   if (output_modified_volume(outfile, NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                              volume, infile, history, NULL) != OK) {
      (void) fprintf(stderr, "Error writing output volume.\n");
      return EXIT_FAILURE;
   }

   return EXIT_SUCCESS;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : match_volume_dimensions
@INPUT      : vol1 - volume 1
              vol2 - volume 2
@OUTPUT     : (nothing)
@RETURNS    : TRUE if dimensions match, FALSE if not
@DESCRIPTION: Routine to compare the dimensions of two volumes to ensure that
              they correspond
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : April 21, 1995 (Peter Neelin)
@MODIFIED   : 
---------------------------------------------------------------------------- */
int match_volume_dimensions(Volume vol1, Volume vol2)
{
   int sizes1[5], sizes2[5];
   int ndims, idim;
   char **names1, **names2;
   Real sep1[5], sep2[5];
   Real voxel[5];
   Real x1, y1, z1, x2, y2, z2;

   /* Check dimensionality */
   ndims = get_volume_n_dimensions(vol1);
   if (get_volume_n_dimensions(vol2) != ndims)
      return FALSE;

   /* Check the dimension names, sizes and separations */
   names1 = get_volume_dimension_names(vol1);
   names2 = get_volume_dimension_names(vol2);
   get_volume_sizes(vol1, sizes1);
   get_volume_sizes(vol2, sizes2);
   get_volume_separations(vol1, sep1);
   get_volume_separations(vol2, sep2);
   for (idim=0; idim < ndims; idim++) {
      if (strcmp(names1[idim], names2[idim]) != 0)
         return FALSE;
      if (sizes1[idim] != sizes2[idim])
         return FALSE;
      if (!match_reals(sep1[idim], sep2[idim]))
         return FALSE;
   }

   /* Check the axes */
   for (idim=0; idim < ndims; idim++)
      voxel[idim] = 0.0;
   for (idim=-1; idim < ndims; idim++) {
      if (idim >= 0)
         voxel[idim] = sizes1[idim] - 1;
      convert_voxel_to_world(vol1, voxel, &x1, &y1, &z1);
      convert_voxel_to_world(vol2, voxel, &x2, &y2, &z2);
      if (!(match_reals(x1, x2) || match_reals(y1, y2) ||
            match_reals(z1, z2))) {
         return FALSE;
      }
      if (idim >= 0)
         voxel[idim] = 0.0;
   }

   return TRUE;
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : match_reals
@INPUT      : value1 - real value 1
              value2 - real value 2
@OUTPUT     : (nothing)
@RETURNS    : TRUE if values match, FALSE if not
@DESCRIPTION: Routine to compare two Real values, allowing a tolerance.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : April 21, 1995 (Peter Neelin)
@MODIFIED   : 
---------------------------------------------------------------------------- */
int match_reals(Real value1, Real value2)
{
#define TOLERANCE ( 100.0 * FLT_EPSILON )

   Real diff, denom;

   diff = value1 - value2;
   denom = (value1 + value2) / 2.0;
   if (denom != 0.0) diff /= denom;
   diff = ABS(diff);
   if (diff < TOLERANCE)
      return TRUE;
   else
      return FALSE;
}
