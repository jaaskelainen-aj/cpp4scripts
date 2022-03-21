/*******************************************************************************
process.cpp
This is a sample for Cpp4Scripting library.

g++ -x c++ -fno-rtti -I.. -Wall -pthread -O0 -ggdb -Wundef -Wno-unused-result\
 -fcompare-debug-second -fexceptions -fuse-cxa-atexit\
 -std=c++17 -o t_process t_process.cpp -lc4s -L../debug

../makec4s --dev -s t_process.cpp
................................................................................
License: LGPLv3
Copyright (c) Menacon Ltd
*******************************************************************************/

#include <string>
#include <iostream>
#include <stdexcept>
#include <time.h>
#include "../cpp4scripts.hpp"

using namespace c4s;
using namespace std;

#include "run_tests.cpp"

void test1()
{
    cout << "Run client, expect error code 10\n";
    int rv = process("./client", "-e 10")();
    if (rv != 10)
        cout << "  Failed - rv = " << rv << '\n';
    else
        cout << "  OK\n";
}

void test2()
{
    cout << "Waiting client output for 30s:\n";
    process ls("client","-w 3", PIPE::LG);
    time_t now = time(0);
    ls.start();
    while(ls.is_running()){
        while (ls.rb_out.read_line(cout))
            cout << '\n';
        if (time(0)-now > 30) {
            ls.stop();
            break;
        }
    }
    cout << "  OK\n";
}

void test3()
{
    user tu(args.get_value("-u").c_str());
    if(!tu.is_ok()) {
        cerr << "Unable to find user "<<tu.get_name()<<" from system\n";
        return;
    }
    cout << "Found user: "<<tu.get_name()<<" ("<<tu.get_uid()<<") / "<<tu.get_group()<<" ("<<tu.get_gid()<<")\n";

    string param("/tmp/");
    param += tu.get_name();
    param += ".tmp";
    process("touch", param, PIPE::NONE, &tu)();
    cout << "Created file with user's name into /tmp\n";
}

void test4()
{
    // Arg 0 : [client]
    // Arg 1 : [samis']
    // Arg 2 : [']
    // Arg 3 : [world for peace]
    // Arg 4 : [one's heart..]
    // Arg 5 : [dude's pants]
    // Arg 6 : [on]
    // Arg 7 : [fire]
    //               | samis\' \'  'world for peace' "one's heart".. 'dude\'s pants' on fire |
    process("echo"," samis\\' \\'  'world for peace'  \"one's heart\".. 'dude\\'s pants' on fire ")();
}
void test5()
{
    // Test the stdout shorthand
    process("ls", "-l", PIPE::SM)(cout);
}
#if 0
void test6()
{
    const char *parts[]={"part-1", "part-2", "part-3", 0 };
    process client("client","client call", PIPE::SM);
    for(const char **cur=parts; *cur; cur++) {
        cout << "------------------------------\n";
        client.execa(*cur);
        client.rb_out.read_into(cout);
    }
}

void test7()
{
    stringstream op;
    user postgres("postgres","postgres",true, "/home/postgres/","/bin/bash","sys");
    int status = postgres.status();
    cout << "test 7 - user postgres status: "<<status<<'\n';
    if(status!=0)
        postgres.create();
    // process pg("/usr/local/pgsql-9.1/bin/initdb","-D /opt/pgdata-9.1 --locale fi_FI.UTF-8 --lc-messages=en_US.UTF-8",&op);
    //process t("touch","/tmp/pg-test");
    //t.set_user(&postgres);
    //t();
    process pg("/usr/local/pgsql-9.1/bin/pg_ctl","-D /opt/pgdata-9.1 -w -t 20 start",&op);
    pg.set_user(&postgres);
    if(pg(30))
        cout << "PgSQL command failed.\n";
    cout << op.str()<<'\n';
}

// ..........................................................................................
void test8()
{
    istringstream is;
    ostringstream os;

    is << "1234567890abcdefg";
    cout << ">> 1234567890abcdefg\n";
    process cli("echo", "-l", &is, &os);
    cli();
    for (string line; is.getline(line); )
        cout << "<< " << line << '\n';

    cout << ">> abcdefg1234567890\n";
    is << "abcdefg1234567890\n";
    cli();
    for (string line; is.getline(line); )
        cout << "<< " << line << '\n';
}
// ..........................................................................................
void test9()
{
    process cli;
    try {
        cli.attach(path(args.get_value("-pf")));
        cli.stop();
        cout<<"Stopped\n";
    }
    catch(const process_exception& pe) {
        cout <<"Failed: "<<pe.what()<<endl;
    }
}
// -------------------------------------------------------------------------------------------------
void test10()
{
    process echo;
    echo = "./echo";
    echo += "hello";
    echo("world", &cout);
    echo("earth", &cout);
    echo.exec("universe");
}
// -------------------------------------------------------------------------------------------------
void test11()
{
    RingBuffer rb;
    string line;
    process big("./child","-big");
    for (big.start(); bit.is_running(&rb); ) {
        rb.read_line(cout);
    }
}
#endif
// -------------------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    TestItem tests[] = {
        { &test1, "Get return value from command"},
        { &test2, "List files with ls."},
        { &test3, "Create [user].tmp file into current directory by running touch as VALUE user."},
        { &test4, "Run test client with several parameters. Testing parameter parsing."},
        { &test5, "Run test client with couple of simple params. Pipe to stdout."},
        // { &test6, "Test the use of execa - running same process with varied arguments."},
        // { &test7, "Test the use of process user (linux only)"},
        // { &test8, "Test the input stream with echo-client."},
        // { &test9, "Terminate process with pid file (-pf)"},
        // { &test10, "Empty constructor, add command, add parameters"},
        // { &test11, "Read big data from client and push it to cout."},
        { 0, 0}
    };

    args += argument("-f", false, "Feed child.txt into the client-bin in test 8.");
    args += argument("-u", true,  "Set the user for test 3");
    args += argument("-pf", true, "Test 9: name the pid-file.");

    return run_tests(argc, argv, tests);
}
