/*! \file recpath.cpp */
// Build with:
// makec4s -s 
// Copyright (c) Menacon Oy
/********************************************************************************/

#define C4S_DEBUGTRACE
#include "../c4s_all.hpp"

using namespace c4s;

program_arguments args;
int main(int argc, char **argv)
{
    cout << "CPP4SCRIPTS - Sample for recursive path\n";

    args += argument("-source", true, "Source/Original path to compare.");
    args += argument("-target", true, "Target/New path to compare.");
    args += argument("-speck",  true, "Filespek to search.");
    try {
        args.initialize(argc,argv);
    }catch(c4s_exception ce){
        cout << "Incorrect parameters.\n"<<ce.what()<<'\n';
        return 1;
    }

    if(!args.is_set("-source") || !args.is_set("-target")) {
        cout << "Missing directories\n";
        args.usage();
        return 2;
    }

    path_list cfglst;
    cfglst.add_recursive(path(args.get_value("-source")), args.get_value("-speck").c_str());
    cout << "Found "<<cfglst.size()<<" files.\n";
    for(path_iterator pi = cfglst.begin(); pi!=cfglst.end(); pi++)
        cout << pi->get_path() << '\n';

    return 0;
}
