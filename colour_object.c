#include  <bicpl.h>

#define  GRAY_STRING       "gray"
#define  HOT_STRING        "hot"
#define  SPECTRAL_STRING   "spectral"
#define  RED_STRING        "red"
#define  GREEN_STRING      "green"
#define  BLUE_STRING       "blue"

int  main(
    int    argc,
    char   *argv[] )
{
    FILE                 *file;
    Real                 value, min_range, max_range;
    Status               status;
    STRING               src_filename, dest_filename, values_filename;
    STRING               under_colour_name, over_colour_name;
    int                  i, p, n_objects, n_points;
    Point                *points;
    File_formats         format;
    object_struct        **object_list;
    Colour               *colours, under_colour, over_colour, prev_colour, col;
    Colour_coding_types  coding_type;
    colour_coding_struct colour_coding;
    Colour_flags         *colour_flag_ptr;
    STRING               coding_type_string;
    Real                 low, high, r, g, b, a, opacity;
    BOOLEAN              per_vertex;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &src_filename ) ||
        !get_string_argument( NULL, &values_filename ) ||
        !get_string_argument( NULL, &dest_filename ) ||
        !get_string_argument( NULL, &coding_type_string ) ||
        !get_real_argument( 0.0, &low ) ||
        !get_real_argument( 0.0, &high ) )
    {
        print_error( "Usage: %s  src.obj values_file dest.obj\n",
                     argv[0] );
        print_error( "           gray|hot|spectral low high\n" );
        print_error( "           [under] [over] [opacity]\n" );
        return( 1 );
    }

    if( equal_strings( coding_type_string, GRAY_STRING ) )
        coding_type = GRAY_SCALE;
    else if( equal_strings( coding_type_string, HOT_STRING ) )
        coding_type = HOT_METAL;
    else if( equal_strings( coding_type_string, SPECTRAL_STRING ) )
        coding_type = SPECTRAL;
    else if( equal_strings( coding_type_string, RED_STRING ) )
        coding_type = RED_COLOUR_MAP;
    else if( equal_strings( coding_type_string, GREEN_STRING ) )
        coding_type = GREEN_COLOUR_MAP;
    else if( equal_strings( coding_type_string, BLUE_STRING ) )
        coding_type = BLUE_COLOUR_MAP;
    else
    {
        print_error( "Invalid coding type: %s\n", coding_type_string );
        return( 1 );
    }

    (void) get_string_argument( "BLACK", &under_colour_name );
    (void) get_string_argument( "WHITE", &over_colour_name );
    (void) get_real_argument( 1.0, &opacity );

    if( !lookup_colour( under_colour_name, &under_colour ) ||
        !lookup_colour( over_colour_name, &over_colour ) )
    {
        print_error( "Invalid colour names.\n" );
        return( 1 );
    }

    initialize_colour_coding( &colour_coding, coding_type,
                              under_colour, over_colour, low, high );

    if( input_graphics_file( src_filename, &format, &n_objects,
                             &object_list ) != OK )
        return( 1 );

    if( n_objects != 1 )
    {
        print_error( "Must contain exactly one object.\n" );
        return( 1 );
    }

    if( open_file( values_filename, READ_FILE, ASCII_FORMAT, &file ) != OK )
        return( 1 );

    for_less( i, 0, n_objects )
    {
        n_points = get_object_points( object_list[i], &points );

        colour_flag_ptr = get_object_colours( object_list[i], &colours );

        if( *colour_flag_ptr == PER_VERTEX_COLOURS )
            per_vertex = TRUE;
        else
        {
            per_vertex = FALSE;
            prev_colour = colours[0];
            REALLOC( colours, n_points );
            set_object_colours( object_list[i], colours );
        }

        *colour_flag_ptr = PER_VERTEX_COLOURS;

        min_range = 0.0;
        max_range = 0.0;

        for_less( p, 0, n_points )
        {
            if( input_real( file, &value ) != OK )
            {
                print_error( "Could not read %d'th value from file.\n", p );
                return( 1 );
            }

            col = get_colour_code( &colour_coding, value );

            if( opacity * get_Colour_a_0_1(col) < 1.0 )
            {
                if( per_vertex )
                    prev_colour = colours[p];

                r = get_Colour_r_0_1( col );
                g = get_Colour_g_0_1( col );
                b = get_Colour_b_0_1( col );
                a = get_Colour_a_0_1( col );
                col = make_rgba_Colour_0_1( r * opacity,
                                            g * opacity,
                                            b * opacity,
                                            a * opacity );

                COMPOSITE_COLOURS( col, col, prev_colour );
            }

            colours[p] = col;

            if( p == 0 || value < min_range )
                min_range = value;
            if( p == 0 || value > max_range )
                max_range = value;
        }

        print( "Value range: %g %g\n", min_range, max_range );
    }

    close_file( file );

    status = output_graphics_file( dest_filename, format,
                                   n_objects, object_list );

    return( status != OK );
}
