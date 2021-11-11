/*! \file builder.cpp
 * \brief Library compiler program.
 *
 * When compiled, builds in turn the Cpp4Scripts library. Make builder with one
 * of the following commands:
 *
 * Linux / OSX:
 *   g++ -o build build.cxx -Wall -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -std=c++17 -lstdc++ -O2 
 *   g++ -o build build.cxx -Wall -pthread -fexceptions -fno-rtti -fuse-cxa-atexit -std=c++17 -lstdc++ -O0 -ggdb
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
#include <sstream>

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
#include "process.hpp"
#include "program_arguments.cpp"
#include "program_arguments.hpp"
#include "user.cpp"
#include "user.hpp"
#include "util.cpp"
#include "util.hpp"
#include "variables.cpp"
#include "variables.hpp"
#include "version.hpp"

using namespace std;
using namespace c4s;

program_arguments args;

const char* cpp_list = "builder.cpp logger.cpp path.cpp path_list.cpp "
                       "process.cpp program_arguments.cpp util.cpp variables.cpp "
                       "settings.cpp";
const char* cpp_win = "builder_vc.cpp builder_ml.cpp";
const char* cpp_linux = "user.cpp builder_gcc.cpp";
// -------------------------------------------------------------------------------------------------
int
documentation(ostream* log)
{
    cout << "Creating documentation\n";
    try {
        if (process("doxygen", "c4s-doxygen.dg", log)(10)) {
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
#if defined(__linux) || defined(__APPLE__)
int
build(ostream* log)
{
    builder *make = 0, *make2 = 0;
    bool debug = false;

    if (args.is_set("-u") && builder::update_build_no("version.hpp"))
        cout << "Warning: Unable to update build number.\n";

    path_list cppFiles(cpp_list, ' ');
    cppFiles.add(cpp_linux, ' ');

    make = new builder_gcc(&cppFiles, "c4s", log);
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
            debug = true;
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
            debug = true;
            make->add(BUILD::DEB);
        } else
            make->add(BUILD::REL);
    }
    if (args.is_set("-V"))
        make->add(BUILD::VERBOSE);
    if (args.is_set("-export"))
        make->add(BUILD::EXPORT);

    cout << "Building library.\n";
    if (args.is_set("-t"))
        make->add_comp("-DDEBUGTRACE");
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

    cout << "\nBuilding makec4s\n";
    path_list plmkc4s;
    plmkc4s += path("makec4s.cpp");

    int rv = 0;
    make2 = new builder_gcc(&plmkc4s, "makec4s", log);
    make2->set(BUILD::BIN);
    make2->add(debug ? BUILD::DEB : BUILD::REL);
    if (args.is_set("-V"))
        make2->add(BUILD::VERBOSE);
    make2->add_comp("-fno-rtti");
    make2->add_link("-lc4s");
    make2->add_link(debug ? " -Ldebug" : " -Lrelease");
    if (builder::is_fail_status(make2->build()) ) {
        cout << "\nBuild failed.\n";
        rv = 2;
    } else
        cout << "Compilation ready.\n";
    delete make;
    delete make2;
    return rv;
}
#endif
// -------------------------------------------------------------------------------------------------
#ifdef _WIN32
int
build(ostream* log)
{
    std::ostringstream liblog;
    int rv = 0;

    cout << "Building library\n";
    if (args.is_set("-u") && builder::update_build_no("version.hpp"))
        cout << "Warning: Unable to update build number\n";

    path_list cppFiles(cpp_list, ' ');
    cppFiles.add(cpp_win, ' ');

    string target = "c4s";
    if (args.is_set("-l")) {
        target += '-';
        target += args.get_value("-l");
    }
    builder* make = new builder_vc(&cppFiles, target.c_str(), log, BUILD(BUILD::LIB));
    make->add_comp("/DLIB_BUILD /D_CRT_SECURE_NO_WARNINGS");
    if (args.is_set("-xp"))
        make->add_comp("/D_WIN32_WINNT=0x0501");
    *make |= args.is_set("-deb") ? BUILD::DEB : BUILD::REL;
    if (args.is_set("-V"))
        *make |= BUILD::VERBOSE;
    if (make->build()) {
        cout << "Build failed\n";
        return 2;
    }

    cout << "Building makec4s\n";
    if (log)
        *log << endl;
    path_list plmkc4s;
    plmkc4s += path("makec4s.cpp");
    builder* make2 = new builder_vc(&plmkc4s, "makec4s", log, BUILD(BUILD::BIN));
    make2->add_link("Advapi32.lib ");
    string c4slib(make->get_target_name());
    c4slib += " /LIBPATH:";
    c4slib += make->get_build_dir();
    make2->add_link(c4slib.c_str());
    *make2 |= args.is_set("-deb") ? BUILD::DEB : BUILD::REL;
    if (args.is_set("-V"))
        *make2 |= BUILD::VERBOSE;
    if (make2->build()) {
        cout << "Build failed\n";
        return 2;
    }

    cout << "Build finished.\n";
    return 0;
}
#endif

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
install()
{
    path lbin;
    path home(args.exe);
    path_iterator pi;

    cout << "Installing Cpp4Scripts\n";
    path inst_root(append_slash(args.get_value("-install")));
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

#if defined(__linux) || defined(__APPLE__)
    sources.add(cpp_linux, ' ');
    path dlib("debug/libc4s.a");
    path rlib("release/libc4s.a");
    path make_name("makec4s");
#else
    sources.add(cpp_win, ' ');
    path dlib(builder_vc(0, target.c_str(), 0, BUILD::LIB | BUILD::DEB).get_target_path());
    path rlib(builder_vc(0, target.c_str(), 0, BUILD::LIB | BUILD::REL).get_target_path());
    path make_name(builder_vc(0, "makec4s", 0, BUILD::BIN | BUILD::REL).get_target_name());
#endif
    int lib_count = 0;
    if (dlib.exists()) {
        path lib(inst_root);
        lib += "lib-d/";
        if (!lib.dirname_exists())
            lib.mkdir();
        dlib.cp(lib, PCF_FORCE);
        if (args.is_set("-V"))
            cout << "Copied " << dlib.get_path() << " to " << lib.get_path() << '\n';
        lib_count++;
    }
    if (rlib.exists()) {
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
#if defined(__linux) || defined(__APPLE__)
    if (inst_root.get_dir().find("local") != string::npos)
        lbin.set("/usr/local/bin/");
    else
        lbin.set("/usr/bin/");
#else
    lbin = inst_root;
    lbin += "bin/";
    if (!lbin.dirname_exists())
        lbin.mkdir();
#endif
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
    args += argument("-deb", false, "Create debug version of library.");
    args += argument("-rel", false, "Create release version of library.");
    args += argument("-export", true, "Export project files [ccdb|cmake]");
    args += argument("-t", false,
                     "Add TRACE define into target build that enables lots of debug output.");
    args += argument("-u", false, "Updates the build number (last part of version number).");
#ifdef _WIN32
    args += argument("-wda", false, "Wait for debugger to attach, i.e. wait for a keypress.");
#endif
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
#ifdef _WIN32
    if (args.is_set("-wda")) {
        char ch;
        cin >> ch;
    }
#endif
    if (args.is_set("-?")) {
        args.usage();
        return 0;
    }
    try {
        if (args.is_set("-clean"))
            return clean();
        if (args.is_set("-install"))
            return install();
    } catch (const exception& ce) {
        cout << "Function failed: " << ce.what() << '\n';
        return 1;
    }
    ostream* log = &cout;
    if (args.is_set("-doc"))
        return documentation(log);

    try {
        return build(log);
    } catch (const exception& ce) {
        cout << "Build failed: " << ce.what() << '\n';
        return 1;
    }
    return 0;
}