#include  <bicpl.h>

private  int  count_tags(
    int             n_objects,
    object_struct   *object_list[],
    int             *ids[],
    int             *counts[] );

int  main(
    int   argc,
    char  *argv[] )
{
    Status               status;
    char                 *tag_filename;
    int                  n_objects, total_tags;
    object_struct        **object_list;
    int                  i, n_ids, *ids, *counts;

    initialize_argument_processing( argc, argv );

    if( !get_string_argument( "", &tag_filename ) )
    {
        print( "Need tag file argument.\n" );
        return( 1 );
    }

    status = input_objects_any_format( (Volume) NULL, tag_filename,
                                       GREEN, 1.0, BOX_MARKER,
                                       &n_objects, &object_list );

    n_ids = count_tags( n_objects, object_list, &ids, &counts );

    print( "Tag file: %s\n", tag_filename );
    print( "------------------\n" );

    total_tags = 0;
    for_less( i, 0, n_ids )
    {
        print( "Structure id: %4d     Number tags: %5d\n", ids[i], counts[i] );
        total_tags += counts[i];
    }

    print( "                                    -----\n" );
    print( "                 Total Number tags: %5d\n", total_tags );

    if( total_tags != n_objects )
        print( "File %s contains %d non-tag objects.\n", tag_filename,
               n_objects - total_tags );

    return( status != OK );
}

private  int  add_structure_id(
    int      structure_id,
    int      *n_ids,
    int      *ids[],
    int      *counts[] )
{
    int   i, id_index;

    id_index = 0;

    while( id_index < *n_ids && structure_id > (*ids)[id_index] )
        ++id_index;

    if( id_index >= *n_ids || (*ids)[id_index] != structure_id )
    {
        SET_ARRAY_SIZE( *ids, *n_ids, *n_ids + 1, DEFAULT_CHUNK_SIZE );
        SET_ARRAY_SIZE( *counts, *n_ids, *n_ids + 1, DEFAULT_CHUNK_SIZE );

        ++(*n_ids);
        for( i = (*n_ids-1);  i > id_index;  --i )
        {
            (*ids)[i] = (*ids)[i-1];
            (*counts)[i] = (*counts)[i-1];
        }

        (*ids)[id_index] = structure_id;
        (*counts)[id_index] = 0;
    }

    ++(*counts)[id_index];
}

private  int  count_tags(
    int             n_objects,
    object_struct   *object_list[],
    int             *ids[],
    int             *counts[] )
{
    int    i, n_ids, structure_id;

    n_ids = 0;

    for_less( i, 0, n_objects )
    {
        if( object_list[i]->object_type == MARKER )
        {
            structure_id = get_marker_ptr(object_list[i])->structure_id;

            add_structure_id( structure_id, &n_ids, ids, counts );
        }
    }

    return( n_ids );
}
