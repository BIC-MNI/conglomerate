#include  <internal_volume_io.h>
#include  <bicpl.h>


private  Real  evaluate_error(
    Volume   volume,
    Real     max_diff );

private  void  normalize_intensities(
    Volume   volume,
    Real     max_diff );

private  BOOLEAN  solve_system(
    int   n,
    Real  **coefs,
    Real  values[],
    Real  solution[] );

int  main(
    int   argc,
    char  *argv[] )
{
    char                 *input_filename, *output_filename;
    Volume               volume;
    Real                 max_diff;
    Real                 before, after;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &input_filename ) ||
        !get_string_argument( "", &output_filename ) ||
        !get_real_argument( 0.0, &max_diff ) )
    {
        print( "%s  input.mnc  output.mnc  max_diff\n", argv[0] );
        return( 1 );
    }

    if( input_volume( input_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0,
                      TRUE, &volume, (minc_input_options *) NULL ) != OK )
        return( 1 );

    before = evaluate_error( volume, max_diff );

    print( "Before: %g\n", before );

    normalize_intensities( volume, max_diff );

    after = evaluate_error( volume, max_diff );

    print( "After : %g\n", after );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED,
                                   FALSE, 0.0, 0.0, volume, input_filename,
                                   "flip_volume", (minc_output_options *) NULL);

    delete_volume( volume );

    return( 0 );
}

private  Real  evaluate_error(
    Volume   volume,
    Real     max_diff )
{
    int   tx, ty, tz, dx, dy, dz, n_voxels;
    int   x, y, z, sizes[N_DIMENSIONS];
    Real  error, value1, value2, diff;

    get_volume_sizes( volume, sizes );

    error = 0.0;
    n_voxels = 0;

    for_less( x, 0, sizes[0] )
    {
        if( x < sizes[0]-1 )
            tx = 1;
        else
            tx = 0;
        for_less( y, 0, sizes[1] )
        {
            if( y < sizes[1]-1 )
                ty = 1;
            else
                ty = 0;
            for_less( z, 0, sizes[2] )
            {
                if( z < sizes[2]-1 )
                    tz = 1;
                else
                    tz = 0;

                GET_VALUE_3D( value1, volume, x, y, z );

                for_inclusive( dx, 0, tx )
                for_inclusive( dy, 0, ty )
                for_inclusive( dz, 0, tz )
                {
                    if( dx == 0 && dy == 0 && dz == 0 )
                        continue;

                    GET_VALUE_3D( value2, volume, x + dx, y + dy, z + dz );

                    diff = value1 - value2;
                    if( diff < 0.0 )
                        diff = -diff;

                    if( diff <= max_diff )
                    {
                        error += diff * diff;
                        ++n_voxels;
                    }
                }
            }
        }
    }

    print( "N voxels: %d\n", n_voxels );

    return( error );
}

private  void  normalize_intensities(
    Volume   volume,
    Real     max_diff )
{
    int   i, j, tx, ty, tz, dx, dy, dz;
    int   x, y, z, sizes[N_DIMENSIONS];
    Real  scale, trans, voxel, sum;
    Real  answer[6], cx, cy, cz;
    Real  x1, y1, z1, x2, y2, z2;
    Real  value1, value2, diff;
    Real  **coefs, constants[6], p[7];
    Real  min_voxel, max_voxel;

    get_volume_sizes( volume, sizes );

    cx = (Real) (sizes[0]-1) / 2.0;
    cy = (Real) (sizes[1]-1) / 2.0;
    cz = (Real) (sizes[2]-1) / 2.0;

    ALLOC2D( coefs, 6, 6 );

    for_less( i, 0, 6 )
    {
        for_less( j, 0, 6 )
        {
            coefs[i][j] = 0.0;
        }
        constants[i] = 0.0;
    }

    for_less( x, 0, sizes[0] )
    {
        if( x < sizes[0]-1 )
            tx = 1;
        else
            tx = 0;
        x1 = x - cx;
        for_less( y, 0, sizes[1] )
        {
            if( y < sizes[1]-1 )
                ty = 1;
            else
                ty = 0;
            y1 = y - cy;
            for_less( z, 0, sizes[2] )
            {
                if( z < sizes[2]-1 )
                    tz = 1;
                else
                    tz = 0;
                z1 = z - cz;

                GET_VALUE_3D( value1, volume, x, y, z );

                for_inclusive( dx, 0, tx )
                for_inclusive( dy, 0, ty )
                for_inclusive( dz, 0, tz )
                {
                    if( dx == 0 && dy == 0 && dz == 0 )
                        continue;

                    GET_VALUE_3D( value2, volume, x + dx, y + dy, z + dz );

                    diff = value1 - value2;
                    if( diff < 0.0 )
                        diff = -diff;

                    if( diff <= max_diff )
                    {
                        x2 = x + dx - cx;
                        y2 = y + dy - cy;
                        z2 = z + dz - cz;

                        p[0] = ((Real) x1 * value1 - (Real) x2 * value2);
                        p[1] = ((Real) y1 * value1 - (Real) y2 * value2);
                        p[2] = ((Real) z1 * value1 - (Real) z2 * value2);

                        p[3] = (Real) x1 - (Real) x2;
                        p[4] = (Real) y1 - (Real) y2;
                        p[5] = (Real) z1 - (Real) z2;

                        p[6] = value1 - value2;

                        for_less( i, 0, 6 )
                        {
                            for_less( j, 0, 6 )
                            {
                                coefs[i][j] += p[i] * p[j];
                            }
                            constants[i] -= p[i] * p[6];
                        }
                    }
                }
            }
        }
    }

    for_less( i, 0, 5 )
    {
        for_less( j, i+1, 6 )
        {
            coefs[j][i] = coefs[i][j];
        }
    }

    if( solve_system( 6, coefs, constants, answer ) )
    {
        for_less( i, 0, 6 )
        {
            sum = 0.0;
            for_less( j, 0, 6 )
                sum += coefs[i][j] * answer[j];
            sum -= constants[i];

            if( sum > 1.0e-2 || sum < -1.0e-2 )
            {
                print( "%g\n", sum );
/*                handle_internal_error( "Dangl\n" ); */
            }
        }

        for_less( i, 0, 6 )
            print( "Coef[%d] = %g\n", i, answer[i] );
    }
    else
    {
        print( "No inverse.\n" );
        FREE2D( coefs );
        return;
    }

    FREE2D( coefs );

    get_volume_voxel_range( volume, &min_voxel, &max_voxel );

    for_less( x, 0, sizes[0] )
    for_less( y, 0, sizes[1] )
    for_less( z, 0, sizes[2] )
    {
        GET_VALUE_3D( value1, volume, x, y, z );
        scale = 1.0 +
                answer[0] * (x-cx) + answer[1] * (y-cy) + answer[2] * (z-cz);
        trans = answer[3] * (x-cx) + answer[4] * (y-cy) + answer[5] * (z-cz);
        value1 = scale * value1 + trans;
        voxel = CONVERT_VALUE_TO_VOXEL( volume, value1 );
        if( voxel < min_voxel )
            voxel = min_voxel;
        else if( voxel > max_voxel )
            voxel = max_voxel;
        SET_VOXEL_3D( volume, x, y, z, voxel );
    }
}

private  BOOLEAN  solve_system(
    int   n,
    Real  **coefs,
    Real  values[],
    Real  solution[] )
{
    int       i, j, k, p, *row, tmp;
    Real      **a, *s, val, best_val, m, sum;
    BOOLEAN   success;

    ALLOC2D( a, n, n+1 );
    ALLOC( row, n );
    ALLOC( s, n );

    for_less( i, 0, n )
    {
        for_less( j, 0, n )
            a[i][j] = coefs[i][j];
        a[i][n] = values[i];
    }

    for_less( i, 0, n )
    {
        row[i] = i;
        s[i] = ABS( a[i][0] );
        for_less( j, 1, n )
        {
            if( ABS(a[i][j]) > s[i] )
               s[i] = ABS(a[i][j]);
        }
    }

    success = TRUE;

    for_less( i, 0, n-1 )
    {
        p = i;
        best_val = a[row[i]][i] / s[row[i]];
        best_val = ABS( best_val );
        for_less( j, i, n )
        {
            val = a[row[j]][i] / s[row[j]];
            val = ABS( val );
            if( val > best_val )
            {
                best_val = val;
                p = j;
            }
        }

        if( a[row[p]][i] == 0.0 )
        {
            success = FALSE;
            break;
        }

        if( i != p )
        {
            tmp = row[i];
            row[i] = row[p];
            row[p] = tmp;
        }

        for_less( j, i+1, n )
        {
            m = a[row[j]][i] / a[row[i]][i];
            for_less( k, i, n+1 )
                a[row[j]][k] -= m * a[row[i]][k];
        }
    }

    if( success && a[row[n-1]][n-1] == 0.0 )
        success = FALSE;

    for( i = n-1;  i >= 0;  --i )
    {
        sum = 0.0;
        for_less( j, i+1, n )
            sum += a[row[i]][j] * solution[j];
        solution[i] = (a[row[i]][n] - sum) / a[row[i]][i];
    }

    FREE2D( a );
    FREE( row );
    FREE( s );

    return( success );
}
