AC_PREREQ(2.59)
AC_INIT([droplet-sh], [0.1.0])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_MACRO_DIR([m4])

AM_CONFIG_HEADER(config.h)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip subdir-objects])

AC_PROG_CC
AM_PROG_CC_C_O

AC_HEADER_STDC
AC_PROG_INSTALL
AC_PROG_LN_S

AC_ARG_WITH(json_c_include_dir,
        [  --with-json-c-include-dir=
			  Specify json-c include dir [default=/usr/include/json]],
        [json_c_include_dir="$withval"],[json_c_include_dir="/usr/include/json"])

json_c_include_dir="$(cd $json_c_include_dir ; pwd)"

JSON_CFLAGS="-I$json_c_include_dir $JSON_CFLAGS"

AC_ARG_WITH(json_c_lib,
        [  --with-json-c-lib=
			  Specify linker cmd to link with json-c lib [default=-ljson]],
        [json_c_lib="$withval"],[json_c_lib="-ljson"])

JSON_LIBS="$json_c_lib $JSON_LIBS"

AC_SUBST(JSON_CFLAGS)
AC_SUBST(JSON_LIBS)


AC_ARG_WITH(droplet_include_dir,
        [  --with-droplet-include-dir=
			  Specify droplet include dir [default=/usr/local/include/droplet]],
        [droplet_include_dir="$withval"],[droplet_include_dir="/usr/local/include/droplet"])

droplet_include_dir="$(cd $json_c_include_dir ; pwd)"

LIBDROPET_CFLAGS="-I$droplet_include_dir $LIBDROPLET_CFLAGS"

AC_ARG_WITH(droplet_lib,
        [  --with-droplet-lib=
			  Specify linker cmd to link with droplet lib [default=-ldroplet]],
        [droplet_lib="$withval"],[droplet_lib="-ldroplet"])

LIBDROPLET_LIBS="$droplet_lib $LIBDROPLET_LIBS"

AC_SUBST(LIBDROPLET_CFLAGS)
AC_SUBST(LIBDROPLET_LIBS)

PKG_CHECK_MODULES(OPENSSL, openssl, [LIBS="$LIBS $OPENSSL_LIBS" CFLAGS="$CFLAGS $OPENSSL_CFLAGS"])
PKG_CHECK_MODULES(LIBXML, libxml-2.0, [LIBS="$LIBS $LIBXML_LIBS" CFLAGS="$CFLAGS $LIBXML_CFLAGS"])

PKG_CHECK_MODULES(LIBEDIT, libedit)
PKG_CHECK_MODULES(LIBDROPLET, droplet-3.0)
AC_SEARCH_LIBS([rl_initialize], [edit], [LIBS="$LIBS $LIBEDIT_LIBS" CFLAGS="$CFLAGS $LIBEDIT_CFLAGS"])
if test x"$ac_cv_search_rl_initialize" = x"no"; then    # Try again linking ncurses this time to workaround a bug on CentOS 5.5
    # where linking with editline will fail if ncurses isn't linked as well.
    unset ac_cv_search_rl_abort
    AC_SEARCH_LIBS([rl_initialize], [edit], [LIBS="$LIBS $LIBEDIT_LIBS -lncurses" CFLAGS="$CFLAGS $LIBEDIT_CFLAGS"], [AC_MSG_ERROR([libedit library missing])], [-lncurses])
fi

AC_CONFIG_FILES([
Makefile
src/Makefile
])
AC_OUTPUT

