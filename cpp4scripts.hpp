/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#ifndef CPP4SCRIPTS_H
#define CPP4SCRIPTS_H

#if !defined(CPP4SCRIPTS_ALL_HPP) && !defined(_CRT_SECURE_NO_WARNINGS) && defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "config.hpp"
#include "exception.hpp"
#if defined(__linux) || defined(__APPLE__)
#include "user.hpp"
#endif
#include "path.hpp"
#include "path_list.hpp"
#include "path_stack.hpp"
#include "compiled_file.hpp"
#include "program_arguments.hpp"
#include "logger.hpp"
#include "process.hpp"
#include "settings.hpp"
#include "util.hpp"
#include "variables.hpp"
#include "builder.hpp"
#if defined(__linux) || defined(__APPLE__)
#include "builder_gcc.hpp"
#endif
#ifdef _MSC_VER
#include "builder_ml.hpp"
#include "builder_vc.hpp"
#endif
#endif
