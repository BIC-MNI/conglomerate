#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               labels_filename, output_filename;
    int                  i, n_dirs, *dx, *dy, *dz;
    int                  sizes[MAX_DIMENSIONS];
    Real                 value, label_value, threshold;
    int                  x, y, z, tx, ty, tz, n_neighbours;
    Volume               labels, new_labels;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &labels_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        return( 1 );
    }

    (void) get_real_argument( 0.5, &threshold );

    if( input_volume( labels_filename, 3, File_order_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &labels,
                      NULL ) != OK )
        return( 1 );

    new_labels = copy_volume_definition( labels, NC_UNSPECIFIED, FALSE,
                                         0.0, 0.0 );

    get_volume_sizes( labels, sizes );

    n_dirs = get_3D_neighbour_directions( EIGHT_NEIGHBOURS, &dx, &dy, &dz );

    for_less( x, 0, sizes[X] )
    for_less( y, 0, sizes[Y] )
    for_less( z, 0, sizes[Z] )
    {
        n_neighbours = 0;
        value = get_volume_real_value( labels, x, y, z, 0, 0 );
        if( value == 0.0 )
        {
            for_less( i, 0, n_dirs )
            {
                tx = x + dx[i];
                ty = y + dy[i];
                tz = z + dz[i];
                if( tx >= 0 && tx < sizes[X] &&
                    ty >= 0 && ty < sizes[Y] &&
                    tz >= 0 && tz < sizes[Z] )
                {
                    label_value = get_volume_real_value( labels, tx, ty, tz,
                                                         0, 0 );
                    if( label_value != 0.0 )
                        ++n_neighbours;
                }
            }
        }

        if( n_neighbours > 0 && get_random_0_to_1() <
                               threshold * (Real) n_neighbours / (Real) n_dirs )
            value = label_value;

        set_volume_real_value( new_labels, x, y, z, 0, 0, value );
    }

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED,
                                   FALSE, 0.0, 0.0, new_labels, labels_filename,
                                   "perturb_labels\n", NULL );

    return( 0 );
}
