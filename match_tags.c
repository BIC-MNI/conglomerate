/* ----------------------------- MNI Header -----------------------------------
@NAME       : match-tags
@INPUT      : argc, argv - command line arguments
@OUTPUT     : (none)
@RETURNS    : status
@DESCRIPTION: Program to match tags in two tag files.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : November 8, 2001 (Peter Neelin)
@MODIFIED   : $Log: match_tags.c,v $
@MODIFIED   : Revision 1.1  2004-04-07 15:53:20  bert
@MODIFIED   : Initial checkin, from Peter's directory
@MODIFIED   :
 * Revision 1.1  2001/11/08  18:11:59  neelin
 * Initial revision
 *
---------------------------------------------------------------------------- */

#ifndef lint
static char rcsid[]="$Header: /private-cvsroot/libraries/conglomerate/match_tags.c,v 1.1 2004-04-07 15:53:20 bert Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <time_stamp.h>
#include <volume_io.h>
#include <ParseArgv.h>

/* Constants */
#ifndef TRUE
#  define TRUE 1
#  define FALSE 0
#endif
#ifndef public
#  define public
#  define private static
#endif

#define WORLD_NDIMS 3

#define SEP_STRING " : "

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

/* Argument variables */
double maximum_distance = FLT_MAX;

/* Argument table */
ArgvInfo argTable[] = {
   {"-max_distance", ARGV_FLOAT, (char *) 1, (char *) &maximum_distance,
    "Maximum distance for two tags to be considered close"},

   {NULL, ARGV_END, NULL, NULL, NULL}
};

/* Main program */

int main(int argc, char *argv[])
{
   char *pname, *tagfile1, *tagfile2, *outtag, *history;
   Real **tags1, **tags2, **new_tags, *this_tag;
   int n_volumes1, n_volumes2, n_tag_points1, n_tag_points2;
   int ipoint1, ipoint2, icoord, near_tag;
   STRING *labels1, *labels2, *new_labels, this_label;
   double min_distsq, distsq, diff, maximum_distsq;
   size_t string_length;
   FILE *fp;
   char dist_string[64];

   /* Save history */
   history = time_stamp(argc, argv);

   /* Parse arguments */
   pname = argv[0];
   if (ParseArgv(&argc, argv, argTable, 0) || (argc != 4)) {
      (void) fprintf(stderr, 
                     "\nUsage: %s [<options>] infile1.tag file2.tag outfile2.tag\n\n",
                     pname);
      exit(EXIT_FAILURE);
   }
   tagfile1 = argv[1];
   tagfile2 = argv[2];
   outtag = argv[3];
   maximum_distsq = maximum_distance * maximum_distance;

   /* Read in first tag file */
   if ((open_file_with_default_suffix(tagfile1,
                  get_default_tag_file_suffix(),
                  READ_FILE, ASCII_FORMAT, &fp) != OK) ||
       (input_tag_points(fp, &n_volumes1, &n_tag_points1, 
                         &tags1, NULL, 
                         NULL, NULL, NULL, &labels1) != OK)) {
      (void) fprintf(stderr, "%s: Error reading tag file %s\n", 
                     pname, tagfile1);
      exit(EXIT_FAILURE);
   }
   (void) close_file(fp);


   /* Read in second tag file */
   if ((open_file_with_default_suffix(tagfile2,
                  get_default_tag_file_suffix(),
                  READ_FILE, ASCII_FORMAT, &fp) != OK) ||
       (input_tag_points(fp, &n_volumes2, &n_tag_points2, 
                         &tags2, NULL, 
                         NULL, NULL, NULL, &labels2) != OK)) {
      (void) fprintf(stderr, "%s: Error reading tag file %s\n", 
                     pname, tagfile2);
      exit(EXIT_FAILURE);
   }
   (void) close_file(fp);

   /* Allocate space for output tags */
   new_tags = MALLOC(n_tag_points1 * sizeof(*new_tags));
   new_labels = MALLOC(n_tag_points1 * sizeof(*new_labels));

   /* Loop through tags in first file */
   for (ipoint1=0; ipoint1 < n_tag_points1; ipoint1++) {

      /* Look for nearest tag in second file */
      min_distsq = 0.0;
      for (ipoint2=0; ipoint2 < n_tag_points2; ipoint2++) {

         /* Calculate distance-squared */
         distsq = 0;
         for (icoord=0; icoord < WORLD_NDIMS; icoord++) {
            diff = tags1[ipoint1][icoord] - tags2[ipoint2][icoord];
            distsq += diff * diff;
         }

         /* Check if this is the minimum */
         if ((ipoint2 == 0) || (min_distsq > distsq)) {
            near_tag = ipoint2;
            min_distsq = distsq;
         }

      }

      /* Save the nearest tag */
      if (min_distsq <= maximum_distsq) {
         this_tag = tags2[near_tag];
         this_label = labels2[near_tag];
      }
      else {
         this_tag = tags1[ipoint1];
         this_label = "not found";
      }
      (void) sprintf(dist_string, " (%.2f mm)", sqrt(min_distsq));
      new_tags[ipoint1] = this_tag;
      string_length = strlen(labels1[ipoint1]) + strlen(this_label) + 
         strlen(SEP_STRING) + strlen(dist_string) + 1;
      new_labels[ipoint1] = MALLOC(string_length);
      (void) strcpy(new_labels[ipoint1], labels1[ipoint1]);
      (void) strcat(new_labels[ipoint1], SEP_STRING);
      (void) strcat(new_labels[ipoint1], this_label);
      (void) strcat(new_labels[ipoint1], dist_string);

   }

   if (output_tag_file(outtag, history, 2, n_tag_points1, 
                       tags1, new_tags, 
                       NULL, NULL, NULL, new_labels) != OK) {
      (void) fprintf(stderr, "Error writing out labels\n");
      return EXIT_FAILURE;
   }
   return EXIT_SUCCESS;
}
