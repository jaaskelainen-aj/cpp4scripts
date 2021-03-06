/*! \file c4s_builder_vc.cpp
 * \brief Microsoft (R) Macro Assembler builder */
// Copyright (c) Menacon Oy
/*******************************************************************************
This file is part of Cpp4Scripts library.

  Cpp4Scripts is free software: you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation, either version 3 of the License, or (at your option) any
  later version.

  Cpp4Scripts is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details:
  http://www.gnu.org/licenses/lgpl.html

Copyright (c) Menacon Ltd, Finland
*******************************************************************************/

#ifdef C4S_LIB_BUILD
 #include "c4s_config.hpp"
 #include "c4s_exception.hpp"
 #include "c4s_variables.hpp"
 #include "c4s_util.hpp"
 #include "c4s_path.hpp"
 #include "c4s_path_list.hpp"
 #include "c4s_process.hpp"
 #include "c4s_builder.hpp"
 #include "c4s_builder_ml.hpp"
 #include "c4s_util.hpp"
 using namespace c4s;
#endif

#ifdef _WIN32

// ------------------------------------------------------------------------------------------
c4s::builder_ml::builder_ml(path_list *_sources, const char *_name, ostream *_log, const int _flags, const char *subsys)
    : builder(_sources,_name,_log,_flags,subsys)
{
    string pdb;

    compiler.set_command("ml64");

    const char *arch = has_any(BUILD::X32)?"32":"64";
    if(log && has_any(BUILD::VERBOSE)){
        *log << "builder_ml - "<<arch<<"bit environment detected.\n";
    }

    // Set the default flags
    if( !has_any(BUILD::NODEFARGS)) {
        c_opts << "/nologo ";
        if(has_any(BUILD::DEB)) {
            c_opts << "/Zi ";
        }
        if(sources && sources->size()>1)
            c_opts << "/c ";
        if(has_any(BUILD::X32))
            c_opts << "/coff ";
    }
    // Set the target name
    target = name;
    if( has_any(BUILD::LIB) ) {
        linker.set_command("lib");
        target += ".lib";
    } else if(!has_any(BUILD::NOLINK)){
        linker.set_command("link");
        target += ".exe";
    }
    if(has_any(BUILD::PAD_NAME)) {
        pad_name(name,subsys,flags);
        pad_name(target,subsys,flags);
    }
    if( !has_any(BUILD::NODEFARGS) && !has_any(BUILD::NOLINK)) {
        l_opts << "/nologo /SUBSYSTEM:CONSOLE ";
        if( has_any(BUILD::DEB) ) {
            l_opts << "/DEBUG ";
        }
        else {
            l_opts << "/RELEASE ";
        }
        if( has_any(BUILD::X64) )
            l_opts << "/MACHINE:X64 ";
    }
}

// ==================================================================================================
int c4s::builder_ml::build()
{
    if(!sources)
        throw c4s_exception("builder_ml::build - build source files not defined.");
    // Only one file?
    if(sources->size()==1) {
        // Combine compile+link.
        ostringstream single;
        path src = sources->front();
        single << " /Fo"<<target;
        single << ' ' << c_opts.str() << ' '<< src.get_base()<<" /link " << l_opts.str();
        if(log && has_any(BUILD::VERBOSE) )
            *log << "Compiling: "<<vars.expand(single.str())<<'\n';
        return compiler.exec(20,vars.expand(single.str()).c_str());
    }
    // Make sure build directory exists.
    path buildp(build_dir+C4S_DSEP);
    if(!buildp.dirname_exists()) {
        buildp.mkdir();
        if(log && has_any(BUILD::VERBOSE) )
            *log << "builder_ml - created build directory:"<<buildp.get_path()<<'\n';
    }
    // Call parent to do the job.
    if(log && has_any(BUILD::VERBOSE) )
        builder::print(*log);
    int rv = builder::compile(".obj","/Fo ",false);
    if(!rv && !has_any(BUILD::NOLINK))
        return builder::link(".obj","/OUT:");
    return rv;
}
#endif _WIN32
