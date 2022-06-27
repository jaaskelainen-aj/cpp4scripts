/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#ifndef C4S_UTIL_HPP
#define C4S_UTIL_HPP

#include <string>
#include <unordered_map>
#include <stdint.h>

#ifdef C4S_DEBUGTRACE
//#pragma GCC warning c4slog is enabled
#include <iostream>
extern std::ofstream c4slog;
#endif

namespace c4s {

// inline void cp(const char *from, const char *to) { path source(from); source.cp(to); }
// inline void cp_to_dir(const char *from, path &to, path::cpf flags=path::NONE) { path
// source(from); source.cp(to,flags); }
enum DATETYPE
{
    DATE_ONLY,
    WITH_TIME
};
bool exists_in_path(const char* file);
#ifdef _WIN32
//! Provides error string output from Windows system with the same name as the linux 'strerror'
//! fuction.
const char* strerror(int);
#endif
//! Returns a string designating the build type. Eg '64bit-Release'
const char* get_build_type();

//! Returns todays timestamp in ISO format
const char* get_ISO_date(DATETYPE);

//! Makes sure that the path separators int the given target string are native type.
std::string force_native_dsep(const std::string&);

//! Gets an environment value.
bool get_env_var(const char*, std::string&);

//! Matches the wildcard to the string and returns true if it succeeds.
bool match_wildcard(const char* pWild, const char* pString);

//! Returns users login or real name as configured to the system.
std::string get_user_name(bool loginname = true);

//! Returns current hostname
const char* get_host_name();

//! Searches needle in the target file.
bool search_file(std::fstream& target, const std::string& needle);

//! Efficient string search function.
bool search_bmh(const unsigned char* haystack,
           SSIZE_T hlen,
           const unsigned char* needle,
           SSIZE_T nlen,
           SIZE_T* offset_out);

//! Waits for input on stdin
bool wait_stdin(int timeout);

//! Appends a slash tot he end of the std::string if it does not have one
std::string append_slash(const std::string&);

//! Creates a 'next' available filename into the base part based on given wild card.
bool generate_next_base(path& target, const char* wild);

#if defined(__linux) || defined(__APPLE__)
//! Maps the mode from numeric hex to linux symbolic
mode_t hex2mode(int);

//! Maps the mode from linux symbolic into numeric hex
int mode2hex(mode_t);

//! Reads the current file mode from named file.
int get_path_mode(const char* pname);

//! Sets owner and mode recursively to entire subtree. Use with care. (Linux&Apple only)
void set_owner_mode(const char* dirname, int userid, int groupid, int dirmode, int filemode);
#endif

uint64_t fnv_hash64_str(const char* str, size_t len, uint64_t salt);
uint64_t fnv_hash64_file(const char* path, uint64_t salt);
bool has_anybits(uint32_t target, uint32_t bits);
bool has_allbits(uint32_t target, uint32_t bits);

//! Two sided trim
void trim(std::string&);

//! Parse given string into map
bool parse_key_values(const char* str, 
                        std::unordered_map<ntbs, ntbs>& kv,
                        char separator=',');

typedef unsigned int flag32;

class flags32_base
{
  public:
    flags32_base(flag32 _value)
      : value(_value)
    {}
    flags32_base()
      : value(0)
    {}

    flag32 get() { return value; }
    bool has_any(flag32 bits) { return (value & bits) > 0 ? true : false; }
    bool has_all(flag32 bits) { return (value & bits) == bits ? true : false; }
    void set(flag32 bits) { value = bits; }
    void clear(flag32 bits) { value &= ~bits; }

    void add(flag32 bits) { value |= bits; }
    flag32& operator|=(flag32 bits)
    {
        value |= bits;
        return value;
    }

  protected:
    flag32 value;
};

} // namespace c4s
#endif
