#include  <bicpl.h>

#define  GRAY_STRING       "gray"
#define  HOT_STRING        "hot"
#define  SPECTRAL_STRING   "spectral"
#define  RED_STRING        "red"
#define  GREEN_STRING      "green"
#define  BLUE_STRING       "blue"

#define  SCALE 100.0

int  main(
    int    argc,
    char   *argv[] )
{
    Real                 min_limit, max_limit, width, tick_width, value, y;
    Real                 scale;
    STRING               output_filename;
    STRING               under_colour_name, over_colour_name;
    int                  p, n_steps, point_index0, point_index1;
    object_struct        **object_list;
    quadmesh_struct      *quadmesh;
    Point                point;
    lines_struct         *lines;
    Colour               under_colour, over_colour;
    Colour_coding_types  coding_type;
    colour_coding_struct colour_coding;
    STRING               coding_type_string;
    Real                 low, high;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &output_filename ) ||
        !get_string_argument( NULL, &coding_type_string ) ||
        !get_real_argument( 0.0, &min_limit ) ||
        !get_real_argument( 0.0, &low ) ||
        !get_real_argument( 0.0, &high ) ||
        !get_real_argument( 0.0, &max_limit ) )
    {
        print_error(
            "Usage: %s  output.obj gray|hot|spectral min low high max\n",
            argv[0]  );
        print_error( "   [under_col]  [over_col]  [n_steps]  \n" );
        print_error( "   [width] [tick_width] [scale]\n" );
        return( 1 );
    }

    (void) get_string_argument( "GREY", &under_colour_name );
    (void) get_string_argument( "WHITE", &over_colour_name );

    (void) get_int_argument( 1000, &n_steps );
    (void) get_real_argument( 0.03, &width );
    (void) get_real_argument( 0.01, &tick_width );
    (void) get_real_argument( SCALE, &scale );

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

    if( !lookup_colour( under_colour_name, &under_colour ) ||
        !lookup_colour( over_colour_name, &over_colour ) )
    {
        print_error( "Invalid colour names.\n" );
        return( 1 );
    }

    initialize_colour_coding( &colour_coding, coding_type,
                              under_colour, over_colour, low, high );

    ALLOC( object_list, 2 );

    object_list[0] = create_object( QUADMESH );
    quadmesh = get_quadmesh_ptr( object_list[0] );
    object_list[1] = create_object( LINES );
    lines = get_lines_ptr( object_list[1] );

    initialize_quadmesh( quadmesh, WHITE, NULL, 2, n_steps );

    REALLOC( quadmesh->colours, 2 * n_steps );
    quadmesh->colour_flag = PER_VERTEX_COLOURS;

    for_less( p, 0, n_steps )
    {
        y = (Real) p / (Real) (n_steps-1);
        value = INTERPOLATE( y, min_limit, max_limit );
        point_index0 = IJ(0,p,n_steps);
        point_index1 = IJ(1,p,n_steps);

        quadmesh->colours[point_index0] = get_colour_code(
                                                &colour_coding, value );
        quadmesh->colours[point_index1] = get_colour_code(
                                                &colour_coding, value );

        fill_Point( quadmesh->points[point_index0], 0.0, scale * y, 0.0 );
        fill_Point( quadmesh->points[point_index1], scale * width,
                    scale * y, 0.0 );

        fill_Vector( quadmesh->normals[point_index0], 0.0, 0.0, 1.0 );
        fill_Vector( quadmesh->normals[point_index1], 0.0, 0.0, 1.0 );
    }

    initialize_lines( lines, WHITE );

    REALLOC( lines->colours, 4 );
    lines->colour_flag = PER_ITEM_COLOURS;
    lines->colours[0] = WHITE;
    lines->colours[1] = GREEN;
    lines->colours[2] = GREEN;
    lines->colours[3] = WHITE;

    start_new_line( lines );
    fill_Point( point, scale * width, 0.0, 0.0 );
    add_point_to_line( lines, &point );
    fill_Point( point, scale * (width + tick_width), 0.0, 0.0 );
    add_point_to_line( lines, &point );

    y = (low - min_limit) / (max_limit - min_limit);
    start_new_line( lines );
    fill_Point( point, scale * width, scale * y, 0.0 );
    add_point_to_line( lines, &point );
    fill_Point( point, scale * (width + tick_width), scale * y, 0.0 );
    add_point_to_line( lines, &point );

    y = (high - min_limit) / (max_limit - min_limit);
    start_new_line( lines );
    fill_Point( point, scale * width, scale * y, 0.0 );
    add_point_to_line( lines, &point );
    fill_Point( point, scale * (width + tick_width), scale * y, 0.0 );
    add_point_to_line( lines, &point );

    start_new_line( lines );
    fill_Point( point, scale * width, scale * 1.0, 0.0 );
    add_point_to_line( lines, &point );
    fill_Point( point, scale * (width + tick_width), scale * 1.0, 0.0 );
    add_point_to_line( lines, &point );

    (void) output_graphics_file( output_filename, BINARY_FORMAT,
                                 2, object_list );

    return( 0 );
}
