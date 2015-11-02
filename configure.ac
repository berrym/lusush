#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([lusush], [0.0.1], [trismegustis@gmail.com])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([readline], [readline])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h \
		  locale.h \
		  stddef.h \
		  stdlib.h \
		  string.h \
		  termios.h \
		  unistd.h \
		  limits.h \
		  editline/readline.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_C_RESTRICT

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit \
		dup2 \
		getcwd \
		gethostname \
		memset \
		setenv \
		setlocale \
		strerror \
		strncasecmp\
		strnlen])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
