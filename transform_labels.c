#include  <internal_volume_io.h>
#include  <bicpl.h>

private  void  usage(
    STRING   executable )
{
    STRING  usage_str = "\n\
Usage: %s  labels.mnc  like.mnc  output.mnc\n\
\n\
\n";

    print_error( usage_str, executable );
}

int  main(
    int   argc,
    char  *argv[] )
{
    STRING               labels_filename, like_filename, output_filename;
    int                  *counts, i;
    int                  sizes[MAX_DIMENSIONS], int_label;
    int                  n_voxels, n_super_voxels, x, y, z;
    int                  label_sizes[MAX_DIMENSIONS];
    int                  int_x_label, int_y_label, int_z_label;
    Real                 x_new_label, y_new_label, z_new_label;
    int                  xs, ys, zs, super_sample;
    int                  n_non_zero, last_nonzero_label, decider;
    Real                 x_label, y_label, z_label;
    Volume               labels, new_labels, like_volume;
    Real                 separations[MAX_DIMENSIONS];
    Real                 label;
    progress_struct      progress;
    General_transform    *labels_trans, *like_trans;
    Transform            *labels_to_world, *new_labels_to_world;
    Transform            world_to_labels, new_labels_to_labels;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &labels_filename ) ||
        !get_string_argument( "", &like_filename ) ||
        !get_string_argument( "", &output_filename ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    (void) get_int_argument( 1, &super_sample );

    if( input_volume( labels_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &labels,
                      NULL ) != OK )
        return( 1 );

    if( input_volume_header_only( like_filename, 3, XYZ_dimension_names,
                                  &like_volume, NULL ) != OK )
        return( 1 );

    new_labels = copy_volume_definition( like_volume, NC_BYTE, FALSE, 0.0, 0.0);

    get_volume_sizes( new_labels, sizes );
    get_volume_sizes( labels, label_sizes );

    labels_trans = get_voxel_to_world_transform( labels );
    like_trans = get_voxel_to_world_transform( new_labels );

    if( get_transform_type(labels_trans) != LINEAR ||
        get_transform_type(like_trans) != LINEAR )
    {
        print(
         "Error in volume transform types.\n" );
        return( 0.0 );
    }

    labels_to_world = get_linear_transform_ptr( labels_trans );
    new_labels_to_world = get_linear_transform_ptr( like_trans );

    compute_transform_inverse( labels_to_world, &world_to_labels );

    concat_transforms( &new_labels_to_labels, new_labels_to_world,
                       &world_to_labels );

    decider = (super_sample * super_sample * super_sample + 1) / 2;
    n_voxels = 0;
    n_super_voxels = 0;

    ALLOC( counts, super_sample * super_sample * super_sample+1 );

    for_less( i, 0, super_sample * super_sample * super_sample +1) 
        counts[i] = 0;

    initialize_progress_report( &progress, FALSE, sizes[X], "Transforming" );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        for_less( z, 0, sizes[Z] )
        {
            n_non_zero = 0;

            for_less( xs, 0, super_sample )
            {
                x_new_label = x - 0.5 + ((Real) xs + 0.5) / (Real) super_sample;
                for_less( ys, 0, super_sample )
                {
                    y_new_label = y - 0.5 + ((Real) ys + 0.5) /
                                            (Real) super_sample;
                    for_less( zs, 0, super_sample )
                    {
                        z_new_label = z - 0.5 + ((Real) zs + 0.5) /
                                                (Real) super_sample;
                        transform_point( &new_labels_to_labels,
                                         x_new_label, y_new_label, z_new_label,
                                         &x_label, &y_label, &z_label );

                        int_x_label = ROUND( x_label );
                        int_y_label = ROUND( y_label );
                        int_z_label = ROUND( z_label );

                        if( int_x_label >= 0 && int_x_label < label_sizes[X] &&
                            int_y_label >= 0 && int_y_label < label_sizes[Y] &&
                            int_z_label >= 0 && int_z_label < label_sizes[Z] )
                            label = get_volume_voxel_value( labels, int_x_label,
                                               int_y_label, int_z_label, 0, 0 );
                        else
                            label = 0.0;

                        int_label = ROUND( label );

                        if( int_label != 0 )
                        {
                            last_nonzero_label = int_label;
                            ++n_non_zero;
                        }
                    }
                }
            }

            n_super_voxels += n_non_zero;
            ++counts[n_non_zero];

            if( n_non_zero >= decider )
            {
                int_label = last_nonzero_label;
                ++n_voxels;
            }
            else
                int_label = 0;

            set_volume_voxel_value( new_labels, x, y, z, 0, 0, int_label );
        }

        update_progress_report( &progress, x + 1 );
    }

    terminate_progress_report( &progress );

    get_volume_separations( new_labels, separations );

    print( "N voxels: %d\n", n_voxels );
    print( "Volume  : %g\n", (Real) n_voxels * separations[0] * separations[1]
                             * separations[2] );

    if( super_sample > 1 )
    {
        print( "Super sampled Volume  : %g\n", (Real) n_super_voxels /
              (Real) super_sample / (Real) super_sample / (Real) super_sample *
               separations[0] * separations[1] * separations[2] );

        for_less( i, 0, super_sample*super_sample*super_sample+1 )
        {
             print( "  %g   Volume: %g\n", (Real) i / (Real)
                        (super_sample*super_sample*super_sample),
                       (Real) counts[i] *
                       separations[0] * separations[1] * separations[2] );
        }
    }

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED,
                                   FALSE, 0.0, 0.0, new_labels, labels_filename,
                                   "transform_volume\n", NULL );

    return( 0 );
}
