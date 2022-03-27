/*******************************************************************************
t_ringbuffer.cpp
This is a unit test file for Cpp4Scripting library.

g++ -x c++ -fno-rtti -I.. -Wall -pthread -O0 -ggdb -Wundef -Wno-unused-result -fcompare-debug-second -fexceptions -fuse-cxa-atexit -std=c++17 -o t_ringbuffer t_ringbuffer.cpp

../makec4s --dev -s t_ringbuffer.cpp
................................................................................
License: LGPLv3
Copyright (c) Menacon Ltd
*******************************************************************************/

#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <time.h>
// --
#include "../RingBuffer.hpp"
#include "../RingBuffer.cpp"

using namespace c4s;
using namespace std;

#include "run_tests.cpp"

const char *lorem70 =
"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse\n"
"et nunc tristique, egestas urna ut, ultricies massa. Suspendisse\n"
"potenti. Quisque tincidunt felis ex, ac rhoncus tortor interdum sed.\n"
"Phasellus eu justo porta, iaculis sem eu, facilisis libero. Nam\n"
"commodo mollis velit sit amet rhoncus. Suspendisse iaculis dolor sed\n"
"orci ornare, nec facilisis velit rutrum. Cras maximus a dui et\n"
"rutrum. Nunc varius risus nunc, eget aliquet odio lacinia ut.\n"
"Pellentesque sit amet lacinia sem, quis varius magna. Nam varius\n"
"sodales ultrices. Mauris semper odio ex. Curabitur sollicitudin et\n"
"est ut vestibulum. Suspendisse aliquam mauris eu dolor lacinia, sit\n"
"amet ullamcorper metus elementum. Proin sodales finibus nibh. Nullam\n"
"rhoncus metus posuere magna dapibus aliquam sit amet ut dui.";

bool test1()
{
    char copy[1024];
    // cout << "Original:\n";
    // cout << lorem70 << '\n';

    RingBuffer rb(1024);

    rb.write(lorem70, strlen(lorem70));
    size_t len = rb.read(copy, sizeof(copy));
    copy[len] = 0;

    // for(size_t ndx=0; ndx<sizeof(copy) && copy[ndx]; ndx++) {
    //     if (lorem70[ndx] != copy[ndx]) {
    //         cout << "Diff at index " << ndx << ". ch: " << copy[ndx] << '\n';
    //         return false;
    //     }
    // }
    // return true;
    return memcmp(lorem70, copy, len) == 0 ? true : false;
}

bool test2()
{
    RingBuffer rb(1024);
    rb.write(lorem70, strlen(lorem70));

    const char* ptr = lorem70;
    while(rb.read_line(cout)) {
        cout << endl;
        while(*ptr && *ptr != '\n') {
            cout << *ptr;
            ptr++;
        }
        if(*ptr == '\n')
            ptr++;
        cout << '\n';
    }
    // Get the final line
    rb.read_line(cout, true);
    cout << '\n';
    while(*ptr) {
        cout << *ptr;
        ptr++;
    }
    cout << '\n';
    return true;
}

bool test3()
{
    const size_t WRMAX = 50;
    const size_t RDMAX = 80;
    char line[RDMAX];
    RingBuffer rb(256);
    const char* wr;
    size_t len_lorem = strlen(lorem70);
    size_t wb = 0;

    cout << "Total write " << len_lorem << '\n';

    // read-write bulk
    for (wr = lorem70; wr < lorem70 + len_lorem - WRMAX; wr += WRMAX) {
        wb += rb.write(wr, WRMAX);
        while (rb.read_line(line, RDMAX)) {
            cout.write(line, rb.gcount());
            cout << '\n';
        }
    }

    // write & read last bit.
    size_t left = len_lorem - wb;
    cout << "--\nwritten " << wb << "; left " << left << '\n';
    rb.write(wr, left);
    while (rb.read_line(line, RDMAX)) {
        cout.write(line, rb.gcount());
        cout << "#\n";
    }
    if (rb.read_line(line, RDMAX, true)) {
        cout.write(line, rb.gcount());
        cout << " (" << rb.gcount() << ")\n";
    }
    return true;
}

// -------------------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    TestItem tests[] = {
        { &test1, "Write buffer full content, read buffer full content"},
        { &test2, "Write buffer full content, read by lines"},
        { &test3, "Write async, read lines"},
        { 0, 0}
    };

    return run_tests(argc, argv, tests);
}
