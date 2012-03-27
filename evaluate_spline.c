#include  <bicpl.h>
#include  <volume_io.h>

extern  void   set_cubic_basis_function_vals(
                      float kf_frame_id[], int i, int  num_intervals, float t );
extern  float  cubic_approximate_unival(float kf_cvs[],int i,
                                        int num_intervals);

private  void  usage(
    STRING   executable )
{
    static  STRING  usage = "\
Usage: %s  b n_cvs knot1 knot2 ... knot_n_cvs-1\n\
                      cvs1 cvs2  ... cvs_n_cvs-1\n\
              u_pos\n\n";

    print_error( usage, executable );
}

private  BOOLEAN  get_float_argument(
    Real   def,
    float  *value )
{
    BOOLEAN status;
    Real    rvalue;

    status = get_real_argument( def, &rvalue );

    *value = (float) rvalue;

    return( status );
}

int  main(
    int    argc,
    char   *argv[] )
{
    STRING        spline_name;
    int           i, n_cvs, interval;
    float         *knots, *cvs, value, u_pos;
    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &spline_name ) ||
        !get_int_argument( 0, &n_cvs ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    ALLOC( knots, n_cvs );
    ALLOC( cvs, n_cvs );

    for_less( i, 0, n_cvs )
    {
        if( !get_float_argument( 0.0, &knots[i] ) )
        {
            usage( argv[0] );
            return( 1 );
        }
    }

    for_less( i, 0, n_cvs )
    {
        if( !get_float_argument( 0.0, &cvs[i] ) )
        {
            usage( argv[0] );
            return( 1 );
        }
    }

    if( !get_float_argument( 0.0, &u_pos ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    interval = 0;
    while( interval < n_cvs-1 && u_pos >= knots[interval+1] )
        ++interval;

    set_cubic_basis_function_vals( knots, interval, n_cvs-1, u_pos );

    value = cubic_approximate_unival( cvs, interval, n_cvs-1 );

    print( "%g\n", value );

    return( 0 );
}
