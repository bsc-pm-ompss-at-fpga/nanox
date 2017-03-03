#
# SYNOPSIS
#
#   AX_CHECK_RESILIENCY
#
# DESCRIPTION
#
#   Checks that all requirements for resiliency features are met.
#
# LICENSE
#
#   Copyright (c) 2016 Jorge Bellon <jbellon@bsc.es>
#
#   This program is free software: you can redistribute it and/or modify it
#   under the terms of the GNU General Public License as published by the
#   Free Software Foundation, either version 3 of the License, or (at your
#   option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
#   Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program. If not, see <http://www.gnu.org/licenses/>.
#
#   As a special exception, the respective Autoconf Macro's copyright owner
#   gives unlimited permission to copy, distribute and modify the configure
#   scripts that are the output of Autoconf when processing the Macro. You
#   need not follow the terms of the GNU General Public License when using
#   or distributing such scripts, even though portions of the text of the
#   Macro appear in them. The GNU General Public License (GPL) does govern
#   all other use of the material that constitutes the Autoconf Macro.
#
#   This special exception to the GPL applies to versions of the Autoconf
#   Macro released by the Autoconf Archive. When you make and distribute a
#   modified version of the Autoconf Macro, you may extend this special
#   exception to the GPL to apply to your modified version as well.

AC_DEFUN([AX_CHECK_RESILIENCY],[

# Resiliency checks
AC_MSG_CHECKING([if task resiliency is enabled])
AC_ARG_ENABLE([resiliency],[AS_HELP_STRING([--enable-resiliency], [Enables task-level resiliency])])
AC_MSG_RESULT([$enable_resiliency])

AS_IF([test "$enable_resiliency" = "yes"],[
  AX_CHECK_COMPILE_FLAG([-fnon-call-exceptions],
    [],
    [AC_MSG_ERROR([resiliency mechanism depends on using compiler flag -fnon-call-exceptions])],
    [-Werror])

  # Boost C++ library
  AX_BOOST_BASE([1.38],
    [],#nothing to do if it passes the test
    [
       AC_MSG_ERROR([resiliency mechanism requires Boost C++ library])
    ])

  AC_DEFINE([NANOS_RESILIENCY_ENABLED],[],[Indicates whether resiliency features should be used or not.])
  resiliency_flags=-fnon-call-exceptions
])

# Fault injection
AC_MSG_CHECKING([if fault injection is enabled])
AC_ARG_ENABLE([fault-injection],
  [AS_HELP_STRING([--enable-fault-injection],
    [Enables page corruption injection module.])])
AC_MSG_RESULT([$enable_fault_injection])

AS_IF([test "$enable_fault_injection" = "yes"],[
    # Check if random generators and engines are compilable
    # Some compilers do not support C++11 standard completely
    # Example: icpc with gcc version less than 4.8
    AC_LANG_PUSH(C++)
    AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
            [[@%:@include <random>
            std::mt19937 generator;
            std::uniform_int_distribution<size_t> distribution(0, 10);
            ]], [[
            size_t pos = distribution(generator);
            ]])],
        [stl_random_support=yes],
        [stl_random_support=no])
    AC_LANG_POP([C++])
    AS_IF([test "$stl_random_support" != "yes"],[
        AC_MSG_ERROR([fault injection module depends on standard random number library. Try using a newer compiler version.])
    ])

    AC_DEFINE([NANOS_FAULT_INJECTION],[1],[Defined when page corruption injection module is enabled by the user.])
])

AC_SUBST([resiliency_flags])  # Extra build flags
AC_SUBST([enable_resiliency]) # Test generator
AM_CONDITIONAL([is_resiliency_enabled],[test "$enable_resiliency" = "yes"])
AM_CONDITIONAL([is_fault_injection_enabled],[test "$enable_fault_injection" = "yes"])

])dnl AX_CHECK_RESILIENCY
