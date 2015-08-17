#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.63])
AC_INIT([xltop],
	[1.1],
	[jhammond@tacc.utexas.edu],
	[xltop],
	[https://github.com/jhammond/xltop/])
AC_CONFIG_SRCDIR([src/xltop.c])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AC_PROG_CC_C99
AC_USE_SYSTEM_EXTENSIONS
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_INSTALL
AC_CHECK_PROG([CURL], "curl", yes, no, [$PATH])

xltop_conf_dir=${sysconfdir}/${PACKAGE_NAME}
AC_ARG_WITH([conf-dir],
            AS_HELP_STRING([--with-conf-dir=DIR],
                           [set path of xltop conf dir @<:@default=${sysconfdir}/xltop/@:>@]),
            [xltop_conf_dir="$withval"])
AC_SUBST(xltop_conf_dir)

xltop_master=
AC_ARG_WITH([master],
            AS_HELP_STRING([--with-master=HOST-OR-ADDR], [set hostname or address of xltop-master]),
            [xltop_master="$withval"])
AC_SUBST(xltop_master)

xltop_port=9901
AC_ARG_WITH([port],
            AS_HELP_STRING([--with-port=PORT], [set port for xltop-master @<:@default=9901@:>@]),
            [xltop_port="$withval"])
AC_SUBST(xltop_port)

xltop_clus=
AC_ARG_WITH([clus],
            AS_HELP_STRING([--with-clus=CLUS], [set default cluster for xltop client]),
            [xltop_clus="$withval"])
AC_SUBST(xltop_clus)

xltop_domain=
AC_ARG_WITH([domain],
            AS_HELP_STRING([--with-domain=DOMAIN], [set default domain for xltop client]),
            [xltop_domain="$withval"])
AC_SUBST(xltop_domain)

AC_ARG_ENABLE([syslog],
	AS_HELP_STRING([--disable-syslog], [disable system logging @<:@default=enabled@:>@]),
	[], enable_syslog=yes)

AS_IF([test "x$enable_syslog" = "xyes"], [
	    AC_DEFINE(ENABLE_SYSLOG, [1], [System logging.])
])

AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug], [enable debug messages @<:@default=disabled@:>@]),
	[], [enable_debug=no])

AS_IF([test "x$enable_debug" = "xyes"], [
	    AC_DEFINE(DEBUG, [1], [Debug messages.])
])

# Checks for libraries.
AC_CHECK_LIB([confuse], [cfg_init])
AC_CHECK_LIB([curl], [curl_easy_init])
AC_CHECK_LIB([ev], [ev_run])
AC_CHECK_LIB([ncurses], [initscr])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h malloc.h netdb.h stddef.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h termios.h unistd.h])

AC_CHECK_HEADERS(confuse.h)
AC_CHECK_HEADERS(curl/curl.h)
AC_CHECK_HEADERS(libev/ev.h)
AC_CHECK_HEADERS(ncurses.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor getcwd gethostname isascii memchr memmove memset socket strcasecmp strchr strcspn strdup strerror strstr strtol strtoul sysinfo])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
AC_MSG_RESULT([
        $PACKAGE $VERSION
        =====

        compiler:               ${CC}
        cflags:                 ${CFLAGS}
        ldflags:                ${LDFLAGS}

        prefix:                 ${prefix}
	conf_dir:               ${xltop_conf_dir}
	master:                 ${xltop_master:-NONE}
        port:                   ${xltop_port}
        syslog:                 ${enable_syslog}
        debug:                  ${enable_debug}
])
