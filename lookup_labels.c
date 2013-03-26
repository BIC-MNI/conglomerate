
/* ----------------------------- MNI Header -----------------------------------
@NAME       : lookup_labels.c
@USAGE      : lookup_labels   input.mnc 
            : lookup_labels   input.mnc  -value value1 values2 ...
            : lookup_labels   input.mnc  -label label1 labels2 ...
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: This program displays the value-label correspondences stored
              in the specified minc file.  With no arguments, it simply
              lists all pairs of values and label strings.  With the
              -value argument, it displays all pairs of values whose values
              match one of the subsequent command line arguments.  With
              the -label option, it does the similar task, based on the
              command line label strings.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Nov. 29, 1994    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

#include  <volume_io.h>
#include  <minc_labels.h>

/* ----------------------------- MNI Header -----------------------------------
@NAME       : usage
@INPUT      : executable
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Displays the usage and aborts.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Nov. 29, 1994    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

static  void  usage(
    VIO_STR   executable )
{
    VIO_STR  usage_str = "\n\
Usage: %s input.mnc\n\
\n\
     Lists all value-label pairs in the minc file.\n\
\n\
   or: %s input.mnc -value val1 val2 ...\n\
\n\
     Lists value-label pairs in the minc file, specified by value\n\
\n\
   or: %s input.mnc -label label1 label2 ...\n\
\n\
     Lists value-label pairs in the minc file, specified by label\n\n";

    print_error( usage_str, executable, executable, executable );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : main
@INPUT      : argc
              argv
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Adds the label lookup to the file.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Nov. 29, 1994    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

int  main(
    int   argc,
    char  *argv[] )
{
    VIO_STR     input_filename, label;
    int        i, a, in_minc_id, value;
    int        n_labels;
    int        *values;
    VIO_STR     *labels;
    BOOLEAN    list_all, by_value;

    /* --- check for enough arguments */

    if( argc < 2 )
    {
        usage( argv[0] );
        return( 1 );
    }

    input_filename = argv[1];
    list_all = FALSE;

    if( argc == 2 )
        list_all = TRUE;
    else if( strcmp( argv[2], "-value" ) == 0 )
        by_value = TRUE;
    else if( strcmp( argv[2], "-label" ) == 0 )
        by_value = FALSE;
    else
    {
        usage( argv[0] );
        return( 1 );
    }

    /* --- open the source minc file */

    ncopts = NC_VERBOSE;

    in_minc_id = miopen( input_filename, NC_NOWRITE );

    if( in_minc_id == MI_ERROR )
        return( 1 );

    /* --- read the label lookup from the minc file */

    if( !read_label_lookup( in_minc_id, &n_labels, &values, &labels ) )
    {
        print_error( "Error reading current label lookup.\n" );
        return( 1 );
    }

    (void) miclose( in_minc_id );

    if( list_all )
    {
        for_less( i, 0, n_labels )
            print_label( values[i], labels[i] );
    }
    else
    {
        for_less( a, 3, argc )
        {
            if( by_value )
            {
                if( sscanf( argv[a], "%d", &value ) != 1 )
                {
                    usage( argv[0] );
                    return( 1 );
                }
                (void) lookup_label_for_value( n_labels, values, labels,
                                               value, &label );
            }
            else
            {
                label = argv[a];
                (void) lookup_value_for_label( n_labels, values, labels,
                                               label, &value );
            }

            print_label( value, label );
        }
    }

    if( n_labels > 0 )
    {
        for_less( i, 0, n_labels )
            FREE( labels[i] );

        FREE( values );
        FREE( labels );
    }

    return( 0 );
}
