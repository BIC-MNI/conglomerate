#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

#define  DEFAULT_DEGREE   0

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s surface.obj  output.mnc nx ny\n\
                 gray|hot|spectral|red|green|blue min_limit max_limit\n\
                 under_colour over_colour\n\
                 [volume.mnc [degree]]\n\
\n\
     Maps a surface to a flat sheet image.  If the volume is not specified,\n\
     then uses surface curvature for colour.  Otherwise uses the volume\n\
     interpolated in the specified degree of either -1, 0, or 2.\n\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               surface_filename;
    STRING               output_filename, colour_coding_name;
    STRING               under_colour_name, over_colour_name;
    Point                *electrode_pos, *electrode_unit_pos;
    STRING               *electrode_names;
    Point                centre;
    FILE                 *file;
    File_formats         format;
    polygons_struct      *surface, unit_sphere;
    int                  i, n_objects, n_electrodes;
    Colour               under_colour, over_colour;
    object_struct        **objects;
    colour_coding_struct colour_coding;

    /*--- get the arguments from the command line */

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &surface_filename ) ||
        !get_string_argument( NULL, &output_filename ) ||
        !get_int_argument( 0, &nx ) ||
        !get_int_argument( 0, &ny ) ||
        !get_string_argument( NULL, &colour_coding_name ) ||
        !get_real_argument( 0.0, &min_limit ) ||
        !get_real_argument( 0.0, &max_limit ) ||
        !get_string_argument( NULL, &under_colour_name ) ||
        !get_string_argument( NULL, &over_colour_name ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    use_volume = get_string_argument( NULL, &volume_filename );
    if( use_volume )
        (void) get_int_argument( DEFAULT_DEGREE, &degree );

    under_colour = convert_string_to_colour( under_colour_name );
    over_colour = convert_string_to_colour( over_colour_name );

    /*--- input the surface */

     if( input_graphics_file( surface_filename, &format, &n_objects, &objects )
         != OK )
        return( 1 );

    /*--- check that the surface file contains a polyhedron */

    if( n_objects != 1 || get_object_type( objects[0] ) != POLYGONS )
    {
        print( "Surface file must contain 1 polygons object.\n" );
        return( 1 );
    }

    if( use_volume )
    {
        if( input_volume( volume_filename, 3, XYZ_dimension_names,
                          NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                          TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );
    }

    if( equal_strings( colour_coding_name, "hot" ) )
        coding_type = HOT_METAL;
    if( equal_strings( colour_coding_name, "gray" ) )
        coding_type = GRAY_SCALE;
    if( equal_strings( colour_coding_name, "spectral" ) )
        coding_type = SPECTRAL;
    if( equal_strings( colour_coding_name, "red" ) )
        coding_type = RED_COLOUR_MAP;
    if( equal_strings( colour_coding_name, "green" ) )
        coding_type = GREEN_COLOUR_MAP;
    if( equal_strings( colour_coding_name, "blue" ) )
        coding_type = BLUE_COLOUR_MAP;

    initialize_colour_coding( &colour_coding,
                              coding_type, under_colour, over_colour,
                              min_limit, max_limit );

    /*--- get a pointer to the surface */

    surface = get_polygons_ptr( objects[0] );

    /*--- create a unit sphere with same number of triangles as skin surface */

    fill_Point( centre, 0.0, 0.0, 0.0 );

    create_tetrahedral_sphere( &centre, 1.0, 1.0, 1.0,
                               surface->n_items, &unit_sphere );

    image = create_volume( 2, NULL, NC_SHORT, FALSE, 0.0, 0.0 );

    /*--- map the electode positions to positions on unit sphere */

    ALLOC( electrode_unit_pos, n_electrodes );

    for_less( i, 0, n_electrodes )
    {
        map_point_to_unit_sphere( surface, &electrode_pos[i],
                                  &unit_sphere, &electrode_unit_pos[i] );
    }

    /*--- output the electrode positions on the unit sphere */

    if( open_file( unit_electrode_filename, WRITE_FILE, ASCII_FORMAT, &file )
        != OK )
        return( 1 );

    for_less( i, 0, n_electrodes )
    {
        if( output_string( file, electrode_names[i] ) != OK ||
            output_string( file, " " ) != OK ||
            io_point( file, WRITE_FILE, ASCII_FORMAT, &electrode_unit_pos[i] )
            != OK ||
            output_newline( file ) != OK )
            return( 1 );
    }

    (void) close_file( file );

    /*--- output the vertex positions on the unit sphere */

    if( open_file( unit_vertices_filename, WRITE_FILE, ASCII_FORMAT, &file )
        != OK )
        return( 1 );

    for_less( i, 0, surface->n_points )
    {
        if( io_point( file, WRITE_FILE, ASCII_FORMAT, &unit_sphere.points[i] )
            != OK ||
            output_newline( file ) != OK )
            return( 1 );
    }

    (void) close_file( file );

    /*--- free up memory */

    delete_polygons( &unit_sphere );
    delete_object_list( n_objects, objects );
    FREE( electrode_unit_pos );
    FREE( electrode_pos );
    FREE( electrode_names );

    return( 0 );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : input_eeg_file
@INPUT      : filename
@OUTPUT     : n_electrodes
              electrode_names
              electrode_pos
@RETURNS    : OK or ERROR
@DESCRIPTION: Reads a file of electrode 3D positions and voltages.
@CREATED    : 1993            David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  Status   input_eeg_file(
    STRING  filename,
    int     *n_electrodes,
    STRING  *electrode_names[],
    Point   *electrode_pos[] )
{
    Real     x, y, z;
    STRING   name;
    FILE     *file;

    /*--- open the file */

    if( open_file( filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
        return( ERROR );

    *n_electrodes = 0;

    /*--- read name, x, y, z until none left */

    while( input_string( file, &name, ' ' ) == OK &&
           input_real( file, &x ) == OK &&
           input_real( file, &y ) == OK &&
           input_real( file, &z ) == OK )
    {
        SET_ARRAY_SIZE( *electrode_names, *n_electrodes, (*n_electrodes)+1,
                        DEFAULT_CHUNK_SIZE );
        SET_ARRAY_SIZE( *electrode_pos, *n_electrodes, (*n_electrodes)+1,
                        DEFAULT_CHUNK_SIZE );
        fill_Point( (*electrode_pos)[*n_electrodes], x, y, z );
        (*electrode_names)[*n_electrodes] = name;
        ++(*n_electrodes);
    }

    (void) close_file( file );

    return( OK );
}
