/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#ifndef C4S_PROCESS_HPP
#define C4S_PROCESS_HPP

namespace c4s {

class compiled_file;
class program_arguments;
class variables;
class user;
// -------------------------------------------------------------------------------------------------
//! Process pipes wraps three pipes needed to communicate with child programs / binaries
class proc_pipes
{
  public:
    proc_pipes();
    ~proc_pipes();

    void reset();
    void init_child();
    void init_parent();
    bool read_child_stdout(std::ostream*);
    bool read_child_stderr(std::ostream*);
    size_t write_child_input(std::istream*);
    void close_child_input();
    size_t get_br_out() { return br_out; }
    size_t get_br_err() { return br_err; }

  protected:
    size_t br_out, br_err;
    bool send_ctrlZ;
    int fd_out[2];
    int fd_err[2];
    int fd_in[2];
    size_t br_in;
};

// -------------------------------------------------------------------------------------------------
//! Class encapsulates an executable process.
/*! Class manages single executable process and its parameters. Process can be executed multiple
  times and its arguments can be changed in between. Process output (both stderr and stdout) is by
  default echoed to parent's stdout. See pipe-functions for alternatives. Class destructor
  terminates the process if it is still running.
*/
class process
{
  public:
    //! Default constructor.
    process() { init_member_vars(); }
    //! Creates a new process object from command and its arguments.
    process(const char*, const char* args);
    //! Creates a new proces, sets arguments and pipe target.
    process(const char*, const char* args, std::ostream* out, std::istream* in=0);
    //! Creates a new process object from command and its arguments.
    process(const std::string&, const char* args, std::ostream* out = 0);
    //! Creates a new process object from command and its arguments.
    process(const std::string&, const std::string& args, std::ostream* out = 0);
    //! Initializes the command only.
    process(const std::string&);
    //! Initializes the command only.
    process(const char*);
    //! Deletes the object and possibly kills the process.
    ~process();

    //! Sets the process to be executed.
    void set_command(const char*);
    path get_command() { return command; }

    //! Operator= override for process.
    void operator=(const process& p);
    //! Operator= override for process.
    void operator=(const char* command) { set_command(command); }

    //! Sets the given string as single argument string for this process.
    void set_args(const char* arg)
    {
        arguments.str("");
        arguments << arg;
    }
    //! Sets the given string as single argument string for this process.
    void set_args(const std::string& arg)
    {
        arguments.str("");
        arguments << arg;
    }
    //! Adds given string into argument list as quoted string
    void add_quoted_args(const std::string& arg) { arguments << " '" << arg << '\''; }
    //! Adds the given string into argument list.
    void operator+=(const char* arg) { arguments << ' ' << arg; }
    //! Adds the given string into argument list.
    void operator+=(const std::string& arg) { arguments << ' ' << arg; }

    //! Forward content from given stream into process' stdin
    void pipe_from(std::istream* in) { stream_source = in; }
    //! Pipes child stdout to given stream (file, stringstream or cout)
    void pipe_to(std::ostream* out) { stream_out = out; }
    //! Pipes child stderr to given stream (file, stringstream or cout)
    void pipe_err(std::ostream* out) { stream_err = out; }
    //! Disables piping from child's stderr and stdout all together.
    void pipe_null() { stream_out = 0; stream_err = 0; }
    //! Closes the send pipe to client.
    void pipe_send_close()
    {
        if (pid && pipes)
            pipes->close_child_input();
    }

    //! Sets the effective owner for the process. (Linux only)
    void set_user(user*);
    //! Sets the daemon flag. Use only for attached processes.
    void set_daemon(bool enable) { daemon = enable; }
    //! Returns the pid for this process.
    int get_pid() { return pid; }
    //! Attaches this object to running process.
    void attach(int pid);
    //! Attaches this object to running process with pid in named pid-file
    void attach(const path& pid_file);

    //! Starts the executable process.
    /*! Process runs assynchronously. Command and possible arguments should have been given before
      this command. If process needs to be killed before it runs it's course, you may simply delete
      the process object. Normally one would wait untill it completes. If process is already
      running, calling this function will call stop first and then start new process.
    */
    void start(const char* args = 0);
    //! Stops the process i.e. terminates it if it is sill running and closes files.
    void stop();
    //! Waits for this process to end or untill the given timeout is expired.
    int wait_for_exit(int timeOut);

    //! Executes command after _appending_ given argument to the current arguments.
    int execa(const char* arg, int timeout = C4S_PROC_TIMEOUT);
    //! Executes the command with optional arguments. Previous arguments are written over. Calls
    //! start and waits for the exit.
    int exec(const char* args = 0, int timeout = C4S_PROC_TIMEOUT);
    //! Executes the command with optional arguments. Previous arguments are written over. Calls
    //! start and waits for the exit.
    int exec(const std::string&, int timeout = C4S_PROC_TIMEOUT);
    //...
    //! Runs the process appending given arguments and default timeout value (execa)
    int operator()(const char* args, int timeout = C4S_PROC_TIMEOUT)
    {
        return execa(args, timeout);
    }
    //! Runs the process appending given arguments and default timeout value (execa)
    int operator()(const std::string& args, int timeout = C4S_PROC_TIMEOUT)
    {
        return execa(args.c_str(), timeout);
    }
    //! Runs the process with optional timeout value (exec)
    int operator()(int timeout = C4S_PROC_TIMEOUT) { return exec(0, timeout); }

    //! Checks if the process is still running.
    bool is_running();
    //! Returns true if command exists in the system.
    bool is_valid() { return !command.empty(); }
    //! Returns return value from last execution.
    int last_return_value() { return last_ret_val; }
    //! Enables or disables command echoing before execution.
    void set_echo(bool e) { echo = e; }

    //! Static function that returns an output from given command.
    static void catch_output(const char* cmd, const char* args, std::string& output);
    //! Static function that appends parameters from one process to another.
    static void append_from(const char* cmd, const char* args, process& target);
    //! Static function to get current PID
    static pid_t get_running_pid() { return getpid(); }
    //! Dumps the process name and arguments into given stream. Use for debugging.
    void dump(std::ostream&);

    size_t get_bytes_out() { return bytes_out; }
    size_t get_bytes_err() { return bytes_err; }

    static bool no_run; // 1< If true then the command is simply echoed to stdout but not actually
                        // run. i.e. dry run.
    static bool nzrv_exception; //!< If true 'Non-Zero Return Value' causes exception.

  protected:
    //! Initializes process member variables. Called by constructors.
    void init_member_vars();
    void stop_daemon();

    path command;                //!< Full path to a command that should be executed.
    std::stringstream arguments; //!< Stream of process arguments. Must not contain variables.

    int interpret_process_status(int);
    user* owner; //!< If defined, process will be executed with user's credentials.
    pid_t pid;
    int last_ret_val;
    bool daemon; //!< If true then the process is to be run as daemon and should not be terminated
                 //!< at class dest
    std::ostream* stream_out; //!< Stream for process stdout
    std::ostream* stream_err; //!< Stream for process stderr
    std::istream* stream_source; //!< Pipe source i.e. process stdin
    path stdin_path;      //!< If defined and exists, files content will be used as input to process.
    proc_pipes* pipes; //!< Pipe to child for input and output. Valid when child is running.
    bool echo; //!< If true then the commands are echoed to stdout before starting them. Use for
               //!< debugging.    
    size_t bytes_out, bytes_err;
};

} // namespace c4s
#endif
