#include  <stdio.h>

main()
{
    int  i, n, ch;

    n = 0;

    while( (ch = getchar()) != EOF )
    {
        if( ch == '\n' )
        {
            for( i = n;  i < 78;  ++i )
                putchar( ' ' );
            putchar( '\\' );
            putchar( '\n' );
            n = 0;
        }
        else
        {
            putchar( ch );
            ++n;
        }
    }
}
