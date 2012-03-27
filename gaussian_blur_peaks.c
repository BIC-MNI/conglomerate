#include  <volume_io.h>
#include  <bicpl.h>

private  Real  gaussian_function(
    Real  exp_constant,
    Real  x1,
    Real  y1,
    Real  z1,
    Real  x2,
    Real  y2,
    Real  z2 );

private  void  usage(
    char   executable[] )
{
    STRING  usage =
"Usage: %s  like_volume.mnc  output.mnc  [-max_value value] x1 y1 z1 fwhm1 \n\
            [x2 y2 z2 fwhm2 ] [-tag filename fwhm]...\n\
\n\
Creates a volume sampled according to like_volume.mnc which contains a\n\
gaussian blurring of the given 3D points.  Each point is specified by a\n\
world coordinate and a filter size defined by the full width of half the max\n\
in millimetres.  The volume created ranges from 0 to 1 in value, unless\n\
overriden by the -max_value argument.\n\n";

    print_error( usage, executable );
}

typedef struct
{
    Real  x;
    Real  y;
    Real  z;
    Real  exp_constant;
} peak_struct;

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               like_filename, output_filename, history_string, arg;
    STRING               tag_filename;
    Volume               volume;
    int                  n_peaks, p, a, xv, yv, zv;
    int                  sizes[N_DIMENSIONS], tag, n_tag_points, n_volumes;
    peak_struct          *peaks, peak;
    Real                 x, y, z, value, v[N_DIMENSIONS], fwhm;
    Real                 min_value, max_value, **tags_volume1;
    progress_struct      progress;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &like_filename ) ||
        !get_string_argument( NULL, &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    min_value = 0.0;
    max_value = 1.0;

    if( input_volume_header_only( like_filename, 3,
                            File_order_dimension_names, &volume, NULL) != OK )
        return( 1 );

    n_peaks = 0;
    peaks = NULL;

    while( get_string_argument( NULL, &arg ) )
    {
        if( equal_strings( arg, "-max_value" ) )
        {
            if( !get_real_argument( 0.0, &max_value ) )
            {
                usage( argv[0] );
                print_error( "Error in -max_value arguments\n" );
                return( 1 );
            }
        }
        else if( equal_strings( arg, "-tag" ) )
        {
            if( !get_string_argument( NULL, &tag_filename ) ||
                !get_real_argument( 0.0, &fwhm ) )
            {
                usage( argv[0] );
                print_error( "Error in -tag arguments\n" );
                return( 1 );
            }

            if( input_tag_file( tag_filename, &n_volumes, &n_tag_points,
                                &tags_volume1, NULL, NULL,
                                NULL, NULL, NULL ) != OK )
                return( 1 );

            for_less( tag, 0, n_tag_points )
            {
                peak.x = tags_volume1[tag][0];
                peak.y = tags_volume1[tag][1];
                peak.z = tags_volume1[tag][2];
                peak.exp_constant = log( 0.5 ) / (fwhm/2.0) / (fwhm/2.0);
                ADD_ELEMENT_TO_ARRAY( peaks, n_peaks, peak, DEFAULT_CHUNK_SIZE);
            }

            free_tag_points( n_volumes, n_tag_points, tags_volume1,
                             NULL, NULL, NULL, NULL, NULL );
        }
        else
        {
            if( sscanf( arg, "%lf", &peak.x ) != 1 ||
                !get_real_argument( 0.0, &peak.y ) ||
                !get_real_argument( 0.0, &peak.z ) ||
                !get_real_argument( 0.0, &fwhm ) )
            {
                usage( argv[0] );
                return( 1 );
            }

            peak.exp_constant = log( 0.5 ) / (fwhm/2.0) / (fwhm/2.0);

            ADD_ELEMENT_TO_ARRAY( peaks, n_peaks, peak, DEFAULT_CHUNK_SIZE );
        }
    }

    if( max_value <= 0.0 )
    {
        max_value = (Real) n_peaks;
        print( "Max value = %g\n", max_value );
    }


    alloc_volume_data( volume );
    set_volume_voxel_range( volume, 0.0, -1.0 );
    set_volume_real_range( volume, min_value, max_value );

    get_volume_sizes( volume, sizes );

    initialize_progress_report( &progress, FALSE, sizes[X] * sizes[Y],
                                "Scanning" );

    for_less( xv, 0, sizes[X] )
    for_less( yv, 0, sizes[Y] )
    {
        for_less( zv, 0, sizes[Z] )
        {
            v[0] = (Real) xv;
            v[1] = (Real) yv;
            v[2] = (Real) zv;

            convert_voxel_to_world( volume, v, &x, &y, &z );

            value = 0.0;
            for_less( p, 0, n_peaks )
            {
                value += gaussian_function( peaks[p].exp_constant,
                                            peaks[p].x, peaks[p].y, peaks[p].z,
                                            x, y, z );
            }

            if( value > max_value )
                value = max_value;
            else if( value < min_value )
                value = min_value;

            set_volume_real_value( volume, xv, yv, zv, 0, 0, value );
        }

        update_progress_report( &progress, xv * sizes[Y] + yv + 1 );
    }

    terminate_progress_report( &progress );
       
    history_string = NULL;
    for_less( a, 0, argc )
    {
        concat_to_string( &history_string, argv[a] );
        concat_to_string( &history_string, " " );
    }

    concat_to_string( &history_string, "\n" );
  
    (void) output_volume( output_filename, NC_UNSPECIFIED, FALSE,
                          0.0, 0.0, volume, history_string, NULL );

    delete_string( history_string );
    delete_volume( volume );

    return( 0 );
}

private  Real  gaussian_function(
    Real  exp_constant,
    Real  x1,
    Real  y1,
    Real  z1,
    Real  x2,
    Real  y2,
    Real  z2 )
{
    Real  dx, dy, dz, dist, value;

    dx = x1 - x2;
    dy = y1 - y2;
    dz = z1 - z2;

    dist = dx * dx + dy * dy + dz * dz;

    value = exp( exp_constant * dist );

    return( value );
}
