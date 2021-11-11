/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#include <cstring>

#include "config.hpp"
#include "exception.hpp"
#include "variables.hpp"
#include "path.hpp"
#include "path_list.hpp"
#include "compiled_file.hpp"
#include "user.hpp"
#include "process.hpp"
#include "util.hpp"
#include "builder.hpp"

using namespace std;

// -------------------------------------------------------------------------------------------------
/**
  Reads the given include file and adds variable definitions from it to the given variable list.
  Variables have following syntax: "name = value". Anything before equal-sign is taken as name of
  the variable. Anything following the equal sign is taken as the value of variable. Any whitespace
  around the equal sign is discarded.

  In Windows variable values are searched for $$-marks. These are replaced with current build
  architecture's word length i.e. 32 or 64.

  \param file Path to the file to be sourced.
*/
void
c4s::variables::include(const path& inc_file)
{
    char *eq, *ptr, line[MAX_LINE];
    string key;
    int lineno = 0;

    ifstream inc(inc_file.get_path().c_str(), ios::in);
    if (!inc) {
        ostringstream os;
        os << "Unable to open include file '" << inc_file.get_path();
        throw path_exception(os.str());
    }
    do {
        lineno++;
        inc.getline(line, sizeof(line));
        if (inc.fail() && !inc.eof()) {
            ostringstream os;
            os << "Insufficient buffer in c4s::include_vars reading file: " << inc_file.get_base()
               << " on line: " << lineno;
            throw c4s_exception(os.str());
        }

        // Ignore empty and comment lines.
        if (line[0] == '#' || line[0] == '\r' || line[0] == '\n' || line[0] == ' ' ||
            line[0] == '\t')
            continue;

        // If no equal sign: continue.
        eq = strchr(line, '=');
        if (!eq || eq == line)
            continue;
        // Trim the end of the key
        ptr = eq;
        do {
            ptr--;
        } while (ptr > line && (*ptr == ' ' || *ptr == ':' || *ptr == '\t'));
        if (ptr == line && (*ptr == ' ' || *ptr == '\t'))
            continue;
        key.assign(line, ptr - line + 1);

        // Trim beginning of value
        ptr = eq;
        do {
            ptr++;
        } while ((*ptr == ' ' || *ptr == '\t') && *ptr);
        if (!ptr)
            continue;
        eq = ptr;

        // Trim the end of the value
        ptr = eq + strlen(eq);
        do {
            ptr--;
        } while ((*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') && ptr != eq);
        if (ptr == eq)
            continue;
        *(ptr + 1) = 0;
#ifdef _WIN32
        exp_arch(builder::get_arch(), eq);
#endif
        // insert key and value to the map
#ifdef C4S_DEBUGTRACE
        cerr << "variables::include - adding key=" << key << "; value=" << eq << endl;
#endif
        vmap[key] = string(eq);

    } while (!inc.eof());
    inc.close();
}
// -------------------------------------------------------------------------------------------------
/** Expands variables in the given source string. Variables have form $(name). Passed variables will
  always override the environment variables. Throws 'c4s_exception' if variable is not
  found.
   \param source Source string to replace
   \param se Search environment flag. If true then the environment variables are searched as well.
   \retval string expanded string.
*/
string
c4s::variables::expand(const string& source, bool se)
{
    ostringstream result;
    string name, enval;
    map<string, string>::iterator mi;
    size_t prev = 0, strend;

    if (!vmap.size() && !se) {
        return string(source);
    }
    size_t offset = source.find("$(");
    while (offset != string::npos) {
        result << source.substr(prev, offset - prev);
        strend = source.find(')', offset + 3);
        if (strend == string::npos)
            throw c4s_exception("Variable syntax error.");
        name = source.substr(offset + 2, strend - offset - 2);
        mi = vmap.find(name);
        if (mi == vmap.end()) {
            if (!se) {
                ostringstream os;
                os << "Variable " << name << " definition not found.";
                throw c4s_exception(os.str());
            }
            if (!get_env_var(name.c_str(), enval)) {
                ostringstream os;
                os << "Variable " << name << " not found from environment nor variable list.";
                throw c4s_exception(os.str());
            }
            result << enval;
        } else {
            result << mi->second;
        }
        prev = strend + 1;
        offset = source.find("$(", prev);
    }
    result << source.substr(prev);
    return result.str();
}
