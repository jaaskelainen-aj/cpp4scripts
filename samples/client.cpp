/*******************************************************************************
client.cpp
Program to test the Cpp4Scripts process class

g++ -x c++ -fno-rtti -I.. -Wall -pthread -O0 -ggdb -Wundef -Wno-unused-result\
 -fcompare-debug-second -fexceptions -fuse-cxa-atexit\
 -std=c++17 -o client client.cpp -lc4s -L../debug

Copyright (c) Menacon Ltd
*******************************************************************************/
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "../cpp4scripts.hpp"

using namespace std;
using namespace c4s;

program_arguments args;
ofstream c4slog;

int main(int argc, char **argv)
{
    args += argument("-uid",false, "Put user and group id into log.");
    args += argument("-w",  true,  "Outputs a line of text every <VALUE> seconds until 10 lines sent.");
    args += argument("-e",  true,  "Return error code <VALUE>.");
    args += argument("-c",  false, "Waits a second and then copies stdin to stdout.");
    try{
        args.initialize(argc,argv);
    }catch(const c4s_exception& re){
        cout << "Cpp4Scripts test client\n";
        args.usage();
        return 1;
    }
    if (args.is_set("-e")) {
        return stoul(args.get_value("-e"));
    }
    if (args.is_set("-w")) {
        int ndx = 1;
        long tm = stoul(args.get_value("-w"));
        cout << "Entring send loop with "<< tm <<"s interval.\n";
        while(ndx<11) {
            sleep(tm);
            cout << "Sending line " << ndx << " through stdout" << endl;
            // cerr << "Sending line " << ndx << " through stderr\n";
            ndx++;
        }
        return 0;
    }
    ofstream log("client.log");
    if(!log) {
        cout << "Failed to open output log. Aborted.\n";
        return 1;
    }
    if (args.is_set("-uid")) {
        log << "Current real UID:"<<getuid()<<'\n';
        log << "Current real GID:"<<getgid()<<'\n';
        log << "Current effective UID:"<<geteuid()<<'\n';
        log << "Current effective GID:"<<getegid()<<'\n';
        log << endl;
    }
    if (args.is_set("-c")) {
        log << "stdin bytes:"<<endl;
        char buffer[256];
        streamsize br;
        do {
            cin.read(buffer, sizeof(buffer));
            br = cin.gcount();
            if (br) {
                cout.write(buffer, br);
                log.write(buffer, br);
            }
        } while (br > 0);
    }
    log.close();
    return 0;
}
