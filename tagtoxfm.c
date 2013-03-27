#include <volume_io.h>
#include <bicpl.h>

/* Main program */

int main(int argc, char *argv[])
{
    char *pname, *tagfile, *xfmfile;
    int i, n_volumes, n_tag_points, n_degrees;
    VIO_Real **tags_volume1, **tags_volume2;
    VIO_Real rms, tx, ty, tz, dx, dy, dz;
    Trans_type      transform_type;
    General_transform transform;
    FILE *fp;
    char *type_string, comment[512];

    pname = argv[0];
    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &tagfile ) ||
        !get_string_argument( "", &xfmfile ) )
    {
        print_error( "Usage: %s input.tag output.xfm [6|7|9|10|12]\n", argv[0] );
        return( 1 );
    }

    if( get_int_argument( 0, &n_degrees ) )
    {
        switch( n_degrees )
        {
        case 6:
            transform_type = TRANS_LSQ6;
            break;
        case 7:
            transform_type = TRANS_LSQ7;
            break;
        case 9:
            transform_type = TRANS_LSQ9;
            break;
        case 10:
            transform_type = TRANS_LSQ10;
            break;
        case 12:
            transform_type = TRANS_LSQ12;
            break;

        default:
            print( "Invalid degrees.\n" );
            return( 1 );
        }
    }
    else
        transform_type = TRANS_TPS;

    if ((open_file_with_default_suffix(tagfile,
                   get_default_tag_file_suffix(),
                   READ_FILE, ASCII_FORMAT, &fp) != VIO_OK) ||
        (input_tag_points(fp, &n_volumes, &n_tag_points, 
                          &tags_volume1, &tags_volume2, 
                          NULL, NULL, NULL, NULL) != VIO_OK)) {
       (void) fprintf(stderr, "%s: Error reading tag file %s\n", 
                      pname, tagfile);
       exit(EXIT_FAILURE);
    }
    (void) close_file(fp);

   /* Check number of volumes */
   if (n_volumes != 2) {
      (void) fprintf(stderr, "%s: Wrong number of volumes in %s\n", 
                     pname, tagfile);
      exit(EXIT_FAILURE);
   }

   /* Check number of points for linear transformation */
   if (((transform_type == TRANS_LSQ6) ||
        (transform_type == TRANS_LSQ7) ||
        (transform_type == TRANS_LSQ9) ||
        (transform_type == TRANS_LSQ10) ||
        (transform_type == TRANS_LSQ12)) &&
       (n_tag_points < MIN_POINTS_LINEAR) ) {
      (void) fprintf(stderr, 
                     "%s: Need at least %d points (only %d in %s)\n", 
                     pname, MIN_POINTS_LINEAR, n_tag_points, tagfile);
      exit(EXIT_FAILURE);
   }

   /* Check number of points for thin-plate spline transformation */
   if ((transform_type == TRANS_TPS) &&
       (n_tag_points < MIN_POINTS_TPS) ) {
      (void) fprintf(stderr, 
                     "%s: Need at least %d points (only %d in %s)\n", 
                     pname, MIN_POINTS_TPS, n_tag_points, tagfile);
      exit(EXIT_FAILURE);
   }

   /* Compute transformation */
   compute_transform_from_tags(n_tag_points, tags_volume1, tags_volume2,
                               transform_type, &transform);

   /* Create output file comment */
   switch (transform_type) {
   case TRANS_LSQ6: type_string = "6 parameter linear least-squares"; break;
   case TRANS_LSQ7: type_string = "7 parameter linear least-squares"; break;
   case TRANS_LSQ9: type_string = "9 parameter linear least-squares"; break;
   case TRANS_LSQ10: type_string = "10 parameter linear least-squares"; break;
   case TRANS_LSQ12: type_string = "12 parameter linear least-squares"; break;
   case TRANS_TPS: type_string = "thin-plate spline"; break;
   default: type_string = "unknown"; break;
   }

   (void) sprintf(comment, " Created from tag file %s\n using %s",
                  tagfile, type_string );

   /* Save transformation */

   if (output_transform_file(xfmfile, comment, &transform) != VIO_OK) {
      (void) fprintf(stderr, "%s: Error writing xfm file %s\n", 
                     pname, xfmfile);
      exit(EXIT_FAILURE);
   }

   rms = 0.0;

   for_less( i, 0, n_tag_points )
   {
       general_transform_point( &transform,
                                tags_volume2[i][X],
                                tags_volume2[i][Y],
                                tags_volume2[i][Z], &tx, &ty, &tz );

       dx = tags_volume1[i][X] - tx;
       dy = tags_volume1[i][Y] - ty;
       dz = tags_volume1[i][Z] - tz;

       rms += dx * dx + dy * dy + dz * dz;
   }

   rms = sqrt( rms / (VIO_Real) n_tag_points );

   print( "Rms: %g\n", rms );

   return EXIT_SUCCESS;
}

