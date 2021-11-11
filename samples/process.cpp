/*******************************************************************************
process.cpp

This is a sample for Cpp4Scripting library. It demonstrates the use of program
arguments and processes.

Copyright (c) Menacon Ltd
*******************************************************************************/

#include <string>
using namespace std;
#define C4S_DEBUGTRACE
#include "../c4s_all.hpp"
using namespace c4s;

program_arguments args;
typedef void (*tfptr)();

void test1()
{
    // List all files in this directory using 'add-hock' process.
#ifdef __linux
    process("ls","-l",&cout)();
#else
    process("cmd","/C dir",&cout)();
#endif
}

void test2()
{
    ostringstream sout;
    cout << "Running sample client\n";
    process("./client", "hello world",&sout)();
    cout << "Client done. Output:>>\n"<<sout.str()<<'\n';
}

void test3()
{
#ifdef __linux
    user tu(args.get_value("-u").c_str());
    if(!tu.is_ok()) {
        cerr << "Unable to find user "<<tu.get_name()<<" from system\n";
        return;
    }
    cout << "Found user: "<<tu.get_name()<<" ("<<tu.get_uid()<<") / "<<tu.get_group()<<" ("<<tu.get_gid()<<")\n";
    process client("./client");
    client.set_user(&tu);
    client.pipe_to(&cout);
    client();
    string param("/tmp/");
    param += tu.get_name();
    param += ".tmp";
    process touch("touch", param); //
    touch.set_user(&tu);
    touch();
    cout << "Created file with user's name into /tmp\n";
#else
    cout << "Sorry, test3 does not work in this environment.\n";
#endif
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
    process("client"," samis\\' \\'  'world for peace'  \"one's heart\".. 'dude\\'s pants' on fire ",&cout)();
    // Arg 0 : [-c]
    // Arg 1 : [\du admins]
    //               |-c "\du admins"|
    process("client","-c \"\\du admins\"",&cout)();
}

void test5()
{
    path space("C:\\Program Files\\hellow.exe");
    string para(space.get_path_quot());
    para += " cheese";
    process client("client",para.c_str(),&cout);
    client();
}

void test6()
{
    const char *parts[]={"part-1", "part-2", "part-3", 0 };
    process client("client","client call",&cout);
    for(const char **cur=parts; *cur; cur++) {
        cout << "------------------------------\n";
        client.execa(*cur);
    }
}

void test7()
{
#ifdef __linux
    ostringstream op;
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
#else
    cout << "Sorry, this test is only for Linux\n";
#endif
}

// ..........................................................................................
void test8()
{
    ostringstream os;
    process cli("client",0,&os);
    if(args.is_set("-f"))
        cli.pipe_from(path("child.txt"));
    cli.start();
    if(!args.is_set("-f")) {
        cli.pipe_send("This is the\ntext to client:\nfamous hello world\nis always good.\n");
        cli.pipe_send_close();
    }
    cli.wait_for_exit(15);
    cout << "Received from client:"<<os.str()<<'\n';
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
    catch(process_exception pe) {
        cout <<"Failed: "<<pe.what()<<endl;
    }
}
// -------------------------------------------------------------------------------------------------
void test10()
{
    process echo;
    echo.pipe_default();
    echo = "./echo";
    echo += "hello";
    echo("world");
    echo("earth");
    echo.exec("universe");
}
// -------------------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    const int tmax = 10;
    tfptr tfunc[tmax] = { &test1, &test2, &test3, &test4, &test5, &test6, &test7, &test8, &test9, &test10 };

    const char *title = "Cpp4Scripts - Process sample and test program";
    const char *info = "Following tests have been defined:\n"               \
        " 1 = List all files in this directory using ls or cmd command.\n" \
        " 2 = Use 7z to archive all *.cpp files in current directory.\n" \
        " 3 = Create [user].tmp file into current directory by running touch as VALUE user.\n" \
        " 4 = Run test client with several parameters. Testing parameter parsing.\n" \
        " 5 = Run test client with couple of simple params. Pipe to stdout.\n" \
        " 6 = Test the use of execa - running same process with varied arguments.\n" \
        " 7 = Test the use of process user (linux only)\n"              \
        " 8 = Test the input stream with client.\n"\
        " 9 = Terminate process with pid file (-pf)\n"\
        "10 = Empty constructor, add command, add parameters\n";

    args += argument("-t",  true, "Sets VALUE as the test to run.");
    args += argument("-l", false, "Append -l parameter to ls command in test 1.");
    args += argument("-f", false, "Feed child.txt into the client-bin in test 8.");
    args += argument("-u", true,  "Set the user for test 3");
    args += argument("-pf", true, "Test 9: name the pid-file.");

    try{
        args.initialize(argc,argv,1);
    }catch(c4s_exception re){
        cerr << "Incorrect or missing parameter: " << re.what() << endl;
        cout << title <<'\n';
        args.usage();
        cout << info;
        return 1;
    }
    if(!args.is_set("-t")) {
        cout << "Missing -t argument\n";
        return 1;
    }
    istringstream iss(args.get_value("-t"));
    int test;
    iss >> test;
    try {
        if(test>0 && test<=tmax)
            tfunc[test-1]();
        else
            cout << "Unknown test number: "<<test<<'\n';
    }catch(process_exception pe){
        cerr << "Process failure: "<<pe.what()<<endl;
        return 1;
    }
    cout << "test completed.\n";
    return 0;
}
