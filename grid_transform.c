#include  <bicpl.h>
#include  <volume_io/internal_volume_io.h>

typedef struct
{
    int   i;
    int   j;
    Real  x_pos;
    Real  y_pos;
} fixed_struct;

private  void  get_fixed_pos(
    int            n_fixed,
    fixed_struct   fixed_points[],
    int            i,
    int            j,
    Real           *x,
    Real           *y );

private  void  find_minimum(
    int            x_grid_size,
    int            y_grid_size,
    int            n_fixed,
    fixed_struct   fixed_points[],
    int            **param_index,
    Real           parameters[] );

int  main(
    int  argc,
    char *argv[] )
{
    STRING           filename;
    int              n_fixed, x_grid_size, y_grid_size, i_grid, j_grid;
    int              **param_index, ind, i, j, n_moving_grid;
    Real             x_pos, y_pos, x, y;
    fixed_struct     f, *fixed_points;
    Smallest_int     **grid_fixed_flags;
    Point            point;
    Vector           normal;
    Real             *parameters;
    object_struct    *object;
    quadmesh_struct  *quadmesh;

    initialize_argument_processing( argc, argv );

    (void) get_string_argument( "test.obj", &filename );
    (void) get_int_argument( 10, &x_grid_size );
    (void) get_int_argument( 10, &y_grid_size );

    n_fixed = 0;
    fixed_points = NULL;

    while( get_int_argument( 0, &i_grid ) &&
           get_int_argument( 0, &j_grid ) &&
           get_real_argument( 0.0, &x_pos ) &&
           get_real_argument( 0.0, &y_pos ) )
    {
        f.i = i_grid;
        f.j = j_grid;
        f.x_pos = x_pos;
        f.y_pos = y_pos;
        ADD_ELEMENT_TO_ARRAY( fixed_points, n_fixed, f, DEFAULT_CHUNK_SIZE );
    }

    ALLOC2D( grid_fixed_flags, x_grid_size, y_grid_size ); 

    for_less( i, 0, x_grid_size )
    for_less( j, 0, y_grid_size )
    {
        grid_fixed_flags[i][j] = FALSE;
    }

    for_less( i, 0, n_fixed )
        grid_fixed_flags[fixed_points[i].i][fixed_points[i].j] = TRUE;

    n_moving_grid = x_grid_size * y_grid_size - n_fixed;
    ALLOC( parameters, 2 * n_moving_grid );
    ALLOC2D( param_index, x_grid_size, y_grid_size );

    ind = 0;
    for_less( i, 0, x_grid_size )
    for_less( j, 0, y_grid_size )
    {
        if( grid_fixed_flags[i][j] )
            param_index[i][j] = -1;
        else
        {
            param_index[i][j] = ind;
            ++ind;
        }
    }

    find_minimum( x_grid_size, y_grid_size, n_fixed, fixed_points,
                  param_index, parameters );

    object = create_object( QUADMESH );
    quadmesh = get_quadmesh_ptr( object );
    initialize_quadmesh( quadmesh, WHITE, NULL, x_grid_size, y_grid_size );

    fill_Vector( normal, 0.0, 0.0, 1.0 );

    ind = 0;
    for_less( i, 0, x_grid_size )
    for_less( j, 0, y_grid_size )
    {
        if( !grid_fixed_flags[i][j] )
        {
            x = parameters[2*ind];
            y = parameters[2*ind+1];
            ++ind;
        }
        else
        {
            get_fixed_pos( n_fixed, fixed_points, i, j, &x, &y );
        }

        fill_Point( point, x, y, 0.0 );

        set_quadmesh_point( quadmesh, i, j, &point, &normal );
    }

    (void) output_graphics_file( filename, BINARY_FORMAT, 1, &object );

    return( 0 );
}

private  void  get_fixed_pos(
    int            n_fixed,
    fixed_struct   fixed_points[],
    int            i,
    int            j,
    Real           *x,
    Real           *y )
{
    int  ind;

    for_less( ind, 0, n_fixed )
    {
        if( fixed_points[ind].i == i && fixed_points[ind].j == j )
        {
            break;
        }
    }

    if( ind >= n_fixed )
        handle_internal_error( "get_fixed_pos" );

    *x = fixed_points[ind].x_pos;
    *y = fixed_points[ind].y_pos;
}

private  void  find_minimum(
    int            x_grid_size,
    int            y_grid_size,
    int            n_fixed,
    fixed_struct   fixed_points[],
    int            **param_index,
    Real           parameters[] )
{
    int                    n_parameters, x, y, i, ind[3], coord, which;
    int                    dx, dy, hor;
    Real                   c, weights[3], pos[2];
    linear_least_squares   lsq;

    n_parameters = 2 * (x_grid_size * y_grid_size - n_fixed);

    initialize_linear_least_squares( &lsq, n_parameters );

    for_less( i, 0, n_parameters )
        parameters[i] = 0.0;

    weights[0] = 0.5;
    weights[1] = -1.0;
    weights[2] = 0.5;

    for_less( x, 0, x_grid_size )
    {
        for_less( y, 0, y_grid_size )
        {
            for_less( hor, 0, 2 )  /* do squared residual of x or y component */
            {
                if( hor == 0 )
                {
                    dx = 1;
                    dy = 0;
                }
                else
                {
                    dx = 0;
                    dy = 1;
                }

                if( x + dx * 2 >= x_grid_size ||   /* at edge of grid */
                    y + dy * 2 >= y_grid_size )
                    continue;

                ind[0] = param_index[x][y];           /* find parameter index */
                ind[1] = param_index[x+1*dx][y+1*dy];
                ind[2] = param_index[x+2*dx][y+2*dy];

                for_less( coord, 0, 2 )
                {
                    c = 0.0;
                    for_less( which, 0, 3 )
                    {
                        if( ind[which] >= 0 )
                            parameters[2*ind[which]+coord] += weights[which];
                        else
                        {
                            get_fixed_pos( n_fixed, fixed_points,
                                           x+which*dx, y+which*dy,
                                           &pos[0], &pos[1] );
                            c -= weights[which] * pos[coord];
                        }
                    }

                    add_to_linear_least_squares( &lsq, parameters, c );

                    for_less( which, 0, 3 )   /* reset weights */
                    {
                        if( ind[which] >= 0 )
                            parameters[2*ind[which]+coord] = 0.0;
                    }

                }
            }
        }
    }


    if( !get_linear_least_squares_solution( &lsq, parameters ) )
        handle_internal_error( "find_minimum" );

    delete_linear_least_squares( &lsq );
}
