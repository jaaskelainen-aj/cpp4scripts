/*! \file builder.cpp
 * \brief Library compiler program.
 *
 * When compiled, builds in turn the Cpp4Scripts library. Make builder with one
 * of the following commands:
 *
 * Linux / OSX:
 *   g++ -o build build.cxx -Wall -DAUTOINSTALL -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -std=c++17 -lstdc++ -O2
 *   g++ -o build build.cxx -Wall -DAUTOINSTALL -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -std=c++17 -lstdc++ -O0 -ggdb
 *
 * Windows Visual Studio:
 *   cl /Febuilder.exe /O2 /MD /EHsc /W2 builder.cpp Advapi32.lib
 *   cl /Febuilder.exe /Od /MDd /Zi /EHsc /W2 builder.cpp Advapi32.lib
 *
 * Runtime dependensies: 7z and Doxygen for publishing. Both must exist in path.
 * ---
 * This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#include <list>
#include <iostream>
#include <sstream>

#include "ntbs.hpp"
#include "ntbs.cpp"
#include "builder.cpp"
#include "builder.hpp"
#include "builder_gcc.cpp"
#include "builder_gcc.hpp"
#include "compiled_file.hpp"
#include "path.cpp"
#include "path.hpp"
#include "path_list.cpp"
#include "path_list.hpp"
#include "process.cpp"
#include "process.hpp" // includes RingBuffer
#include "program_arguments.cpp"
#include "program_arguments.hpp"
#include "RingBuffer.cpp"
#include "user.cpp"
#include "user.hpp"
#include "util.cpp"
#include "util.hpp"
#include "variables.cpp"
#include "variables.hpp"
#include "version.hpp"

using namespace std;
using namespace c4s;

#ifdef C4S_DEBUGTRACE
std::ofstream c4slog;
#endif
program_arguments args;

const char* cpp_list = "builder.cpp logger.cpp path.cpp path_list.cpp "
                       "program_arguments.cpp util.cpp variables.cpp "
                       "settings.cpp process.cpp user.cpp builder_gcc.cpp "
                       "RingBuffer.cpp ntbs.cpp";

int install(const string& install_dir);

// -------------------------------------------------------------------------------------------------
int
documentation()
{
    cout << "Creating documentation\n";
    try {
        if (process("doxygen", "c4s-doxygen.dg")()) {
            cout << "Doxygen error.\n";
            return 1;
        }
    } catch (const exception& re) {
        cerr << "Error: " << re.what() << '\n';
        return 1;
    }
    cout << "OK\n";
    return 0;
}
// -------------------------------------------------------------------------------------------------
int
build(ostream* log)
{
    if (args.is_set("-deb") && builder::update_build_no("version.hpp"))
        cout << "Warning: Unable to update build number.\n";

    path_list cppFiles(cpp_list, ' ');

    builder* make = new builder_gcc(&cppFiles, "c4s", log);
    make->set(BUILD::LIB);

    if (!args.is_set("-deb") && !args.is_set("-rel")) {
#ifdef __APPLE__
        // This piece of code is for Xcode which passes build options via environment.
        string scheme;
        if (!get_env_var("DEBUGGING_SYMBOLS", scheme)) {
            cout << "Missing target! Please specify DEBUGGING_SYMBOLS environment variable.\n";
            return 2;
        }
        if (scheme == "YES") {
            make->add(BUILD::DEB);
        } else
            make->add(BUILD::REL);
#else
        cout << "Missing target! Please specify -deb or -rel as parameter.\n";
        return 2;
#endif
    } else {
        if (args.is_set("-deb")) {
            if (args.is_set("-V"))
                cout << "Setting debug-build.\n";
            make->add(BUILD::DEB);
        } else {
            make->add(BUILD::REL);
            make->add_comp("-DNDEBUG");
        }
    }
    if (args.is_set("-V"))
        make->add(BUILD::VERBOSE);
    if (args.is_set("-export"))
        make->add(BUILD::EXPORT);

    cout << "Building library.\n";
    if (args.is_set("-t"))
        make->add_comp("-DC4S_DEBUGTRACE");
    make->add_comp("-fno-rtti");

    if (!args.is_set("makec4s") && builder::is_fail_status(make->build()) )
    {
        cout << "Build failed\n";
        delete make;
        return 2;
    }
    if (args.is_set("-export")) {
        make->export_prj(args.get_value("-export"), args.exe, args.exe);
        return 0;
    }
    delete make;

    cout << "\nBuilding makec4s\n";
    path_list plmkc4s;
    plmkc4s += path("makec4s.cpp");

    builder* make2 = new builder_gcc(&plmkc4s, "makec4s", log);
    make2->set(BUILD::BIN);
    make2->add(args.is_set("-deb") ? BUILD::DEB : BUILD::REL);
    if (args.is_set("-V"))
        make2->add(BUILD::VERBOSE);
    make2->add_comp("-fno-rtti");
    make2->add_link("-lc4s");
    make2->add_link(args.is_set("-deb") ? " -L./debug" : " -L./release");
    if (builder::is_fail_status(make2->build()) ) {
        cout << "\nBuild failed.\n";
        delete make2;
        return 2;
    } else {
        cout << "Compilation ready.\n";
#ifdef AUTOINSTALL
        install("/usr/local/");
#endif
    }
    delete make2;
    return 0;
}
// -------------------------------------------------------------------------------------------------
int
clean()
{
    path deb("./debug/");
    if (deb.dirname_exists())
        deb.rmdir(true);
    path rel("./release/");
    if (rel.dirname_exists())
        rel.rmdir(true);
    path_list tmp(args.exe, "\\.log$");
    tmp.add(args.exe, "~$");
    tmp.add(args.exe, "\\.obj$");
    tmp.add(args.exe, "\\.ilk$");
    tmp.add(args.exe, "\\.pdb$");
    tmp.add(args.exe, "^makec4s-");
    tmp.rm_all();
    cout << "Build directories and " << tmp.size() << " temp-files removed\n";
    return 0;
}
// -------------------------------------------------------------------------------------------------
int
install(const string& install_dir)
{
    path lbin;
    path home(args.exe);
    path_iterator pi;

    cout << "Installing Cpp4Scripts\n";
    path inst_root(install_dir);
    if (!inst_root.dirname_exists()) {
        cout << "Installation root directory " << inst_root.get_path() << " must exist.\n";
        return 1;
    }
    // Set up the target directories
    if (args.is_set("-V"))
        cout << "Creating target directories\n";
    path inc(inst_root);
    inc += "include/cpp4scripts/";
    if (!inc.dirname_exists())
        inc.mkdir();

    // Copy sources
    if (args.is_set("-V")) {
        cout << "Copying headers and sources.\n";
    }
    path_list sources(cpp_list, ' ');
    string target = "c4s";
    if (args.is_set("-l")) {
        target += '-';
        target += args.get_value("-l");
    }

    path dlib("debug/libc4s.a");
    path rlib("release/libc4s.a");
    path make_name("makec4s");

    int lib_count = 0;
    if (dlib.exists() && args.is_set("-deb")) {
        path lib(inst_root);
        lib += "lib-d/";
        if (!lib.dirname_exists())
            lib.mkdir();
        dlib.cp(lib, PCF_FORCE);
        if (args.is_set("-V"))
            cout << "Copied " << dlib.get_path() << " to " << lib.get_path() << '\n';
        lib_count++;
    }
    if (rlib.exists() && args.is_set("-rel")) {
        path lib(inst_root);
        lib += "lib/";
        if (!lib.dirname_exists())
            lib.mkdir();
        rlib.cp(lib, PCF_FORCE);
        if (args.is_set("-V"))
            cout << "Copied " << rlib.get_path() << " to " << lib.get_path() << '\n';
        lib_count++;
    }
    if (lib_count == 0) {
        cout << "WARNING: Neither of the -deb or -rel libraries will be copied. Sure they are "
                "built?\n";
        cout << "Searched:\n";
        cout << "  " << dlib.get_path() << '\n';
        cout << "  " << rlib.get_path() << '\n';
    }
    sources.set_dir(home);
    path_list headers(args.exe, ".*hpp$");
    if (headers.size() == 0) {
        cout << "No C4S headers found. Installation aborted.\n";
        return 2;
    }
    sources.copy_to(inc, PCF_FORCE);
    headers.copy_to(inc, PCF_FORCE);

    // Copy makec4s utility
    if (args.is_set("-V"))
        cout << "Copying makec4s\n";
    if (inst_root.get_dir().find("local") != string::npos)
        lbin.set("/usr/local/bin/");
    else
        lbin.set("/usr/bin/");
    if (make_name.exists()) {
        make_name.cp(lbin, PCF_FORCE);
        if (args.is_set("-V"))
            cout << "Copied " << make_name.get_path() << " to " << lbin.get_path() << '\n';
    } else {
        cout << "WARNING: makec4s-program was not found. It was not copied!\n";
        if (args.is_set("-V")) {
            cout << "makec4s path:" << make_name.get_path() << '\n';
        }
    }
    cout << "Completed\n";
    return 0;
}
// -------------------------------------------------------------------------------------------------
int
main(int argc, char** argv)
{
#ifdef C4S_DEBUGTRACE
    c4slog.open("build_debug.log");
#endif
    args += argument("-deb", false, "Create debug version of library.");
    args += argument("-rel", false, "Create release version of library.");
    args += argument("-export", true, "Export project files [ccdb|cmake]");
    args += argument("-t", false, "Add C4S_DEBUGTRACE define into target build.");
    args += argument("-u", false, "Updates the build number (last part of version number).");
    args += argument("-CXX", false, "Reads the compiler name from CXX environment variable.");
    args += argument("-doc", false, "Create docbook documentation only.");
    args += argument("-clean", false, "Clean up temporary files.");
    args += argument("-install", true,
                     "Installs the library to given root directory. include- and lib-directories "
                     "are created if necessary.");
    args += argument("makec4s", false, "Build the MakeC4S program only.");
    args += argument("-v", false, "Shows the version nubmer and exists.");
    args += argument("-V", false, "Verbose mode.");
    args += argument("-?", false, "Shows this help");

    cout << "CPP4Scripts Builder Program. " << CPP4SCRIPTS_VERSION << ' ' << get_build_type()
         << '\n';

    try {
        args.initialize(argc, argv);
        args.exe.cd();
    } catch (const exception& ce) {
        cerr << "Error: " << ce.what() << '\n';
        args.usage();
        return 1;
    }
    if (args.is_set("-v"))
        return 0;
    if (args.is_set("-?")) {
        args.usage();
        return 0;
    }
    try {
        if (args.is_set("-clean"))
            return clean();
        if (args.is_set("-install"))
            return install(append_slash(args.get_value("-install")));
    } catch (const exception& ce) {
        cout << "Function failed: " << ce.what() << '\n';
        return 1;
    }
    if (args.is_set("-doc"))
        return documentation();

    try {
        return build(&cout);
    } catch (const exception& ce) {
        cout << "Build failed: " << ce.what() << '\n';
        return 1;
    }
    return 0;
}