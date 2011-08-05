AC_PREREQ(2.59)
AC_INIT([droplet-sh], [0.1.0])
AC_CONFIG_SRCDIR([src])

AM_CONFIG_HEADER(config.h)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip subdir-objects])

AC_PROG_CC
AM_PROG_CC_C_O

AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_LN_S

PKG_CHECK_MODULES(LIBDROPLET, droplet-1.0)
AC_SEARCH_LIBS([rl_abort], [readline])
if test x"$ac_cv_search_rl_abort" = x"no"; then
    # Try again linking ncurses this time to workaround a bug on CentOS 5.5
    # where linking with readline will fail if ncurses isn't linked as well.
    unset ac_cv_search_rl_abort
    AC_SEARCH_LIBS([rl_abort], [readline], [LIBS="$LIBS -lncurses"], [AC_MSG_ERROR([readline library missing])], [-lncurses])
fi

AC_CONFIG_FILES([
Makefile
src/Makefile
])
AC_OUTPUT
