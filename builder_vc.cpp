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
#include "builder_vc.hpp"

using namespace std;
using namespace c4s;

#ifdef _WIN32

// -------------------------------------------------------------------------------------------------
/*! Creates Visual Studio compiler defaults. Depracated function warnings are on by default. You may
   want to use the following: /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS \param sources
   List of source files to compile \param name Name of the target (without extension) \param log Log
   target \param flags Build flags \param subsys Optional subsystem indicator that is appended after
   the build dir name.
*/
c4s::builder_vc::builder_vc(path_list* _sources,
                            const char* _name,
                            ostream* _log,
                            const int _flags,
                            const char* subsys)
  : builder(_sources, _name, _log, _flags, subsys)
{
    string pdb;

    compiler.set_command("cl");
    if (log && has_any(BUILD::VERBOSE)) {
        const char* arch = has_any(BUILD::X32) ? "32" : "64";
        *log << "build_vc - " << arch << "-bit environment detected.\n";
    }
    precompiled = false;
    static_runtime = false;

    // Set the default flags
    if (!has_any(BUILD::NODEFARGS)) {
        c_opts << "/nologo /EHsc /Gy /W3 /Ob1 /Gd ";
        if (has_any(BUILD::DEB)) {
            c_opts << "/Od /Zi /GS /RTC1 ";
        } else {
            c_opts << "/O2 /GS- ";
        }
        if (has_any(BUILD::WIDECH))
            c_opts << "/Zc:wchar_t /DUNICODE /D_UNICODE ";
        if (has_any(BUILD::X64))
            c_opts << "/favor:blend ";
        if (sources && sources->size() > 1)
            c_opts << "/c ";
    }
    // Determine the target name
    target = name;
    if (has_any(BUILD::LIB)) {
        linker.set_command("lib");
        target += ".lib";
    } else {
        linker.set_command("link");
        target += has_any(BUILD::BIN) ? ".exe" : ".dll";
    }
    if (has_any(BUILD::PAD_NAME)) {
        pad_name(name, subsys, flags);
        pad_name(target, subsys, flags);
    }

    if (!has_any(BUILD::NODEFARGS) && !has_any(BUILD::NOLINK)) {
        l_opts << "/NOLOGO ";
        if (has_any(BUILD::X64))
            l_opts << "/MACHINE:X64 ";
        if (has_any(BUILD::LIB))
            return;
        if (has_any(BUILD::SO))
            l_opts << "/DLL /IMPLIB:" << build_dir << C4S_DSEP << name << ".lib ";
        if (!has_any(BUILD::LIB)) {
            if (has_any(BUILD::DEB))
                l_opts << "/DEBUG ";
            else
                l_opts << "/RELEASE ";
        }
    }
}

// -------------------------------------------------------------------------------------------------
int
c4s::builder_vc::build()
{
    if (!sources)
        throw c4s_exception("builder_vc::build - sources have not been defined.");
    // We add this flag at the last minute to make sure caller has time to set the static_runtime
    // flag.
    if (has_any(BUILD::DEB)) {
        if (static_runtime)
            c_opts << "/MTd ";
        else
            c_opts << "/MDd ";
    } else {
        if (static_runtime)
            c_opts << "/MT ";
        else
            c_opts << "/MD ";
    }
    // Only one file?
    if (sources->size() == 1) {
        // Combine compile+link.
        ostringstream single;
        path src = sources->front();
        single << " /Fe" << target;
        if (has_any(BUILD::DEB))
            single << " /Fd" << name << ".pdb";
        single << ' ' << c_opts.str() << ' ' << src.get_base() << " /link " << l_opts.str();
        if (log && has_any(BUILD::VERBOSE))
            *log << "Compiling: " << vars.expand(single.str()) << '\n';
        return compiler.exec(20, vars.expand(single.str()).c_str());
    }
    if (has_any(BUILD::DEB) || precompiled) {
        c_opts << "/Fd" << build_dir << C4S_DSEP << name << ".pdb ";
        if (!has_any(BUILD::LIB))
            l_opts << "/PDB:" << build_dir << C4S_DSEP << name << ".pdb ";
    }
    // Make sure build directory exists.
    path buildp(build_dir + C4S_DSEP);
    if (!buildp.dirname_exists()) {
        if (log && has_any(BUILD::VERBOSE))
            *log << "builder_vc - created build directory:" << buildp.get_path() << '\n';
        buildp.mkdir();
    }
    // Call parent to do the job.
    if (log && has_any(BUILD::VERBOSE))
        builder::print(*log);
    int rv = builder::compile(".obj", "/Fo", false);
    if (!rv)
        return builder::link(".obj", "/OUT:");
    return rv;
}
// -------------------------------------------------------------------------------------------------
int
c4s::builder_vc::precompile(const char* pchname, const char* content, const char* stopname)
/*! Creates a precompiled header file. Function creates a dummy cpp-file with required headers
    and then invokes the precompiled header construction for it.
   \param pchname Target file name. (.pch automatically appended)
   \param content Header includes to put into dummy file.
   \param stopname Name of the header that stops the precompilation.
   \retval int Zero on success. Compiler error otherwice.
*/
{
    // If we already have this, simply add the required precompile options.
    path pch(build_dir, pchname, ".pch");
    pch.unix2dos();
    if (pch.exists()) {
        c_opts << "/Yu" << stopname << " /Fp" << pch.get_path() << ' ';
        return 0;
    }
    // Make sure build directory exists.
    if (!pch.dirname_exists()) {
        pch.mkdir();
        if (log && has_any(BUILD::VERBOSE))
            *log << "builder_vc - created build directory:" << pch.get_dir() << '\n';
    }

    // Create a dummy precompile file.
    path cpp(build_dir, pchname, ".cpp");
    ofstream pchsrc(cpp.get_path().c_str());
    if (!pchsrc) {
        if (*log)
            *log << "builder_vc::precompile - unable to create precompile source:" << cpp.get_path()
                 << '\n';
        return 1;
    }
    pchsrc << content << '\n';
    pchsrc.close();

    // We add this flag at the last minute to make sure caller has time to set the static_runtime
    // flag.
    if (has_any(BUILD::DEB)) {
        if (static_runtime)
            c_opts << "/MTd ";
        else
            c_opts << "/MDd ";
    } else {
        if (static_runtime)
            c_opts << "/MT ";
        else
            c_opts << "/MD ";
    }

    // Allright - ready to precompile
    ostringstream params;
    params << vars.expand(c_opts.str());
    params << " /I. /Yc /Fp" << pch.get_path();
    params << " /Fd" << build_dir << C4S_DSEP << name << ".pdb";
    if (c_opts.str().find("/c") == string::npos)
        params << " /c";
    params << ' ' << cpp.get_path();
    if (log && has_any(BUILD::VERBOSE))
        *log << "Precompiling: " << params.str() << '\n';
    int rv = compiler.exec(40, params.str().c_str());
    if (!rv)
        c_opts << "/Yu" << stopname << " /Fp" << pch.get_path() << ' ';
    return rv;
}
#endif _WIN32
