#include  <Xlib.h>
#include  <stdio.h>

int  main(
    int  argc,
    char *argv[] )
{
    Display            *display;
    int                screen;
    Window             root_window;
    XWindowAttributes  attrib;
    Colormap           cmap;
    XColor             def;
    int                i;

    display = XOpenDisplay( 0 );

    if( display == NULL )
    {
        (void) fprintf( stderr, "Could not open X display\n" );
    }

    display = XOpenDisplay( 0 );

    if( display == NULL )
    {
        (void) fprintf( stderr, "Could not open X display\n" );
    }

    screen = DefaultScreen( display );

    root_window =  RootWindow( display, screen ),

    XGetWindowAttributes( display, root_window, &attrib );

    cmap = attrib.colormap;

    for( i = 0;  i < 32;  ++i )
    {
        def.pixel = i;
        XQueryColor( display, cmap, &def );
        printf( "%d:  %d %d %d\n", def.pixel, def.red, def.green, def.blue  );
    }

    return( 0 );
}
