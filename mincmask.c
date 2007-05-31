 /* ----------------------------- MNI Header -----------------------------------
@NAME       :  mincmask.c 
@INPUT      : 
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: read in a mnc volume, and mask specified regions.
              
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Thu Jul  8 15:01:05 EST 1993 Louis Collins
@MODIFIED   : 
---------------------------------------------------------------------------- */

				/* include list */
#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <limits.h>

#if HAVE_FLOAT_H
#include <float.h>
#endif /* HAVE_FLOAT_H */

#include <volume_io.h>
#include <time_stamp.h>
#include <ParseArgv.h>
#include <bicpl.h>

#ifndef DBL_MAX
#define DBL_MAX 99999999
#endif /* DBL_MAX not defined */

				/* globals */

char *default_dim_names[3] = { MIxspace, MIyspace, MIzspace };

char 
  *prog_name;
int
  invert_mask,
  normalize,
  debug,
  verbose;
int clobber_flag = FALSE;
static double mask_binvalue = 1.0;

static ArgvInfo argTable[] = {
  {"-no_clobber", ARGV_CONSTANT, (char *) FALSE, (char *) &clobber_flag,
     "Do not overwrite output file (default)."},
  {"-clobber", ARGV_CONSTANT, (char *) TRUE, (char *) &clobber_flag,
     "Overwrite output file."},
  {"-invert_mask", ARGV_CONSTANT, (char *) TRUE, (char *) &invert_mask,
     "Erase where mask==1."},
  {"-mask_binvalue", ARGV_FLOAT, (char *)1, (char *)&mask_binvalue,
    "Include mask voxels within 0.5 of this value"},
  {"-normalize", ARGV_CONSTANT, (char *) TRUE, (char *) &normalize,
     "Intensity normalize the resulting volume."},
  {NULL, ARGV_HELP, NULL, NULL,
     "Options for logging progress. Default = -verbose."},
  {"-verbose", ARGV_CONSTANT, (char *) TRUE, (char *) &verbose,
     "Write messages indicating progress"},
  {"-quiet", ARGV_CONSTANT, (char *) FALSE , (char *) &verbose,
     "Do not write log messages"},
  {"-debug", ARGV_CONSTANT, (char *) TRUE, (char *) &debug,
     "Print out debug info."},
  {NULL, ARGV_END, NULL, NULL, NULL}
};


int point_not_masked(Volume volume, Real wx, Real wy, Real wz) {

  double result;

  //printf("in point not masked\n");

  if (volume!=(Volume)NULL) {
    evaluate_volume_in_world(volume, wx, wy, wz, 0, TRUE, 0, &result,
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                             NULL, NULL);
       
    if (result>mask_binvalue-0.5 && result<mask_binvalue+0.5 ) {
      return(TRUE);
    }
  }
  return(FALSE);
}


int 
main ( argc, argv )
     int argc;
     char *argv[];
{   

  
  char 
    *history,
    *infilename,
    *maskfilename,
    *outfilename;
  
  Volume 
      tmp,
      data,mask;
  
  Status 
    status;

  Real
      min, max,
      voxel_value, true_value, zero, wx,wy,wz;
  int
    data_size[3],
    mask_size[3],
    i,j,k;
  
  progress_struct 
    progress;

				/* set globals */
  debug = FALSE;
  verbose = TRUE;
  prog_name = argv[0];
  invert_mask = FALSE;
  normalize = FALSE;
  
  history = time_stamp(argc,argv);

  /* Call ParseArgv to interpret all command line args */
  
  if (ParseArgv(&argc, argv, argTable, 0) || (argc!=4)) {
    (void) fprintf(stderr,
                   "\nUsage: %s [<options>] <inputfile> <maskfile> <outputfile>\n",
                   prog_name);
    (void) fprintf(stderr,"       %s [-help]\n\n", prog_name);
    exit(EXIT_FAILURE);
  }
  
  infilename  = argv[1];        /* set up necessary file names */
  maskfilename= argv[2];
  outfilename = argv[3];

  if (!clobber_flag && file_exists(outfilename)) {
    print ("File %s exists.\n",outfilename);
    print ("Use -clobber to overwrite.\n");
    return ERROR;
  }
  
  if (debug) {
    printf ("input file :%s\n",infilename);
    printf ("mask file  :%s\n",maskfilename);
    printf ("output file:%s\n",outfilename);
    if (invert_mask)
	printf ("mask will be inverted\n");
    else
	printf ("mask will not be inverted\n");
  }

  
				/* check file to be used.  */
  
  
  if ( !file_exists(infilename) ) 
    //print_error_and_line_num ("filename `%s' not found.", __FILE__, __LINE__, infilename);
  
  if (!file_exists(maskfilename) ) 
    //print_error_and_line_num ("filename `%s' not found.", __FILE__, __LINE__, maskfilename);

  if (file_exists(outfilename) && !clobber_flag ) 
    //print_error_and_line_num ("filename `%s' already exists. use -clobber to overwrite.", __FILE__, __LINE__, maskfilename);
  
				/* read input data volume */
  
  if (verbose)  printf ("Reading in input data.\n");
  status = input_volume(infilename, 3, default_dim_names, 
			NC_UNSPECIFIED, FALSE, 0.0, 0.0, 
			TRUE, &data, (minc_input_options *)NULL);
  if ( status != OK ) 
    printf("Problems reading %s.", infilename);
  
  if (verbose)
    printf ("Reading in mask data.\n");
  status = input_volume(maskfilename, 3, default_dim_names, 
			NC_UNSPECIFIED, FALSE, 0.0, 0.0, 
			TRUE, &mask, (minc_input_options *)NULL);
  if ( status != OK ) 
    printf ("Problems reading %s.\n", infilename);

  get_volume_sizes(data, data_size);
  get_volume_sizes(mask, mask_size);

  if (verbose)
    printf ("Building masked volume.\n");

				/* mask the data volume */
  zero = CONVERT_VALUE_TO_VOXEL(data, 0.0);

  if (verbose) initialize_progress_report( &progress, TRUE, data_size[0], "Masking volume");
  for_less(i, 0, data_size[0]) {
    for_less(j, 0, data_size[1]) {
      for_less(k, 0, data_size[2]) {

	convert_3D_voxel_to_world(data, (Real)i, (Real)j, (Real)k, &wx, &wy, &wz);

	if (!invert_mask)
	{
	    if (!point_not_masked(mask, wx,wy,wz) ) {
		SET_VOXEL_3D(data, i,j,k, zero);
            }
	}
	else 
	{
	    if (point_not_masked(mask, wx,wy,wz) ) {
		SET_VOXEL_3D(data, i,j,k, zero);
            }
	}
	
		
      }
    }
    if (verbose) update_progress_report( &progress, i+1 );
    
  }

  if (verbose) terminate_progress_report( &progress );

				/* normalize the intensity necessary */
  if (normalize) 
  {    
      min = DBL_MAX;
      max = -DBL_MAX;
      
      if (verbose) initialize_progress_report( &progress, TRUE, data_size[0], "Min/Max");
      for_less(i, 0, data_size[0]) {
	  for_less(j, 0, data_size[1]) {
	      for_less(k, 0, data_size[2]) {

		  GET_VALUE_3D( true_value, data, i,j,k);
		  if (true_value < min) 
		      min = true_value;
		  if (true_value > max)
		      max = true_value;

	      }
	  }
	  if (verbose) update_progress_report( &progress, i+1 );
	  
      }
      
      if (verbose) terminate_progress_report( &progress );

      if (debug) print ("Volume min and max: %f %f\n",min, max);
      
      tmp = copy_volume_definition_no_alloc(data, NC_UNSPECIFIED, FALSE, 0.0, 0.0);
      set_volume_real_range(tmp, min, max);
      

      if (verbose) initialize_progress_report( &progress, TRUE, data_size[0], "Normalizing");
      for_less(i, 0, data_size[0]) {
	  for_less(j, 0, data_size[1]) {
	      for_less(k, 0, data_size[2]) {

		  GET_VALUE_3D( true_value, data, i,j,k);
		  voxel_value = CONVERT_VALUE_TO_VOXEL( tmp, true_value);
		  SET_VOXEL_3D( data, i,j,k, voxel_value);

	      }
	  }
	  if (verbose) update_progress_report( &progress, i+1 );
	  
      }
      
      if (verbose) terminate_progress_report( &progress );

      set_volume_real_range(data, min, max);

  }
  
				/* output volume data!  */

  if (verbose)
    printf ("Writing volumetric data.\n");
  if (debug)
    printf ("output file:%s\n",outfilename);
  


  status = output_modified_volume(outfilename, NC_UNSPECIFIED, FALSE, 0.0,0.0,
				  data, infilename, history, (minc_output_options *)NULL);

  if (status != OK)
    //print_error_and_line_num("Problems writing masked data",__FILE__, __LINE__);
   
   
  delete_volume(mask);
  delete_volume(data);
  

  return(OK);
}





