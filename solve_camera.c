#include  <interval.h>
#include  <bicpl.h>
#include  <internal_volume_io.h>

private  BOOLEAN  solve_cameras(
    int       n_pairs,
    Real      (*image1)[2],
    Real      (*image2)[2],
    Real      error_in_position,
    Real      precision,
    Interval  *x_rot,
    Interval  *y_rot,
    Interval  *z_rot,
    Interval  *x_trans,
    Interval  *y_trans,
    Interval  *z_trans,
    Interval  *camera_dist );

private   void  usage(
    STRING  executable )
{
    static  STRING  usage = "\n\
Usage: %s \n\
\n\n";

    print_error( usage, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    int    n_pairs;
    Real   error_in_position, x1, y1, x2, y2, precision;
    Real   (*image1)[2], (*image2)[2];
    Interval  x_rot, y_rot, z_rot;
    Interval  x_trans, y_trans, z_trans;
    Interval  camera_distance;

    initialize_argument_processing( argc, argv );

    (void) get_real_argument( 0.0, &error_in_position );
    (void) get_real_argument( 0.0, &precision );
    n_pairs = 0;

    while( get_real_argument( 0.0, &x1 ) )
    {
        if( !get_real_argument( 0.0, &y1 ) ||
            !get_real_argument( 0.0, &x2 ) ||
            !get_real_argument( 0.0, &y2 ) )
        {
            usage( argv[0] );
            return( 0 );
        }

        SET_ARRAY_SIZE( image1, n_pairs, n_pairs+1, DEFAULT_CHUNK_SIZE );
        SET_ARRAY_SIZE( image2, n_pairs, n_pairs+1, DEFAULT_CHUNK_SIZE );

        image1[n_pairs][0] = x1;
        image1[n_pairs][1] = y1;
        image2[n_pairs][0] = x2;
        image2[n_pairs][1] = y2;
        ++n_pairs;
    }

    print( "N pairs: %d\n", n_pairs );

    if( solve_cameras( n_pairs, image1, image2, error_in_position, precision,
                   &x_rot, &y_rot, &z_rot,
                   &x_trans, &y_trans, &z_trans, 
                   &camera_distance ) )
    {
        print( "X rot: %g %g\n", INTERVAL_MIN(x_rot) * 180.0 / PI,
                                 INTERVAL_MAX(x_rot) * 180.0 / PI );
        print( "Y rot: %g %g\n", INTERVAL_MIN(y_rot) * 180.0 / PI,
                                 INTERVAL_MAX(y_rot) * 180.0 / PI );
        print( "Z rot: %g %g\n", INTERVAL_MIN(z_rot) * 180.0 / PI,
                                 INTERVAL_MAX(z_rot) * 180.0 / PI );
        print( "X trans: %g %g\n", INTERVAL_MIN(x_trans), INTERVAL_MAX(x_trans) );
        print( "Y trans: %g %g\n", INTERVAL_MIN(y_trans), INTERVAL_MAX(y_trans) );
        print( "Z trans: %g %g\n", INTERVAL_MIN(z_trans), INTERVAL_MAX(z_trans) );
        print( "Camera dist: %g %g\n", INTERVAL_MIN(camera_distance),
                                       INTERVAL_MAX(camera_distance) );
    }
            
    return( 0 );
}

#define  N_PARMS   7

private  void  transform_interval_point(
    Interval  parms[],
    Interval  *x,
    Interval  *y,
    Interval  *z,
    Interval  *xt,
    Interval  *yt,
    Interval  *zt )
{
    Interval   c, s, cx, sx, cy, sy, cz, sz;
    *xt = *x;
    *yt = *y;
    *zt = *z;

    /*--- x rot ----------- */

    COS_INTERVAL( c, parms[0] );
    COS_INTERVAL( s, parms[0] );

    MULT_INTERVALS( cy, c, *yt );
    MULT_INTERVALS( sy, s, *yt );
    MULT_INTERVALS( cz, c, *zt );
    MULT_INTERVALS( sz, s, *zt );

    SUBTRACT_INTERVALS( *yt, cy, sz );
    ADD_INTERVALS(      *zt, sy, cz );

    /*--- y rot ----------- */

    COS_INTERVAL( c, parms[1] );
    COS_INTERVAL( s, parms[1] );

    MULT_INTERVALS( cz, c, *zt );
    MULT_INTERVALS( sz, s, *zt );
    MULT_INTERVALS( cx, c, *xt );
    MULT_INTERVALS( sx, s, *xt );

    SUBTRACT_INTERVALS( *zt, cz, sx );
    ADD_INTERVALS     ( *xt, sz, cx );

    /*--- z rot ----------- */

    COS_INTERVAL( c, parms[2] );
    COS_INTERVAL( s, parms[2] );

    MULT_INTERVALS( cx, c, *xt );
    MULT_INTERVALS( sx, s, *xt );
    MULT_INTERVALS( cy, c, *yt );
    MULT_INTERVALS( sy, s, *yt );

    SUBTRACT_INTERVALS( *xt, cx, sy );
    ADD_INTERVALS(      *yt, sx, cy );

    ADD_INTERVALS( *xt, *xt, parms[3] );
    ADD_INTERVALS( *yt, *yt, parms[4] );
    ADD_INTERVALS( *zt, *zt, parms[5] );
}

private  BOOLEAN  rays_intersect(
    Interval  *ox1,
    Interval  *oy1,
    Interval  *oz1,
    Interval  *dx1,
    Interval  *dy1,
    Interval  *dz1,
    Interval  *ox2,
    Interval  *oy2,
    Interval  *oz2,
    Interval  *dx2,
    Interval  *dy2,
    Interval  *dz2 )
{
    Interval  cx, cy, cz, c1, c2, c3, dot, dx, dy, dz;

    MULT_INTERVALS( c1, *dy1, *dz2 );
    MULT_INTERVALS( c2, *dz1, *dy2 );
    SUBTRACT_INTERVALS( cx, c1, c2 );

    MULT_INTERVALS( c1, *dz1, *dx2 );
    MULT_INTERVALS( c2, *dx1, *dz2 );
    SUBTRACT_INTERVALS( cy, c1, c2 );

    MULT_INTERVALS( c1, *dx1, *dy2 );
    MULT_INTERVALS( c2, *dy1, *dx2 );
    SUBTRACT_INTERVALS( cz, c1, c2 );

    SUBTRACT_INTERVALS( dx, *ox1, *ox2 );
    SUBTRACT_INTERVALS( dy, *oy1, *oy2 );
    SUBTRACT_INTERVALS( dz, *oz1, *oz2 );

    MULT_INTERVALS( c1, cx, dx );
    MULT_INTERVALS( c2, cy, dy );
    MULT_INTERVALS( c3, cz, dz );

    ADD_INTERVALS( dot, c1, c2 );
    ADD_INTERVALS( dot, dot, c3 );

    return( INTERVAL_CONTAINS( dot, 0.0 ) );
}

private  BOOLEAN  is_solution(
    int       n_pairs,
    Interval  (*interval_image1)[2],
    Interval  (*interval_image2)[2],
    Interval  parms[] )
{
    int       p;
    Interval  origin, xo2, yo2, zo2, xv2, yv2, zv2, persp_dist;

    SET_INTERVAL( origin, 0.0, 0.0 );

    transform_interval_point( parms, &origin, &origin, &origin,
                              &xo2, &yo2, &zo2 );

    MULT_INTERVAL_REAL( persp_dist, parms[6], -1.0 );

    for_less( p, 0, n_pairs )
    {
        transform_interval_point( parms, &interval_image2[p][0],
                                  &interval_image2[p][1], &persp_dist,
                                  &xv2, &yv2, &zv2 );
        if( !rays_intersect( &origin, &origin, &origin,
                             &interval_image1[p][0], &interval_image1[p][1],
                             &persp_dist,
                             &xo2, &yo2, &zo2, &xv2, &yv2, &zv2 ) )
            return( FALSE );
    }

    return( TRUE );
}

#define  N_PROGRESS  10000

private  void  update_progress(
    progress_struct  *progress,
    Real             size,
    Real             *done )
{
    int   prev_which, next_which;

    prev_which = ROUND( (Real) N_PROGRESS * (*done) );
    *done += size;
    next_which = ROUND( (Real) N_PROGRESS * (*done) );

    if( next_which != prev_which )
        update_progress_report( progress, next_which );
}

private  void  recursive_solve(
    progress_struct  *progress,
    Real             size,
    Real             *done,
    int       n_pairs,
    Interval  (*interval_image1)[2],
    Interval  (*interval_image2)[2],
    int       next_dim,
    Interval  parms[],
    Real      precision,
    BOOLEAN   *found,
    Interval  solution[] )
{
    int   dim, p, prev_dim;
    Real  interval_size, save, midpoint;

    if( !is_solution( n_pairs, interval_image1, interval_image2, parms ) )
    {
        update_progress( progress, size, done );
        return;
    }

    prev_dim = (next_dim - 1 + N_PARMS) % N_PARMS;
    dim = prev_dim;

    do
    {
        dim = (dim + 1) % N_PARMS;
        interval_size = INTERVAL_SIZE( parms[dim] );
    }
    while( dim != prev_dim && interval_size <= precision );

    if( interval_size <= precision )
    {
        if( !(*found) )
        {
            *found = TRUE;
            for_less( p, 0, N_PARMS )
                solution[p] = parms[p];
        }
        else
        {
            for_less( p, 0, N_PARMS )
            {
                SET_INTERVAL( solution[p], MIN(INTERVAL_MIN(solution[p]),
                                               INTERVAL_MIN(parms[p])),
                                           MAX(INTERVAL_MAX(solution[p]),
                                               INTERVAL_MAX(parms[p])) );
            }
        }
        update_progress( progress, size, done );
        return;
    }

    midpoint = INTERVAL_MIDPOINT( parms[dim] );

    save = INTERVAL_MAX( parms[dim] );
    SET_INTERVAL_MAX( parms[dim], midpoint );
    recursive_solve( progress, size / 2.0, done,
                     n_pairs, interval_image1, interval_image2, (dim+1)%N_PARMS,
                     parms, precision, found, solution );
    SET_INTERVAL_MAX( parms[dim], save );

    save = INTERVAL_MIN( parms[dim] );
    SET_INTERVAL_MIN( parms[dim], midpoint );
    recursive_solve( progress, size / 2.0, done,
                     n_pairs, interval_image1, interval_image2, (dim+1)%N_PARMS,
                     parms, precision, found, solution );
    SET_INTERVAL_MIN( parms[dim], save );
}

private  BOOLEAN  solve_cameras(
    int       n_pairs,
    Real      (*image1)[2],
    Real      (*image2)[2],
    Real      error_in_position,
    Real      precision,
    Interval  *x_rot,
    Interval  *y_rot,
    Interval  *z_rot,
    Interval  *x_trans,
    Interval  *y_trans,
    Interval  *z_trans,
    Interval  *camera_dist )
{
    Interval  (*interval_image1)[2], (*interval_image2)[2];
    Interval  parms[N_PARMS], solution[N_PARMS];
    int       p;
    BOOLEAN   solution_found;
    Real      done;
    progress_struct  progress;

    SET_INTERVAL( parms[0], 0.0, 10.0 / 180.0 * PI )
    SET_INTERVAL( parms[1], 0.0, 10.0 / 180.0 * PI )
    SET_INTERVAL( parms[2], 0.0, 10.0 / 180.0 * PI )
    SET_INTERVAL( parms[3], -10.0, 10.0 )
    SET_INTERVAL( parms[4], -10.0, 10.0 )
    SET_INTERVAL( parms[5], -10.0, 10.0 )
    SET_INTERVAL( parms[6], 1.0, 1.0 )

    ALLOC( interval_image1, n_pairs );
    ALLOC( interval_image2, n_pairs );

    for_less( p, 0, n_pairs )
    {
        SET_INTERVAL( interval_image1[p][0], image1[p][0] - error_in_position,
                                             image1[p][0] + error_in_position )
        SET_INTERVAL( interval_image1[p][1], image1[p][1] - error_in_position,
                                             image1[p][1] + error_in_position )
        SET_INTERVAL( interval_image2[p][0], image2[p][0] - error_in_position,
                                             image2[p][0] + error_in_position )
        SET_INTERVAL( interval_image2[p][1], image2[p][1] - error_in_position,
                                             image2[p][1] + error_in_position )
    }

    solution_found = FALSE;

    initialize_progress_report( &progress, FALSE, N_PROGRESS, "Solving" );

    done = 0.0;
    recursive_solve( &progress, 1.0, &done,
                     n_pairs, interval_image1, interval_image2, 0,
                     parms, precision, &solution_found, solution );

    terminate_progress_report( &progress );

    *x_rot = solution[0];
    *y_rot = solution[1];
    *z_rot = solution[2];
    *x_trans = solution[3];
    *y_trans = solution[4];
    *z_trans = solution[5];
    *camera_dist = solution[6];

    FREE( interval_image1 );
    FREE( interval_image2 );

    return( solution_found );
}
