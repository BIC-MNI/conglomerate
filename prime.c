#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  faster_primes(
    int           n,
    Smallest_int  prime_flags[] );

private  void  brute_force_primes(
    int           n,
    Smallest_int  prime_flags[] );

int  main(
    int   argc,
    char  *argv[] )
{
    Real          start_time, end_time;
    int           n, method, i, n_primes;
    Smallest_int  *prime_flags;

    initialize_argument_processing( argc, argv );

    (void) get_int_argument( 1000, &n );
    (void) get_int_argument( 0, &method );

    ALLOC( prime_flags, n );

    start_time = current_cpu_seconds();

    if( method == 0 )
        brute_force_primes( n, prime_flags );
    else
        faster_primes( n, prime_flags );

    end_time = current_cpu_seconds();

    n_primes = 0;

    for_less( i, 1, n )
    {
        if( prime_flags[i] )
            ++n_primes;
    }

    print( "n=%d    n_primes=%d Time: %g\n", n, n_primes, end_time - start_time );

    FREE( prime_flags );

    return( 0 );
}

private  void  brute_force_primes(
    int           n,
    Smallest_int  prime_flags[] )
{
    int   i, j, max_x;

    for_less( i, 1, n )
        prime_flags[i] = 1;

    max_x = (int) (sqrt( n ) + 1.0);

    for_inclusive( i, 2, max_x )
    {
        for( j = 2*i;  j < n;  j += i )
            prime_flags[j] = 0;
    }
}

private  void  faster_primes(
    int           n,
    Smallest_int  prime_flags[] )
{
    int   *next_prime, max;
    int   i, j, p, last_prime, max_x;

    ALLOC( next_prime, n );

    for_less( i, 1, n )
        prime_flags[i] = 1;

    for_less( i, 1, n )
    {
        next_prime[i] = i+1;
    }

    max_x = (int) (sqrt( n ) + 1.0);

    for_inclusive( i, 2, max_x )
    {
        if( !prime_flags[i] )
            continue;

        max = (n-1) / i;
        j = i;
        while( j <= max )
        {
            p = i * j;
            prime_flags[p] = 0;
            j = next_prime[j];
        }

        last_prime = 1;
        j = next_prime[1];
        while( j < n )
        {
            if( prime_flags[j] )
            {
                next_prime[last_prime] = j;
                last_prime = j;
            }

            j = next_prime[j];
        }
    }

    FREE( next_prime );
}
