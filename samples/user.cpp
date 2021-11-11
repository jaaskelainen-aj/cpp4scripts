/*******************************************************************************
c4s-user.cpp
This is a sample for Cpp for Scripting library. It demonstrates the use of user-class.
Please note that this sample is usable only on Unix based systems.

makec4s -a -s c4s-user.cpp

To check the results:
grep c4s /etc/passwd
grep c4s /etc/group
ls /home

To remove test users:
userdel c4s-u1
userdel c4s-u2
userdel c4s-u3
userdel c4s-u4
userdel c4s-u5
groupdel c4s-gg
groupdel c4s-xx

Copyright (c) Menacon Ltd
*******************************************************************************/
#define C4S_DEBUGTRACE
#include "../c4s_all.hpp"
using namespace c4s;

int main(int argc, char **argv)
{
    cout << "Cpp4Scripts - User sample and test program.\n";
    cout << "Number of user accounts will be created. Root privileges will be required.\n";
//    cout << "Press any enter to continue, or Ctrl-C to stop\n";
//    cin.get();
//    cin.ignore();

    process::nzrv_exception = true;
    cout <<     "user   | group  | sys | shell      | home\n";
    cout <<     "---------------------------------------------------------\n";
    try {
#if 0
        cout << "c4s-u1 | -      | F   | [default]  | [default]\n";
        user u1("c4s-u1");
        u1.create();

        cout << "c4s-u2 | -      | F   | /bin/bash  | /tmp\n";
        user u2("c4s-u2",0,false,"/bin/bash","/tmp");
        u2.create();

        cout << "c4s-u3 | c4s-u3 | T   | /bin/false | /var/run\n";
        user u3("c4s-u3","c4s-u3",true,"/bin/false", "/var/run");
        u3.dump(cout);
        u3.create();

        cout << "c4s-u4 | c4s-gg | F   | [default]  | [default]\n";
        user u4("c4s-u4","c4s-gg");
        u4.GROUPS = "staff,games";
        u4.dump(cout);
        u4.create();

        cout << "c4s-u5 | users  | F   | [default]  | [default]\n";
        user u5("c4s-u5","users");
        u5.dump(cout);
        u5.create();

        cout << " -     | c4s-xx | F   | [N/A]      | [N/A]\n";
        user u6(,"c4s-xx");
        u6.dump(cout);
        u6.create();
#endif
        cout << "c4s-u3 | c4s-u3 | T   | /bin/false | /var/run\n";
        user u3("c4s-u3","c4s-u3",true,"/bin/false", "/var/run","sys");
        u3.dump(cout);
        u3.create();
        cout << "User u3 status: "<<u3.status()<<'\n';

//        cout << "Users created, check results. About to make chage to c4s-u4.\nPress enter when ready.";
//        cin.get();
//        cin.ignore();
        // Changes user's primary login group and additional groups.

        //cout << "c4s-u4 | c4s-xx | F    | -\n";
        //user u7("c4s-u4","c4s-xx");
    }catch(c4s_exception ce) {
        cout << "User creation failed: "<<ce.what()<<endl;
        return 1;
    }
    cout << "Done.\n";
    return 0;
}
