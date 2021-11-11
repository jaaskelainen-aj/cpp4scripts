/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#ifndef C4S_BUILDER_VC_HPP
#define C4S_BUILDER_VC_HPP

namespace c4s {

//! Builder for Microsoft Visual Studio
class builder_vc : public builder
{
  public:
    //! Constructor for Microsoft Visual Studio builder.
    builder_vc(path_list* sources, const char* name, ostream* log);
    int build();
    //! Helper function for MS VC to create precompiled headers.
    int precompile(const char* name, const char* content, const char* stopname);
    //! Sets static VisualStudio runtime (MT). Default is the dynamic (MD)
    void setStaticRuntime() { static_runtime = true; }

  private:
    bool precompiled;
    bool static_runtime;
};
}

#endif