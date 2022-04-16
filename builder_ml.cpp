/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#include "config.hpp"
#include "exception.hpp"
#include "variables.hpp"
#include "path.hpp"
#include "path_list.hpp"
#include "process.hpp"
#include "compiled_file.hpp"
#include "util.hpp"
#include "builder.hpp"
#include "builder_ml.hpp"

using namespace std;
using namespace c4s;

#ifdef _WIN32

// -------------------------------------------------------------------------------------------------
c4s::builder_ml::builder_ml(path_list& _sources,
                            const char* _name,
                            ostream* _log,
                            const int _flags,
                            const char* subsys)
  : builder(_sources, _name, _log, _flags, subsys)
{
    string pdb;

    compiler.set_command("ml64");

    const char* arch = has_any(BUILD::X32) ? "32" : "64";
    if (log && has_any(BUILD::VERBOSE)) {
        *log << "builder_ml - " << arch << "bit environment detected.\n";
    }

    // Set the default flags
    if (!has_any(BUILD::NODEFARGS)) {
        c_opts << "/nologo ";
        if (has_any(BUILD::DEB)) {
            c_opts << "/Zi ";
        }
        if (sources && sources.size() > 1)
            c_opts << "/c ";
        if (has_any(BUILD::X32))
            c_opts << "/coff ";
    }
    // Set the target name
    target = name;
    if (has_any(BUILD::LIB)) {
        linker.set_command("lib");
        target += ".lib";
    } else if (!has_any(BUILD::NOLINK)) {
        linker.set_command("link");
        target += ".exe";
    }
    if (has_any(BUILD::PAD_NAME)) {
        pad_name(name, subsys, flags);
        pad_name(target, subsys, flags);
    }
    if (!has_any(BUILD::NODEFARGS) && !has_any(BUILD::NOLINK)) {
        l_opts << "/nologo /SUBSYSTEM:CONSOLE ";
        if (has_any(BUILD::DEB)) {
            l_opts << "/DEBUG ";
        } else {
            l_opts << "/RELEASE ";
        }
        if (has_any(BUILD::X64))
            l_opts << "/MACHINE:X64 ";
    }
}

// -------------------------------------------------------------------------------------------------
int
c4s::builder_ml::build()
{
    if (!sources)
        throw c4s_exception("builder_ml::build - build source files not defined.");
    // Only one file?
    if (sources.size() == 1) {
        // Combine compile+link.
        ostringstream single;
        path src = sources.front();
        single << " /Fo" << target;
        single << ' ' << c_opts.str() << ' ' << src.get_base() << " /link " << l_opts.str();
        if (log && has_any(BUILD::VERBOSE))
            *log << "Compiling: " << vars.expand(single.str()) << '\n';
        return compiler.exec(20, vars.expand(single.str()).c_str());
    }
    // Make sure build directory exists.
    path buildp(build_dir + C4S_DSEP);
    if (!buildp.dirname_exists()) {
        buildp.mkdir();
        if (log && has_any(BUILD::VERBOSE))
            *log << "builder_ml - created build directory:" << buildp.get_path() << '\n';
    }
    // Call parent to do the job.
    if (log && has_any(BUILD::VERBOSE))
        builder::print(*log);
    int rv = builder::compile(".obj", "/Fo ", false);
    if (!rv && !has_any(BUILD::NOLINK))
        return builder::link(".obj", "/OUT:");
    return rv;
}
#endif _WIN32
