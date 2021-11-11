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
#include "builder_gcc.hpp"

using namespace std;
using namespace c4s;

namespace c4s {

// -------------------------------------------------------------------------------------------------
void
builder_gcc::parse_flags()
{
    // Determine the compiler name
    string gpp, link;
    gpp = has_any(BUILD::PLAIN_C) ? "gcc" : "g++";
    link = has_any(BUILD::PLAIN_C) ? "gcc" : "g++";
    compiler.set_command(gpp.c_str());

    if (!sources)
        throw c4s_exception("builder_gcc::parse_flags - sources not defined!");

    // Determine the real target name.
    if (has_any(BUILD::LIB)) {
        linker.set_command("ar");
        target = "lib";
        target += name;
        target += ".a";
        l_opts << "-rcs ";
    } else {
        target = name;
        linker.set_command(link.c_str());
        if (has_any(BUILD::SO)) {
            c_opts << "-fpic ";
            l_opts << "-shared -fpic ";
            target += ".so";
        }
    }

    if (build_dir.size() == 0) {
        build_dir = has_any(BUILD::DEB) ? "debug" : "release";
        if (log && has_any(BUILD::VERBOSE))
            *log << "builder::builder - output dir set to: " << build_dir << '\n';
    }

    if (!has_any(BUILD::NODEFARGS)) {
#if __GNUC__ >= 5
        c_opts << "-Wall -fexceptions -pthread -fuse-cxa-atexit -Wundef -Wno-unused-result "
                  "-std=c++17 ";
#elif defined(__APPLE__) && __clang_major__ >= 5
        c_opts << "-Wall -fexceptions -pthread -fuse-cxa-atexit -Wundef -Wno-unused-result "
                  "-std=c++17 ";
#else
        c_opts << "-Wall -fexceptions -pthread -fuse-cxa-atexit -Wundef -Wno-unused-result ";
#endif
        if (!has_any(BUILD::LIB) && sources->size() > 1) {
            l_opts << "-fexceptions -pthread ";
        }
    }

    if (has_any(BUILD::DEB)) {
        c_opts << "-ggdb -O0 -D_DEBUG ";
        if (!has_any(BUILD::LIB) && sources && sources->size() > 1) {
            l_opts << "-ggdb -O0 ";
        }
    } else {
        c_opts << "-O2 ";
        if (!has_any(BUILD::LIB)) {
            l_opts << "-O2 ";
        }
    }
    if (has_any(BUILD::WIDECH))
        c_opts << "-D_UNICODE -DUNICODE ";
    if (sources->size() > 1)
        c_opts << "-c ";
}

// -------------------------------------------------------------------------------------------------
BUILD_STATUS
builder_gcc::build()
{
    if (!sources)
        throw c4s_exception("builder_gcc::build - no sources to build.");
    parse_flags();
    if (has_any(BUILD::EXPORT))
        return BUILD_STATUS::OK;

    // Only one file?
    if (sources->size() == 1) {
        // Combine compile and link.
        ostringstream single;
        path src = sources->front();
        single << c_opts.str();
        single << "-o " << target << ' ' << src.get_base() << ' ';
        single << l_opts.str();
        try {
            if (log && has_any(BUILD::VERBOSE)) {
                *log << "Compiling " << src.get_base() << '\n';
                *log << "Compile parameters: " << single.str() << '\n';
            }
            int rv = compiler.exec(vars.expand(single.str()), 20);
            return  rv ? BUILD_STATUS::ERROR : BUILD_STATUS::OK; 
        } catch (const c4s_exception& ce) {
            if (log)
                *log << "builder_gcc::build - Failed:" << ce.what() << '\n';
        }
        return BUILD_STATUS::ERROR;
    }
    // Make sure build dir exists
    path buildp(build_dir + C4S_DSEP);
    if (!buildp.dirname_exists()) {
        if (log && has_any(BUILD::VERBOSE))
            *log << "builder_gcc - created build directory:" << buildp.get_path() << '\n';
        buildp.mkdir();
    }
    // Call parent to do the job
    if (log && has_any(BUILD::VERBOSE))
        builder::print(*log);
    BUILD_STATUS bs = builder::compile(".o", "-o ", true);
    if (bs == BUILD_STATUS::OK) {
        if (has_any(BUILD::LIB))
            bs = builder::link(".o", 0);
        else
            bs = builder::link(".o", "-o ");
    }
    if (log && has_any(BUILD::VERBOSE))
        *log << "builder_gcc::build - build status = " << (int)bs << "\n";
    return bs;
}

} // namespace c4s