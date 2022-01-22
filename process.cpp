/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#include <iostream>
#include <string.h>
#include <fcntl.h>
#include <grp.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config.hpp"
#include "exception.hpp"
#include "user.hpp"
#include "path.hpp"
#include "path_list.hpp"
#include "compiled_file.hpp"
#include "variables.hpp"
#include "program_arguments.hpp"
#include "process.hpp"
#include "util.hpp"

using namespace std;
using namespace c4s;

namespace c4s {

// -------------------------------------------------------------------------------------------------
/*!
  Initializes process pipes by creating three internal pipes.
*/
proc_pipes::proc_pipes()
{
    int fd_tmp[2];
    memset(fd_out, 0, sizeof(fd_out));
    memset(fd_err, 0, sizeof(fd_err));
    memset(fd_in, 0, sizeof(fd_in));

    if (pipe(fd_out))
        throw process_exception(
            "proc_pipes::proc_pipes - Unable to create pipe for the process std output.");
    if (fd_out[0] < 3 || fd_out[1] < 3) {
        if (pipe(fd_tmp))
            throw process_exception(
                "proc_pipes::proc_pipes - Unable to create pipe for the process std output (2).");
        fd_out[0] = fd_tmp[0];
        fd_out[1] = fd_tmp[1];
    }
    if (pipe(fd_err))
        throw process_exception(
            "proc_pipes::proc_pipes - Unable to create pipe for the process std error.");
    if (pipe(fd_in))
        throw process_exception(
            "proc_pipes::proc_pipes - Unable to create pipe for the process std input.");

    // Make out and err read pipes nonblocking
    int fflag = fcntl(fd_out[0], F_GETFL, 0);
    fcntl(fd_out[0], F_SETFL, fflag | O_NONBLOCK);
    fflag = fcntl(fd_err[0], F_GETFL, 0);
    fcntl(fd_err[0], F_SETFL, fflag | O_NONBLOCK);
    br_out = 0;
    br_err = 0;
    br_in = 0;
    send_ctrlZ = false;
}
// -------------------------------------------------------------------------------------------------
void
proc_pipes::close_child_input()
{
    if (fd_in[0]) {
        close(fd_in[0]);
        fd_in[0] = 0;
    }
    if (fd_in[1]) {
        close(fd_in[1]);
        fd_in[1] = 0;
    }
}
// -------------------------------------------------------------------------------------------------
/*!
  Closes the pipes created by the constructor.
*/
proc_pipes::~proc_pipes()
{
    if (fd_out[0])
        close(fd_out[0]);
    if (fd_out[1])
        close(fd_out[1]);
    if (fd_err[0])
        close(fd_err[0]);
    if (fd_err[1])
        close(fd_err[1]);
    if (fd_in[0])
        close(fd_in[0]);
    if (fd_in[1])
        close(fd_in[1]);
}
// -------------------------------------------------------------------------------------------------
/*!
  Initilizes child side pipes. Call this from the child right after the fork and before exec.
  After this function you should destroy this object.
  \retval bool True on succes, false on error.
*/
void
proc_pipes::init_child()
{
    close(fd_in[1]);
    close(fd_out[0]);
    close(fd_err[0]);
    dup2(fd_in[0], STDIN_FILENO);   // = 0
    dup2(fd_out[1], STDOUT_FILENO); // = 1
    dup2(fd_err[1], STDERR_FILENO); // = 2
    close(fd_in[0]);
    close(fd_out[1]);
    close(fd_err[1]);
    memset(fd_out, 0, sizeof(fd_out));
    memset(fd_err, 0, sizeof(fd_err));
    memset(fd_in, 0, sizeof(fd_in));
}
// -------------------------------------------------------------------------------------------------
/*!
  Initializes the parent side pipes. Call this from parent right after the fork.
  NOTE! This does nothing in Windows since the functionality is not needed
*/
void
proc_pipes::init_parent()
{
    // Close the input read side
    close(fd_in[0]);
    fd_in[0] = 0;
    // Close the output write sides
    close(fd_out[1]);
    close(fd_err[1]);
    fd_out[1] = 0;
    fd_err[1] = 0;
}
// -------------------------------------------------------------------------------------------------
/*!
  Read stdout from the child process and print it out on the given stream.
  \param pout Stream for the output
*/
bool
proc_pipes::read_child_stdout(ostream* pout)
{
    static char out_buffer[MAX_PIPE_BUFFER];
    ssize_t rsize = read(fd_out[0], out_buffer, MAX_PIPE_BUFFER);
    if (rsize > 0 && pout) {
        pout->write(out_buffer, rsize);
        pout->flush();
        br_out += rsize;
#ifdef C4S_DEBUGTRACE
        cerr << "proc_pipes::read_child_stdout - Wrote " << cnt << " bytes\n";
#endif
    }
    return rsize > 0 ? true : false;
}
// -------------------------------------------------------------------------------------------------
/*!
  Read stderr from the child process and print it out on the given stream.
  \param pout Stream for the output
*/
bool
proc_pipes::read_child_stderr(ostream* perr)
{
    static char out_buffer[MAX_PIPE_BUFFER];
    ssize_t rsize = read(fd_err[0], out_buffer, MAX_PIPE_BUFFER);
    if (rsize > 0 && perr) {
        perr->write(out_buffer, rsize);
        perr->flush();
        br_err += rsize;
#ifdef C4S_DEBUGTRACE
        cerr << "proc_pipes::read_child_stderr - Wrote " << cnt << " bytes\n";
#endif
    }
    return rsize > 0 ? true : false;
}
// -------------------------------------------------------------------------------------------------
/*!
  Writes the given string into the child input.
  \param input String to write.
*/
size_t
proc_pipes::write_child_input(std::istream* input)
{
    char buffer[1024];
    size_t cnt = 0, ss;
    if (!input)
        return 0;
    do {
        input->read(buffer, sizeof(buffer));
        ss = input->gcount();
        if (ss > 0) {
            write(fd_in[1], buffer, ss);
            cnt += ss;
        }
    } while (ss > 0 && input->good());
#ifdef C4S_DEBUGTRACE
    cerr << "proc_pipes::write_child_input - Wrote " << cnt << " bytes to child stdin\n";
#endif
    close_child_input();
    return cnt;
}

// -------------------------------------------------------------------------------------------------
// ###############################  PROCESS ########################################################
// -------------------------------------------------------------------------------------------------
bool c4s::process::no_run = false;
bool c4s::process::nzrv_exception = false; //!< If true 'Non-Zero Return Value' causes exception.

// -------------------------------------------------------------------------------------------------
/*! Common initialize code for all constructors. Constructors call this function first.
 */
void
process::init_member_vars()
{
    pid = 0;
    last_ret_val = 0;
    stream_out = 0;
    stream_err = 0;
    stream_source = 0;
    pipes = 0;
    echo = false;
    owner = 0;
    daemon = false;
    bytes_out = 0;
    bytes_err = 0;
}

// -------------------------------------------------------------------------------------------------
/*! \param cmd Command to execute.
  \param args Arguments to pass to the executable.
*/
process::process(const char* cmd, const char* args)
{
    init_member_vars();
    set_command(cmd);
    if (args)
        set_args(args);
}

// -------------------------------------------------------------------------------------------------
/*! \param cmd Command to execute.
  \param args Arguments to pass to the executable.
  \param out Process output is sent to this stream.
  \param in Data from this stream is written to process stdin.
 */
process::process(const char* cmd, const char* args, ostream* out, istream* in)
{
    init_member_vars();
    set_command(cmd);
    if (args)
        arguments << args;
    stream_out = out;
    stream_source = in;
}

// -------------------------------------------------------------------------------------------------
/*! \param cmd Command to execute.
  \param args Arguments to pass to the executable.
*/
process::process(const string& cmd, const char* args, ostream* out)
{
    init_member_vars();
    set_command(cmd.c_str());
    if (args)
        arguments << args;
    if (out)
        stream_out = out;
}

// -------------------------------------------------------------------------------------------------
/*! \param cmd Command to execute.
  \param args Arguments to pass to the executable.
*/
process::process(const string& cmd, const string& args, ostream* out)
{
    init_member_vars();
    set_command(cmd.c_str());
    if (!args.empty())
        arguments << args;
    if (out)
        stream_out = out;
}

// -------------------------------------------------------------------------------------------------
/*! \param cmd Name of the command to execute. No arguments should be specified with command.
 */
process::process(const string& cmd)
{
    init_member_vars();
    set_command(cmd.c_str());
}
// -------------------------------------------------------------------------------------------------
/*! \param cmd Name of the command to execute. No arguments should be specified with command.
 */
process::process(const char* cmd)
{
    init_member_vars();
    set_command(cmd);
}
// -------------------------------------------------------------------------------------------------
/*! If the the daemon mode has NOT been set then the destructor kills the process if it is still
 * running.
 */
process::~process()
{
#ifdef C4S_DEBUGTRACE
    cerr << "process::~process - name=" << command.get_base() << '\n';
#endif
    if (pid && !daemon)
        stop();

    if (pipes) {
        delete pipes;
        pipes = 0;
    }
}
// -------------------------------------------------------------------------------------------------
/*!  Finds the command from current directory or path. If not found throws process
  exception. In windows .exe-is appended automatically if not specified in command.
  \param cmd Name of the command.
*/
void
process::set_command(const char* cmd)
{
    if (!cmd)
        throw process_exception("process::set_command - empty command");
    command = cmd;
    struct stat sbuf;
    memset(&sbuf, 0, sizeof(sbuf));
    // Check the user provided path first. This includes current directory.
    if (stat(command.get_path().c_str(), &sbuf) == -1 ||
        !has_anybits(sbuf.st_mode, S_IXUSR | S_IXGRP | S_IXOTH)) {
        // cerr << "DEBUG - set_command:"<<command.get_path()<<";
        // st_mode="<<hex<<sbuf.st_mode<<dec<<'\n';
        if (!command.exists_in_env_path("PATH", true)) {
            ostringstream ss;
            command.clear();
            ss << "process::set_command - Command not found: " << cmd;
            throw process_exception(ss.str());
        }
    }
}
// -------------------------------------------------------------------------------------------------
/*!  Copies the command name, arguments and stream_out. Other
  attributes are simply cleared. Function should not be called if this process is running.
  \param source Source process to copy.
*/
void
process::operator=(const process& source)
{
    command = source.command;
    arguments.str("");
    arguments << source.arguments.str();
    pid = 0;
    stream_out = source.stream_out;
    daemon = source.daemon;
}
// ### \TODO continue to improve documentation from here on down.
// -------------------------------------------------------------------------------------------------
void
process::dump(ostream& os)
{
    const char* pe = echo ? "true" : "false";
    const char* pt = stream_out ? "OK" : "None";
    os << "Process - " << command.get_path() << "(";
    os << arguments.str();
    os << ");\n   PID=" << pid << "; echo=" << pe << "; LRV=" << last_ret_val << "; stdout=" << pt
       << '\n';
}
// -------------------------------------------------------------------------------------------------
void
process::start(const char* args)
{
    streamsize max;
    char tmp_argbuf[512];
    char tmp_cmdbuf[128];
    char *arg_buffer, *dynamic_buffer = 0;
    const char* arg_ptr[MAX_PROCESS_ARGS];
    int ptr_count;

    if (command.empty())
        throw process_exception("process::start - Unable to start process. No command specified.");

    if (pid)
        stop();
    if (args) {
        arguments.str("");
        arguments << args;
    }
    last_ret_val = 0;
    if (no_run)
        return;

    memset(arg_ptr, 0, sizeof(arg_ptr));
    // convention requires the first argument to be the path to command itself
    strcpy(tmp_cmdbuf, command.get_path().c_str());
    arg_ptr[0] = tmp_cmdbuf;
    ptr_count = 1;
    // These need to be in *argv[]. Copy from argumetn streamd to static buffer and
    // change spaces to zeros and at the same time make pointers to args.
    max = arguments.tellp();
    if (max > 0) {
        size_t arg_buffer_length = max;
        if (arg_buffer_length > sizeof(tmp_argbuf)) {
            dynamic_buffer = new char[arg_buffer_length + 1];
            arg_buffer = dynamic_buffer;
            arg_buffer_length++;
        } else {
            arg_buffer = tmp_argbuf;
            arg_buffer_length = sizeof(tmp_argbuf);
        }
        memset(arg_buffer, 0, arg_buffer_length);
        arg_ptr[ptr_count++] = arg_buffer;

        int ch, prev = ' ', quote = 0;
        stringbuf* argsb = arguments.rdbuf();
        argsb->pubseekpos(0, ios_base::in);
        for (ch = argsb->sgetc(); ch != EOF && max > 0; ch = argsb->snextc()) {
            if (quote) {
                if (quote == ch) {
                    if (prev != '\\')
                        quote = 0;
                    else
                        arg_buffer--;
                } else
                    *arg_buffer++ = (char)(ch & 0xff);
            } else if (ch == '\'' || ch == '\"') {
                if (prev == '\\')
                    *(arg_buffer - 1) = (char)(ch & 0xff);
                else
                    quote = ch;
            } else if (ch == ' ') {
                if (prev != ' ') {
                    *arg_buffer++ = 0;
                    arg_ptr[ptr_count++] = arg_buffer;
                    if (ptr_count >= MAX_PROCESS_ARGS - 1)
                        throw process_exception(
                            "process::start - Too many arguments. Use response file.");
                }
            } else
                *arg_buffer++ = (char)(ch & 0xff);
            prev = ch;
            max--;
        }
        *arg_buffer = 0;
        if (quote)
            throw process_exception("process::start - Unmatched quote marks in arguments.");
    } // if arguments.tellp()
    else {
        arg_buffer = 0;
    }

    // Finalize argument pointer array
    arg_ptr[ptr_count] = 0;
    if (arg_ptr[ptr_count - 1][0] == 0)
        arg_ptr[ptr_count - 1] = 0;

#ifdef C4S_DEBUGTRACE
    cerr << "process::start - " << command.get_path() << '(' << tmp_cmdbuf << "):\n";
    for (int i = 0; arg_ptr[i]; i++)
        cerr << " [" << i << "] " << arg_ptr[i] << '\n';
    cerr << "process::start - About to fork, pipe=";
    if (stream_out)
        cerr << "enabled";
    else
        cerr << "disabled";
#else
    if (echo) {
        cerr << command.get_base() << '(';
        for (int i = 0; arg_ptr[i]; i++) {
            if (i > 0)
                cerr << ' ';
            cerr << '\'' << arg_ptr[i] << '\'';
        }
        cerr << ")\n";
    }
#endif

    if (pipes)
        delete pipes;
    pipes = new proc_pipes();

    // Create the child process i.e. fork
    pid = fork();
    if (!pid) {
#ifdef C4S_DEBUGTRACE
        cerr << "process::start - created child: " << getpid() << endl;
#endif
        pipes->init_child();
        delete pipes;
        if (owner) {
            if (initgroups(owner->get_name().c_str(), owner->get_gid()) != 0 ||
                setuid(owner->get_uid()) != 0) {
                int er = errno;
                cerr << "process::start - child-process: Unable to change process persona. User:"
                     << owner->get_name() << ".\nError (" << er << ") ";
                cerr << strerror(er) << '\n';
                _exit(EXIT_FAILURE);
            }
        }
        if (execv(tmp_cmdbuf, (char**)arg_ptr) == -1) {
            cerr << "process::start - child-process: Unable to start process:" << tmp_cmdbuf
                 << "\nError (" << errno << ") " << strerror(errno) << '\n';
        }
        _exit(EXIT_FAILURE);
    }
    pipes->init_parent();
    if (dynamic_buffer)
        delete[] dynamic_buffer;
    // If child input file has been defined, feed it to child.
    if (stream_source) {
        pipes->write_child_input(stream_source);
    }
}
// -------------------------------------------------------------------------------------------------
void
process::set_user(user* _owner)
{
    if (_owner && _owner->status() == 0)
        owner = _owner;
    else {
#ifdef C4S_DEBUGTRACE
        cerr << "process::set_user() - unable to set user.\n";
#endif
        owner = 0;
    }
}
// -------------------------------------------------------------------------------------------------
/*! Attaching allows developer to stop running processes by first attaching object to a process
  and then calling stop-function. Exception is thrown if pid is not found. If process alredy is
  running this function does nothing. Daemon mode is set on. \param _pid Process id to attach to.
 */
void
process::attach(int _pid)
{
    if (pid)
        return;
    pid = _pid;
    last_ret_val = 0;
    daemon = true;
    if (!is_running()) {
        ostringstream os;
        os << "process::attach - Cannot attach. Process with PID (" << pid << ") not found.";
        throw process_exception(os.str());
    }
}
// -------------------------------------------------------------------------------------------------
void
process::attach(const path& pid_file)
{
    long attach_pid = 0;
    ostringstream os;
    if (!pid_file.exists()) {
        os << "process::attach - pid file " << pid_file.get_path() << " not found";
        throw process_exception(os.str());
    }
    ifstream pf(pid_file.get_path().c_str());
    if (!pf) {
        os << "process::attach - Unable to open pid file " << pid_file.get_path();
        throw process_exception(os.str());
    }
    pf >> attach_pid;
    if (attach_pid)
        attach((int)attach_pid);
    else {
        os << "process::attach - Unable to read pid from file " << pid_file.get_path();
        throw process_exception(os.str());
    }
}
// -------------------------------------------------------------------------------------------------
/*! If the timeout expires the process_exception is trown.
  \param timeout Number of seconds to wait.
  \retval int Return value from the process.
*/
int
process::wait_for_exit(int timeout)
{
    if (!pid)
        return last_ret_val;
    if (no_run) {
        last_ret_val = 0;
        return 0;
    }
#ifdef C4S_DEBUGTRACE
    cerr << "process::wait_for_exit - name=" << command.get_base() << ", pid=" << pid
         << ", timeout=" << timeout;
    if (stream_out)
        cerr << ", stream_out";
    if (stream_err)
        cerr << ', stream_err';
    cerr << '\n';
#endif
    time_t beg = time(0);
    int status;
    int counter = timeout * 10;
    struct timespec ts_delay, ts_remain;
    ts_delay.tv_sec = 0;
    ts_delay.tv_nsec = 100000000L;
    pid_t wait_val;
    do {
        nanosleep(&ts_delay, &ts_remain);
        if (stream_err)
            pipes->read_child_stderr(stream_err);
        if (stream_out)
            pipes->read_child_stdout(stream_out);
        wait_val = waitpid(pid, &status, WNOHANG);
        if (wait_val && wait_val != pid) {
            ostringstream os;
            os << "process::wait_for_exit - name=" << command.get_base()
               << ", wait error: " << strerror(errno);
            throw process_exception(os.str());
        }
        counter--;
    } while (!wait_val && counter > 0);
    if (counter) {
        bool out_data = stream_out ? true : false;
        bool err_data = stream_err ? true : false;
        while (time(0) - beg < timeout && (out_data || err_data)) {
            if (err_data)
                err_data = pipes->read_child_stderr(stream_err);
            if (out_data)
                out_data = pipes->read_child_stdout(stream_out);
        }
    }
    last_ret_val = interpret_process_status(status);
#ifdef C4S_DEBUGTRACE
    cerr << "process::wait_for_exit - name=" << command.get_base() << ", status:" << status
         << ", last_ret_val:" << last_ret_val;
    cerr << ", counter:" << counter << ", seconds:" << time(0) - beg;
    cerr << ", br_out:" << pipes->get_br_out() << ", br_err:" << pipes->get_br_err() << '\n';
#else
    bytes_err = pipes->get_br_err();
    bytes_out = pipes->get_br_out();
#endif
    if (counter <= 0) {
        ostringstream os;
        os << "process::wait_for_exit - name=" << command.get_base() << ", pid=" << pid
           << "; Timeout!";
        throw process_timeout(os.str());
    }
    pid = 0;
    stop();
    if (nzrv_exception && last_ret_val != 0) {
        ostringstream os;
        os << "Process: '" << command.get_base() << ' ' << arguments.str()
           << "' retured:" << last_ret_val;
        throw process_exception(os.str());
    }
    return last_ret_val;
}
// -------------------------------------------------------------------------------------------------
/*!
  Executes process with additional argument. Given argument is not stored permanently.
  Returns when the process is completed or timeout exeeded.
  \param args Additional argument to append to current set.
  \param timeout Number of seconds to wait. Defaults to C4S_PROC_TIMEOUT.
  \retval int Return value from the command.
*/
int
process::execa(const char* plus, int timeout)
{
    streampos end = arguments.tellp();
    arguments << ' ' << plus;
    start();
    int rv = wait_for_exit(timeout);
    arguments.seekp(end);
    return rv;
}

// -------------------------------------------------------------------------------------------------
/*!
  Returns when the process is completed or
  timeout exeeded. This is a shorthand for start-wait_for_exit combination.
  \param timeout Number of seconds to wait for the process completion.
  \param args Optional arguments for the command. Overrides previously entered arguments if
  specified. \retval int Return value from the command.
*/
int
process::exec(const char* args, int timeout)
{
    start(args);
    return wait_for_exit(timeout);
}

// -------------------------------------------------------------------------------------------------
/*!
  Executes the command with optional arguments. Returns when the process is completed or
  timeout exeeded. This is a shorthand for start-wait_for_exit combination.
  \param timeout Number of seconds to wait for the process completion.
  \param args Arguments for the command. Overrides existing arguments.
  \retval int Return value from the command.
*/
int
process::exec(const string& args, int timeout)
{
    start(args.c_str());
    return wait_for_exit(timeout);
}

// -------------------------------------------------------------------------------------------------
/*!
   \retval bool True if it is, false if not.
*/
bool
process::is_running()
{
    if (!pid)
        return false;
    if (kill(pid, 0) == 0)
        return true;
    pid = 0;
    stop();
    return false;
}

// -------------------------------------------------------------------------------------------------
void
process::stop_daemon()
{
    ostringstream os;
#ifdef C4S_DEBUGTRACE
    cerr << "process::stop_daemon - name=" << command.get_base() << '\n';
#endif
    if (!pid)
        return;
    // Send termination signal.
    if (kill(pid, SIGTERM)) {
#ifdef C4S_DEBUGTRACE
        cerr << "process::stop: kill(pid,SIGTERM) error:" << strerror(errno) << '\n';
#else
        os << "process::stop: kill(pid,SIGTERM) error:" << strerror(errno) << '\n';
#endif
        throw process_exception(os.str());
    }
    // Lets wait for a while:
    struct timespec ts_delay, ts_remain;
    ts_delay.tv_sec = 0;
    ts_delay.tv_nsec = 400000000L;
    int rv, count = 20;
    do {
        nanosleep(&ts_delay, &ts_remain);
        rv = kill(pid, 0);
        count--;
    } while (count > 0 && rv == 0);
#ifdef C4S_DEBUGTRACE
    cerr << "process::stop_daemon - term result:" << rv << "; count=" << count
         << "; errno=" << errno << '\n';
#endif
    if (count == 0) {
        count = 10;
        kill(pid, SIGKILL);
        do {
            nanosleep(&ts_delay, &ts_remain);
            rv = kill(pid, 0);
            count--;
        } while (count > 0 && rv == 0);
#ifdef C4S_DEBUGTRACE
        cerr << "process::stop_daemon - kill result:" << rv << "; count=" << count
             << "; errno=" << errno << '\n';
#endif
        if (count == 0)
            throw process_exception("process::stop_daemon - Failed, daemon sill running.");
    }
    if (errno != ESRCH) {
#ifdef C4S_DEBUGTRACE
        cerr << "process::stop_daemon - error stopping daemon\n";
#endif
        throw process_exception("process::stop_daemon - error stopping daemon");
    }
    pid = 0;
    last_ret_val = 0;
}

// -------------------------------------------------------------------------------------------------
// Private - Linux/Apple only
int
process::interpret_process_status(int status)
{
    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    else if (WTERMSIG(status) || WIFSTOPPED(status))
        return -1;
    return 0;
}
// ---------------------------------------------------------------------------------------------------------
/*!
  It is not recommended that this function is used to stop the process since the processes usually
  stop on their own. Use the exec- or wait_for_exit-functions instead. If the wait_for_exit gives
  timeout exception then this function can be called to stop the run-loose process.
*/
void
process::stop()
{
#ifdef C4S_DEBUGTRACE
    cerr << "process::stop - name=" << command.get_base() << '\n';
#endif
    if (pid) {
        if (daemon) {
            stop_daemon();
            return;
        }
        int status;
        ostringstream os;
    AGAIN:
        pid_t cid = waitpid(pid, &status, WNOHANG | WUNTRACED);
        if (cid == 0) {
            if (kill(pid, SIGTERM)) {
                os << "Unable to send termination signal to running process:" << pid
                   << ". (errno=" << errno << ")";
                throw process_exception(os.str());
            }
            cid = waitpid(pid, &status, WNOHANG | WUNTRACED);
            if (cid == 0) {
                if (kill(pid, SIGKILL)) {
                    os << "Unable to kill process " << pid << ". (errno=" << errno << ")";
                    throw process_exception(os.str());
                }
            }
#ifdef C4S_DEBUGTRACE
            cerr << "Process::stop - used TERM/KILL to stop " << pid << ".\n";
#endif
        }
        if (cid == -1) {
#ifdef C4S_DEBUGTRACE
            cerr << "process::stop - name=" << command.get_base()
                 << ", waitpid failed. Errno=" << errno << '\n';
#endif
            if (errno == EINTR)
                goto AGAIN;
            else if (errno == ECHILD) {
                // Process was probably attached and hence this is not a parent for the process.
                os << "process::stop: waitpid error:" << strerror(errno) << '\n';
                throw process_exception(os.str());
            } else {
                throw process_exception("Process::stop: Invalid argument to wait-function.");
            }
        } else {
            last_ret_val = interpret_process_status(status);
        }
        pid = 0;
    } // if(pid)

    if (pipes) {
        delete pipes;
        pipes = 0;
    }
}

// -------------------------------------------------------------------------------------------------
/*!  Possible errors with command will throw an exception.

  \param cmd Command to run.
  \param args Command arguments.
  \param output Buffer where the output will be stored into.
*/
void
process::catch_output(const char* cmd, const char* args, string& output)
{
    stringstream os;
    process source(cmd, args);
    source.pipe_to(&os);
    int rv = source();
    if (rv) {
        ostringstream err;
        err << "process::catch-output - command returned error " << rv;
        err << ". Output: " << os.str();
        throw process_exception(err.str());
    }
    output = os.str();
}

// -------------------------------------------------------------------------------------------------
/*! Possible errors with command will throw an exception. \see catch_output

  \param cmd Command to run.
  \param args Command arguments.
  \param target Process where the output will be appended to.
*/
void
process::append_from(const char* cmd, const char* args, process& target)
{
    stringstream os;
    process source(cmd, args);
    source.pipe_to(&os);
    int rv = source();
    if (rv) {
        ostringstream err;
        err << "process::append_from - command returned error " << rv;
        err << ". Output: " << os.str();
        throw process_exception(err.str());
    }
    target += os.str();
}

} // namespace c4s