#include  <stdio.h>
#include  <stdlib.h>

const  unsigned char  NOT_ASSIGNED = 183; 
const  unsigned char  ASSIGNED     = 47; 

#ifdef  SIMPLE
#define SIMPLE
typedef  int  Int;
#else
class  Int
{
private:
    unsigned   char    assigned_flag;
    int        value;

public:

    void assigned() { assigned_flag = ASSIGNED; }
    void check_assigned() const
    {
        if( assigned_flag != ASSIGNED )
        {
            (void) fprintf( stderr, "Error not assigned\n" );
            abort();
        }
    }

    Int( void ) { assigned_flag = NOT_ASSIGNED; }
    Int( int x ) { value = x; assigned(); }

    int  operator < ( const Int &i2 ) const
    {
        check_assigned();
        i2.check_assigned();
        return( value < i2.value );
    }

    int  operator < ( const int i2 ) const
    {
        check_assigned();
        return( value < i2 );
    }

    const Int  & operator ++ ()
    {
        check_assigned();
        ++value;
        return( *this );
    }
    Int  operator +( const Int &i2 ) const
    {
        Int  sum;

        check_assigned();
        i2.check_assigned();
        sum = (Int) (value + i2.value);

        return( sum );
    }

    operator int () const
    {
        check_assigned();
        return( value );
    }

    operator int * ()
    {
        assigned();
        return( (int *) &value );
    }

    operator int * ( Int *i )
    {
        i->assigned();
        return( (int *) &i->value );
    }
};
#endif

static  Int  y = 0;

Int  func(
    Int  x )
{
    return( x + y );
}

int  main(
    int    argc,
    char   *argv[] )
{
    int   ans, *ptr;
    Int   x, ass, sum, i, j, n_iters;

    if( argc < 2 || sscanf( argv[1], "%d", (int *) &n_iters ) != 1 )
        n_iters = 1;

    printf( "%d\n", (int) n_iters );

    if( argc < 100 || sscanf( argv[2], "%d", &n_iters ) != 1 )
        y = 1;

    x = 4;
    sum = 0;

    for( i = 0;  i < n_iters;  ++i )
    for( j = 0;  j < 100000;  ++j )
    {
        sum = sum + func( x );
    }

    ans = (int) sum;

    (void) printf( "Answer: %d\n", ans );

    return( 0 );
}
