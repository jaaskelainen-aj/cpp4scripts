/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#include <string.h>
#include <time.h>
#if defined(__linux)
#include <stdlib.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <stdlib.h>
#endif
#include "config.hpp"
#include "exception.hpp"
#include "path.hpp"
#include "program_arguments.hpp"
#include "user.hpp"

using namespace std;
using namespace c4s;

// -------------------------------------------------------------------------------------------------
bool
c4s::argument::operator==(const char* t) const
{
    return strcmp(text, t) == 0 ? true : false;
}
// -------------------------------------------------------------------------------------------------
/*!
  \param val Pointer to the value of the second part.
*/
void
c4s::argument::set_value(char* val)
{
    set = true;
    two_part = true;
    if (val[0] == '\'') {
        val[strlen(val) - 1] = 0;
        value = val + 1;
    } else
        value = val;
}

// -------------------------------------------------------------------------------------------------
/*! Valid arguments should have been added into this object before calling this function.
    Exception is thrown if the argument is not found.
  \param argc Parameter to main function = number of arguments in argv
  \param argv Parameter to main function = array of argument strings.
  \param min_args Number of arguments that should be specified in the command line.
                  Excluding the automatic first argument given by the system, i.e. value is zero
  based.
*/
void
c4s::program_arguments::initialize(int argc, char* argv[], int min_args)
{
    // Initialize program paths
    argv0 = argv[0];
    cwd.read_cwd();
    char link_path[512];
#if defined(__linux)
    char link_name[512];
    pid_t pid = getpid();
    sprintf(link_name, "/proc/%d/exe", pid);
    int rv = readlink(link_name, link_path, sizeof(link_path));
#elif defined(__APPLE__)
    uint32_t size = sizeof(link_path);
    int rv = _NSGetExecutablePath(link_path, &size) == 0 ? strlen(link_path) : 0;
#else
    DWORD rv = GetModuleFileName(0, link_path, sizeof(link_path));
#endif
    if (rv > 0) {
        link_path[rv] = 0;
        exe = link_path;
    } else
        exe = argv[0];

    // Initialize the random number generator in case we need to use it.
    srand((unsigned int)time(0));

    if (argc < min_args + 1)
        throw c4s_exception("Arguments initialize - Too few arguments specified in command line.");

    // Check the arguments
    list<argument>::iterator pi;
    int argi = 1;
    while (argi < argc) {
        // For potential arguments:
        for (pi = arguments.begin(); pi != arguments.end(); pi++) {
            // If found:
            if (!strcmp(argv[argi], pi->get_text())) {
                // Set argument on, and copy value if it is twopart param.
                pi->set_on();
                if (pi->is_two_part()) {
                    if (argi + 1 >= argc) {
                        ostringstream os;
                        os << "Arguments initialize - Missing value for argument: "
                           << pi->get_text() << '\n';
                        throw c4s_exception(os.str());
                    }
                    pi->set_value(argv[argi + 1]);
                    argi++;
                }
                break;
            }
        }
        if (pi == arguments.end()) {
            ostringstream os;
            os << "program_arguments::initialize - Unknown argument: " << argv[argi] << '\n';
            throw c4s_exception(os.str());
        }
        argi++;
    }
}

// -------------------------------------------------------------------------------------------------
/*!
  Prints the help on program arguments to stdout. Initialize function should have been called
  before this function.
  \param title A title to print before the arguments.
  \param info A informational message to be printed after the arguments.
*/
void
c4s::program_arguments::usage()
{
    string paramTxt;
    list<argument>::iterator pi;
    cout << "Usage: " << argv0.get_base();
    if (arguments.size() > 0)
        cout << " [Options]\n";
    for (pi = arguments.begin(); pi != arguments.end(); pi++) {
        cout << "  ";
        cout.width(20);
        paramTxt = pi->get_text();
        if (pi->is_two_part())
            paramTxt += " VALUE";
        cout << left << paramTxt << right << pi->get_info() << '\n';
    }
    cout << '\n';
}

// -------------------------------------------------------------------------------------------------
/*!
  \param param Pointer to parameter string.
  \retval bool True if argument is set, false if not.
*/
bool
c4s::program_arguments::is_set(const char* param)
{
    list<argument>::iterator pi;
    for (pi = arguments.begin(); pi != arguments.end(); pi++) {
        if (*pi == param)
            return pi->is_on();
    }
    return false;
}

// -------------------------------------------------------------------------------------------------
/*!
  \param param Pointer to parameter string.
  \param value Pointer to value to check.
  \retval bool True if argument is set and correct, false if not.
*/
bool
c4s::program_arguments::is_value(const char* param, const char* value)
{
    list<argument>::iterator pi;
    for (pi = arguments.begin(); pi != arguments.end(); pi++) {
        if (*pi == param) {
            if (pi->is_on()) {
                // bool val = pi->get_value().compare(value) == 0 ? true : false;
                return pi->get_value().compare(value) == 0 ? true : false;
            }
            return false;
        }
    }
    return false;
}

// -------------------------------------------------------------------------------------------------
/*!
  \param param Pointer to parameter string.
  \retval char* Pointer to value string. Do not store the pointer!
*/
const char*
c4s::program_arguments::get_value_ptr(const char* param)
{
    static string value;
    list<argument>::iterator pi;
    for (pi = arguments.begin(); pi != arguments.end(); pi++) {
        if (!strcmp(pi->get_text(), param) && pi->is_two_part()) {
            pi->get_value(value);
            return value.c_str();
        }
    }
    return "";
}

// -------------------------------------------------------------------------------------------------
/*!
   \param param Parameter name.
   \param choises Array of strings terminated by null pointer.
   \retval int Index of the matched choise. -1 if no match or argument is not value type.
*/
int
c4s::program_arguments::get_value_index(const char* param, const char** choises)
{
    const char* value = get_value_ptr(param);
    if (*value == 0)
        return -1;
    for (int ndx = 0; choises[ndx]; ndx++) {
        if (!strcmp(value, choises[ndx]))
            return ndx;
    }
    return -1;
}

// -------------------------------------------------------------------------------------------------
/*!
  \param param Pointer to parameter string
  \retval string Argument value. If value is not specified or argument is not two part type an empty
  string is returned.
*/
string
c4s::program_arguments::get_value(const char* param)
{
    list<argument>::iterator pi;
    for (pi = arguments.begin(); pi != arguments.end(); pi++) {
        if (*pi == param && pi->is_two_part()) {
            // string value;
            // pi->get_value(value);
            return pi->get_value();
        }
    }
    return string("");
}

// -------------------------------------------------------------------------------------------------
/*!
   \param param Ptr to argument name
   \param value Ptr to value string.
   \retval bool True on succes, false if argument is not found.
*/
bool
c4s::program_arguments::set_value(const char* param, char* value)
{
    list<argument>::iterator pi;
    for (pi = arguments.begin(); pi != arguments.end(); pi++) {
        if (!strcmp(pi->get_text(), param)) {
            pi->set_value(value);
            return true;
        }
    }
    return false;
}

// -------------------------------------------------------------------------------------------------
/*!
  \param param Ptr to argument name.
  \param value Value to append
  \retval bool True on succes, false if argument is not found.
*/
bool
c4s::program_arguments::append_value(const char* param, char* value)
{
    list<argument>::iterator pi;
    for (pi = arguments.begin(); pi != arguments.end(); pi++) {
        if (!strcmp(pi->get_text(), param)) {
            pi->append_value(value);
            return true;
        }
    }
    return false;
}
