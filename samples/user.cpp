/*******************************************************************************
user.cpp
This is a sample for Cpp for Scripting library. It demonstrates the use of user-class.
Please note that this sample is usable only on Unix based systems.

makec4s --dev -s user.cpp

To check the results:
grep c4s /etc/passwd
grep c4s /etc/group
ls /home

To remove test users:
userdel c4s-u1
groupdel c4s-gg

Copyright (c) Menacon Ltd
*******************************************************************************/

#include <string>
#include <iostream>
#include <stdexcept>

#include "../cpp4scripts.hpp"

using namespace std;
using namespace c4s;

#include "run_tests.cpp"

void test1()
{
    cout << "Test 1: create 2 users\n";
    user u1("c4s-u1");
    u1.create();

    user u2("c4s-u2", "staff", false, "/bin/bash", "/tmp");
    u2.create();
}

void test2()
{
    user pg("postgres","postgres",false,"/var/lib/postgresql");
    pg.dump(cout);
    int rv = pg.status();
    if (rv != 0) {
        cout << "Error: postgres user status should be zero instead of: " << rv << '\n';
        return;
    }

    process id("id", 0, &cout);
    id.set_user(&pg);
    id();
}

int main(int argc, char **argv)
{

    process::nzrv_exception = true;

    TestItem tests[] = {
        { &test1, "Create users c4s-u1 and c4s-u2"},
        { &test2, "Test postgres user"},
        { 0, 0 }
    };

    cout << "Cpp4Scripts - User sample and test program.\n";
    return run_tests(argc, argv, tests);
}
