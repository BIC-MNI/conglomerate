#include  <volume_io.h>
#include  <bicpl.h>

int  main(
    int  argc,
    char *argv[] )
{
    int   n_stations, n_positions, station, pos, i_next, i_curr;
    int   i, j, swap_int, best, *sorted;
    int   sum_pos, sum_total;
    Real  **stations, weight, dx, *current_cost, *next_cost, *swap;
    Real  best_cost, cost, threshold;

    initialize_argument_processing( argc, argv );

    (void) get_int_argument( 10, &n_stations );
    (void) get_int_argument( 10, &n_positions );
    (void) get_real_argument( 1.0, &weight );

    ALLOC2D( stations, n_stations, n_positions );

    set_random_seed( 13244231 );

    for_less( station, 0, n_stations )
    for_less( pos, 0, n_positions )
        stations[station][pos] = get_random_0_to_1();

    ALLOC( current_cost, n_positions );
    ALLOC( next_cost, n_positions );
    ALLOC( sorted, n_positions );

    for_less( pos, 0, n_positions )
        next_cost[pos] = stations[0][pos];

    sum_pos = 0;
    sum_total = 0;
    for_less( station, 1, n_stations )
    {
        swap = current_cost;
        current_cost = next_cost;
        next_cost = swap;

        for_less( pos, 0, n_positions )
            sorted[pos] = pos;

        for_less( i, 0, n_positions-1 )
        {
            best = i;
            for_less( j, i+1, n_positions )
            {
                if( current_cost[sorted[j]] < current_cost[sorted[best]] )
                    best = j;
            }
            swap_int = sorted[i];
            sorted[i] = sorted[best];
            sorted[best] = swap_int;
        }

        for_less( i_next, 0, n_positions )
        {
            Real  test_best;

            /*---------------------- */

            threshold = current_cost[sorted[n_positions-1]];
            pos = 0;
            test_best = 0.0;
            while( pos < n_positions && current_cost[sorted[pos]] <= threshold )
            {
                i_curr = sorted[pos];
                dx = (Real) (i_next - i_curr);
                cost = current_cost[i_curr] + weight * dx * dx +
                       stations[station][i_next];
                if( pos == 0 || cost < test_best )
                {
                    test_best = cost;
                    threshold = current_cost[i_curr] + weight * dx * dx;
                }
                ++pos;
            }

            sum_pos += pos;
            sum_total += n_positions;

#ifdef NO
            /*---------------------- */

            best_cost = 0.0;
            for_less( i_curr, 0, n_positions )
            {
                dx = (Real) (i_next - i_curr);
                cost = current_cost[i_curr] + weight * dx * dx +
                       stations[station][i_next];
                if( i_curr == 0 || cost < best_cost )
                    best_cost = cost;
            }

            if( test_best != best_cost )
                handle_internal_error( "Here" );

            next_cost[i_next] = best_cost;
#else
            next_cost[i_next] = test_best;
#endif
        }
    }

    print( "%g\n", (Real) sum_pos / (Real) sum_total );

    best_cost = 0.0;
    for_less( pos, 0, n_positions )
    {
        if( pos == 0 || next_cost[pos] < best_cost )
            best_cost = next_cost[pos];
    }
 
    FREE( current_cost );
    FREE( next_cost );
    FREE2D( stations );

    print( "Cost: %g\n", best_cost );

    return( 0 );
}
