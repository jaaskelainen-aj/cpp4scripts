/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#include "cpp4scripts.hpp"
#include "version.hpp"

using namespace std;
using namespace c4s;

program_arguments args;

int
make_template()
{
    path tname(args.get_value("-new"));
    if (tname.exists()) {
        cout << "Template " << args.get_value("-new")
             << " already exists. Please use another name.\n";
        return 2;
    }
    ofstream ts(tname.get_path().c_str());
    if (!ts) {
        cout << "Unable to open template " << args.get_value("-new") << " for writing.\n";
        return 2;
    }
    ts << "#include <iostream>\n";
    ts << "#include <cpp4scripts.hpp>\n"
          "using namespace std;\n"
          "using namespace c4s;\n\n";
    ts << "int main(int argc, char **argv)\n{\n"
          "    program_arguments args;\n\n";

    ts << "    args += argument(\"--help\",  false, \"Outputs this help / parameter list.\");\n"
          "    try {\n"
          "        args.initialize(argc,argv);\n"
          "    }catch(const c4s_exception &ce){\n"
          "        cout << \"Incorrect parameters.\\n\"<<ce.what()<<'\\n';\n"
          "        return 1;\n"
          "    }\n";
    ts << "    if( args.is_set(\"--help\") ) {\n"
          "        args.usage();\n"
          "        return 0;\n"
          "    }\n";
    ts << "    return 0;\n";
    ts << "}\n";

    ts.close();
    cout << "Template created.\n";
    return 0;
}

int
main(int argc, char** argv)
{
    bool debug = false;
    bool verbose = false;
    int timeout = 15;

    // Set arguments and initialize them.
    args += argument("-s", true, "Sets VALUE as a source file to be compiled.");
    args += argument("-deb", false, "Create debug version.");
    args += argument("-rel", false, "Create release version (default).");
    args += argument("-def", false, "Print the default compiler argumens to stdout.");
    args += argument("-new", true, "Make a new c4s-template file with VALUE as the name.");
    args += argument("-c4s", true,
                     "Path where Cpp4Scripts is installed. If not defined, then $C4S is tried and "
                     "then '/usr/local/'");
    args += argument("-inc", true, "External include file to add to the build.");
    args += argument("-lib", true, "External library to add to the link command");
    args += argument("-m", true, "Set the VALUE as timeout (seconds) for the compile.");
    args += argument("-hash", true, "Calculate FNV hash for named file.");
    args += argument("-t", false, "Enable C4S_DEBUGTRACE define for tracing the cpp4scripts code.");
    args += argument("-v", false, "Prints the version number.");
    args += argument("-V", false, "Verbose mode. Prints more messages, including build command.");
    args += argument("--dev", false,
                     "Run  builder for unit test files in CPP4Scripts samples directory");
    args += argument("--help", false, "Outputs this help / parameter list.");
    try {
        args.initialize(argc, argv);
    } catch (const c4s_exception& ce) {
        cout << "Incorrect parameters.\n" << ce.what() << '\n';
        return 1;
    }
    // ............................................................
    // Handle simple arguments.
    if (args.is_set("--help")) {
        cout << "Cpp4Scripts make program. " << CPP4SCRIPTS_VERSION;
        cout << "\n\nWith no parameters makec4s searches for environment variable "
                "MAKEC4S_DEF_SOURCE.\n"
                "If value is defined and file found in current directory makec4s will build it.\n"
                "Use the '-s' parameter to specify a Cpp4Scripts source file to compile.\n\n"
                "Rest of the parameters:\n";
        args.usage();
        return 0;
    }
    cout << "Cpp4Scripts make program. " << CPP4SCRIPTS_VERSION << ' ' << get_build_type() << '\n';
    if (args.is_set("-v")) {
        return 0;
    }
    if (args.is_set("-new")) {
        return make_template();
    }
    if (args.is_set("-hash")) {
        path target(args.get_value("-hash"));
        cout << "FNV hash: " << hex << target.fnv_hash64() << "\n";
        return 0;
    }
    if (args.is_set("-V"))
        verbose = true;
    else {
        string verbose_env;
        if (get_env_var("MAKEC4S_VERBOSITY", verbose_env) && verbose_env[0] == '1')
            verbose = true;
    }
    if (!args.is_set("-deb") && !args.is_set("-rel")) {
        string scheme;
        if (get_env_var("DEBUGGING_SYMBOLS", scheme)) {
            debug = scheme.compare("YES") == 0 ? true : false;
            if (verbose)
                cout << "Debugging mode selected via environment variable.\n";
        }
    } else
        debug = args.is_set("-deb") ? true : false;

    if (args.is_set("-def")) {
        try {
            builder_gcc gcc("dummy", 0);
            gcc.set(BUILD::BIN);
            gcc.add(debug ? BUILD::DEB : BUILD::REL);
            cout << "Binary build default options:\n";
            gcc.print(cout);
            cout << "Library build default options:\n";
            builder_gcc link("dummy", 0);
            link.set(BUILD::LIB);
            link.add(debug ? BUILD::DEB : BUILD::REL);
            link.print(cout);
        } catch (const c4s_exception& ce) {
            cout << "Parameter output failed:" << ce.what() << '\n';
            return 1;
        }
        return 0;
    }
    // ............................................................
    // Set the sources and the target
    path_list sources;
    path src;
    string target;
    if (args.is_set("-s"))
        src = args.get_value("-s");
    else {
        // try the environment
        string source_file;
        if (get_env_var("MAKEC4S_DEF_SOURCE", source_file))
            src.set(source_file);
        else {
            cout << "Nothing to do. Use either -s [source] or env.var. 'MAKEC4S_DEF_SOURCE'\n";
            return 1;
        }
    }
    if (!src.exists()) {
        cout << "Source file '" << src.get_path() << "' does not exist.\n";
        return 1;
    }
    if (verbose)
        cout << "Using " << src.get_path() << " as a source file.\n";
    sources += src;
    if (args.is_set("-m")) {
        timeout = strtol(args.get_value("-m").c_str(), 0, 10);
        if (timeout == 0) {
            cout
                << "Warning: unable to recognize the compile process timeout. Using the default.\n";
            timeout = 15;
        }
    }
    target = src.get_base_plain();
    builder* make = 0;
    try {
        // Gcc options for Linux
        // Build options
        string libname("-lc4s");
        make = new builder_gcc(&sources, target.c_str(), &cout);
        // Get C4S location
        string c4svar;
        make->add_comp("-x c++ -fno-rtti");
        if (args.is_set("--dev"))
            make->add_comp("-I..");
        else {
            if (args.is_set("-c4s"))
                make->set_variable("C4S", args.get_value("-c4s"));
            else if (!get_env_var("C4S", c4svar))
                make->set_variable("C4S", "/usr/local");
            make->add_comp("-I$(C4S)/include/cpp4scripts");
        }
        if (args.is_set("-t"))
            make->add_comp("-DC4S_DEBUGTRACE");
        make->add_link(libname.c_str());
        if (args.is_set("--dev"))
            make->add_link("-L../debug");
        else {
            if (debug)
                make->add_link("-L$(C4S)/lib-d");
            else
                make->add_link("-L$(C4S)/lib");
        }
        make->set(BUILD::BIN);
        make->set(debug ? BUILD::DEB : BUILD::REL);
        if (verbose)
            make->set(BUILD::VERBOSE);
        if (args.is_set("-inc"))
            make->add_comp(args.get_value("-inc").c_str());
        if (args.is_set("-lib"))
            make->add_link(args.get_value("-lib").c_str());
        make->set_timeout(timeout);
        if (builder::is_fail_status(make->build()) ) {
            cout << "Build failed.\n";
            delete make;
            return 2;
        } else
            cout << make->get_target_name() << " ready.\n";
        delete make;
    } catch (const c4s_exception& ce) {
        cout << "Error: " << ce.what() << '\n';
        if (make)
            delete make;
        return 1;
    }
    return 0;
}