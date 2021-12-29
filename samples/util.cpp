/*******************************************************************************
util.cpp
This is a sample and test source for Cpp for Scripting library.

................................................................................
License: LGPLv3
Copyright (c) Menacon Ltd, Finland
*******************************************************************************/

#include <sstream>
#include <fstream>
#include <unordered_map>

#include "../cpp4scripts.hpp"

using namespace std;
using namespace c4s;

program_arguments args;
typedef void (*tfptr)();

// -------------------------------------------------------------------------------------------------
void
test1()
{
    char txt[6];
    txt[5] = 0;
    fstream file("util1.txt", ios_base::in);
    if (!file) {
        cout << "Input test file missing.\n";
        return;
    }
    if (!args.is_set("-s")) {
        cout << "Missing search text.\n";
        args.usage();
        return;
    }
    if (search_file(file, args.get_value("-s"))) {
        cout << "Found the text at:" << file.tellg() << '\n';
        file.read(txt, 5);
        cout << "Few chars: " << txt << '\n';
    } else
        cout << "Text not found\n";
}
// -------------------------------------------------------------------------------------------------
void
test2()
{
    const char* str[3][2] = { { "basic", "bas*" },
                              { "antti.mcdev.fi.pem", "psql.mcdev.fi*" },
                              { "psql.mcdev.fi.pem", "psql.mcdev.fi*" } };

    for (int i = 0; i < 3; i++) {
        cout << str[i][0] << " == " << str[i][1] << " ? ";
        if (match_wildcard(str[i][0], str[i][1]))
            cout << "TRUE\n";
        else
            cout << "FALSE\n";
    }
}
// -------------------------------------------------------------------------------------------------
void
test3()
{
    path here("./");
    if (!generate_next_base(here, "replace*.txt"))
        cout << "Failed to create next file name\n";
    else
        cout << "Next is " << here.get_base() << '\n';
    if (!generate_next_base(here, "util*.cpp"))
        cout << "Failed to create next file name\n";
    else
        cout << "Next is " << here.get_base() << '\n';
}
// -------------------------------------------------------------------------------------------------
void
test4()
{
    BUILD fg(BUILD::DEB);
    cout << "flags = " << hex << fg.get() << '\n';
    fg |= BUILD::VERBOSE | BUILD::NOLINK;
    cout << "flags = " << hex << fg.get() << '\n';
}
// -------------------------------------------------------------------------------------------------
void
test5()
{
    unordered_map<string, string> map1;
    unordered_map<string, string> map2;
    unordered_map<string, string> map3;
    bool rv;

    const char* kv_str1 = "key1=val1&  key2=  val2&key3   =val3";
    const char* kv_str2 = "CN=MyApp ACES CA 2, OU=MyApp Public Sector, O=MyApp, C=US";
    const char* kv_str3 = "This,should,fail";

    cout << kv_str1  << '\n';
    rv = parse_key_values(kv_str1, map1, '&');
    for (const auto& n : map1) {
        cout << n.first << "|" << n.second  << "|\n";
    }
    cout << "Parsing " << (rv ? "OK" : "Failed")  << '\n';
    cout  << '\n';
    
    cout << kv_str2  << '\n';
    rv = parse_key_values(kv_str2, map2);
    for (const auto& n : map2) {
        cout << n.first << "|" << n.second  << "|\n";
    }
    cout << "Parsing " << (rv ? "OK" : "Failed")  << '\n';
    cout  << '\n';

    cout << kv_str3  << '\n';
    rv = parse_key_values(kv_str3, map3);
    for (const auto& n : map3) {
        cout << n.first << "|" << n.second  << "|\n";
    }
    cout << "Parsing " << (rv ? "OK" : "Failed")  << '\n';
}
// =================================================================================================
int
main(int argc, char** argv)
{
    tfptr tfunc[] = { &test1, &test2, &test3, &test4, &test5, 0 };

    args += argument("-t", true, "Sets VALUE as the test to run.");
    args += argument("-s", true, "Sets the text to search for.");
    try {
        args.initialize(argc, argv, 1);
    } catch (const c4s_exception&) {
        args.usage();
        return 1;
    }
    if (!args.is_set("-t")) {
        cout << "Missing -t argument\n";
        cout << " 1 = searches the util1.txt for given text (-s)\n";
        cout << " 2 = Tests various wild card matchings\n";
        cout << " 3 = Generate next file index.\n";
        cout << " 4 = BUILD flags.\n";
        return 1;
    }
    int tmax = 0;
    while(tfunc[tmax])
        tmax++;
    istringstream iss(args.get_value("-t"));
    int test;
    iss >> test;
    if (test > 0 && test < tmax+1)
        tfunc[test - 1]();
    else
        cout << "Unknown test number: " << test << '\n';

    return 0;
}
