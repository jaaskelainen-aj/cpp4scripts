/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#ifndef C4S_VARIABLES_HPP
#define C4S_VARIABLES_HPP

namespace c4s {

class path;
// -----------------------------------------------------------------------------------------------------------
/// Variable substitution class
/*! Variables are simple string substitutions, i.e. one string is replaced with another. Variables
 are stored in STL map container. What makes this class convenient is the ability to read variable
 definitions from text files are run-time.
 Variable files follow unix common configuration files syntax: variable name is followed by equal
 sign. Rest of the line is taken as a value to variable. '#' can be used as a comment. Blank lines
 are ignored.
*/
class variables
{
  public:
    variables() {}
    variables(const path& p) { include(p); }
    void include(const path&);
    std::string expand(const std::string&, bool se = false);
    void push_back(const std::string& key, const std::string& value) { vmap[key] = value; }

  protected:
    std::map<std::string, std::string> vmap;
};

}

#endif
