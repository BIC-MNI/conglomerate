#include  <minc.h>

#define  LENGTH  1000000

main()
{
    int   i, cdf, xvar, dim;
    char  name[LENGTH+1];

    for( i = 0;  i < LENGTH;  ++i )
        name[i] = 'a' + i % 26;

    name[i] = (char) 0;

    cdf = micreate( "test.mnc", NC_CLOBBER );
    dim=ncdimdef(cdf, "xcoord", 256);
    xvar=ncvardef(cdf,"xcoord", NC_DOUBLE, 1, &dim);
    ncattput( cdf, xvar, "long_name", NC_CHAR, LENGTH+1, name );

    miclose( cdf );
}
