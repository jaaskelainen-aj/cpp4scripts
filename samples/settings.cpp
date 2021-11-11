#include <iostream>
#include "../c4s_all.hpp"

using namespace std;
using namespace c4s;

program_arguments args;

typedef bool (*tfptr)(const string&);

void print_settings(configuration &conf)
{
    cout<<"# Settings:\n";
    for(list<settings::section*>::iterator si = conf.begin();
        si != conf.end(); si++)
    {
        cout<<"Section: "<<(*si)->get_name()<<'\n';
        for(auto item : (*si)->items) {
            cout<<item.first<<"\t\t"<<item.second<<'\n';
        }
    }
    cout<<'\n';
}
// ==========================================================================================
bool test1(const string&)
{
    cout<<"Flat settings:";

    ifstream cf("settings_flat.conf");
    if(!cf) {
        cout<<"Failed - Unable to open settings_flat.conf\n";
        return false;
    }
    string value;
    int status = 0;
    try {
        configuration conf(configuration::FORMAT::FLAT, cf);
        print_settings(conf);
        cout<<"Verify values:";
        if(conf.get_value("general", "setting1", value)) {
            if(value.compare("value of setting 1"))
                status |= 0x1;
        }
        if(conf.get_value("general", "setting two", value)) {
            if(!value.compare("2"))
                status |= 0x2;
        }
        if(conf.get_value("general", "setting3", value)) {
            if(!value.compare("third"))
                status |= 0x4;
        }
    }
    catch(const c4s_exception &ce) {
        cout<<"Failed: "<<ce.what()<<'\n';
        return false;
    }
    if(!status) {
        cout<<"FAIL "<<hex<<status<<"\n";
        return false;
    }
    cout<<"OK\n";
    return true;
}
// ==========================================================================================
bool test2(const std::string& arg)
{
    cout<<"Testing parsing JSON settings\n";
    if(arg.empty()) {
        cout<<"Missing json file name\n";
        return false;
    }
    ifstream json(arg.c_str());
    if(!json) {
        cout<<"Unable to find "<<arg<<'\n';
        return false;
    }
    try {
        configuration conf(configuration::FORMAT::JSON, json);
        print_settings(conf);
    }
    catch(const c4s_exception &ce) {
        cout<<"failed: "<<ce.what()<<'\n';
        return false;
    }
    return true;
}
// ==========================================================================================
bool test3(const std::string& arg)
{
    cout<<"Testing querying JSON setting\n";
    if(arg.empty()) {
        cout<<"Missing json file name\n";
        return false;
    }
    ifstream json(arg.c_str());
    if(!json) {
        cout<<"Unable to find "<<arg<<'\n';
        return false;
    }
    try {
        configuration conf(configuration::FORMAT::JSON, json);
        settings::section *ssn = conf.get_section("Section2");

        vector<string> skeys;
        ssn->get_subkeys("top.*", skeys);
        if(!skeys.size()){
            cout<<"Subkeys not found\n";
            return false;
        }
        cout<<"Subkeys for 'top.*'\n";
        for(auto key : skeys) {
            cout<<"  "<<key<<'\n';
        }

        vector<string> values;
        ssn->get_values("array.[*]", values);
        if(!values.size()) {
            cout<<"array.[*] values not found\n";
            return false;
        }
        cout<<"Values for Section2.array:\n";
        for(auto val : values) {
            cout<<val<<'\n';
        }
    }
    catch(const c4s_exception &ce) {
        cout<<"failed: "<<ce.what()<<'\n';
        return false;
    }
    return true;
}
// ==========================================================================================
int main(int argc, char **argv)
{
    string param;
    const int tmax=3;
    tfptr tfunc[] = { &test1, &test2, &test3, 0 };

    args += argument("-t",  true, "Sets VALUE as the test to run.");
    args += argument("-p",  true, "Send VALUE as parameter to test(s)");
    try{
        args.initialize(argc,argv,0);
    }catch(const c4s_exception &ce){
        cout<<ce.what()<<'\n';
        args.usage();
        return 1;
    }
    if(args.is_set("-p"))
        param = args.get_value("-p");
    if(args.is_set("-t")) {
        istringstream iss(args.get_value("-t"));
        int test;
        iss >> test;
        if(test>0 && test<=tmax) {
            tfunc[test-1](param);
        }
        else
            cout << "Unknown test number: "<<test<<'\n';
    }
    else {
        cout<<"# Running unit tests:\n";
        int count=0, success=0;
        for(int ndx=0; tfunc[ndx]; ndx++) {
            if(tfunc[ndx](param))
                success++;
            count++;
        }
        cout<<">> "<<count<<" tests completed. "<<success<<" tests successful.\n";
    }
    return 0;
}
