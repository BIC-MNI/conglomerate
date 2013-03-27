#include  <volume_io.h>
#include  <bicpl.h>

  VIO_Status  process_object(
    object_struct  *object );

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s  input.obj  xview yview zview xup yup zup\n\
\n\
     Computes the world limits of the object in the orthogonal view \n\
     defined by ( xview yview zview xup yup zup )\n\n";

    print_error( usage_str, executable );
}

int  main(
    int    argc,
    char   *argv[] )
{
    VIO_STR              input_filename, transform_filename;
    VIO_Real                xview, yview, zview, xup, yup, zup, x, y, z;
    int                 point, n_points, n_objects, dim, i;
    VIO_Real                min_position[VIO_N_DIMENSIONS];
    VIO_Real                max_position[VIO_N_DIMENSIONS];
    VIO_Real                min_pos, max_pos;
    VIO_Point               *points, trans_point;
    VIO_Vector              x_axis, y_axis, z_axis;
    VIO_Vector              axis[VIO_N_DIMENSIONS];
    VIO_General_transform   transform;
    VIO_File_formats        format;
    object_struct       **object_list;
    VIO_BOOL             first, have_transform;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &input_filename ) ||
        !get_real_argument( 0.0, &xview ) ||
        !get_real_argument( 0.0, &yview ) ||
        !get_real_argument( 0.0, &zview ) ||
        !get_real_argument( 0.0, &xup ) ||
        !get_real_argument( 0.0, &yup ) ||
        !get_real_argument( 0.0, &zup ) )
    {
        usage( argv[0] );
        return( 1 );
    }

    have_transform = get_string_argument( NULL, &transform_filename );
    if( have_transform &&
        input_transform_file( transform_filename, &transform ) != VIO_OK )
    {
        return( 1 );
    }

    if( input_graphics_file( input_filename, &format, &n_objects,
                             &object_list ) != VIO_OK )
        return( 1 );

    fill_Vector( z_axis, xview, yview, zview );
    fill_Vector( y_axis, xup, yup, zup );
    CROSS_VECTORS( x_axis, z_axis, y_axis );
    CROSS_VECTORS( y_axis, x_axis, z_axis );
    NORMALIZE_VECTOR( x_axis, x_axis );
    NORMALIZE_VECTOR( y_axis, y_axis );
    NORMALIZE_VECTOR( z_axis, z_axis );

    axis[0] = x_axis;
    axis[1] = y_axis;
    axis[2] = z_axis;

    for_less( dim, 0, VIO_N_DIMENSIONS )
    {
        min_position[dim] = 0.0;
        max_position[dim] = 0.0;
    }
    first = TRUE;

    for_less( i, 0, n_objects )
    {
        n_points = get_object_points( object_list[i], &points );

        for_less( point, 0, n_points )
        {
            if( have_transform )
            {
                general_transform_point( &transform,
                                         RPoint_x( points[point] ),
                                         RPoint_y( points[point] ),
                                         RPoint_z( points[point] ),
                                         &x, &y, &z );
                fill_Point( trans_point, x, y, z );
            }
            else
                trans_point = points[point];

            for_less( dim, 0, VIO_N_DIMENSIONS )
            {
                min_pos = DOT_VECTORS( axis[dim], trans_point );
                max_pos = min_pos;
                if( get_object_type(object_list[i]) == LINES )
                {
                    min_pos -= get_lines_ptr(object_list[i])->
                                               line_thickness / 2.0;
                    max_pos += get_lines_ptr(object_list[i])->
                                               line_thickness / 2.0;
                }

                if( first )
                {
                    min_position[dim] = min_pos;
                    max_position[dim] = max_pos;
                }
                else
                {
                    if( min_pos < min_position[dim] )
                        min_position[dim] = min_pos;
                    if( max_pos > max_position[dim])
                        max_position[dim] = max_pos;
                }
            }

            first = FALSE;
        }

        if( get_object_type(object_list[i]) == LINES )
        {
            for_less( dim, 0, VIO_N_DIMENSIONS )
            {
                min_position[dim] -= get_lines_ptr(object_list[i])->
                                           line_thickness / 2.0;
                max_position[dim] += get_lines_ptr(object_list[i])->
                                           line_thickness / 2.0;
            }
        }
    }

    print( "Bounding_box: %g %g %g %g %g %g\n",
           min_position[0], max_position[0],
           min_position[1], max_position[1],
           min_position[2], max_position[2] );

    delete_object_list( n_objects, object_list );

    return( 0 );
}
