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

#include <sstream>
#include "RingBuffer.hpp"
#include "ntbs.hpp"

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
    bool read_child_stdout(RingBuffer*);
    bool read_child_stderr(RingBuffer*);
    size_t write_child_input(RingBuffer*);
    size_t write_child_input(ntbs*);
    void close_child_input();

  protected:
    bool send_ctrlZ;
    int fd_out[2];
    int fd_err[2];
    int fd_in[2];
    size_t br_in;
};

enum class PIPE { NONE, SM, LG };

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
    process(const char*, const char* args=nullptr, PIPE pipe=PIPE::NONE, user* owner=nullptr);
    //! Creates a new process object from command and its arguments.
    //-- process(const std::string&, const char* args, PIPE pipe=PIPE::NONE, user* owner=nullptr);
    //! Creates a new process object from command and its arguments.
    process(const std::string&, const std::string& args, PIPE pipe=PIPE::NONE, user* owner=nullptr);
    //! Create executable process from path
    process(const path&, const char* args = 0);

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

    //! Forward content from given buffer into process' stdin
    void write_stdin(RingBuffer *data) {
        if (pipes)
            pipes->write_child_input(data);
    }
    //! Forward content from given string into process' stdin
    void write_stdin(ntbs *data) {
        if (pipes)
            pipes->write_child_input(data);
    }
    //! Closes the send pipe to client.
    void close_stdin() {
        if (pid && pipes)
            pipes->close_child_input();
    }

    //! Sets the effective owner for the process. (Linux only)
    void set_user(user*);
    //! Sets the daemon flag. Use only for attached processes.
    void set_daemon(bool enable) { daemon = enable; }
    //! Set the timeout for this process overriding general timeout value.
    void set_timeout(unsigned int to) { timeout = to; }
    //! Enables or disables command echoing before execution.
    void set_echo(bool e) { echo = e; }
    //! Set explicit pipe buffer sizes for stdout and stderr.
    void set_pipe_size(size_t s_out, size_t s_err) {
        rb_out.max_size(s_out);
        rb_err.max_size(s_err);
    }
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
    void start(const char* args=nullptr);
    void start(const std::string &opts) {
        arguments.str("");
        arguments << opts;
        start(nullptr);
    }
    //! Stops the process i.e. terminates it if it is sill running and closes files.
    void stop();

    //! Executes command after _appending_ given argument to the current arguments.
    int execa(const char* arg);
    //! Runs the process overriding current arguments.
    int operator()(const char* args=nullptr);
    //! Runs the process overriding current arguments.
    int operator()(const std::string& args);
    //! Runs the process copying output into named stream.
    int operator()(std::ostream&);
    //! Runs process overriding arguments and sends output to given stream.
    int operator()(const std::string& args, std::ostream&);
    //! Runs process sending input and waiting for answer.
    int query(ntbs* question, ntbs* answer, int timeout=5);
    //! Static function that returns an output from given command.
    static int query(const char* cmd, const char* args, ntbs* input, ntbs* answer=0, int timeout=15);
    //! Checks if the process is still running.
    bool is_running();
    //! Waits for the process to exit.
    int wait_for_exit();

    //! Returns true if command exists in the system.
    bool is_valid() { return !command.empty(); }
    //! Returns return value from last execution.
    int last_return_value() { return last_ret_val; }
    //! Returns number of seconds process ran at the last execution.
    double duration() { return ((double) (proc_ended - proc_started)) / CLOCKS_PER_SEC; }

    //! Dumps the process name and arguments into given stream. Use for debugging.
    void dump(std::ostream&);

    RingBuffer rb_out;
    RingBuffer rb_err;

    static bool no_run; // 1< If true then the command is simply echoed to stdout but not actually run. i.e. dry run.
    static bool nzrv_exception; //!< If true 'Non-Zero Return Value' causes exception.
    static unsigned int general_timeout;

  protected:
    //! Initializes process member variables. Called by constructors.
    void init_member_vars();
    //! Stops a deamon i.e. process started with another process object earlier.
    void stop_daemon();

    path command;                //!< Full path to a command that should be executed.
    std::stringstream arguments; //!< Stream of process arguments. Must not contain variables.

    int interpret_process_status(int);
    int wait_with_stream(std::ostream* log=0);

    user* owner;                //!< If defined, process will be executed with user's credentials.
    pid_t pid;
    int last_ret_val;
    bool daemon;                //!< If true then the process is to be run as daemon and should not be terminated
                                //!< at class dest
    std::iostream* stream_in;   //!< Data for client stdin
    path stdin_path;            //!< If defined and exists, files content will be used as input to process.
    proc_pipes* pipes;          //!< Pipe to child for input and output. Valid when child is running.
    bool echo;                  //!< If true then the commands are echoed to stdout before starting them. Use for debugging.
    unsigned int timeout;       //!< Number of seconds to wait before stopping the process.
    clock_t proc_started;       //!< Clock time when process was started.
    clock_t proc_ended;         //!< Clock time when process ended.
};

} // namespace c4s
#endif
