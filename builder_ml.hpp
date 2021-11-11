/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#ifndef C4S_BUILDER_ML_HPP
#define C4S_BUILDER_ML_HPP

namespace c4s {

//! Builder for the Microsoft Macro Assembler
class builder_ml : public builder
{
  public:
    //! Constructor for Microsoft Macro Assembler
    builder_ml(path_list* sources, const char* name, ostream* log);
    int build();
};
}

#endif