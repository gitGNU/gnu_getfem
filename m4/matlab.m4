dnl matlab.m4 --- check for Matlab.
dnl
dnl Copyright (C) 2000--2002 Ralph Schleicher
dnl
dnl This program is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU General Public License as
dnl published by the Free Software Foundation; either version 2,
dnl or (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; see the file COPYING.  If not, write to
dnl the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
dnl Boston, MA 02111-1307, USA.
dnl
dnl As a special exception to the GNU General Public License, if
dnl you distribute this file as part of a program that contains a
dnl configuration script generated by GNU Autoconf, you may include
dnl it under the same distribution terms that you use for the rest
dnl of that program.
dnl
dnl Code:

# MX_MATLAB
# ---------
# Check for Matlab.
AC_DEFUN([MX_MATLAB],
[dnl
AC_PREREQ([2.50])
mx_enable_matlab=
AC_ARG_WITH([matlab], AC_HELP_STRING([--with-matlab=ARG], [check for Matlab [[yes]]]),
[case $withval in
  yes | no)
    # Explicitly enable or disable Matlab but determine
    # Matlab prefix automatically.
    mx_enable_matlab=$withval
    ;;
  *)
    # Enable Matlab and use ARG as the Matlab prefix.
    # ARG must be an existing directory.
    mx_enable_matlab=yes
    MATLAB=`cd "${withval-/}" > /dev/null 2>&1 && pwd`
    if test -z "$MATLAB" ; then
	AC_MSG_ERROR([invalid value \`$withval' for --with-matlab])
    fi
    ;;
esac])
AC_CACHE_CHECK([for Matlab prefix], [mx_cv_matlab],
[if test "${MATLAB+set}" = set ; then
    mx_cv_matlab=`cd "${MATLAB-/}" > /dev/null 2>&1 && pwd`
else
    mx_cv_matlab=
    IFS=${IFS= 	} ; mx_ifs=$IFS ; IFS=:
    for mx_dir in ${PATH-/opt/bin:/usr/local/bin:/usr/bin:/bin} ; do
	if test -z "$mx_dir" ; then
	    mx_dir=.
	fi
	if test -x "$mx_dir/matlab" ; then
	    mx_dir=`echo "$mx_dir" | sed 's,/bin$,,'`
	    # Directory sanity check.
	    mx_cv_matlab=`cd "${mx_dir-/}" > /dev/null 2>&1 && pwd`
	    if test -n "$mx_cv_matlab" ; then
		break
	    fi
	fi
    done
    IFS=$mx_ifs
fi
if test -z "$mx_cv_matlab" ; then
    mx_cv_matlab="not found"
fi])
if test "$mx_cv_matlab" = "not found" ; then
    unset MATLAB
else
    # Strip trailing dashes.
    MATLAB=`echo "$mx_cv_matlab" | sed 's,/*$,,'`
fi
AC_MSG_CHECKING([whether to enable Matlab support])
if test x$mx_enable_matlab != xno ; then
    if test "${MATLAB+set}" = set && test -d "$MATLAB/extern/include" ; then
	mx_enable_matlab=yes
    elif test x$mx_enable_matlab = x ; then
	mx_enable_matlab=no
    else
	# Fail if Matlab was explicitly enabled.
	AC_MSG_RESULT([failure])
	AC_MSG_ERROR([check your Matlab setup])
    fi
fi
AC_MSG_RESULT([$mx_enable_matlab])
if test x$mx_enable_matlab = xyes ; then
    AC_DEFINE([HAVE_MATLAB], [1], [Define if you have Matlab.])
fi
AC_SUBST([MATLAB])
])

# MX_REQUIRE_MATLAB
# -----------------
# Like MX_MATLAB but fail if Matlab support is disabled.
AC_DEFUN([MX_REQUIRE_MATLAB],
[dnl
AC_PREREQ([2.50])
AC_REQUIRE([MX_MATLAB])
if test x$mx_enable_matlab = xno ; then
    AC_MSG_ERROR([can not enable Matlab support])
fi
])

# MX_MATLAB_CONDITIONAL
# ---------------------
# Define Matlab conditional for GNU Automake.
AC_DEFUN([MX_MATLAB_CONDITIONAL],
[dnl
AC_PREREQ([2.50])
AC_REQUIRE([MX_MATLAB])
AM_CONDITIONAL([MATLAB], [test x$mx_enable_matlab = xyes])
])

dnl matlab.m4 ends here