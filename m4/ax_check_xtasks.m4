#
# SYNOPSIS
#
#   AX_CHECK_XTASKS
#
# DESCRIPTION
#
#   Check whether a valid xTasks library is available, and the path to the headers
#   and libraries are correctly specified.
#
# LICENSE
#
#   Copyright 2017-2018 Barcelona Supercomputing Center
#
#   This file is part of the NANOS++ library.
#
#   NANOS++ is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Lesser General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   NANOS++ is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public License
#   along with NANOS++.  If not, see <http://www.gnu.org/licenses/>.
#

AC_DEFUN([AX_CHECK_XTASKS],
[
#Check if xTasks lib is installed.
AC_ARG_WITH(xtasks,
  [AS_HELP_STRING([--with-xtasks,--with-xtasks=PATH],
                [search in system directories or specify prefix directory for installed xTasks package.])],
  [
    # Check if the user provided a valid PATH
    AS_IF([test -d "$withval"],[
      xtasks=yes
      xtasks_path_provided=yes
    ],[
      xtasks=$withval
      xtasks_path_provided=no
    ])dnl
  ],[
    # Default: check if xtasks is available
    xtasks=no
    xtasks_path_provided=no
  ])

# If the user specifies --with-xtasks, $with_xtasks value will be 'yes'
#                       --without-xtasks, $with_xtasks value will be 'no'
#                       --with-xtasks=somevalue, $with_xtasks value will be 'somevalue'
AS_IF([test "$xtasks_path_provided" = yes],[
  xtasksinc="-I$with_xtasks/include"
  AS_IF([test -d $with_xtasks/lib64],
    [xtaskslib="-L$with_xtasks/lib64 -Wl,-rpath,$with_xtasks/lib64"],
    [xtaskslib="-L$with_xtasks/lib -Wl,-rpath,$with_xtasks/lib"])dnl
])dnl

AS_IF([test "$xtasks" = yes],[
  AX_VAR_PUSHVALUE([CPPFLAGS],[$CPPFLAGS $xtasksinc])
  AX_VAR_PUSHVALUE([CXXFLAGS])
  AX_VAR_PUSHVALUE([LDFLAGS],[$LDFLAGS $xtaskslib])
  AX_VAR_PUSHVALUE([LIBS],[])

  # Check for header
  AC_CHECK_HEADERS([libxtasks.h],
    [xtasks=yes],
    [xtasks=no])

  # Look for xtasksInit function in libxtasks
  AS_IF([test "$xtasks" = yes],[
      AC_SEARCH_LIBS([xtasksInit],
                [xtasks],
                [xtasks=yes],
                [xtasks=no])
  ])dnl


  xtaskslibs="$LIBS"

  AX_VAR_POPVALUE([CPPFLAGS])
  AX_VAR_POPVALUE([CXXFLAGS])
  AX_VAR_POPVALUE([LDFLAGS])
  AX_VAR_POPVALUE([LIBS])

])dnl

AC_SUBST([xtasksinc])
AC_SUBST([xtaskslib])
AC_SUBST([xtaskslibs])
AM_CONDITIONAL([XTASKS_SUPPORT],[test "$xtasks" = yes])

AS_IF([test "$xtasks" = yes], [
   AC_SUBST([HAVE_XTASKS], [XTASKS])
], [
   AC_SUBST([HAVE_XTASKS], [NO_XTASKS])
])

])dnl AX_CHECK_XTASKS
