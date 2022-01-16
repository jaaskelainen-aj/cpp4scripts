/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#ifndef C4S_LOGGER_HPP
#define C4S_LOGGER_HPP

namespace c4s {

class path;

//! Log levels in increasing order of priority
//  NOTE! Please see the name strings from the beginning of cpp file as well.
enum LOG_LEVEL
{
    LL_NONE,
    LL_TRACE,
    LL_DEBUG,
    LL_INFO,
    LL_NOTICE,
    LL_WARNING,
    LL_ERROR,
    LL_CRITICAL,
    LL_MAX
};

// -----------------------------------------------------------------------------------------------------------
//! Pure virtual base class for log sinks (appender in Log4j teminlogy)
/*! Sink provides uniform interface to various log outputs. Please see the inherited sink classes
   for actual implementations.*/
class log_sink
{
  public:
    //! Sink constructor (empty)
    log_sink() {}
    //! Sink destructor (empty)
    virtual ~log_sink() {}
    //! Prints single line of info into log output.
    /*! Requested level in print comamnd must be equal or higher than current logbase level in order
        for this message to be printed into the log output.
        \param ll Log level of this message.
        \param msg Message to be printed into output.*/
    virtual void print(LOG_LEVEL ll, const char* msg) = 0;

  protected:
    const char* get_datetime(LOG_LEVEL);
};

// ==========================================================================================
#if defined(__linux) || defined(__APPLE__)
//! Syslog log message target. Currently only for Linux
class syslog_sink : public log_sink
{
  public:
    //! Initializes the syslog to name and facility.
    syslog_sink(const char* name, int facility);
    //! Stops sendking messages to syslog.
    ~syslog_sink();
    void print(LOG_LEVEL, const char*);

  protected:
    int levelmap[8];
};
#endif

// -----------------------------------------------------------------------------------------------------------
//! File stream (buffered) based log message target.
class fstream_sink : public log_sink
{
  public:
    //! Initialize traditional file logging. Log level defined with constant.
    fstream_sink(const path& ph);
    //! Destructor closes the file stream.
    ~fstream_sink();
    void print(LOG_LEVEL, const char*);

  protected:
    std::ofstream log_file;
};

// -----------------------------------------------------------------------------------------------------------
//! File based (unbuffered) log target.
class lowio_sink : public log_sink
{
  public:
    //! Initializes and opens the low level io log target.
    lowio_sink(const path& ph);
    //! Destructor closes the open file.
    ~lowio_sink();
    void print(LOG_LEVEL, const char*);
#if defined(__linux) || defined(__APPLE__)
    static int mode;
#endif
  protected:
    int fid;
};
// -----------------------------------------------------------------------------------------------------------
//! Log target using stderr stream.
class stderr_sink : public log_sink
{
  public:
    //! Constructor for stderr log stream (empty)
    stderr_sink() {}
    //! Destructor for stderr log stream (empty)
    ~stderr_sink() {}
    void print(LOG_LEVEL, const char*);
};

// ==========================================================================================
//! Base and main class for logging. Requires a sink to operate.
/*! Logbase is a very thin layer between application and the log sink. It basically provides
  standard interface to log regardless of the log sink.
  NOTE! You must use init_log, close_log functions with the CS_PRINT... and CS_VAPRT... macros.
  Regular constructor is only if you intend to use the function interface directly and possibly
  have more than one log open simultaneously.
*/
class logbase
{
  public:
    //! Creates log framework, sets logging level and output.
    /*! \param ll Log level for this engine.
        \param sk Log output sink for the engine.*/
    logbase(LOG_LEVEL ll, log_sink* sk)
      : sink(sk)
      , level(ll)
    {}
    //! Destroys the log engine by deleting the associated sink.
    ~logbase()
    {
        if (sink)
            delete sink;
    }

    //! Log initialization function to be used if log macros are used.
    /*! Call this function once at the beginning of your program. Before any macros are called.*/
    static void init_log(LOG_LEVEL ll, log_sink* sk)
    {
        if (!thelog)
            thelog = new logbase(ll, sk);
    }
    static void close_log()
    {
        if (thelog) {
            delete thelog;
            thelog = 0;
        }
    }
    static logbase* get()
    {
        if (!thelog)
            thelog = new logbase(LL_ERROR, new stderr_sink());
        return thelog;
    }
    static bool is_open() { return thelog ? true : false; }

    void print(LOG_LEVEL ll, const char* str)
    {
        if (ll != LL_NONE && ll >= level)
            sink->print(ll, str);
    }
    void vaprt(LOG_LEVEL ll, const char*, ...);

    //! Changes the current logging level.
    void set_level(LOG_LEVEL ll) { level = ll; }
    //! Utility function to convert text levels into numerical levels
    static LOG_LEVEL str2level(const std::string& name);

  protected:
    static logbase* thelog;
    log_sink* sink;
    LOG_LEVEL level;
};

// ==========================================================================================
//! Log method that appears as STL stream and buffers log data to memory.
class logger : public logbase
{
  public:
    //! Default constructor.
    logger(LOG_LEVEL ll, log_sink* sk)
      : logbase(ll, sk)
    {}
    //! Closes the log
    ~logger() {}

    static void init_log(LOG_LEVEL ll, log_sink* sk)
    {
        if (!thelog)
            thelog = new logger(ll, sk);
    }
    static void close_log()
    {
        if (thelog) {
            delete thelog;
            thelog = 0;
        }
    }
    static logger* get() { return (logger*)thelog; }

    logger& operator<<(char val)
    {
        if (level != LL_NONE)
            buffer << val;
        return *this;
    }
    logger& operator<<(int val)
    {
        if (level != LL_NONE)
            buffer << val;
        return *this;
    }
    logger& operator<<(long val)
    {
        if (level != LL_NONE)
            buffer << val;
        return *this;
    }
    logger& operator<<(double val)
    {
        if (level != LL_NONE)
            buffer << val;
        return *this;
    }
    logger& operator<<(unsigned int val)
    {
        if (level != LL_NONE)
            buffer << val;
        return *this;
    }
    logger& operator<<(unsigned long val)
    {
        if (level != LL_NONE)
            buffer << val;
        return *this;
    }
#ifdef _WIN64
    logger& operator<<(size_t val)
    {
        if (level != LL_NONE)
            buffer << val;
        return *this;
    }
#endif
    logger& operator<<(const char* val)
    {
        if (level != LL_NONE)
            buffer << val;
        return *this;
    }
    logger& operator<<(const std::string& val)
    {
        if (level != LL_NONE)
            buffer << val;
        return *this;
    }
    logger& operator<<(LOG_LEVEL LL);

  protected:
    std::ostringstream buffer;
};
}

#define CSLOG *(c4s::logger::get())
#define CS_TRCE c4s::LL_TRACE
#define CS_DEBU c4s::LL_DEBUG
#define CS_INFO c4s::LL_INFO
#define CS_NOTE c4s::LL_NOTICE
#define CS_WARN c4s::LL_WARNING
#define CS_ERRO c4s::LL_ERROR
#define CS_CRIT c4s::LL_CRITICAL

#define _C(x) x.c_str()

#if C4S_LOG_LEVEL <= 1
#define CS_PRINT_TRCE(x) c4s::logbase::get()->print(CS_TRCE, x)
#else
#define CS_PRINT_TRCE(x)                                                                           \
    do {                                                                                           \
    } while (0)
#endif
#if C4S_LOG_LEVEL <= 2
#define CS_PRINT_DEBU(x) c4s::logbase::get()->print(CS_DEBU, x)
#else
#define CS_PRINT_DEBU(x)                                                                           \
    do {                                                                                           \
    } while (0)
#endif
#if C4S_LOG_LEVEL <= 3
#define CS_PRINT_INFO(x) c4s::logbase::get()->print(CS_INFO, x)
#else
#define CS_PRINT_INFO(x)                                                                           \
    do {                                                                                           \
    } while (0)
#endif
#if C4S_LOG_LEVEL <= 4
#define CS_PRINT_NOTE(x) c4s::logbase::get()->print(CS_NOTE, x)
#else
#define CS_PRINT_NOTE(x)                                                                           \
    do {                                                                                           \
    } while (0)
#endif
#if C4S_LOG_LEVEL <= 5
#define CS_PRINT_WARN(x) c4s::logbase::get()->print(CS_WARN, x)
#else
#define CS_PRINT_WARN(x)                                                                           \
    do {                                                                                           \
    } while (0)
#endif
#if C4S_LOG_LEVEL <= 6
#define CS_PRINT_ERRO(x) c4s::logbase::get()->print(CS_ERRO, x)
#else
#define CS_PRINT_ERRO(x)                                                                           \
    do {                                                                                           \
    } while (0)
#endif
#if C4S_LOG_LEVEL <= 7
#define CS_PRINT_CRIT(x) c4s::logbase::get()->print(CS_CRIT, x)
#else
#define CS_PRINT_CRIT(x)                                                                           \
    do {                                                                                           \
    } while (0)
#endif

#if C4S_LOG_LEVEL <= 1
#define CS_VAPRT_TRCE(x, ...) c4s::logbase::get()->vaprt(CS_TRCE, x, __VA_ARGS__)
#else
#define CS_VAPRT_TRCE(x, ...)                                                                      \
    do {                                                                                           \
    } while (0)
#endif
#if C4S_LOG_LEVEL <= 2
#define CS_VAPRT_DEBU(x, ...) c4s::logbase::get()->vaprt(CS_DEBU, x, __VA_ARGS__)
#else
#define CS_VAPRT_DEBU(x, ...)                                                                      \
    do {                                                                                           \
    } while (0)
#endif
#if C4S_LOG_LEVEL <= 3
#define CS_VAPRT_INFO(x, ...) c4s::logbase::get()->vaprt(CS_INFO, x, __VA_ARGS__)
#else
#define CS_VAPRT_INFO(x, ...)                                                                      \
    do {                                                                                           \
    } while (0)
#endif
#if C4S_LOG_LEVEL <= 4
#define CS_VAPRT_NOTE(x, ...) c4s::logbase::get()->vaprt(CS_NOTE, x, __VA_ARGS__)
#else
#define CS_VAPRT_NOTE(x, ...)                                                                      \
    do {                                                                                           \
    } while (0)
#endif
#if C4S_LOG_LEVEL <= 5
#define CS_VAPRT_WARN(x, ...) c4s::logbase::get()->vaprt(CS_WARN, x, __VA_ARGS__)
#else
#define CS_VAPRT_WARN(x, ...)                                                                      \
    do {                                                                                           \
    } while (0)
#endif
#if C4S_LOG_LEVEL <= 6
#define CS_VAPRT_ERRO(x, ...) c4s::logbase::get()->vaprt(CS_ERRO, x, __VA_ARGS__)
#else
#define CS_VAPRT_ERRO(x, ...)                                                                      \
    do {                                                                                           \
    } while (0)
#endif
#if C4S_LOG_LEVEL <= 7
#define CS_VAPRT_CRIT(x, ...) c4s::logbase::get()->vaprt(CS_CRIT, x, __VA_ARGS__)
#else
#define CS_VAPRT_CRIT(x, ...)                                                                      \
    do {                                                                                           \
    } while (0)
#endif

#endif
