
#include  <graphics.h>
#include  <rgb_files.h>

#define  DEFAULT_FRAME_RATE   10.0

private  void  display_loop(
    STRING           filenames[],
    window_struct    *window,
    int              n_frames,
    pixels_struct    pixels[],
    Real             frame_rate );
private  void  display_frame_info(
    window_struct   *window,
    int             frame_number,
    int             frame_rate,
    char            filename[] );

int  main( argc, argv )
    int   argc;
    char  *argv[];
{
    Colour           background = GREY;
    int              i, x_size, y_size;
    window_struct    *window;
    Real             frame_rate;
    int              n_frames;
    STRING           *filenames;
    pixels_struct    *pixels;
    progress_struct  progress;

    background = GREY;
    if( argc < 2 )
    {
        print( "film_loop  image1  image2  ...\n" );
        return( 1 );
    }

    n_frames = argc - 1;
    ALLOC( pixels, n_frames );
    ALLOC( filenames, n_frames );

    x_size = 1;
    y_size = 1;

    initialize_progress_report( &progress, FALSE, n_frames, "Reading Images" );

    for_less( i, 0, n_frames )
    {
        strip_off_directories( argv[i+1], filenames[i] );

        if( input_rgb_file( argv[i+1], &pixels[i] ) != OK )
            return( 1 );

        if( pixels[i].x_size > x_size )
            x_size = pixels[i].x_size;
        if( pixels[i].x_size > y_size )
            y_size = pixels[i].y_size;

        update_progress_report( &progress, i+1 );
    }

    terminate_progress_report( &progress );

    frame_rate = DEFAULT_FRAME_RATE;

    print( "\nCommands:\n" );
    print( "LEFT   MOUSE:   slow down\n" );
    print( "MIDDLE MOUSE:   stop/start\n" );
    print( "RIGHT  MOUSE:   speed up\n" );
    print( "Esc         :   quit\n" );

    (void) G_create_window( argv[1], -1, -1, x_size, y_size, &window );

    G_set_background_colour( window, background );

    display_loop( filenames, window, n_frames, pixels, frame_rate );

    return( 0 );
}

private  void  display_loop(
    STRING           filenames[],
    window_struct    *window,
    int              n_frames,
    pixels_struct    pixels[],
    Real             frame_rate )
{
    BOOLEAN        running;
    Real           seconds_per_frame;
    BOOLEAN        done;
    int            frame;
    Real           next_update;
    Event_types    event_type;
    int            key_pressed;
    window_struct  *event_window;

    G_set_view_type( window, PIXEL_VIEW );

    seconds_per_frame = 1.0 / (Real) frame_rate;

    frame = 0;

    running = TRUE;

    done = FALSE;

    next_update = 0.0;

    while( !done )
    {
        event_type = G_get_event( &event_window, &key_pressed );

        if( event_window == window || event_type == NO_EVENT )
        {
            switch( event_type )
            {
            case KEY_DOWN_EVENT:
                if( key_pressed == '' )
                    done = TRUE;
                break;

            case MIDDLE_MOUSE_DOWN_EVENT:
                running = !running;
                if( running )
                    next_update = current_realtime_seconds();
                break;

            case RIGHT_MOUSE_DOWN_EVENT:
                ++frame_rate;
                seconds_per_frame = 1.0 / (Real) frame_rate;
                next_update = current_realtime_seconds();
                break;

            case LEFT_MOUSE_DOWN_EVENT:
                if( frame_rate > 1 )
                {
                    --frame_rate;
                    seconds_per_frame = 1.0 / (Real) frame_rate;
                    next_update = current_realtime_seconds();
                }
                break;

            case NO_EVENT:
                if( running && current_realtime_seconds() >= next_update )
                {
                    G_draw_pixels( window, &pixels[frame] );
                    display_frame_info( window, frame, frame_rate,
                                        filenames[frame] );
                    G_update_window( window );

                    ++frame;
                    if( frame >= n_frames )
                        frame = 0;
                    next_update += seconds_per_frame;
                }
            }
        }
    }

    (void) G_delete_window( window );
}

private  void  display_frame_info(
    window_struct   *window,
    int             frame_number,
    int             frame_rate,
    char            filename[] )
{
    static   BOOLEAN       first = TRUE;
    static   text_struct   text;
    static   Point         origin = { 10.0, 10.0, 0.0 };

    if( first )
    {
        first = FALSE;
        initialize_text( &text, &origin, GREEN, FIXED_FONT, 10.0 );
    }

    (void) sprintf( text.string, "%3d f/s %3d: %s",
                    frame_rate, frame_number+1, filename );

    G_draw_text( window, &text );
}
