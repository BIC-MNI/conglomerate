#include  <mni.h>

#define  BACKGROUND_VALUE   0.0

typedef  struct
{
    int  x;
    int  y;
} xy_struct;

typedef  struct
{
    int         n_pixels;
    xy_struct   *pixels;

    int         roi_id;

    int         x_min;
    int         x_max;
    int         y_min;
    int         y_max;
} contour_struct;

private  int  extract_contours(
    Volume           volume,
    int              axis,
    int              slice,
    Real             min_threshold,
    Real             max_threshold,
    contour_struct   *contours[] );

private  Status  write_contours(
    FILE             *file,
    int              n_contours,
    contour_struct   contours[] );

private  void  delete_contours(
    int              n_contours,
    contour_struct   contours[] );

int  main(
    int   argc,
    char  *argv[] )
{
    Real             min_threshold, max_threshold;
    FILE             *file;
    int              axis, slice, sizes[N_DIMENSIONS], n_contours;
    char             *axis_name;
    char             *volume_filename, *output_roi_filename;
    Volume           volume;
    BOOLEAN          min_present, max_present;
    progress_struct  progress;
    contour_struct   *contours;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &volume_filename ) ||
        !get_string_argument( "", &output_roi_filename ) )
    {
        print( "Usage: %s  volume_file  roi_output   [min_threshold]  [max_threshold] [x|y|z]\n",
               argv[0] );
        return( 1 );
    }

    axis = Z;
    min_present = FALSE;
    max_present = FALSE;

    while( get_string_argument( "", &axis_name ) )
    {
        if( axis_name[0] >= 'x' && axis_name[0] <= 'z' )
            axis = (int) (axis_name[0] - 'x');
        else if( axis_name[0] >= 'X' && axis_name[0] <= 'Z' )
            axis = (int) (axis_name[0] - 'X');
        else if( !min_present )
        {
            (void) sscanf( axis_name, "%lf", &min_threshold );
            min_present = TRUE;
        }
        else if( !max_present )
        {
            (void) sscanf( axis_name, "%lf", &max_threshold );
            max_present = TRUE;
        }
        else
        {
            print( "Too many thresholds on command line.\n" );
            return( 1 );
        }
    }

    if( !min_present )
    {
        min_threshold = 0.0;
        max_threshold = -1.0;
    }
    else if( !max_present )
        max_threshold = min_threshold;

    if( input_volume( volume_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    get_volume_sizes( volume, sizes );

    if( open_file( output_roi_filename, WRITE_FILE, ASCII_FORMAT, &file ) != OK)
        return( 1 );

    initialize_progress_report( &progress, FALSE, sizes[axis],
                                "Creating ROI's" );

    for_less( slice, 0, sizes[axis] )
    {
        n_contours = extract_contours( volume, axis, slice, min_threshold,
                                       max_threshold, &contours );
        if( write_contours( file, n_contours, contours ) != OK )
            return( 1 );

        update_progress_report( &progress, slice + 1 );
        delete_contours( n_contours, contours );
    }

    terminate_progress_report( &progress );

    (void) close_file( file );

    return( 0 );
}

private  void  delete_contour(
    contour_struct   *contour )
{
    if( contour->n_pixels > 0 )
        FREE( contour->pixels );
}

private  void  delete_contours(
    int              n_contours,
    contour_struct   contours[] )
{
    int    i;

    for_less( i, 0, n_contours )
        delete_contour( &contours[i] );

    if( n_contours > 0 )
        FREE( contours );
}

private  Status  write_contour(
    FILE             *file,
    contour_struct   *contour )
{
    int      i;

    if( output_int( file, contour->roi_id ) != OK ||
        output_int( file, contour->x_max ) != OK ||
        output_int( file, contour->x_min ) != OK ||
        output_int( file, contour->y_max ) != OK ||
        output_int( file, contour->y_min ) != OK ||
        output_newline( file ) != OK )
    {
        return( ERROR );
    }

    if( output_int( file, contour->n_pixels ) != OK ||
        output_newline( file ) != OK )
    {
        return( ERROR );
    }

    for_less( i, 0, contour->n_pixels )
    {
        if( output_int( file, contour->pixels[i].x ) != OK ||
            output_int( file, contour->pixels[i].y ) != OK ||
            output_newline( file ) != OK )
        {
            return( ERROR );
        }
    }

    if( output_newline( file ) != OK )
        return( ERROR );

    return( OK );
}

private  Status  write_contours(
    FILE             *file,
    int              n_contours,
    contour_struct   contours[] )
{
    int      i;

    if( output_int( file, n_contours ) != OK ||
        output_newline( file ) != OK )
        return( ERROR );

    for_less( i, 0, n_contours )
    {
        if( write_contour( file, &contours[i] ) != OK )
            return( ERROR );
    }

    return( OK );
}

private  float  get_value(
    int             x_size,
    int             y_size,
    float           **slice,
    int             x,
    int             y )
{
    if( x < 0 || x >= x_size || y < 0 || y >= y_size )
        return( BACKGROUND_VALUE );
    else
        return( slice[x][y] );
}

#define  N_DIRECTIONS    8

private  int   Delta_x[N_DIRECTIONS] = { 1, 1, 0, -1, -1, -1,  0,  1 };
private  int   Delta_y[N_DIRECTIONS] = { 0, 1, 1,  1,  0, -1, -1, -1 };

private  BOOLEAN  is_inside(
    int     x_size,
    int     y_size,
    float   **slice,
    Real    min_threshold,
    Real    max_threshold,
    int     x,
    int     y )
{
    Real   value;

    value = get_value( x_size, y_size, slice, x, y );

    if( value == BACKGROUND_VALUE )
        return( FALSE );

    if( min_threshold <= max_threshold )
    {
        if( value < min_threshold || value > max_threshold )
            return( FALSE );
    }

    return( TRUE );
}

private  BOOLEAN  is_boundary_pixel(
    int     x_size,
    int     y_size,
    float   **slice,
    Real    min_threshold,
    Real    max_threshold,
    int     x,
    int     y )
{
    int    i;

    if( !is_inside( x_size, y_size, slice, min_threshold, max_threshold,
                    x, y ) )
    {
        return( FALSE );
    }

    for( i = 0;  i < N_DIRECTIONS;  i += 2 )
    {
        if( !is_inside( x_size, y_size, slice, min_threshold, max_threshold,
                        x+Delta_x[i], y+Delta_y[i] ) )
            return( TRUE );
    }

    return( FALSE );
}

private  void  extract_contour(
    int             x_size,
    int             y_size,
    float           **slice,
    Real            min_threshold,
    Real            max_threshold,
    Smallest_int    **done,
    int             x_start,
    int             y_start,
    contour_struct  *contour )
{
    int         i, x, y, dir;
    xy_struct   xy;
    float       value;
    static  int new_dirs[N_DIRECTIONS] = { 6, 6, 0, 0, 2, 2, 4, 4 };

    value = slice[x_start][y_start];

    contour->n_pixels = 0;
    contour->roi_id = ROUND( value );

    xy.x = x_start;
    xy.y = y_start;

    for( dir = 0;  dir < N_DIRECTIONS;  dir += 2 )
    {
        if( !is_inside( x_size, y_size, slice, min_threshold, max_threshold,
                        x_start+Delta_x[dir], y_start+Delta_y[dir] ) )
        {
            break;
        }
    }

    do
    {
        ADD_ELEMENT_TO_ARRAY( contour->pixels, contour->n_pixels, xy,
                              DEFAULT_CHUNK_SIZE );
        done[xy.x][xy.y] = TRUE;

        if( contour->n_pixels > 1000000 )
            HANDLE_INTERNAL_ERROR( "daffd" );

        for_less( i, 0, N_DIRECTIONS )
        {
            dir = (dir + 1) % N_DIRECTIONS;
            if( is_inside( x_size, y_size, slice, min_threshold, max_threshold,
                       xy.x + Delta_x[dir], xy.y + Delta_y[dir] ) )
                break;
        }

        if( i == N_DIRECTIONS )
            break;

        xy.x = xy.x + Delta_x[dir];
        xy.y = xy.y + Delta_y[dir];

        dir = new_dirs[dir];
    }
    while( xy.x != x_start || xy.y != y_start );

    for_less( i, 0, contour->n_pixels )
    {
        x = contour->pixels[i].x;
        y = contour->pixels[i].y;
        if( i == 0 )
        {
            contour->x_min = x;
            contour->x_max = x;
            contour->y_min = y;
            contour->y_max = y;
        }
        else
        {
            if( x < contour->x_min )
                contour->x_min = x;
            else if( x > contour->x_max )
                contour->x_max = x;
            if( y < contour->y_min )
                contour->y_min = y;
            else if( y > contour->y_max )
                contour->y_max = y;
        }
    }
}

private  int  extract_contours(
    Volume           volume,
    int              axis,
    int              slice,
    Real             min_threshold,
    Real             max_threshold,
    contour_struct   *contours[] )
{
    int             a1, a2, n_contours, sizes[MAX_DIMENSIONS];
    int             x, y, ind[MAX_DIMENSIONS];
    float           **values;
    Real            used_min, used_max;
    Smallest_int    **done;
    contour_struct  contour;

    get_volume_sizes( volume, sizes );

    a1 = (axis + 1) % N_DIMENSIONS;
    a2 = (axis + 2) % N_DIMENSIONS;

    ALLOC2D( done, sizes[a1], sizes[a2] );
    ALLOC2D( values, sizes[a1], sizes[a2] );

    n_contours = 0;

    ind[axis] = slice;

    for_less( ind[a1], 0, sizes[a1] )
    {
        for_less( ind[a2], 0, sizes[a2] )
        {
            done[ind[a1]][ind[a2]] = FALSE;
            GET_VALUE_3D( values[ind[a1]][ind[a2]], volume,
                          ind[X], ind[Y], ind[Z] );
        }
    }

    for_less( x, 0, sizes[a1] )
    {
        for_less( y, 0, sizes[a2] )
        {
            if( min_threshold <= max_threshold )
            {
                used_min = min_threshold;
                used_max = max_threshold;
            }
            else
            {
                used_min = values[x][y];
                used_max = values[x][y];
            }

            if( !done[x][y] &&
                is_boundary_pixel( sizes[a1], sizes[a2], values,
                                   used_min, used_max, x, y ) )
            {
                extract_contour( sizes[a1], sizes[a2], values,
                                 used_min, used_max, done, x, y, &contour );
                ADD_ELEMENT_TO_ARRAY( *contours, n_contours, contour, 1 );
            }
        }
    }

    FREE2D( done );
    FREE2D( values );

    return( n_contours );
}
