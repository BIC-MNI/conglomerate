#include  <internal_volume_io.h>

/* ----------------------------------------------------------------------------
               define the minc variable and attribute names
---------------------------------------------------------------------------- */

#define  MIlabel_lookup   "label_lookup"
#define  MIlabel_values   "label_values"
#define  MIlabel_strings  "label_strings"

/* ----------------------------- MNI Header -----------------------------------
@NAME       : get_label_lookup_var
@INPUT      : minc_id
@OUTPUT     : label_var
@RETURNS    : TRUE if found
@DESCRIPTION: Looks up the label_lookup variable, returning TRUE if it exists.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Nov. 29, 1994    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  BOOLEAN  get_label_lookup_var(
    int   minc_id,
    int   *label_var )
{
    int   save_opts;

    save_opts = ncopts;
    ncopts = 0;

    *label_var = ncvarid( minc_id, MIlabel_lookup );

    ncopts = save_opts;

    return( *label_var != MI_ERROR );
}

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
@CREATED    : Nov. 29, 1994            David MacDonald
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

    if( !get_label_lookup_var( minc_id, &label_var ) )
    {
        ncopts = save_opts;        /* --- this is okay, return 0 labels */
        return( TRUE );
    }

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

/* ----------------------------- MNI Header -----------------------------------
@NAME       : write_label_lookup
@INPUT      : minc_id
            : n_labels   - number of labels to write
              values     - volume value of each label
              labels     - string value of each label
@OUTPUT     : 
@RETURNS    : TRUE if successful
@DESCRIPTION: Writes the label lookup to the open MINC file.  The label
              lookup is simply a list of volume values, and corresponding
              unique string labels.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Nov. 29, 1994            David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  BOOLEAN  write_label_lookup(
    int    minc_id,
    int    n_labels,
    int    values[],
    char   *labels[] )
{
    BOOLEAN   okay;
    int       i, c, start_index, len;
    char      *label_strings;
    int       label_var, save_opts, length_label_strings;

    /* --- initialize the labels to null */

    okay = TRUE;

    /* --- find the total length of all label strings */

    length_label_strings = 0;

    for_less( i, 0, n_labels )
        length_label_strings += 1 + strlen( labels[i] );

    ALLOC( label_strings, length_label_strings );

    /* --- merge the label strings into a single newline-delimited array */

    start_index = 0;
    for_less( i, 0, n_labels )
    {
        /* --- copy the label into place, changing newlines to spaces */

        len = strlen( labels[i] );
        for_less( c, 0, len )
        {
            if( labels[i][c] == '\n' )
                label_strings[start_index] = ' ';
            else
                label_strings[start_index] = labels[i][c];
            ++start_index;
        }

        /* --- put a newline at the end of each string */

        label_strings[start_index] = '\n';
        ++start_index;
    }

    /* --- set netcdf to verbose mode, to print an errors in variables, etc. */

    save_opts = ncopts;
    ncopts = NC_VERBOSE;

    /* --- check if label variable exists */

    if( !get_label_lookup_var( minc_id, &label_var ) )
    {
        /* --- define the label variable */

        label_var = ncvardef( minc_id, MIlabel_lookup, NC_DOUBLE, 0, NULL );

        if( label_var == MI_ERROR )
            okay = FALSE;
    }

    /* --- put the label_values attribute of the label variable */

    if( okay &&
        ncattput( minc_id, label_var, MIlabel_values, NC_LONG, n_labels,
                  (void *) values ) == MI_ERROR )
        okay = FALSE;

    /* --- put the list of label strings */

    if( okay && ncattput( minc_id, label_var, MIlabel_strings, NC_CHAR,
                          length_label_strings, (void *) label_strings )
            == MI_ERROR )
        okay = FALSE;

    /* --- free the label strings read from the file */

    if( length_label_strings > 0 )
        FREE( label_strings );

    /* --- restore the netcdf options */

    ncopts = save_opts;

    return( okay );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : lookup_label_value
@INPUT      : n_labels
              values
              value
@OUTPUT     :
@RETURNS    : index if found, -1 otherwise 
@DESCRIPTION: Searches for value in the array values, returning the index or
              -1 if not found.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Nov. 29, 1994    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  int   lookup_label_value(
    int    n_labels,
    int    values[],
    int    value )
{
    int   i;

    for_less( i, 0, n_labels )
        if( values[i] == value )
            return( i );

    return( -1 );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : lookup_label
@INPUT      : n_labels
              labels
              label
@OUTPUT     :
@RETURNS    : index if found, -1 otherwise 
@DESCRIPTION: Searches for label in the array labels, returning the index or
              -1 if not found.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Nov. 29, 1994    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

private  int   lookup_label(
    int    n_labels,
    char   *labels[],
    char   label[] )
{
    int   i;

    for_less( i, 0, n_labels )
        if( strcmp( labels[i], label ) == 0 )
            return( i );

    return( -1 );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : add_label_to_list
@INPUT      : n_labels
              values
              labels
              value_to_add
              label_to_add
@OUTPUT     : 
@RETURNS    : 
@DESCRIPTION: Adds the value-label pair to the list of values and labels.
              If the specified value already exists in the values array,
              then the corresponding label is replaced by label_to_add.
              Otherwise, the value-label pair is added to the end of the array.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Nov. 29, 1994    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  void  add_label_to_list(
    int    *n_labels,
    int    *values[],
    char   **labels[],
    int    value_to_add,
    char   label_to_add[] )
{
    int    ind;

    /* --- see if the value is already in the list */

    ind = lookup_label_value( *n_labels, *values, value_to_add );

    if( ind < 0 )
    {
        SET_ARRAY_SIZE( (*values), *n_labels, *n_labels+1, 1 );
        SET_ARRAY_SIZE( (*labels), *n_labels, *n_labels+1, 1 );
        ind = *n_labels;
        ++(*n_labels);
    }
    else
        FREE( (*labels)[ind] );
 
    (*values)[ind] = value_to_add;

    ALLOC( (*labels)[ind], strlen( label_to_add ) + 1 );
    (void) strcpy( (*labels)[ind], label_to_add );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : lookup_value_for_label
@INPUT      : n_labels
              values
              labels
              label
@OUTPUT     : value
@RETURNS    : TRUE if found
@DESCRIPTION: Searches for the specified label in the list.  If it is found,
              passes back the value corresponding to this label.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Nov. 29, 1994    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  BOOLEAN  lookup_value_for_label(
    int    n_labels,
    int    values[],
    char   *labels[],
    char   label[],
    int    *value )
{
    int   i;

    i = lookup_label( n_labels, labels, label );

    if( i >= 0 )
        *value = values[i];

    return( i >= 0 );
}

/* ----------------------------- MNI Header -----------------------------------
@NAME       : lookup_label_for_value
@INPUT      : n_labels
              values
              labels
              value
@OUTPUT     : label
@RETURNS    : TRUE if found
@DESCRIPTION: Searches for the specified value in the list.  If it is found,
              passes back a pointer to the label corresponding to this value.
@METHOD     : 
@GLOBALS    : 
@CALLS      : 
@CREATED    : Nov. 29, 1994    David MacDonald
@MODIFIED   : 
---------------------------------------------------------------------------- */

public  BOOLEAN  lookup_label_for_value(
    int    n_labels,
    int    values[],
    char   *labels[],
    int    value,
    char   *label[] )
{
    int   i;

    i = lookup_label_value( n_labels, values, value );

    if( i >= 0 )
        *label = labels[i];

    return( i >= 0 );
}

public  void  print_label(
    int    value,
    char   label[] )
{
    print( "%6d %s\n", value, label );
}
