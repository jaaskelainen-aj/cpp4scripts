/**
 * Build: ../makec4s -deb -c4s .. -s build_check_includes.cpp
 */

#include <iostream>
#include "../cpp4scripts.hpp"

using namespace std;
using namespace c4s;

// These are for testing
const char* cpp_list = "build_test1.cpp build_test2.cpp";

int main(int argc, char **argv)
{
    try {
        path_list cpp(cpp_list, ' ');
        builder_gcc make(&cpp, "build_test", &cout);
        make.add(BUILD::DEB | BUILD::VERBOSE);
        make.build();
    }
    catch(const c4s_exception& ce) {
        cout<<"build failed: "<<ce.what()<<'\n';
        return -1;
    }
    return 0;
}
