#include <stdio.h>
#include <math.h>

int  main(
    int  argc,
    char *argv[] )
{
    int      a, n, m, i;
    double   scale, expon, sum_neg, sum_pos, diff;
    double   *w, *correct;

    a = 1;
    if( a >= argc || sscanf( argv[a], "%d", &n ) != 1 )
        n = 100;
    ++a;

    if( a >= argc || sscanf( argv[a], "%d", &m ) != 1 )
        m = 100;
    ++a;

    if( a >= argc || sscanf( argv[a], "%lf", &scale ) != 1 )
        scale = 0.9;
    ++a;

    if( a >= argc || sscanf( argv[a], "%lf", &expon ) != 1 )
        expon = 0.01;
    ++a;

    w = new double [n];
    correct = new double [n];

    for( i = 0;  i < n;  ++i )
        w[i] = 0.0;
    w[n/2] = 1.0;

    apply_filter( w, n, m, scale );
    apply_gaussian( correct, n, expon );

    sum_pos = 0.0;
    sum_neg = 0.0;
    for( i = 0;  i < n;  ++i )
    {
        diff = w[i] - correct[i];
        if( diff > 0.0 )
            sum_pos += diff;
        else
            sum_neg += -diff;
    }

    (void) printf( "Error: %g %g\n", sum_pos, sum_diff );
    
    delete [] w;
    delete [] correct;
}
