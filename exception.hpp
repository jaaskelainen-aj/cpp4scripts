/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#ifndef C4S_EXCEPTION_HPP
#define C4S_EXCEPTION_HPP

#include <cstring>
#include <stdexcept>

namespace c4s {

//! Exception for process errors.
class c4s_exception : public std::runtime_error
{
  public:
    c4s_exception(const std::string& m)
      : runtime_error(m)
    {}
};
//! Process exception
class process_exception : public c4s_exception
{
  public:
    process_exception(const std::string& m)
      : c4s_exception(m)
    {}
};
//! Process timeout exception
class process_timeout : public c4s_exception
{
  public:
    process_timeout(const std::string& m)
      : c4s_exception(m)
    {}
};

//! Exceptions for path errors.
class path_exception : public c4s_exception
{
  public:
    path_exception(const std::string& m)
      : c4s_exception(m)
    {}
};

} // namespace c4s

#endif
