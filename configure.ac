#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT(lusush, 0.1, trismegustis@gmail.com)
AC_CONFIG_SRCDIR([src/lusush.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC_C99

# Checks for libraries.
#AC_CHECK_LIB(readline, readline)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([getcwd setenv strdup strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
