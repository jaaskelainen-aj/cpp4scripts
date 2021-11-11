/*******************************************************************************
c4s-args.cpp
This is a sample for Cpp for Scripting library. It demonstrates the use of program arguments.

# GCC Compilation - debug
g++ -o c4s-args c4s-args.cpp -Wall -ggdb -O0 -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_DEBUG -I/usr/local/include/stlport -lstlport

Copyright (c) Menacon Ltd
*******************************************************************************/

// Include the Cpp4Script files. Add the path for these files into the compile command or set it in an environment variable.
#include <cpp4scripts.hpp>
#include <cpp4scripts.cpp>
using namespace c4s;


int main(int argc, char **argv)
{
    program_arguments args;

    cout << "Argument sample program for the Cpp-for-scripting library.\n";

    args += argument("-a",  true, "Parameter with value.");
    args += argument("-b",  true, "Another parameter with value.");
    args += argument("-c",  false, "Plain (single) parameter.");
    args += argument("-d",  false, "Another single parameter.");

    try{
        args.initialize(argc,argv,1);
    }catch(runtime_error re){
        cerr << "Incorrect or too few parameters given." << re.what() << endl;
        args.usage();
        return 1;
    }

    if(args.is_set("-a"))
        cout << "Parameter -a specified. Value: "<< args.get_value("-a") << endl;
    if(args.is_set("-b"))
        cout << "Parameter -b specified. Value: "<< args.get_value("-b") << endl;
    if(args.is_set("-c"))
        cout << "Parameter -c specified.\n";
    if(args.is_set("-d"))
        cout << "Parameter -d specified.\n";

    return 0;
}
