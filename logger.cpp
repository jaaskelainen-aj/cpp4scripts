/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#if defined(__linux) || defined(__APPLE__)
#include <stdarg.h>
#include <syslog.h>
#else
#include <io.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "config.hpp"
#include "exception.hpp"
#include "user.hpp"
#include "path.hpp"
#include "logger.hpp"

using namespace std;
using namespace c4s;

#if defined(__linux) || defined(__APPLE__)
#define _stricmp strcasecmp
#define _write write
#endif

c4s::logbase* c4s::logbase::thelog = 0;
#if defined(__linux) || defined(__APPLE__)
int c4s::lowio_sink::mode = 0;
#endif
const char* g_level_names[c4s::LL_MAX] = { "NONE",   "TRACE",   "DEBUG", "INFO",
                                           "NOTICE", "WARNING", "ERROR", "CRITICAL" };

// -------------------------------------------------------------------------------------------------
const char*
c4s::log_sink::get_datetime(LOG_LEVEL ll)
{
    static char datetime[64]; // 2013-01-01 23:30:30 [CRITICAL]
    time_t now = time(0);
    struct tm* lt = localtime(&now);
    sprintf(datetime, "%d-%02d-%02d %02d:%02d:%02d [%-8s] ", lt->tm_year + 1900, lt->tm_mon + 1,
            lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec, g_level_names[ll]);
    return datetime;
}
// -------------------------------------------------------------------------------------------------
void
c4s::logbase::vaprt(c4s::LOG_LEVEL ll, const char* str, ...)
{
    static char vabuffer[C4S_LOG_VABUFFER_SIZE];
    va_list va;
    if (ll != LL_NONE && ll >= level) {
        va_start(va, str);
        vsnprintf(vabuffer, sizeof(vabuffer) - 1, str, va);
        va_end(va);
        sink->print(ll, vabuffer);
    }
}

// -------------------------------------------------------------------------------------------------
#if defined(__linux) || defined(__APPLE__)
c4s::syslog_sink::syslog_sink(const char* name, int facility)
{
    levelmap[0] = 0;
    levelmap[1] = 0;
    levelmap[2] = LOG_DEBUG;
    levelmap[3] = LOG_INFO;
    levelmap[4] = LOG_NOTICE;
    levelmap[5] = LOG_WARNING;
    levelmap[6] = LOG_ERR;
    levelmap[7] = LOG_CRIT;
    openlog(name, LOG_NDELAY, facility);
}

c4s::syslog_sink::~syslog_sink()
{
    closelog();
}

#if defined(__linux) || defined(__APPLE__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
void
c4s::syslog_sink::print(c4s::LOG_LEVEL ll, const char* str)
{
    int slfac = levelmap[(int)ll];
    if (slfac)
        syslog(slfac, str);
}
#if defined(__linux) || defined(__APPLE__)
#pragma GCC diagnostic pop
#endif
#endif
// -------------------------------------------------------------------------------------------------
fstream_sink::fstream_sink(const path& logp)
{
    bool append = false;
    if (logp.exists()) {
        log_file.open(logp.get_path().c_str(), ios_base::out | ios_base::app);
        append = true;
    } else
        log_file.open(logp.get_path().c_str());
    if (!log_file)
        throw c4s_exception("fstream_sink::fstream_sink - unable to open given file for logging.");
    if (append)
        log_file << "---------------------------------------------------------------------\n";
    time_t now = time(0);
    log_file << "=== Log started: " << ctime(&now);
}
fstream_sink::~fstream_sink()
{
    log_file.close();
}
void
fstream_sink::print(c4s::LOG_LEVEL ll, const char* str)
{
    log_file << get_datetime(ll) << str << '\n';
    if (ll == LL_TRACE)
        log_file.flush();
}

// -------------------------------------------------------------------------------------------------
lowio_sink::lowio_sink(const path& logp)
{
    bool append = logp.exists();
#if defined(__linux) || defined(__APPLE__)
    fid = open(logp.get_path().c_str(), O_CREAT | O_WRONLY | O_APPEND,
               S_IREAD | S_IWRITE | S_IRGRP | S_IROTH);
    if (fid == -1) {
        ostringstream sserr;
        sserr << "lowio_sink::lowio_sink - unable to open given file for logging: (" << errno
              << ") " << strerror(errno);
        throw c4s_exception(sserr.str());
    }
    if (!mode)
        fchmod(fid, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    else
        fchmod(fid, mode);
#else
    if (_sopen_s(&fid, logp.get_path().c_str(), _O_CREAT | _O_APPEND | _O_WRONLY, _SH_DENYWR,
                 _S_IREAD | _S_IWRITE))
        throw c4s_exception("lowio_sink::lowio_sink - unable to open given file for loggind.");
#endif
    if (append)
        _write(fid, "---------------------------------------------------------------------\n", 70);
    // _write(fid,"Log started:",12);
    // _write(fid,datestr,(unsigned int)strlen(datestr));
    // _write(fid,"\n",1);
}
lowio_sink::~lowio_sink()
{
    if (fid > 0)
#if defined(__linux) || defined(__APPLE__)
        close(fid);
#else
        _close(fid);
#endif
}
void
lowio_sink::print(c4s::LOG_LEVEL ll, const char* str)
{
    write(fid, get_datetime(ll), 31);
#if defined(__linux) || defined(__APPLE__)
    write(fid, str, strlen(str));
    write(fid, "\n", 1);
#else
    _write(fid, str, (unsigned int)strlen(str));
    _write(fid, "\n", 1);
#endif
}
// -------------------------------------------------------------------------------------------------
void
stderr_sink::print(LOG_LEVEL ll, const char* str)
{
    cerr << get_datetime(ll) << str << '\n';
}
// -------------------------------------------------------------------------------------------------
c4s::LOG_LEVEL
c4s::logbase::str2level(const char* name)
{
    const int MAX = 8;
    LOG_LEVEL level_value[MAX] = { LL_NONE,   LL_TRACE,   LL_DEBUG, LL_INFO,
                                   LL_NOTICE, LL_WARNING, LL_ERROR, LL_CRITICAL };
    for (int ndx = 0; ndx < MAX; ndx++) {
        if (!_stricmp(name, g_level_names[ndx]))
            return level_value[ndx];
    }
    ostringstream err;
    err << "logbase::str2level - Unknown log level string:" << name;
    throw c4s_exception(err.str());
    return LL_NONE;
}

// -------------------------------------------------------------------------------------------------
c4s::logger&
c4s::logger::operator<<(c4s::LOG_LEVEL ll)
{
    if (ll != LL_NONE && ll >= level)
        sink->print(ll, buffer.str().c_str());
    buffer.str("");
    return *this;
}
