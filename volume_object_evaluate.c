#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

#include  "ParseArgv.h"

/* argument defaults */
int                  degrees_continuity = 0; /* default: linear */
int                  rgb_conversion = 0;     /* default: colour flag */

/* the argument table */
ArgvInfo argTable[] = {
  { "-linear", ARGV_CONSTANT, (char *) 0, 
    (char *) &degrees_continuity,
    "Use linear interpolation (Default)." },
  { "-nearest_neighbour", ARGV_CONSTANT, (char *) -1, 
    (char *) &degrees_continuity,
    "Use nearest neighbour interpolation." },
  { "-cubic", ARGV_CONSTANT, (char *) 2,
    (char *) &degrees_continuity,
        "Use cubic interpolation." },
  { "-rgb", ARGV_CONSTANT, (char *) 1,
    (char *) &rgb_conversion,
        "Convert rgb colour to colour flag." },
  
  { NULL, ARGV_END, NULL, NULL, NULL }
};

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               input_volume_filename, object_filename;
    STRING               output_filename;
    File_formats         format;
    minc_input_options   options;
    Volume               volume;
    int                  point, n_points, n_objects;
    Point                *points;
    object_struct        **objects;
    Real                 value;
//    Real                 rgb_values[4];
    FILE                 *file;

    /* Call ParseArgv */
    if ( ParseArgv( &argc, argv, argTable, 0 ) || ( argc != 4 ) ) {
      (void) fprintf( stderr,
                      "\nUsage: %s [options] <volume.mnc> <object.obj> <output.txt>\n", argv[0] );
      exit( 1 );
    }

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_volume_filename ) ||
        !get_string_argument( NULL, &object_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        print_error(
           "Usage: %s  volume.mnc  object.obj  output_file.txt\n", argv[0]);
        return( 1 );
    }

    set_default_minc_input_options( &options );
    if( rgb_conversion ) {
      degrees_continuity = -1;  // must use nearest with colour flags
      set_minc_input_vector_to_colour_flag( &options, TRUE );
      set_minc_input_vector_to_scalar_flag( &options, FALSE );
    }

    if( input_volume( input_volume_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, &options ) != OK )
        return( 1 );

    if( input_graphics_file( object_filename,
                             &format, &n_objects, &objects ) != OK )
        return( 1 );

    if( n_objects != 1 )
        print( "Warning, more than one object in file: %s\n", object_filename );

    n_points = get_object_points( objects[0], &points );

    if( open_file( output_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    for_less( point, 0, n_points )
    {
        evaluate_volume_in_world( volume,
                                  RPoint_x(points[point]),
                                  RPoint_y(points[point]),
                                  RPoint_z(points[point]),
                                  degrees_continuity, 
                                  FALSE, 0.0,
                                  &value,
                                  NULL, NULL, NULL,
                                  NULL, NULL, NULL, NULL, NULL, NULL );

        if( rgb_conversion ) {
          // rgb_values[0] = get_Colour_r_0_1( value );
          // rgb_values[1] = get_Colour_g_0_1( value );
          // rgb_values[2] = get_Colour_b_0_1( value );
          if( output_int( file, (int)value ) != OK ||
              output_newline( file ) != OK )
              return( 1 );
        } else {
          if( output_real( file, value ) != OK ||
              output_newline( file ) != OK )
              return( 1 );
        }
    }

    (void) close_file( file );

    delete_object_list( n_objects, objects );

    return( 0 );
}
