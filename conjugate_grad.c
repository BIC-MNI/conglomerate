#include  <volume_io.h>
#include  <bicpl.h>
#include  <conjugate_grad.h>

struct  conjugate_grad_struct
{
    VIO_BOOL  first_call;
    int      n_parameters;
    VIO_Real     *g;
    VIO_Real     *h;
};

  void   reinitialize_conjugate_gradient(
    conjugate_grad   con )
{
    con->first_call = TRUE;
}

  conjugate_grad   initialize_conjugate_gradient(
    int       n_parameters )
{
    conjugate_grad  con;

    ALLOC( con, 1 );

    con->n_parameters = n_parameters;
    con->first_call = TRUE;

    if( n_parameters > 0 )
    {
        ALLOC( con->g, n_parameters );
        ALLOC( con->h, n_parameters );
    }

    return( con );
}

  void   delete_conjugate_gradient(
    conjugate_grad   con )
{
    if( con->n_parameters >= 0 )
    {
        FREE( con->g );
        FREE( con->h );
    }

    FREE( con );
}

  VIO_BOOL  get_conjugate_unit_direction(
    conjugate_grad   con,
    VIO_Real        derivative[],
    VIO_Real        unit_dir[] )
{
    int      p;
    VIO_Real     len, gg, dgg, gam;
    VIO_BOOL  found;

    found = TRUE;

    if( con->first_call )
    {
        con->first_call = FALSE;

        for_less( p, 0, con->n_parameters )
        {
            con->g[p] = -derivative[p];
            con->h[p] = -derivative[p];
            unit_dir[p] = -derivative[p];
        }
    }
    else
    {
        gg = 0.0;
        dgg = 0.0;
        for_less( p, 0, con->n_parameters )
        {
            gg += con->g[p] * con->g[p];
            dgg += (derivative[p] + con->g[p]) * derivative[p];
/*
            dgg += derivative[p] * derivative[p];
*/
        }

        if( gg != 0.0 )
            gam = dgg / gg;
        else
        {
            gam = 0.0;
            found = FALSE;
        }

        for_less( p, 0, con->n_parameters )
        {
            con->g[p] = -derivative[p];
            con->h[p] = con->g[p] + gam * con->h[p];
            unit_dir[p] = con->h[p];
        }
    }

    len = 0.0;
    for_less( p, 0, con->n_parameters )
        len += unit_dir[p] * unit_dir[p];

    if( len == 0.0 )
        return( FALSE );

    len = sqrt( len );

    for_less( p, 0, con->n_parameters )
        unit_dir[p] /= len;

    return( found );
}
