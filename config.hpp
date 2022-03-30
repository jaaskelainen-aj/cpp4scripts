/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#ifndef CPP4SCRIPTS_CONFIG
#define CPP4SCRIPTS_CONFIG

/* If log macros are used then log statements can be removed from executable at the compile time.
   The higher the level number, less messages are displayed. 1 = trace ... 7 critical. Set
   C4S_LOG_LEVEL define to 0 to include all logs to build and to 8 to remove all from binary.*/
#ifndef C4S_LOG_LEVEL
#ifdef NDEBUG
#define C4S_LOG_LEVEL 3
#else
#define C4S_LOG_LEVEL 2
#endif
#endif

/* Specifies the internal buffer size for logger::vaprt function.*/
#ifndef C4S_LOG_VABUFFER_SIZE
#define C4S_LOG_VABUFFER_SIZE 0x800
#endif

#ifndef C4S_BUILD_RECURSIVE_CHECK_MAX
#define C4S_BUILD_RECURSIVE_CHECK_MAX 5
#endif

#if defined(__linux) || defined(__APPLE__)
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#define HANDLE pid_t
#define C4S_DSEP '/'
#define C4S_PSEP ':'
#define C4S_QUOT '\''
#define SSIZE_T ssize_t
#define SIZE_T size_t
#endif

#ifdef _WIN32
#include <windows.h>
#define C4S_DSEP '\\'
#define C4S_PSEP ';'
#define C4S_QUOT '\"'
#endif

namespace c4s {

// Library constants
const SSIZE_T SSIZE_T_MAX = ~0;
const int     MAX_LINE    = 512;
const size_t  RB_SIZE_SM  = 256;
const size_t  RB_SIZE_LG  = 8192;
const int     MAX_NESTING = 50;
const int     BUILDER_TIMEOUT = 45;
const unsigned long FNV_1_PRIME = 0x84222325cbf29ce4UL;
const int MAX_PROCESS_ARGS = 100;

}

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#endif // CPP4SCRIPTS_CONFIG
