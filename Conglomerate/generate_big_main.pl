#!/usr/local/bin/perl

    $main = shift;

    if( ! $main )
    {
        die( "Usage: $0 output.c input1.c [input2.c] ...\n" );
    }

    $dir = $main;
    $dir =~ /(.*\/)/;
    $dir = $1;

    $main_prefix = $main;
    $main_prefix =~ s/\.c//;
    $main_prefix =~ s/.*\///;

    open( MAIN, "> $main" ) || die "Could not open output file $main";

    print( MAIN "\n" .
                "#include  <stdio.h>\n" .
                "#include  <string.h>\n" .
                "\n" );

    foreach $file ( @ARGV )
    {
        $name = $file;
        $name =~ s/\.c//;

        print( MAIN "extern  int  ${name}_main( int argc, char *argv[] );\n" .
                    "\n" );

        $new_file = "${dir}${name}_main.c";
        system( "sed -e 's/int \*main(/int ${name}_main(/' $file > $new_file" );
    }

    print( MAIN "int  main(\n" .
                "    int   argc,\n" .
                "    char  *argv[] )\n" .
                "{\n" .
                "    int  return_status, ind;\n" .
                "    char *exec_name;\n" .
                "\n" .
                "    ind = (int) strlen(argv[0]);\n" .
                "    while( ind > 0 && argv[0][ind-1] != '/' ) --ind;\n" .
                "    exec_name = &argv[0][ind];\n" .
                "    \n" );

    $first = 1;

    foreach $file ( @ARGV )
    {
        $name = $file;
        $name =~ s/\.c//;

        if( $first )
        { 
            $first = 0;
            print( MAIN "    " );
        }
        else
            { print( MAIN "    else " ); }

        print( MAIN "if( strcmp( exec_name, \"${name}\" ) == 0 )\n" .
                    "        return_status = ${name}_main( argc, argv );\n" );
    }

    print( MAIN
      "    else\n" .
      "    {\n" .
      "        int ch;\n" .
      "        \n" .
      "        (void) fprintf( stderr, \n" .
      "               \"Executable must be named one of:\\n\" );\n" );

    foreach $file ( @ARGV )
    {
        $name = $file;
        $name =~ s/\.c//;
        print( MAIN "        (void) fprintf( stderr, \"        ${name}\\n\" );\n" );
    }

    print( MAIN "
        return_status = 0;

        (void) fprintf( stderr, \"\\n\" );
        (void) fprintf( stderr,
            \"Would you like to create the links in the current directory? \" );

        ch = getchar();

        if( ch == 'y' || ch == 'Y' )
        {\n" );

    foreach $file ( @ARGV )
    {
        $name = $file;
        $name =~ s/\.c//;
        print( MAIN "            (void) system( \"ln -s $main_prefix $name\" );\n" );
    }

    print( MAIN "        }\n" .
                "    }\n" .
                "    return( return_status );\n" .
                "}\n" );

    close( MAIN );

#---- now create the Makefile list

    print( "CONGLOMERATE_OBJECTS =" );

    foreach $file ( @ARGV )
    {
        $name = $file;
        $name =~ s/\.c//;
        $new_file = "${dir}${name}_main.o";
        print( " \\\n" .
               "                       $new_file" );
    }
    print( "\n" );
