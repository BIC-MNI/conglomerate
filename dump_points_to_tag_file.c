#include  <volume_io/internal_volume_io.h>
#include  <bicpl.h>

int  main(
    int  argc,
    char *argv[] )
{
    STRING         filename1, filename2, filename3, output_filename;
    STRING         filenames[2];
    Real           **tags[2];
    int            i, surface, n_surfaces, n_points[2];
    Point          *points[2];
    File_formats   format;
    object_struct  **object_list;
    int            n_objects;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( NULL, &filename1 ) ||
        !get_string_argument( NULL, &filename2 ) )
    {
        print_error( "Usage: %s input1.obj [input2.obj] output.txt\n" );
        return( 1 );
    }

    if( get_string_argument( NULL, &filename3 ) )
    {
        filenames[0] = filename1;
        filenames[1] = filename2;
        output_filename = filename3;
        n_surfaces = 2;
    }
    else
    {
        filenames[0] = filename1;
        output_filename = filename2;
        n_surfaces = 1;
    }

    for_less( surface, 0, n_surfaces )
    {
        if( input_graphics_file( filenames[surface], &format, &n_objects,
                                 &object_list ) != OK )
        {
            print( "Couldn't read %s.\n", filenames[surface] );
            return( 1 );
        }
    
        if( n_objects != 1 )
        {
            print( "File must contain exactly 1 object.\n" );
            return( 1 );
        }

        n_points[surface] =
                 get_object_points( object_list[0], &points[surface] );

        if( surface > 0 && n_points[surface] != n_points[surface-1] )
        {
            print_error( "Mismatching number of points.\n" );
            return( 1 );
        }

        ALLOC2D( tags[surface], n_points[surface], 3 );
        for_less( i, 0, n_points[surface] )
        {
            tags[surface][i][X] = (Real) Point_x(points[surface][i]);
            tags[surface][i][Y] = (Real) Point_y(points[surface][i]);
            tags[surface][i][Z] = (Real) Point_z(points[surface][i]);
        }
    }

    (void) output_tag_file( output_filename, "From two object files.",
                     n_surfaces, n_points[0], tags[0], tags[1],
                     NULL, NULL, NULL, NULL );

    return( 0 );
}
