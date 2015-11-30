
/* ----------------------------- MNI Header -----------------------------------
@NAME       : add_labels.c
@USAGE      : add_labels   input.mnc  output.mnc [-clobber] lookup_filename
            : add_labels   input.mnc  output.mnc [-clobber] -label val1 str1
                                                            [val2 str2 ...]
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: This program adds label lookups to a minc file.  A label lookup
              is a list of volume values and corresponding label strings for
              each.  These are either specified on the command line, using the
              -label option, or from a file which contains pairs of integers
              and strings, separated by white space.
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
Usage: %s [-clobber] input.mnc output.mnc  -label val1 str1 [val2 str2 ...]\n\
\n\
   or: %s [-clobber] input.mnc output.mnc lookup_file\n\
\n\
     Creates a new minc file, adding a set of value-label pairs, specified\n\
     on the command line or in a file, to the set in the input minc file.\n\
     If the label added, e.g. str1, is empty, then it deletes the value from\n\
     the list.\n\n";

    print_error( usage_str, executable, executable );
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
    VIO_STR     input_filename, output_filename, label_filename;
    VIO_STR     argument;
    int        clobber_mode, a, in_minc_id, out_minc_id, value;
    int        i, n_excluded, label_lookup_var;
    VIO_BOOL    label_specified;
    VIO_STR     label_string;
    int        n_labels;
    int        *values;
    VIO_STR     *labels;
    FILE       *file;

    /* --- initialize the information that will be in the arguments */

    input_filename = (char *) NULL;
    output_filename = (char *) NULL;
    label_filename = (char *) NULL;

    label_specified = FALSE;

    clobber_mode = NC_NOCLOBBER;

    /* --- parse arguments */

    for_less( a, 1, argc )
    {
        argument = argv[a];

        if( equal_strings( argument, "-clobber" ) )
            clobber_mode = NC_CLOBBER;
        else if( equal_strings( argument, "-label" ) )
        {
            label_specified = TRUE;   /* --- remaining args are pairs */
            break;
        }
        else if( input_filename == (char *) NULL )
            input_filename = argument;
        else if( output_filename == (char *) NULL )
            output_filename = argument;
        else if( label_filename == (char *) NULL )
            label_filename = argument;
        else
        {
            usage( argv[0] );
            return( 1 );
        }
    }

    /* --- check input and output have been specified */

    if( output_filename == NULL )
    {
        usage( argv[0] );
        return( 1 );
    }

    /* --- check that we are not trying to write the same minc file as reading*/

    if( equal_strings( input_filename, output_filename ) )
    {
        print_error( "Cannot output to same file as input.\n" );
        return( 1 );
    }

    /* --- open the source minc file */

    set_ncopts(NC_VERBOSE);

    in_minc_id = miopen( input_filename, NC_NOWRITE );

    if( in_minc_id == MI_ERROR )
        return( 1 );

    /* --- read the label lookup from the minc file */

    if( !read_label_lookup( in_minc_id, &n_labels, &values, &labels ) )
    {
        print_error( "Error reading current label lookup.\n" );
        return( 1 );
    }

    /* --- if must take remaining arguments as value-label pairs */

    if( label_specified )
    {
        /* --- if usage incorrect, quit */

        ++a;
        if( label_filename != NULL || a + 2 > argc || ((argc - a) % 2) != 0 )
        {
            usage( argv[0] );
            return( 1 );
        }

        /* --- grab pairs of arguments */

        for( ;  a < argc;  a += 2 )
        {
            if( sscanf( argv[a], "%d", &value ) != 1 )
            {
                usage( argv[0] );
                return( 1 );
            }

            /* --- add value-label pair to list */

            if( string_length( argv[a+1] ) > 0 )
            {
                add_label_to_list( &n_labels, &values, &labels,
                                   value, argv[a+1] );
            }
            else
            {
                delete_label_from_list( &n_labels, &values, &labels,
                                        value );
            }
        }
    }
    else if( label_filename == NULL )    /* --- no -label or filename */
    {
        usage( argv[0] );
        return( 1 );
    }
    else
    {
        /* --- open the file containing the list of value-label pairs */

        if( open_file( label_filename, READ_FILE, ASCII_FORMAT, &file ) != VIO_OK )
        {
            print_error( "Could not open \"%s\".\n", label_filename);
            return( 1 );
        }

        /* --- read an int and a string */

        while( input_int( file, &value ) == VIO_OK &&
               input_possibly_quoted_string( file, &label_string ) == VIO_OK)
        {
            /* --- add value-label pair to list */

            if( string_length( label_string ) > 0 )
            {
                add_label_to_list( &n_labels, &values, &labels,
                                   value, label_string );
            }
            else
            {
                delete_label_from_list( &n_labels, &values, &labels,
                                        value );
            }
        }

        (void) close_file( file );
    }

    /* --- create the output minc file */

    out_minc_id = micreate( output_filename, clobber_mode );

    if( out_minc_id == MI_ERROR )
        return( 1 );

    /* --- copy all the variable definitions from the input to the output file*/

    n_excluded = 0;
    if( get_label_lookup_var( in_minc_id, &label_lookup_var ) )
        ++n_excluded;

    (void) micopy_all_var_defs( in_minc_id, out_minc_id, n_excluded,
                                &label_lookup_var );

    /* --- output the label lookup to the output file */

    if( !write_label_lookup( out_minc_id, n_labels, values, labels ) )
        print_error( "Error writing label lookup.\n" );

    (void) ncendef( out_minc_id );

    /* --- copy all the variable values from the input to the output file */

    (void) micopy_all_var_values( in_minc_id, out_minc_id, n_excluded,
                                  &label_lookup_var );

    /* --- close the files */

    miclose( in_minc_id );
    miclose( out_minc_id );

    /* --- free up the memory */

    if( n_labels > 0 )
    {
        for_less( i, 0, n_labels )
            FREE( labels[i] );

        FREE( values );
        FREE( labels );
    }

    return( 0 );
}
