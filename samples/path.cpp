/*******************************************************************************
path.cpp
This is a sample for Cpp for Scripting library.
Use of path and path_list is tested and demonstrated.
................................................................................
License: LGPLv3
Copyright (c) Menacon Ltd, Finland
*******************************************************************************/

#include <sstream>
#include <fstream>
#include "../cpp4scripts.hpp"

using namespace std;
using namespace c4s;

#include "run_tests.cpp"

// -------------------------------------------------------------------------------------------------
void test1()
{
    try{
        path target("c4stest/");
        if(!target.exists())
            target.mkdir();
        cout << "Copying *.cpp  to ./c4stest/ ...";
        path_list pl(path("./"), "*.cpp");
        pl.copy_to(target);
        pl.set_dir(target);
        pl.chmod(0x600);
    }catch(const path_exception &pe){
        cerr << "\nPath failure: "<<pe.what()<<'\n';
        return;
    }
    cout << "OK\n";
}

// -------------------------------------------------------------------------------------------------
void test2()
{
    string key;
    path target("c4stest/");
    try {
        if(!target.exists())
            target.mkdir();
        // Lets make few more temp files
        path sub("c4test/sub/");
        sub.mkdir();
        for(int ndx=0; ndx<10; ndx++) {
            ostringstream oss;
            oss << "tmp_"<<ndx<<".txt";
            sub.set_base(oss.str());
            ofstream tmp(sub.get_path().c_str());
            if(tmp) {
                tmp << "This is c4s test file "<<ndx;
                tmp.close();
            }
        }
        cout << "Press [enter] to continue";
        cin >> key;
        // Now remove everything.
        target.rmdir(true);
    } catch( const path_exception &pe) {
        cerr << "Test 2 fail: "<<pe.what()<<'\n';
        return;
    }
    cout << "Test 2 OK\n";
}

// -------------------------------------------------------------------------------------------------
void test3()
{
    try {
        path orig("c4s-path.cpp");
        path targ("c4s-path.tmp"); //, user("testu"), 0x755);
        orig.cp(targ);
    }catch(const c4s_exception &pe){
        cerr << "Test 3 failed: "<<pe.what()<<'\n';
        return;
    }
    cout << "Test 3 OK\n";
}
// -------------------------------------------------------------------------------------------------
void test4()
{
#ifndef _WIN32
    user udir("c4s_dir","users");
    user ufile("c4s_file","users");

    cout << "Creating temp users: c4s_dir, c4s_file\n";
    udir.create();
    ufile.create();
    const char* fname = "/tmp/long/test/path/file.txt";
    cout << "Target file: " << fname << '\n';
    path lp(fname, &ufile, 0x660);
    try {
        lp.mkdir(&udir, 0x755);
        ofstream of(lp.get_path().c_str());
        of << "Test file created\n";
        of.close();
        lp.ch_owner_mode();

        user rpowner;
        path rp(fname, &rpowner);
        rp.read_owner_mode();

        if (lp.get_path().compare(rp.get_path()) != 0)
            cout << "Name mismatch.\n";
        else if (lp.get_mode() != rp.get_mode())
            cout << "Mode mismatch\n";
        else if (!rpowner.match(ufile)) {
            cout << "User mismatch\n";
            rpowner.dump(cout);
        }
        else
            cout << "Test OK.\n";
    }catch(const c4s_exception &ce) {
        cerr << "Test 4 failed: "<<ce.what()<<'\n';
    }
#else
    cout << "Not supported in this environment!\n";
#endif
}
// -------------------------------------------------------------------------------------------------
void test5()
{
    path tmp1("tmp1/");
    path tmp2("tmp2/");
    path lnk("link1");
    try {
        cout << "Creating temporary dirs and links\n";
        if(!tmp1.dirname_exists())
            tmp1.mkdir();
        if(!tmp2.dirname_exists())
            tmp2.mkdir();
        if(!lnk.exists())
            tmp1.symlink(lnk);

        path_list pl1(path("./"), string());
        cout << "This dir has "<<pl1.size()<<" files.\n";
        path_list pl2(path("./"), 0, PLF_DIRS);
        cout << "Including sub dirs it has "<<pl2.size()<<" files.\n";
#ifdef __linux
        path_list pl3(path("./"), 0, PLF_NOREG|PLF_SYML);
        cout << "This dir has "<<pl3.size()<<" symbolic links.\n";
#endif
    }catch(const c4s_exception &ce) {
        cerr << "Test 5 failed: "<<ce.what()<<'\n';
        return;
    }
}

// -------------------------------------------------------------------------------------------------
void test6()
{
    path parent("../"), current("./");
    parent.cd();
    path samples("samples/");
    path_list cpp(current,"\\.cpp$");
    cpp.add(samples,"\\.cpp$");

    cout << "List of cpp files in CPP4Scripts and samples:\n";
    for(path_iterator pi=cpp.begin(); pi!=cpp.end(); pi++)
        cout << pi->get_path() << '\n';
    cout << "Total "<<cpp.size()<<" files.\n";
}

// -------------------------------------------------------------------------------------------------
void test7()
{
    path s1("sample1.txt");
    path s2("sample2.txt");
    try {
        s1.cat(s2);
    }catch(const c4s_exception &ce) {
        cout << "cat failed:"<<ce.what()<<'\n';
        return;
    }
    cout << "OK\n";
}

// -------------------------------------------------------------------------------------------------
void test8()
{
    path p1(string("hello"),string("world"));
    path p2("hello/world");
    path p3("hello/world/");
    path p4("~/hello");
    p1.dump(cout);
    p2.dump(cout);
    p3.dump(cout);
    p4.dump(cout);
}

// -------------------------------------------------------------------------------------------------
void test9()
{
    path orig("replace1.txt");
    path copy("replace1.tmp");

    if(!args.is_set("-s") || !args.is_set("-r")) {
        cout << "Specify -s [search] and -r [replace]\n";
        return;
    }
    try {
        orig.cp(copy,PCF_FORCE);
        cout << copy.search_replace(args.get_value("-s"), args.get_value("-r"), true) << " values replaced.\n";
    }catch(const path_exception &pe) {
        cout << "search-replace failed: "<<pe.what()<<'\n';
    }
}
// -------------------------------------------------------------------------------------------------
void test10()
{
    if(!args.is_set("-s") || !args.is_set("-r") || !args.is_set("-e") || !args.is_set("-f")) {
        cout << "Specify -s, -r, -e and -f\n";
        args.usage();
        return;
    }
    path orig(args.get_value("-f"));
    path copy(orig);
    copy.set_ext(".tmp");
    try {
        orig.cp(copy,PCF_FORCE);
        if(copy.replace_block(args.get_value("-s"), args.get_value("-e"), args.get_value("-r"),false))
            cout <<"Block replace was succesful.\n";
        else
            cout <<"Replace not completed. Either start or end tag not found.\n";
    }catch(const path_exception &pe) {
        cout << "search-replace failed: "<<pe.what()<<'\n';
    }
}
// -------------------------------------------------------------------------------------------------
void test11()
{
    string start("<!-- REPLACEMENT START -->");
    string end("<!-- REPLACEMENT END -->");
    const char *newxml =
        "\n<option name=\"shortname\"  type=\"string\">CUSTOMER</option>\n"\
        "<option name=\"longname\"   type=\"string\">FULL COMPANY NAME</option>\n"\
        "<option name=\"email\"      type=\"string\">EMMAIL</option>\n";

    if(!args.is_set("-f")) {
        cout << "Input must be specified with -f\n";
        return;
    }
    path orig(args.get_value("-f"));
    path copy(orig);
    copy.set_ext(".tmp");
    try {
        orig.cp(copy,PCF_FORCE);
        if(copy.replace_block(start, end, newxml,false))
            cout <<"Block replace was succesful.\n";
        else
            cout <<"Replace not completed. Either start or end tag not found.\n";
    }catch(const path_exception &pe) {
        cout << "search-replace failed: "<<pe.what()<<'\n';
    }
}
// -------------------------------------------------------------------------------------------------
void test12()
{
    path p1("/var/tmp/","test",".txt");
    string d("/var/tmp/");
    string b("test");
    string e(".txt");
    path p2(d,b,e);

    p1.dump(cout);
    p2.dump(cout);
}
// -------------------------------------------------------------------------------------------------
void test13()
{
    if(!args.is_set("-s")) {
        cout<<"Missing search regex\n";
        return;
    }
    string exex;
    if(args.is_set("-e"))
        exex = args.get_value("-e");
    path_list cpp(path("./"), args.get_value("-s"), PLF_NONE, exex);
    cout << "List of results:\n";
    for(path_iterator pi=cpp.begin(); pi!=cpp.end(); pi++)
        cout << pi->get_path() << '\n';
}
// ==========================================================================================
int main(int argc, char **argv)
{
    TestItem tests[] = {
        { &test1,  "path_list, mkdir: copies *.cpp files from current dir to c4stest dir."},
        { &test2,  "rmdir recursive: removes the c4stest dir from the current dir."},
        { &test3,  "cp: copy c4s-path.cpp to c4s-path.tmp."},
        { &test4,  "mkdir recursive: Creates long path and sets user rights on the way"},
        { &test5,  "path_list: Tests path list cration options."},
        { &test6,  "path_list: Test relative paths for path list."},
        { &test7,  "cat function: puts together two files."},
        { &test8,  "path constructors."},
        { &test9,  "Search-replace."},
        { &test10, "Replace block."},
        { &test11, "Replace block within custom tags."},
        { &test12, "Path construction with const char* and const string&."},
        { &test13, "path_list: test exclude regex. (-s search regex; -e exclude regex)."},
        { 0, 0}
    };

    args += argument("-t",  true, "Sets VALUE as the test to run.");
    args += argument("-s",  true, "Sets VALUE as the text to search.");
    args += argument("-r",  true, "Sets VALUE as the text to replace.");
    args += argument("-e",  true, "Sets VALUE as end tag for replace block.");
    args += argument("-f",  true, "File to open in search and replace tests.");

    return run_tests(argc, argv, tests);
}
