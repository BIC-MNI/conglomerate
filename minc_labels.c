#include  <internal_volume_io.h>

/* ----------------------------------------------------------------------------
               define the minc variable and attribute names
---------------------------------------------------------------------------- */

#define  MIlabel_lookup   "label_lookup"
#define  MIlabel_values   "label_values"
#define  MIlabel_strings  "label_strings"

/* ----------------------------- MNI Header -----------------------------------
@NAME       : read_label_lookup
@INPUT      : minc_id
@OUTPUT     : n_labels   - number of labels read
              values     - volume value of each label
              labels     - string value of each label
@RETURNS    : TRUE if successful
@DESCRIPTION: Reads the label lookup from the open MINC file.  The label
              lookup is simply a list of volume values, and corresponding
              unique string labels.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : 1993            David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  BOOLEAN  read_label_lookup(
    int    minc_id,
    int    *n_labels,
    int    *values[],
    char   **labels[] )
{
    BOOLEAN   okay;
    int       i, start_index, end_index;
    char      *label_strings;
    int       label_var, save_opts, length_label_strings, length;
    nc_type   type;

    /* --- initialize the labels to null */

    okay = TRUE;
    *n_labels = 0;
    *values = (int *) NULL;
    *labels = (char **) NULL;

    label_strings = (char *) NULL;

    /* --- set netcdf to verbose mode, to print an errors in variables, etc. */

    save_opts = ncopts;
    ncopts = NC_VERBOSE;

    /* --- find the label variable */

    label_var = ncvarid( minc_id, MIlabel_lookup );

    if( label_var == MI_ERROR )
        okay = FALSE;

    /* --- find the label_values attribute of the label variable */

    if( okay &&
        ncattinq( minc_id, label_var, MIlabel_values, &type, &length ) ==
                          MI_ERROR )
        okay = FALSE;

    if( okay && type != NC_LONG )
    {
        (void) fprintf( stderr, "Error: label values are not long integers.\n");
        okay = FALSE;
    }

    /* --- read the label_values array */

    *n_labels = length;

    if( okay && *n_labels > 0 )
    {
        ALLOC( *values, length );
        if( ncattget( minc_id, label_var, MIlabel_values, (void *) (*values) )
            == MI_ERROR )
            okay = FALSE;
    }

    /* --- find the list of label strings */

    if( okay &&
        ncattinq( minc_id, label_var, MIlabel_strings, &type,
                  &length_label_strings ) == MI_ERROR )
    {
        okay = FALSE;
    }

    if( okay && type != NC_CHAR )
    {
        (void) fprintf( stderr, "Error: label strings are not char type.\n");
        okay = FALSE;
    }

    /* --- read the list of label strings */

    if( okay && length_label_strings > 0 )
    {
        ALLOC( label_strings, length_label_strings );

        if( ncattget( minc_id, label_var, MIlabel_strings,
                      (void *) label_strings )
            == MI_ERROR )
            okay = FALSE;
    }

    /* --- convert the list of label strings, which is one new-line delimited
           string, into an array of strings */

    if( okay )
    {
        if( *n_labels > 0 )
            ALLOC( *labels, *n_labels );

        /* --- start_index is the beginning of the current string,
               end_index is the last valid character index of the current
               string */

        start_index = 0;
        for_less( i, 0, *n_labels )
        {
            /* --- find the end of the current string, which is the next
                   new-line, null character, or the end of the entire
                   label_string */

            end_index = start_index;
            while( end_index < length_label_strings &&
                   label_strings[end_index] != '\n' &&
                   label_strings[end_index] != (char) 0 )
                ++end_index;

            /* --- now that the length of the string is known, allocate
                   memory for it, and copy it */


            ALLOC( (*labels)[i], end_index - start_index + 1 );
            if( end_index > start_index )
            {
                (void) strncpy( (*labels)[i], &label_strings[start_index],
                                end_index - start_index );
            }

            /* --- add the ending null character */

            (*labels)[i][end_index - start_index + 1] = (char) 0;
            start_index = end_index + 1;
        }
    }

    /* --- free the label strings read from the file */

    if( length_label_strings > 0 && label_strings != NULL )
        FREE( label_strings );

    /* --- if not successful, free up any memory allocated to be passed back */

    if( !okay )
    {
        if( *labels != (char **) NULL )
        {
            for( i = 0;  i < *n_labels;  ++i )
                FREE( (*labels)[i] );
            FREE( *labels );
        }

        if( *values != (int *) NULL )
            FREE( *values );

        *n_labels = 0;
    }

    /* --- restore the netcdf options */

    ncopts = save_opts;

    return( okay );
}
