/*******************************************************************************
client.cpp
Program to test the Cpp4Scripts process class
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

int main(int argc, char **argv)
{
    args += argument("-w",  false, "Waits for 5s before reading stdin");
    args += argument("-uid",false, "Put user and group id into log.");
    try{
        args.initialize(argc,argv);
    }catch(const c4s_exception& re){
        cout << "Cpp4Scripts test client\n";
        args.usage();
        return 1;
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
    if(args.is_set("-w")) {
        for(int n=0; n<5; n++) {
            sleep(1);
        }
    }
    ostringstream os;
    if(c4s::wait_stdin(500)) {
        log << "Detected stdin input. Reading untill EOF."<<endl;
        char ch;
        while(!feof(stdin)) {
            if(fread(&ch,1,1,stdin)) {
                log << ch;
                os << ch;
            }
        }
        log << "\n<< Done with input.\n"<<endl;
        cout << os.str();
    }
    return 0;
}
