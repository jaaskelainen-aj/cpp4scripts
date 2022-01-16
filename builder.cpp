/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#include <cstring>
#include <stdlib.h>

#include "config.hpp"
#include "exception.hpp"
#include "variables.hpp"
#include "path.hpp"
#include "path_list.hpp"
#include "process.hpp"
#include "util.hpp"
#include "compiled_file.hpp"
#include "builder.hpp"

using namespace std;

namespace c4s {

// -------------------------------------------------------------------------------------------------
/** builder base class constructor.
    Handles the debug/release selection.
    \param _sources List of source files to compile.
    \param _name Name of the target binary
    \param _log If specified, will receive compiler output.
    \param _flags Combination of build flags
*/
builder::builder(path_list* _sources, const char* _name, iostream* _log)
  : log(_log)
  , name(_name)
{
    sources = _sources;
    my_sources = false;
    if (log) {
        compiler.pipe_to(log);
        compiler.pipe_err(log);
        linker.pipe_to(log);
        linker.pipe_err(log);
    }
    timeout = 30;
}
// -------------------------------------------------------------------------------------------------
/** builder base class constructor.
    Files are automatically read from git filelist. Files with cpp-extension are considered part of
    of the project.
    \param _name Name of the target binary
    \param _log If specified, will receive compiler output.
    \param _flags Combination of build flags
*/
builder::builder(const char* _name, iostream* _log)
  : log(_log)
  , name(_name)
{
    if (log) {
        compiler.pipe_to(log);
        compiler.pipe_err(log);
        linker.pipe_to(log);
        linker.pipe_err(log);
    }
    timeout = 20;
    try {
        sources = new path_list();
        my_sources = true;
        char gitline[128];
        stringstream gitfiles;
        process("git", "ls-files", &gitfiles)(20);
        do {
            gitfiles.getline(gitline, sizeof(gitline));
            // cout<<gitline<<'\n';
            if (strstr(gitline, ".cpp"))
                sources->add(path(gitline));
        } while (!gitfiles.eof() && gitfiles.gcount() > 0);
    } catch (const c4s_exception& ce) {
        if (_log)
            *_log << "Unable to read source list from git: " << ce.what() << '\n';
    }
}
// -------------------------------------------------------------------------------------------------
builder::~builder()
{
    if (my_sources && sources)
        delete sources;
}

// -------------------------------------------------------------------------------------------------
/** Read build variables from a given file.
    If the file is not specified environment variable C4S_VARIABLES will be read and its value used as a
    path to variable file. 
    \param filename Optional name of the variable file to be included
 */
void
builder::include_variables(const char* filename)
{
    string c4s_var_name;
    if (!filename) {
        if (!get_env_var("C4S_VARIABLES", c4s_var_name)) {
            throw c4s_exception("builder::include_variables - Unable to find 'C4S_VARIABLES' "
                                "environment variable.");
        }
    } else {
        c4s_var_name = filename;
    }
    path inc_path(c4s_var_name);
    if (!inc_path.exists()) {
        ostringstream error;
        error << "builder::include_variables - C4S_VARIABLES file " << c4s_var_name
              << " does not exist.";
        throw c4s_exception(error.str());
    }
    if (log && has_any(BUILD::VERBOSE))
        *log << "builder - including variables from: " << inc_path.get_path() << '\n';
    vars.include(inc_path);
}
// -------------------------------------------------------------------------------------------------
void
builder::add_comp(const string& arg)
{
    if (!arg.empty())
        c_opts << vars.expand(arg, true) << ' ';
}
// -------------------------------------------------------------------------------------------------
void
builder::add_link(const string& arg)
{
    if (!arg.empty())
        l_opts << vars.expand(arg, true) << ' ';
}
// -------------------------------------------------------------------------------------------------
void
builder::add_link(const compiled_file& cf)
{
    extra_obj.add(cf.target);
}
// -------------------------------------------------------------------------------------------------
bool
builder::check_includes(const path& source, int rlevel)
{
#ifdef _DEBUG
    if (log && has_any(BUILD::VERBOSE))
        *log << "check include at level "<<rlevel<<" :"<<source.get_base()<<'\n';
#endif
    ifstream sf(source.get_pp(), ios::in);
    if (!sf) {
        ostringstream os;
        os << "Outdate check - Unable to find source file:" << source.get_path().c_str();
        throw c4s_exception(os.str());
    }

    char *end, line[250];
    int count = 0;
    while (count < 80) {
        // Read next line from source code
        sf.getline(line, sizeof(line));
        count++;
        // Check for start of first function
        if (line[0] == '{' || sf.eof())
            break;
        if (strncmp("#include \"", line, 10))
            continue;
        end = strchr(line + 10, '\"');
        if (!end)
            continue;
        // Make path to include file
        *end = 0;
        path inc_path(source);
        inc_path += line + 10;
        // Compare file times with original source
        inc_path.read_changetime();
        if (current_obj.compare_times(inc_path) < 0)
            return true;
        if (rlevel < C4S_BUILD_RECURSIVE_CHECK_MAX && check_includes(inc_path, rlevel + 1))
            return true;
    }
    sf.close();
    return false;
}
// -------------------------------------------------------------------------------------------------
BUILD_STATUS
builder::compile(const char* out_ext, const char* out_arg, bool echo_name)
{
    list<path>::iterator src;
    ostringstream options;
    bool exec = false;
    bool logging = log && has_any(BUILD::VERBOSE);

    if (!sources)
        throw c4s_exception("builder::compile - sources not defined!");

    string prepared(vars.expand(c_opts.str()));
    try {
        if (logging)
            *log << "Considering " << sources->size() << " source files for build.\n";
        for (src = sources->begin(); src != sources->end(); src++) {
            current_obj.set(build_dir + C4S_DSEP, src->get_base_plain(), out_ext);
            if (src->outdated(current_obj) || (!has_any(BUILD::NOINCLUDES) && check_includes(*src))) {
                if (compiler.is_running() && compiler.wait_for_exit(timeout)) {
                    return BUILD_STATUS::ERROR;
                }
                if (log && echo_name)
                    *log << src->get_base() << " >>\n";
                options.str("");
                options << prepared;
                options << ' ' << out_arg << current_obj.get_path();
                options << ' ' << src->get_path();
                if (log && has_any(BUILD::VERBOSE))
                    *log << "  " << options.str() << '\n';
                compiler.start(options.str().c_str());
                exec = true;
            }
        }
        current_obj.clear();
        if (compiler.is_running()) {
            compiler.wait_for_exit(timeout);
        }
        if (!exec) {
            if (has_any(BUILD::FORCELINK)) {
                if (logging)
                    *log << "No outdated source files found but forcing link step.\n";
                return BUILD_STATUS::OK;
            }
            if (logging)
                *log << "No outdated source files found.\n";
            return BUILD_STATUS::NOTHING_TO_DO;
        }
    } catch (const process_timeout& pt) {
        if (log)
            *log << "builder::compile - timeout";
        return BUILD_STATUS::TIMEOUT;
    } catch (const c4s_exception& ex) {
        if (log)
            *log << "builder::compile - " << ex.what() << '\n';
        return BUILD_STATUS::ERROR;
    }
    return compiler.last_return_value() ? BUILD_STATUS::ERROR : BUILD_STATUS::OK;
}
// -------------------------------------------------------------------------------------------------
BUILD_STATUS
builder::link(const char* out_ext, const char* out_arg)
{
    BUILD_STATUS bs;
    ostringstream options;
    if (!sources)
        throw c4s_exception("builder::link - sources not defined!");
    if (!out_ext)
        throw c4s_exception("builder::link - link file extenstion missing. Unable to link.");
    path_list linkFiles(*sources, build_dir + C4S_DSEP, out_ext);
    try {
        if (log && has_any(BUILD::VERBOSE))
            *log << "Linking " << target << '\n';
        if (has_any(BUILD::LIB))
            options << ' ' << vars.expand(l_opts.str()) << ' ';
        if (out_arg)
            options << out_arg;
        options << build_dir << C4S_DSEP << target << ' ';
        if (has_any(BUILD::RESPFILE)) {
            string respname(name + ".resp");
            if (log && has_any(BUILD::VERBOSE))
                *log << "builder::link - using response file: " << respname << '\n';
            ofstream respf(respname.c_str());
            if (!respf)
                throw c4s_exception("builder::link - Unable to open linker response file.\n");
            respf << linkFiles.str('\n', false);
            if (extra_obj.size())
                respf << extra_obj.str('\n', false);
            respf.close();
            options << '@' << respname;
        } else {
            options << linkFiles.str(' ', false);
            if (extra_obj.size())
                options << ' ' << extra_obj.str(' ', false);
        }
        if (!has_any(BUILD::LIB))
            options << ' ' << vars.expand(l_opts.str());
        if (log && has_any(BUILD::VERBOSE))
            *log << "Link options: " << options.str() << '\n';
        int rv = linker.exec(options.str(), 3 * timeout);
        if(rv) {
            if(log)
                *log << "builder::link - returned: "<<rv<<'\n';
            bs = BUILD_STATUS::ERROR;
        } else
            bs = BUILD_STATUS::OK;
    } catch (const process_timeout &pt) { 
        if (log)
            *log << "builder::link - timeout\n";
        return BUILD_STATUS::TIMEOUT;
    } catch (const c4s_exception& ce) {
        if (log)
            *log << "builder::link - Error: " << ce.what() << '\n';
        return BUILD_STATUS::ERROR;
    }
#ifndef _DEBUG
    if (has_any(BUILD::RESPFILE))
        path("cpp4scripts_link.resp").rm();
#endif
    return bs;
}
// -------------------------------------------------------------------------------------------------
/**
   \param os Reference to output stream.
   \param list_sources If true, lists source files as well.
 */
void
builder::print(ostream& os, bool list_sources)
{
    using namespace std;
    os << "  COMPILER options: " << c_opts.str() << '\n';
    if (has_any(BUILD::LIB))
        os << "  LIB options: " << l_opts.str() << '\n';
    else
        os << "  LINK options: " << l_opts.str() << '\n';

    if (list_sources) {
        os << "  Source files:\n";
        list<path>::iterator src;
        for (src = sources->begin(); src != sources->end(); src++) {
            os << "    " << src->get_base() << '\n';
        }
    }
}
// -------------------------------------------------------------------------------------------------
/** Function opens the named file and increments the last number seen in the file. No special tags
    are needed. File is expected to be very short, just a variable declaration with version
    string.
    \param filename Relative path to the version file.
 */
int
builder::update_build_no(const char* filename)
{
    char *vbuffer, *tail, *head, *dummy, bno_str[24];
    ifstream fbn(filename);
    if (!fbn) {
        // cout << "builder::update_build_no - Unable to open given file for reading.\n";
        return -1;
    }
    fbn.seekg(0, ios_base::end);
    size_t max = (size_t)fbn.tellg();
    if (max > 255) {
        // cout << "builder::update_build_no - Build number file is too large to handle.\n";
        fbn.close();
        return -2;
    }
    vbuffer = new char[max];
    fbn.seekg(0, ios_base::beg);
    fbn.read(vbuffer, max);
    tail = vbuffer + max - 1;
    fbn.close();

    // Search the last number.
    while (tail > vbuffer) {
        if (*tail >= '0' && *tail <= '9')
            break;
        tail--;
    }
    if (tail == vbuffer) {
        // cout << "builder::update_build_no - Number not found from build number file.\n";
        delete[] vbuffer;
        fbn.close();
        return -3;
    }
    head = tail++;
    while (head > vbuffer) {
        if (*head < '0' || *head > '9')
            break;
        head--;
    }
    if (head > vbuffer)
        head++;
    // Get and update number.
    long bno = strtol(head, &dummy, 10);
    int bno_len = sprintf(bno_str, "%ld", bno + 1);

    // Rewrite the number file.
    ofstream obn(filename, ios_base::out | ios_base::trunc);
    if (head > vbuffer)
        obn.write(vbuffer, head - vbuffer);
    obn.write(bno_str, bno_len);
    obn.write(tail, max - (tail - vbuffer));
    obn.close();
    delete[] vbuffer;
    return 0;
}
// -----------------------------------------------------------------------------------------------------------------
void
builder::export_prj(const std::string& type, path& exp, const path& ccd)
{
    path expdir(exp);
    expdir.append_dir("export");
    if (!expdir.dirname_exists())
        expdir.mkdir();
    cout << "Exporting project into:" << expdir.get_path() << "\n";
    if (!type.compare("cmake"))
        export_cmake(expdir);
    else if (!type.compare("ccdb"))
        export_compiler_commands(expdir, ccd);
    else
        cout << "Unknown export type.\n";
}
// -----------------------------------------------------------------------------------------------------------------
/// \todo Move export_compiler_commands function to compiler specific code.
void
builder::export_compiler_commands(path& exp, const path& ccd)
{
    list<path>::iterator src;
    ostringstream options;
    bool first_ccdb = true;

    if (!sources) {
        cout << "builder::export - sources not defined!\n";
        return;
    }
    exp.set_base("compile_commands.json");
    ofstream cc_db(exp.get_pp());
    if (!cc_db) {
        throw c4s_exception("builder::compile - unable to create compile_commands.json");
    }
    cc_db << "[\n";

    string prepared(vars.expand(c_opts.str()));
    for (src = sources->begin(); src != sources->end(); src++) {
        path objfile(build_dir + C4S_DSEP, src->get_base_plain(), ".o");
        cout << src->get_base() << " >>\n";
        options.str("");
        options << prepared;
        options << " -o " << objfile.get_path();
        options << ' ' << src->get_base();

        path srcdir(ccd);
        srcdir.merge(*src);

        if (first_ccdb)
            first_ccdb = false;
        else
            cc_db << ",\n";
        cc_db << "{\n";
        cc_db << "\"directory\":\"" << srcdir.get_dir_plain() << "\",\n";
        cc_db << "\"command\":\"" << compiler.get_command().get_path() << ' ' << options.str()
              << "\",\n";
        cc_db << "\"file\":\"" << src->get_base() << "\"\n}";
    }
    cc_db << "\n]\n";
    cc_db.close();
    cout << "Done.\n";
}
// -----------------------------------------------------------------------------------------------------------------
void
builder::export_cmake(path& dir)
{
    list<path>::iterator src;
    ostringstream options;
    bool first = true;

    if (!sources) {
        cout << "builder::export - sources not defined!\n";
        return;
    }
    dir.set_base("CMakeLists.txt");
    ofstream cml(dir.get_pp());
    if (!cml) {
        cout << "builder::compile - unable to create CMakeLists.txt.\n";
        return;
    }
    cml << "project(" << name << ")\n";
    cml << "cmake_minimum_required(VERSION 3.19)\n\n";
    cml << "add_executable(" << name << '\n';
    for (src = sources->begin(); src != sources->end(); src++) {
        if (first)
            first = false;
        else
            cml << '\n';
        cml << src->get_path();
    }
    cml << ")\n\n";
    cml << "target_include_directories(fcgi PUBLIC /usr/local/include/cpp4scripts)\n";
    cml.close();
    cout << "Done.\n";
}

} // namespace c4s