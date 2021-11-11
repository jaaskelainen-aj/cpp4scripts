/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#ifndef C4S_BUILDER_GCC_HPP
#define C4S_BUILDER_GCC_HPP

namespace c4s {

//! Builder for g++ in GCC
class builder_gcc : public builder
{
  public:
    //! g++ builder constructor
    builder_gcc(path_list* sources, const char* name, std::ostream* log)
      : builder(sources, name, log)
    {}

    builder_gcc(const char* name, std::ostream* log)
      : builder(name, log)
    {}

    //! Executes the build.
    BUILD_STATUS build();

  private:
    void parse_flags();
};

}

#endif