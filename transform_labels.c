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
    int                  sizes[MAX_DIMENSIONS];
    int                  x, y, z;
    int                  label_sizes[MAX_DIMENSIONS];
    Real                 voxel[MAX_DIMENSIONS];
    int                  xs, ys, zs, super_sample;
    int                  n_samples;
    int                  degrees_continuity;
    int                  x_voxel, y_voxel, z_voxel;
    Real                 weighted_n_voxels;
    Volume               labels, new_labels, like_volume;
    Real                 separations[MAX_DIMENSIONS];
    Real                 label, target_label, sum;
    Real                 x_voxel2, y_voxel2, z_voxel2;
    int                  n_found;
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
    (void) get_int_argument( -1, &degrees_continuity );

    if( input_volume( labels_filename, 3, XYZ_dimension_names,
                      NC_UNSPECIFIED, FALSE, 0.0, 0.0, TRUE, &labels,
                      NULL ) != OK )
        return( 1 );

    if( input_volume_header_only( like_filename, 3, XYZ_dimension_names,
                                  &like_volume, NULL ) != OK )
        return( 1 );

    new_labels = copy_volume_definition( like_volume, NC_BYTE, FALSE, 0.0, 0.0);
    set_volume_real_range( new_labels, 0.0, 100.0 );

    get_volume_sizes( new_labels, sizes );
    get_volume_sizes( labels, label_sizes );

    n_found = 0;
    target_label = 0.0;

    for_less( x, 0, label_sizes[X] )
    for_less( y, 0, label_sizes[Y] )
    for_less( z, 0, label_sizes[Z] )
    {
        label = get_volume_real_value( labels, x, y, z, 0, 0 );
        if( label != 0.0 )
        {
            if( n_found > 0 )
            {
                if( label != target_label )
                {
                    print_error( "    Found: %g %g at %d %d %d\n",
                                  label, target_label, x, y, z );
                    ++n_found;
                }
            }
            else
            {
                target_label = label;
                n_found = 1;
            }
        }
    }

    if( n_found != 1 )
    {
        print_error( "Label volume must contain only one label.\n");
        return( 1 );
    }

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

    n_samples = super_sample * super_sample * super_sample;

    weighted_n_voxels = 0.0;

    initialize_progress_report( &progress, FALSE, sizes[X], "Transforming" );

    for_less( x, 0, sizes[X] )
    {
        for_less( y, 0, sizes[Y] )
        for_less( z, 0, sizes[Z] )
        {
            sum = 0.0;

            for_less( xs, 0, super_sample )
            {
                x_voxel2 = x - 0.5 + ((Real) xs + 0.5) / (Real) super_sample;
                for_less( ys, 0, super_sample )
                {
                    y_voxel2 = y - 0.5 + ((Real) ys + 0.5) / (Real)super_sample;
                    for_less( zs, 0, super_sample )
                    {
                        z_voxel2 = z - 0.5 + ((Real) zs + 0.5) /
                                              (Real) super_sample;

                        transform_point( &new_labels_to_labels,
                                         x_voxel2, y_voxel2, z_voxel2,
                                         &voxel[X], &voxel[Y], &voxel[Z] );

                        if( degrees_continuity < 0 )
                        {
                            x_voxel = ROUND( voxel[X] );
                            y_voxel = ROUND( voxel[Y] );
                            z_voxel = ROUND( voxel[Z] );
                            if( x_voxel >= 0 && x_voxel < label_sizes[X] &&
                                y_voxel >= 0 && y_voxel < label_sizes[Y] &&
                                z_voxel >= 0 && z_voxel < label_sizes[Z] )
                            {
                                label = get_volume_real_value( labels,
                                              x_voxel, y_voxel, z_voxel, 0, 0 );
                            }
                            else
                                label = 0.0;
                        }
                        else
                        {
                            evaluate_volume( labels, voxel, NULL,
                                             degrees_continuity, TRUE, 0.0,
                                             &label, NULL, NULL );
                        }

                        sum += label;
                    }
                }
            }

            sum /= target_label * (Real) n_samples;
            weighted_n_voxels += sum;

            set_volume_real_value( new_labels, x, y, z, 0, 0, sum * 100.0 );
        }

        update_progress_report( &progress, x + 1 );
    }

    terminate_progress_report( &progress );

    get_volume_separations( new_labels, separations );

    print( "Volume  : %g\n", (Real) weighted_n_voxels *
                    separations[0] * separations[1] * separations[2] );

    (void) output_modified_volume( output_filename, NC_UNSPECIFIED,
                                   FALSE, 0.0, 0.0, new_labels, labels_filename,
                                   "transform_volume\n", NULL );

    return( 0 );
}
