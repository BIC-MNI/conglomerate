#include  <volume_io.h>
#include  <deform.h>
#include  <bicpl/numerical.h>

#define  MAX_DERIVS   2

private  void   get_voxel_coefs(
    int     degrees_continuity,
    Real    voxel[],
    Real    coefs[MAX_DERIVS+2][MAX_DERIVS+2][MAX_DERIVS+2] );
private  void   make_coefs_uv(
    int     degrees_continuity,
    Real    voxels[MAX_DERIVS+2],
    Real    interpolation_coefs[MAX_DERIVS+2][MAX_DERIVS+2],
    Real    coefs[MAX_DERIVS+2] );
private  void   make_coefs_u(
    int     degrees_continuity,
    Real    coefs_uv[MAX_DERIVS+2][MAX_DERIVS+2],
    Real    interpolation_coefs[MAX_DERIVS+2][MAX_DERIVS+2],
    Real    coefs_u[MAX_DERIVS+2][MAX_DERIVS+2] );
private  void  find_coefs_of_line(
    int         degrees_continuity,
    Real        coefs[MAX_DERIVS+2][MAX_DERIVS+2][MAX_DERIVS+2],
    Real        ou,
    Real        ov,
    Real        ow,
    Real        du,
    Real        dv,
    Real        dw,
    Real        line_coefs[(MAX_DERIVS+1) * N_DIMENSIONS+1] );

private  void  get_voxel_line_tricubic(
    Real        coefs[],
    int         x,
    int         y,
    int         z,
    Real        line_origin[],
    Real        line_direction[],
    Real        line_poly[] )
{
    Real  du, dv, dw, ou, ov, ow, delta0, delta1, delta0t, delta1t, delta0tt;
    Real  d00, d01, d10, d11;
    Real  c00a, c01a, c10a, c11a;
    Real  c00t, c01t, c10t, c11t;
    Real  c0a, c1a, c0t, c1t, c0tt, c1tt;

    d00 = coefs[1] - coefs[0];
    d01 = coefs[3] - coefs[2];
    d10 = coefs[5] - coefs[4];
    d11 = coefs[7] - coefs[6];

    du = line_direction[X];
    dv = line_direction[Y];
    dw = line_direction[Z];
    ou = line_origin[X] - (Real) x;
    ov = line_origin[Y] - (Real) y;
    ow = line_origin[Z] - (Real) z;

    c00a = coefs[0] + ow * d00;
    c01a = coefs[2] + ow * d01;
    c10a = coefs[4] + ow * d10;
    c11a = coefs[6] + ow * d11;
    c00t = dw * d00;
    c01t = dw * d01;
    c10t = dw * d10;
    c11t = dw * d11;

    delta0 = c01a - c00a;
    delta1 = c11a - c10a;
    delta0t = c01t - c00t;
    delta1t = c11t - c10t;

    c0a = c00a + ov * delta0;
    c0t = c00t + ov * delta0t + dv * delta0;
    c0tt = dv * delta0t;
    c1a = c10a + ov * delta1;
    c1t = c10t + ov * delta1t + dv * delta1;
    c1tt = dv * delta1t;

    delta0 = c1a - c0a;
    delta0t = c1t - c0t;
    delta0tt = c1tt - c0tt;

    line_poly[0] = c0a + ou * delta0;
    line_poly[1] = c0t + ou * delta0t + du * delta0;
    line_poly[2] = c0tt + ou * delta0tt + du * delta0t;
    line_poly[3] = du * delta0tt;
}

public  int  find_voxel_line_polynomial(
    Real        coefs[],
    int         degrees_continuity,
    int         x,
    int         y,
    int         z,
    Real        line_origin[],
    Real        line_direction[],
    Real        line_poly[] )
{
    Real  du, dv, dw, ou, ov, ow, voxel_offset;
    Real  voxel_coefs[MAX_DERIVS+2][MAX_DERIVS+2][MAX_DERIVS+2];

    if( degrees_continuity == 0 )
    {
        get_voxel_line_tricubic( coefs, x, y, z, line_origin,
                                 line_direction, line_poly );

        return( 4 );
    }

    get_voxel_coefs( degrees_continuity, coefs, voxel_coefs );

    if( degrees_continuity % 2 == 1 )
        voxel_offset = -0.5;
    else
        voxel_offset = 0.0;

    du = line_direction[X];
    dv = line_direction[Y];
    dw = line_direction[Z];
    ou = line_origin[X] - ((Real) x + voxel_offset);
    ov = line_origin[Y] - ((Real) y + voxel_offset);
    ow = line_origin[Z] - ((Real) z + voxel_offset);

    find_coefs_of_line( degrees_continuity, voxel_coefs, ou, ov, ow, du, dv, dw,
                        line_poly );

    return( (degrees_continuity + 1) * 3 + 1 );
}

#define  N_STEPS  8

private  int   get_cubic_root(
    Real  coefs[],
    Real  u_min,
    Real  u_max,
    Real  *solution )
{
#ifndef OLD
    int    n;
    Real   sol[3];
    n = get_roots_of_polynomial( 4, coefs, u_min, u_max, 0.0, sol );
    if( n > 0 )
    {
        n = 1;
        *solution = sol[0];
    }
    return( n );
#else
    int   i;
    Real  value, prev_value, h, hh, u, alpha;
    Real  h1, h2, h3;

    u = u_min;
    h = (u_max - u_min) / (Real) N_STEPS;
    value = evaluate_polynomial( 4, coefs, u );

    hh = h * h;
    h1 = h*(coefs[3]*(3.0*u*(h+u) + hh) + coefs[2]*(2.0*u+h) + coefs[1]);
    h2 = 2.0 * hh * (3.0*coefs[3]*(u+h) + coefs[2]);
    h3 = 6.0*coefs[3]*h*hh;

    for_less( i, 0, N_STEPS )
    {
        prev_value = value;
        value += h1;
        h1 += h2;
        h2 += h3;

        if( prev_value * value <= 0.0 )
        {
            if( prev_value == 0.0 && value == 0.0 )
                alpha = ((Real) i + 0.5) / (Real) N_STEPS;
            else
            {
                alpha = ((Real) i + prev_value / (prev_value - value)) /
                        (Real) N_STEPS;
            }

            *solution = (1.0 - alpha) * u_min + alpha * u_max;
            return( 1 );
        }
    }

    return( 0 );
#endif
}

public  int  find_voxel_line_value_intersection(
    Real        coefs[],
    int         degrees_continuity,
    int         x,
    int         y,
    int         z,
    Real        line_origin[],
    Real        line_direction[],
    Real        t_min,
    Real        t_max,
    Real        isovalue,
    Real        distances[3] )
{
    Real  line_coefs[(MAX_DERIVS+1) * N_DIMENSIONS+1];
    int   degrees, n_intersections;

    degrees = find_voxel_line_polynomial( coefs, degrees_continuity,
                                          x, y, z, line_origin, line_direction,
                                          line_coefs );

    n_intersections = 0;

    if( degrees > 0 )
    {
        line_coefs[0] -= isovalue;
        if( degrees == 4 )
        {
            n_intersections = get_cubic_root( line_coefs,
                                              t_min, t_max, &distances[0] );
        }
        else
        {
            n_intersections = get_roots_of_polynomial( degrees,
                                     line_coefs, t_min, t_max, .01, distances );
        }
    }

    return( n_intersections );
}

private  void   get_voxel_coefs(
    int     degrees_continuity,
    Real    voxel[],
    Real    coefs[MAX_DERIVS+2][MAX_DERIVS+2][MAX_DERIVS+2] )
{
    int    u, v, du, dv, dw, i, j, ind;
    Real   val, **bases;
    Real   interpolation_coefs[MAX_DERIVS+2][MAX_DERIVS+2];
    Real   coefs_uv[MAX_DERIVS+2][MAX_DERIVS+2][MAX_DERIVS+2];
    Real   coefs_u[MAX_DERIVS+2][MAX_DERIVS+2][MAX_DERIVS+2];

    ALLOC2D( bases, 4, 4 );

    switch( degrees_continuity )
    {
    case 0:
        get_linear_spline_coefs( bases );
        break;

    case 1:
        get_quadratic_spline_coefs( bases );
        break;

    case 2:
        get_cubic_spline_coefs( bases );
        break;
    }

    for_less( i, 0, degrees_continuity + 2 )
    {
        for_less( j, 0, degrees_continuity + 2 )
            interpolation_coefs[i][j] = bases[i][j];
    }

    FREE2D( bases );

    ind = 0;
    for_less( u, 0, degrees_continuity + 2 )
    {
        for_less( v, 0, degrees_continuity + 2 )
        {
            make_coefs_uv( degrees_continuity, &voxel[ind],
                            interpolation_coefs, coefs_uv[u][v] );
            ind += degrees_continuity + 2;
        }
    }

    for_less( u, 0, degrees_continuity + 2 )
    {
        make_coefs_u( degrees_continuity, coefs_uv[u],
                       interpolation_coefs, coefs_u[u] );
    }

    for_less( du, 0, degrees_continuity + 2 )
    {
        for_less( dv, 0, degrees_continuity + 2 )
        {
            for_less( dw, 0, degrees_continuity + 2 )
            {
                val = 0.0;
                
                for_less( u, 0, degrees_continuity + 2 )
                    val += coefs_u[u][dv][dw] * interpolation_coefs[du][u];

                coefs[du][dv][dw] = val;
            }
        }
    }
}

private  void   make_coefs_uv(
    int     degrees_continuity,
    Real    voxels[MAX_DERIVS+2],
    Real    interpolation_coefs[MAX_DERIVS+2][MAX_DERIVS+2],
    Real    coefs[MAX_DERIVS+2] )
{
    Real  val;
    int   dw, w;

    for_less( dw, 0, degrees_continuity + 2 )
    {
        val = 0.0;
        for_less( w, 0, degrees_continuity + 2 )
            val += voxels[w] * interpolation_coefs[dw][w];
        coefs[dw] = val;
    }
}

private  void   make_coefs_u(
    int     degrees_continuity,
    Real    coefs_uv[MAX_DERIVS+2][MAX_DERIVS+2],
    Real    interpolation_coefs[MAX_DERIVS+2][MAX_DERIVS+2],
    Real    coefs_u[MAX_DERIVS+2][MAX_DERIVS+2] )
{
    Real  val;
    int   v, dv, dw;

    for_less( dv, 0, degrees_continuity + 2 )
    {
        for_less( dw, 0, degrees_continuity + 2 )
        {
            val = 0.0;
            for_less( v, 0, degrees_continuity + 2 )
                val += coefs_uv[v][dw] * interpolation_coefs[dv][v];
            coefs_u[dv][dw] = val;
        }
    }
}

private  void   add_components(
    int   du,
    int   dv,
    int   dw,
    Real  coef,
    Real  line[2][N_DIMENSIONS],
    Real  line_coefs[(MAX_DERIVS+1) * N_DIMENSIONS+1] )
{
    int   u1, u2, u3, v1, v2, v3, w1, w2, w3;
    Real  u1f, u2f, u3f, v1f, v2f, v3f, w1f, w2f, w3f;

    for_less( u1, 0, 2 )
    {
        if( du < 1 )
        {
            if( u1 == 1 )
                break;
            u1f = coef;
        }
        else
            u1f = coef * line[u1][X];
    for_less( u2, 0, 2 )
    {
        if( du < 2 )
        {
            if( u2 == 1 )
                break;
            u2f = u1f;
        }
        else
            u2f = u1f * line[u2][X];
    for_less( u3, 0, 2 )
    {
        if( du < 3 )
        {
            if( u3 == 1 )
                break;
            u3f = u2f;
        }
        else
            u3f = u2f * line[u3][X];
    for_less( v1, 0, 2 )
    {
        if( dv < 1 )
        {
            if( v1 == 1 )
                break;
            v1f = u3f;
        }
        else
            v1f = u3f * line[v1][Y];
    for_less( v2, 0, 2 )
    {
        if( dv < 2 )
        {
            if( v2 == 1 )
                break;
            v2f = v1f;
        }
        else
            v2f = v1f * line[v2][Y];
    for_less( v3, 0, 2 )
    {
        if( dv < 3 )
        {
            if( v3 == 1 )
                break;
            v3f = v2f;
        }
        else
            v3f = v2f * line[v3][Y];
    for_less( w1, 0, 2 )
    {
        if( dw < 1 )
        {
            if( w1 == 1 )
                break;
            w1f = v3f;
        }
        else
            w1f = v3f * line[w1][Z];
    for_less( w2, 0, 2 )
    {
        if( dw < 2 )
        {
            if( w2 == 1 )
                break;
            w2f = w1f;
        }
        else
            w2f = w1f * line[w2][Z];
    for_less( w3, 0, 2 )
    {
        if( dw < 3 )
        {
            if( w3 == 1 )
                break;
            w3f = w2f;
        }
        else
            w3f = w2f * line[w3][Z];
        
        line_coefs[u1+u2+u3+v1+v2+v3+w1+w2+w3] += w3f;
    }
    }
    }
    }
    }
    }
    }
    }
    }
}

private  void  find_coefs_of_line(
    int         degrees_continuity,
    Real        coefs[MAX_DERIVS+2][MAX_DERIVS+2][MAX_DERIVS+2],
    Real        ou,
    Real        ov,
    Real        ow,
    Real        du,
    Real        dv,
    Real        dw,
    Real        line_coefs[(MAX_DERIVS+1) * N_DIMENSIONS+1] )
{
    int   deg, u_deg, v_deg, w_deg;
    Real  line[2][N_DIMENSIONS];

    line[0][0] = ou;
    line[0][1] = ov;
    line[0][2] = ow;
    line[1][0] = du;
    line[1][1] = dv;
    line[1][2] = dw;

    for_less( deg, 0, (degrees_continuity+1) * N_DIMENSIONS+1 )
        line_coefs[deg] = 0.0;

    for_less( u_deg, 0, degrees_continuity + 2 )
    {
        for_less( v_deg, 0, degrees_continuity + 2 )
        {
            for_less( w_deg, 0, degrees_continuity + 2 )
            {
                add_components( u_deg, v_deg, w_deg, coefs[u_deg][v_deg][w_deg],
                                line, line_coefs );
            }
        }
    }
}
