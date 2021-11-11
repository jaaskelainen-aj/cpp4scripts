/***************************************************************************************************
c4s-logger.cpp

This is a sample for Cpp for Scripting library. It demonstrates the use of application logger.
....................................................................................................
License: LGPLv3
Copyright (c) Menacon Ltd
****************************************************************************************************/

//#define C4S_LOG_LEVEL 8
#include "../c4s_all.hpp"
using namespace c4s;

program_arguments args;

// -------------------------------------------------------------------------------------------------
void test1()
{
    cout << "Log into file stream with macros\n";
    path lfile(args.get_value("-f"));
    try {
        LOG_LEVEL ll = args.is_set("-l") ? logbase::str2level(args.get_value("-l").c_str()) : LL_INFO;
        logbase::init_log(ll, new fstream_sink(lfile));
    }catch(c4s_exception ce) {
        cout << "Unable to open log: "<<ce.what()<<'\n';
        return;
    }

    CS_PRINT_TRCE("This is CS_PRINT_TRCE");
    CS_PRINT_DEBU("This is CS_PRINT_DEBU");
    CS_PRINT_INFO("This is CS_PRINT_INFO");
    CS_PRINT_NOTE("This is CS_PRINT_NOTE");
    CS_PRINT_WARN("This is CS_PRINT_WARN");
    CS_PRINT_ERRO("This is CS_PRINT_ERRO");
    CS_PRINT_CRIT("This is CS_PRINT_CRIT");
    int n=1;
    CS_VAPRT_TRCE("This is log %d as CS_VAPRT_TRCE",n++);
    CS_VAPRT_DEBU("This is log %d as CS_VAPRT_DEBU",n++);
    CS_VAPRT_INFO("This is log %d as CS_VAPRT_INFO",n++);
    CS_VAPRT_NOTE("This is log %d as CS_VAPRT_NOTE",n++);
    CS_VAPRT_WARN("This is log %d as CS_VAPRT_WARN",n++);
    CS_VAPRT_ERRO("This is log %d as CS_VAPRT_ERRO",n++);
    CS_VAPRT_CRIT("This is log %d as CS_VAPRT_CRIT",n);

    logbase::close_log();
    cout << "Logging completed.\n";
}
// -------------------------------------------------------------------------------------------------
void test2()
{
    logbase *log;
    cout << "Log with low level io using direct interface\n";
    path lfile(args.get_value("-f"));
#ifdef __linux
    lowio_sink::mode = S_IRUSR|S_IWUSR;
#endif
    try {
        LOG_LEVEL ll = args.is_set("-l") ? logbase::str2level(args.get_value("-l").c_str()) : LL_INFO;
        log = new logbase(ll,new lowio_sink(lfile));
    }catch(c4s_exception ce) {
        cout << "Unable to open log:"<<ce.what()<<'\n';
        return;
    }
    log->print(CS_TRCE,"This is TRCE");
    log->print(CS_DEBU,"This is DEBU");
    log->print(CS_INFO,"This is INFO");
    log->print(CS_NOTE,"This is NOTE");
    log->print(CS_WARN,"This is WARN");
    log->print(CS_ERRO,"This is ERRO");
    log->print(CS_CRIT,"This is CRIT");

    delete log;
    cout << "Logging completed\n";
}
// -------------------------------------------------------------------------------------------------
void test3()
{
    cout << "Log to stderr using buffered interface\n";
    try {
        LOG_LEVEL ll = args.is_set("-l") ? logbase::str2level(args.get_value("-l").c_str()) : LL_INFO;
        logger::init_log(ll, new stderr_sink());
    }catch(c4s_exception ce) {
        cout << "Logger init failed: "<<ce.what()<<endl;
        return;
    }
    int ndx=1;
    CSLOG << "log entry "<<ndx++<<" to TRACE   " << CS_TRCE;
    CSLOG << "log entry "<<ndx++<<" to DEBUG   " << CS_DEBU;
    CSLOG << "log entry "<<ndx++<<" to INFO    " << CS_INFO;
    CSLOG << "log entry "<<ndx++<<" to NOTICE  " << CS_NOTE;
    CSLOG << "log entry "<<ndx++<<" to WARNING " << CS_WARN;
    CSLOG << "log entry "<<ndx++<<" to ERROR   " << CS_ERRO;
    CSLOG << "log entry "<<ndx++<<" to CRITICAL" << CS_CRIT;
    logger::close_log();
    cout << "Logging completed\n";
}

typedef void (*tfptr)();
// ==========================================================================================
int main(int argc, char **argv)
{
    const int TMAX=3;
    tfptr tfunc[TMAX] = { &test1, &test2, &test3 };

    const char *info = "Following tests have been defined:\n"\
        " 1 = Std file stream logging using log macros.\n"\
        " 2 = Low level io with direct interface.\n"\
        " 3 = Buffered log to stderr.\n";

    args += argument("-t",  true, "Sets VALUE as the test to run.");
    args += argument("-l",  true, "Sets VALUE as log level [TRACE|DEBUG|INFO|NOTICE|WARNING|ERROR|CRIT] for application.");
    args += argument("-f",  true, "Sets VALUE as log file name. Use with test 1");

    cout << "Cpp4Scripts - logger sample and test program\n";
    try{
        args.initialize(argc,argv,1);
    }catch(c4s_exception){
        args.usage();
        cout << info;
        return 1;
    }

    if(!args.is_set("-t")) {
        cout << "Missing test number (-t).\n";
        return 1;
    }
    if(!args.is_set("-l")) {
        cout << "Missing log level (-l).\n";
        return 1;
    }
    istringstream iss(args.get_value("-t"));
    int test;
    iss >> test;
    if(test>0 && test<=TMAX)
        tfunc[test-1]();
    else
        cout << "Unknown test number: "<<test<<endl;

    return 0;
}
