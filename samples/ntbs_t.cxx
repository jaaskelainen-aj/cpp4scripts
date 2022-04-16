/*
ntbs_t.cxx

g++ -x c++ -fno-rtti -I.. -Wall -pthread -O0 -ggdb -Wundef -Wno-unused-result\
 -fcompare-debug-second -fexceptions -fuse-cxa-atexit\
 -DC4S_DEBUGTRACE -std=c++17 -o ntbs_t ntbs_t.cxx

 */
#include <cstring>
#include <iostream>
#include "../ntbs.cpp"

using namespace c4s;
using namespace std;

#include "run_tests.cpp"

bool test1()
{
    ntbs empty(50);
    ntbs consted("This is a constant string wrapped with ntbs-class");
    NTBS(hosted, 50);

    empty.dump(cout);
    consted.dump(cout);
    hosted.dump(cout);

    empty = "Constant string to empty";
    consted = "Constant string to constant";
    hosted = "Constant string to constant. This is too long for initial 50 chars.";

    empty.dump(cout);
    consted.dump(cout);
    hosted.dump(cout);

    return true;
}

int main(int argc, char *argv[])
{
    TestItem tests[] = {
        { &test1, "Constructors / Destructors"},
        { 0, 0}
    };

    return run_tests(argc, argv, tests);
}