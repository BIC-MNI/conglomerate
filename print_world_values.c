/*
*---------------------------------------------------------------------------
*@COPYRIGHT :
*             Copyright 2005, Jonathan Harlap
*             McConnell Brain Imaging Centre,
*             Montreal Neurological Institute, McGill University.
*             Permission to use, copy, modify, and distribute this
*             software and its documentation for any purpose and without
*             fee is hereby granted, provided that the above copyright
*             notice appear in all copies.  The author and McGill University
*             make no representations about the suitability of this
*             software for any purpose.  It is provided "as is" without
*             express or implied warranty.
*---------------------------------------------------------------------------- 
*$Revision: 1.3 $
*$Author: jharlap $
*$Date: 2006-02-26 13:33:46 $
*---------------------------------------------------------------------------
*
* print_world_values <minclist> <coordlist> <outputfile>
* 
* Reads the list of minc files in <minclist> (note that to handle
* glim_image files, it only reads the first word per line).  Reads the
* list of world coordinates in <coordlist> assuming that each line (up
* to a max of 255) contains space separated x, y, and z coordinates -
* implicitly this means that this program only deals with 3D volumes.
* Finally, for each minc it reads the real value of each coordinate
* specified in the coordlist and writes it out as a tab separated list
* into the output file.
*/

#include  <stdio.h>
#include  <volume_io.h>
#include  <bicpl.h>

void usage(char* progname) {
	printf("Usage: %s <glimfile> <coordlist file> <outputfile>\n\nNote that the glimfile is the list of minc files, using the first column of the file as the list of minc files.\nThe coordlist file contains one world coordinate in the form 'x y z' per line with up to 255 lines.\n", progname);
}

int  main(
			 int   argc,
			 char  *argv[] )
{
	STRING     glim_filename, coordlist_filename, output_filename;
	char       cur_minc[255];
	float      x[255], y[255], z[255];
	double       value;
	Volume     volume;
	float      curx, cury, curz;
	int        voxx, voxy, voxz, sizes[MAX_DIMENSIONS];
	Real       voxel[MAX_DIMENSIONS];
	int        i, r, keep_looping, n_coords;
	FILE*      coordfile;
	FILE*      glimfile;
	FILE*      outputfile;
	
	initialize_argument_processing( argc, argv );

	if( !get_string_argument( "", &glim_filename ) )
		{
			usage(argv[0]);
			return( 1 );
		}
 
	if( !get_string_argument( "", &coordlist_filename ) )
		{
			usage(argv[0]);
			return( 1 );
		}
 
	if( !get_string_argument( "", &output_filename ) )
		{
			usage(argv[0]);
			return( 1 );
		}
 
	/* read coordlist into x/y/z arrays */
	coordfile = fopen(coordlist_filename, "r");
	i = 0;
	keep_looping = 1;
	while(keep_looping && i < 255) {
		curx = 0;
		cury = 0;
		curz = 0;
		if(fscanf(coordfile, "%f%f%f", &curx, &cury, &curz) != 3)
			keep_looping = 0;
		else {
			x[i] = curx;
			y[i] = cury;
			z[i] = curz;
			++i;
		}
	}
	n_coords = i;
	fclose(coordfile);

	printf("There are %d coords\n", n_coords);
	for(i = 0; i < n_coords; ++i)
		printf("%d: %f %f %f\n", i, x[i], y[i], z[i]);

 
	/* read the glim file to get the minc filenames */
	keep_looping = 1;
	glimfile = fopen(glim_filename, "r");
	outputfile = fopen(output_filename, "w");

	/* print out the x,y,z of each coordinate */
	fprintf(outputfile, "x\t");
	for(i = 0; i < n_coords; ++i)
	  fprintf(outputfile, "%f\t", x[i]);
	fprintf(outputfile, "\ny\t");
	for(i = 0; i < n_coords; ++i)
	  fprintf(outputfile, "%f\t", y[i]);
	fprintf(outputfile, "\nz\t");
	for(i = 0; i < n_coords; ++i)
	  fprintf(outputfile, "%f\t", z[i]);
	fprintf(outputfile, "\n");
	  
	while(keep_looping) {
		if(fscanf(glimfile, "%[^ \t] %*[^\n] %*1[\n]", &cur_minc) > 0) {

			if( input_volume( cur_minc, 3, XYZ_dimension_names,
									NC_UNSPECIFIED, FALSE, 0.0, 0.0,
									TRUE, &volume, (minc_input_options *) NULL ) != OK ) {
				printf("Failed to read %s\n", cur_minc);
				return( 1 );
			}
			
			fprintf(outputfile, "%s\t", cur_minc);

			for(i = 0; i < n_coords ; ++i) {

				get_volume_sizes( volume, sizes );

				convert_world_to_voxel(volume, x[i], y[i], z[i], voxel);

				printf("Reading %f %f %f (%d %d %d) from %s\n", x[i], y[i], z[i], voxx, voxy, voxz, cur_minc);

				voxx = ROUND( voxel[0] );
				voxy = ROUND( voxel[1] );
				voxz = ROUND( voxel[2] );

				if( voxx >= 0 && voxx < sizes[0] &&
					 voxy >= 0 && voxy < sizes[1] &&
					 voxz >= 0 && voxz < sizes[2] )
					{
						value = (double) get_volume_real_value( volume, voxx, voxy, voxz, 0, 0);
						fprintf(outputfile, "%lf\t", value );
					} else {
						printf("Voxel %d %d %d is outside %s\n", voxx, voxy, voxz, cur_minc);
						fprintf(outputfile, "\t");
					}
			}
			fprintf(outputfile, "\n");

			delete_volume(volume);
			
		} else {
			keep_looping = 0;
		}
	}
	fclose(glimfile);
	fclose(outputfile);
		
	return(0);

}
