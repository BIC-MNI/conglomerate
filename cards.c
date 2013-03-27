#include  <mni.h>

typedef int  Card;

static Card get_card( char [] );
static  int  get_value(
    Card   card );
static  int  get_suit(
    Card  card );

static  VIO_Real  decide(
    int   n_cards,
    Card  hand[],
    int   n_iters );

static  int  get_bit(
    int     i,
    int     which )
{
    return( (i & (1 << which)) ? 1 : 0 );
}

static  VIO_Real  get_payoff(
    VIO_Real      payoff[2][2][2][2][2],
    int       i )
{
    int   i1, i2, i3, i4, i5;

    i1 = get_bit( i, 0 );
    i2 = get_bit( i, 1 );
    i3 = get_bit( i, 2 );
    i4 = get_bit( i, 3 );
    i5 = get_bit( i, 4 );

    return( payoff[i1][i2][i3][i4][i5] );
}

int main(
    int   argc,
    char  *argv[] )
{
    int       i, col;
    int       i1, i2, i3, i4, i5;
    int       n_iters;
    Card      cards[5], hand[5];
    int       sorted[32];
    VIO_Real      payoff[2][2][2][2][2];
    int       n_cards;

    n_cards = MIN( 5, argc - 1 );

    for_less( i, 0, n_cards )
        cards[i] = get_card( argv[i+1] );

    if( argc < 7 || sscanf( argv[6], "%d", &n_iters ) != 1 )
        n_iters = 100000;

    if( n_cards < 5 )
    {
        VIO_Real  gain;

        gain = decide( n_cards, cards, n_iters );

        print( "%g\n", gain );

        return( 0 );
    }

    for_less( i1, 0, 2 )
    for_less( i2, 0, 2 )
    for_less( i3, 0, 2 )
    for_less( i4, 0, 2 )
    for_less( i5, 0, 2 )
    {
        n_cards = 0;
        if( i1 == 1 )
            hand[n_cards++] = cards[0];
        if( i2 == 1 )
            hand[n_cards++] = cards[1];
        if( i3 == 1 )
            hand[n_cards++] = cards[2];
        if( i4 == 1 )
            hand[n_cards++] = cards[3];
        if( i5 == 1 )
            hand[n_cards++] = cards[4];

        payoff[i1][i2][i3][i4][i5] = decide( n_cards, hand, n_iters );
    }

    col = 0;

    for_less( i1, 0, 32 )
        sorted[i1] = i1;

    for_less( i1, 0, 31 )
    {
        int  best, tmp;

        best = i1;
        for_less( i2, i1+1, 32 )
        {
            if( get_payoff( payoff, sorted[i2] ) >
                get_payoff( payoff, sorted[best] ) )
                best = i2;
        }

        tmp = sorted[best];
        sorted[best] = sorted[i1];
        sorted[i1] = tmp;
    }

    for_less( i1, 0, 32 )
    {
        print( "\t[" );
        print( "%1c", " ^"[get_bit( sorted[i1], 0)] );
        print( "%1c", " ^"[get_bit( sorted[i1], 1)] );
        print( "%1c", " ^"[get_bit( sorted[i1], 2)] );
        print( "%1c", " ^"[get_bit( sorted[i1], 3)] );
        print( "%1c", " ^"[get_bit( sorted[i1], 4)] );
        print( "]:  %g", get_payoff( payoff, sorted[i1] ) );

        ++col;
        if( col % 2 == 0 )
            print( "\n" );
    }

    return( 0 );
}

static Card get_card(
    char str[] )
{
    int  value, suit;

    if( str[0] == 'a' || str[0] == 'A' )
        value = 14;
    else if( str[0] == 'j' || str[0] == 'J' )
        value = 11;
    else if( str[0] == 'q' || str[0] == 'Q' )
        value = 12;
    else if( str[0] == 'k' || str[0] == 'K' )
        value = 13;
    else
        (void) sscanf( str, "%d", &value );

    switch( str[strlen(str)-1] )
    {
    case 'c':
    case 'C':   suit = 0;  break;
    case 'd':
    case 'D':   suit = 1;  break;
    case 'h':
    case 'H':   suit = 2;  break;
    case 's':
    case 'S':   suit = 3;  break;
    default:
        print( "Invalid suit\n" );
    }

    return( (value-2) + suit * 13 );
}

static  int  get_value(
    Card   card )
{
    return( card % 13 + 2 );
}

static  int  get_suit(
    Card  card )
{
    return( card / 13 );
}

static int   n_in_pack;
static Card  pack[52];

static  Card  deal()
{
    int  i;
    Card card;

    i = get_random_int( n_in_pack );

    card = pack[i];
    --n_in_pack;
    if( i != n_in_pack )
    {
        pack[i] = pack[n_in_pack];
        pack[n_in_pack] = card;
    }

    return( card );
}

static  VIO_BOOL  is_hand_flush(
    Card  hand[] )
{
    return( get_suit( hand[0] ) == get_suit( hand[1] ) &&
            get_suit( hand[0] ) == get_suit( hand[2] ) &&
            get_suit( hand[0] ) == get_suit( hand[3] ) &&
            get_suit( hand[0] ) == get_suit( hand[4] ) );
}

static  VIO_BOOL  is_hand_straight(
    int   values[],
    int   *max_val )
{
    int  i, min_val, val;

    min_val = values[0];
    *max_val = min_val;
    for_less( i, 1, 5 )
    {
        val = values[i];
        if( val < min_val )
            min_val = val;
        else if( val > *max_val )
            *max_val = val;
    }

    return( *max_val - min_val == 4 );
}

static  int  count_pairs(
    Card   values[],
    int    pairs[2],
    int    *pair_value )
{
    VIO_BOOL  counted[5];
    int      i, j, n_pairs;

    for_less( i, 0, 5 )
        counted[i] = FALSE;

    n_pairs = 0;
    pairs[0] = 1;
    pairs[1] = 1;

    for_less( i, 0, 4 )
    {
        if( !counted[i] )
        {
            for_less( j, i+1, 5 )
            {
                if( values[i] == values[j] )
                {
                    *pair_value = values[i];
                    ++pairs[n_pairs];
                    counted[j] = TRUE;
                }
            }

            if( pairs[n_pairs] > 1 )
                ++n_pairs;
        }
    }

    return( n_pairs );
}

#ifndef LV
static  int  evaluate_hand(
    Card  hand[] )
{
    int      i, n_pairs, max_value, pairs[2], values[5], pair_value;
    VIO_BOOL  is_flush, is_straight;

    for_less( i, 0, 5 )
        values[i] = get_value( hand[i] );

    n_pairs = count_pairs( values, pairs, &pair_value );

    if( n_pairs == 0 )
    {
        is_flush = is_hand_flush( hand );
        is_straight = is_hand_straight( values, &max_value );

        if( is_flush )
        {
            if( is_straight )
            {
                if( max_value == 14 )
                    return( 200 );
                else
                    return( 128 );
            }
            else
                return( 16 );
        }
        else if( is_straight )
            return( 8 );
    }
    else if( n_pairs == 1 )
    {
        if( pairs[0] == 2 )
        {
            if( pair_value == 14 )
                return( 2 );
            else if( pair_value >= 10 )
                return( 1 );
        }
        else if( pairs[0] == 3 )
            return( 4 );
        else if( pairs[0] == 4 )
            return( 64 );
    }
    else
    {
        if( pairs[0] == 2 && pairs[1] == 2 )
            return( 3 );
        if( pairs[0] == 3 && pairs[1] == 2 ||
            pairs[0] == 2 && pairs[1] == 3 )
            return( 32 );
    }
    return( 0 );
}
#else
static  int  evaluate_hand(
    Card  hand[] )
{
    int      i, n_pairs, max_value, pairs[2], values[5], pair_value;
    VIO_BOOL  is_flush, is_straight;

    for_less( i, 0, 5 )
        values[i] = get_value( hand[i] );

    n_pairs = count_pairs( values, pairs, &pair_value );

    if( n_pairs == 0 )
    {
        is_flush = is_hand_flush( hand );
        is_straight = is_hand_straight( values, &max_value );

        if( is_flush )
        {
            if( is_straight )
            {
                if( max_value == 13 )
                    return( 500 );
                else
                    return( 100 );
            }
            else
                return( 7 );
        }
        else if( is_straight )
            return( 5 );
    }
    else if( n_pairs == 1 )
    {
        if( pairs[0] == 3 )
            return( 3 );
        else if( pairs[0] == 4 )
            return( 40 );
    }
    else
    {
        if( pairs[0] == 2 && pairs[1] == 2 )
            return( 2 );
        if( pairs[0] == 3 && pairs[1] == 2 ||
            pairs[0] == 2 && pairs[1] == 3 )
            return( 10 );
    }
    return( 0 );
}
#endif

static  int  play(
    int   n_cards,
    Card  hand[] )
{
    int   i;

    n_in_pack = 52 - n_cards;

    for_less( i, n_cards, 5 )
    {
        hand[i] = deal();
    }

    return( evaluate_hand( hand ) );
}

static  VIO_Real  decide(
    int   n_cards,
    Card  hand[],
    int   n_iters )
{
    int   i, j;
    int   gain, iter;

    n_in_pack = 0;
    for_less( i, 0, 52 )
    {
        for_less( j, 0, n_cards )
            if( hand[j] == i )
                break;
        if( j == n_cards )
        {
            pack[n_in_pack] = i;
            ++n_in_pack;
        }
    }

    if( n_in_pack + n_cards != 52 )
    {
        print( "Duplicate cards\n" );
        exit( 1 );
    }

    gain = 0;
    for_less( iter, 0, n_iters )
    {
        gain += play( n_cards, hand );
    }

    return( (VIO_Real) gain / (VIO_Real) iter );
}
