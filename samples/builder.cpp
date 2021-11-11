/*! \file builder.cpp
 * \brief This is a sample for Cpp for Scripting library. 
................................................................................
License: LGPLv3 */

// Copyright (c) Menacon Ltd, Finland
/********************************************************************************/
#include "../c4s_all.hpp"
using namespace c4s;

program_arguments args;
typedef void (*tfptr)();

void test1()
{
    cout << "Updating build number file\n";
    int rv = builder::update_build_no("build_no.txt");
    if(rv) {
        cout << "Update failed with error: "<<rv<<'\n';
        return;
    }
    cout << "OK\n";
}

void test2()
{
    cout << "List cpp files in project\n";
    builder_gcc make("test2", &cout);
    make.print(cout, true);
}

// ==========================================================================================
int main(int argc, char **argv)
{
    const int tmax = 2;
    tfptr tfunc[tmax] = { &test1, &test2 };
    args += argument("-t",  true, "Sets VALUE as the test to run.");

    try{
        args.initialize(argc,argv,1);
    }catch(const c4s_exception &){
        cout << "Builder test program\n";
        args.usage();
        return 1;
    }

    if(!args.is_set("-t")) {
        cout << "Missing -t argument\n";
        return 1;
    }
    istringstream iss(args.get_value("-t"));
    int test;
    iss >> test;
    if(test>0 && test<=tmax)
        tfunc[test-1]();
    else
        cout << "Unknown test number: "<<test<<'\n';

    return 0;

    return 0;
}
