/*******************************************************************************
process_t.cpp
This is a sample and unit test file for Cpp4Scripting library.

g++ -x c++ -fno-rtti -I.. -Wall -pthread -O0 -ggdb -Wundef -Wno-unused-result\
 -fcompare-debug-second -fexceptions -fuse-cxa-atexit\
 -DC4S_DEBUGTRACE -std=c++17 -o process_t process_t.cxx -lc4s -L../debug

../makec4s --dev -s process_t.cxx
................................................................................
License: LGPLv3
Copyright (c) Menacon Ltd
*******************************************************************************/

#include <string>
#include <iostream>
#include <stdexcept>
#include <time.h>

// #include "../cpp4scripts.hpp"
#include "../ntbs.cpp"
#include "../path.hpp"
#include "../RingBuffer.hpp"
#include "../RingBuffer.cpp"
#include "../process.hpp"
#include "../process.cpp"

using namespace c4s;
using namespace std;

#include "run_tests.cpp"

bool test1()
{
    cout << "Run client, expect error code 10\n";
    int rv = process("./client", "-e 10")();
    if (rv != 10) {
        cout << "  Failed - rv = " << rv << '\n';
        return false;
    }
    cout << "  OK\n";
    return false;
}

bool test2()
{
    size_t count = 0;
    char gitline[255];
    process git("git", "ls-files", PIPE::LG);
    for (git.start(); git.is_running(); ) {
        while (git.rb_out.read_line(gitline, sizeof(gitline)) ) {
            if (strstr(gitline, ".cpp")) {
                cout << gitline << '\n';
                count++;
            }
        }
    }
    if (count > 10) {
        return true;
    }
    return false;
}

bool test3()
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
    process("echo"," samis\\' \\'  'world for peace'  \"one's heart\".. 'dude\\'s pants' on fire ")(cout);
    return true;
}

bool test4()
{
    // Test the output stream
    process("ls", "-l", PIPE::SM)(cout);
    return true;
}

bool test5()
{
    ntbs original("Curabitur vehicula arcu sit amet dolor placerat pharetra ut vel metus.\n"
    " Aenean ac ultrices sem, at accumsan nibh. Sed sed pellentesque ligula.\n"
    " In sed metus tincidunt massa lobortis condimentum in quis velit. Morbi luctus rhoncus ornare.");
    ntbs copy(256);
    process::query("client", "-c", &original, &copy);

    cout << original.get() << '\n';
    cout << copy.get() << '\n';

    return true;
}

#if 0

bool test5()
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

    return true;
}

bool test6()
{
    const char *parts[]={"part-1", "part-2", "part-3", 0 };
    process client("client","client call", PIPE::SM);
    for(const char **cur=parts; *cur; cur++) {
        cout << "------------------------------\n";
        client.execa(*cur);
        client.rb_out.read_into(cout);
    }
}

bool test7()
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
bool test8()
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
bool test9()
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
bool test10()
{
    process echo;
    echo = "./echo";
    echo += "hello";
    echo("world", &cout);
    echo("earth", &cout);
    echo.exec("universe");
}
// -------------------------------------------------------------------------------------------------
bool test11()
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
        { &test2, "List cpp-files via git ls-files."},
        { &test3, "Run test client with several parameters. Testing parameter parsing."},
        { &test4, "Run test client with couple of simple params. Pipe to stdout."},
        { &test5, "Static process::query."},
        // { &test3, "Create [user].tmp file into current directory by running touch as VALUE user."},
        // { &test6, "Test the use of execa - running same process with varied arguments."},
        // { &test7, "Test the use of process user (linux only)"},
        // { &test8, "Test the input stream with echo-client."},
        // { &test9, "Terminate process with pid file (-pf)"},
        // { &test10, "Empty constructor, add command, add parameters"},
        // { &test11, "Read big data from client and push it to cout."},
        { 0, 0}
    };

    return run_tests(argc, argv, tests);
}
