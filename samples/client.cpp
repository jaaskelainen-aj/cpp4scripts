/*******************************************************************************
client.cpp
Program to test the Cpp4Scripts process class
Copyright (c) Menacon Ltd
*******************************************************************************/
#include <iostream>
#include <stdio.h>
#include <string.h>
using namespace std;
#if defined(__linux) || defined(__APPLE__)
 #include <sys/types.h>
 #include <unistd.h>
#else
 #include <windows.h>
#endif
#include "../c4s_all.hpp"

program_arguments args;

int main(int argc, char **argv)
{
    cout << "C4S test client\n";

    args += argument("-w",  false, "Waits until SIG_TERM.");
    try{
        args.initialize(argc,argv,1);
    }catch(c4s_exception re){
        args.usage();
        return 1;
    }
    ofstream log("client.log");
    if(!log) {
        cout << "Failed to open output log. Aborted.\n";
        return 1;
    }
#ifdef __linux
    log << "Current real UID:"<<getuid()<<'\n';
    log << "Current real GID:"<<getgid()<<'\n';
    log << "Current effective UID:"<<geteuid()<<'\n';
    log << "Current effective GID:"<<getegid()<<'\n';
    log << endl;
#endif

    if(args.is_set("-w")) {
        cout<<"Waiting until terminated..."<<endl;
        while(1)
            sleep(1);
    }
    else {
        if(c4s::wait_stdin(500)) {
            log << "Detected stdin input. Reading untill EOF."<<endl;
            char ch;
            while(!feof(stdin)) {
                fread(&ch,1,1,stdin);
                if(ch == '\n') log << endl;
                else log << ch;
            }
            log << "\n<< Done with input.\n"<<endl;
        }
        // Print the arguments
        for(int ndx=0; ndx<argc; ndx++) {
            log << "  Arg "<<ndx<<" : ["<<argv[ndx]<<"]\n";
        }
    }
    cout << "Client done.\n";
    return 0;
}
