/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#ifndef C4S_BUILDER_HPP
#define C4S_BUILDER_HPP

namespace c4s {

class BUILD : public flags32_base
{
  public:
    static const flag32 NONE = 0x000;    //!< No flags.
    static const flag32 DEB = 0x001;     //!< Builds a debug version
    static const flag32 REL = 0x002;     //!< Builds a release version
    static const flag32 EXPORT = 0x004;  //!< Build prepares options for export and skips build step.
    static const flag32 BIN = 0x010;     //!< Builds a normal binary.
    static const flag32 SO = 0x020;      //!< Builds a dynamic library (DLL or SO)
    static const flag32 LIB = 0x040;     //!< Builds a library
    static const flag32 VERBOSE = 0x080; //!< Verbose build, shows more output
    static const flag32 RESPFILE = 0x100;//!< Puts arguments into response file before compiling or linking.
    static const flag32 WIDECH = 0x400;  //!< Uses wide characters i.e. wchar_t
    static const flag32 NOLINK = 0x800;  //!< Only compilation is done at build-command.
    static const flag32 NODEFARGS = 0x1000;  //!< Skip use of default arguments. All build arguments must be specifically added.
    static const flag32 PLAIN_C = 0x2000;    //!< Use C-compilation instead of the default C++.
    static const flag32 NOINCLUDES = 0x4000; //!< Don't check includes for oudated status
    static const flag32 FORCELINK = 0x8000;  //!< Do link step even if no outdated files found.

    BUILD()
      : flags32_base(NONE)
    {}
    explicit BUILD(flag32 fx)
      : flags32_base(fx)
    {}
};

enum class BUILD_STATUS : int
{
    OK,
    TIMEOUT,
    ERROR,
    ABORTED,
    NOTHING_TO_DO
};

/** An abstract builder class that defines an interface to CPP4Scripts builder feature. This is a
    common base class for all specific compilers. It collects options, constructs the binary and
    build directory name and has the build-function interface.
*/
class builder : public BUILD
{
  public:
    // Destructor, releases source list if it was reserved during build process.
    virtual ~builder();

    //! Adds more compiler options to existing ones.
    void add_comp(const std::string& arg);
    //! Adds more linker/librarian options to existing ones.
    void add_link(const std::string& arg);

    //! Adds more object files to be linked.
    void add_link(const path_list& src, const char* obj)
    {
        extra_obj.add(src, build_dir + C4S_DSEP, obj);
    }
    //! Adds a compiled file for linking
    void add_link(const compiled_file& cf);

    //! Reads compiler variables from a file.
    void include_variables(const char* filename = 0);
    //! Inserts single variable to the variable list.
    void set_variable(const std::string& key, const std::string& value)
    {
        vars.push_back(key, value);
    }
    //! Prints current options into given stream
    void print(std::ostream& out, bool list_sources = false);
    //! Returns the padded name.
    std::string get_name() { return name; }
    //! Returns the padded name, i.e. with system specific extension and possibly prepended 'lib'
    std::string get_target_name() { return target; }
    //! Returns relative target path, i.e. padded name with prepended build_dir
    path get_target_path() { return path(build_dir, target); }
    //! Returns generated build directory name. Note: name does not have dir-separator at the end.
    std::string get_build_dir() { return build_dir; }
    //! Sets the timeout value to other than default 15s
    void set_timeout(int to)
    {
        if (to > 0 && to < 300)
            timeout = to;
    }

    //! Generates compiler_commands.json.
    void export_compiler_commands(c4s::path& exp, const c4s::path& src);
    //! Generates cmake CMakeLists.txt.
    void export_cmake(c4s::path& dir);
    //! Generic export creating 'export' directory with export files.
    //! Calls specific exports based on type parameter [cmake|ccdb]
    void export_prj(const std::string& type, c4s::path& exp, const c4s::path& ccd);

    //! Template for the build command
    virtual BUILD_STATUS build() = 0;

    //! Increments the build number in the given file
    static int update_build_no(const char* filename);
    //! Shorthand logic to determine outcome of build/compile
    static bool is_fail_status(BUILD_STATUS bs)
    {
        if(bs == BUILD_STATUS::TIMEOUT || bs == BUILD_STATUS::ERROR)
            return true;
        return false;
    }

  protected:
    //! Protected constructor: Initialize builder with initial list of files to compile
    builder(path_list* sources, const char* name, std::ostream* log);
    //! Protected constructor: File list is read from git.
    builder(const char* name, std::ostream* log);
    //! Executes compile step
    BUILD_STATUS compile(const char* out_ext, const char* out_arg, bool echo_name = true);
    //! Executes link/library step.
    BUILD_STATUS link(const char* out_ext, const char* out_arg);
    //! Check if includes have been changed for named source (or include)
    bool check_includes(const c4s::path& source, int rlevel = 0);

    c4s::variables vars;       //!< Variables list. Compiler arguments are automatically expanded for
                               //!< variables before the execution.
    c4s::process compiler;     //!< Compiler process for this builder
    c4s::process linker;       //!< Linker process for this builder.
    std::ostringstream c_opts; //!< List of options for the compiler.
    std::ostringstream l_opts; //!< List of options for the linker.
    std::ostream* log;        //!< If not null, will receive compiler and linker output.
    c4s::path_list* sources;   //!< List of source files. Relative paths are possible.
    c4s::path_list extra_obj;  //!< Optional additional object files to be included at link step.
    std::string name;          //!< Simple target name.
    std::string target;        //!< Decorated and final target name.
    std::string build_dir;     //!< Generated build directory name. No dir-separater at the end.
    std::string ccdb_root;     //!< Root directory for compiler_commands.json generation.
    int timeout;               //!< Number of seconds to wait before build and link commands fail.
    bool my_sources;           //!< If true builder has allocated the sources list.
    c4s::path current_obj;     //!< Path of the file currently being compiled.
};

} // namespace c4s

#endif