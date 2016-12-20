/*
Copyright 2010-2013 SourceGear, LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/**
 *
 * @file u0054_repo_encodings.c
 *
 */

//////////////////////////////////////////////////////////////////

#include <sg.h>
#include "unittests.h"
#include "unittests_pendingtree.h"

#define MOZ_AC_V1 "dnl -*- Mode: Autoconf; tab-width: 4; indent-tabs-mode: nil; -*-\n\
dnl vi: set tabstop=4 shiftwidth=4 expandtab:\n\
dnl ***** BEGIN LICENSE BLOCK *****\n\
dnl Version: MPL 1.1/GPL 2.0/LGPL 2.1\n\
dnl\n\
dnl The contents of this file are subject to the Mozilla Public License Version\n\
dnl 1.1 (the \"License\"); you may not use this file except in compliance with\n\
dnl the License. You may obtain a copy of the License at\n\
dnl http://www.mozilla.org/MPL/\n\
dnl\n\
dnl Software distributed under the License is distributed on an \"AS IS\" basis,\n\
dnl WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License\n\
dnl for the specific language governing rights and limitations under the\n\
dnl License.\n\
dnl\n\
dnl The Original Code is this file as it was released upon August 6, 1998.\n\
dnl\n\
dnl The Initial Developer of the Original Code is\n\
dnl Christopher Seawood.\n\
dnl Portions created by the Initial Developer are Copyright (C) 1998-2001\n\
dnl the Initial Developer. All Rights Reserved.\n\
dnl\n\
dnl Contributor(s):\n\
dnl   Jamie Zawinski <jwz@jwz.org>\n\
dnl   gettimeofday args check\n\
dnl   Christopher Blizzard <blizzard@appliedtheory.com>\n\
dnl   gnomefe update & enable-pthreads\n\
dnl   Ramiro Estrugo <ramiro@netscape.com>\n\
dnl   X11 makedepend support\n\
dnl   Insure support.\n\
dnl   Henry Sobotka <sobotka@axess.com>\n\
dnl   OS/2 support\n\
dnl   Dan Mosedale <dmose@mozilla.org>\n\
dnl   LDAP support\n\
dnl   Seth Spitzer <sspitzer@netscape.com>\n\
dnl   xpctools support\n\
dnl   Benjamin Smedberg <benjamin@smedbergs.us>\n\
dnl   Howard Chu <hyc@symas.com>\n\
dnl   MSYS support\n\
dnl   Mark Mentovai <mark@moxienet.com>:\n\
dnl   Mac OS X 10.4 support\n\
dnl   Giorgio Maone <g.maone@informaction.com>\n\
dnl   MSVC l10n compatible version check\n\
dnl\n\
dnl Alternatively, the contents of this file may be used under the terms of\n\
dnl either the GNU General Public License Version 2 or later (the \"GPL\"), or\n\
dnl the GNU Lesser General Public License Version 2.1 or later (the \"LGPL\"),\n\
dnl in which case the provisions of the GPL or the LGPL are applicable instead\n\
dnl of those above. If you wish to allow use of your version of this file only\n\
dnl under the terms of either the GPL or the LGPL, and not to allow others to\n\
dnl use your version of this file under the terms of the MPL, indicate your\n\
dnl decision by deleting the provisions above and replace them with the notice\n\
dnl and other provisions required by the GPL or the LGPL. If you do not delete\n\
dnl the provisions above, a recipient may use your version of this file under\n\
dnl the terms of any one of the MPL, the GPL or the LGPL.\n\
dnl\n\
dnl ***** END LICENSE BLOCK *****\n\
\n\
dnl Process this file with autoconf to produce a configure script.\n\
dnl ========================================================\n\
\n\
AC_PREREQ(2.13)\n\
AC_INIT(config/config.mk)\n\
AC_CONFIG_AUX_DIR(${srcdir}/build/autoconf)\n\
AC_CANONICAL_SYSTEM\n\
TARGET_CPU=\"${target_cpu}\"\n\
TARGET_VENDOR=\"${target_vendor}\"\n\
TARGET_OS=\"${target_os}\"\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Don\'t change the following two lines.  Doing so breaks:\n\
dnl =\n\
dnl = CFLAGS=\"-foo\" ./configure\n\
dnl =\n\
dnl ========================================================\n\
CFLAGS=\"${CFLAGS=}\"\n\
CPPFLAGS=\"${CPPFLAGS=}\"\n\
CXXFLAGS=\"${CXXFLAGS=}\"\n\
LDFLAGS=\"${LDFLAGS=}\"\n\
HOST_CFLAGS=\"${HOST_CFLAGS=}\"\n\
HOST_CXXFLAGS=\"${HOST_CXXFLAGS=}\"\n\
HOST_LDFLAGS=\"${HOST_LDFLAGS=}\"\n\
\n\
dnl ========================================================\n\
dnl = Preserve certain environment flags passed to configure\n\
dnl = We want sub projects to receive the same flags\n\
dnl = untainted by this configure script\n\
dnl ========================================================\n\
_SUBDIR_CC=\"$CC\"\n\
_SUBDIR_CXX=\"$CXX\"\n\
_SUBDIR_CFLAGS=\"$CFLAGS\"\n\
_SUBDIR_CPPFLAGS=\"$CPPFLAGS\"\n\
_SUBDIR_CXXFLAGS=\"$CXXFLAGS\"\n\
_SUBDIR_LDFLAGS=\"$LDFLAGS\"\n\
_SUBDIR_HOST_CC=\"$HOST_CC\"\n\
_SUBDIR_HOST_CFLAGS=\"$HOST_CFLAGS\"\n\
_SUBDIR_HOST_CXXFLAGS=\"$HOST_CXXFLAGS\"\n\
_SUBDIR_HOST_LDFLAGS=\"$HOST_LDFLAGS\"\n\
_SUBDIR_CONFIG_ARGS=\"$ac_configure_args\"\n\
\n\
dnl Set the version number of the libs included with mozilla\n\
dnl ========================================================\n\
MOZJPEG=62\n\
MOZPNG=10207\n\
MOZZLIB=1.2.3\n\
NSPR_VERSION=4\n\
NSS_VERSION=3\n\
\n\
dnl Set the minimum version of toolkit libs used by mozilla\n\
dnl ========================================================\n\
GLIB_VERSION=1.2.0\n\
GTK_VERSION=1.2.0\n\
LIBIDL_VERSION=0.6.3\n\
PERL_VERSION=5.006\n\
QT_VERSION=3.2.0\n\
QT_VERSION_NUM=320\n\
LIBART_VERSION=2.3.4\n\
CAIRO_VERSION=0.9.1\n\
GLITZ_VERSION=0.4.0\n\
GTK2_VERSION=1.3.7\n\
MAKE_VERSION=3.78\n\
WINDRES_VERSION=2.14.90\n\
W32API_VERSION=3.8\n\
GNOMEVFS_VERSION=2.0\n\
GNOMEUI_VERSION=2.2.0\n\
GCONF_VERSION=1.2.1\n\
LIBGNOME_VERSION=2.0\n\
STARTUP_NOTIFICATION_VERSION=0.8\n\
\n\
MSMANIFEST_TOOL=\n\
\n\
dnl Set various checks\n\
dnl ========================================================\n\
MISSING_X=\n\
AC_PROG_AWK\n\
\n\
dnl Initialize the Pthread test variables early so they can be\n\
dnl  overridden by each platform.\n\
dnl ========================================================\n\
USE_PTHREADS=\n\
_PTHREAD_LDFLAGS=\"\"\n\
\n\
dnl Do not allow a separate objdir build if a srcdir build exists.\n\
dnl ==============================================================\n\
_topsrcdir=`cd \\`dirname $0\\`; pwd`\n\
_objdir=`pwd`\n\
if test \"$_topsrcdir\" != \"$_objdir\"\n\
then\n\
  # Check for a couple representative files in the source tree\n\
  _conflict_files=\n\
  for file in $_topsrcdir/Makefile $_topsrcdir/config/autoconf.mk; do\n\
    if test -f $file; then\n\
      _conflict_files=\"$_conflict_files $file\"\n\
    fi\n\
  done\n\
  if test \"$_conflict_files\"; then\n\
    echo \"***\"\n\
    echo \"*   Your source tree contains these files:\"\n\
    for file in $_conflict_files; do\n\
      echo \"*         $file\"\n\
    done\n\
    cat 1>&2 <<-EOF\n\
	*   This indicates that you previously built in the source tree.\n\
	*   A source tree build can confuse the separate objdir build.\n\
	*\n\
	*   To clean up the source tree:\n\
	*     1. cd $_topsrcdir\n\
	*     2. gmake distclean\n\
	***\n\
	EOF\n\
    exit 1\n\
    break\n\
  fi\n\
fi\n\
MOZ_BUILD_ROOT=`pwd`\n\
\n\
dnl Default to MSVC for win32\n\
dnl ==============================================================\n\
if test -z \"$CROSS_COMPILE\"; then\n\
case \"$target\" in\n\
*-cygwin*|*-mingw*|*-msvc*|*-mks*)\n\
    MAKE_VERSION=3.79\n\
    if test -z \"$CC\"; then CC=cl; fi\n\
    if test -z \"$CXX\"; then CXX=cl; fi\n\
    if test -z \"$CPP\"; then CPP=cl; fi\n\
    if test -z \"$LD\"; then LD=link; fi\n\
    if test -z \"$AS\"; then AS=ml; fi\n\
    if test -z \"$MIDL\"; then MIDL=midl; fi\n\
    ;;\n\
esac\n\
fi\n\
\n\
COMPILE_ENVIRONMENT=1\n\
MOZ_ARG_ENABLE_BOOL(compile-environment,\n\
[  --disable-compile-environment\n\
                           Disable compiler/library checks.],\n\
    COMPILE_ENVIRONMENT=1,\n\
    COMPILE_ENVIRONMENT= )\n\
\n\
AC_PATH_PROGS(NSINSTALL_BIN, nsinstall )\n\
if test -z \"$COMPILE_ENVIRONMENT\"; then\n\
if test -z \"$NSINSTALL_BIN\" || test \"$NSINSTALL_BIN\" = \":\"; then\n\
    AC_MSG_ERROR([nsinstall not found in \\$PATH])\n\
fi\n\
fi\n\
AC_SUBST(NSINSTALL_BIN)\n\
\n\
dnl ========================================================\n\
dnl Checks for compilers.\n\
dnl ========================================================\n\
dnl Set CROSS_COMPILE in the environment when running configure\n\
dnl to use the cross-compile setup for now\n\
dnl ========================================================\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
\n\
dnl Do some special WinCE toolchain stuff\n\
case \"$target\" in\n\
*wince)\n\
    echo -----------------------------------------------------------------------------\n\
    echo Building Windows CE Shunt Library and Tool Chain\n\
    echo -----------------------------------------------------------------------------\n\
\n\
    echo -n \"#define TOPSRCDIR \\\"\" > $srcdir/build/wince/tools/topsrcdir.h\n\
    `$srcdir/build/cygwin-wrapper echo -n $_topsrcdir >> $srcdir/build/wince/tools/topsrcdir.h`\n\
    echo -n \\\" >> $srcdir/build/wince/tools/topsrcdir.h\n\
\n\
    make -C  $srcdir/build/wince/tools\n\
    echo -----------------------------------------------------------------------------\n\
    ;;\n\
esac\n\
\n\
if test -n \"$CROSS_COMPILE\" && test \"$target\" != \"$host\"; then\n\
    echo \"cross compiling from $host to $target\"\n\
    cross_compiling=yes\n\
\n\
    _SAVE_CC=\"$CC\"\n\
    _SAVE_CFLAGS=\"$CFLAGS\"\n\
    _SAVE_LDFLAGS=\"$LDFLAGS\"\n\
\n\
    AC_MSG_CHECKING([for host c compiler])\n\
    AC_CHECK_PROGS(HOST_CC, $HOST_CC gcc cc /usr/ucb/cc cl icc, \"\")\n\
    if test -z \"$HOST_CC\"; then\n\
        AC_MSG_ERROR([no acceptable c compiler found in \\$PATH])\n\
    fi\n\
    AC_MSG_RESULT([$HOST_CC])\n\
    AC_MSG_CHECKING([for host c++ compiler])\n\
    AC_CHECK_PROGS(HOST_CXX, $HOST_CXX $CCC c++ g++ gcc CC cxx cc++ cl icc, \"\")\n\
    if test -z \"$HOST_CXX\"; then\n\
        AC_MSG_ERROR([no acceptable c++ compiler found in \\$PATH])\n\
    fi\n\
    AC_MSG_RESULT([$HOST_CXX])\n\
\n\
    if test -z \"$HOST_CFLAGS\"; then\n\
        HOST_CFLAGS=\"$CFLAGS\"\n\
    fi\n\
    if test -z \"$HOST_CXXFLAGS\"; then\n\
        HOST_CXXFLAGS=\"$CXXFLAGS\"\n\
    fi\n\
    if test -z \"$HOST_LDFLAGS\"; then\n\
        HOST_LDFLAGS=\"$LDFLAGS\"\n\
    fi\n\
    AC_CHECK_PROGS(HOST_RANLIB, $HOST_RANLIB ranlib, ranlib, :)\n\
    AC_CHECK_PROGS(HOST_AR, $HOST_AR ar, ar, :)\n\
    CC=\"$HOST_CC\"\n\
    CFLAGS=\"$HOST_CFLAGS\"\n\
    LDFLAGS=\"$HOST_LDFLAGS\"\n\
\n\
    AC_MSG_CHECKING([whether the host c compiler ($HOST_CC $HOST_CFLAGS $HOST_LDFLAGS) works])\n\
    AC_TRY_COMPILE([], [return(0);], \n\
	[ac_cv_prog_hostcc_works=1 AC_MSG_RESULT([yes])],\n\
	AC_MSG_ERROR([installation or configuration problem: host compiler $HOST_CC cannot create executables.]) )\n\
\n\
    CC=\"$HOST_CXX\"\n\
    CFLAGS=\"$HOST_CXXFLAGS\"\n\
\n\
    AC_MSG_CHECKING([whether the host c++ compiler ($HOST_CXX $HOST_CXXFLAGS $HOST_LDFLAGS) works])\n\
    AC_TRY_COMPILE([], [return(0);], \n\
	[ac_cv_prog_hostcxx_works=1 AC_MSG_RESULT([yes])],\n\
	AC_MSG_ERROR([installation or configuration problem: host compiler $HOST_CXX cannot create executables.]) )\n\
    \n\
    CC=$_SAVE_CC\n\
    CFLAGS=$_SAVE_CFLAGS\n\
    LDFLAGS=$_SAVE_LDFLAGS\n\
\n\
    case \"$build:$target\" in\n\
      powerpc-apple-darwin8*:i?86-apple-darwin*)\n\
        dnl The Darwin cross compiler doesn\'t necessarily point itself at a\n\
        dnl root that has libraries for the proper architecture, it defaults\n\
        dnl to the system root.  The libraries in the system root on current\n\
        dnl versions of PPC OS X 10.4 aren\'t fat, so these target compiler\n\
        dnl checks will fail.  Fake a working SDK in that case.\n\
        _SAVE_CFLAGS=$CFLAGS\n\
        _SAVE_CXXFLAGS=$CXXLAGS\n\
        CFLAGS=\"-isysroot /Developer/SDKs/MacOSX10.4u.sdk $CFLAGS\"\n\
        CXXFLAGS=\"-isysroot /Developer/SDKs/MacOSX10.4u.sdk $CXXFLAGS\"\n\
        ;;\n\
    esac\n\
\n\
    AC_CHECK_PROGS(CC, $CC \"${target_alias}-gcc\" \"${target}-gcc\", :)\n\
    unset ac_cv_prog_CC\n\
    AC_PROG_CC\n\
    AC_CHECK_PROGS(CXX, $CXX \"${target_alias}-g++\" \"${target}-g++\", :)\n\
    unset ac_cv_prog_CXX\n\
    AC_PROG_CXX\n\
\n\
    case \"$build:$target\" in\n\
      powerpc-apple-darwin8*:i?86-apple-darwin*)\n\
        dnl Revert the changes made above.  From this point on, the target\n\
        dnl compiler will never be used without applying the SDK to CFLAGS\n\
        dnl (see --with-macos-sdk below).\n\
        CFLAGS=$_SAVE_CFLAGS\n\
        CXXFLAGS=$_SAVE_CXXFLAGS\n\
        ;;\n\
    esac\n\
\n\
    AC_CHECK_PROGS(RANLIB, $RANLIB \"${target_alias}-ranlib\" \"${target}-ranlib\", :)\n\
    AC_CHECK_PROGS(AR, $AR \"${target_alias}-ar\" \"${target}-ar\", :)\n\
    AC_PATH_PROGS(AS, $AS \"${target_alias}-as\" \"${target}-as\", :)\n\
    AC_CHECK_PROGS(LD, $LD \"${target_alias}-ld\" \"${target}-ld\", :)\n\
    AC_CHECK_PROGS(STRIP, $STRIP \"${target_alias}-strip\" \"${target}-strip\", :)\n\
    AC_CHECK_PROGS(WINDRES, $WINDRES \"${target_alias}-windres\" \"${target}-windres\", :)\n\
    AC_DEFINE(CROSS_COMPILE)\n\
else\n\
    AC_PROG_CC\n\
    AC_PROG_CXX\n\
    AC_PROG_RANLIB\n\
    AC_PATH_PROGS(AS, $AS as, $CC)\n\
    AC_CHECK_PROGS(AR, ar, :)\n\
    AC_CHECK_PROGS(LD, ld, :)\n\
    AC_CHECK_PROGS(STRIP, strip, :)\n\
    AC_CHECK_PROGS(WINDRES, windres, :)\n\
    if test -z \"$HOST_CC\"; then\n\
        HOST_CC=\"$CC\"\n\
    fi\n\
    if test -z \"$HOST_CFLAGS\"; then\n\
        HOST_CFLAGS=\"$CFLAGS\"\n\
    fi\n\
    if test -z \"$HOST_CXX\"; then\n\
        HOST_CXX=\"$CXX\"\n\
    fi\n\
    if test -z \"$HOST_CXXFLAGS\"; then\n\
        HOST_CXXFLAGS=\"$CXXFLAGS\"\n\
    fi\n\
    if test -z \"$HOST_LDFLAGS\"; then\n\
        HOST_LDFLAGS=\"$LDFLAGS\"\n\
    fi\n\
    if test -z \"$HOST_RANLIB\"; then\n\
        HOST_RANLIB=\"$RANLIB\"\n\
    fi\n\
    if test -z \"$HOST_AR\"; then\n\
       HOST_AR=\"$AR\"\n\
    fi\n\
fi\n\
\n\
GNU_AS=\n\
GNU_LD=\n\
GNU_CC=\n\
GNU_CXX=\n\
CC_VERSION=\'N/A\'\n\
CXX_VERSION=\'N/A\'\n\
if test \"$GCC\" = \"yes\"; then\n\
    GNU_CC=1\n\
    CC_VERSION=`$CC -v 2>&1 | grep \'gcc version\'`\n\
fi\n\
if test \"$GXX\" = \"yes\"; then\n\
    GNU_CXX=1\n\
    CXX_VERSION=`$CXX -v 2>&1 | grep \'gcc version\'`\n\
fi\n\
if test \"`echo | $AS -v 2>&1 | grep -c GNU`\" != \"0\"; then\n\
    GNU_AS=1\n\
fi\n\
if test \"`echo | $LD -v 2>&1 | grep -c GNU`\" != \"0\"; then\n\
    GNU_LD=1\n\
fi\n\
if test \"$GNU_CC\"; then\n\
    if `$CC -print-prog-name=ld` -v 2>&1 | grep -c GNU >/dev/null; then\n\
        GCC_USE_GNU_LD=1\n\
    fi\n\
fi\n\
\n\
dnl Special win32 checks\n\
dnl ========================================================\n\
case \"$target\" in\n\
*-wince)\n\
    WINVER=400\n\
    ;;\n\
*)\n\
    if test -n \"$GNU_CC\"; then  \n\
        WINVER=501\n\
    else    \n\
        WINVER=500\n\
    fi\n\
    ;;\n\
esac\n\
\n\
MOZ_ARG_WITH_STRING(windows-version,\n\
[  --with-windows-version=WINVER\n\
                          Minimum Windows version (WINVER) to support\n\
                              400: Windows 95\n\
                              500: Windows 2000\n\
                              501: Windows XP],\n\
  WINVER=$withval)\n\
\n\
case \"$WINVER\" in\n\
400|500|501)\n\
    ;;\n\
\n\
*)\n\
    AC_MSG_ERROR([Invalid value --with-windows-version, must be 400, 500 or 501]);\n\
    ;;\n\
\n\
esac\n\
\n\
case \"$target\" in\n\
*-cygwin*|*-mingw*|*-msvc*|*-mks*|*-wince)\n\
    if test \"$GCC\" != \"yes\"; then\n\
        # Check to see if we are really running in a msvc environemnt\n\
        _WIN32_MSVC=1\n\
        AC_CHECK_PROGS(MIDL, midl)\n\
\n\
        # Make sure compilers are valid\n\
        CFLAGS=\"$CFLAGS -TC -nologo\"\n\
        CXXFLAGS=\"$CXXFLAGS -TP -nologo\"\n\
        AC_LANG_SAVE\n\
        AC_LANG_C\n\
        AC_TRY_COMPILE([#include <stdio.h>],\n\
            [ printf(\"Hello World\\n\"); ],,\n\
            AC_MSG_ERROR([\\$(CC) test failed.  You must have MS VC++ in your path to build.]) )\n\
\n\
        AC_LANG_CPLUSPLUS\n\
        AC_TRY_COMPILE([#include <new.h>],\n\
            [ unsigned *test = new unsigned(42); ],,\n\
            AC_MSG_ERROR([\\$(CXX) test failed.  You must have MS VC++ in your path to build.]) )\n\
        AC_LANG_RESTORE\n\
        \n\
        changequote(,)\n\
        _MSVC_VER_FILTER=\'s|.* \\([0-9][0-9]*\\.[0-9][0-9]*\\.[0-9][0-9]*\\).*|\\1|p\'\n\
        changequote([,])\n\
        \n\
        # Determine compiler version\n\
        CC_VERSION=`\"${CC}\" -v 2>&1 | sed -ne \"$_MSVC_VER_FILTER\"`\n\
        _CC_MAJOR_VERSION=`echo ${CC_VERSION} | $AWK -F\\. \'{ print $1 }\'`\n\
        _CC_MINOR_VERSION=`echo ${CC_VERSION} | $AWK -F\\. \'{ print $2 }\'`\n\
        _MSC_VER=${_CC_MAJOR_VERSION}${_CC_MINOR_VERSION}\n\
\n\
        CXX_VERSION=`\"${CXX}\" -v 2>&1 | sed -ne \"$_MSVC_VER_FILTER\"`\n\
        _CXX_MAJOR_VERSION=`echo ${CXX_VERSION} | $AWK -F\\. \'{ print $1 }\'`\n\
\n\
        if test \"$_CC_MAJOR_VERSION\" != \"$_CXX_MAJOR_VERSION\"; then\n\
            AC_MSG_ERROR([The major versions of \\$CC and \\$CXX do not match.])\n\
        fi\n\
        if test \"$_CC_MAJOR_VERSION\" = \"12\"; then\n\
            _CC_SUITE=6\n\
        elif test \"$_CC_MAJOR_VERSION\" = \"13\"; then\n\
            _CC_SUITE=7\n\
        elif test \"$_CC_MAJOR_VERSION\" = \"14\"; then\n\
            _CC_SUITE=8\n\
            CXXFLAGS=\"$CXXFLAGS -Zc:wchar_t-\"\n\
            AC_DEFINE(_CRT_SECURE_NO_DEPRECATE)\n\
            AC_DEFINE(_CRT_NONSTDC_NO_DEPRECATE)\n\
        elif test \"$_CC_MAJOR_VERSION\" = \"15\"; then\n\
            _CC_SUITE=9\n\
            CXXFLAGS=\"$CXXFLAGS -Zc:wchar_t-\"\n\
            AC_DEFINE(_CRT_SECURE_NO_WARNINGS)\n\
            AC_DEFINE(_CRT_NONSTDC_NO_WARNINGS)\n\
        else\n\
            AC_MSG_ERROR([This version of the MSVC compiler, $CC_VERSION , is unsupported.])\n\
        fi\n\
\n\
        _MOZ_RTTI_FLAGS_ON=\'-GR\'\n\
        _MOZ_RTTI_FLAGS_OFF=\'-GR-\'\n\
        _MOZ_EXCEPTIONS_FLAGS_ON=\'-EHsc\'\n\
        _MOZ_EXCEPTIONS_FLAGS_OFF=\'\'\n\
\n\
        if test -n \"$WIN32_REDIST_DIR\"; then\n\
            WIN32_REDIST_DIR=`cd \"$WIN32_REDIST_DIR\" && pwd`\n\
        fi\n\
	\n\
        # bug #249782\n\
        # ensure that mt.exe is Microsoft (R) Manifest Tool and not magnetic tape manipulation utility (or something else)\n\
        if test \"$_CC_SUITE\" -ge \"8\"; then\n\
                MSMT_TOOL=`mt 2>&1|grep \'Microsoft (R) Manifest Tool\'`\n\
                if test -n \"MSMT_TOOL\"; then\n\
                        MSMANIFEST_TOOL_VERSION=`echo ${MSMANIFEST_TOOL}|grep -Po \"(^|\\s)[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?(\\s|$)\"`\n\
                        if test -z \"MSMANIFEST_TOOL_VERSION\"; then\n\
                                AC_MSG_WARN([Unknown version of the Microsoft (R) Manifest Tool.])\n\
                        fi\n\
                        MSMANIFEST_TOOL=1\n\
                        unset MSMT_TOOL\n\
                else\n\
                        AC_MSG_ERROR([Microsoft (R) Manifest Tool must be in your \\$PATH.])\n\
                fi\n\
        fi\n\
\n\
        # Check linker version\n\
        _LD_FULL_VERSION=`\"${LD}\" -v 2>&1 | sed -ne \"$_MSVC_VER_FILTER\"`\n\
        _LD_MAJOR_VERSION=`echo ${_LD_FULL_VERSION} | $AWK -F\\. \'{ print $1 }\'`\n\
        if test \"$_LD_MAJOR_VERSION\" != \"$_CC_SUITE\"; then\n\
            AC_MSG_ERROR([The linker major version, $_LD_FULL_VERSION,  does not match the compiler suite version, $_CC_SUITE.])\n\
        fi\n\
        INCREMENTAL_LINKER=1\n\
\n\
        # Check midl version\n\
        _MIDL_FULL_VERSION=`\"${MIDL}\" -v 2>&1 | sed -ne \"$_MSVC_VER_FILTER\"`\n\
        _MIDL_MAJOR_VERSION=`echo ${_MIDL_FULL_VERSION} | $AWK -F\\. \'{ print $1 }\'`\n\
        _MIDL_MINOR_VERSION=`echo ${_MIDL_FULL_VERSION} | $AWK -F\\. \'{ print $2 }\'`\n\
        _MIDL_REV_VERSION=`echo ${_MIDL_FULL_VERSION} | $AWK -F\\. \'{ print $3 }\'`\n\
         # Add flags if necessary\n\
         AC_MSG_CHECKING([for midl flags])\n\
         if test \\( \"$_MIDL_MAJOR_VERSION\" -gt \"6\" \\) -o \\( \"$_MIDL_MAJOR_VERSION\" = \"6\" -a \"$_MIDL_MINOR_VERSION\" -gt \"0\" \\) -o \\( \"$_MIDL_MAJOR_VERSION\" = \"6\" -a \"$_MIDL_MINOR_VERSION\" = \"00\" -a \"$_MIDL_REV_VERSION\" -gt \"359\" \\); then\n\
             # Starting with MIDL version 6.0.359, the MIDL compiler\n\
             # generates /Oicf /robust stubs by default, which is not\n\
             # compatible with versions of Windows older than Win2k.\n\
             # This switches us back to the old behaviour. When we drop\n\
             # support for Windows older than Win2k, we should remove\n\
             # this.\n\
             MIDL_FLAGS=\"${MIDL_FLAGS} -no_robust\"\n\
             AC_MSG_RESULT([need -no_robust])\n\
         else\n\
             MIDL_FLAGS=\"${MIDL_FLAGS}\"\n\
             AC_MSG_RESULT([none needed])\n\
        fi\n\
        \n\
        unset _MSVC_VER_FILTER\n\
        \n\
    else\n\
        # Check w32api version\n\
        _W32API_MAJOR_VERSION=`echo $W32API_VERSION | $AWK -F\\. \'{ print $1 }\'`\n\
        _W32API_MINOR_VERSION=`echo $W32API_VERSION | $AWK -F\\. \'{ print $2 }\'`\n\
        AC_MSG_CHECKING([for w32api version >= $W32API_VERSION])\n\
        AC_TRY_COMPILE([#include <w32api.h>],\n\
            #if (__W32API_MAJOR_VERSION < $_W32API_MAJOR_VERSION) || \\\n\
                (__W32API_MAJOR_VERSION == $_W32API_MAJOR_VERSION && \\\n\
                 __W32API_MINOR_VERSION < $_W32API_MINOR_VERSION)\n\
                #error \"test failed.\"\n\
            #endif\n\
            , [ res=yes ], [ res=no ])\n\
        AC_MSG_RESULT([$res])\n\
        if test \"$res\" != \"yes\"; then\n\
            AC_MSG_ERROR([w32api version $W32API_VERSION or higher required.])\n\
        fi\n\
        # Check windres version\n\
        AC_MSG_CHECKING([for windres version >= $WINDRES_VERSION])\n\
        _WINDRES_VERSION=`${WINDRES} --version 2>&1 | grep -i windres 2>/dev/null | $AWK \'{ print $3 }\'`\n\
        AC_MSG_RESULT([$_WINDRES_VERSION])\n\
        _WINDRES_MAJOR_VERSION=`echo $_WINDRES_VERSION | $AWK -F\\. \'{ print $1 }\'`\n\
        _WINDRES_MINOR_VERSION=`echo $_WINDRES_VERSION | $AWK -F\\. \'{ print $2 }\'`\n\
        _WINDRES_RELEASE_VERSION=`echo $_WINDRES_VERSION | $AWK -F\\. \'{ print $3 }\'`\n\
        WINDRES_MAJOR_VERSION=`echo $WINDRES_VERSION | $AWK -F\\. \'{ print $1 }\'`\n\
        WINDRES_MINOR_VERSION=`echo $WINDRES_VERSION | $AWK -F\\. \'{ print $2 }\'`\n\
        WINDRES_RELEASE_VERSION=`echo $WINDRES_VERSION | $AWK -F\\. \'{ print $3 }\'`\n\
        if test \"$_WINDRES_MAJOR_VERSION\" -lt \"$WINDRES_MAJOR_VERSION\" -o \\\n\
                \"$_WINDRES_MAJOR_VERSION\" -eq \"$WINDRES_MAJOR_VERSION\" -a \\\n\
                \"$_WINDRES_MINOR_VERSION\" -lt \"$WINDRES_MINOR_VERSION\" -o \\\n\
                \"$_WINDRES_MAJOR_VERSION\" -eq \"$WINDRES_MAJOR_VERSION\" -a \\\n\
                \"$_WINDRES_MINOR_VERSION\" -eq \"$WINDRES_MINOR_VERSION\" -a \\\n\
                \"$_WINDRES_RELEASE_VERSION\" -lt \"$WINDRES_RELEASE_VERSION\"\n\
        then\n\
            AC_MSG_ERROR([windres version $WINDRES_VERSION or higher is required to build.])\n\
        fi\n\
    fi # !GNU_CC\n\
\n\
    AC_DEFINE_UNQUOTED(WINVER,0x$WINVER)\n\
    AC_DEFINE_UNQUOTED(_WIN32_WINNT,0x$WINVER)\n\
    # Require OS features provided by IE 4.0\n\
    AC_DEFINE_UNQUOTED(_WIN32_IE,0x0400)\n\
    ;;\n\
esac\n\
\n\
if test -n \"$_WIN32_MSVC\"; then\n\
    SKIP_PATH_CHECKS=1\n\
    SKIP_COMPILER_CHECKS=1\n\
    SKIP_LIBRARY_CHECKS=1\n\
    AC_CHECK_HEADERS(mmintrin.h)\n\
fi\n\
\n\
dnl Test breaks icc on OS/2 && MSVC\n\
if test \"$CC\" != \"icc\" -a -z \"$_WIN32_MSVC\"; then\n\
    AC_PROG_CC_C_O\n\
    if grep \"NO_MINUS_C_MINUS_O 1\" ./confdefs.h >/dev/null; then\n\
        USING_HCC=1\n\
        _OLDCC=$CC\n\
        _OLDCXX=$CXX\n\
        CC=\"${srcdir}/build/hcc \'$CC\'\"\n\
        CXX=\"${srcdir}/build/hcpp \'$CXX\'\"\n\
    fi\n\
fi\n\
\n\
AC_PROG_CPP\n\
AC_PROG_CXXCPP\n\
\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
AC_SUBST(MIDL_FLAGS)\n\
AC_SUBST(_MSC_VER)\n\
\n\
AC_SUBST(GNU_AS)\n\
AC_SUBST(GNU_LD)\n\
AC_SUBST(GNU_CC)\n\
AC_SUBST(GNU_CXX)\n\
\n\
dnl ========================================================\n\
dnl Checks for programs.\n\
dnl ========================================================\n\
AC_PROG_INSTALL\n\
AC_PROG_LN_S\n\
AC_PATH_PROGS(PERL, $PERL perl5 perl )\n\
if test -z \"$PERL\" || test \"$PERL\" = \":\"; then\n\
    AC_MSG_ERROR([perl not found in \\$PATH])\n\
fi\n\
\n\
if test -z \"$TINDERBOX_SKIP_PERL_VERSION_CHECK\"; then\n\
AC_MSG_CHECKING([for minimum required perl version >= $PERL_VERSION])\n\
_perl_version=`PERL_VERSION=$PERL_VERSION $PERL -e \'print \"$]\"; if ($] >= $ENV{PERL_VERSION}) { exit(0); } else { exit(1); }\' 2>&5`\n\
_perl_res=$?\n\
AC_MSG_RESULT([$_perl_version])\n\
\n\
if test \"$_perl_res\" != 0; then\n\
    AC_MSG_ERROR([Perl $PERL_VERSION or higher is required.])\n\
fi\n\
fi\n\
\n\
AC_MSG_CHECKING([for full perl installation])\n\
_perl_archlib=`$PERL -e \'use Config; if ( -d $Config{archlib} ) { exit(0); } else { exit(1); }\' 2>&5`\n\
_perl_res=$?\n\
if test \"$_perl_res\" != 0; then\n\
    AC_MSG_RESULT([no])\n\
    AC_MSG_ERROR([Cannot find Config.pm or \\$Config{archlib}.  A full perl installation is required.])\n\
else\n\
    AC_MSG_RESULT([yes])    \n\
fi\n\
\n\
AC_PATH_PROGS(PYTHON, $PYTHON python)\n\
if test -z \"$PYTHON\"; then\n\
    AC_MSG_ERROR([python was not found in \\$PATH])\n\
fi\n\
echo PYTHON=\"$PYTHON\"\n\
\n\
AC_PATH_PROG(DOXYGEN, doxygen, :)\n\
AC_PATH_PROG(WHOAMI, whoami, :)\n\
AC_PATH_PROG(AUTOCONF, autoconf, :)\n\
AC_PATH_PROG(UNZIP, unzip, :)\n\
AC_PATH_PROGS(ZIP, zip)\n\
if test -z \"$ZIP\" || test \"$ZIP\" = \":\"; then\n\
    AC_MSG_ERROR([zip not found in \\$PATH])\n\
fi\n\
AC_PATH_PROG(SYSTEM_MAKEDEPEND, makedepend)\n\
AC_PATH_PROG(XARGS, xargs)\n\
if test -z \"$XARGS\" || test \"$XARGS\" = \":\"; then\n\
    AC_MSG_ERROR([xargs not found in \\$PATH .])\n\
fi\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
\n\
dnl ========================================================\n\
dnl = Mac OS X toolchain support\n\
dnl ========================================================\n\
\n\
case \"$target_os\" in\n\
darwin*)\n\
    dnl Current known valid versions for GCC_VERSION are 2.95.2 3.1 3.3 4.0.\n\
    dnl 4.0 identifies itself as 4.0.x, so strip the decidecimal for\n\
    dnl the environment and includedir purposes (when using an SDK, below),\n\
    dnl but remember the full version number for the libdir (SDK).\n\
    changequote(,)\n\
    GCC_VERSION_FULL=`echo $CXX_VERSION | $PERL -pe \'s/^.*gcc version ([^ ]*).*/$1/\'`\n\
    GCC_VERSION=`echo $GCC_VERSION_FULL | $PERL -pe \'(split(/\\./))[0]>=4&&s/(^\\d*\\.\\d*).*/$1/;\'`\n\
    changequote([,])\n\
    if test \"$GCC_VERSION_FULL\" = \"4.0.0\" ; then\n\
        dnl Bug 280479, but this keeps popping up in bug 292530 too because\n\
        dnl 4.0.0/4061 is the default compiler in Tiger.\n\
        changequote(,)\n\
        GCC_BUILD=`echo $CXX_VERSION | $PERL -pe \'s/^.*build ([^ )]*).*/$1/\'`\n\
        changequote([,])\n\
        if test \"$GCC_BUILD\" = \"4061\" ; then\n\
            AC_MSG_ERROR([You are attempting to use Apple gcc 4.0 build 4061.\n\
This compiler was supplied with Xcode 2.0, and contains bugs that prevent it\n\
from building Mozilla.\n\
Either upgrade to Xcode 2.1 or later, or switch the system\'s default compiler\n\
to gcc 3.3 by running \\\"sudo gcc_select 3.3\\\".])\n\
        fi\n\
    fi\n\
\n\
    dnl xcodebuild needs GCC_VERSION defined in the environment, since it\n\
    dnl doesn\'t respect the CC/CXX setting.  With GCC_VERSION set, it will use\n\
    dnl /usr/bin/g(cc|++)-$GCC_VERSION.\n\
    AC_PATH_PROGS(PBBUILD, pbbuild xcodebuild pbxbuild)\n\
\n\
    case \"$PBBUILD\" in\n\
      *xcodebuild*)\n\
        changequote(,)\n\
        XCODEBUILD_VERSION=`$PBBUILD -version 2>/dev/null | sed -e \'s/.*DevToolsCore-\\([0-9]*\\).*/\\1/\'`\n\
        changequote([,])\n\
        if test -n \"$XCODEBUILD_VERSION\" && test \"$XCODEBUILD_VERSION\" -ge 620 ; then\n\
          HAS_XCODE_2_1=1;\n\
        fi\n\
      ;;\n\
    esac\n\
\n\
    dnl sdp was formerly in /Developer/Tools.  As of Mac OS X 10.4 (Darwin 8),\n\
    dnl it has moved into /usr/bin.\n\
    AC_PATH_PROG(SDP, sdp, :, [$PATH:/usr/bin:/Developer/Tools])\n\
    ;;\n\
esac\n\
\n\
AC_SUBST(GCC_VERSION)\n\
AC_SUBST(XCODEBUILD_VERSION)\n\
AC_SUBST(HAS_XCODE_2_1)\n\
\n\
dnl The universal machinery sets UNIVERSAL_BINARY to inform packager.mk\n\
dnl that a universal binary is being produced.\n\
AC_SUBST(UNIVERSAL_BINARY)\n\
\n\
dnl ========================================================\n\
dnl = Mac OS X SDK support\n\
dnl ========================================================\n\
MACOS_SDK_DIR=\n\
NEXT_ROOT=\n\
MOZ_ARG_WITH_STRING(macos-sdk,\n\
[  --with-macos-sdk=dir   Location of platform SDK to use (Mac OS X only)],\n\
    MACOS_SDK_DIR=$withval)\n\
\n\
dnl MACOS_SDK_DIR will be set to the SDK location whenever one is in use.\n\
dnl NEXT_ROOT will be set and exported only if it\'s needed.\n\
AC_SUBST(MACOS_SDK_DIR)\n\
AC_SUBST(NEXT_ROOT)\n\
\n\
if test \"$MACOS_SDK_DIR\"; then\n\
  dnl Sync this section with the ones in NSPR and NSS.\n\
  dnl Changes to the cross environment here need to be accounted for in\n\
  dnl the libIDL checks (below) and xpidl build.\n\
\n\
  if test ! -d \"$MACOS_SDK_DIR\"; then\n\
    AC_MSG_ERROR([SDK not found.  When using --with-macos-sdk, you must\n\
specify a valid SDK.  SDKs are installed when the optional cross-development\n\
tools are selected during the Xcode/Developer Tools installation.])\n\
  fi\n\
\n\
  GCC_VERSION_MAJOR=`echo $GCC_VERSION_FULL | $PERL -pe \'s/(^\\d*).*/$1/;\'`\n\
  if test \"$GCC_VERSION_MAJOR\" -lt \"4\" ; then\n\
    SDK_C_INCLUDE=\"-isystem ${MACOS_SDK_DIR}/usr/include/gcc/darwin/${GCC_VERSION} -isystem ${MACOS_SDK_DIR}/usr/include -F${MACOS_SDK_DIR}/System/Library/Frameworks\"\n\
    if test -d \"${MACOS_SDK_DIR}/Library/Frameworks\" ; then\n\
      SDK_C_INCLUDE=\"$SDK_C_INCLUDE -F${MACOS_SDK_DIR}/Library/Frameworks\"\n\
    fi\n\
    SDK_CXX_INCLUDE=\"-I${MACOS_SDK_DIR}/usr/include/gcc/darwin/${GCC_VERSION}/c++ -I${MACOS_SDK_DIR}/usr/include/gcc/darwin/${GCC_VERSION}/c++/ppc-darwin -I${MACOS_SDK_DIR}/usr/include/gcc/darwin/${GCC_VERSION}/c++/backward\"\n\
\n\
    CFLAGS=\"$CFLAGS -nostdinc ${SDK_C_INCLUDE}\"\n\
    CXXFLAGS=\"$CXXFLAGS -nostdinc -nostdinc++ ${SDK_CXX_INCLUDE} ${SDK_C_INCLUDE}\"\n\
\n\
    dnl CPP/CXXCPP needs to be set for AC_CHECK_HEADER.\n\
    CPP=\"$CPP -nostdinc ${SDK_C_INCLUDE}\"\n\
    CXXCPP=\"$CXXCPP -nostdinc -nostdinc++ ${SDK_CXX_INCLUDE} ${SDK_C_INCLUDE}\"\n\
\n\
    dnl ld support for -syslibroot is compiler-agnostic, but only available\n\
    dnl on Tiger.  Although it\'s possible to switch on the build host\'s\n\
    dnl OS release to use ld -syslibroot when available, ld -syslibroot will\n\
    dnl cause warnings as long as NEXT_ROOT is set.  NEXT_ROOT should be\n\
    dnl set because both the compiler and linker use it.\n\
    LIBS=\"-L${MACOS_SDK_DIR}/usr/lib/gcc/darwin -L${MACOS_SDK_DIR}/usr/lib/gcc/darwin/${GCC_VERSION_FULL} -L${MACOS_SDK_DIR}/usr/lib $LIBS\"\n\
    export NEXT_ROOT=$MACOS_SDK_DIR\n\
\n\
    if test -n \"$CROSS_COMPILE\" ; then\n\
      dnl NEXT_ROOT will be in the environment, but it shouldn\'t be set for\n\
      dnl the build host.  HOST_CXX is presently unused.\n\
      HOST_CC=\"NEXT_ROOT= $HOST_CC\"\n\
      HOST_CXX=\"NEXT_ROOT= $HOST_CXX\"\n\
    fi\n\
  else\n\
    dnl gcc >= 4.0 uses different paths than above, but knows how to find\n\
    dnl them itself.\n\
    CFLAGS=\"$CFLAGS -isysroot ${MACOS_SDK_DIR}\"\n\
    CXXFLAGS=\"$CXXFLAGS -isysroot ${MACOS_SDK_DIR}\"\n\
\n\
    dnl CPP/CXXCPP needs to be set for AC_CHECK_HEADER.\n\
    CPP=\"$CPP -isysroot ${MACOS_SDK_DIR}\"\n\
    CXXCPP=\"$CXXCPP -isysroot ${MACOS_SDK_DIR}\"\n\
\n\
    if test \"$GCC_VERSION_FULL\" = \"4.0.0\" ; then\n\
      dnl If gcc >= 4.0, we\'re guaranteed to be on Tiger, which has an ld\n\
      dnl that supports -syslibroot.  Don\'t set NEXT_ROOT because it will\n\
      dnl be ignored and cause warnings when -syslibroot is specified.\n\
      dnl gcc 4.0.1 will pass -syslibroot to ld automatically based on\n\
      dnl the -isysroot it receives, so this is only needed with 4.0.0.\n\
      LDFLAGS=\"$LDFLAGS -Wl,-syslibroot,${MACOS_SDK_DIR}\"\n\
    fi\n\
  fi\n\
\n\
  AC_LANG_SAVE\n\
  AC_MSG_CHECKING([for valid compiler/Mac OS X SDK combination])\n\
  AC_LANG_CPLUSPLUS\n\
  AC_TRY_COMPILE([#include <new>\n\
                 int main() { return 0; }],\n\
   result=yes,\n\
   result=no)\n\
  AC_LANG_RESTORE\n\
  AC_MSG_RESULT($result)\n\
\n\
  if test \"$result\" = \"no\" ; then\n\
    AC_MSG_ERROR([The selected compiler and Mac OS X SDK are incompatible.])\n\
  fi\n\
fi\n\
\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl Be sure the make we use is GNU make.\n\
dnl on win32, gmake.exe is the generally the wrong version\n\
case \"$host_os\" in\n\
cygwin*|mingw*|mks*|msvc*)\n\
    AC_PATH_PROGS(MAKE, $MAKE make gmake, :)\n\
    ;;\n\
*)\n\
    AC_PATH_PROGS(MAKE, $MAKE gmake make, :)\n\
    ;;\n\
esac\n\
_make_try=`$MAKE --version 2>/dev/null | grep GNU`\n\
if test ! \"$_make_try\"\n\
then\n\
	echo\n\
	echo \"*** $MAKE is not GNU Make.  You will not be able to build Mozilla without GNU Make.\"\n\
	echo\n\
	exit 1\n\
fi\n\
dnl Now exit if version if < MAKE_VERSION\n\
rm -f dummy.mk\n\
echo \'all: ; @echo $(MAKE_VERSION)\' > dummy.mk\n\
_make_vers=`$MAKE --no-print-directory -f dummy.mk all 2>/dev/null`\n\
rm -f dummy.mk\n\
_MAKE_MAJOR_VERSION=`echo $_make_vers | $AWK -F\\. \'{ print $1 }\'`\n\
_MAKE_MINOR_VERSION=`echo $_make_vers | $AWK -F\\. \'{ print $2 }\'`\n\
MAKE_MAJOR_VERSION=`echo $MAKE_VERSION | $AWK -F\\. \'{ print $1 }\'`\n\
MAKE_MINOR_VERSION=`echo $MAKE_VERSION | $AWK -F\\. \'{ print $2 }\'`\n\
if test \"$_MAKE_MAJOR_VERSION\" -lt \"$MAKE_MAJOR_VERSION\" || \\\n\
   test \"$_MAKE_MAJOR_VERSION\" = \"$MAKE_MAJOR_VERSION\" -a \\\n\
        \"$_MAKE_MINOR_VERSION\" -lt \"$MAKE_MINOR_VERSION\"; then\n\
   AC_MSG_ERROR([GNU Make $MAKE_VERSION or higher is required to build Mozilla.])\n\
fi\n\
AC_SUBST(MAKE)\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
\n\
AC_PATH_XTRA\n\
\n\
dnl Check in X11 include directory too.\n\
if test \"$no_x\" != \"yes\"; then\n\
    CPPFLAGS=\"$CPPFLAGS $X_CFLAGS\"\n\
fi\n\
\n\
XCFLAGS=\"$X_CFLAGS\"\n\
\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl ========================================================\n\
dnl set the defaults first\n\
dnl ========================================================\n\
AS_BIN=$AS\n\
AR_FLAGS=\'cr $@\'\n\
AR_LIST=\'$(AR) t\'\n\
AR_EXTRACT=\'$(AR) x\'\n\
AR_DELETE=\'$(AR) d\'\n\
AS=\'$(CC)\'\n\
AS_DASH_C_FLAG=\'-c\'\n\
DLL_PREFIX=lib\n\
LIB_PREFIX=lib\n\
DLL_SUFFIX=.so\n\
OBJ_SUFFIX=o\n\
LIB_SUFFIX=a\n\
ASM_SUFFIX=s\n\
IMPORT_LIB_SUFFIX=\n\
TARGET_MD_ARCH=unix\n\
DIRENT_INO=d_ino\n\
CYGWIN_WRAPPER=\n\
WIN_TOP_SRC=\n\
MOZ_USER_DIR=\".mozilla\"\n\
HOST_AR=\'$(AR)\'\n\
HOST_AR_FLAGS=\'$(AR_FLAGS)\'\n\
\n\
MOZ_JPEG_CFLAGS=\n\
MOZ_JPEG_LIBS=\'$(call EXPAND_LIBNAME_PATH,mozjpeg,$(DEPTH)/jpeg)\'\n\
MOZ_ZLIB_CFLAGS=\n\
MOZ_ZLIB_LIBS=\'$(call EXPAND_LIBNAME_PATH,mozz,$(DEPTH)/modules/zlib/src)\'\n\
MOZ_PNG_CFLAGS=\n\
MOZ_PNG_LIBS=\'$(call EXPAND_LIBNAME_PATH,mozpng,$(DEPTH)/modules/libimg/png)\'\n\
\n\
MOZ_JS_LIBS=\'-L$(LIBXUL_DIST)/bin -lmozjs\'\n\
DYNAMIC_XPCOM_LIBS=\'-L$(LIBXUL_DIST)/bin -lxpcom -lxpcom_core\'\n\
MOZ_FIX_LINK_PATHS=\'-Wl,-rpath-link,$(LIBXUL_DIST)/bin\'\n\
XPCOM_FROZEN_LDOPTS=\'-L$(LIBXUL_DIST)/bin $(MOZ_FIX_LINK_PATHS) -lxpcom\'\n\
LIBXUL_LIBS=\'$(XPCOM_FROZEN_LDOPTS) -lxul\'\n\
XPCOM_GLUE_LDOPTS=\'$(LIBXUL_DIST)/lib/$(LIB_PREFIX)xpcomglue_s.$(LIB_SUFFIX) $(XPCOM_FROZEN_LDOPTS)\'\n\
XPCOM_STANDALONE_GLUE_LDOPTS=\'$(LIBXUL_DIST)/lib/$(LIB_PREFIX)xpcomglue.$(LIB_SUFFIX)\'\n\
\n\
MOZ_COMPONENT_NSPR_LIBS=\'-L$(LIBXUL_DIST)/bin $(NSPR_LIBS)\'\n\
MOZ_XPCOM_OBSOLETE_LIBS=\'-L$(LIBXUL_DIST)/lib -lxpcom_compat\'\n\
\n\
USE_DEPENDENT_LIBS=1\n\
\n\
_PLATFORM_DEFAULT_TOOLKIT=cairo-gtk2\n\
MOZ_GFX_TOOLKIT=\'$(MOZ_WIDGET_TOOLKIT)\'\n\
\n\
MOZ_ENABLE_POSTSCRIPT=1 \n\
\n\
if test -n \"$CROSS_COMPILE\"; then\n\
    OS_TARGET=\"${target_os}\"\n\
    OS_ARCH=`echo $target_os | sed -e \'s|/|_|g\'`\n\
    OS_RELEASE=\n\
    OS_TEST=\"${target_cpu}\"\n\
    case \"${target_os}\" in\n\
        linux*)       OS_ARCH=Linux ;;\n\
        solaris*)     OS_ARCH=SunOS OS_RELEASE=5 ;;\n\
        mingw*)       OS_ARCH=WINNT ;;\n\
        wince*)       OS_ARCH=WINCE ;;\n\
        darwin*)      OS_ARCH=Darwin OS_TARGET=Darwin ;;\n\
    esac\n\
else\n\
    OS_TARGET=`uname -s`\n\
    OS_ARCH=`uname -s | sed -e \'s|/|_|g\'`\n\
    OS_RELEASE=`uname -r`\n\
    OS_TEST=`uname -m`\n\
fi\n\
_COMPILER_PREFIX=\n\
\n\
HOST_OS_ARCH=`echo $host_os | sed -e \'s|/|_|g\'`\n\
\n\
#######################################################################\n\
# Master \"Core Components\" macros for getting the OS target           #\n\
#######################################################################\n\
\n\
#\n\
# Note: OS_TARGET should be specified on the command line for gmake.\n\
# When OS_TARGET=WIN95 is specified, then a Windows 95 target is built.\n\
# The difference between the Win95 target and the WinNT target is that\n\
# the WinNT target uses Windows NT specific features not available\n\
# in Windows 95. The Win95 target will run on Windows NT, but (supposedly)\n\
# at lesser performance (the Win95 target uses threads; the WinNT target\n\
# uses fibers).\n\
#\n\
# When OS_TARGET=WIN16 is specified, then a Windows 3.11 (16bit) target\n\
# is built. See: win16_3.11.mk for lots more about the Win16 target.\n\
#\n\
# If OS_TARGET is not specified, it defaults to $(OS_ARCH), i.e., no\n\
# cross-compilation.\n\
#\n\
\n\
#\n\
# The following hack allows one to build on a WIN95 machine (as if\n\
# s/he were cross-compiling on a WINNT host for a WIN95 target).\n\
# It also accomodates for MKS\'s uname.exe.  If you never intend\n\
# to do development on a WIN95 machine, you don\'t need this hack.\n\
#\n\
case \"$OS_ARCH\" in\n\
WIN95)\n\
    OS_ARCH=WINNT\n\
    OS_TARGET=WIN95\n\
    ;;\n\
Windows_95)\n\
    OS_ARCH=Windows_NT\n\
    OS_TARGET=WIN95\n\
    ;;\n\
Windows_98)\n\
    OS_ARCH=Windows_NT\n\
    OS_TARGET=WIN95\n\
    ;;\n\
CYGWIN_9*|CYGWIN_ME*)\n\
    OS_ARCH=\'CYGWIN_NT-4.0\'\n\
    OS_TARGET=WIN95\n\
    ;;\n\
esac\n\
\n\
#\n\
# Define and override various archtecture-specific variables, including\n\
# HOST_OS_ARCH\n\
# OS_ARCH\n\
# OS_TEST\n\
# OS_TARGET\n\
# OS_RELEASE\n\
# OS_MINOR_RELEASE\n\
#\n\
\n\
case \"$HOST_OS_ARCH\" in\n\
cygwin*|mingw*|mks*|msvc*)\n\
    HOST_OS_ARCH=WINNT\n\
    ;;\n\
linux*)\n\
    HOST_OS_ARCH=Linux\n\
    ;;\n\
solaris*)\n\
    HOST_OS_ARCH=SunOS\n\
    ;;\n\
BSD_386)\n\
    HOST_OS_ARCH=BSD\n\
    ;;\n\
dgux)\n\
    HOST_OS_ARCH=DGUX\n\
    ;;\n\
IRIX64)\n\
    HOST_OS_ARCH=IRIX\n\
    ;;\n\
UNIX_SV)\n\
    if \"`cat /etc/bcheckrc | grep -c NCR 2>/dev/null`\" != \"0\"; then\n\
        HOST_OS_ARCH=NCR\n\
    else\n\
        HOST_OS_ARCH=UNIXWARE\n\
    fi\n\
    ;;\n\
ncr)\n\
    HOST_OS_ARCH=NCR\n\
    ;;\n\
UNIX_SYSTEM_V)\n\
    HOST_OS_ARCH=NEC\n\
    ;;\n\
OSF1)\n\
    ;;\n\
*OpenVMS*)\n\
    HOST_OS_ARCH=OpenVMS\n\
    ;;\n\
OS_2)\n\
    HOST_OS_ARCH=OS2\n\
    ;;\n\
QNX)\n\
    ;;\n\
SCO_SV)\n\
    HOST_OS_ARCH=SCOOS\n\
    ;;\n\
SINIX-N | SINIX-Y | SINIX-Z |ReliantUNIX-M)\n\
    HOST_OS_ARCH=SINIX\n\
    ;;\n\
UnixWare)\n\
    HOST_OS_ARCH=UNIXWARE\n\
    ;;\n\
esac\n\
\n\
case \"$OS_ARCH\" in\n\
WINNT)\n\
    OS_TEST=`uname -p`\n\
    ;;\n\
Windows_NT)\n\
#\n\
# If uname -s returns \"Windows_NT\", we assume that we are using\n\
# the uname.exe in MKS toolkit.\n\
#\n\
# The -r option of MKS uname only returns the major version number.\n\
# So we need to use its -v option to get the minor version number.\n\
# Moreover, it doesn\'t have the -p option, so we need to use uname -m.\n\
#\n\
    OS_ARCH=WINNT\n\
    OS_TARGET=WINNT\n\
    OS_MINOR_RELEASE=`uname -v`\n\
    if test \"$OS_MINOR_RELEASE\" = \"00\"; then\n\
        OS_MINOR_RELEASE=0\n\
    fi\n\
    OS_RELEASE=\"${OS_RELEASE}.${OS_MINOR_RELEASE}\"\n\
    ;;\n\
CYGWIN32_NT|CYGWIN_NT*|MINGW*_NT*)\n\
#\n\
# If uname -s returns \"CYGWIN_NT-4.0\", we assume that we are using\n\
# the uname.exe in the Cygwin tools.\n\
# Prior to the Beta 20 release, Cygwin was called GNU-Win32.\n\
# If uname -s returns \"CYGWIN32/NT\", we assume that we are using\n\
# the uname.exe in the GNU-Win32 tools.\n\
# If uname -s returns MINGW32_NT-5.1, we assume that we are using\n\
# the uname.exe in the MSYS tools.\n\
#\n\
    OS_RELEASE=`expr $OS_ARCH : \'.*NT-\\(.*\\)\'`\n\
    OS_ARCH=WINNT\n\
    OS_TARGET=WINNT\n\
    ;;\n\
AIX)\n\
    OS_RELEASE=`uname -v`.`uname -r`\n\
    OS_TEST=`uname -p`\n\
    ;;\n\
BSD_386)\n\
    OS_ARCH=BSD\n\
    ;;\n\
dgux)\n\
    OS_ARCH=DGUX\n\
    ;;\n\
IRIX64)\n\
    OS_ARCH=IRIX\n\
    ;;\n\
UNIX_SV)\n\
    if \"`cat /etc/bcheckrc | grep -c NCR 2>/dev/null`\" != \"0\"; then\n\
        OS_ARCH=NCR\n\
    else\n\
        OS_ARCH=UNIXWARE\n\
        OS_RELEASE=`uname -v`\n\
    fi\n\
    ;;\n\
ncr)\n\
    OS_ARCH=NCR\n\
    ;;\n\
UNIX_SYSTEM_V)\n\
    OS_ARCH=NEC\n\
    ;;\n\
OSF1)\n\
    case `uname -v` in\n\
    148)\n\
        OS_RELEASE=V3.2C\n\
        ;;\n\
    564)\n\
        OS_RELEASE=V4.0B\n\
        ;;\n\
    878)\n\
        OS_RELEASE=V4.0D\n\
        ;;\n\
    esac\n\
    ;;\n\
*OpenVMS*)\n\
    OS_ARCH=OpenVMS\n\
    OS_RELEASE=`uname -v`\n\
    OS_TEST=`uname -p`\n\
    ;;\n\
OS_2)\n\
    OS_ARCH=OS2\n\
    OS_TARGET=OS2\n\
    OS_RELEASE=`uname -v`\n\
    ;;\n\
QNX)\n\
    if test \"$OS_TARGET\" != \"NTO\"; then\n\
        changequote(,)\n\
        OS_RELEASE=`uname -v | sed \'s/^\\([0-9]\\)\\([0-9]*\\)$/\\1.\\2/\'`\n\
        changequote([,])\n\
    fi\n\
    OS_TEST=x86\n\
    ;;\n\
SCO_SV)\n\
    OS_ARCH=SCOOS\n\
    OS_RELEASE=5.0\n\
    ;;\n\
SINIX-N | SINIX-Y | SINIX-Z |ReliantUNIX-M)\n\
    OS_ARCH=SINIX\n\
    OS_TEST=`uname -p`\n\
    ;;\n\
UnixWare)\n\
    OS_ARCH=UNIXWARE\n\
    OS_RELEASE=`uname -v`\n\
    ;;\n\
WINCE)\n\
    OS_ARCH=WINCE\n\
    OS_TARGET=WINCE\n\
    ;;\n\
Darwin)\n\
    case \"${target_cpu}\" in\n\
    powerpc*)\n\
        OS_TEST=ppc\n\
        ;;\n\
    i*86*)\n\
        OS_TEST=i386 \n\
        ;;\n\
    *)\n\
        if test -z \"$CROSS_COMPILE\" ; then\n\
            OS_TEST=`uname -p`\n\
        fi\n\
        ;;\n\
    esac\n\
    ;;\n\
esac\n\
\n\
if test \"$OS_ARCH\" = \"NCR\"; then\n\
    changequote(,)\n\
    OS_RELEASE=`awk \'{print $3}\' /etc/.relid | sed \'s/^\\([0-9]\\)\\(.\\)\\(..\\)\\(.*\\)$/\\2.\\3/\'`\n\
    changequote([,])\n\
fi\n\
\n\
# Only set CPU_ARCH if we recognize the value of OS_TEST\n\
\n\
case \"$OS_TEST\" in\n\
*86 | i86pc)\n\
    CPU_ARCH=x86\n\
    ;;\n\
\n\
powerpc* | ppc)\n\
    CPU_ARCH=ppc\n\
    ;;\n\
\n\
Alpha | alpha | ALPHA)\n\
    CPU_ARCH=Alpha\n\
    ;;\n\
\n\
sun4u)\n\
    CPU_ARCH=sparc\n\
    ;;\n\
\n\
x86_64 | sparc | ppc | ia64)\n\
    CPU_ARCH=\"$OS_TEST\"\n\
    ;;\n\
esac\n\
\n\
if test -z \"$OS_TARGET\"; then\n\
    OS_TARGET=$OS_ARCH\n\
fi\n\
if test \"$OS_TARGET\" = \"WIN95\"; then\n\
    OS_RELEASE=\"4.0\"\n\
fi\n\
if test \"$OS_TARGET\" = \"WIN16\"; then\n\
    OS_RELEASE=\n\
fi\n\
OS_CONFIG=\"${OS_TARGET}${OS_RELEASE}\"\n\
\n\
dnl ========================================================\n\
dnl GNU specific defaults\n\
dnl ========================================================\n\
if test \"$GNU_CC\"; then\n\
    MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -Wl,-h,$@ -o $@\'\n\
    MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -Wl,-h,$@ -o $@\'\n\
    DSO_LDOPTS=\'-shared\'\n\
    if test \"$GCC_USE_GNU_LD\"; then\n\
        # Don\'t allow undefined symbols in libraries\n\
        DSO_LDOPTS=\"$DSO_LDOPTS -Wl,-z,defs\"\n\
    fi\n\
    DSO_CFLAGS=\'\'\n\
    DSO_PIC_CFLAGS=\'-fPIC\'\n\
    _MOZ_RTTI_FLAGS_ON=${_COMPILER_PREFIX}-frtti\n\
    _MOZ_RTTI_FLAGS_OFF=${_COMPILER_PREFIX}-fno-rtti\n\
    _MOZ_EXCEPTIONS_FLAGS_ON=\'-fhandle-exceptions\'\n\
    _MOZ_EXCEPTIONS_FLAGS_OFF=\'-fno-handle-exceptions\'\n\
\n\
    # Turn on GNU specific features\n\
    # -Wall - turn on all warnings\n\
    # -pedantic - make compiler warn about non-ANSI stuff, and\n\
    #             be a little bit stricter\n\
    # Warnings slamm took out for now (these were giving more noise than help):\n\
    # -Wbad-function-cast - warns when casting a function to a new return type\n\
    # -Wconversion - complained when char\'s or short\'s were used a function args\n\
    # -Wshadow - removed because it generates more noise than help --pete\n\
    _WARNINGS_CFLAGS=\"${_WARNINGS_CFLAGS} -Wall -W -Wno-unused -Wpointer-arith -Wcast-align\"\n\
\n\
    dnl Turn pedantic on but disable the warnings for long long\n\
    _PEDANTIC=1\n\
    _IGNORE_LONG_LONG_WARNINGS=1\n\
\n\
    _DEFINES_CFLAGS=\'-include $(DEPTH)/mozilla-config.h -DMOZILLA_CLIENT\'\n\
    _USE_CPP_INCLUDE_FLAG=1\n\
else\n\
    MKSHLIB=\'$(LD) $(DSO_LDOPTS) -h $@ -o $@\'\n\
    MKCSHLIB=\'$(LD) $(DSO_LDOPTS) -h $@ -o $@\'\n\
\n\
    DSO_LDOPTS=\'-shared\'\n\
    if test \"$GNU_LD\"; then\n\
        # Don\'t allow undefined symbols in libraries\n\
        DSO_LDOPTS=\"$DSO_LDOPTS -z defs\"\n\
    fi\n\
\n\
    DSO_CFLAGS=\'\'\n\
    DSO_PIC_CFLAGS=\'-KPIC\'\n\
    _DEFINES_CFLAGS=\'$(ACDEFINES) -D_MOZILLA_CONFIG_H_ -DMOZILLA_CLIENT\'\n\
fi\n\
\n\
if test \"$GNU_CXX\"; then\n\
    # Turn on GNU specific features\n\
    _WARNINGS_CXXFLAGS=\"${_WARNINGS_CXXFLAGS} -Wall -Wconversion -Wpointer-arith -Wcast-align -Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor\"\n\
\n\
    _DEFINES_CXXFLAGS=\'-DMOZILLA_CLIENT -include $(DEPTH)/mozilla-config.h\'\n\
    _USE_CPP_INCLUDE_FLAG=1\n\
else\n\
    _DEFINES_CXXFLAGS=\'-DMOZILLA_CLIENT -D_MOZILLA_CONFIG_H_ $(ACDEFINES)\'\n\
fi\n\
\n\
dnl gcc can come with its own linker so it is better to use the pass-thru calls\n\
dnl MKSHLIB_FORCE_ALL is used to force the linker to include all object\n\
dnl files present in an archive. MKSHLIB_UNFORCE_ALL reverts the linker to\n\
dnl normal behavior.\n\
dnl ========================================================\n\
MKSHLIB_FORCE_ALL=\n\
MKSHLIB_UNFORCE_ALL=\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test \"$GNU_CC\"; then\n\
  AC_MSG_CHECKING(whether ld has archive extraction flags)\n\
  AC_CACHE_VAL(ac_cv_mkshlib_force_and_unforce,\n\
   [_SAVE_LDFLAGS=$LDFLAGS; _SAVE_LIBS=$LIBS\n\
    ac_cv_mkshlib_force_and_unforce=\"no\"\n\
    exec 3<&0 <<LOOP_INPUT\n\
	force=\"-Wl,--whole-archive\";   unforce=\"-Wl,--no-whole-archive\"\n\
	force=\"-Wl,-z -Wl,allextract\"; unforce=\"-Wl,-z -Wl,defaultextract\"\n\
	force=\"-Wl,-all\";              unforce=\"-Wl,-none\"\n\
LOOP_INPUT\n\
    while read line\n\
    do\n\
      eval $line\n\
      LDFLAGS=$force\n\
      LIBS=$unforce\n\
      AC_TRY_LINK(,, ac_cv_mkshlib_force_and_unforce=$line; break)\n\
    done\n\
    exec 0<&3 3<&-\n\
    LDFLAGS=$_SAVE_LDFLAGS; LIBS=$_SAVE_LIBS\n\
   ])\n\
  if test \"$ac_cv_mkshlib_force_and_unforce\" = \"no\"; then\n\
    AC_MSG_RESULT(no)\n\
  else\n\
    AC_MSG_RESULT(yes)\n\
    eval $ac_cv_mkshlib_force_and_unforce\n\
    MKSHLIB_FORCE_ALL=$force\n\
    MKSHLIB_UNFORCE_ALL=$unforce\n\
  fi\n\
fi # GNU_CC\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl =================================================================\n\
dnl Set up and test static assertion macros used to avoid AC_TRY_RUN,\n\
dnl which is bad when cross compiling.\n\
dnl =================================================================\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
configure_static_assert_macros=\'\n\
#define CONFIGURE_STATIC_ASSERT(condition) CONFIGURE_STATIC_ASSERT_IMPL(condition, __LINE__)\n\
#define CONFIGURE_STATIC_ASSERT_IMPL(condition, line) CONFIGURE_STATIC_ASSERT_IMPL2(condition, line)\n\
#define CONFIGURE_STATIC_ASSERT_IMPL2(condition, line) typedef int static_assert_line_##line[(condition) ? 1 : -1]\n\
\'\n\
\n\
dnl test that the macros actually work:\n\
AC_MSG_CHECKING(that static assertion macros used in autoconf tests work)\n\
AC_CACHE_VAL(ac_cv_static_assertion_macros_work,\n\
 [AC_LANG_SAVE\n\
  AC_LANG_C\n\
  ac_cv_static_assertion_macros_work=\"yes\"\n\
  AC_TRY_COMPILE([$configure_static_assert_macros],\n\
                 [CONFIGURE_STATIC_ASSERT(1)],\n\
                 ,\n\
                 ac_cv_static_assertion_macros_work=\"no\")\n\
  AC_TRY_COMPILE([$configure_static_assert_macros],\n\
                 [CONFIGURE_STATIC_ASSERT(0)],\n\
                 ac_cv_static_assertion_macros_work=\"no\",\n\
                 )\n\
  AC_LANG_CPLUSPLUS\n\
  AC_TRY_COMPILE([$configure_static_assert_macros],\n\
                 [CONFIGURE_STATIC_ASSERT(1)],\n\
                 ,\n\
                 ac_cv_static_assertion_macros_work=\"no\")\n\
  AC_TRY_COMPILE([$configure_static_assert_macros],\n\
                 [CONFIGURE_STATIC_ASSERT(0)],\n\
                 ac_cv_static_assertion_macros_work=\"no\",\n\
                 )\n\
  AC_LANG_RESTORE\n\
 ])\n\
AC_MSG_RESULT(\"$ac_cv_static_assertion_macros_work\")\n\
if test \"$ac_cv_static_assertion_macros_work\" = \"no\"; then\n\
    AC_MSG_ERROR([Compiler cannot compile macros used in autoconf tests.])\n\
fi\n\
\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl ========================================================\n\
dnl Checking for 64-bit OS\n\
dnl ========================================================\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
AC_LANG_SAVE\n\
AC_LANG_C\n\
AC_MSG_CHECKING(for 64-bit OS)\n\
AC_TRY_COMPILE([$configure_static_assert_macros],\n\
               [CONFIGURE_STATIC_ASSERT(sizeof(long) == 8)],\n\
               result=\"yes\", result=\"no\")\n\
AC_MSG_RESULT(\"$result\")\n\
if test \"$result\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_64BIT_OS)\n\
    HAVE_64BIT_OS=1\n\
fi\n\
AC_SUBST(HAVE_64BIT_OS)\n\
AC_LANG_RESTORE\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl ========================================================\n\
dnl Enable high-memory support on OS/2, disabled by default\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(os2-high-mem,\n\
[  --enable-os2-high-mem Enable high-memory support on OS/2],\n\
    MOZ_OS2_HIGH_MEMORY=1,\n\
    MOZ_OS2_HIGH_MEMORY= )\n\
AC_SUBST(MOZ_OS2_HIGH_MEMORY)\n\
\n\
dnl ========================================================\n\
dnl System overrides of the defaults for host\n\
dnl ========================================================\n\
case \"$host\" in\n\
*-beos*)\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_BEOS -DBeOS -DBEOS -D_POSIX_SOURCE -DNO_X11\"\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_beos.cfg\\\"\'\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O3}\"\n\
    ;;\n\
\n\
*cygwin*|*mingw*|*mks*|*msvc*|*wince)\n\
    if test -n \"$_WIN32_MSVC\"; then\n\
        HOST_AR=lib\n\
        HOST_AR_FLAGS=\'-NOLOGO -OUT:\"$@\"\'\n\
        HOST_CFLAGS=\"$HOST_CFLAGS -TC -nologo -Fd\\$(HOST_PDBFILE)\"\n\
        HOST_RANLIB=\'echo ranlib\'\n\
    else\n\
        HOST_CFLAGS=\"$HOST_CFLAGS -mno-cygwin\"\n\
    fi\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_WIN32 -DXP_WIN -DWIN32 -D_WIN32 -DNO_X11\"\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_winnt.cfg\\\"\'\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O2}\"\n\
    HOST_BIN_SUFFIX=.exe\n\
    case \"$host\" in\n\
    *mingw*)\n\
    dnl MinGW/MSYS does not need CYGWIN_WRAPPER\n\
        ;;\n\
    *)\n\
        CYGWIN_WRAPPER=\"${srcdir}/build/cygwin-wrapper\"\n\
        if test \"`echo ${srcdir} | grep -c ^/ 2>/dev/null`\" = 0; then\n\
            _pwd=`pwd`\n\
            CYGWIN_WRAPPER=\"${_pwd}/${srcdir}/build/cygwin-wrapper\"\n\
        fi\n\
        if test \"`${PERL} -v | grep -c cygwin  2>/dev/null`\" = 0; then\n\
            AS_PERL=1\n\
            PERL=\"${CYGWIN_WRAPPER} $PERL\"\n\
        fi\n\
        PYTHON=\"${CYGWIN_WRAPPER} $PYTHON\"\n\
        ;;\n\
    esac\n\
    ;;\n\
\n\
*-darwin*)\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_UNIX -DXP_MACOSX -DNO_X11\"\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_darwin.cfg\\\"\'\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O3}\"\n\
    MOZ_FIX_LINK_PATHS=\'-Wl,-executable_path,$(LIBXUL_DIST)/bin\'\n\
    LIBXUL_LIBS=\'$(XPCOM_FROZEN_LDOPTS) $(LIBXUL_DIST)/bin/XUL -lobjc\'\n\
    ;;\n\
\n\
*-linux*)\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_UNIX\"\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_linux.cfg\\\"\'\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O3}\"\n\
    ;;\n\
\n\
*os2*)\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_OS2 -DNO_X11 -Zomf\"\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_os2.cfg\\\"\'\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O2}\"\n\
    HOST_BIN_SUFFIX=.exe\n\
    MOZ_FIX_LINK_PATHS=\n\
    ;;\n\
\n\
*-osf*)\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_UNIX\"\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_osf1.cfg\\\"\'\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O2}\"\n\
    ;;\n\
\n\
*)\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_UNIX\"\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O2}\"\n\
    ;;\n\
esac\n\
\n\
dnl Get mozilla version from central milestone file\n\
MOZILLA_VERSION=`$PERL $srcdir/config/milestone.pl -topsrcdir $srcdir`\n\
\n\
dnl Get version of various core apps from the version files.\n\
FIREFOX_VERSION=`cat $topsrcdir/browser/config/version.txt`\n\
THUNDERBIRD_VERSION=`cat $topsrcdir/mail/config/version.txt`\n\
SUNBIRD_VERSION=`cat $topsrcdir/calendar/sunbird/config/version.txt`\n\
SEAMONKEY_VERSION=`cat $topsrcdir/suite/config/version.txt`\n\
\n\
AC_DEFINE_UNQUOTED(MOZILLA_VERSION,\"$MOZILLA_VERSION\")\n\
AC_DEFINE_UNQUOTED(MOZILLA_VERSION_U,$MOZILLA_VERSION)\n\
\n\
dnl ========================================================\n\
dnl System overrides of the defaults for target\n\
dnl ========================================================\n\
\n\
case \"$target\" in\n\
*-aix*)\n\
    AC_DEFINE(AIX)\n\
    if test ! \"$GNU_CC\"; then\n\
        if test ! \"$HAVE_64BIT_OS\"; then\n\
            # Compiling with Visual Age C++ object model compat is the\n\
            # default. To compile with object model ibm, add \n\
            # AIX_OBJMODEL=ibm to .mozconfig.\n\
            if test \"$AIX_OBJMODEL\" = \"ibm\"; then\n\
                CXXFLAGS=\"$CXXFLAGS -qobjmodel=ibm\"\n\
            else\n\
                AIX_OBJMODEL=compat\n\
            fi\n\
        else\n\
            AIX_OBJMODEL=compat\n\
        fi\n\
        AC_SUBST(AIX_OBJMODEL)\n\
        DSO_LDOPTS=\'-qmkshrobj=1\'\n\
        DSO_CFLAGS=\'-qflag=w:w\'\n\
        DSO_PIC_CFLAGS=\n\
        LDFLAGS=\"$LDFLAGS -Wl,-brtl -blibpath:/usr/lib:/lib\"\n\
        AC_MSG_WARN([Clearing MOZ_FIX_LINK_PATHS till we can fix bug 332075.])\n\
        MOZ_FIX_LINK_PATHS=\n\
        MKSHLIB=\'$(CXX) $(DSO_LDOPTS) -o $@\'\n\
        MKCSHLIB=\'$(CC) $(DSO_LDOPTS) -o $@\'\n\
        if test \"$COMPILE_ENVIRONMENT\"; then\n\
            AC_LANG_SAVE\n\
            AC_LANG_CPLUSPLUS\n\
            AC_MSG_CHECKING([for VisualAge C++ compiler version >= 5.0.2.0])\n\
            AC_TRY_COMPILE([],\n\
                [#if (__IBMCPP__ < 502)\n\
                 #error \"Bad compiler\"\n\
                 #endif],\n\
                _BAD_COMPILER=,_BAD_COMPILER=1)\n\
            if test -n \"$_BAD_COMPILER\"; then\n\
                AC_MSG_RESULT([no])    \n\
                AC_MSG_ERROR([VisualAge C++ version 5.0.2.0 or higher is required to build.])\n\
            else\n\
                AC_MSG_RESULT([yes])    \n\
            fi\n\
            AC_LANG_RESTORE\n\
            TARGET_COMPILER_ABI=\"ibmc\"\n\
            CC_VERSION=`lslpp -Lcq vac.C 2>/dev/null | awk -F: \'{ print $3 }\'`\n\
            CXX_VERSION=`lslpp -Lcq vacpp.cmp.core 2>/dev/null | awk -F: \'{ print $3 }\'`\n\
        fi\n\
    fi\n\
    case \"${target_os}\" in\n\
    aix4.1*)\n\
        DLL_SUFFIX=\'_shr.a\'\n\
        ;;\n\
    esac\n\
    if test \"$COMPILE_ENVIRONMENT\"; then\n\
        AC_CHECK_HEADERS(sys/inttypes.h)\n\
    fi\n\
    AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
    ;;\n\
\n\
*-beos*)\n\
    no_x=yes\n\
    MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_LDOPTS) -Wl,-h,$@ -o $@\'\n\
    _PLATFORM_DEFAULT_TOOLKIT=\"cairo-beos\"\n\
    DSO_LDOPTS=\'-nostart\'\n\
    TK_LIBS=\'-lbe -lroot\'\n\
    LIBS=\"$LIBS -lbe\"\n\
    if test \"$COMPILE_ENVIRONMENT\"; then\n\
        AC_CHECK_LIB(bind,main,LIBS=\"$LIBS -lbind\")\n\
        AC_CHECK_LIB(zeta,main,LIBS=\"$LIBS -lzeta\")\n\
    fi\n\
    _WARNINGS_CFLAGS=\"${_WARNINGS_CFLAGS} -Wno-multichar\"\n\
    _WARNINGS_CXXFLAGS=\"${_WARNINGS_CXXFLAGS} -Wno-multichar\"\n\
    _MOZ_USE_RTTI=1\n\
    USE_DEPENDENT_LIBS=\n\
    MOZ_USER_DIR=\"Mozilla\"\n\
    ;;\n\
\n\
*-bsdi*)\n\
    dnl -pedantic doesn\'t play well with BSDI\'s _very_ modified gcc (shlicc2)\n\
    _PEDANTIC=\n\
    _IGNORE_LONG_LONG_WARNINGS=\n\
    case $OS_RELEASE in\n\
	4.*|5.*)\n\
            STRIP=\"$STRIP -d\"\n\
            ;;\n\
	*)\n\
	    DSO_CFLAGS=\'\'\n\
	    DSO_LDOPTS=\'-r\'\n\
	    _WARNINGS_CFLAGS=\"-Wall\"\n\
	    _WARNINGS_CXXFLAGS=\"-Wall\"\n\
	    # The test above doesn\'t work properly, at least on 3.1.\n\
	    MKSHLIB_FORCE_ALL=\'\'\n\
	    MKSHLIB_UNFORCE_ALL=\'\'\n\
	;;\n\
    esac\n\
    ;;\n\
\n\
*-darwin*) \n\
    MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
    MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
\n\
    _PEDANTIC=\n\
    CFLAGS=\"$CFLAGS -fpascal-strings -no-cpp-precomp -fno-common\"\n\
    CXXFLAGS=\"$CXXFLAGS -fpascal-strings -no-cpp-precomp -fno-common\"\n\
    DLL_SUFFIX=\".dylib\"\n\
    DSO_LDOPTS=\'\'\n\
    STRIP=\"$STRIP -x -S\"\n\
    _PLATFORM_DEFAULT_TOOLKIT=\'cairo-cocoa\'\n\
    MOZ_ENABLE_POSTSCRIPT=\n\
    TARGET_NSPR_MDCPUCFG=\'\\\"md/_darwin.cfg\\\"\'\n\
    # set MACOSX to generate lib/mac/MoreFiles/Makefile\n\
    MACOSX=1\n\
\n\
    dnl check for the presence of the -dead_strip linker flag\n\
    AC_MSG_CHECKING([for -dead_strip option to ld])\n\
    _SAVE_LDFLAGS=$LDFLAGS\n\
    LDFLAGS=\"$LDFLAGS -Wl,-dead_strip\"\n\
    AC_TRY_LINK(,[return 0;],_HAVE_DEAD_STRIP=1,_HAVE_DEAD_STRIP=)\n\
    if test -n \"$_HAVE_DEAD_STRIP\" ; then\n\
        AC_MSG_RESULT([yes])\n\
        MOZ_OPTIMIZE_LDFLAGS=\"-Wl,-dead_strip\"\n\
    else\n\
        AC_MSG_RESULT([no])\n\
    fi\n\
    LDFLAGS=$_SAVE_LDFLAGS\n\
    ;;\n\
\n\
*-freebsd*)\n\
    if test `test -x /usr/bin/objformat && /usr/bin/objformat || echo aout` != \"elf\"; then\n\
	DLL_SUFFIX=\".so.1.0\"\n\
	DSO_LDOPTS=\"-shared\"\n\
    fi\n\
    if test ! \"$GNU_CC\"; then\n\
	DSO_LDOPTS=\"-Bshareable $DSO_LDOPTS\"\n\
    fi\n\
# Can\'t have force w/o an unforce.\n\
#    # Hack for FreeBSD 2.2\n\
#    if test -z \"$MKSHLIB_FORCE_ALL\"; then\n\
#	MKSHLIB_FORCE_ALL=\'-Wl,-Bforcearchive\'\n\
#	MKSHLIB_UNFORCE_ALL=\'\'\n\
#    fi\n\
    ;; \n\
\n\
*-hpux*)\n\
    DLL_SUFFIX=\".sl\"\n\
    if test ! \"$GNU_CC\"; then\n\
    	DSO_LDOPTS=\'-b -Wl,+s\'\n\
    	DSO_CFLAGS=\"\"\n\
    	DSO_PIC_CFLAGS=\"+Z\"\n\
    	MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_LDOPTS) -L$(LIBXUL_DIST)/bin -o $@\'\n\
    	MKCSHLIB=\'$(LD) -b +s -L$(LIBXUL_DIST)/bin -o $@\'\n\
        CXXFLAGS=\"$CXXFLAGS -Wc,-ansi_for_scope,on\"\n\
    else\n\
        DSO_LDOPTS=\'-b -E +s\'\n\
        MKSHLIB=\'$(LD) $(DSO_LDOPTS) -L$(LIBXUL_DIST)/bin -L$(LIBXUL_DIST)/lib -o $@\'\n\
        MKCSHLIB=\'$(LD) $(DSO_LDOPTS) -L$(LIBXUL_DIST)/bin -L$(LIBXUL_DIST)/lib -o $@\'\n\
    fi\n\
    MOZ_POST_PROGRAM_COMMAND=\'chatr +s enable\'\n\
    AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
    ;;\n\
\n\
*-irix5*)\n\
    AC_DEFINE(IRIX)\n\
    DSO_LDOPTS=\'-elf -shared\'\n\
\n\
    if test \"$GNU_CC\"; then\n\
       MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
       MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
       MKSHLIB_FORCE_ALL=\'-Wl,-all\'\n\
       MKSHLIB_UNFORCE_ALL=\'-Wl,-none\'\n\
       CXXFLAGS=\"$CXXFLAGS -D_LANGUAGE_C_PLUS_PLUS\"\n\
    else\n\
       MKSHLIB=\'$(LD) $(DSO_LDOPTS) -o $@\'\n\
       MKCSHLIB=\'$(LD) $(DSO_LDOPTS) -o $@\'\n\
       MKSHLIB_FORCE_ALL=\'-all\'\n\
       MKSHLIB_UNFORCE_ALL=\'-none\'\n\
    fi\n\
    ;;\n\
\n\
*-irix6*)\n\
    AC_DEFINE(IRIX)\n\
    dnl the irix specific xptcinvoke code is written against the n32 ABI so we *must* \n\
    dnl compile and link using -n32\n\
    USE_N32=1\n\
    TARGET_COMPILER_ABI=n32\n\
    DSO_LDOPTS=\'-elf -shared\'\n\
    MKSHLIB=\'$(CCC) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
    MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
    _MOZ_EXCEPTIONS_FLAGS_OFF=\"-LANG:exceptions=OFF\"\n\
    _MOZ_EXCEPTIONS_FLAGS_ON=\"-LANG:exceptions=ON\"\n\
    if test \"$GNU_CC\"; then\n\
       MKSHLIB_FORCE_ALL=\'-Wl,-all\'\n\
       MKSHLIB_UNFORCE_ALL=\'-Wl,-none\'\n\
       _WARNINGS_CFLAGS=\"-Wall\"\n\
       _WARNINGS_CXXFLAGS=\"-Wall\"\n\
       CXXFLAGS=\"$CXXFLAGS -D_LANGUAGE_C_PLUS_PLUS\"\n\
    else\n\
       MKSHLIB_FORCE_ALL=\'-all\'\n\
       MKSHLIB_UNFORCE_ALL=\'-none\'\n\
	   AR_LIST=\"$AR t\"\n\
	   AR_EXTRACT=\"$AR x\"\n\
	   AR_DELETE=\"$AR d\"\n\
	   AR=\'$(CXX) -ar\'\n\
	   AR_FLAGS=\'-o $@\'\n\
       CFLAGS=\"$CFLAGS -woff 3262 -G 4\"\n\
       CXXFLAGS=\"$CXXFLAGS -woff 3262 -G 4\"\n\
       if test -n \"$USE_N32\"; then\n\
	   ASFLAGS=\"$ASFLAGS -n32\"\n\
	   CFLAGS=\"$CFLAGS -n32\"\n\
	   CXXFLAGS=\"$CXXFLAGS -n32\"\n\
	   LDFLAGS=\"$LDFLAGS -n32\"\n\
       fi\n\
       AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
       AC_MSG_WARN([Clearing MOZ_FIX_LINK_PATHS for OSF/1 as fix for bug 333545 (till the reference bug 332075 is fixed.])\n\
       MOZ_FIX_LINK_PATHS=\n\
    fi\n\
    if test -z \"$GNU_CXX\"; then\n\
      MIPSPRO_CXX=1\n\
    fi\n\
    ;;\n\
\n\
*-*linux*)\n\
    TARGET_NSPR_MDCPUCFG=\'\\\"md/_linux.cfg\\\"\'\n\
    MOZ_DEBUG_FLAGS=\"-g -fno-inline\"  # most people on linux use gcc/gdb,\n\
                                      # and that combo is not yet good at\n\
                                      # debugging inlined functions (even\n\
                                      # when using DWARF2 as the debugging\n\
                                      # format)    \n\
\n\
    case \"${target_cpu}\" in\n\
    alpha*)\n\
    	CFLAGS=\"$CFLAGS -mieee\"\n\
    	CXXFLAGS=\"$CXXFLAGS -mieee\"\n\
    ;;\n\
    i*86)\n\
    	USE_ELF_DYNSTR_GC=1\n\
        MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS=1\n\
    ;;\n\
    mips*)\n\
        CFLAGS=\"$CFLAGS -Wa,-xgot\"\n\
        CXXFLAGS=\"$CXXFLAGS -Wa,-xgot\"\n\
    ;;\n\
    esac\n\
    ;;\n\
\n\
*-wince*)\n\
\n\
    MOZ_TOOLS_DIR=`echo $MOZ_TOOLS`\n\
    AR_LIST=\"$AR -list\"\n\
    AR_EXTRACT=\"$AR -extract\"\n\
    AR_DELETE=\"$AR d\"\n\
    AR_FLAGS=\'-OUT:\"$@\"\'\n\
\n\
    DSO_CFLAGS=\n\
    DSO_PIC_CFLAGS=\n\
    DLL_SUFFIX=.dll\n\
    BIN_SUFFIX=\'.exe\'\n\
    RC=rc.exe\n\
    # certain versions of cygwin\'s makedepend barf on the \n\
    # #include <string> vs -I./dist/include/string issue so don\'t use it\n\
    SYSTEM_MAKEDEPEND=\n\
\n\
    HOST_CC=cl\n\
    HOST_CXX=cl\n\
    HOST_LD=link\n\
    HOST_AR=\'lib -OUT:$@\'\n\
    HOST_RANLIB=\'echo ranlib\'\n\
        \n\
	MOZ_OPTIMIZE_FLAGS=\'-O1\'\n\
    AR_FLAGS=\'-NOLOGO -OUT:\"$@\"\'\n\
    ASM_SUFFIX=asm\n\
    CFLAGS=\"$CFLAGS -W3 -Gy -Fd\\$(PDBFILE)\"\n\
    CXXFLAGS=\"$CXXFLAGS -W3 -Gy -Fd\\$(PDBFILE)\"\n\
    DLL_PREFIX=\n\
    DOXYGEN=:\n\
    DSO_LDOPTS=-SUBSYSTEM:WINDOWSCE\n\
    DYNAMIC_XPCOM_LIBS=\'$(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xpcom_core.lib\'\n\
    GARBAGE=\n\
    IMPORT_LIB_SUFFIX=lib\n\
    LIBS=\"$LIBS\"\n\
    LIBXUL_LIBS=\'$(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xul.lib\'\n\
    LIB_PREFIX=\n\
    LIB_SUFFIX=lib \n\
    MKCSHLIB=\'$(LD) -NOLOGO -DLL -OUT:$@ $(DSO_LDOPTS)\'\n\
    MKSHLIB=\'$(LD) -NOLOGO -DLL -OUT:$@ $(DSO_LDOPTS)\'\n\
    MKSHLIB_FORCE_ALL=\n\
    MKSHLIB_UNFORCE_ALL=\n\
    MOZ_COMPONENT_NSPR_LIBS=\'$(NSPR_LIBS)\'\n\
    MOZ_COMPONENT_NSPR_LIBS=\'$(NSPR_LIBS)\'\n\
    MOZ_DEBUG_FLAGS=\'-Zi\'\n\
    MOZ_DEBUG_LDFLAGS=\'-DEBUG -DEBUGTYPE:CV\'\n\
    MOZ_FIX_LINK_PATHS=\n\
    MOZ_JS_LIBS=\'$(LIBXUL_DIST)/lib/js$(MOZ_BITS)$(VERSION_NUMBER).lib\'\n\
    MOZ_OPTIMIZE_FLAGS=\'-O1\'\n\
    MOZ_XPCOM_OBSOLETE_LIBS=\'$(LIBXUL_DIST)/lib/xpcom_compat.lib\'\n\
    OBJ_SUFFIX=obj\n\
    RANLIB=\'echo not_ranlib\'\n\
    STRIP=\'echo not_strip\'\n\
    TARGET_NSPR_MDCPUCFG=\'\\\"md/_wince.cfg\\\"\'\n\
    UNZIP=unzip\n\
    XARGS=xargs\n\
    XPCOM_FROZEN_LDOPTS=\'$(LIBXUL_DIST)/lib/xpcom.lib\'\n\
    ZIP=zip\n\
    LIBIDL_CFLAGS=\"-I$MOZ_TOOLS_DIR/include ${GLIB_CFLAGS}\"\n\
    LIBIDL_LIBS=\"$MOZ_TOOLS_DIR/lib/libidl-0.6_s.lib $MOZ_TOOLS_DIR/lib/glib-1.2_s.lib\"\n\
    STATIC_LIBIDL=1\n\
\n\
    AC_DEFINE(HAVE_SNPRINTF)\n\
    AC_DEFINE(_WINDOWS)\n\
    AC_DEFINE(_WIN32)\n\
    AC_DEFINE(WIN32)\n\
    AC_DEFINE(XP_WIN)\n\
    AC_DEFINE(XP_WIN32)\n\
    AC_DEFINE(HW_THREADS)\n\
    AC_DEFINE(STDC_HEADERS)\n\
    AC_DEFINE(NEW_H, <new>)\n\
    AC_DEFINE(WIN32_LEAN_AND_MEAN)\n\
\n\
    TARGET_MD_ARCH=win32\n\
    _PLATFORM_DEFAULT_TOOLKIT=\'windows\'\n\
    BIN_SUFFIX=\'.exe\'\n\
    USE_SHORT_LIBNAME=1\n\
    MOZ_ENABLE_COREXFONTS=\n\
    MOZ_ENABLE_POSTSCRIPT=\n\
    MOZ_USER_DIR=\"Mozilla\"\n\
;;\n\
\n\
\n\
*-mingw*|*-cygwin*|*-msvc*|*-mks*)\n\
    DSO_CFLAGS=\n\
    DSO_PIC_CFLAGS=\n\
    DLL_SUFFIX=.dll\n\
    RC=rc.exe\n\
    # certain versions of cygwin\'s makedepend barf on the \n\
    # #include <string> vs -I./dist/include/string issue so don\'t use it\n\
    SYSTEM_MAKEDEPEND=\n\
    if test -n \"$GNU_CC\"; then\n\
        CC=\"$CC -mno-cygwin\"\n\
        CXX=\"$CXX -mno-cygwin\"\n\
        CPP=\"$CPP -mno-cygwin\"\n\
        CFLAGS=\"$CFLAGS -mms-bitfields\"\n\
        CXXFLAGS=\"$CXXFLAGS -mms-bitfields\"\n\
        DSO_LDOPTS=\'-shared\'\n\
        MKSHLIB=\'$(CXX) $(DSO_LDOPTS) -o $@\'\n\
        MKCSHLIB=\'$(CC) $(DSO_LDOPTS) -o $@\'\n\
        RC=\'$(WINDRES)\'\n\
        # Use temp file for windres (bug 213281)\n\
        RCFLAGS=\'-O coff --use-temp-file\'\n\
        # mingw doesn\'t require kernel32, user32, and advapi32 explicitly\n\
        LIBS=\"$LIBS -lgdi32 -lwinmm -lwsock32\"\n\
        MOZ_JS_LIBS=\'-L$(LIBXUL_DIST)/lib -ljs$(MOZ_BITS)$(VERSION_NUMBER)\'\n\
        MOZ_FIX_LINK_PATHS=\n\
        DYNAMIC_XPCOM_LIBS=\'-L$(LIBXUL_DIST)/lib -lxpcom -lxpcom_core\'\n\
        XPCOM_FROZEN_LDOPTS=\'-L$(LIBXUL_DIST)/lib -lxpcom\'\n\
        DLL_PREFIX=\n\
        IMPORT_LIB_SUFFIX=dll.a\n\
    else\n\
        TARGET_COMPILER_ABI=msvc\n\
        HOST_CC=\'$(CC)\'\n\
        HOST_CXX=\'$(CXX)\'\n\
        HOST_LD=\'$(LD)\'\n\
        AR=\'lib -NOLOGO -OUT:\"$@\"\'\n\
        AR_FLAGS=\n\
        RANLIB=\'echo not_ranlib\'\n\
        STRIP=\'echo not_strip\'\n\
        XARGS=xargs\n\
        ZIP=zip\n\
        UNZIP=unzip\n\
        DOXYGEN=:\n\
        GARBAGE=\'$(OBJDIR)/vc20.pdb $(OBJDIR)/vc40.pdb\'\n\
        OBJ_SUFFIX=obj\n\
        LIB_SUFFIX=lib\n\
        DLL_PREFIX=\n\
        LIB_PREFIX=\n\
        IMPORT_LIB_SUFFIX=lib\n\
        MKSHLIB=\'$(LD) -NOLOGO -DLL -OUT:$@ -PDB:$(PDBFILE) $(DSO_LDOPTS)\'\n\
        MKCSHLIB=\'$(LD) -NOLOGO -DLL -OUT:$@ -PDB:$(PDBFILE) $(DSO_LDOPTS)\'\n\
        MKSHLIB_FORCE_ALL=\n\
        MKSHLIB_UNFORCE_ALL=\n\
        DSO_LDOPTS=-SUBSYSTEM:WINDOWS\n\
        CFLAGS=\"$CFLAGS -W3 -Gy -Fd\\$(PDBFILE)\"\n\
        CXXFLAGS=\"$CXXFLAGS -W3 -Gy -Fd\\$(PDBFILE)\"\n\
        LIBS=\"$LIBS kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib advapi32.lib\"\n\
        MOZ_DEBUG_FLAGS=\'-Zi\'\n\
        MOZ_DEBUG_LDFLAGS=\'-DEBUG -DEBUGTYPE:CV\'\n\
    	MOZ_OPTIMIZE_FLAGS=\'-O1\'\n\
        MOZ_JS_LIBS=\'$(LIBXUL_DIST)/lib/js$(MOZ_BITS)$(VERSION_NUMBER).lib\'\n\
        MOZ_FIX_LINK_PATHS=\n\
        DYNAMIC_XPCOM_LIBS=\'$(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xpcom_core.lib\'\n\
        XPCOM_FROZEN_LDOPTS=\'$(LIBXUL_DIST)/lib/xpcom.lib\'\n\
        LIBXUL_LIBS=\'$(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xul.lib\'\n\
        MOZ_COMPONENT_NSPR_LIBS=\'$(NSPR_LIBS)\'\n\
        MOZ_XPCOM_OBSOLETE_LIBS=\'$(LIBXUL_DIST)/lib/xpcom_compat.lib\'\n\
    fi\n\
    MOZ_JPEG_LIBS=\'$(call EXPAND_LIBNAME_PATH,jpeg$(MOZ_BITS)$(VERSION_NUMBER),$(DEPTH)/jpeg)\'\n\
    MOZ_PNG_LIBS=\'$(call EXPAND_LIBNAME_PATH,png,$(DEPTH)/modules/libimg/png)\'\n\
    AC_DEFINE(HAVE_SNPRINTF)\n\
    AC_DEFINE(_WINDOWS)\n\
    AC_DEFINE(_WIN32)\n\
    AC_DEFINE(WIN32)\n\
    AC_DEFINE(XP_WIN)\n\
    AC_DEFINE(XP_WIN32)\n\
    AC_DEFINE(HW_THREADS)\n\
    AC_DEFINE(STDC_HEADERS)\n\
    AC_DEFINE(NEW_H, <new>)\n\
    AC_DEFINE(WIN32_LEAN_AND_MEAN)\n\
    TARGET_MD_ARCH=win32\n\
    _PLATFORM_DEFAULT_TOOLKIT=\'cairo-windows\'\n\
    BIN_SUFFIX=\'.exe\'\n\
    USE_SHORT_LIBNAME=1\n\
    MOZ_ENABLE_POSTSCRIPT=\n\
    MOZ_USER_DIR=\"Mozilla\"\n\
\n\
    dnl Hardcode to win95 for now - cls\n\
    TARGET_NSPR_MDCPUCFG=\'\\\"md/_win95.cfg\\\"\'\n\
\n\
    dnl set NO_X11 defines here as the general check is skipped on win32\n\
    no_x=yes\n\
    AC_DEFINE(NO_X11)\n\
\n\
    dnl MinGW/MSYS doesn\'t provide or need cygpath\n\
    case \"$host\" in\n\
    *-mingw*)\n\
	CYGPATH_W=echo\n\
	CYGPATH_S=cat\n\
	MOZ_BUILD_ROOT=`cd $MOZ_BUILD_ROOT && pwd -W`\n\
	;;\n\
    *-cygwin*|*-msvc*|*-mks*)\n\
	CYGPATH_W=\"cygpath -a -w\"\n\
	CYGPATH_S=\"sed -e s|\\\\\\\\|/|g\"\n\
	MOZ_BUILD_ROOT=`$CYGPATH_W $MOZ_BUILD_ROOT | $CYGPATH_S`\n\
	;;\n\
    esac\n\
    case \"$host\" in\n\
    *-mingw*|*-cygwin*|*-msvc*|*-mks*)\n\
\n\
    if test -z \"$MOZ_TOOLS\"; then\n\
        AC_MSG_ERROR([MOZ_TOOLS is not set])\n\
    fi\n\
\n\
    MOZ_TOOLS_DIR=`cd $MOZ_TOOLS && pwd`\n\
    if test \"$?\" != \"0\" || test -z \"$MOZ_TOOLS_DIR\"; then\n\
        AC_MSG_ERROR([cd \\$MOZ_TOOLS failed. MOZ_TOOLS ==? $MOZ_TOOLS])\n\
    fi\n\
    if test `echo ${PATH}: | grep -ic \"$MOZ_TOOLS_DIR/bin:\"` = 0; then\n\
        AC_MSG_ERROR([\\$MOZ_TOOLS\\\\bin must be in your path.])\n\
    fi\n\
    MOZ_TOOLS_DIR=`$CYGPATH_W $MOZ_TOOLS_DIR | $CYGPATH_S`\n\
\n\
    if test -n \"$GLIB_PREFIX\"; then\n\
        _GLIB_PREFIX_DIR=`cd $GLIB_PREFIX && pwd`\n\
        if test \"$?\" = \"0\"; then\n\
            if test `echo ${PATH}: | grep -ic \"$_GLIB_PREFIX_DIR/bin:\"` = 0; then\n\
                AC_MSG_ERROR([GLIB_PREFIX must be in your \\$PATH.])\n\
            fi\n\
            _GLIB_PREFIX_DIR=`$CYGPATH_W $_GLIB_PREFIX_DIR | $CYGPATH_S`\n\
        else\n\
            AC_MSG_ERROR([GLIB_PREFIX is set but \"${GLIB_PREFIX}\" is not a directory.])\n\
        fi\n\
    else\n\
        _GLIB_PREFIX_DIR=$MOZ_TOOLS_DIR\n\
    fi\n\
    if test ! -f \"${_GLIB_PREFIX_DIR}/include/glib.h\"; then\n\
        AC_MSG_ERROR([Cannot find $_GLIB_PREFIX_DIR/include/glib.h .])\n\
    fi\n\
    GLIB_CFLAGS=\"-I${_GLIB_PREFIX_DIR}/include\"\n\
    if test -f \"${_GLIB_PREFIX_DIR}/lib/glib-1.2_s.lib\"; then\n\
        GLIB_LIBS=\"${_GLIB_PREFIX_DIR}/lib/glib-1.2_s.lib\"\n\
    elif test -f \"${_GLIB_PREFIX_DIR}/lib/glib-1.2.lib\"; then\n\
        GLIB_LIBS=\"${_GLIB_PREFIX_DIR}/lib/glib-1.2.lib\"\n\
    else\n\
        AC_MSG_ERROR([Cannot find $_GLIB_PREFIX_DIR/lib/glib-1.2.lib or $_GLIB_PREFIX_DIR/lib/glib-1.2_s.lib])\n\
    fi\n\
\n\
    if test -n \"$LIBIDL_PREFIX\"; then\n\
        _LIBIDL_PREFIX_DIR=`cd $LIBIDL_PREFIX && pwd`\n\
        if test \"$?\" = \"0\"; then\n\
            if test `echo ${PATH}: | grep -ic \"$_LIBIDL_PREFIX_DIR/bin:\"` = 0; then\n\
                AC_MSG_ERROR([LIBIDL_PREFIX must be in your \\$PATH.])\n\
            fi\n\
            _LIBIDL_PREFIX_DIR=`$CYGPATH_W $_LIBIDL_PREFIX_DIR | $CYGPATH_S`\n\
        else\n\
            AC_MSG_ERROR([LIBIDL_PREFIX is set but \"${LIBIDL_PREFIX}\" is not a directory.])\n\
        fi\n\
    else\n\
        _LIBIDL_PREFIX_DIR=$MOZ_TOOLS_DIR\n\
    fi        \n\
    if test ! -f \"${_LIBIDL_PREFIX_DIR}/include/libIDL/IDL.h\"; then\n\
        AC_MSG_ERROR([Cannot find $_LIBIDL_PREFIX_DIR/include/libIDL/IDL.h .])\n\
    fi\n\
    LIBIDL_CFLAGS=\"-I${_LIBIDL_PREFIX_DIR}/include ${GLIB_CFLAGS}\"\n\
    if test -f \"${_LIBIDL_PREFIX_DIR}/lib/libidl-0.6_s.lib\"; then\n\
        LIBIDL_LIBS=\"${_LIBIDL_PREFIX_DIR}/lib/libidl-0.6_s.lib\"\n\
        STATIC_LIBIDL=1\n\
    elif test -f \"${_LIBIDL_PREFIX_DIR}/lib/libidl-0.6.lib\"; then\n\
        LIBIDL_LIBS=\"${_LIBIDL_PREFIX_DIR}/lib/libidl-0.6.lib\"\n\
    else\n\
        AC_MSG_ERROR([Cannot find $_LIBIDL_PREFIX_DIR/lib/libidl-0.6.lib or $_LIBIDL_PREFIX_DIR/lib/libidl-0.6_s.lib])\n\
    fi\n\
    LIBIDL_LIBS=\"${LIBIDL_LIBS} ${GLIB_LIBS}\"\n\
    ;;\n\
\n\
    *) # else cross-compiling\n\
        if test -n \"$GLIB_PREFIX\"; then\n\
            GLIB_CFLAGS=\"-I${GLIB_PREFIX}/include\"\n\
            if test -f \"${GLIB_PREFIX}/lib/glib-1.2_s.lib\"; then\n\
                GLIB_LIBS=\"${GLIB_PREFIX}/lib/glib-1.2_s.lib\"\n\
            elif test -f \"${GLIB_PREFIX}/lib/glib-1.2.lib\"; then\n\
                GLIB_LIBS=\"${GLIB_PREFIX}/lib/glib-1.2.lib\"\n\
            else\n\
                AC_MSG_ERROR([Cannot find $GLIB_PREFIX/lib/glib-1.2.lib or $GLIB_PREFIX/lib/glib-1.2_s.lib])\n\
            fi\n\
        fi\n\
        if test -n \"$LIBIDL_PREFIX\"; then\n\
            LIBIDL_CFLAGS=\"-I${LIBIDL_PREFIX}/include ${GLIB_CFLAGS}\"\n\
            if test -f \"${LIBIDL_PREFIX}/lib/libIDL-0.6_s.lib\"; then\n\
                LIBIDL_LIBS=\"${LIBIDL_PREFIX}/lib/libIDL-0.6_s.lib\"\n\
                STATIC_LIBIDL=1\n\
            elif test -f \"${LIBIDL_PREFIX}/lib/libIDL-0.6.lib\"; then\n\
                LIBIDL_LIBS=\"${LIBIDL_PREFIX}/lib/libIDL-0.6.lib\"\n\
            else\n\
                AC_MSG_ERROR([Cannot find $LIBIDL_PREFIX/lib/libIDL-0.6.lib or $LIBIDL_PREFIX/lib/libIDL-0.6_s.lib])\n\
            fi\n\
        fi\n\
        LIBIDL_LIBS=\"${LIBIDL_LIBS} ${GLIB_LIBS}\"\n\
        ;;\n\
    esac \n\
\n\
    case \"$target\" in\n\
    i*86-*)\n\
    	AC_DEFINE(_X86_)\n\
	;;\n\
    alpha-*)\n\
    	AC_DEFINE(_ALPHA_)\n\
	;;\n\
    mips-*)\n\
    	AC_DEFINE(_MIPS_)\n\
	;;\n\
    *)\n\
    	AC_DEFINE(_CPU_ARCH_NOT_DEFINED)\n\
	;;\n\
    esac\n\
    ;;\n\
\n\
*-netbsd*)\n\
    DSO_CFLAGS=\'\'\n\
    CFLAGS=\"$CFLAGS -Dunix\"\n\
    CXXFLAGS=\"$CXXFLAGS -Dunix\"\n\
    if $CC -E - -dM </dev/null | grep __ELF__ >/dev/null; then\n\
        DLL_SUFFIX=\".so\"\n\
        DSO_PIC_CFLAGS=\'-fPIC -DPIC\'\n\
        DSO_LDOPTS=\'-shared\'\n\
	BIN_FLAGS=\'-Wl,--export-dynamic\'\n\
    else\n\
    	DSO_PIC_CFLAGS=\'-fPIC -DPIC\'\n\
    	DLL_SUFFIX=\".so.1.0\"\n\
    	DSO_LDOPTS=\'-shared\'\n\
    fi\n\
    # This will fail on a.out systems prior to 1.5.1_ALPHA.\n\
    MKSHLIB_FORCE_ALL=\'-Wl,--whole-archive\'\n\
    MKSHLIB_UNFORCE_ALL=\'-Wl,--no-whole-archive\'\n\
    if test \"$LIBRUNPATH\"; then\n\
	DSO_LDOPTS=\"-Wl,-R$LIBRUNPATH $DSO_LDOPTS\"\n\
    fi\n\
    MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -Wl,-soname,lib$(LIBRARY_NAME)$(DLL_SUFFIX) -o $@\'\n\
    MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -Wl,-soname,lib$(LIBRARY_NAME)$(DLL_SUFFIX) -o $@\'\n\
    ;;\n\
\n\
*-nto*) \n\
	AC_DEFINE(NTO)	\n\
	AC_DEFINE(_QNX_SOURCE)\n\
	AC_DEFINE(_i386)\n\
	OS_TARGET=NTO\n\
	MOZ_OPTIMIZE_FLAGS=\"-O\"\n\
	MOZ_DEBUG_FLAGS=\"-gstabs\"\n\
	USE_PTHREADS=1\n\
	_PEDANTIC=\n\
	LIBS=\"$LIBS -lsocket -lstdc++\"\n\
	_DEFINES_CFLAGS=\'-Wp,-include -Wp,$(DEPTH)/mozilla-config.h -DMOZILLA_CLIENT -D_POSIX_C_SOURCE=199506\'\n\
	_DEFINES_CXXFLAGS=\'-DMOZILLA_CLIENT -Wp,-include -Wp,$(DEPTH)/mozilla-config.h -D_POSIX_C_SOURCE=199506\'\n\
	if test \"$with_x\" != \"yes\"\n\
	then\n\
		_PLATFORM_DEFAULT_TOOLKIT=\"photon\"\n\
	    TK_CFLAGS=\'-I/usr/include/photon\'\n\
		TK_LIBS=\'-lph\'\n\
	fi\n\
	case \"${target_cpu}\" in\n\
	ppc*)\n\
	AC_DEFINE(HAVE_VA_LIST_AS_ARRAY)	\n\
	;;\n\
	esac\n\
	case \"${host_cpu}\" in\n\
	i*86)\n\
	USE_ELF_DYNSTR_GC=1\n\
	;;\n\
	esac\n\
	;;\n\
\n\
*-openbsd*)\n\
    DLL_SUFFIX=\".so.1.0\"\n\
    DSO_CFLAGS=\'\'\n\
    DSO_PIC_CFLAGS=\'-fPIC\'\n\
    DSO_LDOPTS=\'-shared -fPIC\'\n\
    if test \"$LIBRUNPATH\"; then\n\
	DSO_LDOPTS=\"-R$LIBRUNPATH $DSO_LDOPTS\"\n\
    fi\n\
    ;;\n\
\n\
*-openvms*) \n\
    AC_DEFINE(NO_PW_GECOS)\n\
    AC_DEFINE(NO_UDSOCK)\n\
    AC_DEFINE(POLL_WITH_XCONNECTIONNUMBER)\n\
    USE_PTHREADS=1\n\
    MKSHLIB_FORCE_ALL=\'-all\'\n\
    MKSHLIB_UNFORCE_ALL=\'-none\'\n\
    AS=\'as\'\n\
    AS_DASH_C_FLAG=\'-Wc/names=as_is\'\n\
    AR_FLAGS=\'c $@\'\n\
    DSO_LDOPTS=\'-shared -auto_symvec\'\n\
    DSO_PIC_CFLAGS=\n\
    MOZ_DEBUG_LDFLAGS=\'-g\'\n\
    COMPAQ_CXX=1\n\
    CC_VERSION=`$CC -V 2>&1 | awk \'/ C / { print $3 }\'`\n\
    CXX_VERSION=`$CXX -V 2>&1 | awk \'/ C\\+\\+ / { print $3 }\'`\n\
    ;;\n\
\n\
\n\
*-os2*)\n\
    MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
    MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
    AC_DEFINE(XP_OS2)\n\
    USE_SHORT_LIBNAME=1\n\
    DLL_PREFIX=\n\
    LIB_PREFIX=\n\
    LIB_SUFFIX=lib\n\
    BIN_SUFFIX=\".exe\"\n\
    DLL_SUFFIX=\".dll\"\n\
    IMPORT_LIB_SUFFIX=lib\n\
    DSO_PIC_CFLAGS=\n\
    TARGET_MD_ARCH=os2\n\
    _PLATFORM_DEFAULT_TOOLKIT=os2\n\
    MOZ_ENABLE_POSTSCRIPT=\n\
    RC=rc.exe\n\
    RCFLAGS=\'-n\'\n\
    MOZ_USER_DIR=\"Mozilla\"\n\
\n\
    if test \"$MOZTOOLS\"; then\n\
        MOZ_TOOLS_DIR=`echo $MOZTOOLS | sed -e \'s|\\\\\\\\|/|g\'`\n\
    else\n\
        AC_MSG_ERROR([MOZTOOLS is not set])\n\
    fi\n\
\n\
    # EMX/GCC build\n\
    if test -n \"$GNU_CC\"; then\n\
        AC_DEFINE(OS2)\n\
        AC_DEFINE(XP_OS2_EMX)\n\
        AC_DEFINE(OS2EMX_PLAIN_CHAR)\n\
        AC_DEFINE(TCPV40HDRS)\n\
        AR=emxomfar\n\
        AR_FLAGS=\'r $@\'\n\
        CFLAGS=\"$CFLAGS -Zomf\"\n\
        CXXFLAGS=\"$CXXFLAGS -Zomf\"\n\
        DSO_LDOPTS=\'-Zdll\'\n\
        BIN_FLAGS=\'-Zlinker /ST:0x100000\'\n\
        IMPLIB=\'emximp -o\'\n\
        FILTER=\'emxexp -o\'\n\
        LDFLAGS=\'-Zmap\'\n\
        MOZ_DEBUG_FLAGS=\"-g -fno-inline\"\n\
        MOZ_OPTIMIZE_FLAGS=\"-O2 -s\"\n\
        MOZ_OPTIMIZE_LDFLAGS=\"-Zlinker /EXEPACK:2 -Zlinker /PACKCODE -Zlinker /PACKDATA\"\n\
        MOZ_XPCOM_OBSOLETE_LIBS=\'-L$(LIBXUL_DIST)/lib $(LIBXUL_DIST)/lib/xpcomct.lib\'\n\
        DYNAMIC_XPCOM_LIBS=\'-L$(LIBXUL_DIST)/lib $(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xpcomcor.lib\'\n\
        LIBXUL_LIBS=\'-L$(LIBXUL_DIST)/lib $(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xul.lib\'\n\
        if test -n \"$MOZ_OS2_HIGH_MEMORY\"; then\n\
          DSO_LDOPTS=\"$DSO_LDOPTS -Zhigh-mem\"\n\
          LDFLAGS=\"$LDFLAGS -Zhigh-mem\"\n\
          MOZ_OPTIMIZE_LDFLAGS=\"$MOZ_OPTIMIZE_LDFLAGS -Zhigh-mem\"\n\
          AC_DEFINE(MOZ_OS2_HIGH_MEMORY)\n\
        fi\n\
\n\
        # GCC for OS/2 currently predefines these, but we don\'t want them\n\
        _DEFINES_CFLAGS=\"$_DEFINES_CFLAGS -Uunix -U__unix -U__unix__\"\n\
        _DEFINES_CXXFLAGS=\"$_DEFINES_CXXFLAGS -Uunix -U__unix -U__unix__\"\n\
\n\
        AC_CACHE_CHECK(for __declspec(dllexport),\n\
           ac_os2_declspec,\n\
           [AC_TRY_COMPILE([__declspec(dllexport) void ac_os2_declspec(void) {}],\n\
                           [return 0;],\n\
                           ac_os2_declspec=\"yes\",\n\
                           ac_os2_declspec=\"no\")])\n\
        if test \"$ac_os2_declspec\" = \"yes\"; then\n\
           FILTER=\'true\'\n\
           MOZ_OS2_USE_DECLSPEC=\'1\'\n\
        fi\n\
        \n\
    # Visual Age C++ build\n\
    elif test \"$VACPP\" = \"yes\"; then\n\
        MOZ_BUILD_ROOT=`pwd -D`\n\
        OBJ_SUFFIX=obj\n\
        AR=-ilib\n\
        AR_FLAGS=\'/NOL /NOI /O:$(subst /,\\\\,$@)\'\n\
        AR_LIST=\'/L\'\n\
        AR_EXTRACT=\'-*\'\n\
        AR_DELETE=\'-\'\n\
        AS=alp\n\
        ASFLAGS=\'-Mb\'\n\
        AS_DASH_C_FLAG=\'\'\n\
        ASM_SUFFIX=asm\n\
        LD=\'-ilink\'\n\
        CFLAGS=\"/Q /qlibansi /Gm+ /Su4 /Mp /Tl9\"\n\
        CXXFLAGS=\"/Q /qlibansi /Gm+ /Su4 /Mp /Tl9 /Gx+\"\n\
        MOZ_DEBUG_FLAGS=\"/Ti+\"\n\
        MOZ_OPTIMIZE_FLAGS=\"/O+ /Gl+ /G5 /qarch=pentium\"\n\
        LDFLAGS=\"/NOL /M\"\n\
        MOZ_DEBUG_LDFLAGS=\"/DE\"\n\
        MOZ_OPTIMIZE_LDFLAGS=\"/OPTFUNC /EXEPACK:2 /PACKCODE /PACKDATA\"\n\
        DSO_LDOPTS=\'\'\n\
        DSO_PIC_CFLAGS=\n\
        IMPLIB=\'implib /NOL /NOI\'\n\
        FILTER=\'cppfilt -q -B -P\'\n\
        AC_DEFINE(NO_ANSI_KEYWORDS)\n\
        AC_DEFINE(OS2,4)\n\
        AC_DEFINE(_X86_)\n\
        AC_DEFINE(XP_OS2_VACPP)\n\
        AC_DEFINE(TCPV40HDRS)\n\
        AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
        AC_DEFINE(STDC_HEADERS)\n\
        MOZ_COMPONENT_NSPR_LIBS=\'$(NSPR_LIBS)\'\n\
        MKSHLIB=\'$(LD) $(DSO_LDOPTS)\'\n\
        MKCSHLIB=\'$(LD) $(DSO_LDOPTS)\'\n\
        MOZ_JS_LIBS=\'$(LIBXUL_DIST)/lib/mozjs.lib\'\n\
        MOZ_XPCOM_OBSOLETE_LIBS=\'$(LIBXUL_DIST)/lib/xpcomct.lib\'\n\
        DYNAMIC_XPCOM_LIBS=\'$(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xpcomcor.lib\'\n\
        LIBXUL_LIBS=\'$(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xul.lib\'\n\
    fi\n\
    ;;\n\
\n\
alpha*-*-osf*)\n\
    if test \"$GNU_CC\"; then\n\
      MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -Wl,-soname,$@ -o $@\'\n\
      MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -Wl,-soname,$@ -o $@\'\n\
\n\
    else\n\
	MOZ_DEBUG_FLAGS=\'-g\'\n\
	ASFLAGS=\'-I$(topsrcdir)/xpcom/reflect/xptcall/public -g\'\n\
	CFLAGS=\"$CFLAGS -ieee\"\n\
	CXXFLAGS=\"$CXXFLAGS \"\'-noexceptions -ieee  -ptr $(DIST)/cxx_repository\'\n\
	DSO_LDOPTS=\'-shared -msym -expect_unresolved \\* -update_registry $(DIST)/so_locations\'\n\
	DSO_CFLAGS=\n\
	DSO_PIC_CFLAGS=\n\
	MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -soname $@ -o $@\'\n\
	MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -soname $@ -o $@\'\n\
	MKSHLIB_FORCE_ALL=\'-all\'\n\
	MKSHLIB_UNFORCE_ALL=\'-none\'\n\
	dnl Might fix the libxpcom.so breakage on this platform as well....\n\
	AC_DEFINE(NSCAP_DISABLE_TEST_DONTQUERY_CASES)\n\
	AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
    fi\n\
    if test -z \"$GNU_CXX\"; then\n\
      COMPAQ_CXX=1\n\
    fi\n\
    AC_DEFINE(NEED_USLEEP_PROTOTYPE)\n\
    ;;\n\
\n\
*-qnx*) \n\
    DIRENT_INO=d_stat.st_ino\n\
    dnl Solves the problems the QNX compiler has with nsCOMPtr.h.\n\
    AC_DEFINE(NSCAP_DISABLE_TEST_DONTQUERY_CASES)\n\
    AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
    dnl Explicit set STDC_HEADERS to workaround QNX 6.0\'s failing of std test\n\
    AC_DEFINE(STDC_HEADERS)\n\
    if test \"$no_x\" = \"yes\"; then\n\
	    _PLATFORM_DEFAULT_TOOLKIT=\'photon\'\n\
	    TK_CFLAGS=\'-I/usr/nto/include/photon\'\n\
	    TK_LIBS=\'-lphoton -lphrender\'\n\
    fi\n\
    ;;\n\
\n\
*-sco*) \n\
    AC_DEFINE(NSCAP_DISABLE_TEST_DONTQUERY_CASES)\n\
    AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
    CXXFLAGS=\"$CXXFLAGS -I/usr/include/CC\"\n\
    if test ! \"$GNU_CC\"; then\n\
       DSO_LDOPTS=\'-G\'\n\
    fi\n\
    ;;\n\
\n\
dnl the qsort routine under solaris is faulty\n\
*-solaris*) \n\
    AC_DEFINE(SOLARIS)\n\
    TARGET_NSPR_MDCPUCFG=\'\\\"md/_solaris.cfg\\\"\'\n\
    SYSTEM_MAKEDEPEND=\n\
    # $ORIGIN/.. is for shared libraries under components/ to locate shared\n\
    # libraries one level up (e.g. libnspr4.so)\n\
    LDFLAGS=\"$LDFLAGS -z ignore -R \'\\$\\$ORIGIN:\\$\\$ORIGIN/..\'\"\n\
    if test -z \"$GNU_CC\"; then\n\
       NS_USE_NATIVE=1\n\
       MOZ_FIX_LINK_PATHS=\'-R $(LIBXUL_DIST)/bin\'\n\
       AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
       CFLAGS=\"$CFLAGS -xstrconst -xbuiltin=%all\"\n\
       CXXFLAGS=\"$CXXFLAGS -xbuiltin=%all -features=tmplife -norunpath\"\n\
       LDFLAGS=\"-xildoff -z lazyload -z combreloc $LDFLAGS\"\n\
       if test -z \"$CROSS_COMPILE\" && test -f /usr/lib/ld/map.noexstk; then\n\
           _SAVE_LDFLAGS=$LDFLAGS\n\
           LDFLAGS=\"-M /usr/lib/ld/map.noexstk $LDFLAGS\" \n\
           AC_TRY_LINK([#include <stdio.h>],\n\
                       [printf(\"Hello World\\n\");],\n\
                       ,\n\
                       [LDFLAGS=$_SAVE_LDFLAGS])\n\
       fi\n\
       MOZ_OPTIMIZE_FLAGS=\"-xO4\"\n\
       MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_FLAGS) $(DSO_LDOPTS) -h $@ -o $@\'\n\
       MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_FLAGS) -G -z muldefs -h $@ -o $@\'\n\
       MKSHLIB_FORCE_ALL=\'-z allextract\'\n\
       MKSHLIB_UNFORCE_ALL=\'\'\n\
       DSO_LDOPTS=\'-G -z muldefs\'\n\
       AR_LIST=\"$AR t\"\n\
       AR_EXTRACT=\"$AR x\"\n\
       AR_DELETE=\"$AR d\"\n\
       AR=\'$(CXX) -xar\'\n\
       AR_FLAGS=\'-o $@\'\n\
       AS=\'/usr/ccs/bin/as\'\n\
       ASFLAGS=\"$ASFLAGS -K PIC -L -P -D_ASM -D__STDC__=0\"\n\
       AS_DASH_C_FLAG=\'\'\n\
       TARGET_COMPILER_ABI=\"sunc\"\n\
        CC_VERSION=`$CC -V 2>&1 | grep \'^cc:\' 2>/dev/null | $AWK -F\\: \'{ print $2 }\'`\n\
        CXX_VERSION=`$CXX -V 2>&1 | grep \'^CC:\' 2>/dev/null | $AWK -F\\: \'{ print $2 }\'`\n\
       AC_MSG_CHECKING([for Forte compiler version >= WS6U2])\n\
       AC_LANG_SAVE\n\
       AC_LANG_CPLUSPLUS\n\
       AC_TRY_COMPILE([],\n\
           [#if (__SUNPRO_CC < 0x530)\n\
           #error \"Denied\"\n\
           #endif],\n\
           _BAD_COMPILER=,_BAD_COMPILER=1)\n\
        if test -n \"$_BAD_COMPILER\"; then\n\
            _res=\"no\"\n\
            AC_MSG_ERROR([Forte version WS6U2 or higher is required to build. Your compiler version is $CC_VERSION .])\n\
        else\n\
            _res=\"yes\"\n\
        fi\n\
        AC_MSG_RESULT([$_res])\n\
        AC_LANG_RESTORE\n\
    else\n\
       ASFLAGS=\"$ASFLAGS -fPIC\"\n\
       DSO_LDOPTS=\'-G\'\n\
       _WARNINGS_CFLAGS=\'\'\n\
       _WARNINGS_CXXFLAGS=\'\'\n\
       if test \"$OS_RELEASE\" = \"5.3\"; then\n\
	  AC_DEFINE(MUST_UNDEF_HAVE_BOOLEAN_AFTER_INCLUDES)\n\
       fi\n\
    fi\n\
    if test \"$OS_RELEASE\" = \"5.5.1\"; then\n\
       AC_DEFINE(NEED_USLEEP_PROTOTYPE)\n\
    fi\n\
    ;;\n\
\n\
*-sunos*) \n\
    DSO_LDOPTS=\'-Bdynamic\'\n\
    MKSHLIB=\'-$(LD) $(DSO_LDOPTS) -o $@\'\n\
    MKCSHLIB=\'-$(LD) $(DSO_LDOPTS) -o $@\'\n\
    AC_DEFINE(SUNOS4)\n\
    AC_DEFINE(SPRINTF_RETURNS_STRING)\n\
    case \"$(target_os)\" in\n\
    sunos4.1*)\n\
        DLL_SUFFIX=\'.so.1.0\'\n\
        ;;\n\
    esac\n\
    ;;\n\
\n\
*-sysv4.2uw7*) \n\
	NSPR_LIBS=\"-lnspr$NSPR_VERSION -lplc$NSPR_VERSION -lplds$NSPR_VERSION -L/usr/ccs/lib -lcrt\"\n\
    ;;\n\
\n\
*-os2*)\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_os2.cfg\\\"\'\n\
    ;;\n\
\n\
esac\n\
\n\
dnl Only one oddball right now (QNX), but this gives us flexibility\n\
dnl if any other platforms need to override this in the future.\n\
AC_DEFINE_UNQUOTED(D_INO,$DIRENT_INO)\n\
\n\
dnl ========================================================\n\
dnl Any platform that doesn\'t have MKSHLIB_FORCE_ALL defined\n\
dnl by now will not have any way to link most binaries (tests\n\
dnl as well as viewer, apprunner, etc.), because some symbols\n\
dnl will be left out of the \"composite\" .so\'s by ld as unneeded.\n\
dnl So, by defining NO_LD_ARCHIVE_FLAGS for these platforms,\n\
dnl they can link in the static libs that provide the missing\n\
dnl symbols.\n\
dnl ========================================================\n\
NO_LD_ARCHIVE_FLAGS=\n\
if test -z \"$MKSHLIB_FORCE_ALL\" || test -z \"$MKSHLIB_UNFORCE_ALL\"; then\n\
    NO_LD_ARCHIVE_FLAGS=1\n\
fi\n\
case \"$target\" in\n\
*-os2*)\n\
    NO_LD_ARCHIVE_FLAGS=\n\
    ;;\n\
*-aix4.3*|*-aix5*)\n\
    NO_LD_ARCHIVE_FLAGS=\n\
    ;;\n\
*-openvms*)\n\
    NO_LD_ARCHIVE_FLAGS=\n\
    ;;\n\
*-msvc*|*-mks*|*-mingw*|*-cygwin*|*-wince)\n\
    if test -z \"$GNU_CC\"; then\n\
        NO_LD_ARCHIVE_FLAGS=\n\
    fi\n\
    ;;\n\
esac\n\
AC_SUBST(NO_LD_ARCHIVE_FLAGS)\n\
\n\
dnl\n\
dnl Indicate that platform requires special thread safe \n\
dnl locking when starting up the OJI JVM \n\
dnl (see mozilla/modules/oji/src/nsJVMManager.cpp)\n\
dnl ========================================================\n\
case \"$target\" in\n\
    *-hpux*)      \n\
        AC_DEFINE(MOZ_OJI_REQUIRE_THREAD_SAFE_ON_STARTUP)\n\
        ;;\n\
esac\n\
\n\
dnl ========================================================\n\
dnl = Flags to strip unused symbols from .so components\n\
dnl ========================================================\n\
case \"$target\" in\n\
    *-linux*)\n\
        MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-Wl,--version-script -Wl,$(BUILD_TOOLS)/gnu-ld-scripts/components-version-script\'\n\
        ;;\n\
    *-solaris*)\n\
        if test -z \"$GNU_CC\"; then\n\
         MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-M $(BUILD_TOOLS)/gnu-ld-scripts/components-mapfile\'\n\
        else\n\
         if test -z \"$GCC_USE_GNU_LD\"; then\n\
          MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-Wl,-M -Wl,$(BUILD_TOOLS)/gnu-ld-scripts/components-mapfile\'\n\
         else\n\
          MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-Wl,--version-script -Wl,$(BUILD_TOOLS)/gnu-ld-scripts/components-version-script\'\n\
         fi\n\
        fi\n\
        ;;\n\
    *-nto*) \n\
        MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-Wl,--version-script,$(BUILD_TOOLS)/gnu-ld-scripts/components-version-script\'\n\
        ;;\n\
    *-darwin*)\n\
        AC_MSG_CHECKING(for -exported_symbols_list option to ld)\n\
        AC_CACHE_VAL(ac_cv_exported_symbols_list,\n\
         [\n\
           if $LD -exported_symbols_list 2>&1 | grep \"argument missing\" >/dev/null; then\n\
             ac_cv_exported_symbols_list=true\n\
           else\n\
             ac_cv_exported_symbols_list=false\n\
           fi\n\
         ])\n\
        if test \"$ac_cv_exported_symbols_list\" = true; then\n\
          MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-Wl,-exported_symbols_list -Wl,$(BUILD_TOOLS)/gnu-ld-scripts/components-export-list\'\n\
          AC_MSG_RESULT(yes)\n\
        else\n\
          AC_MSG_RESULT(no)\n\
        fi\n\
        ;;\n\
    *-cygwin*|*-mingw*|*-mks*|*-msvc|*-wince)\n\
        if test -n \"$GNU_CC\"; then\n\
           MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-Wl,--version-script,$(BUILD_TOOLS)/gnu-ld-scripts/components-version-script\'\n\
        fi\n\
        ;;\n\
esac\n\
\n\
if test -z \"$COMPILE_ENVIRONMENT\"; then\n\
    SKIP_COMPILER_CHECKS=1\n\
fi\n\
\n\
if test -z \"$SKIP_COMPILER_CHECKS\"; then\n\
dnl Checks for typedefs, structures, and compiler characteristics.\n\
dnl ========================================================\n\
AC_LANG_C\n\
AC_HEADER_STDC\n\
AC_C_CONST\n\
AC_TYPE_MODE_T\n\
AC_TYPE_OFF_T\n\
AC_TYPE_PID_T\n\
AC_TYPE_SIZE_T\n\
AC_STRUCT_ST_BLKSIZE\n\
AC_MSG_CHECKING(for siginfo_t)\n\
AC_CACHE_VAL(ac_cv_siginfo_t,\n\
 [AC_TRY_COMPILE([#define _POSIX_C_SOURCE 199506L\n\
                  #include <signal.h>],\n\
                 [siginfo_t* info;],\n\
                 [ac_cv_siginfo_t=true],\n\
                 [ac_cv_siginfo_t=false])])\n\
if test \"$ac_cv_siginfo_t\" = true ; then\n\
  AC_DEFINE(HAVE_SIGINFO_T)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
\n\
dnl Visual Age for os/2 also defines size_t and off_t in certain \n\
dnl  header files.  These defines make Visual Age use the mozilla\n\
dnl  defines types.\n\
if test \"$VACPP\" = \"yes\"; then\n\
   AC_DEFINE(__size_t)\n\
   AC_DEFINE(__off_t)\n\
fi\n\
\n\
dnl Check for int16_t, int32_t, int64_t, int64, uint, uint_t, and uint16_t.\n\
dnl ========================================================\n\
AC_MSG_CHECKING(for int16_t)\n\
AC_CACHE_VAL(ac_cv_int16_t,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [int16_t foo = 0;],\n\
                 [ac_cv_int16_t=true],\n\
                 [ac_cv_int16_t=false])])\n\
if test \"$ac_cv_int16_t\" = true ; then\n\
  AC_DEFINE(HAVE_INT16_T)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
AC_MSG_CHECKING(for int32_t)\n\
AC_CACHE_VAL(ac_cv_int32_t,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [int32_t foo = 0;],\n\
                 [ac_cv_int32_t=true],\n\
                 [ac_cv_int32_t=false])])\n\
if test \"$ac_cv_int32_t\" = true ; then\n\
  AC_DEFINE(HAVE_INT32_T)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
AC_MSG_CHECKING(for int64_t)\n\
AC_CACHE_VAL(ac_cv_int64_t,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [int64_t foo = 0;],\n\
                 [ac_cv_int64_t=true],\n\
                 [ac_cv_int64_t=false])])\n\
if test \"$ac_cv_int64_t\" = true ; then\n\
  AC_DEFINE(HAVE_INT64_T)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
AC_MSG_CHECKING(for int64)\n\
AC_CACHE_VAL(ac_cv_int64,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [int64 foo = 0;],\n\
                 [ac_cv_int64=true],\n\
                 [ac_cv_int64=false])])\n\
if test \"$ac_cv_int64\" = true ; then\n\
  AC_DEFINE(HAVE_INT64)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
AC_MSG_CHECKING(for uint)\n\
AC_CACHE_VAL(ac_cv_uint,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [uint foo = 0;],\n\
                 [ac_cv_uint=true],\n\
                 [ac_cv_uint=false])])\n\
if test \"$ac_cv_uint\" = true ; then\n\
  AC_DEFINE(HAVE_UINT)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
AC_MSG_CHECKING(for uint_t)\n\
AC_CACHE_VAL(ac_cv_uint_t,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [uint_t foo = 0;],\n\
                 [ac_cv_uint_t=true],\n\
                 [ac_cv_uint_t=false])])\n\
if test \"$ac_cv_uint_t\" = true ; then\n\
  AC_DEFINE(HAVE_UINT_T)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
AC_MSG_CHECKING(for uint16_t)\n\
AC_CACHE_VAL(ac_cv_uint16_t,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [uint16_t foo = 0;],\n\
                 [ac_cv_uint16_t=true],\n\
                 [ac_cv_uint16_t=false])])\n\
if test \"$ac_cv_uint16_t\" = true ; then\n\
  AC_DEFINE(HAVE_UINT16_T)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
\n\
dnl On the gcc trunk (as of 2001-02-09) _GNU_SOURCE, and thus __USE_GNU,\n\
dnl are defined when compiling C++ but not C.  Since the result of this\n\
dnl test is used only in C++, do it in C++.\n\
AC_LANG_CPLUSPLUS\n\
\n\
AC_MSG_CHECKING(for uname.domainname)\n\
AC_CACHE_VAL(ac_cv_have_uname_domainname_field,\n\
    [AC_TRY_COMPILE([#include <sys/utsname.h>],\n\
        [ struct utsname *res; char *domain; \n\
            (void)uname(res);  if (res != 0) { domain = res->domainname; } ],\n\
        [ac_cv_have_uname_domainname_field=true],\n\
        [ac_cv_have_uname_domainname_field=false])])\n\
\n\
if test \"$ac_cv_have_uname_domainname_field\" = \"true\"; then\n\
    AC_DEFINE(HAVE_UNAME_DOMAINNAME_FIELD)\n\
    AC_MSG_RESULT(yes)\n\
else\n\
    AC_MSG_RESULT(no)\n\
fi\n\
\n\
AC_MSG_CHECKING(for uname.__domainname)\n\
AC_CACHE_VAL(ac_cv_have_uname_us_domainname_field,\n\
    [AC_TRY_COMPILE([#include <sys/utsname.h>],\n\
        [ struct utsname *res; char *domain; \n\
            (void)uname(res);  if (res != 0) { domain = res->__domainname; } ],\n\
        [ac_cv_have_uname_us_domainname_field=true],\n\
        [ac_cv_have_uname_us_domainname_field=false])])\n\
\n\
if test \"$ac_cv_have_uname_us_domainname_field\" = \"true\"; then\n\
    AC_DEFINE(HAVE_UNAME_US_DOMAINNAME_FIELD)\n\
    AC_MSG_RESULT(yes)\n\
else\n\
    AC_MSG_RESULT(no)\n\
fi\n\
\n\
AC_LANG_C\n\
\n\
dnl Check for usable wchar_t (2 bytes, unsigned)\n\
dnl (we really don\'t need the unsignedness check anymore)\n\
dnl ========================================================\n\
\n\
AC_CACHE_CHECK(for usable wchar_t (2 bytes, unsigned),\n\
    ac_cv_have_usable_wchar_v2,\n\
    [AC_TRY_COMPILE([#include <stddef.h>\n\
                     $configure_static_assert_macros],\n\
                    [CONFIGURE_STATIC_ASSERT(sizeof(wchar_t) == 2);\n\
                     CONFIGURE_STATIC_ASSERT((wchar_t)-1 > (wchar_t) 0)],\n\
                    ac_cv_have_usable_wchar_v2=\"yes\",\n\
                    ac_cv_have_usable_wchar_v2=\"no\")])\n\
if test \"$ac_cv_have_usable_wchar_v2\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_CPP_2BYTE_WCHAR_T)\n\
    HAVE_CPP_2BYTE_WCHAR_T=1\n\
else\n\
dnl This is really gcc-only\n\
dnl Do this test using CXX only since some versions of gcc\n\
dnl 2.95-2.97 have a signed wchar_t in c++ only and some versions\n\
dnl only have short-wchar support for c++.\n\
dnl Note that we assume that mac & win32 have short wchar (see nscore.h)\n\
\n\
    AC_LANG_SAVE\n\
    AC_LANG_CPLUSPLUS\n\
    _SAVE_CXXFLAGS=$CXXFLAGS\n\
    CXXFLAGS=\"$CXXFLAGS -fshort-wchar\"\n\
\n\
    AC_CACHE_CHECK(for compiler -fshort-wchar option, \n\
        ac_cv_have_usable_wchar_option_v2,\n\
        [AC_TRY_COMPILE([#include <stddef.h>\n\
                         $configure_static_assert_macros],\n\
                        [CONFIGURE_STATIC_ASSERT(sizeof(wchar_t) == 2);\n\
                         CONFIGURE_STATIC_ASSERT((wchar_t)-1 > (wchar_t) 0)],\n\
                        ac_cv_have_usable_wchar_option_v2=\"yes\",\n\
                        ac_cv_have_usable_wchar_option_v2=\"no\")])\n\
\n\
    if test \"$ac_cv_have_usable_wchar_option_v2\" = \"yes\"; then\n\
        AC_DEFINE(HAVE_CPP_2BYTE_WCHAR_T)\n\
        HAVE_CPP_2BYTE_WCHAR_T=1\n\
    else    \n\
        CXXFLAGS=$_SAVE_CXXFLAGS\n\
    fi\n\
    AC_LANG_RESTORE\n\
fi\n\
\n\
dnl Check for .hidden assembler directive and visibility attribute.\n\
dnl Borrowed from glibc configure.in\n\
dnl ===============================================================\n\
if test \"$GNU_CC\"; then\n\
  AC_CACHE_CHECK(for visibility(hidden) attribute,\n\
                 ac_cv_visibility_hidden,\n\
                 [cat > conftest.c <<EOF\n\
                  int foo __attribute__ ((visibility (\"hidden\"))) = 1;\n\
EOF\n\
                  ac_cv_visibility_hidden=no\n\
                  if ${CC-cc} -Werror -S conftest.c -o conftest.s >/dev/null 2>&1; then\n\
                    if egrep \'\\.(hidden|private_extern).*foo\' conftest.s >/dev/null; then\n\
                      ac_cv_visibility_hidden=yes\n\
                    fi\n\
                  fi\n\
                  rm -f conftest.[cs]\n\
                 ])\n\
  if test \"$ac_cv_visibility_hidden\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_VISIBILITY_HIDDEN_ATTRIBUTE)\n\
\n\
    AC_CACHE_CHECK(for visibility(default) attribute,\n\
                   ac_cv_visibility_default,\n\
                   [cat > conftest.c <<EOF\n\
                    int foo __attribute__ ((visibility (\"default\"))) = 1;\n\
EOF\n\
                    ac_cv_visibility_default=no\n\
                    if ${CC-cc} -fvisibility=hidden -Werror -S conftest.c -o conftest.s >/dev/null 2>&1; then\n\
                      if ! egrep \'\\.(hidden|private_extern).*foo\' conftest.s >/dev/null; then\n\
                        ac_cv_visibility_default=yes\n\
                      fi\n\
                    fi\n\
                    rm -f conftest.[cs]\n\
                   ])\n\
    if test \"$ac_cv_visibility_default\" = \"yes\"; then\n\
      AC_DEFINE(HAVE_VISIBILITY_ATTRIBUTE)\n\
\n\
      AC_CACHE_CHECK(for visibility pragma support,\n\
                     ac_cv_visibility_pragma,\n\
                     [cat > conftest.c <<EOF\n\
#pragma GCC visibility push(hidden)\n\
                      int foo_hidden = 1;\n\
#pragma GCC visibility push(default)\n\
                      int foo_default = 1;\n\
EOF\n\
                      ac_cv_visibility_pragma=no\n\
                      if ${CC-cc} -Werror -S conftest.c -o conftest.s >/dev/null 2>&1; then\n\
                        if egrep \'\\.(hidden|extern_private).*foo_hidden\' conftest.s >/dev/null; then\n\
                          if ! egrep \'\\.(hidden|extern_private).*foo_default\' conftest.s > /dev/null; then\n\
                            ac_cv_visibility_pragma=yes\n\
                          fi\n\
                        fi\n\
                      fi\n\
                      rm -f conftest.[cs]\n\
                    ])\n\
      if test \"$ac_cv_visibility_pragma\" = \"yes\"; then\n\
        AC_CACHE_CHECK(For gcc visibility bug with class-level attributes (GCC bug 26905),\n\
                       ac_cv_have_visibility_class_bug,\n\
                       [cat > conftest.c <<EOF\n\
#pragma GCC visibility push(hidden)\n\
struct __attribute__ ((visibility (\"default\"))) TestStruct {\n\
  static void Init();\n\
};\n\
__attribute__ ((visibility (\"default\"))) void TestFunc() {\n\
  TestStruct::Init();\n\
}\n\
EOF\n\
                       ac_cv_have_visibility_class_bug=no\n\
                       if ! ${CXX-g++} ${CXXFLAGS} ${DSO_PIC_CFLAGS} ${DSO_LDOPTS} -S -o conftest.S conftest.c > /dev/null 2>&1 ; then\n\
                         ac_cv_have_visibility_class_bug=yes\n\
                       else\n\
                         if test `grep -c \"@PLT\" conftest.S` = 0; then\n\
                           ac_cv_have_visibility_class_bug=yes\n\
                         fi\n\
                       fi\n\
                       rm -rf conftest.{c,S}\n\
                       ])\n\
\n\
        AC_CACHE_CHECK(For x86_64 gcc visibility bug with builtins (GCC bug 20297),\n\
                       ac_cv_have_visibility_builtin_bug,\n\
                       [cat > conftest.c <<EOF\n\
#pragma GCC visibility push(hidden)\n\
#pragma GCC visibility push(default)\n\
#include <string.h>\n\
#pragma GCC visibility pop\n\
\n\
__attribute__ ((visibility (\"default\"))) void Func() {\n\
  char c[[100]];\n\
  memset(c, 0, sizeof(c));\n\
}\n\
EOF\n\
                       ac_cv_have_visibility_builtin_bug=no\n\
                       if ! ${CC-cc} ${CFLAGS} ${DSO_PIC_CFLAGS} ${DSO_LDOPTS} -O2 -S -o conftest.S conftest.c > /dev/null 2>&1 ; then\n\
                         ac_cv_have_visibility_builtin_bug=yes\n\
                       else\n\
                         if test `grep -c \"@PLT\" conftest.S` = 0; then\n\
                           ac_cv_visibility_builtin_bug=yes\n\
                         fi\n\
                       fi\n\
                       rm -f conftest.{c,S}\n\
                       ])\n\
        if test \"$ac_cv_have_visibility_builtin_bug\" = \"no\" -a \\\n\
                \"$ac_cv_have_visibility_class_bug\" = \"no\"; then\n\
          VISIBILITY_FLAGS=\'-I$(DIST)/include/system_wrappers -include $(topsrcdir)/config/gcc_hidden.h\'\n\
          WRAP_SYSTEM_INCLUDES=1\n\
        else\n\
          VISIBILITY_FLAGS=\'-fvisibility=hidden\'\n\
        fi # have visibility pragma bug\n\
      fi   # have visibility pragma\n\
    fi     # have visibility(default) attribute\n\
  fi       # have visibility(hidden) attribute\n\
fi         # GNU_CC\n\
\n\
AC_SUBST(WRAP_SYSTEM_INCLUDES)\n\
AC_SUBST(VISIBILITY_FLAGS)\n\
\n\
dnl Checks for header files.\n\
dnl ========================================================\n\
AC_HEADER_DIRENT\n\
case \"$target_os\" in\n\
freebsd*)\n\
# for stuff like -lXshm\n\
    CPPFLAGS=\"${CPPFLAGS} ${X_CFLAGS}\"\n\
    ;;\n\
esac\n\
AC_CHECK_HEADERS(sys/byteorder.h compat.h getopt.h)\n\
AC_CHECK_HEADERS(sys/bitypes.h memory.h unistd.h)\n\
AC_CHECK_HEADERS(gnu/libc-version.h nl_types.h)\n\
AC_CHECK_HEADERS(malloc.h)\n\
AC_CHECK_HEADERS(X11/XKBlib.h)\n\
\n\
dnl These are all the places some variant of statfs can be hiding.\n\
AC_CHECK_HEADERS(sys/statvfs.h sys/statfs.h sys/vfs.h sys/mount.h)\n\
\n\
dnl Try for MMX support\n\
dnl NB - later gcc versions require -mmmx for this header to be successfully\n\
dnl included (or another option which implies it, such as -march=pentium-mmx)\n\
AC_CHECK_HEADERS(mmintrin.h)\n\
\n\
dnl Check whether the compiler supports the new-style C++ standard\n\
dnl library headers (i.e. <new>) or needs the old \"new.h\"\n\
AC_LANG_CPLUSPLUS\n\
NEW_H=new.h\n\
AC_CHECK_HEADER(new, [NEW_H=new])\n\
AC_DEFINE_UNQUOTED(NEW_H, <$NEW_H>)\n\
AC_LANG_C\n\
\n\
case $target in\n\
*-aix4.3*|*-aix5*)\n\
	;;\n\
*)\n\
	AC_CHECK_HEADERS(sys/cdefs.h)\n\
	;;\n\
esac\n\
\n\
dnl Checks for libraries.\n\
dnl ========================================================\n\
case $target in\n\
*-hpux11.*)\n\
	;;\n\
*)\n\
	AC_CHECK_LIB(c_r, gethostbyname_r)\n\
	;;\n\
esac\n\
\n\
dnl We don\'t want to link with libdl even if it\'s present on OS X, since\n\
dnl it\'s not used and not part of the default installation.\n\
dnl The same goes for BeOS.\n\
dnl We don\'t want to link against libm or libpthread on Darwin since\n\
dnl they both are just symlinks to libSystem and explicitly linking\n\
dnl against libSystem causes issues when debugging (see bug 299601).\n\
case $target in\n\
*-darwin*)\n\
    ;;\n\
*-beos*)\n\
    ;;\n\
*)\n\
    AC_CHECK_LIB(m, atan)\n\
    AC_CHECK_LIB(dl, dlopen,\n\
    AC_CHECK_HEADER(dlfcn.h, \n\
        LIBS=\"-ldl $LIBS\"\n\
        AC_DEFINE(HAVE_LIBDL)))\n\
    ;;\n\
esac\n\
if test ! \"$GNU_CXX\"; then\n\
\n\
    case $target in\n\
    *-aix*)\n\
	AC_CHECK_LIB(C_r, demangle)\n\
	;;\n\
     *)\n\
	AC_CHECK_LIB(C, demangle)\n\
	;;\n\
     esac\n\
fi\n\
AC_CHECK_LIB(socket, socket)\n\
\n\
XLDFLAGS=\"$X_LIBS\"\n\
XLIBS=\"$X_EXTRA_LIBS\"\n\
\n\
dnl ========================================================\n\
dnl Checks for X libraries.\n\
dnl Ordering is important.\n\
dnl Xt is dependent upon SM as of X11R6\n\
dnl ========================================================\n\
if test \"$no_x\" = \"yes\"; then\n\
    AC_DEFINE(NO_X11)\n\
else\n\
    AC_DEFINE_UNQUOTED(FUNCPROTO,15)\n\
	XLIBS=\"-lX11 $XLIBS\"\n\
	_SAVE_LDFLAGS=\"$LDFLAGS\"\n\
	LDFLAGS=\"$XLDFLAGS $LDFLAGS\"\n\
	AC_CHECK_LIB(X11, XDrawLines, [X11_LIBS=\"-lX11\"],\n\
		[MISSING_X=\"$MISSING_X -lX11\"], $XLIBS)\n\
	AC_CHECK_LIB(Xext, XextAddDisplay, [XEXT_LIBS=\"-lXext\"],\n\
		[MISSING_X=\"$MISSING_X -lXext\"], $XLIBS)\n\
   \n\
     \n\
	AC_CHECK_LIB(Xt, XtFree, [ XT_LIBS=\"-lXt\"], [\n\
        unset ac_cv_lib_Xt_XtFree\n\
	    AC_CHECK_LIB(ICE, IceFlush, [XT_LIBS=\"-lICE $XT_LIBS\"],, $XT_LIBS $XLIBS)\n\
	    AC_CHECK_LIB(SM, SmcCloseConnection, [XT_LIBS=\"-lSM $XT_LIBS\"],, $XT_LIBS $XLIBS) \n\
        AC_CHECK_LIB(Xt, XtFree, [ XT_LIBS=\"-lXt $XT_LIBS\"],\n\
		    [MISSING_X=\"$MISSING_X -lXt\"], $X_PRE_LIBS $XT_LIBS $XLIBS)\n\
        ])\n\
\n\
    # AIX needs the motif library linked before libXt to prevent\n\
    # crashes in plugins linked against Motif - Bug #98892\n\
    case \"${target_os}\" in\n\
    aix*)\n\
        XT_LIBS=\"-lXm $XT_LIBS\"\n\
        ;;\n\
    esac\n\
\n\
    dnl ========================================================\n\
    dnl = Check for Xinerama\n\
    dnl ========================================================\n\
    AC_CHECK_LIB(Xinerama, XineramaIsActive, [MOZ_XINERAMA_LIBS=\"-lXinerama\"],,\n\
        $XLIBS $XEXT_LIBS)\n\
    AC_CHECK_HEADER(X11/extensions/Xinerama.h)\n\
\n\
    dnl ========================================================\n\
    dnl = Check for XShm\n\
    dnl ========================================================\n\
    AC_CHECK_LIB(Xext, XShmCreateImage, _HAVE_XSHM_XEXT=1,,\n\
        $XLIBS $XEXT_LIBS)\n\
    AC_CHECK_HEADER(X11/extensions/XShm.h)\n\
    if test -n \"$ac_cv_header_X11_extensions_XShm_h\" &&\n\
        test -n \"$_HAVE_XSHM_XEXT\"; then\n\
        AC_DEFINE(HAVE_XSHM)\n\
    fi\n\
\n\
    dnl ========================================================\n\
    dnl = Check for XIE\n\
    dnl ========================================================\n\
    AC_CHECK_LIB(XIE, XieFloGeometry, [MOZ_XIE_LIBS=\"-lXIE\"],,\n\
        $XLIBS $XEXT_LIBS)\n\
    AC_CHECK_HEADER(X11/extensions/XIElib.h)\n\
\n\
    if test \"$MOZ_XIE_LIBS\"; then\n\
	dnl ====================================================\n\
	dnl = If XIE is present and is desired, turn it on\n\
	dnl ====================================================\n\
	case $target in\n\
	    *-hpux*)\n\
		;;\n\
	    *)\n\
		HAVE_XIE=1\n\
		;;\n\
	esac\n\
    fi\n\
\n\
	LDFLAGS=\"$_SAVE_LDFLAGS\"\n\
\n\
    AC_CHECK_FT2(6.1.0, [_HAVE_FREETYPE2=1], [_HAVE_FREETYPE2=])\n\
\n\
fi # $no_x\n\
\n\
AC_SUBST(XCFLAGS)\n\
AC_SUBST(XLDFLAGS)\n\
AC_SUBST(XLIBS)\n\
AC_SUBST(XT_LIBS)\n\
\n\
dnl ========================================================\n\
dnl = pthread support\n\
dnl = Start by checking whether the system support pthreads\n\
dnl ========================================================\n\
case \"$target_os\" in\n\
darwin*)\n\
    USE_PTHREADS=1\n\
    ;;\n\
*)\n\
    MOZ_CHECK_PTHREADS(pthreads,\n\
        USE_PTHREADS=1 _PTHREAD_LDFLAGS=\"-lpthreads\",\n\
        MOZ_CHECK_PTHREADS(pthread,\n\
            USE_PTHREADS=1 _PTHREAD_LDFLAGS=\"-lpthread\",\n\
            MOZ_CHECK_PTHREADS(c_r,\n\
                USE_PTHREADS=1 _PTHREAD_LDFLAGS=\"-lc_r\",\n\
                MOZ_CHECK_PTHREADS(c,\n\
                    USE_PTHREADS=1\n\
                )\n\
            )\n\
        )\n\
    )\n\
    ;;\n\
esac\n\
\n\
dnl ========================================================\n\
dnl Check the command line for --with-pthreads \n\
dnl ========================================================\n\
MOZ_ARG_WITH_BOOL(pthreads,\n\
[  --with-pthreads         Force use of system pthread library with NSPR ],\n\
[ if test \"$USE_PTHREADS\"x = x; then\n\
    AC_MSG_ERROR([ --with-pthreads specified for a system without pthread support ]);\n\
fi],\n\
    USE_PTHREADS=\n\
    _PTHREAD_LDFLAGS=\n\
)\n\
\n\
dnl ========================================================\n\
dnl Do the platform specific pthread hackery\n\
dnl ========================================================\n\
if test \"$USE_PTHREADS\"x != x\n\
then\n\
	dnl\n\
	dnl See if -pthread is supported.\n\
	dnl\n\
	rm -f conftest*\n\
	ac_cv_have_dash_pthread=no\n\
	AC_MSG_CHECKING(whether ${CC-cc} accepts -pthread)\n\
	echo \'int main() { return 0; }\' | cat > conftest.c\n\
	${CC-cc} -pthread -o conftest conftest.c > conftest.out 2>&1\n\
	if test $? -eq 0; then\n\
		if test -z \"`egrep -i \'(unrecognize|unknown)\' conftest.out | grep pthread`\" && test -z \"`egrep -i \'(error|incorrect)\' conftest.out`\" ; then\n\
			ac_cv_have_dash_pthread=yes\n\
	        case \"$target_os\" in\n\
	        freebsd*)\n\
# Freebsd doesn\'t use -pthread for compiles, it uses them for linking\n\
                ;;\n\
	        *)\n\
			    CFLAGS=\"$CFLAGS -pthread\"\n\
			    CXXFLAGS=\"$CXXFLAGS -pthread\"\n\
                ;;\n\
	        esac\n\
		fi\n\
	fi\n\
	rm -f conftest*\n\
    AC_MSG_RESULT($ac_cv_have_dash_pthread)\n\
\n\
	dnl\n\
	dnl See if -pthreads is supported.\n\
	dnl\n\
    ac_cv_have_dash_pthreads=no\n\
    if test \"$ac_cv_have_dash_pthread\" = \"no\"; then\n\
	    AC_MSG_CHECKING(whether ${CC-cc} accepts -pthreads)\n\
    	echo \'int main() { return 0; }\' | cat > conftest.c\n\
	    ${CC-cc} -pthreads -o conftest conftest.c > conftest.out 2>&1\n\
    	if test $? -eq 0; then\n\
	    	if test -z \"`egrep -i \'(unrecognize|unknown)\' conftest.out | grep pthreads`\" && test -z \"`egrep -i \'(error|incorrect)\' conftest.out`\" ; then\n\
			    ac_cv_have_dash_pthreads=yes\n\
			    CFLAGS=\"$CFLAGS -pthreads\"\n\
			    CXXFLAGS=\"$CXXFLAGS -pthreads\"\n\
		    fi\n\
	    fi\n\
	    rm -f conftest*\n\
    	AC_MSG_RESULT($ac_cv_have_dash_pthreads)\n\
    fi\n\
\n\
	case \"$target\" in\n\
	    *-*-freebsd*)\n\
			AC_DEFINE(_REENTRANT)\n\
			AC_DEFINE(_THREAD_SAFE)\n\
			dnl -pthread links in -lc_r, so don\'t specify it explicitly.\n\
			if test \"$ac_cv_have_dash_pthread\" = \"yes\"; then\n\
				_PTHREAD_LDFLAGS=\"-pthread\"\n\
			else\n\
				_PTHREAD_LDFLAGS=\"-lc_r\"\n\
			fi\n\
			;;\n\
\n\
	    *-*-openbsd*|*-*-bsdi*)\n\
			AC_DEFINE(_REENTRANT)\n\
			AC_DEFINE(_THREAD_SAFE)\n\
			dnl -pthread links in -lc_r, so don\'t specify it explicitly.\n\
			if test \"$ac_cv_have_dash_pthread\" = \"yes\"; then\n\
                _PTHREAD_LDFLAGS=\"-pthread\"\n\
			fi\n\
			;;\n\
\n\
	    *-*-linux*) \n\
			AC_DEFINE(_REENTRANT) \n\
			;;\n\
\n\
	    *-*-nto*) \n\
			AC_DEFINE(_REENTRANT) \n\
			;;\n\
\n\
	    *-aix4.3*|*-aix5*)\n\
			AC_DEFINE(_REENTRANT) \n\
			;;\n\
\n\
	    *-hpux11.*)\n\
			AC_DEFINE(_REENTRANT) \n\
			;;\n\
\n\
	    alpha*-*-osf*)\n\
			AC_DEFINE(_REENTRANT)\n\
			;;\n\
\n\
	    *-*-solaris*) \n\
    			AC_DEFINE(_REENTRANT) \n\
			if test ! \"$GNU_CC\"; then\n\
				CFLAGS=\"$CFLAGS -mt\" \n\
				CXXFLAGS=\"$CXXFLAGS -mt\" \n\
			fi\n\
			;;\n\
	esac\n\
    LDFLAGS=\"${_PTHREAD_LDFLAGS} ${LDFLAGS}\"\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Check for MacOS deployment target version\n\
dnl ========================================================\n\
\n\
MOZ_ARG_ENABLE_STRING(macos-target,\n\
                      [  --enable-macos-target=VER (default=10.3/ppc, 10.4/x86)\n\
                          Set the minimum MacOS version needed at runtime],\n\
                      [_MACOSX_DEPLOYMENT_TARGET=$enableval])\n\
\n\
case \"$target\" in\n\
*-darwin*)\n\
    if test -n \"$_MACOSX_DEPLOYMENT_TARGET\" ; then\n\
        dnl Use the specified value\n\
        export MACOSX_DEPLOYMENT_TARGET=$_MACOSX_DEPLOYMENT_TARGET\n\
    elif test -z \"$MACOSX_DEPLOYMENT_TARGET\" ; then\n\
        dnl No value specified on the command line or in the environment,\n\
        dnl use the lesser of the application\'s minimum or the architecture\'s\n\
        dnl minimum.\n\
        case \"${target_cpu}\" in\n\
            powerpc*)\n\
                dnl Architecture minimum 10.3\n\
                dnl export MACOSX_DEPLOYMENT_TARGET=10.3\n\
                export MACOSX_DEPLOYMENT_TARGET=10.2\n\
                ;;\n\
            i*86*)\n\
                dnl Architecture minimum 10.4\n\
                export MACOSX_DEPLOYMENT_TARGET=10.4\n\
                ;;\n\
        esac\n\
    fi\n\
    ;;\n\
esac\n\
\n\
AC_SUBST(MACOSX_DEPLOYMENT_TARGET)\n\
\n\
dnl ========================================================\n\
dnl See if mmap sees writes\n\
dnl For cross compiling, just define it as no, which is a safe default\n\
dnl ========================================================\n\
AC_MSG_CHECKING(whether mmap() sees write()s)\n\
\n\
changequote(,)\n\
mmap_test_prog=\'\n\
    #include <stdlib.h>\n\
    #include <unistd.h>\n\
    #include <sys/mman.h>\n\
    #include <sys/types.h>\n\
    #include <sys/stat.h>\n\
    #include <fcntl.h>\n\
\n\
    char fname[] = \"conftest.file\";\n\
    char zbuff[1024]; /* Fractional page is probably worst case */\n\
\n\
    int main() {\n\
	char *map;\n\
	int fd;\n\
	int i;\n\
	unlink(fname);\n\
	fd = open(fname, O_RDWR | O_CREAT, 0660);\n\
	if(fd<0) return 1;\n\
	unlink(fname);\n\
	write(fd, zbuff, sizeof(zbuff));\n\
	lseek(fd, 0, SEEK_SET);\n\
	map = (char*)mmap(0, sizeof(zbuff), PROT_READ, MAP_SHARED, fd, 0);\n\
	if(map==(char*)-1) return 2;\n\
	for(i=0; fname[i]; i++) {\n\
	    int rc = write(fd, &fname[i], 1);\n\
	    if(map[i]!=fname[i]) return 4;\n\
	}\n\
	return 0;\n\
    }\n\
\'\n\
changequote([,])\n\
\n\
AC_TRY_RUN($mmap_test_prog , result=\"yes\", result=\"no\", result=\"yes\")\n\
\n\
AC_MSG_RESULT(\"$result\")\n\
\n\
if test \"$result\" = \"no\"; then\n\
    AC_DEFINE(MMAP_MISSES_WRITES)\n\
fi\n\
\n\
\n\
dnl Checks for library functions.\n\
dnl ========================================================\n\
AC_PROG_GCC_TRADITIONAL\n\
AC_FUNC_MEMCMP\n\
AC_CHECK_FUNCS(random strerror lchown fchmod snprintf statvfs memmove rint stat64 lstat64)\n\
AC_CHECK_FUNCS(flockfile getpagesize)\n\
\n\
dnl localtime_r and strtok_r are only present on MacOS version 10.2 and higher\n\
if test -z \"$MACOS_DEPLOYMENT_TARGET\" || test \"$MACOS_DEPLOYMENT_TARGET\" -ge \"100200\"; then\n\
  AC_CHECK_FUNCS(localtime_r strtok_r)\n\
fi\n\
\n\
dnl check for wcrtomb/mbrtowc\n\
dnl =======================================================================\n\
if test -z \"$MACOS_DEPLOYMENT_TARGET\" || test \"$MACOS_DEPLOYMENT_TARGET\" -ge \"100300\"; then\n\
AC_LANG_SAVE\n\
AC_LANG_CPLUSPLUS\n\
AC_CACHE_CHECK(for wcrtomb,\n\
    ac_cv_have_wcrtomb,\n\
    [AC_TRY_LINK([#include <wchar.h>],\n\
                 [mbstate_t ps={0};wcrtomb(0,\'f\',&ps);],\n\
                 ac_cv_have_wcrtomb=\"yes\",\n\
                 ac_cv_have_wcrtomb=\"no\")])\n\
if test \"$ac_cv_have_wcrtomb\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_WCRTOMB)\n\
fi\n\
AC_CACHE_CHECK(for mbrtowc,\n\
    ac_cv_have_mbrtowc,\n\
    [AC_TRY_LINK([#include <wchar.h>],\n\
                 [mbstate_t ps={0};mbrtowc(0,0,0,&ps);],\n\
                 ac_cv_have_mbrtowc=\"yes\",\n\
                 ac_cv_have_mbrtowc=\"no\")])\n\
if test \"$ac_cv_have_mbrtowc\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_MBRTOWC)\n\
fi\n\
AC_LANG_RESTORE\n\
fi\n\
\n\
AC_CACHE_CHECK(\n\
    [for res_ninit()],\n\
    ac_cv_func_res_ninit,\n\
    [AC_TRY_LINK([\n\
        #ifdef linux\n\
        #define _BSD_SOURCE 1\n\
        #endif\n\
        #include <resolv.h>\n\
        ],\n\
        [int foo = res_ninit(&_res);],\n\
        [ac_cv_func_res_ninit=yes],\n\
        [ac_cv_func_res_ninit=no])\n\
    ])\n\
\n\
if test \"$ac_cv_func_res_ninit\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_RES_NINIT)\n\
dnl must add the link line we do something as foolish as this... dougt\n\
dnl else\n\
dnl    AC_CHECK_LIB(bind, res_ninit, AC_DEFINE(HAVE_RES_NINIT),\n\
dnl        AC_CHECK_LIB(resolv, res_ninit, AC_DEFINE(HAVE_RES_NINIT)))\n\
fi\n\
\n\
AC_LANG_CPLUSPLUS\n\
AC_CACHE_CHECK(\n\
    [for gnu_get_libc_version()],\n\
    ac_cv_func_gnu_get_libc_version,\n\
    [AC_TRY_LINK([\n\
        #ifdef HAVE_GNU_LIBC_VERSION_H\n\
        #include <gnu/libc-version.h>\n\
        #endif\n\
        ],\n\
        [const char *glibc_version = gnu_get_libc_version();],\n\
        [ac_cv_func_gnu_get_libc_version=yes],\n\
        [ac_cv_func_gnu_get_libc_version=no] \n\
        )]\n\
    )\n\
\n\
if test \"$ac_cv_func_gnu_get_libc_version\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_GNU_GET_LIBC_VERSION)\n\
fi\n\
\n\
case $target_os in\n\
    os2*|msvc*|mks*|cygwin*|mingw*|darwin*|wince*|beos*)\n\
        ;;\n\
    *)\n\
    \n\
AC_CHECK_LIB(c, iconv, [_ICONV_LIBS=\"$_ICONV_LIBS\"],\n\
    AC_CHECK_LIB(iconv, iconv, [_ICONV_LIBS=\"$_ICONV_LIBS -liconv\"],\n\
        AC_CHECK_LIB(iconv, libiconv, [_ICONV_LIBS=\"$_ICONV_LIBS -liconv\"])))\n\
_SAVE_LIBS=$LIBS\n\
LIBS=\"$LIBS $_ICONV_LIBS\"\n\
AC_CACHE_CHECK(\n\
    [for iconv()],\n\
    ac_cv_func_iconv,\n\
    [AC_TRY_LINK([\n\
        #include <stdlib.h>\n\
        #include <iconv.h>\n\
        ],\n\
        [\n\
            iconv_t h = iconv_open(\"\", \"\");\n\
            iconv(h, NULL, NULL, NULL, NULL);\n\
            iconv_close(h);\n\
        ],\n\
        [ac_cv_func_iconv=yes],\n\
        [ac_cv_func_iconv=no] \n\
        )]\n\
    )\n\
if test \"$ac_cv_func_iconv\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_ICONV)\n\
    DYNAMIC_XPCOM_LIBS=\"$DYNAMIC_XPCOM_LIBS $_ICONV_LIBS\"\n\
    LIBXUL_LIBS=\"$LIBXUL_LIBS $_ICONV_LIBS\"\n\
    LIBICONV=\"$_ICONV_LIBS\"\n\
    AC_CACHE_CHECK(\n\
        [for iconv() with const input],\n\
        ac_cv_func_const_iconv,\n\
        [AC_TRY_COMPILE([\n\
            #include <stdlib.h>\n\
            #include <iconv.h>\n\
            ],\n\
            [\n\
                const char *input = \"testing\";\n\
                iconv_t h = iconv_open(\"\", \"\");\n\
                iconv(h, &input, NULL, NULL, NULL);\n\
                iconv_close(h);\n\
            ],\n\
            [ac_cv_func_const_iconv=yes],\n\
            [ac_cv_func_const_iconv=no] \n\
            )]\n\
        )\n\
    if test \"$ac_cv_func_const_iconv\" = \"yes\"; then\n\
        AC_DEFINE(HAVE_ICONV_WITH_CONST_INPUT)\n\
    fi\n\
fi\n\
LIBS=$_SAVE_LIBS\n\
\n\
    ;;\n\
esac\n\
\n\
AM_LANGINFO_CODESET\n\
\n\
AC_LANG_C\n\
\n\
dnl **********************\n\
dnl *** va_copy checks ***\n\
dnl **********************\n\
dnl we currently check for all three va_copy possibilities, so we get\n\
dnl all results in config.log for bug reports.\n\
AC_MSG_CHECKING(for an implementation of va_copy())\n\
AC_CACHE_VAL(ac_cv_va_copy,[\n\
    AC_TRY_RUN([\n\
        #include <stdarg.h>\n\
        void f (int i, ...) {\n\
            va_list args1, args2;\n\
            va_start (args1, i);\n\
            va_copy (args2, args1);\n\
            if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)\n\
                exit (1);\n\
            va_end (args1); va_end (args2);\n\
        }\n\
        int main() { f (0, 42); return 0; }],\n\
        ac_cv_va_copy=yes,\n\
        ac_cv_va_copy=no,\n\
        ac_cv_va_copy=no\n\
    )\n\
])\n\
AC_MSG_RESULT($ac_cv_va_copy)\n\
AC_MSG_CHECKING(for an implementation of __va_copy())\n\
AC_CACHE_VAL(ac_cv___va_copy,[\n\
    AC_TRY_RUN([\n\
        #include <stdarg.h>\n\
        void f (int i, ...) {\n\
            va_list args1, args2;\n\
            va_start (args1, i);\n\
            __va_copy (args2, args1);\n\
            if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)\n\
                exit (1);\n\
            va_end (args1); va_end (args2);\n\
        }\n\
        int main() { f (0, 42); return 0; }],\n\
        ac_cv___va_copy=yes,\n\
        ac_cv___va_copy=no,\n\
        ac_cv___va_copy=no\n\
    )\n\
])\n\
AC_MSG_RESULT($ac_cv___va_copy)\n\
AC_MSG_CHECKING(whether va_lists can be copied by value)\n\
AC_CACHE_VAL(ac_cv_va_val_copy,[\n\
    AC_TRY_RUN([\n\
        #include <stdarg.h>\n\
        void f (int i, ...) {\n\
            va_list args1, args2;\n\
            va_start (args1, i);\n\
            args2 = args1;\n\
            if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)\n\
                exit (1);\n\
            va_end (args1); va_end (args2);\n\
        }\n\
        int main() { f (0, 42); return 0; }],\n\
        ac_cv_va_val_copy=yes,\n\
        ac_cv_va_val_copy=no,\n\
        ac_cv_va_val_copy=yes\n\
    )\n\
])\n\
if test \"x$ac_cv_va_copy\" = \"xyes\"; then\n\
    AC_DEFINE(VA_COPY, va_copy)\n\
    AC_DEFINE(HAVE_VA_COPY)\n\
elif test \"x$ac_cv___va_copy\" = \"xyes\"; then\n\
    AC_DEFINE(VA_COPY, __va_copy)\n\
    AC_DEFINE(HAVE_VA_COPY)\n\
fi\n\
\n\
if test \"x$ac_cv_va_val_copy\" = \"xno\"; then\n\
   AC_DEFINE(HAVE_VA_LIST_AS_ARRAY)\n\
fi\n\
AC_MSG_RESULT($ac_cv_va_val_copy)\n\
\n\
dnl Check for dll-challenged libc\'s.\n\
dnl This check is apparently only needed for Linux.\n\
case \"$target\" in\n\
	*-linux*)\n\
	    dnl ===================================================================\n\
	    _curdir=`pwd`\n\
	    export _curdir\n\
	    rm -rf conftest* _conftest\n\
	    mkdir _conftest\n\
	    cat >> conftest.C <<\\EOF\n\
#include <stdio.h>\n\
#include <link.h>\n\
#include <dlfcn.h>\n\
#ifdef _dl_loaded\n\
void __dump_link_map(void) {\n\
  struct link_map *map = _dl_loaded;\n\
  while (NULL != map) {printf(\"0x%08x %s\\n\", map->l_addr, map->l_name); map = map->l_next;}\n\
}\n\
int main() {\n\
  dlopen(\"./conftest1.so\",RTLD_LAZY);\n\
  dlopen(\"./../_conftest/conftest1.so\",RTLD_LAZY);\n\
  dlopen(\"CURDIR/_conftest/conftest1.so\",RTLD_LAZY);\n\
  dlopen(\"CURDIR/_conftest/../_conftest/conftest1.so\",RTLD_LAZY);\n\
  __dump_link_map();\n\
}\n\
#else\n\
/* _dl_loaded isn\'t defined, so this should be either a libc5 (glibc1) system, or a glibc2 system that doesn\'t have the multiple load bug (i.e., RH6.0).*/\n\
int main() { printf(\"./conftest1.so\\n\"); }\n\
#endif\n\
EOF\n\
\n\
	    $PERL -p -i -e \"s/CURDIR/\\$ENV{_curdir}/g;\" conftest.C\n\
\n\
	    cat >> conftest1.C <<\\EOF\n\
#include <stdio.h>\n\
void foo(void) {printf(\"foo in dll called\\n\");}\n\
EOF\n\
	    ${CXX-g++} -fPIC -c -g conftest1.C\n\
	    ${CXX-g++} -shared -Wl,-h -Wl,conftest1.so -o conftest1.so conftest1.o\n\
	    ${CXX-g++} -g conftest.C -o conftest -ldl\n\
	    cp -f conftest1.so conftest _conftest\n\
	    cd _conftest\n\
	    if test `./conftest | grep conftest1.so | wc -l` -gt 1\n\
	    then\n\
		echo\n\
		echo \"*** Your libc has a bug that can result in loading the same dynamic\"\n\
		echo \"*** library multiple times.  This bug is known to be fixed in glibc-2.0.7-32\"\n\
		echo \"*** or later.  However, if you choose not to upgrade, the only effect\"\n\
		echo \"*** will be excessive memory usage at runtime.\"\n\
		echo\n\
	    fi\n\
	    cd ${_curdir}\n\
	    rm -rf conftest* _conftest\n\
	    dnl ===================================================================\n\
	    ;;\n\
esac\n\
\n\
dnl ===================================================================\n\
dnl ========================================================\n\
dnl By default, turn rtti and exceptions off on g++/egcs\n\
dnl ========================================================\n\
if test \"$GNU_CXX\"; then\n\
\n\
  AC_MSG_CHECKING(for C++ exceptions flag)\n\
\n\
  dnl They changed -f[no-]handle-exceptions to -f[no-]exceptions in g++ 2.8\n\
  AC_CACHE_VAL(ac_cv_cxx_exceptions_flags,\n\
  [echo \"int main() { return 0; }\" | cat > conftest.C\n\
\n\
  ${CXX-g++} ${CXXFLAGS} -c -fno-handle-exceptions conftest.C > conftest.out 2>&1\n\
\n\
  if egrep \"warning.*renamed\" conftest.out >/dev/null; then\n\
    ac_cv_cxx_exceptions_flags=${_COMPILER_PREFIX}-fno-exceptions\n\
  else\n\
    ac_cv_cxx_exceptions_flags=${_COMPILER_PREFIX}-fno-handle-exceptions\n\
  fi\n\
\n\
  rm -f conftest*])\n\
\n\
  AC_MSG_RESULT($ac_cv_cxx_exceptions_flags)\n\
  _MOZ_EXCEPTIONS_FLAGS_OFF=$ac_cv_cxx_exceptions_flags\n\
  _MOZ_EXCEPTIONS_FLAGS_ON=`echo $ac_cv_cxx_exceptions_flags | sed \'s|no-||\'`\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Put your C++ language/feature checks below\n\
dnl ========================================================\n\
AC_LANG_CPLUSPLUS\n\
\n\
HAVE_GCC3_ABI=\n\
if test \"$GNU_CC\"; then\n\
  AC_CACHE_CHECK(for gcc 3.0 ABI,\n\
      ac_cv_gcc_three_abi,\n\
      [AC_TRY_COMPILE([],\n\
                      [\n\
#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100 /* G++ V3 ABI */\n\
  return 0;\n\
#else\n\
#error Not gcc3.\n\
#endif\n\
                      ],\n\
                      ac_cv_gcc_three_abi=\"yes\",\n\
                      ac_cv_gcc_three_abi=\"no\")])\n\
  if test \"$ac_cv_gcc_three_abi\" = \"yes\"; then\n\
      TARGET_COMPILER_ABI=\"${TARGET_COMPILER_ABI-gcc3}\"\n\
      HAVE_GCC3_ABI=1\n\
  else\n\
      TARGET_COMPILER_ABI=\"${TARGET_COMPILER_ABI-gcc2}\"\n\
  fi\n\
fi\n\
AC_SUBST(HAVE_GCC3_ABI)\n\
\n\
\n\
AC_CACHE_CHECK(for C++ \\\"explicit\\\" keyword,\n\
               ac_cv_cpp_explicit,\n\
               [AC_TRY_COMPILE(class X {\n\
                               public: explicit X(int i) : i_(i) {}\n\
                               private: int i_;\n\
                               };,\n\
                               X x(3);,\n\
                               ac_cv_cpp_explicit=yes,\n\
                               ac_cv_cpp_explicit=no)])\n\
if test \"$ac_cv_cpp_explicit\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_EXPLICIT)\n\
fi\n\
\n\
AC_CACHE_CHECK(for C++ \\\"typename\\\" keyword,\n\
               ac_cv_cpp_typename,\n\
               [AC_TRY_COMPILE(class param {\n\
                               public:\n\
                                   typedef unsigned long num_type;\n\
                               };\n\
\n\
                               template <class T> class tplt {\n\
                               public:\n\
                                   typedef typename T::num_type t_num_type;\n\
                                   t_num_type foo(typename T::num_type num) {\n\
                                       return num;\n\
                                   }\n\
                               };,\n\
                               tplt<param> A;\n\
                               A.foo(0);,\n\
                               ac_cv_cpp_typename=yes,\n\
                               ac_cv_cpp_typename=no)])\n\
if test \"$ac_cv_cpp_typename\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_TYPENAME)\n\
fi\n\
\n\
dnl Check for support of modern template specialization syntax\n\
dnl Test code and requirement from scc@netscape.com.\n\
dnl Autoconf cut-and-paste job by waterson@netscape.com\n\
AC_CACHE_CHECK(for modern C++ template specialization syntax support,\n\
               ac_cv_cpp_modern_specialize_template_syntax,\n\
               [AC_TRY_COMPILE(template <class T> struct X { int a; };\n\
                               class Y {};\n\
                               template <> struct X<Y> { double a; };,\n\
                               X<int> int_x;\n\
                               X<Y> y_x;,\n\
                               ac_cv_cpp_modern_specialize_template_syntax=yes,\n\
                               ac_cv_cpp_modern_specialize_template_syntax=no)])\n\
if test \"$ac_cv_cpp_modern_specialize_template_syntax\" = yes ; then\n\
  AC_DEFINE(HAVE_CPP_MODERN_SPECIALIZE_TEMPLATE_SYNTAX)\n\
fi\n\
\n\
\n\
dnl Some compilers support only full specialization, and some don\'t.\n\
AC_CACHE_CHECK(whether partial template specialization works,\n\
               ac_cv_cpp_partial_specialization,\n\
               [AC_TRY_COMPILE(template <class T> class Foo {};\n\
                               template <class T> class Foo<T*> {};,\n\
                               return 0;,\n\
                               ac_cv_cpp_partial_specialization=yes,\n\
                               ac_cv_cpp_partial_specialization=no)])\n\
if test \"$ac_cv_cpp_partial_specialization\" = yes ; then\n\
  AC_DEFINE(HAVE_CPP_PARTIAL_SPECIALIZATION)\n\
fi\n\
\n\
dnl Some compilers have limited support for operators with templates;\n\
dnl specifically, it is necessary to define derived operators when a base\n\
dnl class\'s operator declaration should suffice.\n\
AC_CACHE_CHECK(whether operators must be re-defined for templates derived from templates,\n\
               ac_cv_need_derived_template_operators,\n\
               [AC_TRY_COMPILE([template <class T> class Base { };\n\
                                template <class T>\n\
                                Base<T> operator+(const Base<T>& lhs, const Base<T>& rhs) { return lhs; }\n\
                                template <class T> class Derived : public Base<T> { };],\n\
                               [Derived<char> a, b;\n\
                                Base<char> c = a + b;\n\
                                return 0;],\n\
                               ac_cv_need_derived_template_operators=no,\n\
                               ac_cv_need_derived_template_operators=yes)])\n\
if test \"$ac_cv_need_derived_template_operators\" = yes ; then\n\
  AC_DEFINE(NEED_CPP_DERIVED_TEMPLATE_OPERATORS)\n\
fi\n\
\n\
\n\
dnl Some compilers have trouble detecting that a template class\n\
dnl that derives from another template is actually an instance\n\
dnl of the base class. This test checks for that.\n\
AC_CACHE_CHECK(whether we need to cast a derived template to pass as its base class,\n\
               ac_cv_need_cpp_template_cast_to_base,\n\
               [AC_TRY_COMPILE([template <class T> class Base { };\n\
                                template <class T> class Derived : public Base<T> { };\n\
                                template <class T> int foo(const Base<T>&) { return 0; }],\n\
                               [Derived<char> bar; return foo(bar);],\n\
                               ac_cv_need_cpp_template_cast_to_base=no,\n\
                               ac_cv_need_cpp_template_cast_to_base=yes)])\n\
if test \"$ac_cv_need_cpp_template_cast_to_base\" = yes ; then\n\
  AC_DEFINE(NEED_CPP_TEMPLATE_CAST_TO_BASE)\n\
fi\n\
\n\
dnl Some compilers have trouble resolving the ambiguity between two\n\
dnl functions whose arguments differ only by cv-qualifications.\n\
AC_CACHE_CHECK(whether the compiler can resolve const ambiguities for templates,\n\
               ac_cv_can_resolve_const_ambiguity,\n\
               [AC_TRY_COMPILE([\n\
                                template <class T> class ptrClass {\n\
                                  public: T* ptr;\n\
                                };\n\
\n\
                                template <class T> T* a(ptrClass<T> *arg) {\n\
                                  return arg->ptr;\n\
                                }\n\
\n\
                                template <class T>\n\
                                const T* a(const ptrClass<T> *arg) {\n\
                                  return arg->ptr;\n\
                                }\n\
                               ],\n\
                               [ ptrClass<int> i;\n\
                                 a(&i); ],\n\
                               ac_cv_can_resolve_const_ambiguity=yes,\n\
                               ac_cv_can_resolve_const_ambiguity=no)])\n\
if test \"$ac_cv_can_resolve_const_ambiguity\" = no ; then\n\
  AC_DEFINE(CANT_RESOLVE_CPP_CONST_AMBIGUITY)\n\
fi\n\
\n\
dnl\n\
dnl We don\'t do exceptions on unix.  The only reason this used to be here\n\
dnl is that mozilla/xpcom/tests/TestCOMPtr.cpp has a test which uses \n\
dnl exceptions.  But, we turn exceptions off by default and this test breaks.\n\
dnl So im commenting this out until someone writes some artificial \n\
dnl intelligence to detect not only if the compiler has exceptions, but if \n\
dnl they are enabled as well.\n\
dnl \n\
dnl AC_CACHE_CHECK(for C++ \\\"exceptions\\\",\n\
dnl                ac_cv_cpp_exceptions,\n\
dnl                [AC_TRY_COMPILE(class X { public: X() {} };\n\
dnl                                static void F() { throw X(); },\n\
dnl                                try { F(); } catch(X & e) { },\n\
dnl                                ac_cv_cpp_exceptions=yes,\n\
dnl                                ac_cv_cpp_exceptions=no)])\n\
dnl if test $ac_cv_cpp_exceptions = yes ; then\n\
dnl    AC_DEFINE(HAVE_CPP_EXCEPTIONS)\n\
dnl fi\n\
\n\
dnl Some compilers have marginal |using| support; for example, gcc-2.7.2.3\n\
dnl supports it well enough to allow us to use it to change access, but not\n\
dnl to resolve ambiguity. The next two tests determine how well the |using|\n\
dnl keyword is supported.\n\
dnl\n\
dnl Check to see if we can change access with |using|.  Test both a\n\
dnl legal and an illegal example.\n\
AC_CACHE_CHECK(whether the C++ \\\"using\\\" keyword can change access,\n\
               ac_cv_cpp_access_changing_using2,\n\
               [AC_TRY_COMPILE(\n\
                   class A { protected: int foo() { return 0; } };\n\
                   class B : public A { public: using A::foo; };,\n\
                   B b; return b.foo();,\n\
                   [AC_TRY_COMPILE(\n\
                       class A { public: int foo() { return 1; } };\n\
                       class B : public A { private: using A::foo; };,\n\
                       B b; return b.foo();,\n\
                       ac_cv_cpp_access_changing_using2=no,\n\
                       ac_cv_cpp_access_changing_using2=yes)],\n\
                   ac_cv_cpp_access_changing_using2=no)])\n\
if test \"$ac_cv_cpp_access_changing_using2\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_ACCESS_CHANGING_USING)\n\
fi\n\
\n\
dnl Check to see if we can resolve ambiguity with |using|.\n\
AC_CACHE_CHECK(whether the C++ \\\"using\\\" keyword resolves ambiguity,\n\
               ac_cv_cpp_ambiguity_resolving_using,\n\
               [AC_TRY_COMPILE(class X { \n\
                                 public: int go(const X&) {return 3;}\n\
                                         int jo(const X&) {return 3;}\n\
                               };\n\
                               class Y : public X {\n\
                                 public:  int go(int) {return 2;}\n\
                                          int jo(int) {return 2;}\n\
                                          using X::jo;\n\
                                 private: using X::go;\n\
                               };,\n\
                               X x; Y y; y.jo(x);,\n\
                               ac_cv_cpp_ambiguity_resolving_using=yes,\n\
                               ac_cv_cpp_ambiguity_resolving_using=no)])\n\
if test \"$ac_cv_cpp_ambiguity_resolving_using\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_AMBIGUITY_RESOLVING_USING)\n\
fi\n\
\n\
dnl Check to see if the |std| namespace is supported. If so, we\'ll want\n\
dnl to qualify any standard library calls with \"std::\" to ensure that\n\
dnl those functions can be resolved.\n\
AC_CACHE_CHECK(for \\\"std::\\\" namespace,\n\
               ac_cv_cpp_namespace_std,\n\
               [AC_TRY_COMPILE([#include <algorithm>],\n\
                               [return std::min(0, 1);],\n\
                               ac_cv_cpp_namespace_std=yes,\n\
                               ac_cv_cpp_namespace_std=no)])\n\
if test \"$ac_cv_cpp_namespace_std\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_NAMESPACE_STD)\n\
fi\n\
\n\
dnl Older compilers are overly ambitious with respect to using the standard\n\
dnl template library\'s |operator!=()| when |operator==()| is defined. In\n\
dnl which case, defining |operator!=()| in addition to |operator==()| causes\n\
dnl ambiguity at compile-time. This test checks for that case.\n\
AC_CACHE_CHECK(whether standard template operator!=() is ambiguous,\n\
               ac_cv_cpp_unambiguous_std_notequal,\n\
               [AC_TRY_COMPILE([#include <algorithm>\n\
                                struct T1 {};\n\
                                int operator==(const T1&, const T1&) { return 0; }\n\
                                int operator!=(const T1&, const T1&) { return 0; }],\n\
                               [T1 a,b; return a != b;],\n\
                               ac_cv_cpp_unambiguous_std_notequal=unambiguous,\n\
                               ac_cv_cpp_unambiguous_std_notequal=ambiguous)])\n\
if test \"$ac_cv_cpp_unambiguous_std_notequal\" = unambiguous ; then\n\
  AC_DEFINE(HAVE_CPP_UNAMBIGUOUS_STD_NOTEQUAL)\n\
fi\n\
\n\
\n\
AC_CACHE_CHECK(for C++ reinterpret_cast,\n\
               ac_cv_cpp_reinterpret_cast,\n\
               [AC_TRY_COMPILE(struct X { int i; };\n\
                               struct Y { int i; };,\n\
                               X x; X*const z = &x;Y*y = reinterpret_cast<Y*>(z);,\n\
                               ac_cv_cpp_reinterpret_cast=yes,\n\
                               ac_cv_cpp_reinterpret_cast=no)])\n\
if test \"$ac_cv_cpp_reinterpret_cast\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_NEW_CASTS)\n\
fi\n\
\n\
dnl See if a dynamic_cast to void* gives the most derived object.\n\
AC_CACHE_CHECK(for C++ dynamic_cast to void*,\n\
               ac_cv_cpp_dynamic_cast_void_ptr,\n\
               [AC_TRY_RUN([class X { int i; public: virtual ~X() { } };\n\
                            class Y { int j; public: virtual ~Y() { } };\n\
                            class Z : public X, public Y { int k; };\n\
\n\
                            int main() {\n\
                                 Z mdo;\n\
                                 X *subx = (X*)&mdo;\n\
                                 Y *suby = (Y*)&mdo;\n\
                                 return !((((void*)&mdo != (void*)subx) &&\n\
                                           ((void*)&mdo == dynamic_cast<void*>(subx))) ||\n\
                                          (((void*)&mdo != (void*)suby) &&\n\
                                           ((void*)&mdo == dynamic_cast<void*>(suby))));\n\
                            }],\n\
                           ac_cv_cpp_dynamic_cast_void_ptr=yes,\n\
                           ac_cv_cpp_dynamic_cast_void_ptr=no,\n\
                           ac_cv_cpp_dynamic_cast_void_ptr=no)])\n\
if test \"$ac_cv_cpp_dynamic_cast_void_ptr\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_DYNAMIC_CAST_TO_VOID_PTR)\n\
fi\n\
\n\
\n\
dnl note that this one is reversed - if the test fails, then\n\
dnl we require implementations of unused virtual methods. Which\n\
dnl really blows because it means we\'ll have useless vtable\n\
dnl bloat.\n\
AC_CACHE_CHECK(whether C++ requires implementation of unused virtual methods,\n\
               ac_cv_cpp_unused_required,\n\
               [AC_TRY_LINK(class X {private: virtual void never_called();};,\n\
                               X x;,\n\
                               ac_cv_cpp_unused_required=no,\n\
                               ac_cv_cpp_unused_required=yes)])\n\
if test \"$ac_cv_cpp_unused_required\" = yes ; then\n\
   AC_DEFINE(NEED_CPP_UNUSED_IMPLEMENTATIONS)\n\
fi\n\
\n\
\n\
dnl Some compilers have trouble comparing a constant reference to a templatized\n\
dnl class to zero, and require an explicit operator==() to be defined that takes\n\
dnl an int. This test separates the strong from the weak.\n\
\n\
AC_CACHE_CHECK(for trouble comparing to zero near std::operator!=(),\n\
               ac_cv_trouble_comparing_to_zero,\n\
               [AC_TRY_COMPILE([#include <algorithm>\n\
                                template <class T> class Foo {};\n\
                                class T2;\n\
                                template <class T> int operator==(const T2*, const T&) { return 0; }\n\
                                template <class T> int operator!=(const T2*, const T&) { return 0; }],\n\
                               [Foo<int> f; return (0 != f);],\n\
                               ac_cv_trouble_comparing_to_zero=no,\n\
                               ac_cv_trouble_comparing_to_zero=yes)])\n\
if test \"$ac_cv_trouble_comparing_to_zero\" = yes ; then\n\
  AC_DEFINE(HAVE_CPP_TROUBLE_COMPARING_TO_ZERO)\n\
fi\n\
\n\
\n\
\n\
dnl End of C++ language/feature checks\n\
AC_LANG_C\n\
\n\
dnl ========================================================\n\
dnl =  Internationalization checks\n\
dnl ========================================================\n\
dnl\n\
dnl Internationalization and Locale support is different\n\
dnl on various UNIX platforms.  Checks for specific i18n\n\
dnl features go here.\n\
\n\
dnl check for LC_MESSAGES\n\
AC_CACHE_CHECK(for LC_MESSAGES,\n\
		ac_cv_i18n_lc_messages,\n\
		[AC_TRY_COMPILE([#include <locale.h>],\n\
				[int category = LC_MESSAGES;],\n\
				ac_cv_i18n_lc_messages=yes,\n\
				ac_cv_i18n_lc_messages=no)])\n\
if test \"$ac_cv_i18n_lc_messages\" = yes; then\n\
   AC_DEFINE(HAVE_I18N_LC_MESSAGES)\n\
fi 	\n\
\n\
fi # SKIP_COMPILER_CHECKS\n\
\n\
TARGET_XPCOM_ABI=\n\
if test -n \"${CPU_ARCH}\" -a -n \"${TARGET_COMPILER_ABI}\"; then\n\
    TARGET_XPCOM_ABI=\"${CPU_ARCH}-${TARGET_COMPILER_ABI}\"\n\
fi\n\
\n\
dnl Mozilla specific options\n\
dnl ========================================================\n\
dnl The macros used for command line options\n\
dnl are defined in build/autoconf/altoptions.m4.\n\
\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Check for external package dependencies\n\
dnl =\n\
dnl ========================================================\n\
MOZ_ARG_HEADER(External Packages)\n\
\n\
MOZ_ENABLE_LIBXUL=\n\
\n\
MOZ_ARG_WITH_STRING(libxul-sdk,\n\
[  --with-libxul-sdk=PFX   Use the libXUL SDK at <PFX>],\n\
  LIBXUL_SDK_DIR=$withval)\n\
\n\
if test \"$LIBXUL_SDK_DIR\" = \"yes\"; then\n\
    AC_MSG_ERROR([--with-libxul-sdk must specify a path])\n\
elif test -n \"$LIBXUL_SDK_DIR\" -a \"$LIBXUL_SDK_DIR\" != \"no\"; then\n\
    LIBXUL_SDK=`cd \"$LIBXUL_SDK_DIR\" && pwd`\n\
\n\
    if test ! -f \"$LIBXUL_SDK/sdk/include/xpcom-config.h\"; then\n\
        AC_MSG_ERROR([$LIBXUL_SDK/sdk/include/xpcom-config.h doesn\'t exist])\n\
    fi\n\
\n\
    MOZ_ENABLE_LIBXUL=1\n\
fi\n\
AC_SUBST(LIBXUL_SDK)\n\
\n\
dnl ========================================================\n\
dnl = If NSPR was not detected in the system, \n\
dnl = use the one in the source tree (mozilla/nsprpub)\n\
dnl ========================================================\n\
MOZ_ARG_WITH_BOOL(system-nspr,\n\
[  --with-system-nspr      Use system installed NSPR],\n\
    _USE_SYSTEM_NSPR=1 )\n\
\n\
if test -n \"$_USE_SYSTEM_NSPR\"; then\n\
    AM_PATH_NSPR(4.0.0, [MOZ_NATIVE_NSPR=1], [MOZ_NATIVE_NSPR=])\n\
fi\n\
\n\
if test -z \"$MOZ_NATIVE_NSPR\"; then\n\
    NSPR_CFLAGS=\'`$(DEPTH)/nsprpub/config/nspr-config --prefix=$(LIBXUL_DIST) --includedir=$(LIBXUL_DIST)/include/nspr --cflags`\'\n\
    # explicitly set libs for Visual Age C++ for OS/2\n\
    if test \"$OS_ARCH\" = \"OS2\" -a \"$VACPP\" = \"yes\"; then\n\
        NSPR_LIBS=\'$(LIBXUL_DIST)/lib/nspr\'$NSPR_VERSION\'.lib $(LIBXUL_DIST)/lib/plc\'$NSPR_VERSION\'.lib $(LIBXUL_DIST)/lib/plds\'$NSPR_VERSION\'.lib \'$_PTHREAD_LDFLAGS\'\'\n\
    elif test \"$OS_ARCH\" = \"WINCE\"; then\n\
        NSPR_CFLAGS=\'-I$(LIBXUL_DIST)/include/nspr\'\n\
        NSPR_LIBS=\'$(LIBXUL_DIST)/lib/nspr\'$NSPR_VERSION\'.lib $(LIBXUL_DIST)/lib/plc\'$NSPR_VERSION\'.lib $(LIBXUL_DIST)/lib/plds\'$NSPR_VERSION\'.lib \'\n\
    elif test \"$OS_ARCH\" = \"WINNT\"; then\n\
        NSPR_CFLAGS=\'-I$(LIBXUL_DIST)/include/nspr\'\n\
        if test -n \"$GNU_CC\"; then\n\
            NSPR_LIBS=\"-L\\$(LIBXUL_DIST)/lib -lnspr$NSPR_VERSION -lplc$NSPR_VERSION -lplds$NSPR_VERSION\"\n\
        else\n\
            NSPR_LIBS=\'$(LIBXUL_DIST)/lib/nspr\'$NSPR_VERSION\'.lib $(LIBXUL_DIST)/lib/plc\'$NSPR_VERSION\'.lib $(LIBXUL_DIST)/lib/plds\'$NSPR_VERSION\'.lib \'\n\
        fi\n\
    else\n\
        NSPR_LIBS=\'`$(DEPTH)/nsprpub/config/nspr-config --prefix=$(LIBXUL_DIST) --libdir=$(LIBXUL_DIST)/lib --libs`\'\n\
    fi\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = If NSS was not detected in the system, \n\
dnl = use the one in the source tree (mozilla/security/nss)\n\
dnl ========================================================\n\
\n\
MOZ_ARG_WITH_BOOL(system-nss,\n\
[  --with-system-nss      Use system installed NSS],\n\
    _USE_SYSTEM_NSS=1 )\n\
\n\
if test -n \"$_USE_SYSTEM_NSS\"; then\n\
    AM_PATH_NSS(3.0.0, [MOZ_NATIVE_NSS=1], [MOZ_NATIVE_NSS=])\n\
fi\n\
\n\
if test -n \"$MOZ_NATIVE_NSS\"; then\n\
   NSS_LIBS=\"$NSS_LIBS -lcrmf\"\n\
else\n\
   NSS_CFLAGS=\'-I$(LIBXUL_DIST)/public/nss\'\n\
   NSS_DEP_LIBS=\'\\\\\\\n\
        $(LIBXUL_DIST)/lib/$(LIB_PREFIX)crmf.$(LIB_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(DLL_PREFIX)smime\'$NSS_VERSION\'$(DLL_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(DLL_PREFIX)ssl\'$NSS_VERSION\'$(DLL_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(DLL_PREFIX)nss\'$NSS_VERSION\'$(DLL_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(DLL_PREFIX)softokn\'$NSS_VERSION\'$(DLL_SUFFIX)\'\n\
\n\
   if test -z \"$GNU_CC\" && test \"$OS_ARCH\" = \"WINNT\" -o \"$OS_ARCH\" = \"WINCE\" -o \"$OS_ARCH\" = \"OS2\"; then\n\
       NSS_LIBS=\'\\\\\\\n\
        $(LIBXUL_DIST)/lib/$(LIB_PREFIX)crmf.$(LIB_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(LIB_PREFIX)smime\'$NSS_VERSION\'.$(IMPORT_LIB_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(LIB_PREFIX)ssl\'$NSS_VERSION\'.$(IMPORT_LIB_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(LIB_PREFIX)nss\'$NSS_VERSION\'.$(IMPORT_LIB_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(LIB_PREFIX)softokn\'$NSS_VERSION\'.$(IMPORT_LIB_SUFFIX)\'\n\
   else\n\
       NSS_LIBS=\'$(LIBS_DIR)\'\" -lcrmf -lsmime$NSS_VERSION -lssl$NSS_VERSION -lnss$NSS_VERSION -lsoftokn$NSS_VERSION\"\n\
   fi\n\
fi\n\
\n\
if test -z \"$SKIP_LIBRARY_CHECKS\"; then\n\
dnl system JPEG support\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(system-jpeg,\n\
[  --with-system-jpeg[=PFX]\n\
                          Use system libjpeg [installed at prefix PFX]],\n\
    JPEG_DIR=$withval)\n\
\n\
_SAVE_CFLAGS=$CFLAGS\n\
_SAVE_LDFLAGS=$LDFLAGS\n\
_SAVE_LIBS=$LIBS\n\
if test -n \"${JPEG_DIR}\" -a \"${JPEG_DIR}\" != \"yes\"; then\n\
    CFLAGS=\"-I${JPEG_DIR}/include $CFLAGS\"\n\
    LDFLAGS=\"-L${JPEG_DIR}/lib $LDFLAGS\"\n\
fi\n\
if test -z \"$JPEG_DIR\" -o \"$JPEG_DIR\" = no; then\n\
    SYSTEM_JPEG=\n\
else\n\
    AC_CHECK_LIB(jpeg, jpeg_destroy_compress, [SYSTEM_JPEG=1 JPEG_LIBS=\"-ljpeg $JPEG_LIBS\"], SYSTEM_JPEG=, $JPEG_LIBS)\n\
fi\n\
\n\
if test \"$SYSTEM_JPEG\" = 1; then\n\
    LIBS=\"$JPEG_LIBS $LIBS\"\n\
    AC_TRY_RUN( \n\
	#include <stdio.h>\n\
	#include <sys/types.h>\n\
	#include <jpeglib.h>\n\
	int main () {\n\
	    #if JPEG_LIB_VERSION >= $MOZJPEG\n\
		exit(0);\n\
	    #else\n\
		exit(1);\n\
	    #endif\n\
	}\n\
	, SYSTEM_JPEG=1, [SYSTEM_JPEG= JPEG_CFLAGS= JPEG_LIBS=], SYSTEM_JPEG= ) \n\
    rm -f core\n\
fi \n\
CFLAGS=$_SAVE_CFLAGS\n\
LDFLAGS=$_SAVE_LDFLAGS\n\
LIBS=$_SAVE_LIBS\n\
\n\
if test -n \"${JPEG_DIR}\" -a -d \"${JPEG_DIR}\" -a \"$SYSTEM_JPEG\" = 1; then\n\
    JPEG_CFLAGS=\"-I${JPEG_DIR}/include\"\n\
    JPEG_LIBS=\"-L${JPEG_DIR}/lib ${JPEG_LIBS}\"\n\
fi\n\
\n\
dnl system ZLIB support\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(system-zlib,\n\
[  --with-system-zlib[=PFX]\n\
                          Use system libz [installed at prefix PFX]],\n\
    ZLIB_DIR=$withval)\n\
\n\
_SAVE_CFLAGS=$CFLAGS\n\
_SAVE_LDFLAGS=$LDFLAGS\n\
_SAVE_LIBS=$LIBS\n\
if test -n \"${ZLIB_DIR}\" -a \"${ZLIB_DIR}\" != \"yes\"; then\n\
    CFLAGS=\"-I${ZLIB_DIR}/include $CFLAGS\"\n\
    LDFLAGS=\"-L${ZLIB_DIR}/lib $LDFLAGS\"\n\
fi\n\
if test -z \"$ZLIB_DIR\" -o \"$ZLIB_DIR\" = no; then\n\
    SYSTEM_ZLIB=\n\
else\n\
    AC_CHECK_LIB(z, gzread, [SYSTEM_ZLIB=1 ZLIB_LIBS=\"-lz $ZLIB_LIBS\"], \n\
	[SYSTEM_ZLIB= ZLIB_CFLAGS= ZLIB_LIBS=], $ZLIB_LIBS)\n\
fi\n\
if test \"$SYSTEM_ZLIB\" = 1; then\n\
    LIBS=\"$ZLIB_LIBS $LIBS\"\n\
    AC_TRY_RUN([\n\
    #include <stdio.h>\n\
    #include <string.h>\n\
    #include <zlib.h>\n\
    int parse_version(const char *str, int *ver) {\n\
      return (sscanf(str, \"%d.%d.%d\", ver, ver+1, ver+2)==3?0:1);\n\
    }\n\
    int main() {\n\
      int sys[3], req[3];\n\
      if (parse_version(zlib_version, sys) || parse_version(\"$MOZZLIB\", req))\n\
        exit(1);\n\
      if ((sys[0] == req[0]) &&\n\
          ((sys[1] > req[1]) || ((sys[1] == req[1]) && (sys[2] >= req[2]))))\n\
        exit(0);\n\
      else\n\
        exit(1);\n\
    }\n\
    ], SYSTEM_ZLIB=1, [SYSTEM_ZLIB= ZLIB_CFLAGS= ZLIB_LIBS=], SYSTEM_ZLIB= ) \n\
    rm -f core\n\
fi\n\
CFLAGS=$_SAVE_CFLAGS\n\
LDFLAGS=$_SAVE_LDFLAGS\n\
LIBS=$_SAVE_LIBS\n\
\n\
if test \"${ZLIB_DIR}\" -a -d \"${ZLIB_DIR}\" -a \"$SYSTEM_ZLIB\" = 1; then\n\
    ZLIB_CFLAGS=\"-I${ZLIB_DIR}/include\"\n\
    ZLIB_LIBS=\"-L${ZLIB_DIR}/lib ${ZLIB_LIBS}\"\n\
fi\n\
\n\
dnl system PNG Support\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(system-png, \n\
[  --with-system-png[=PFX]\n\
                          Use system libpng [installed at prefix PFX]],\n\
    PNG_DIR=$withval)\n\
\n\
_SAVE_CFLAGS=$CFLAGS\n\
_SAVE_LDFLAGS=$LDFLAGS\n\
_SAVE_LIBS=$LIBS\n\
CFLAGS=\"$ZLIB_CFLAGS $CFLAGS\"\n\
LDFLAGS=\"$ZLIB_LIBS -lz $LDFLAGS\"\n\
if test -n \"${PNG_DIR}\" -a \"${PNG_DIR}\" != \"yes\"; then\n\
    CFLAGS=\"-I${PNG_DIR}/include $CFLAGS\"\n\
    LDFLAGS=\"-L${PNG_DIR}/lib $LDFLAGS\"\n\
fi\n\
if test -z \"$PNG_DIR\" -o \"$PNG_DIR\" = no; then\n\
    SYSTEM_PNG=\n\
else\n\
    _SAVE_PNG_LIBS=$PNG_LIBS\n\
    AC_CHECK_LIB(png, png_get_valid, [SYSTEM_PNG=1 PNG_LIBS=\"-lpng $PNG_LIBS\"],\n\
                 SYSTEM_PNG=, $PNG_LIBS)\n\
    AC_CHECK_LIB(png, png_get_acTl, ,\n\
                 [SYSTEM_PNG= PNG_LIBS=$_SAVE_PNG_LIBS], $_SAVE_PNG_LIBS)\n\
fi\n\
if test \"$SYSTEM_PNG\" = 1; then\n\
    LIBS=\"$PNG_LIBS $LIBS\"\n\
    AC_TRY_RUN(\n\
	#include <stdio.h>\n\
	#include <sys/types.h>\n\
	#include <png.h>\n\
	int main () {\n\
            #if PNG_LIBPNG_VER >= $MOZPNG\n\
                #ifdef PNG_UINT_31_MAX\n\
		    exit(0);\n\
	        #else\n\
	            exit(1);\n\
                #endif\n\
	    #else\n\
		exit(1);\n\
	    #endif\n\
	}\n\
	, SYSTEM_PNG=1, [SYSTEM_PNG= PNG_CFLAGS= PNG_LIBS=], SYSTEM_PNG= ) \n\
    rm -f core\n\
fi\n\
CFLAGS=$_SAVE_CFLAGS\n\
LDFLAGS=$_SAVE_LDFLAGS\n\
LIBS=$_SAVE_LIBS\n\
\n\
if test \"${PNG_DIR}\" -a -d \"${PNG_DIR}\" -a \"$SYSTEM_PNG\" = 1; then\n\
    PNG_CFLAGS=\"-I${PNG_DIR}/include\"\n\
    PNG_LIBS=\"-L${PNG_DIR}/lib ${PNG_LIBS}\"\n\
fi\n\
\n\
fi # SKIP_LIBRARY_CHECKS\n\
\n\
dnl check whether to enable glitz\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(glitz,\n\
[  --enable-glitz          Enable Glitz for use with Cairo],\n\
    MOZ_ENABLE_GLITZ=1,\n\
    MOZ_ENABLE_GLITZ= )\n\
if test \"$MOZ_ENABLE_GLITZ\"; then\n\
    AC_DEFINE(MOZ_ENABLE_GLITZ)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Java SDK support\n\
dnl ========================================================\n\
JAVA_INCLUDE_PATH=\n\
MOZ_ARG_WITH_STRING(java-include-path,\n\
[  --with-java-include-path=dir   Location of Java SDK headers],\n\
    JAVA_INCLUDE_PATH=$withval)\n\
\n\
JAVA_BIN_PATH=\n\
MOZ_ARG_WITH_STRING(java-bin-path,\n\
[  --with-java-bin-path=dir   Location of Java binaries (java, javac, jar)],\n\
    JAVA_BIN_PATH=$withval)\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Application\n\
dnl =\n\
dnl ========================================================\n\
\n\
MOZ_ARG_HEADER(Application)\n\
\n\
BUILD_STATIC_LIBS=\n\
ENABLE_TESTS=1\n\
MOZ_ACTIVEX_SCRIPTING_SUPPORT=\n\
MOZ_BRANDING_DIRECTORY=\n\
MOZ_CALENDAR=\n\
MOZ_DBGRINFO_MODULES=\n\
MOZ_ENABLE_CANVAS=1\n\
MOZ_EXTENSIONS_ALL=\" wallet xml-rpc help p3p venkman inspector irc typeaheadfind gnomevfs sroaming datetime finger cview layout-debug tasks sql xforms schema-validation reporter\"\n\
MOZ_FEEDS=1\n\
MOZ_IMG_DECODERS_DEFAULT=\"png gif jpeg bmp xbm icon\"\n\
MOZ_IMG_ENCODERS_DEFAULT=\"png jpeg\"\n\
MOZ_IPCD=\n\
MOZ_JAVAXPCOM=\n\
MOZ_JSDEBUGGER=1\n\
MOZ_JSLOADER=1\n\
MOZ_LDAP_XPCOM=\n\
MOZ_LIBART_CFLAGS=\n\
MOZ_LIBART_LIBS=\n\
MOZ_MAIL_NEWS=\n\
MOZ_MATHML=1\n\
MOZ_MOCHITEST=1\n\
MOZ_MORK=1\n\
MOZ_MORKREADER=\n\
MOZ_AUTH_EXTENSION=1\n\
MOZ_NO_ACTIVEX_SUPPORT=1\n\
MOZ_NO_INSPECTOR_APIS=\n\
MOZ_NO_XPCOM_OBSOLETE=\n\
MOZ_NO_FAST_LOAD=\n\
MOZ_OJI=1\n\
MOZ_PERMISSIONS=1\n\
MOZ_PLACES=\n\
MOZ_PLAINTEXT_EDITOR_ONLY=\n\
MOZ_PLUGINS=1\n\
MOZ_PREF_EXTENSIONS=1\n\
MOZ_PROFILELOCKING=1\n\
MOZ_PROFILESHARING=1\n\
MOZ_PSM=1\n\
MOZ_PYTHON_EXTENSIONS=\"xpcom dom\"\n\
MOZ_PYTHON=\n\
MOZ_PYTHON_DEBUG_SUFFIX=\n\
MOZ_PYTHON_DLL_SUFFIX=\n\
MOZ_PYTHON_INCLUDES=\n\
MOZ_PYTHON_LIBS=\n\
MOZ_PYTHON_PREFIX=\n\
MOZ_PYTHON_VER=\n\
MOZ_PYTHON_VER_DOTTED=\n\
MOZ_RDF=1\n\
MOZ_REFLOW_PERF=\n\
MOZ_SAFE_BROWSING=\n\
MOZ_SINGLE_PROFILE=\n\
MOZ_SPELLCHECK=1\n\
MOZ_STATIC_MAIL_BUILD=\n\
MOZ_STORAGE=1\n\
MOZ_SVG=1\n\
MOZ_TIMELINE=\n\
MOZ_UI_LOCALE=en-US\n\
MOZ_UNIVERSALCHARDET=1\n\
MOZ_URL_CLASSIFIER=\n\
MOZ_USE_NATIVE_UCONV=\n\
MOZ_V1_STRING_ABI=1\n\
MOZ_VIEW_SOURCE=1\n\
MOZ_WEBSERVICES=1\n\
MOZ_XMLEXTRAS=1\n\
MOZ_XPFE_COMPONENTS=1\n\
MOZ_XPINSTALL=1\n\
MOZ_XSLT_STANDALONE=\n\
MOZ_XTF=1\n\
MOZ_XUL=1\n\
NS_PRINTING=1\n\
NECKO_COOKIES=1\n\
NECKO_DISK_CACHE=1\n\
NECKO_PROTOCOLS_DEFAULT=\"about data file ftp gopher http res viewsource\"\n\
NECKO_SMALL_BUFFERS=\n\
SUNCTL=\n\
JS_STATIC_BUILD=\n\
XPC_IDISPATCH_SUPPORT=\n\
\n\
\n\
case \"$target_os\" in\n\
darwin*)\n\
    ACCESSIBILITY=\n\
    ;;\n\
*)\n\
    ACCESSIBILITY=1\n\
    ;;\n\
esac\n\
\n\
case \"$target_os\" in\n\
    msvc*|mks*|cygwin*|mingw*)\n\
        if test -z \"$GNU_CC\"; then \n\
            XPC_IDISPATCH_SUPPORT=1\n\
            MOZ_NO_ACTIVEX_SUPPORT=\n\
            MOZ_ACTIVEX_SCRIPTING_SUPPORT=1\n\
        fi\n\
        ;;\n\
esac\n\
\n\
MOZ_ARG_ENABLE_STRING(application,\n\
[  --enable-application=APP\n\
                          Options include:\n\
                            suite\n\
                            browser (Firefox)\n\
                            mail (Thunderbird)\n\
                            minimo\n\
                            composer\n\
                            calendar (Sunbird)\n\
                            xulrunner\n\
                            camino\n\
                            content/xslt (Standalone Transformiix XSLT)\n\
                            netwerk (Standalone Necko)\n\
                            tools/update-packaging (AUS-related packaging tools)\n\
                            standalone (use this for standalone\n\
                              xpcom/xpconnect or to manually drive a build)],\n\
[ MOZ_BUILD_APP=$enableval ] )\n\
\n\
if test \"$MOZ_BUILD_APP\" = \"macbrowser\"; then\n\
    MOZ_BUILD_APP=camino\n\
fi\n\
\n\
case \"$MOZ_BUILD_APP\" in\n\
minimo)\n\
  MOZ_EMBEDDING_PROFILE=basic\n\
  ;;\n\
\n\
*)\n\
  MOZ_EMBEDDING_PROFILE=default\n\
  ;;\n\
esac\n\
\n\
MOZ_ARG_WITH_STRING(embedding-profile,\n\
[  --with-embedding-profile=default|basic|minimal\n\
                       see http://wiki.mozilla.org/Gecko:Small_Device_Support],\n\
[ MOZ_EMBEDDING_PROFILE=$withval ])\n\
\n\
case \"$MOZ_EMBEDDING_PROFILE\" in\n\
default)\n\
  MOZ_EMBEDDING_LEVEL_DEFAULT=1\n\
  MOZ_EMBEDDING_LEVEL_BASIC=1\n\
  MOZ_EMBEDDING_LEVEL_MINIMAL=1\n\
  AC_DEFINE(MOZ_EMBEDDING_LEVEL_DEFAULT)\n\
  AC_DEFINE(MOZ_EMBEDDING_LEVEL_BASIC)\n\
  AC_DEFINE(MOZ_EMBEDDING_LEVEL_MINIMAL)\n\
  ;;\n\
\n\
basic)\n\
  MOZ_EMBEDDING_LEVEL_DEFAULT=\n\
  MOZ_EMBEDDING_LEVEL_BASIC=1\n\
  MOZ_EMBEDDING_LEVEL_MINIMAL=1\n\
  AC_DEFINE(MOZ_EMBEDDING_LEVEL_BASIC)\n\
  AC_DEFINE(MOZ_EMBEDDING_LEVEL_MINIMAL)\n\
  ENABLE_TESTS=\n\
  MOZ_ACTIVEX_SCRIPTING_SUPPORT=\n\
  MOZ_COMPOSER=\n\
  MOZ_ENABLE_CANVAS=\n\
  MOZ_ENABLE_POSTSCRIPT=\n\
  MOZ_EXTENSIONS_DEFAULT=\" spatialnavigation\"\n\
  MOZ_IMG_DECODERS_DEFAULT=\"png gif jpeg\"\n\
  MOZ_IMG_ENCODERS_DEFAULT=\n\
  MOZ_IMG_ENCODERS=\n\
  MOZ_INSTALLER=\n\
  MOZ_JSDEBUGGER=\n\
  MOZ_LDAP_XPCOM=\n\
  MOZ_MAIL_NEWS=\n\
  MOZ_MATHML=\n\
  MOZ_AUTH_EXTENSION=\n\
  MOZ_NO_ACTIVEX_SUPPORT=1\n\
  MOZ_NO_INSPECTOR_APIS=1\n\
  MOZ_NO_XPCOM_OBSOLETE=1\n\
  MOZ_NO_FAST_LOAD=1\n\
  MOZ_OJI=\n\
  MOZ_PLAINTEXT_EDITOR_ONLY=1\n\
#  MOZ_PLUGINS=\n\
  MOZ_PREF_EXTENSIONS=\n\
  MOZ_PROFILELOCKING=\n\
  MOZ_PROFILESHARING=\n\
  MOZ_SINGLE_PROFILE=1\n\
  MOZ_SPELLCHECK=\n\
  MOZ_SVG=\n\
  MOZ_UNIVERSALCHARDET=\n\
  MOZ_UPDATER=\n\
  MOZ_USE_NATIVE_UCONV=1\n\
  MOZ_V1_STRING_ABI=\n\
  MOZ_VIEW_SOURCE=\n\
  MOZ_XPFE_COMPONENTS=\n\
  MOZ_XPINSTALL=\n\
  MOZ_XTF=\n\
  MOZ_XUL_APP=1\n\
  NECKO_DISK_CACHE=\n\
  NECKO_PROTOCOLS_DEFAULT=\"about data http file res\"\n\
  NECKO_SMALL_BUFFERS=1\n\
  NS_DISABLE_LOGGING=1\n\
  NS_PRINTING=\n\
  JS_STATIC_BUILD=1\n\
  ;;\n\
\n\
minimal)\n\
  MOZ_EMBEDDING_LEVEL_DEFAULT=\n\
  MOZ_EMBEDDING_LEVEL_BASIC=\n\
  MOZ_EMBEDDING_LEVEL_MINIMAL=1\n\
  AC_DEFINE(MOZ_EMBEDDING_LEVEL_MINIMAL)\n\
  ENABLE_TESTS=\n\
  MOZ_ACTIVEX_SCRIPTING_SUPPORT=\n\
  MOZ_COMPOSER=\n\
  MOZ_ENABLE_CANVAS=\n\
  MOZ_ENABLE_POSTSCRIPT=\n\
  MOZ_EXTENSIONS_DEFAULT=\" spatialnavigation\"\n\
  MOZ_IMG_DECODERS_DEFAULT=\"png gif jpeg\"\n\
  MOZ_IMG_ENCODERS_DEFAULT=\n\
  MOZ_IMG_ENCODERS=\n\
  MOZ_INSTALLER=\n\
  MOZ_JSDEBUGGER=\n\
  MOZ_LDAP_XPCOM=\n\
  MOZ_MAIL_NEWS=\n\
  MOZ_MATHML=\n\
  MOZ_AUTH_EXTENSION=\n\
  MOZ_NO_ACTIVEX_SUPPORT=1\n\
  MOZ_NO_INSPECTOR_APIS=1\n\
  MOZ_NO_XPCOM_OBSOLETE=1\n\
  MOZ_NO_FAST_LOAD=1\n\
  MOZ_OJI=\n\
  MOZ_PLAINTEXT_EDITOR_ONLY=1\n\
  MOZ_PLUGINS=\n\
  MOZ_PREF_EXTENSIONS=\n\
  MOZ_PROFILELOCKING=\n\
  MOZ_PROFILESHARING=\n\
  MOZ_SINGLE_PROFILE=1\n\
  MOZ_SPELLCHECK=\n\
  MOZ_STORAGE=\n\
  MOZ_SVG=\n\
  MOZ_UNIVERSALCHARDET=\n\
  MOZ_UPDATER=\n\
  MOZ_USE_NATIVE_UCONV=1\n\
  MOZ_V1_STRING_ABI=\n\
  MOZ_VIEW_SOURCE=\n\
  MOZ_XPFE_COMPONENTS=\n\
  MOZ_XPINSTALL=\n\
  MOZ_XTF=\n\
  MOZ_XUL=\n\
  MOZ_RDF=\n\
  MOZ_XUL_APP=1\n\
  NECKO_DISK_CACHE=\n\
  NECKO_PROTOCOLS_DEFAULT=\"about data http file res\"\n\
  NECKO_SMALL_BUFFERS=1\n\
  NS_DISABLE_LOGGING=1\n\
  NS_PRINTING=\n\
  JS_STATIC_BUILD=1\n\
  ;;\n\
\n\
*)\n\
  AC_MSG_ERROR([Unrecognized value: --with-embedding-profile=$MOZ_EMBEDDING_PROFILE])\n\
  ;;\n\
esac\n\
\n\
AC_SUBST(MOZ_EMBEDDING_LEVEL_DEFAULT)\n\
AC_SUBST(MOZ_EMBEDDING_LEVEL_BASIC)\n\
AC_SUBST(MOZ_EMBEDDING_LEVEL_MINIMAL)\n\
\n\
case \"$MOZ_BUILD_APP\" in\n\
suite)\n\
  MOZ_APP_NAME=seamonkey\n\
  MOZ_APP_DISPLAYNAME=SeaMonkey\n\
  MOZ_MAIL_NEWS=1\n\
  MOZ_LDAP_XPCOM=1\n\
  MOZ_COMPOSER=1\n\
  MOZ_SUITE=1\n\
  MOZ_PROFILESHARING=\n\
  MOZ_APP_VERSION=$SEAMONKEY_VERSION\n\
  if test \"$MOZ_XUL_APP\"; then\n\
    MOZ_EXTENSIONS_DEFAULT=\" wallet xml-rpc p3p venkman inspector irc typeaheadfind gnomevfs sroaming reporter\"\n\
  else\n\
    MOZ_EXTENSIONS_DEFAULT=\" wallet xml-rpc help p3p venkman inspector irc typeaheadfind gnomevfs sroaming reporter\"\n\
  fi\n\
  AC_DEFINE(MOZ_SUITE)\n\
  ;;\n\
\n\
browser)\n\
  MOZ_APP_NAME=firefox\n\
  MOZ_XUL_APP=1\n\
  MOZ_UPDATER=1\n\
  MOZ_PHOENIX=1\n\
  MOZ_PLACES=1\n\
  MOZ_PLACES_BOOKMARKS=\n\
  # always enabled for form history\n\
  MOZ_MORKREADER=1\n\
  MOZ_SAFE_BROWSING=1\n\
  MOZ_APP_VERSION=$FIREFOX_VERSION\n\
  MOZ_NO_XPCOM_OBSOLETE=1\n\
  MOZ_EXTENSIONS_DEFAULT=\" xml-rpc inspector gnomevfs reporter\"\n\
  AC_DEFINE(MOZ_PHOENIX)\n\
  # MOZ_APP_DISPLAYNAME will be set by branding/configure.sh\n\
  ;;\n\
\n\
minimo)\n\
  MOZ_APP_NAME=minimo\n\
  MOZ_APP_DISPLAYNAME=minimo\n\
  AC_DEFINE(MINIMO)\n\
  MINIMO=1\n\
  MOZ_APP_VERSION=`cat $topsrcdir/minimo/config/version.txt`\n\
  ;;\n\
\n\
mail)\n\
  MOZ_APP_NAME=thunderbird\n\
  MOZ_APP_DISPLAYNAME=Thunderbird\n\
  MOZ_XUL_APP=1\n\
  MOZ_UPDATER=1\n\
  MOZ_THUNDERBIRD=1\n\
  MOZ_MATHML=\n\
  MOZ_NO_ACTIVEX_SUPPORT=1\n\
  MOZ_ACTIVEX_SCRIPTING_SUPPORT=\n\
  ENABLE_TESTS=\n\
  MOZ_OJI=\n\
  NECKO_DISK_CACHE=\n\
  NECKO_PROTOCOLS_DEFAULT=\"http file viewsource res data\"\n\
  MOZ_ENABLE_CANVAS=\n\
  MOZ_IMG_DECODERS_DEFAULT=`echo \"$MOZ_IMG_DECODERS_DEFAULT\" | sed \"s/ xbm//\"`\n\
  MOZ_MAIL_NEWS=1\n\
  MOZ_LDAP_XPCOM=1\n\
  MOZ_STATIC_MAIL_BUILD=1\n\
  MOZ_COMPOSER=1\n\
  MOZ_SAFE_BROWSING=1\n\
  MOZ_SVG=\n\
  MOZ_APP_VERSION=$THUNDERBIRD_VERSION\n\
  MOZ_EXTENSIONS_DEFAULT=\" wallet\"\n\
  AC_DEFINE(MOZ_THUNDERBIRD)\n\
  ;;\n\
\n\
composer)\n\
  MOZ_APP_NAME=nvu\n\
  MOZ_APP_DISPLAYNAME=NVU\n\
  MOZ_XUL_APP=1\n\
  MOZ_UPDATER=1\n\
  MOZ_STANDALONE_COMPOSER=1\n\
  MOZ_COMPOSER=1\n\
  MOZ_APP_VERSION=0.17+\n\
  AC_DEFINE(MOZ_STANDALONE_COMPOSER)\n\
  ;;\n\
\n\
calendar)\n\
  MOZ_APP_NAME=sunbird\n\
  MOZ_APP_DISPLAYNAME=Calendar\n\
  MOZ_XUL_APP=1\n\
  MOZ_UPDATER=1\n\
  MOZ_SUNBIRD=1\n\
  MOZ_CALENDAR=1\n\
  MOZ_APP_VERSION=$SUNBIRD_VERSION\n\
  MOZ_PLAINTEXT_EDITOR_ONLY=1\n\
  NECKO_PROTOCOLS_DEFAULT=\"about http ftp file res viewsource\"\n\
  MOZ_NO_ACTIVEX_SUPPORT=1\n\
  MOZ_ACTIVEX_SCRIPTING_SUPPORT=\n\
  MOZ_INSTALLER=\n\
  MOZ_MATHML=\n\
  NECKO_DISK_CACHE=\n\
  MOZ_OJI=\n\
  MOZ_PLUGINS=\n\
  NECKO_COOKIES=\n\
  MOZ_NO_XPCOM_OBSOLETE=1\n\
  MOZ_EXTENSIONS_DEFAULT=\n\
  MOZ_UNIVERSALCHARDET=\n\
  AC_DEFINE(MOZ_SUNBIRD)\n\
  ;;\n\
\n\
xulrunner)\n\
  MOZ_APP_NAME=xulrunner\n\
  MOZ_APP_DISPLAYNAME=XULRunner\n\
  MOZ_XUL_APP=1\n\
  MOZ_UPDATER=1\n\
  MOZ_XULRUNNER=1\n\
  MOZ_ENABLE_LIBXUL=1\n\
  MOZ_APP_VERSION=$MOZILLA_VERSION\n\
  MOZ_JAVAXPCOM=1\n\
  if test \"$MOZ_STORAGE\"; then\n\
    MOZ_PLACES=1\n\
  fi\n\
  MOZ_EXTENSIONS_DEFAULT=\" xml-rpc gnomevfs\"\n\
  MOZ_NO_XPCOM_OBSOLETE=1\n\
  AC_DEFINE(MOZ_XULRUNNER)\n\
\n\
  if test \"$LIBXUL_SDK\"; then\n\
    AC_MSG_ERROR([Building XULRunner --with-libxul-sdk doesn\'t make sense; XULRunner provides the libxul SDK.])\n\
  fi\n\
  ;;\n\
\n\
camino) \n\
  MOZ_APP_NAME=mozilla\n\
  ENABLE_TESTS=\n\
  ACCESSIBILITY=\n\
  MOZ_JSDEBUGGER=\n\
  MOZ_SINGLE_PROFILE=1\n\
  MOZ_PROFILESHARING=\n\
  MOZ_APP_DISPLAYNAME=Mozilla\n\
  MOZ_APP_VERSION=$MOZILLA_VERSION\n\
  MOZ_EXTENSIONS_DEFAULT=\" typeaheadfind\"\n\
# MOZ_XUL_APP=1\n\
  MOZ_PREF_EXTENSIONS=\n\
  MOZ_WEBSERVICES=\n\
  AC_DEFINE(MOZ_MACBROWSER)\n\
  ;;\n\
\n\
content/xslt)\n\
  MOZ_APP_NAME=mozilla\n\
  MOZ_EXTENSIONS_DEFAULT=\"\"\n\
  AC_DEFINE(TX_EXE)\n\
  MOZ_XSLT_STANDALONE=1\n\
  MOZ_APP_VERSION=$MOZILLA_VERSION\n\
  ;;\n\
\n\
netwerk)\n\
  MOZ_APP_NAME=mozilla\n\
  MOZ_EXTENSIONS_DEFAULT=\"\"\n\
  MOZ_APP_VERSION=$MOZILLA_VERSION\n\
  MOZ_NO_XPCOM_OBSOLETE=1\n\
  MOZ_SINGLE_PROFILE=1\n\
  MOZ_PROFILESHARING=\n\
  MOZ_XPINSTALL=\n\
  ;;\n\
\n\
standalone) \n\
  MOZ_APP_NAME=mozilla\n\
  MOZ_APP_DISPLAYNAME=Mozilla\n\
  MOZ_APP_VERSION=$MOZILLA_VERSION\n\
  ;;\n\
\n\
tools/update-packaging)\n\
  MOZ_APP_NAME=mozilla\n\
  MOZ_APP_DISPLAYNAME=Mozilla\n\
  MOZ_APP_VERSION=$MOZILLA_VERSION\n\
  MOZ_UPDATE_PACKAGING=1\n\
  MOZ_UPDATER=1\n\
  ;;\n\
\n\
*)\n\
  if test -z \"$MOZ_BUILD_APP\"; then\n\
    AC_MSG_ERROR([--enable-application=APP is required.])\n\
  else\n\
    AC_MSG_ERROR([--enable-application value not recognized.])\n\
  fi\n\
  ;;\n\
\n\
esac\n\
\n\
AC_SUBST(MOZ_BUILD_APP)\n\
AC_SUBST(MOZ_XUL_APP)\n\
AC_SUBST(MOZ_SUITE)\n\
AC_SUBST(MOZ_PHOENIX)\n\
AC_SUBST(MOZ_THUNDERBIRD)\n\
AC_SUBST(MOZ_STANDALONE_COMPOSER)\n\
AC_SUBST(MOZ_SUNBIRD)\n\
AC_SUBST(MOZ_XULRUNNER)\n\
\n\
AC_DEFINE_UNQUOTED(MOZ_BUILD_APP,$MOZ_BUILD_APP)\n\
\n\
if test \"$MOZ_XUL_APP\"; then\n\
  MOZ_SINGLE_PROFILE=1\n\
  MOZ_PROFILESHARING=\n\
  AC_DEFINE(MOZ_XUL_APP)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = \n\
dnl = Toolkit Options\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Toolkit Options)\n\
\n\
    dnl ========================================================\n\
    dnl = Select the default toolkit\n\
    dnl ========================================================\n\
	MOZ_ARG_ENABLE_STRING(default-toolkit,\n\
	[  --enable-default-toolkit=TK\n\
                          Select default toolkit\n\
                          Platform specific defaults:\n\
                            BeOS - beos\n\
                            Mac OS X - mac (carbon)\n\
                            Neutrino/QNX - photon\n\
                            OS/2 - os2\n\
                            Win32 - windows\n\
                            * - gtk],\n\
    [ _DEFAULT_TOOLKIT=$enableval ],\n\
    [ _DEFAULT_TOOLKIT=$_PLATFORM_DEFAULT_TOOLKIT])\n\
\n\
    if test \"$_DEFAULT_TOOLKIT\" = \"gtk\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"qt\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"gtk2\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"xlib\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"os2\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"beos\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"photon\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"mac\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"windows\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cocoa\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-windows\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-gtk2\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-beos\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-os2\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-xlib\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-mac\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-cocoa\"\n\
    then\n\
        dnl nglayout only supports building with one toolkit,\n\
        dnl so ignore everything after the first comma (\",\").\n\
        MOZ_WIDGET_TOOLKIT=`echo \"$_DEFAULT_TOOLKIT\" | sed -e \"s/,.*$//\"`\n\
    else\n\
        if test \"$no_x\" != \"yes\"; then\n\
            AC_MSG_ERROR([Toolkit must be xlib, gtk, gtk2 or qt.])\n\
        else\n\
            AC_MSG_ERROR([Toolkit must be $_PLATFORM_DEFAULT_TOOLKIT or cairo-$_PLATFORM_DEFAULT_TOOLKIT (if supported).])\n\
        fi\n\
    fi\n\
\n\
AC_DEFINE_UNQUOTED(MOZ_DEFAULT_TOOLKIT,\"$MOZ_WIDGET_TOOLKIT\")\n\
\n\
dnl ========================================================\n\
dnl = Enable the toolkit as needed                         =\n\
dnl ========================================================\n\
\n\
case \"$MOZ_WIDGET_TOOLKIT\" in\n\
gtk)\n\
	MOZ_ENABLE_GTK=1\n\
    MOZ_ENABLE_XREMOTE=1\n\
    if test \"$_HAVE_FREETYPE2\"; then\n\
        MOZ_ENABLE_FREETYPE2=1\n\
    fi\n\
    MOZ_ENABLE_XPRINT=1\n\
    TK_CFLAGS=\'$(MOZ_GTK_CFLAGS)\'\n\
    TK_LIBS=\'$(MOZ_GTK_LDFLAGS)\'\n\
	AC_DEFINE(MOZ_WIDGET_GTK)\n\
    ;;\n\
\n\
gtk2)\n\
    MOZ_ENABLE_GTK2=1\n\
    MOZ_ENABLE_XREMOTE=1\n\
    MOZ_ENABLE_COREXFONTS=${MOZ_ENABLE_COREXFONTS-}\n\
    TK_CFLAGS=\'$(MOZ_GTK2_CFLAGS)\'\n\
    TK_LIBS=\'$(MOZ_GTK2_LIBS)\'\n\
    AC_DEFINE(MOZ_WIDGET_GTK2)\n\
    ;;\n\
\n\
xlib)\n\
	MOZ_ENABLE_XLIB=1\n\
    if test \"$_HAVE_FREETYPE2\"; then\n\
        MOZ_ENABLE_FREETYPE2=1\n\
    fi\n\
    MOZ_ENABLE_XPRINT=1\n\
	TK_CFLAGS=\'$(MOZ_XLIB_CFLAGS)\'\n\
	TK_LIBS=\'$(MOZ_XLIB_LDFLAGS)\'\n\
	AC_DEFINE(MOZ_WIDGET_XLIB)\n\
    ;;\n\
\n\
qt)\n\
    MOZ_ENABLE_QT=1\n\
    if test \"$_HAVE_FREETYPE2\"; then\n\
        MOZ_ENABLE_FREETYPE2=1\n\
    fi\n\
    MOZ_ENABLE_XPRINT=1\n\
    TK_CFLAGS=\'$(MOZ_QT_CFLAGS)\'\n\
    TK_LIBS=\'$(MOZ_QT_LDFLAGS)\'\n\
    AC_DEFINE(MOZ_WIDGET_QT)\n\
    ;;\n\
\n\
photon)\n\
	MOZ_ENABLE_PHOTON=1\n\
	AC_DEFINE(MOZ_WIDGET_PHOTON)\n\
    ;;\n\
mac|cocoa)\n\
    TK_LIBS=\'-framework Carbon\'\n\
    TK_CFLAGS=\"-I${MACOS_SDK_DIR}/Developer/Headers/FlatCarbon\"\n\
    CFLAGS=\"$CFLAGS $TK_CFLAGS\"\n\
    CXXFLAGS=\"$CXXFLAGS $TK_CFLAGS\"\n\
    MOZ_USER_DIR=\"Mozilla\"\n\
    AC_DEFINE(XP_MACOSX)\n\
    AC_DEFINE(TARGET_CARBON)\n\
    AC_DEFINE(TARGET_API_MAC_CARBON)\n\
    if test \"$MOZ_WIDGET_TOOLKIT\" = \"cocoa\"; then\n\
        MOZ_ENABLE_COCOA=1\n\
        AC_DEFINE(MOZ_WIDGET_COCOA)\n\
    fi\n\
    ;;\n\
\n\
cairo-windows)\n\
    MOZ_WIDGET_TOOLKIT=windows\n\
    MOZ_GFX_TOOLKIT=cairo\n\
    MOZ_ENABLE_CAIRO_GFX=1\n\
    ;;\n\
\n\
cairo-gtk2)\n\
    MOZ_WIDGET_TOOLKIT=gtk2\n\
    MOZ_GFX_TOOLKIT=cairo\n\
    MOZ_ENABLE_CAIRO_GFX=1\n\
    MOZ_ENABLE_GTK2=1\n\
    MOZ_ENABLE_XREMOTE=1\n\
    TK_CFLAGS=\'$(MOZ_GTK2_CFLAGS) $(MOZ_CAIRO_CFLAGS)\'\n\
    TK_LIBS=\'$(MOZ_GTK2_LIBS) $(MOZ_CAIRO_LIBS)\'\n\
    AC_DEFINE(MOZ_WIDGET_GTK2)\n\
    ;;\n\
cairo-beos)\n\
    MOZ_WIDGET_TOOLKIT=beos\n\
    MOZ_GFX_TOOLKIT=cairo\n\
    MOZ_ENABLE_CAIRO_GFX=1\n\
    TK_CFLAGS=\'$(MOZ_CAIRO_CFLAGS)\'\n\
    TK_LIBS=\'$(MOZ_CAIRO_LIBS)\'\n\
    ;;\n\
\n\
cairo-os2)\n\
    MOZ_WIDGET_TOOLKIT=os2\n\
    MOZ_GFX_TOOLKIT=cairo\n\
    MOZ_ENABLE_CAIRO_GFX=1\n\
    TK_CFLAGS=\'$(MOZ_CAIRO_CFLAGS)\'\n\
    TK_LIBS=\'$(MOZ_CAIRO_LIBS)\'\n\
    ;;\n\
\n\
cairo-xlib)\n\
    MOZ_WIDGET_TOOLKIT=xlib\n\
    MOZ_GFX_TOOLKIT=cairo\n\
    MOZ_ENABLE_CAIRO_GFX=1\n\
	MOZ_ENABLE_XLIB=1\n\
	TK_CFLAGS=\'$(MOZ_XLIB_CFLAGS) $(MOZ_CAIRO_FLAGS)\'\n\
	TK_LIBS=\'$(MOZ_XLIB_LDFLAGS) $(MOZ_CAIRO_LIBS)\'\n\
	AC_DEFINE(MOZ_WIDGET_XLIB)\n\
    ;;\n\
\n\
cairo-mac|cairo-cocoa)\n\
    if test \"$MOZ_WIDGET_TOOLKIT\" = \"cairo-cocoa\"; then\n\
        MOZ_WIDGET_TOOLKIT=cocoa\n\
        AC_DEFINE(MOZ_WIDGET_COCOA)\n\
        MOZ_ENABLE_COCOA=1\n\
    else\n\
        MOZ_WIDGET_TOOLKIT=mac\n\
    fi\n\
    MOZ_ENABLE_CAIRO_GFX=1\n\
    MOZ_GFX_TOOLKIT=cairo\n\
    MOZ_USER_DIR=\"Mozilla\"\n\
    AC_DEFINE(XP_MACOSX)\n\
    AC_DEFINE(TARGET_CARBON)\n\
    AC_DEFINE(TARGET_API_MAC_CARBON)\n\
    TK_LIBS=\'-framework Carbon\'\n\
    TK_CFLAGS=\"-I${MACOS_SDK_DIR}/Developer/Headers/FlatCarbon\"\n\
    CFLAGS=\"$CFLAGS $TK_CFLAGS\"\n\
    CXXFLAGS=\"$CXXFLAGS $TK_CFLAGS\"\n\
    ;;\n\
esac\n\
\n\
if test \"$MOZ_ENABLE_XREMOTE\"; then\n\
    AC_DEFINE(MOZ_ENABLE_XREMOTE)\n\
fi\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test \"$MOZ_ENABLE_GTK\"\n\
then\n\
    AM_PATH_GTK($GTK_VERSION,,\n\
    AC_MSG_ERROR(Test for GTK failed.))\n\
\n\
    MOZ_GTK_LDFLAGS=$GTK_LIBS\n\
    MOZ_GTK_CFLAGS=$GTK_CFLAGS\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_GTK2\"\n\
then\n\
    PKG_CHECK_MODULES(MOZ_GTK2, gtk+-2.0 >= 1.3.7 gdk-x11-2.0 glib-2.0 gobject-2.0)\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_XLIB\"\n\
then\n\
    MOZ_XLIB_CFLAGS=\"$X_CFLAGS\"\n\
    MOZ_XLIB_LDFLAGS=\"$XLDFLAGS\"\n\
    MOZ_XLIB_LDFLAGS=\"$MOZ_XLIB_LDFLAGS $XEXT_LIBS $X11_LIBS\"\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_QT\"\n\
then\n\
    MOZ_ARG_WITH_STRING(qtdir,\n\
    [  --with-qtdir=\\$dir       Specify Qt directory ],\n\
    [ QTDIR=$withval])\n\
\n\
    if test -z \"$QTDIR\"; then\n\
      QTDIR=\"/usr\"\n\
    fi\n\
    QTINCDIR=\"/include/qt\"\n\
    if test ! -d \"$QTDIR$QTINCDIR\"; then\n\
       QTINCDIR=\"/include/X11/qt\"\n\
    fi\n\
    if test ! -d \"$QTDIR$QTINCDIR\"; then\n\
       QTINCDIR=\"/include\"\n\
    fi\n\
\n\
    if test -x \"$QTDIR/bin/moc\"; then\n\
      HOST_MOC=\"$QTDIR/bin/moc\"\n\
    else\n\
      AC_CHECK_PROGS(HOST_MOC, moc, \"\")\n\
    fi\n\
    if test -z \"$HOST_MOC\"; then\n\
      AC_MSG_ERROR([no acceptable moc preprocessor found])\n\
    fi\n\
    MOC=$HOST_MOC\n\
\n\
    QT_CFLAGS=\"-I${QTDIR}${QTINCDIR} -DQT_GENUINE_STR -DQT_NO_STL\"\n\
    if test -z \"$MOZ_DEBUG\"; then\n\
      QT_CFLAGS=\"$QT_CFLAGS -DQT_NO_DEBUG -DNO_DEBUG\"\n\
    fi\n\
    _SAVE_LDFLAGS=$LDFLAGS\n\
    QT_LDFLAGS=-L${QTDIR}/lib\n\
    LDFLAGS=\"$LDFLAGS $QT_LDFLAGS\"\n\
    AC_LANG_SAVE\n\
    AC_LANG_CPLUSPLUS\n\
    AC_CHECK_LIB(qt, main, QT_LIB=-lqt,\n\
        AC_CHECK_LIB(qt-mt, main, QT_LIB=-lqt-mt,\n\
            AC_MSG_ERROR([Cannot find QT libraries.])))\n\
    LDFLAGS=$_SAVE_LDFLAGS\n\
    QT_LIBS=\"-L/usr/X11R6/lib $QT_LDFLAGS $QT_LIB -lXext -lX11\"\n\
\n\
    MOZ_QT_LDFLAGS=$QT_LIBS\n\
    MOZ_QT_CFLAGS=$QT_CFLAGS\n\
\n\
    _SAVE_CXXFLAGS=$CXXFLAGS\n\
    _SAVE_LIBS=$LIBS\n\
\n\
    CXXFLAGS=\"$CXXFLAGS $QT_CFLAGS\"\n\
    LIBS=\"$LIBS $QT_LIBS\"\n\
    \n\
    AC_MSG_CHECKING(Qt - version >= $QT_VERSION)\n\
    AC_TRY_COMPILE([#include <qglobal.h>],\n\
    [\n\
        #if (QT_VERSION < $QT_VERSION_NUM)\n\
            #error  \"QT_VERSION too old\"\n\
        #endif\n\
    ],result=\"yes\",result=\"no\")\n\
\n\
    AC_MSG_RESULT(\"$result\")\n\
    if test \"$result\" = \"no\"; then\n\
        AC_MSG_ERROR([Qt Mozilla requires at least version $QT_VERSION of Qt])\n\
    fi\n\
    CXXFLAGS=$_SAVE_CXXFLAGS\n\
    LIBS=$_SAVE_LIBS\n\
\n\
    AC_LANG_RESTORE\n\
fi\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
AC_SUBST(MOZ_DEFAULT_TOOLKIT)\n\
\n\
dnl ========================================================\n\
dnl = startup-notification support module\n\
dnl ========================================================\n\
\n\
if test \"$MOZ_ENABLE_GTK2\"\n\
then\n\
    MOZ_ENABLE_STARTUP_NOTIFICATION=\n\
\n\
    MOZ_ARG_ENABLE_BOOL(startup-notification,\n\
    [  --enable-startup-notification       Enable startup-notification support (default: disabled) ],\n\
        MOZ_ENABLE_STARTUP_NOTIFICATION=force,\n\
        MOZ_ENABLE_STARTUP_NOTIFICATION=)\n\
    if test \"$MOZ_ENABLE_STARTUP_NOTIFICATION\"\n\
    then\n\
        PKG_CHECK_MODULES(MOZ_STARTUP_NOTIFICATION,\n\
                          libstartup-notification-1.0 >= $STARTUP_NOTIFICATION_VERSION,\n\
        [MOZ_ENABLE_STARTUP_NOTIFICATION=1], [\n\
            if test \"$MOZ_ENABLE_STARTUP_NOTIFICATION\" = \"force\"\n\
            then\n\
                AC_MSG_ERROR([* * * Could not find startup-notification >= $STARTUP_NOTIFICATION_VERSION])\n\
            fi\n\
            MOZ_ENABLE_STARTUP_NOTIFICATION=\n\
        ])\n\
    fi\n\
\n\
    if test \"$MOZ_ENABLE_STARTUP_NOTIFICATION\"; then\n\
        AC_DEFINE(MOZ_ENABLE_STARTUP_NOTIFICATION)\n\
    fi\n\
\n\
    TK_LIBS=\"$TK_LIBS $MOZ_STARTUP_NOTIFICATION_LIBS\"\n\
fi\n\
AC_SUBST(MOZ_ENABLE_STARTUP_NOTIFICATION)\n\
AC_SUBST(MOZ_STARTUP_NOTIFICATION_CFLAGS)\n\
AC_SUBST(MOZ_STARTUP_NOTIFICATION_LIBS)\n\
\n\
AC_SUBST(GTK_CONFIG)\n\
AC_SUBST(TK_CFLAGS)\n\
AC_SUBST(TK_LIBS)\n\
\n\
AC_SUBST(MOZ_ENABLE_GTK)\n\
AC_SUBST(MOZ_ENABLE_XLIB)\n\
AC_SUBST(MOZ_ENABLE_GTK2)\n\
AC_SUBST(MOZ_ENABLE_QT)\n\
AC_SUBST(MOZ_ENABLE_PHOTON)\n\
AC_SUBST(MOZ_ENABLE_COCOA)\n\
AC_SUBST(MOZ_ENABLE_CAIRO_GFX)\n\
AC_SUBST(MOZ_ENABLE_GLITZ)\n\
AC_SUBST(MOZ_ENABLE_XREMOTE)\n\
AC_SUBST(MOZ_GTK_CFLAGS)\n\
AC_SUBST(MOZ_GTK_LDFLAGS)\n\
AC_SUBST(MOZ_GTK2_CFLAGS)\n\
AC_SUBST(MOZ_GTK2_LIBS)\n\
AC_SUBST(MOZ_XLIB_CFLAGS)\n\
AC_SUBST(MOZ_XLIB_LDFLAGS)\n\
AC_SUBST(MOZ_QT_CFLAGS)\n\
AC_SUBST(MOZ_QT_LDFLAGS)\n\
\n\
AC_SUBST(MOC)\n\
\n\
if test \"$MOZ_ENABLE_CAIRO_GFX\"\n\
then\n\
    AC_DEFINE(MOZ_THEBES)\n\
    AC_DEFINE(MOZ_CAIRO_GFX)\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_GTK\" \\\n\
|| test \"$MOZ_ENABLE_QT\" \\\n\
|| test \"$MOZ_ENABLE_XLIB\" \\\n\
|| test \"$MOZ_ENABLE_GTK2\"\n\
then\n\
    AC_DEFINE(MOZ_X11)\n\
    MOZ_X11=1\n\
fi\n\
AC_SUBST(MOZ_X11)\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Components & Features\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Components and Features)\n\
\n\
dnl ========================================================\n\
dnl = Localization\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_STRING(ui-locale,\n\
[  --enable-ui-locale=ab-CD\n\
                          Select the user interface locale (default: en-US)],\n\
    MOZ_UI_LOCALE=$enableval )\n\
AC_SUBST(MOZ_UI_LOCALE)\n\
\n\
dnl =========================================================\n\
dnl = Calendar client\n\
dnl =========================================================\n\
MOZ_ARG_ENABLE_BOOL(calendar,,\n\
    MOZ_OLD_CALENDAR=1,\n\
    MOZ_OLD_CALENDAR= )\n\
\n\
if test \"$MOZ_OLD_CALENDAR\"; then\n\
    AC_MSG_WARN([Building with the calendar extension is no longer supported.])\n\
    if test \"$MOZ_THUNDERBIRD\"; then\n\
        AC_MSG_WARN([Since you\'re trying to build mail, you could try adding])\n\
        AC_MSG_WARN([\'--enable-extensions=default,lightning\' to your mozconfig])\n\
        AC_MSG_WARN([and building WITH A FRESH TREE.])\n\
    fi\n\
    AC_MSG_WARN([For more information, please visit:])\n\
    AC_MSG_ERROR([http://www.mozilla.org/projects/calendar/])\n\
fi\n\
\n\
AC_SUBST(MOZ_CALENDAR)\n\
\n\
dnl =========================================================\n\
dnl = Mail & News\n\
dnl =========================================================\n\
MOZ_ARG_DISABLE_BOOL(mailnews,\n\
[  --disable-mailnews      Disable building of mail & news components],\n\
    MOZ_MAIL_NEWS=,\n\
    MOZ_MAIL_NEWS=1 )\n\
AC_SUBST(MOZ_MAIL_NEWS)\n\
\n\
dnl ========================================================\n\
dnl static mail build off by default \n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(static-mail,\n\
[  --enable-static-mail Enable static mail build support],\n\
    MOZ_STATIC_MAIL_BUILD=1,\n\
    MOZ_STATIC_MAIL_BUILD= )\n\
\n\
if test \"$MOZ_STATIC_MAIL_BUILD\"; then\n\
    AC_DEFINE(MOZ_STATIC_MAIL_BUILD)\n\
fi\n\
\n\
AC_SUBST(MOZ_STATIC_MAIL_BUILD)\n\
\n\
dnl =========================================================\n\
dnl = LDAP\n\
dnl =========================================================\n\
MOZ_ARG_DISABLE_BOOL(ldap,\n\
[  --disable-ldap          Disable LDAP support],\n\
    MOZ_LDAP_XPCOM=,\n\
    MOZ_LDAP_XPCOM=1)\n\
\n\
dnl ========================================================\n\
dnl = Trademarked Branding \n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(official-branding,\n\
[  --enable-official-branding Enable Official mozilla.org Branding\n\
                          Do not distribute builds with\n\
                          --enable-official-branding unless you have\n\
                          permission to use trademarks per\n\
                          http://www.mozilla.org/foundation/trademarks/ .],\n\
[case \"$MOZ_BUILD_APP\" in\n\
browser)\n\
    MOZ_BRANDING_DIRECTORY=other-licenses/branding/firefox\n\
    MOZ_APP_DISPLAYNAME=Firefox\n\
    ;;\n\
\n\
calendar)\n\
    MOZ_BRANDING_DIRECTORY=other-licenses/branding/sunbird\n\
    MOZ_APP_DISPLAYNAME=Sunbird\n\
    ;;\n\
\n\
mail)\n\
    MOZ_BRANDING_DIRECTORY=other-licenses/branding/thunderbird\n\
    ;;\n\
\n\
*)]\n\
    AC_MSG_ERROR([Official branding is only available for Firefox Sunbird and Thunderbird.])\n\
esac\n\
)\n\
\n\
MOZ_ARG_WITH_STRING(branding,\n\
[  --with-branding=dir    Use branding from the specified directory.],\n\
    MOZ_BRANDING_DIRECTORY=$withval)\n\
\n\
REAL_BRANDING_DIRECTORY=\"${MOZ_BRANDING_DIRECTORY}\"\n\
if test -z \"$REAL_BRANDING_DIRECTORY\"; then\n\
  REAL_BRANDING_DIRECTORY=${MOZ_BUILD_APP}/branding/nightly\n\
fi\n\
\n\
if test -f \"$topsrcdir/$REAL_BRANDING_DIRECTORY/configure.sh\"; then\n\
  . \"$topsrcdir/$REAL_BRANDING_DIRECTORY/configure.sh\"\n\
fi\n\
\n\
AC_SUBST(MOZ_BRANDING_DIRECTORY)\n\
\n\
dnl ========================================================\n\
dnl = Distribution ID\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(distribution-id,\n\
[  --with-distribution-id=ID  Set distribution-specific id (default=org.mozilla)],\n\
[ val=`echo $withval`\n\
    MOZ_DISTRIBUTION_ID=\"$val\"])\n\
\n\
if test -z \"$MOZ_DISTRIBUTION_ID\"; then\n\
   MOZ_DISTRIBUTION_ID=\"org.mozilla\"\n\
fi\n\
\n\
AC_DEFINE_UNQUOTED(MOZ_DISTRIBUTION_ID,\"$MOZ_DISTRIBUTION_ID\")\n\
AC_SUBST(MOZ_DISTRIBUTION_ID)\n\
\n\
dnl ========================================================\n\
dnl = FreeType2\n\
dnl = Enable freetype by default if building against X11 \n\
dnl = and freetype is available\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(freetype2,\n\
[  --disable-freetype2     Disable FreeType2 support ],\n\
    MOZ_ENABLE_FREETYPE2=,\n\
    MOZ_ENABLE_FREETYPE2=1)\n\
\n\
if test \"$MOZ_ENABLE_FREETYPE2\" && test -z \"$MOZ_X11\" -o -z \"$_HAVE_FREETYPE2\"; then\n\
    AC_MSG_ERROR([Cannot enable FreeType2 support for non-X11 toolkits or if FreeType2 is not detected.])\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_FREETYPE2\"; then\n\
    AC_DEFINE(MOZ_ENABLE_FREETYPE2)\n\
    _NON_GLOBAL_ACDEFINES=\"$_NON_GLOBAL_ACDEFINES MOZ_ENABLE_FREETYPE2\"\n\
fi\n\
AC_SUBST(MOZ_ENABLE_FREETYPE2)\n\
\n\
dnl ========================================================\n\
dnl = Xft\n\
dnl ========================================================\n\
if test \"$MOZ_ENABLE_GTK2\"; then\n\
    MOZ_ENABLE_XFT=1\n\
fi\n\
\n\
MOZ_ARG_DISABLE_BOOL(xft,\n\
[  --disable-xft           Disable Xft support ],\n\
    MOZ_ENABLE_XFT=,\n\
    MOZ_ENABLE_XFT=1)\n\
\n\
if test \"$MOZ_ENABLE_XFT\" && test -z \"$MOZ_ENABLE_GTK2\"; then\n\
    AC_MSG_ERROR([Cannot enable XFT support for non-GTK2 toolkits.])\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_XFT\" && test \"$MOZ_ENABLE_FREETYPE2\"; then\n\
    AC_MSG_ERROR([Cannot enable XFT and FREETYPE2 at the same time.])\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_XFT\"\n\
then\n\
    AC_DEFINE(MOZ_ENABLE_XFT)\n\
    PKG_CHECK_MODULES(MOZ_XFT, xft)\n\
    PKG_CHECK_MODULES(_PANGOCHK, pango >= 1.1.0)\n\
fi\n\
\n\
AC_SUBST(MOZ_ENABLE_XFT)\n\
AC_SUBST(MOZ_XFT_CFLAGS)\n\
AC_SUBST(MOZ_XFT_LIBS)\n\
\n\
dnl ========================================================\n\
dnl = pango font rendering\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(pango,\n\
[  --enable-pango          Enable Pango font rendering support],\n\
    MOZ_ENABLE_PANGO=1,\n\
    MOZ_ENABLE_PANGO=)\n\
\n\
if test \"$MOZ_ENABLE_PANGO\" && test -z \"$MOZ_ENABLE_CAIRO_GFX\"\n\
then\n\
    AC_DEFINE(MOZ_ENABLE_PANGO)\n\
    PKG_CHECK_MODULES(MOZ_PANGO, pangoxft >= 1.6.0)\n\
\n\
    AC_SUBST(MOZ_ENABLE_PANGO)\n\
    AC_SUBST(MOZ_PANGO_CFLAGS)\n\
    AC_SUBST(MOZ_PANGO_LIBS)\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_GTK2\" && test \"$MOZ_ENABLE_CAIRO_GFX\"\n\
then\n\
    # For gtk2, we require --enable-pango; gtk2 already implies --enable-xft\n\
    if test -z \"$MOZ_ENABLE_PANGO\"\n\
    then\n\
        AC_MSG_WARN([Pango is required for cairo gfx builds, assuming --enable-pango])\n\
        MOZ_ENABLE_PANGO=1\n\
    fi\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_PANGO\" && test \"$MOZ_ENABLE_CAIRO_GFX\"\n\
then\n\
    AC_DEFINE(MOZ_ENABLE_PANGO)\n\
    dnl PKG_CHECK_MODULES(MOZ_PANGO, pango >= 1.10.0 pangocairo >= 1.10.0)\n\
    if test \"$MOZ_X11\"; then\n\
        PKG_CHECK_MODULES(MOZ_PANGO, pango >= 1.6.0 pangoft2 >= 1.6.0 pangoxft >= 1.6.0)\n\
    else\n\
        PKG_CHECK_MODULES(MOZ_PANGO, pango >= 1.6.0 pangoft2 >= 1.6.0)\n\
    fi\n\
\n\
    AC_SUBST(MOZ_ENABLE_PANGO)\n\
    AC_SUBST(MOZ_PANGO_CFLAGS)\n\
    AC_SUBST(MOZ_PANGO_LIBS)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = x11 core font support (default and ability to enable depend on toolkit)\n\
dnl ========================================================\n\
if test \"$MOZ_X11\"\n\
then\n\
    MOZ_ENABLE_COREXFONTS=${MOZ_ENABLE_COREXFONTS-1}\n\
else\n\
    MOZ_ENABLE_COREXFONTS=\n\
fi\n\
if test \"$MOZ_ENABLE_COREXFONTS\"\n\
then\n\
    AC_DEFINE(MOZ_ENABLE_COREXFONTS)\n\
fi\n\
\n\
AC_SUBST(MOZ_ENABLE_COREXFONTS)\n\
\n\
dnl ========================================================\n\
dnl = PostScript print module\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(postscript,\n\
[  --disable-postscript    Disable PostScript printing support ],\n\
    MOZ_ENABLE_POSTSCRIPT=,\n\
    MOZ_ENABLE_POSTSCRIPT=1 )\n\
\n\
dnl ========================================================\n\
dnl = Xprint print module\n\
dnl ========================================================\n\
if test \"$MOZ_X11\"\n\
then\n\
    _SAVE_LDFLAGS=\"$LDFLAGS\"\n\
    LDFLAGS=\"$XLDFLAGS $LDFLAGS\"\n\
    AC_CHECK_LIB(Xp, XpGetPrinterList, [XPRINT_LIBS=\"-lXp\"],\n\
        MOZ_ENABLE_XPRINT=, $XEXT_LIBS $XLIBS)\n\
    LDFLAGS=\"$_SAVE_LDFLAGS\"\n\
\n\
    MOZ_XPRINT_CFLAGS=\"$X_CFLAGS\"\n\
    MOZ_XPRINT_LDFLAGS=\"$XLDFLAGS $XPRINT_LIBS\"\n\
    MOZ_XPRINT_LDFLAGS=\"$MOZ_XPRINT_LDFLAGS $XEXT_LIBS $X11_LIBS\"\n\
\n\
    MOZ_ARG_DISABLE_BOOL(xprint,\n\
    [  --disable-xprint        Disable Xprint printing support ],\n\
        MOZ_ENABLE_XPRINT=,\n\
        MOZ_ENABLE_XPRINT=1 )\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = GnomeVFS support module\n\
dnl ========================================================\n\
\n\
if test \"$MOZ_X11\"\n\
then\n\
    dnl build the gnomevfs extension by default only when the\n\
    dnl GTK2 toolkit is in use.\n\
    if test \"$MOZ_ENABLE_GTK2\"\n\
    then\n\
        MOZ_ENABLE_GNOMEVFS=1\n\
        MOZ_ENABLE_GCONF=1\n\
        MOZ_ENABLE_LIBGNOME=1\n\
    fi\n\
\n\
    MOZ_ARG_DISABLE_BOOL(gnomevfs,\n\
    [  --disable-gnomevfs      Disable GnomeVFS support ],\n\
        MOZ_ENABLE_GNOMEVFS=,\n\
        MOZ_ENABLE_GNOMEVFS=force)\n\
\n\
    if test \"$MOZ_ENABLE_GNOMEVFS\"\n\
    then\n\
        PKG_CHECK_MODULES(MOZ_GNOMEVFS, gnome-vfs-2.0 >= $GNOMEVFS_VERSION gnome-vfs-module-2.0 >= $GNOMEVFS_VERSION,[\n\
            MOZ_GNOMEVFS_LIBS=`echo $MOZ_GNOMEVFS_LIBS | sed \'s/-llinc\\>//\'`\n\
            MOZ_ENABLE_GNOMEVFS=1\n\
        ],[\n\
            if test \"$MOZ_ENABLE_GNOMEVFS\" = \"force\"\n\
            then\n\
                AC_MSG_ERROR([* * * Could not find gnome-vfs-module-2.0 >= $GNOMEVFS_VERSION])\n\
            fi\n\
            MOZ_ENABLE_GNOMEVFS=\n\
        ])\n\
    fi\n\
\n\
    AC_SUBST(MOZ_GNOMEVFS_CFLAGS)\n\
    AC_SUBST(MOZ_GNOMEVFS_LIBS)\n\
\n\
    if test \"$MOZ_ENABLE_GCONF\"\n\
    then\n\
        PKG_CHECK_MODULES(MOZ_GCONF, gconf-2.0 >= $GCONF_VERSION,[\n\
            MOZ_GCONF_LIBS=`echo $MOZ_GCONF_LIBS | sed \'s/-llinc\\>//\'`\n\
            MOZ_ENABLE_GCONF=1\n\
        ],[\n\
            MOZ_ENABLE_GCONF=\n\
        ])\n\
    fi\n\
\n\
    AC_SUBST(MOZ_GCONF_CFLAGS)\n\
    AC_SUBST(MOZ_GCONF_LIBS)\n\
\n\
    if test \"$MOZ_ENABLE_LIBGNOME\"\n\
    then\n\
        PKG_CHECK_MODULES(MOZ_LIBGNOME, libgnome-2.0 >= $LIBGNOME_VERSION,[\n\
            MOZ_LIBGNOME_LIBS=`echo $MOZ_LIBGNOME_LIBS | sed \'s/-llinc\\>//\'`\n\
            MOZ_ENABLE_LIBGNOME=1\n\
        ],[\n\
            MOZ_ENABLE_LIBGNOME=\n\
        ])\n\
    fi\n\
\n\
    AC_SUBST(MOZ_LIBGNOME_CFLAGS)\n\
    AC_SUBST(MOZ_LIBGNOME_LIBS)\n\
\n\
    # The GNOME component is built if gtk2, gconf, gnome-vfs, and libgnome\n\
    # are all available.\n\
\n\
    if test \"$MOZ_ENABLE_GTK2\" -a \"$MOZ_ENABLE_GCONF\" -a \\\n\
            \"$MOZ_ENABLE_GNOMEVFS\" -a \"$MOZ_ENABLE_LIBGNOME\"; then\n\
      MOZ_ENABLE_GNOME_COMPONENT=1\n\
    else\n\
      MOZ_ENABLE_GNOME_COMPONENT=\n\
    fi\n\
\n\
    AC_SUBST(MOZ_ENABLE_GNOME_COMPONENT)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = libgnomeui support module\n\
dnl ========================================================\n\
\n\
if test \"$MOZ_ENABLE_GTK2\"\n\
then\n\
    MOZ_ENABLE_GNOMEUI=1\n\
\n\
    MOZ_ARG_DISABLE_BOOL(gnomeui,\n\
    [  --disable-gnomeui       Disable libgnomeui support (default: auto, optional at runtime) ],\n\
        MOZ_ENABLE_GNOMEUI=,\n\
        MOZ_ENABLE_GNOMEUI=force)\n\
\n\
    if test \"$MOZ_ENABLE_GNOMEUI\"\n\
    then\n\
        PKG_CHECK_MODULES(MOZ_GNOMEUI, libgnomeui-2.0 >= $GNOMEUI_VERSION,\n\
        [\n\
            MOZ_GNOMEUI_LIBS=`echo $MOZ_GNOMEUI_LIBS | sed \'s/-llinc\\>//\'`\n\
            MOZ_ENABLE_GNOMEUI=1\n\
        ],[\n\
            if test \"$MOZ_ENABLE_GNOMEUI\" = \"force\"\n\
            then\n\
                AC_MSG_ERROR([* * * Could not find libgnomeui-2.0 >= $GNOMEUI_VERSION])\n\
            fi\n\
            MOZ_ENABLE_GNOMEUI=\n\
        ])\n\
    fi\n\
\n\
    if test \"$MOZ_ENABLE_GNOMEUI\"; then\n\
        AC_DEFINE(MOZ_ENABLE_GNOMEUI)\n\
    fi\n\
fi\n\
AC_SUBST(MOZ_ENABLE_GNOMEUI)\n\
AC_SUBST(MOZ_GNOMEUI_CFLAGS)\n\
AC_SUBST(MOZ_GNOMEUI_LIBS)\n\
\n\
dnl ========================================================\n\
dnl = dbus support\n\
dnl ========================================================\n\
\n\
if test \"$MOZ_ENABLE_GTK2\"\n\
then\n\
    MOZ_ENABLE_DBUS=1\n\
\n\
    MOZ_ARG_DISABLE_BOOL(dbus,\n\
    [  --disable-dbus       Disable dbus support (default: auto, optional at runtime) ],\n\
        MOZ_ENABLE_DBUS=,\n\
        MOZ_ENABLE_DBUS=force)\n\
\n\
    if test \"$MOZ_ENABLE_DBUS\"\n\
    then\n\
        PKG_CHECK_MODULES(MOZ_DBUS_GLIB, dbus-glib-1,\n\
        [\n\
            MOZ_ENABLE_DBUS=1\n\
        ],[\n\
            if test \"$MOZ_ENABLE_DBUS\" = \"force\"\n\
            then\n\
                AC_MSG_ERROR([* * * Could not find dbus-glib-1])\n\
            fi\n\
            MOZ_ENABLE_DBUS=\n\
        ])\n\
    fi\n\
\n\
    if test \"$MOZ_ENABLE_DBUS\"; then\n\
        AC_DEFINE(MOZ_ENABLE_DBUS)\n\
    fi\n\
fi\n\
AC_SUBST(MOZ_ENABLE_DBUS)\n\
AC_SUBST(MOZ_DBUS_GLIB_CFLAGS)\n\
AC_SUBST(MOZ_DBUS_GLIB_LIBS)\n\
\n\
dnl ========================================================\n\
dnl = Setting MOZ_EXTRA_X11CONVERTERS turns on additional \n\
dnl = converters in intl/uconv that are used only by X11 gfx \n\
dnl = implementations. By default, it\'s undefined so that \n\
dnl = those converters are not built on other platforms/toolkits. \n\
dnl = (see bug 180851)\n\
dnl ========================================================\n\
\n\
if (test \"$MOZ_ENABLE_GTK\"  || test \"$MOZ_ENABLE_GTK2\") \\\n\
&& test \"$MOZ_ENABLE_COREXFONTS\" \\\n\
|| test \"$MOZ_ENABLE_XLIB\" \\\n\
|| test \"$MOZ_ENABLE_XPRINT\" \n\
then\n\
    AC_DEFINE(MOZ_EXTRA_X11CONVERTERS)\n\
    MOZ_EXTRA_X11CONVERTERS=1\n\
fi\n\
AC_SUBST(MOZ_EXTRA_X11CONVERTERS)\n\
\n\
dnl ========================================================\n\
dnl = Build Personal Security Manager\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(crypto,\n\
[  --disable-crypto        Disable crypto support (Personal Security Manager)],\n\
    MOZ_PSM=,\n\
    MOZ_PSM=1 )\n\
\n\
dnl ========================================================\n\
dnl = JS Debugger XPCOM component (js/jsd)\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(jsd,\n\
[  --disable-jsd           Disable JavaScript debug library],\n\
    MOZ_JSDEBUGGER=,\n\
    MOZ_JSDEBUGGER=1)\n\
\n\
\n\
dnl ========================================================\n\
dnl = Disable plugin support\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(plugins,\n\
[  --disable-plugins       Disable plugins support],\n\
    MOZ_PLUGINS=,\n\
    MOZ_PLUGINS=1)\n\
\n\
dnl ========================================================\n\
dnl = Open JVM Interface (OJI) support\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(oji,\n\
[  --disable-oji           Disable Open JVM Integration support],\n\
    MOZ_OJI=,\n\
    MOZ_OJI=1)\n\
if test -n \"$MOZ_OJI\"; then\n\
    AC_DEFINE(OJI)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = This turns on xinerama support.  We just can\'t use the\n\
dnl = autodetection of the libraries since on Red Hat 7 linking with\n\
dnl = Xinerama crashes the dynamic loader.  Make people turn it on\n\
dnl = explicitly.  The autodetection is done above in the Xlib\n\
dnl = detection routines.\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(xinerama,\n\
[  --enable-xinerama       Enable Xinerama support\n\
                           ( not safe for Red Hat 7.0 ) ],\n\
    _ENABLE_XINERAMA=1,\n\
    _ENABLE_XINERAMA= )\n\
\n\
if test -n \"$_ENABLE_XINERAMA\" -a -n \"$MOZ_XINERAMA_LIBS\" -a \\\n\
    -n \"$ac_cv_header_X11_extensions_Xinerama_h\"; then\n\
    MOZ_ENABLE_XINERAMA=1\n\
    AC_DEFINE(MOZ_ENABLE_XINERAMA)\n\
fi\n\
\n\
dnl bi-directional support always on\n\
IBMBIDI=1\n\
AC_DEFINE(IBMBIDI)\n\
\n\
dnl ========================================================\n\
dnl complex text support off by default\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(ctl,\n\
[  --enable-ctl            Enable Thai Complex Script support],\n\
    SUNCTL=1,\n\
    SUNCTL= )\n\
\n\
dnl ========================================================\n\
dnl view source support on by default\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(view-source,\n\
[  --disable-view-source     Disable view source support],\n\
    MOZ_VIEW_SOURCE=,\n\
    MOZ_VIEW_SOURCE=1 )\n\
if test \"$MOZ_VIEW_SOURCE\"; then\n\
    AC_DEFINE(MOZ_VIEW_SOURCE)\n\
fi\n\
\n\
\n\
dnl ========================================================\n\
dnl accessibility support on by default on all platforms \n\
dnl except OS X.\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(accessibility,\n\
[  --disable-accessibility Disable accessibility support (off by default on OS X)],\n\
    ACCESSIBILITY=,\n\
    ACCESSIBILITY=1 )\n\
if test \"$ACCESSIBILITY\"; then\n\
    AC_DEFINE(ACCESSIBILITY)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl xpfe/components on by default\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xpfe-components,\n\
[  --disable-xpfe-components\n\
                          Disable xpfe components],\n\
    MOZ_XPFE_COMPONENTS=,\n\
    MOZ_XPFE_COMPONENTS=1 )\n\
\n\
dnl ========================================================\n\
dnl xpinstall support on by default\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xpinstall,\n\
[  --disable-xpinstall     Disable xpinstall support],\n\
    MOZ_XPINSTALL=,\n\
    MOZ_XPINSTALL=1 )\n\
if test \"$MOZ_XPINSTALL\"; then\n\
    AC_DEFINE(MOZ_XPINSTALL)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Single profile support off by default\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(single-profile,\n\
[  --enable-single-profile Enable single profile support ],\n\
    MOZ_SINGLE_PROFILE=1,\n\
    MOZ_SINGLE_PROFILE= )\n\
\n\
dnl ========================================================\n\
dnl xpcom js loader support on by default\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(jsloader,\n\
[  --disable-jsloader      Disable xpcom js loader support],\n\
    MOZ_JSLOADER=,\n\
    MOZ_JSLOADER=1 )\n\
if test \"$MOZ_JSLOADER\"; then\n\
    AC_DEFINE(MOZ_JSLOADER)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Disable printing\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(printing,\n\
[  --disable-printing  Disable printing support],\n\
    NS_PRINTING=,\n\
    NS_PRINTING=1 )\n\
\n\
if test \"$NS_PRINTING\"; then\n\
    AC_DEFINE(NS_PRINTING)\n\
    AC_DEFINE(NS_PRINT_PREVIEW)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl use native unicode converters\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(native-uconv,\n\
[  --enable-native-uconv   Enable iconv support],\n\
    MOZ_USE_NATIVE_UCONV=1,\n\
    MOZ_USE_NATIVE_UCONV= )\n\
if test \"$MOZ_USE_NATIVE_UCONV\"; then\n\
    AC_DEFINE(MOZ_USE_NATIVE_UCONV)\n\
fi\n\
if test \"$OS_ARCH\" != \"WINCE\" -a \"$OS_ARCH\" != \"WINNT\" -a \"$MOZ_USE_NATIVE_UCONV\" -a \"$ac_cv_func_iconv\" != \"yes\"; then\n\
    AC_MSG_ERROR([iconv() not found.  Cannot enable native uconv support.])\n\
fi\n\
\n\
\n\
dnl ========================================================\n\
dnl Libeditor can be build as plaintext-only,\n\
dnl or as a full html and text editing component.\n\
dnl We build both by default.\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(plaintext-editor-only,\n\
[  --enable-plaintext-editor-only\n\
                          Allow only plaintext editing],\n\
    MOZ_PLAINTEXT_EDITOR_ONLY=1,\n\
    MOZ_PLAINTEXT_EDITOR_ONLY= )\n\
dnl Note the #define is MOZILLA, not MOZ, for compat with the Mac build.\n\
AC_SUBST(MOZ_PLAINTEXT_EDITOR_ONLY)\n\
\n\
dnl ========================================================\n\
dnl Composer is on by default.\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(composer,\n\
[  --disable-composer      Disable building of Composer],\n\
    MOZ_COMPOSER=,\n\
    MOZ_COMPOSER=1 )\n\
AC_SUBST(MOZ_COMPOSER)\n\
\n\
\n\
dnl ========================================================\n\
dnl = Drop XPCOM Obsolete library\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xpcom-obsolete,\n\
[  --disable-xpcom-obsolete           Disable XPCOM Obsolete Library],\n\
    MOZ_NO_XPCOM_OBSOLETE=1,\n\
    MOZ_NO_XPCOM_OBSOLETE=)\n\
\n\
if test -n \"$MOZ_NO_XPCOM_OBSOLETE\"; then\n\
if test -z \"$MOZ_SINGLE_PROFILE\"; then\n\
    AC_MSG_ERROR([Building without xpcom-obsolete isn\'t support when building full profile support.])\n\
fi\n\
    AC_DEFINE(MOZ_NO_XPCOM_OBSOLETE)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Disable Fast Load\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xpcom-fastload,\n\
[  --disable-xpcom-fastload           Disable XPCOM fastload support],\n\
    MOZ_NO_FAST_LOAD=1,\n\
    MOZ_NO_FAST_LOAD=)\n\
\n\
AC_SUBST(MOZ_NO_FAST_LOAD)\n\
\n\
if test -n \"$MOZ_NO_FAST_LOAD\"; then\n\
    AC_DEFINE(MOZ_NO_FAST_LOAD)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Permissions System\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(permissions,\n\
[  --disable-permissions   Disable permissions (popup and cookie blocking)],\n\
    MOZ_PERMISSIONS=,\n\
    MOZ_PERMISSIONS=1\n\
)\n\
\n\
dnl ========================================================\n\
dnl NegotiateAuth\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(negotiateauth,\n\
[  --disable-negotiateauth Disable GSS-API negotiation ],\n\
    MOZ_AUTH_EXTENSION=,\n\
    MOZ_AUTH_EXTENSION=1 )\n\
\n\
dnl ========================================================\n\
dnl XTF\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xtf,\n\
[  --disable-xtf           Disable XTF (pluggable xml tags) support],\n\
    MOZ_XTF=,\n\
    MOZ_XTF=1 )\n\
if test \"$MOZ_XTF\"; then\n\
  AC_DEFINE(MOZ_XTF)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Inspector APIs\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(inspector-apis,\n\
[  --disable-inspector-apis Disable the DOM inspection APIs ],\n\
    MOZ_NO_INSPECTOR_APIS=1,\n\
    MOZ_NO_INSPECTOR_APIS= )\n\
\n\
dnl ========================================================\n\
dnl XMLExtras\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xmlextras,\n\
[  --disable-xmlextras Disable XMLExtras such as XPointer support ],\n\
    MOZ_XMLEXTRAS=,\n\
    MOZ_XMLEXTRAS=1 )\n\
\n\
dnl ========================================================\n\
dnl Webservices\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(webservices,\n\
[  --disable-webservices Disable Webservices/SOAP support ],\n\
    MOZ_WEBSERVICES=,\n\
    MOZ_WEBSERVICES=1 )\n\
\n\
dnl ========================================================\n\
dnl Pref extensions (autoconfig and system-pref)\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(pref-extensions,\n\
[  --disable-pref-extensions\n\
                          Disable pref extensions such as autoconfig and\n\
                          system-pref],\n\
  MOZ_PREF_EXTENSIONS=,\n\
  MOZ_PREF_EXTENSIONS=1 )\n\
\n\
dnl ========================================================\n\
dnl = Universalchardet\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(universalchardet,\n\
[  --disable-universalchardet\n\
                          Disable universal encoding detection],\n\
  MOZ_UNIVERSALCHARDET=,\n\
  MOZ_UNIVERSALCHARDET=1 )\n\
\n\
dnl ========================================================\n\
dnl JavaXPCOM\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(javaxpcom,\n\
[  --enable-javaxpcom\n\
                          Enable Java-XPCOM bridge],\n\
    MOZ_JAVAXPCOM=1,\n\
    MOZ_JAVAXPCOM= )\n\
\n\
if test -n \"${MOZ_JAVAXPCOM}\"; then\n\
  case \"$host_os\" in\n\
    cygwin*|msvc*|mks*)\n\
      if test -n \"$JAVA_HOME\"; then\n\
        JAVA_HOME=`cygpath -u \\`cygpath -m -s \"$JAVA_HOME\"\\``\n\
      fi\n\
      ;;\n\
    *mingw*)\n\
      if test -n \"$JAVA_HOME\"; then\n\
        JAVA_HOME=`cd \"$JAVA_HOME\" && pwd`\n\
      fi\n\
      ;;\n\
  esac\n\
\n\
  if test -n \"${JAVA_INCLUDE_PATH}\"; then\n\
    dnl Make sure jni.h exists in the given include path.\n\
    if test ! -f \"$JAVA_INCLUDE_PATH/jni.h\"; then\n\
      AC_MSG_ERROR([jni.h was not found in given include path $JAVA_INCLUDE_PATH.])\n\
    fi\n\
  else\n\
    case \"$target_os\" in\n\
      darwin*)\n\
        dnl Default to java system location\n\
        JAVA_INCLUDE_PATH=/System/Library/Frameworks/JavaVM.framework/Headers\n\
        ;;\n\
      *)\n\
        dnl Try $JAVA_HOME\n\
        JAVA_INCLUDE_PATH=\"$JAVA_HOME/include\"\n\
        ;;\n\
    esac\n\
    if test ! -f \"$JAVA_INCLUDE_PATH/jni.h\"; then\n\
      AC_MSG_ERROR([The header jni.h was not found.  Set \\$JAVA_HOME to your java sdk directory, use --with-java-bin-path={java-bin-dir}, or reconfigure with --disable-javaxpcom.])\n\
    fi\n\
  fi\n\
\n\
  if test -n \"${JAVA_BIN_PATH}\"; then\n\
    dnl Look for javac and jar in the specified path.\n\
    JAVA_PATH=\"$JAVA_BIN_PATH\"\n\
  else\n\
    dnl No path specified, so look for javac and jar in $JAVA_HOME & $PATH.\n\
    JAVA_PATH=\"$JAVA_HOME/bin:$PATH\"\n\
  fi\n\
\n\
  AC_PATH_PROG(JAVA, java, :, [$JAVA_PATH])\n\
  AC_PATH_PROG(JAVAC, javac, :, [$JAVA_PATH])\n\
  AC_PATH_PROG(JAR, jar, :, [$JAVA_PATH])\n\
  if test -z \"$JAVA\" || test \"$JAVA\" = \":\" || test -z \"$JAVAC\" || test \"$JAVAC\" = \":\" || test -z \"$JAR\" || test \"$JAR\" = \":\"; then\n\
    AC_MSG_ERROR([The programs java, javac and jar were not found.  Set \\$JAVA_HOME to your java sdk directory, use --with-java-bin-path={java-bin-dir}, or reconfigure with --disable-javaxpcom.])\n\
  fi\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Airbag crash reporting (on by default on supported platforms)\n\
dnl ========================================================\n\
\n\
if test \"$OS_ARCH\" = \"WINNT\" -a -z \"$GNU_CC\"; then\n\
   MOZ_AIRBAG=1\n\
fi\n\
\n\
MOZ_ARG_DISABLE_BOOL(airbag,\n\
[  --disable-airbag          Disable airbag crash reporting],\n\
    MOZ_AIRBAG=,\n\
    MOZ_AIRBAG=1)\n\
\n\
if test -n \"$MOZ_AIRBAG\"; then\n\
   AC_DEFINE(MOZ_AIRBAG)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Build mochitest JS/DOM tests (on by default)\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(mochitest,\n\
[  --disable-mochitest        Disable mochitest harness],\n\
    MOZ_MOCHITEST=,\n\
    MOZ_MOCHITEST=1 )\n\
\n\
dnl ========================================================\n\
dnl = Enable compilation of specific extension modules\n\
dnl ========================================================\n\
\n\
MOZ_ARG_ENABLE_STRING(extensions,\n\
[  --enable-extensions     Enable extensions],\n\
[ for option in `echo $enableval | sed \'s/,/ /g\'`; do\n\
    if test \"$option\" = \"yes\" || test \"$option\" = \"all\"; then\n\
        MOZ_EXTENSIONS=\"$MOZ_EXTENSIONS $MOZ_EXTENSIONS_ALL\"\n\
    elif test \"$option\" = \"no\" || test \"$option\" = \"none\"; then\n\
        MOZ_EXTENSIONS=\"\"\n\
    elif test \"$option\" = \"default\"; then\n\
        MOZ_EXTENSIONS=\"$MOZ_EXTENSIONS $MOZ_EXTENSIONS_DEFAULT\"\n\
    elif test `echo \"$option\" | grep -c \\^-` != 0; then\n\
        option=`echo $option | sed \'s/^-//\'`\n\
        MOZ_EXTENSIONS=`echo \"$MOZ_EXTENSIONS\" | sed \"s/ ${option}//\"`\n\
    else\n\
        MOZ_EXTENSIONS=\"$MOZ_EXTENSIONS $option\"\n\
    fi\n\
done],\n\
    MOZ_EXTENSIONS=\"$MOZ_EXTENSIONS_DEFAULT\")\n\
\n\
if test -z \"$MOZ_ENABLE_GNOMEVFS\" && test `echo \"$MOZ_EXTENSIONS\" | grep -c gnomevfs` -ne 0; then\n\
    # Suppress warning on non-X11 platforms\n\
    if test -n \"$MOZ_X11\"; then\n\
        AC_MSG_WARN([Cannot build gnomevfs without required libraries. Removing gnomevfs from MOZ_EXTENSIONS.])\n\
    fi\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|gnomevfs||\'`\n\
fi\n\
\n\
if test -z \"$MOZ_JSDEBUGGER\" && test `echo \"$MOZ_EXTENSIONS\" | grep -c venkman` -ne 0; then\n\
    AC_MSG_WARN([Cannot build venkman without JavaScript debug library. Removing venkman from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|venkman||\'`\n\
fi\n\
\n\
if test \"$MOZ_XUL_APP\" && test `echo \"$MOZ_EXTENSIONS\" | grep -c help` -ne 0; then\n\
    AC_MSG_WARN([Cannot build old help extension with MOZ_XUL_APP set. Removing help from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|help||\'`\n\
fi\n\
\n\
dnl This might be temporary: build tridentprofile only on Windows\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c tridentprofile` -ne 0 && test \"$OS_ARCH\" != \"WINNT\"; then\n\
    AC_MSG_WARN([tridentprofile extension works only on Windows at this time. Removing tridentprofile from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|tridentprofile||\'`\n\
fi\n\
\n\
dnl cookie must be built before tridentprofile. put it at list\'s end.\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c tridentprofile` -ne 0; then\n\
  MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|tridentprofile||\'`\n\
  MOZ_EXTENSIONS=\"$MOZ_EXTENSIONS tridentprofile\"\n\
fi\n\
\n\
dnl xforms requires xtf and webservices and schema-validation\n\
if test -z \"$MOZ_XTF\" && test `echo \"$MOZ_EXTENSIONS\" | grep -c xforms` -ne 0; then\n\
    AC_MSG_WARN([Cannot build XForms without XTF support.  Removing XForms from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|xforms||g\'`\n\
fi\n\
if test -z \"$MOZ_WEBSERVICES\" && test `echo \"$MOZ_EXTENSIONS\" | grep -c xforms` -ne 0; then\n\
    AC_MSG_WARN([Cannot build XForms without webservices.  Removing XForms from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|xforms||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c xforms` -ne 0 && test `echo \"$MOZ_EXTENSIONS\" | grep -c schema-validation` -eq 0; then\n\
    AC_MSG_WARN([Cannot build XForms without schema validation.  Removing XForms from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|xforms||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c auth` -ne 0; then\n\
    AC_MSG_WARN([auth is no longer an extension, use --disable-negotiateauth to disable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|auth||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c xmlextras` -ne 0; then\n\
    AC_MSG_WARN([xmlextras is no longer an extension, use --disable-xmlextras to disable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|xmlextras||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c \'cookie\\|permissions\'` -ne 0; then\n\
    AC_MSG_WARN([cookie and permissions are no longer extensions, use --disable-permissions to disable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|cookie||g; s|permissions||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c webservices` -ne 0; then\n\
    AC_MSG_WARN([webservices is no longer an extension, use --disable-webservices to disable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|webservices||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c pref` -ne 0; then\n\
    AC_MSG_WARN([pref is no longer an extension, use --disable-pref-extensions to disable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|pref||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c universalchardet` -ne 0; then\n\
    AC_MSG_WARN([universalchardet is no longer an extension, use --disable-universalchardet to disable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|universalchardet||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c java` -ne 0; then\n\
    AC_MSG_WARN([java is no longer an extension, use --enable-javaxpcom to enable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|java||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c spellcheck` -ne 0; then\n\
    AC_MSG_WARN([spellcheck is no longer an extension.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|spellcheck||g\'`\n\
fi\n\
\n\
if test -n \"$MOZ_NO_XPCOM_OBSOLETE\" && test `echo \"$MOZ_EXTENSIONS\" | grep -c sroaming` -ne 0; then\n\
    AC_MSG_WARN([Cannot currently build sroaming without xpcom obsolete -- bug 249343. Removing sroaming from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|sroaming||\'`\n\
fi\n\
\n\
dnl Remove dupes\n\
MOZ_EXTENSIONS=`${PERL} ${srcdir}/build/unix/uniq.pl ${MOZ_EXTENSIONS}`\n\
\n\
dnl ========================================================\n\
dnl Image decoders\n\
dnl ========================================================\n\
case \"$MOZ_WIDGET_TOOLKIT\" in\n\
beos|windows|os2|mac|cocoa)\n\
    ;;\n\
*)\n\
    if test -z \"$MOZ_ENABLE_GNOMEUI\"; then\n\
       MOZ_IMG_DECODERS_DEFAULT=`echo $MOZ_IMG_DECODERS_DEFAULT | sed -e \'s|icon||\'`                \n\
    fi\n\
    ;;\n\
esac\n\
\n\
MOZ_ARG_ENABLE_STRING(image-decoders,\n\
[  --enable-image-decoders[={mod1,mod2,default,all,none}]\n\
                          Enable specific image decoders],\n\
[ for option in `echo $enableval | sed \'s/,/ /g\'`; do\n\
    if test \"$option\" = \"yes\" || test \"$option\" = \"all\"; then\n\
        MOZ_IMG_DECODERS=\"$MOZ_IMG_DECODERS $MOZ_IMG_DECODERS_DEFAULT\"\n\
    elif test \"$option\" = \"no\" || test \"$option\" = \"none\"; then\n\
        MOZ_IMG_DECODERS=\"\"\n\
    elif test \"$option\" = \"default\"; then\n\
        MOZ_IMG_DECODERS=\"$MOZ_IMG_DECODERS $MOZ_IMG_DECODERS_DEFAULT\"\n\
    elif test `echo \"$option\" | grep -c \\^-` != 0; then\n\
        option=`echo $option | sed \'s/^-//\'`\n\
        MOZ_IMG_DECODERS=`echo \"$MOZ_IMG_DECODERS\" | sed \"s/ ${option}//\"`\n\
    else\n\
        MOZ_IMG_DECODERS=\"$MOZ_IMG_DECODERS $option\"\n\
    fi\n\
done],\n\
    MOZ_IMG_DECODERS=\"$MOZ_IMG_DECODERS_DEFAULT\")\n\
\n\
if test `echo \"$MOZ_IMG_DECODERS\" | grep -c png` -ne 0; then\n\
    MOZ_PNG_CFLAGS=\"$MOZ_PNG_CFLAGS -DMOZ_PNG_READ\"\n\
    AC_MSG_CHECKING([if pnggccrd.c can be compiled without PNG_NO_MMX_CODE])\n\
    AC_TRY_COMPILE([#define MOZ_PNG_READ 1\n\
                    #include \"$_topsrcdir/modules/libimg/png/pnggccrd.c\"],,\n\
                    _results=yes,\n\
                    _results=no)\n\
    AC_MSG_RESULT([$_results])\n\
    if test \"$_results\" = \"no\"; then\n\
        MOZ_PNG_CFLAGS=\"$MOZ_PNG_CFLAGS -DPNG_NO_MMX_CODE\"\n\
    fi\n\
fi\n\
\n\
dnl Dupes are removed in the encoder section because it will also add decoders\n\
\n\
dnl ========================================================\n\
dnl Image encoders\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_STRING(image-encoders,\n\
[  --enable-image-encoders[={mod1,mod2,default,all,none}]\n\
                          Enable specific image encoders],\n\
[ for option in `echo $enableval | sed \'s/,/ /g\'`; do\n\
    if test \"$option\" = \"yes\" || test \"$option\" = \"all\"; then\n\
        addencoder=\"$MOZ_IMG_ENCODERS_DEFAULT\"\n\
    elif test \"$option\" = \"no\" || test \"$option\" = \"none\"; then\n\
        MOZ_IMG_ENCODERS=\"\"\n\
        addencoder=\"\"\n\
    elif test \"$option\" = \"default\"; then\n\
        addencoder=\"$MOZ_IMG_ENCODERS_DEFAULT\"\n\
    elif test `echo \"$option\" | grep -c \\^-` != 0; then\n\
        option=`echo $option | sed \'s/^-//\'`\n\
        addencoder=`echo \"$MOZ_IMG_ENCODERS\" | sed \"s/ ${option}//\"`\n\
    else\n\
        addencoder=\"$option\"\n\
    fi\n\
    MOZ_IMG_ENCODERS=\"$MOZ_IMG_ENCODERS $addencoder\"\n\
done],\n\
    MOZ_IMG_ENCODERS=\"$MOZ_IMG_ENCODERS_DEFAULT\")\n\
\n\
if test `echo \"$MOZ_IMG_ENCODERS\" | grep -c png` -ne 0; then\n\
    MOZ_PNG_CFLAGS=\"$MOZ_PNG_CFLAGS -DMOZ_PNG_WRITE\"\n\
fi\n\
\n\
dnl Remove dupes\n\
MOZ_IMG_DECODERS=`${PERL} ${srcdir}/build/unix/uniq.pl ${MOZ_IMG_DECODERS}`\n\
MOZ_IMG_ENCODERS=`${PERL} ${srcdir}/build/unix/uniq.pl ${MOZ_IMG_ENCODERS}`\n\
\n\
dnl ========================================================\n\
dnl experimental ldap features\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(ldap-experimental,\n\
[  --enable-ldap-experimental\n\
                          Enable LDAP experimental features],\n\
    MOZ_LDAP_XPCOM_EXPERIMENTAL=1,\n\
    MOZ_LDAP_XPCOM_EXPERIMENTAL=)\n\
\n\
dnl ========================================================\n\
dnl MathML on by default\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(mathml,\n\
[  --disable-mathml        Disable MathML support],\n\
    MOZ_MATHML=,\n\
    MOZ_MATHML=1 )\n\
if test \"$MOZ_MATHML\"; then\n\
  AC_DEFINE(MOZ_MATHML)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Canvas\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(canvas,\n\
[  --disable-canvas          Disable html:canvas feature],\n\
    MOZ_ENABLE_CANVAS=,\n\
    MOZ_ENABLE_CANVAS=1 )\n\
if test -n \"$MOZ_ENABLE_CANVAS\"; then\n\
    AC_DEFINE(MOZ_ENABLE_CANVAS)\n\
fi\n\
AC_SUBST(MOZ_ENABLE_CANVAS)\n\
\n\
dnl ========================================================\n\
dnl SVG\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(svg,\n\
[  --disable-svg            Disable SVG support],\n\
    MOZ_SVG=,\n\
    MOZ_SVG=1 )\n\
if test -n \"$MOZ_SVG\"; then\n\
  if test -z \"$MOZ_ENABLE_CAIRO_GFX\"; then\n\
    AC_MSG_ERROR([SVG requires cairo gfx])\n\
  else\n\
    AC_DEFINE(MOZ_SVG)\n\
  fi\n\
fi\n\
\n\
MOZ_SVG_FOREIGNOBJECT=$MOZ_ENABLE_CAIRO_GFX\n\
dnl ========================================================\n\
dnl SVG <foreignObject>\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(svg-foreignobject,\n\
   [  --disable-svg-foreignobject\n\
                        Disable SVG <foreignObject> support],\n\
   MOZ_SVG_FOREIGNOBJECT=,\n\
   MOZ_SVG_FOREIGNOBJECT=1 )\n\
if test \"$MOZ_SVG_FOREIGNOBJECT\"; then\n\
  if test -z \"$MOZ_ENABLE_CAIRO_GFX\"; then\n\
    AC_MSG_ERROR([<foreignobject> requires cairo gfx])\n\
  else\n\
    if test \"$MOZ_SVG\"; then\n\
      AC_DEFINE(MOZ_SVG_FOREIGNOBJECT)\n\
    else\n\
      MOZ_SVG_FOREIGNOBEJCT=\n\
    fi\n\
  fi\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Installer\n\
dnl ========================================================\n\
case \"$target_os\" in\n\
    aix*|solaris*|linux*|msvc*|mks*|cygwin*|mingw*|os2*|wince*)\n\
        MOZ_INSTALLER=1\n\
        ;;\n\
esac\n\
\n\
MOZ_ARG_DISABLE_BOOL(installer,\n\
[  --disable-installer     Disable building of installer],\n\
    MOZ_INSTALLER=,\n\
    MOZ_INSTALLER=1 )\n\
if test -n \"$MOZ_INSTALLER\" -a -n \"$MOZ_XUL_APP\" -a \"$OS_ARCH\" = \"WINNT\"; then\n\
    # Disable installer for Windows builds that use the new toolkit if NSIS\n\
    # isn\'t in the path.\n\
    AC_PATH_PROGS(MAKENSIS, makensis)\n\
    if test -z \"$MAKENSIS\" || test \"$MAKENSIS\" = \":\"; then\n\
        AC_MSG_ERROR([To build the installer makensis is required in your path. To build without the installer reconfigure using --disable-installer.])\n\
    fi\n\
    # The Windows build for NSIS requires the iconv command line utility to\n\
    # convert the charset of the locale files.\n\
    AC_PATH_PROGS(HOST_ICONV, $HOST_ICONV \"iconv\", \"\")\n\
    if test -z \"$HOST_ICONV\"; then\n\
        AC_MSG_ERROR([To build the installer iconv is required in your path. To build without the installer reconfigure using --disable-installer.])\n\
    fi\n\
fi\n\
\n\
# Automatically disable installer if xpinstall isn\'t built\n\
if test -z \"$MOZ_XPINSTALL\"; then\n\
    MOZ_INSTALLER=\n\
fi\n\
AC_SUBST(MOZ_INSTALLER)\n\
\n\
AC_MSG_CHECKING([for tar archiver])\n\
AC_CHECK_PROGS(TAR, gnutar gtar tar, \"\")\n\
if test -z \"$TAR\"; then\n\
    AC_MSG_ERROR([no tar archiver found in \\$PATH])\n\
fi\n\
AC_MSG_RESULT([$TAR])\n\
AC_SUBST(TAR)\n\
\n\
dnl ========================================================\n\
dnl Updater\n\
dnl ========================================================\n\
\n\
MOZ_ARG_DISABLE_BOOL(updater,\n\
[  --disable-updater       Disable building of updater],\n\
    MOZ_UPDATER=,\n\
    MOZ_UPDATER=1 )\n\
# The Windows build requires the iconv command line utility\n\
# in order to build the updater.\n\
case \"$target_os\" in\n\
    msvc*|mks*|cygwin*|mingw*|wince*)\n\
        if test -n \"$MOZ_UPDATER\"; then\n\
            AC_MSG_CHECKING([for iconv])\n\
            AC_CHECK_PROGS(HOST_ICONV, $HOST_ICONV \"iconv\", \"\")\n\
            if test -z \"$HOST_ICONV\"; then\n\
                AC_MSG_ERROR([iconv not found in \\$PATH])\n\
            fi\n\
        fi\n\
        ;;\n\
esac\n\
AC_SUBST(MOZ_UPDATER)\n\
\n\
# app update channel is \'default\' when not supplied.\n\
MOZ_ARG_ENABLE_STRING([update-channel],\n\
[  --enable-update-channel=CHANNEL\n\
                           Select application update channel (default=default)],\n\
    MOZ_UPDATE_CHANNEL=`echo $enableval | tr A-Z a-z`)\n\
\n\
if test -z \"$MOZ_UPDATE_CHANNEL\"; then\n\
    MOZ_UPDATE_CHANNEL=default\n\
fi\n\
AC_DEFINE_UNQUOTED(MOZ_UPDATE_CHANNEL, $MOZ_UPDATE_CHANNEL)\n\
\n\
# tools/update-packaging is not checked out by default.\n\
MOZ_ARG_ENABLE_BOOL(update-packaging,\n\
[  --enable-update-packaging\n\
                           Enable tools/update-packaging],\n\
    MOZ_UPDATE_PACKAGING=1,\n\
    MOZ_UPDATE_PACKAGING= )\n\
AC_SUBST(MOZ_UPDATE_PACKAGING)\n\
\n\
dnl ========================================================\n\
dnl ActiveX\n\
dnl ========================================================\n\
\n\
MOZ_ARG_DISABLE_BOOL(xpconnect-idispatch,\n\
[  --disable-xpconnect-idispatch\n\
                          Disable building of xpconnect support for IDispatch\n\
                          (win32 only)],\n\
    XPC_IDISPATCH_SUPPORT=,\n\
    XPC_IDISPATCH_SUPPORT=1)\n\
AC_SUBST(XPC_IDISPATCH_SUPPORT)\n\
\n\
MOZ_ARG_DISABLE_BOOL(activex,\n\
[  --disable-activex       Disable building of ActiveX control (win32 only)],\n\
    MOZ_NO_ACTIVEX_SUPPORT=1,\n\
    MOZ_NO_ACTIVEX_SUPPORT= )\n\
AC_SUBST(MOZ_NO_ACTIVEX_SUPPORT)\n\
\n\
MOZ_ARG_DISABLE_BOOL(activex-scripting,\n\
[  --disable-activex-scripting\n\
                          Disable building of ActiveX scripting support (win32)],\n\
    MOZ_ACTIVEX_SCRIPTING_SUPPORT=,\n\
    MOZ_ACTIVEX_SCRIPTING_SUPPORT=1)\n\
AC_SUBST(MOZ_ACTIVEX_SCRIPTING_SUPPORT)\n\
\n\
if test -n \"$MOZ_NO_ACTIVEX_SUPPORT\" -a -n \"$MOZ_ACTIVEX_SCRIPTING_SUPPORT\";\n\
then\n\
    AC_MSG_ERROR([Cannot enable ActiveX scripting support when ActiveX support is disabled.])\n\
fi\n\
\n\
dnl ========================================================\n\
dnl leaky\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(leaky,\n\
[  --enable-leaky          Build leaky memory tool],\n\
    MOZ_LEAKY=1,\n\
    MOZ_LEAKY=)\n\
\n\
\n\
dnl ========================================================\n\
dnl xpctools\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(xpctools,\n\
[  --enable-xpctools       Build JS profiling tool],\n\
    MOZ_XPCTOOLS=1,\n\
    MOZ_XPCTOOLS= )\n\
\n\
\n\
dnl ========================================================\n\
dnl build the tests by default\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(tests,\n\
[  --disable-tests         Do not build test libraries & programs],\n\
    ENABLE_TESTS=,\n\
    ENABLE_TESTS=1 )\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Module specific options\n\
dnl =\n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Individual module options)\n\
\n\
dnl ========================================================\n\
dnl = Enable Lea malloc in xpcom. OFF by default.\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(xpcom-lea,\n\
[  --enable-xpcom-lea      Use Lea malloc in xpcom ],\n\
    XPCOM_USE_LEA=1,\n\
    XPCOM_USE_LEA= )\n\
if test -n \"$XPCOM_USE_LEA\"; then\n\
    AC_DEFINE(XPCOM_USE_LEA)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Enable places (new history/bookmarks)\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(places,\n\
[  --enable-places        Enable \'places\' bookmark/history implementation],\n\
    MOZ_PLACES=1,\n\
    MOZ_PLACES= )\n\
if test -n \"$MOZ_PLACES\"; then\n\
    AC_DEFINE(MOZ_PLACES)\n\
    MOZ_MORK=\n\
    MOZ_MORKREADER=1\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Enable places bookmarks (use places for bookmarks)\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(places-bookmarks,\n\
[  --enable-places_bookmarks        Enable \'places\' for bookmarks backend],\n\
    MOZ_PLACES_BOOKMARKS=1,\n\
    MOZ_PLACES_BOOKMARKS= )\n\
if test -n \"$MOZ_PLACES_BOOKMARKS\"; then\n\
    AC_DEFINE(MOZ_PLACES_BOOKMARKS)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Disable feed handling components\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(feeds,\n\
[  --disable-feeds        Disable feed handling and processing components],\n\
    MOZ_FEEDS=,\n\
    MOZ_FEEDS=1 )\n\
if test -n \"$MOZ_FEEDS\"; then\n\
    AC_DEFINE(MOZ_FEEDS)\n\
else\n\
    if test \"$MOZ_BUILD_APP\" = \"browser\"; then\n\
        AC_MSG_ERROR([Cannot build Firefox with --disable-feeds.])\n\
    fi\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Enable mozStorage\n\
dnl = XXX need to implement --with-system-sqlite3 (see bug 263381)\n\
dnl ========================================================\n\
dnl Implicitly enabled by default if building calendar or places\n\
MOZ_ARG_ENABLE_BOOL(storage,\n\
[  --enable-storage        Enable mozStorage module and related components],\n\
    MOZ_STORAGE=1,\n\
    MOZ_STORAGE= )\n\
if test -n \"$MOZ_STORAGE\"; then\n\
    AC_DEFINE(MOZ_STORAGE)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Enable safe browsing (anti-phishing)\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(safe-browsing,\n\
[  --enable-safe-browsing        Enable safe browsing (anti-phishing) implementation],\n\
    MOZ_SAFE_BROWSING=1,\n\
    MOZ_SAFE_BROWSING= )\n\
if test -n \"$MOZ_SAFE_BROWSING\"; then\n\
    AC_DEFINE(MOZ_SAFE_BROWSING)\n\
fi\n\
AC_SUBST(MOZ_SAFE_BROWSING)\n\
\n\
dnl ========================================================\n\
dnl = Enable url-classifier\n\
dnl ========================================================\n\
dnl Implicitly enabled by default if building with safe-browsing\n\
if test -n \"$MOZ_SAFE_BROWSING\"; then\n\
    MOZ_URL_CLASSIFIER=1\n\
fi\n\
MOZ_ARG_ENABLE_BOOL(url-classifier,\n\
[  --enable-url-classifier        Enable url classifier module],\n\
    MOZ_URL_CLASSIFIER=1,\n\
    MOZ_URL_CLASSIFIER= )\n\
if test -n \"$MOZ_URL_CLASSIFIER\"; then\n\
    AC_DEFINE(MOZ_URL_CLASSIFIER)\n\
fi\n\
AC_SUBST(MOZ_URL_CLASSIFIER)\n\
\n\
dnl ========================================================\n\
dnl = Enable Ultrasparc specific optimizations for JS\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(js-ultrasparc,\n\
[  --enable-js-ultrasparc  Use UltraSPARC optimizations in JS],\n\
    JS_ULTRASPARC_OPTS=1,\n\
    JS_ULTRASPARC_OPTS= )\n\
\n\
dnl only enable option for ultrasparcs\n\
if test `echo \"$target_os\" | grep -c \\^solaris 2>/dev/null` = 0 -o \\\n\
    \"$OS_TEST\" != \"sun4u\"; then\n\
    JS_ULTRASPARC_OPTS=\n\
fi\n\
AC_SUBST(JS_ULTRASPARC_OPTS)\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Feature options that require extra sources to be pulled\n\
dnl =\n\
dnl ========================================================\n\
dnl MOZ_ARG_HEADER(Features that require extra sources)\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Debugging Options\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Debugging and Optimizations)\n\
\n\
dnl ========================================================\n\
dnl = Disable building with debug info.\n\
dnl = Debugging is OFF by default\n\
dnl ========================================================\n\
if test -z \"$MOZ_DEBUG_FLAGS\"\n\
then\n\
    case \"$target\" in\n\
    *-irix*)\n\
        if test \"$GNU_CC\"; then\n\
            GCC_VERSION=`$CC -v 2>&1 | awk \'/version/ { print $3 }\'`\n\
            case \"$GCC_VERSION\" in\n\
            2.95.*)\n\
                MOZ_DEBUG_FLAGS=\"\"\n\
                ;;\n\
            *)\n\
                MOZ_DEBUG_FLAGS=\"-g\"\n\
                ;;\n\
            esac\n\
        else\n\
            MOZ_DEBUG_FLAGS=\"-g\"\n\
        fi\n\
        ;;\n\
    *)\n\
    	MOZ_DEBUG_FLAGS=\"-g\"\n\
        ;;\n\
    esac\n\
fi\n\
\n\
MOZ_ARG_ENABLE_STRING(debug,\n\
[  --enable-debug[=DBG]    Enable building with developer debug info\n\
                          (Using compiler flags DBG)],\n\
[ if test \"$enableval\" != \"no\"; then\n\
    MOZ_DEBUG=1\n\
    if test -n \"$enableval\" && test \"$enableval\" != \"yes\"; then\n\
        MOZ_DEBUG_FLAGS=`echo $enableval | sed -e \'s|\\\\\\ | |g\'`\n\
    fi\n\
  else\n\
    MOZ_DEBUG=\n\
  fi ],\n\
  MOZ_DEBUG=)\n\
\n\
MOZ_DEBUG_ENABLE_DEFS=\"-DDEBUG -D_DEBUG\"\n\
 case \"${target_os}\" in\n\
    beos*)\n\
        MOZ_DEBUG_ENABLE_DEFS=\"$MOZ_DEBUG_ENABLE_DEFS -DDEBUG_${USER}\"\n\
        ;;\n\
    msvc*|mks*|cygwin*|mingw*|os2*|wince*)\n\
        MOZ_DEBUG_ENABLE_DEFS=\"$MOZ_DEBUG_ENABLE_DEFS -DDEBUG_`echo ${USERNAME} | sed -e \'s| |_|g\'`\"\n\
        ;;\n\
    *) \n\
        MOZ_DEBUG_ENABLE_DEFS=\"$MOZ_DEBUG_ENABLE_DEFS -DDEBUG_`$WHOAMI`\"\n\
        ;;\n\
  esac\n\
MOZ_DEBUG_ENABLE_DEFS=\"$MOZ_DEBUG_ENABLE_DEFS -DTRACING\"\n\
\n\
MOZ_DEBUG_DISABLE_DEFS=\"-DNDEBUG -DTRIMMED\"\n\
\n\
if test -n \"$MOZ_DEBUG\"; then\n\
    AC_MSG_CHECKING([for valid debug flags])\n\
    _SAVE_CFLAGS=$CFLAGS\n\
    CFLAGS=\"$CFLAGS $MOZ_DEBUG_FLAGS\"\n\
    AC_TRY_COMPILE([#include <stdio.h>], \n\
        [printf(\"Hello World\\n\");],\n\
        _results=yes,\n\
        _results=no)\n\
    AC_MSG_RESULT([$_results])\n\
    if test \"$_results\" = \"no\"; then\n\
        AC_MSG_ERROR([These compiler flags are invalid: $MOZ_DEBUG_FLAGS])\n\
    fi\n\
    CFLAGS=$_SAVE_CFLAGS\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Enable code optimization. ON by default.\n\
dnl ========================================================\n\
if test -z \"$MOZ_OPTIMIZE_FLAGS\"; then\n\
	MOZ_OPTIMIZE_FLAGS=\"-O\"\n\
fi\n\
\n\
MOZ_ARG_ENABLE_STRING(optimize,\n\
[  --disable-optimize      Disable compiler optimization\n\
  --enable-optimize=[OPT] Specify compiler optimization flags [OPT=-O]],\n\
[ if test \"$enableval\" != \"no\"; then\n\
    MOZ_OPTIMIZE=1\n\
    if test -n \"$enableval\" && test \"$enableval\" != \"yes\"; then\n\
        MOZ_OPTIMIZE_FLAGS=`echo \"$enableval\" | sed -e \'s|\\\\\\ | |g\'`\n\
        MOZ_OPTIMIZE=2\n\
    fi\n\
else\n\
    MOZ_OPTIMIZE=\n\
fi ], MOZ_OPTIMIZE=1)\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test -n \"$MOZ_OPTIMIZE\"; then\n\
    AC_MSG_CHECKING([for valid optimization flags])\n\
    _SAVE_CFLAGS=$CFLAGS\n\
    CFLAGS=\"$CFLAGS $MOZ_OPTIMIZE_FLAGS\"\n\
    AC_TRY_COMPILE([#include <stdio.h>], \n\
        [printf(\"Hello World\\n\");],\n\
        _results=yes,\n\
        _results=no)\n\
    AC_MSG_RESULT([$_results])\n\
    if test \"$_results\" = \"no\"; then\n\
        AC_MSG_ERROR([These compiler flags are invalid: $MOZ_OPTIMIZE_FLAGS])\n\
    fi\n\
    CFLAGS=$_SAVE_CFLAGS\n\
fi\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
AC_SUBST(MOZ_OPTIMIZE)\n\
AC_SUBST(MOZ_OPTIMIZE_FLAGS)\n\
AC_SUBST(MOZ_OPTIMIZE_LDFLAGS)\n\
\n\
dnl ========================================================\n\
dnl = Enable/disable debug for specific modules only\n\
dnl =   module names beginning with ^ will be disabled \n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_STRING(debug-modules,\n\
[  --enable-debug-modules  Enable/disable debug info for specific modules],\n\
[ MOZ_DEBUG_MODULES=`echo $enableval| sed \'s/,/ /g\'` ] )\n\
\n\
dnl ========================================================\n\
dnl = Enable/disable generation of debugger info for specific modules only\n\
dnl =    the special module name ALL_MODULES can be used to denote all modules\n\
dnl =    module names beginning with ^ will be disabled\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_STRING(debugger-info-modules,\n\
[  --enable-debugger-info-modules\n\
                          Enable/disable debugger info for specific modules],\n\
[ for i in `echo $enableval | sed \'s/,/ /g\'`; do\n\
      dnl note that the list of module names is reversed as it is copied\n\
      dnl this is important, as it will allow config.mk to interpret stuff like\n\
      dnl \"^ALL_MODULES xpcom\" properly\n\
      if test \"$i\" = \"no\"; then\n\
        i=\"^ALL_MODULES\"\n\
      fi\n\
      if test \"$i\" = \"yes\"; then\n\
        i=\"ALL_MODULES\"\n\
      fi\n\
      MOZ_DBGRINFO_MODULES=\"$i $MOZ_DBGRINFO_MODULES\";\n\
    done ])\n\
\n\
dnl ========================================================\n\
dnl Enable garbage collector\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(boehm,\n\
[  --enable-boehm          Enable the Boehm Garbage Collector],\n\
    GC_LEAK_DETECTOR=1,\n\
    GC_LEAK_DETECTOR= )\n\
if test -n \"$GC_LEAK_DETECTOR\"; then\n\
    AC_DEFINE(GC_LEAK_DETECTOR)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Disable runtime logging checks\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(logging,\n\
[  --disable-logging       Disable logging facilities],\n\
    NS_DISABLE_LOGGING=1,\n\
    NS_DISABLE_LOGGING= )\n\
if test \"$NS_DISABLE_LOGGING\"; then\n\
    AC_DEFINE(NS_DISABLE_LOGGING)\n\
else\n\
    AC_DEFINE(MOZ_LOGGING)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = dnl This will enable logging of addref, release, ctor, dtor.\n\
dnl ========================================================\n\
_ENABLE_LOGREFCNT=42\n\
MOZ_ARG_ENABLE_BOOL(logrefcnt,\n\
[  --enable-logrefcnt      Enable logging of refcounts (default=debug) ],\n\
    _ENABLE_LOGREFCNT=1,\n\
    _ENABLE_LOGREFCNT= )\n\
if test \"$_ENABLE_LOGREFCNT\" = \"1\"; then\n\
    AC_DEFINE(FORCE_BUILD_REFCNT_LOGGING)\n\
elif test -z \"$_ENABLE_LOGREFCNT\"; then\n\
    AC_DEFINE(NO_BUILD_REFCNT_LOGGING)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Use malloc wrapper lib\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(wrap-malloc,\n\
[  --enable-wrap-malloc    Wrap malloc calls (gnu linker only)],\n\
    _WRAP_MALLOC=1,\n\
    _WRAP_MALLOC= )\n\
\n\
if test -n \"$_WRAP_MALLOC\"; then\n\
    if test \"$GNU_CC\"; then\n\
    WRAP_MALLOC_CFLAGS=\"${LDFLAGS} -Wl,--wrap -Wl,malloc -Wl,--wrap -Wl,free -Wl,--wrap -Wl,realloc -Wl,--wrap -Wl,__builtin_new -Wl,--wrap -Wl,__builtin_vec_new -Wl,--wrap -Wl,__builtin_delete -Wl,--wrap -Wl,__builtin_vec_delete -Wl,--wrap -Wl,PR_Free -Wl,--wrap -Wl,PR_Malloc -Wl,--wrap -Wl,PR_Calloc -Wl,--wrap -Wl,PR_Realloc\"\n\
    MKSHLIB=\'$(CXX) $(DSO_LDOPTS) $(WRAP_MALLOC_CFLAGS) -o $@\'\n\
    fi\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Location of malloc wrapper lib\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(wrap-malloc,\n\
[  --with-wrap-malloc=DIR  Location of malloc wrapper library],\n\
    WRAP_MALLOC_LIB=$withval)\n\
\n\
dnl ========================================================\n\
dnl = Use Electric Fence\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(efence,\n\
[  --enable-efence         Link with Electric Fence],\n\
    _ENABLE_EFENCE=1,\n\
    _ENABLE_EFENCE= )\n\
if test -n \"$_ENABLE_EFENCE\"; then\n\
    AC_CHECK_LIB(efence,malloc)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl jprof\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(jprof,\n\
[  --enable-jprof          Enable jprof profiling tool (needs mozilla/tools/jprof)],\n\
    MOZ_JPROF=1,\n\
    MOZ_JPROF= )\n\
if test -n \"$MOZ_JPROF\"; then\n\
    AC_DEFINE(MOZ_JPROF)\n\
fi\n\
\n\
\n\
dnl ========================================================\n\
dnl = Enable stripping of libs & executables\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(strip,\n\
[  --enable-strip          Enable stripping of libs & executables ],\n\
    ENABLE_STRIP=1,\n\
    ENABLE_STRIP= )\n\
\n\
dnl ========================================================\n\
dnl = --enable-elf-dynstr-gc\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(elf-dynstr-gc,\n\
[  --enable-elf-dynstr-gc  Enable elf dynstr garbage collector (opt builds only)],\n\
    USE_ELF_DYNSTR_GC=1,\n\
    USE_ELF_DYNSTR_GC= )\n\
\n\
dnl ========================================================\n\
dnl = --enable-old-abi-compat-wrappers\n\
dnl ========================================================\n\
dnl on x86 linux, the current builds of some popular plugins (notably\n\
dnl flashplayer and real) expect a few builtin symbols from libgcc\n\
dnl which were available in some older versions of gcc.  However,\n\
dnl they\'re _NOT_ available in newer versions of gcc (eg 3.1), so if\n\
dnl we want those plugin to work with a gcc-3.1 built binary, we need\n\
dnl to provide these symbols.  MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS defaults\n\
dnl to true on x86 linux, and false everywhere else.\n\
dnl\n\
\n\
MOZ_ARG_ENABLE_BOOL(old-abi-compat-wrappers,\n\
[  --enable-old-abi-compat-wrappers\n\
                          Support old GCC ABI symbols to ease the pain \n\
                          of the linux compiler change],\n\
    MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS=1,\n\
    MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS= )\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test \"$MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS\"; then\n\
    AC_LANG_SAVE\n\
    AC_LANG_CPLUSPLUS\n\
    AC_CHECK_FUNCS(__builtin_vec_new __builtin_vec_delete __builtin_new __builtin_delete __pure_virtual)\n\
    AC_LANG_RESTORE\n\
    AC_DEFINE(MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS)\n\
fi\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl ========================================================\n\
dnl = --enable-prebinding\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(prebinding,\n\
[  --enable-prebinding     Enable prebinding (Mac OS X only)],\n\
    USE_PREBINDING=1,\n\
    USE_PREBINDING= )\n\
\n\
dnl ========================================================\n\
dnl = \n\
dnl = Profiling and Instrumenting\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Profiling and Instrumenting)\n\
\n\
dnl ========================================================\n\
dnl = Enable timeline service, which provides lightweight\n\
dnl = instrumentation of mozilla for performance measurement.\n\
dnl = Timeline is off by default.\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(timeline,\n\
[  --enable-timeline       Enable timeline services ],\n\
    MOZ_TIMELINE=1,\n\
    MOZ_TIMELINE= )\n\
if test -n \"$MOZ_TIMELINE\"; then\n\
    AC_DEFINE(MOZ_TIMELINE)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Turn on reflow counting\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(reflow-perf,\n\
[  --enable-reflow-perf    Enable reflow performance tracing],\n\
    MOZ_REFLOW_PERF=1,\n\
    MOZ_REFLOW_PERF= )\n\
if test -n \"$MOZ_REFLOW_PREF\"; then\n\
    AC_DEFINE(MOZ_REFLOW_PREF)\n\
fi\n\
AC_SUBST(MOZ_REFLOW_PERF)\n\
\n\
dnl ========================================================\n\
dnl Enable performance metrics.\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(perf-metrics,\n\
[  --enable-perf-metrics   Enable performance metrics],\n\
    MOZ_PERF_METRICS=1,\n\
    MOZ_PERF_METRICS= )\n\
if test -n \"$MOZ_PERF_METRICS\"; then\n\
    AC_DEFINE(MOZ_PERF_METRICS)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Enable code size metrics.\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(codesighs,\n\
[  --enable-codesighs      Enable code size analysis tools],\n\
    _ENABLE_CODESIGHS=1,\n\
    _ENABLE_CODESIGHS= )\n\
if test -n \"$_ENABLE_CODESIGHS\"; then\n\
    if test -d $srcdir/tools/codesighs; then \n\
        MOZ_MAPINFO=1\n\
    else\n\
        AC_MSG_ERROR([Codesighs directory $srcdir/tools/codesighs required.])\n\
    fi\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Enable trace malloc\n\
dnl ========================================================\n\
NS_TRACE_MALLOC=${MOZ_TRACE_MALLOC}\n\
MOZ_ARG_ENABLE_BOOL(trace-malloc,\n\
[  --enable-trace-malloc   Enable malloc tracing],\n\
    NS_TRACE_MALLOC=1,\n\
    NS_TRACE_MALLOC= )\n\
if test \"$NS_TRACE_MALLOC\"; then\n\
  # Please, Mr. Linker Man, don\'t take away our symbol names\n\
  MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\n\
  USE_ELF_DYNSTR_GC=\n\
  AC_DEFINE(NS_TRACE_MALLOC)\n\
fi\n\
AC_SUBST(NS_TRACE_MALLOC)\n\
\n\
dnl ========================================================\n\
dnl = Add support for Eazel profiler\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(eazel-profiler-support,\n\
[  --enable-eazel-profiler-support\n\
                          Enable Corel/Eazel profiler support],\n\
    ENABLE_EAZEL_PROFILER=1,\n\
    ENABLE_EAZEL_PROFILER= )\n\
if test -n \"$ENABLE_EAZEL_PROFILER\"; then\n\
    AC_DEFINE(ENABLE_EAZEL_PROFILER)\n\
    USE_ELF_DYNSTR_GC=\n\
    MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\n\
    EAZEL_PROFILER_CFLAGS=\"-g -O -gdwarf-2 -finstrument-functions -D__NO_STRING_INLINES  -D__NO_MATH_INLINES\"\n\
    EAZEL_PROFILER_LIBS=\"-lprofiler -lpthread\"\n\
fi\n\
\n\
MOZ_ARG_ENABLE_STRING(profile-modules,\n\
[  --enable-profile-modules\n\
                          Enable/disable profiling for specific modules],\n\
[ MOZ_PROFILE_MODULES=`echo $enableval| sed \'s/,/ /g\'` ] )\n\
\n\
MOZ_ARG_ENABLE_BOOL(insure,\n\
[  --enable-insure         Enable insure++ instrumentation (linux only)],\n\
    _ENABLE_INSURE=1,\n\
    _ENABLE_INSURE= )\n\
if test -n \"$_ENABLE_INSURE\"; then\n\
    MOZ_INSURE=\"insure\"\n\
    MOZ_INSURIFYING=1\n\
    MOZ_INSURE_DIRS=\".\"\n\
    MOZ_INSURE_EXCLUDE_DIRS=\"config\"\n\
fi\n\
\n\
MOZ_ARG_WITH_STRING(insure-dirs,\n\
[  --with-insure-dirs=DIRS\n\
                          Dirs to instrument with insure ],\n\
    MOZ_INSURE_DIRS=$withval )\n\
\n\
MOZ_ARG_WITH_STRING(insure-exclude-dirs,\n\
[  --with-insure-exclude-dirs=DIRS\n\
                          Dirs to not instrument with insure ],\n\
    MOZ_INSURE_EXCLUDE_DIRS=\"config $withval\" )\n\
\n\
dnl ========================================================\n\
dnl = Support for Quantify (Windows)\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(quantify,\n\
[  --enable-quantify      Enable Quantify support (Windows only) ],\n\
    MOZ_QUANTIFY=1,\n\
    MOZ_QUANTIFY= )\n\
\n\
dnl ========================================================\n\
dnl = Support for demangling undefined symbols\n\
dnl ========================================================\n\
if test -z \"$SKIP_LIBRARY_CHECKS\"; then\n\
    AC_LANG_SAVE\n\
    AC_LANG_CPLUSPLUS\n\
    AC_CHECK_FUNCS(__cxa_demangle, HAVE_DEMANGLE=1, HAVE_DEMANGLE=)\n\
    AC_LANG_RESTORE\n\
fi\n\
\n\
# Demangle only for debug or trace-malloc builds\n\
MOZ_DEMANGLE_SYMBOLS=\n\
if test \"$HAVE_DEMANGLE\" -a \"$HAVE_GCC3_ABI\" && test \"$MOZ_DEBUG\" -o \"$NS_TRACE_MALLOC\"; then\n\
    MOZ_DEMANGLE_SYMBOLS=1\n\
    AC_DEFINE(MOZ_DEMANGLE_SYMBOLS)\n\
fi\n\
AC_SUBST(MOZ_DEMANGLE_SYMBOLS)\n\
\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Misc. Options\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Misc. Options)\n\
\n\
dnl ========================================================\n\
dnl update xterm title\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(xterm-updates,\n\
[  --enable-xterm-updates  Update XTERM titles with current command.],\n\
    MOZ_UPDATE_XTERM=1,\n\
    MOZ_UPDATE_XTERM= )\n\
\n\
dnl =========================================================\n\
dnl = Chrome format\n\
dnl =========================================================\n\
MOZ_ARG_ENABLE_STRING([chrome-format],\n\
[  --enable-chrome-format=jar|flat|both|symlink\n\
                          Select FORMAT of chrome files (default=jar)],\n\
    MOZ_CHROME_FILE_FORMAT=`echo $enableval | tr A-Z a-z`)\n\
\n\
if test -z \"$MOZ_CHROME_FILE_FORMAT\"; then\n\
    MOZ_CHROME_FILE_FORMAT=jar\n\
fi\n\
\n\
if test \"$MOZ_CHROME_FILE_FORMAT\" != \"jar\" && \n\
    test \"$MOZ_CHROME_FILE_FORMAT\" != \"flat\" &&\n\
    test \"$MOZ_CHROME_FILE_FORMAT\" != \"symlink\" &&\n\
    test \"$MOZ_CHROME_FILE_FORMAT\" != \"both\"; then\n\
    AC_MSG_ERROR([--enable-chrome-format must be set to either jar, flat, both, or symlink])\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Define default location for MOZILLA_FIVE_HOME\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(default-mozilla-five-home,\n\
[  --with-default-mozilla-five-home\n\
                          Set the default value for MOZILLA_FIVE_HOME],\n\
[ val=`echo $withval`\n\
  AC_DEFINE_UNQUOTED(MOZ_DEFAULT_MOZILLA_FIVE_HOME,\"$val\") ])\n\
\n\
dnl ========================================================\n\
dnl = Location of the mozilla user directory (default is ~/.mozilla).],\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(user-appdir,\n\
[  --with-user-appdir=DIR  Set user-specific appdir (default=.mozilla)],\n\
[ val=`echo $withval`\n\
if echo \"$val\" | grep \"\\/\" >/dev/null; then\n\
    AC_MSG_ERROR(\"Homedir must be single relative path.\")\n\
else \n\
    MOZ_USER_DIR=\"$val\"\n\
fi])\n\
\n\
AC_DEFINE_UNQUOTED(MOZ_USER_DIR,\"$MOZ_USER_DIR\")\n\
\n\
dnl ========================================================\n\
dnl = Doxygen configuration\n\
dnl ========================================================\n\
dnl Use commas to specify multiple dirs to this arg\n\
MOZ_DOC_INPUT_DIRS=\'./dist/include ./dist/idl\'\n\
MOZ_ARG_WITH_STRING(doc-input-dirs,\n\
[  --with-doc-input-dirs=DIRS\n\
                          Header/idl dirs to create docs from],\n\
[ MOZ_DOC_INPUT_DIRS=`echo \"$withval\" | sed \"s/,/ /g\"` ] )\n\
AC_SUBST(MOZ_DOC_INPUT_DIRS)\n\
\n\
dnl Use commas to specify multiple dirs to this arg\n\
MOZ_DOC_INCLUDE_DIRS=\'./dist/include ./dist/include/nspr\'\n\
MOZ_ARG_WITH_STRING(doc-include-dirs,\n\
[  --with-doc-include-dirs=DIRS  \n\
                          Include dirs to preprocess doc headers],\n\
[ MOZ_DOC_INCLUDE_DIRS=`echo \"$withval\" | sed \"s/,/ /g\"` ] )\n\
AC_SUBST(MOZ_DOC_INCLUDE_DIRS)\n\
\n\
MOZ_DOC_OUTPUT_DIR=\'./dist/docs\'\n\
MOZ_ARG_WITH_STRING(doc-output-dir,\n\
[  --with-doc-output-dir=DIR\n\
                          Dir to generate docs into],\n\
[ MOZ_DOC_OUTPUT_DIR=$withval ] )\n\
AC_SUBST(MOZ_DOC_OUTPUT_DIR)\n\
\n\
if test -z \"$SKIP_COMPILER_CHECKS\"; then\n\
dnl ========================================================\n\
dnl =\n\
dnl = Compiler Options\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Compiler Options)\n\
\n\
dnl ========================================================\n\
dnl Check for gcc -pipe support\n\
dnl ========================================================\n\
AC_MSG_CHECKING([for gcc -pipe support])\n\
if test -n \"$GNU_CC\" && test -n \"$GNU_CXX\" && test -n \"$GNU_AS\"; then\n\
    echo \'#include <stdio.h>\' > dummy-hello.c\n\
    echo \'int main() { printf(\"Hello World\\n\"); exit(0); }\' >> dummy-hello.c\n\
    ${CC} -S dummy-hello.c -o dummy-hello.s 2>&5\n\
    cat dummy-hello.s | ${AS_BIN} -o dummy-hello.S - 2>&5\n\
    if test $? = 0; then\n\
        _res_as_stdin=\"yes\"\n\
    else\n\
        _res_as_stdin=\"no\"\n\
    fi\n\
    if test \"$_res_as_stdin\" = \"yes\"; then\n\
        _SAVE_CFLAGS=$CFLAGS\n\
        CFLAGS=\"$CFLAGS -pipe\"\n\
        AC_TRY_COMPILE( [ #include <stdio.h> ],\n\
            [printf(\"Hello World\\n\");],\n\
            [_res_gcc_pipe=\"yes\"],\n\
            [_res_gcc_pipe=\"no\"] )\n\
        CFLAGS=$_SAVE_CFLAGS\n\
    fi\n\
    if test \"$_res_as_stdin\" = \"yes\" && test \"$_res_gcc_pipe\" = \"yes\"; then\n\
        _res=\"yes\";\n\
        CFLAGS=\"$CFLAGS -pipe\"\n\
        CXXFLAGS=\"$CXXFLAGS -pipe\"\n\
    else\n\
        _res=\"no\"\n\
    fi\n\
    rm -f dummy-hello.c dummy-hello.s dummy-hello.S dummy-hello a.out\n\
    AC_MSG_RESULT([$_res])\n\
else\n\
    AC_MSG_RESULT([no])\n\
fi\n\
\n\
dnl pass -Wno-long-long to the compiler\n\
MOZ_ARG_ENABLE_BOOL(long-long-warning,\n\
[  --enable-long-long-warning \n\
                          Warn about use of non-ANSI long long type],\n\
    _IGNORE_LONG_LONG_WARNINGS=,\n\
    _IGNORE_LONG_LONG_WARNINGS=1)\n\
\n\
if test \"$_IGNORE_LONG_LONG_WARNINGS\"; then\n\
     _SAVE_CFLAGS=\"$CFLAGS\"\n\
     CFLAGS=\"$CFLAGS ${_COMPILER_PREFIX}-Wno-long-long\"\n\
     AC_MSG_CHECKING([whether compiler supports -Wno-long-long])\n\
     AC_TRY_COMPILE([], [return(0);], \n\
	[ _WARNINGS_CFLAGS=\"${_WARNINGS_CFLAGS} ${_COMPILER_PREFIX}-Wno-long-long\"\n\
	  _WARNINGS_CXXFLAGS=\"${_WARNINGS_CXXFLAGS} ${_COMPILER_PREFIX}-Wno-long-long\"\n\
	  result=\"yes\" ], result=\"no\")\n\
     AC_MSG_RESULT([$result])\n\
     CFLAGS=\"$_SAVE_CFLAGS\"\n\
fi\n\
\n\
dnl Test for profiling options\n\
dnl Under gcc 3.3, use -fprofile-arcs/-fbranch-probabilities\n\
dnl Under gcc 3.4+, use -fprofile-generate/-fprofile-use\n\
\n\
_SAVE_CFLAGS=\"$CFLAGS\"\n\
CFLAGS=\"$CFLAGS -fprofile-generate\"\n\
\n\
AC_MSG_CHECKING([whether C compiler supports -fprofile-generate])\n\
AC_TRY_COMPILE([], [return 0;],\n\
               [ PROFILE_GEN_CFLAGS=\"-fprofile-generate\"\n\
                 result=\"yes\" ], result=\"no\")\n\
AC_MSG_RESULT([$result])\n\
\n\
if test $result = \"yes\"; then\n\
  PROFILE_USE_CFLAGS=\"-fprofile-use\"\n\
else\n\
  CFLAGS=\"$_SAVE_CFLAGS -fprofile-arcs\"\n\
  AC_MSG_CHECKING([whether C compiler supports -fprofile-arcs])\n\
  AC_TRY_COMPILE([], [return 0;],\n\
                 [ PROFILE_GEN_CFLAGS=\"-fprofile-arcs\"\n\
                   result=\"yes\" ], result=\"no\")\n\
  AC_MSG_RESULT([$result])\n\
  if test $result = \"yes\"; then\n\
    PROFILE_USE_CFLAGS=\"-fbranch-probabilities\"\n\
  fi\n\
fi\n\
\n\
CFLAGS=\"$_SAVE_CFLAGS\"\n\
\n\
AC_SUBST(PROFILE_GEN_CFLAGS)\n\
AC_SUBST(PROFILE_USE_CFLAGS)\n\
\n\
AC_LANG_CPLUSPLUS\n\
\n\
dnl ========================================================\n\
dnl Test for -pedantic bustage\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(pedantic,\n\
[  --disable-pedantic      Issue all warnings demanded by strict ANSI C ],\n\
_PEDANTIC= )\n\
if test \"$_PEDANTIC\"; then\n\
    _SAVE_CXXFLAGS=$CXXFLAGS\n\
    CXXFLAGS=\"$CXXFLAGS ${_WARNINGS_CXXFLAGS} ${_COMPILER_PREFIX}-pedantic\"\n\
    AC_MSG_CHECKING([whether C++ compiler has -pedantic long long bug])\n\
    AC_TRY_COMPILE([$configure_static_assert_macros],\n\
                   [CONFIGURE_STATIC_ASSERT(sizeof(long long) == 8)],\n\
                   result=\"no\", result=\"yes\" )\n\
    AC_MSG_RESULT([$result])\n\
    CXXFLAGS=\"$_SAVE_CXXFLAGS\"\n\
\n\
    case \"$result\" in\n\
    no)\n\
        _WARNINGS_CFLAGS=\"${_WARNINGS_CFLAGS} ${_COMPILER_PREFIX}-pedantic\"\n\
        _WARNINGS_CXXFLAGS=\"${_WARNINGS_CXXFLAGS} ${_COMPILER_PREFIX}-pedantic\"\n\
        ;;\n\
    yes)\n\
        AC_MSG_ERROR([Your compiler appears to have a known bug where long long is miscompiled when using -pedantic.  Reconfigure using --disable-pedantic. ])\n\
        ;;\n\
    esac\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Test for correct temporary object destruction order\n\
dnl ========================================================\n\
dnl We want to make sure the compiler follows the C++ spec here as \n\
dnl xpcom and the string classes depend on it (bug 235381).\n\
AC_MSG_CHECKING([for correct temporary object destruction order])\n\
AC_TRY_RUN([ class A {\n\
             public:  A(int& x) : mValue(x) {}\n\
                      ~A() { mValue--; }\n\
                      operator char**() { return 0; }\n\
             private:  int& mValue;\n\
             };\n\
             void func(char **arg) {}\n\
             int m=2;\n\
             void test() {\n\
                  func(A(m));\n\
                  if (m==1) m = 0;\n\
             }\n\
             int main() {\n\
                 test();\n\
                 return(m);\n\
             }\n\
             ],\n\
     result=\"yes\", result=\"no\", result=\"maybe\")\n\
AC_MSG_RESULT([$result])\n\
\n\
if test \"$result\" = \"no\"; then\n\
    AC_MSG_ERROR([Your compiler does not follow the C++ specification for temporary object destruction order.])\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Autoconf test for gcc 2.7.2.x (and maybe others?) so that we don\'t\n\
dnl provide non-const forms of the operator== for comparing nsCOMPtrs to\n\
dnl raw pointers in nsCOMPtr.h.  (VC++ has the same bug.)\n\
dnl ========================================================\n\
_SAVE_CXXFLAGS=$CXXFLAGS\n\
CXXFLAGS=\"$CXXFLAGS ${_WARNINGS_CXXFLAGS}\"\n\
AC_CACHE_CHECK(for correct overload resolution with const and templates,\n\
    ac_nscap_nonconst_opeq_bug,\n\
    [AC_TRY_COMPILE([\n\
                      template <class T>\n\
                      class Pointer\n\
                        {\n\
                        public:\n\
                          T* myPtr;\n\
                        };\n\
                      \n\
                      template <class T, class U>\n\
                      int operator==(const Pointer<T>& rhs, U* lhs)\n\
                        {\n\
                          return rhs.myPtr == lhs;\n\
                        }\n\
                      \n\
                      template <class T, class U>\n\
                      int operator==(const Pointer<T>& rhs, const U* lhs)\n\
                        {\n\
                          return rhs.myPtr == lhs;\n\
                        }\n\
                    ],\n\
                    [\n\
                      Pointer<int> foo;\n\
                      const int* bar;\n\
                      return foo == bar;\n\
                    ],\n\
                    ac_nscap_nonconst_opeq_bug=\"no\",\n\
                    ac_nscap_nonconst_opeq_bug=\"yes\")])\n\
CXXFLAGS=\"$_SAVE_CXXFLAGS\"\n\
\n\
if test \"$ac_nscap_nonconst_opeq_bug\" = \"yes\" ; then\n\
    AC_DEFINE(NSCAP_DONT_PROVIDE_NONCONST_OPEQ)\n\
fi\n\
fi # SKIP_COMPILER_CHECKS\n\
\n\
dnl ========================================================\n\
dnl C++ rtti\n\
dnl Should be smarter and check that the compiler does indeed have rtti\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(cpp-rtti,\n\
[  --enable-cpp-rtti       Enable C++ RTTI ],\n\
[ _MOZ_USE_RTTI=1 ],\n\
[ _MOZ_USE_RTTI= ])\n\
\n\
if test \"$_MOZ_USE_RTTI\"; then\n\
   _MOZ_RTTI_FLAGS=$_MOZ_RTTI_FLAGS_ON\n\
else\n\
   _MOZ_RTTI_FLAGS=$_MOZ_RTTI_FLAGS_OFF\n\
fi\n\
\n\
AC_SUBST(_MOZ_RTTI_FLAGS_ON)\n\
\n\
dnl ========================================================\n\
dnl C++ exceptions (g++/egcs only - for now)\n\
dnl Should be smarter and check that the compiler does indeed have exceptions\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(cpp-exceptions,\n\
[  --enable-cpp-exceptions Enable C++ exceptions ],\n\
[ _MOZ_CPP_EXCEPTIONS=1 ],\n\
[ _MOZ_CPP_EXCEPTIONS= ])\n\
\n\
if test \"$_MOZ_CPP_EXCEPTIONS\"; then\n\
    _MOZ_EXCEPTIONS_FLAGS=$_MOZ_EXCEPTIONS_FLAGS_ON\n\
else\n\
    _MOZ_EXCEPTIONS_FLAGS=$_MOZ_EXCEPTIONS_FLAGS_OFF\n\
fi\n\
\n\
# Irix & OSF native compilers do not like exception declarations \n\
# when exceptions are disabled\n\
if test -n \"$MIPSPRO_CXX\" -o -n \"$COMPAQ_CXX\" -o -n \"$VACPP\"; then\n\
    AC_DEFINE(CPP_THROW_NEW, [])\n\
else\n\
    AC_DEFINE(CPP_THROW_NEW, [throw()])\n\
fi\n\
AC_LANG_C\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Build depencency options\n\
dnl =\n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Build dependencies)\n\
\n\
dnl ========================================================\n\
dnl = Do not auto generate dependency info\n\
dnl ========================================================\n\
MOZ_AUTO_DEPS=1\n\
MOZ_ARG_DISABLE_BOOL(auto-deps,\n\
[  --disable-auto-deps     Do not automatically generate dependency info],\n\
    MOZ_AUTO_DEPS=,\n\
    MOZ_AUTO_DEPS=1)\n\
\n\
if test -n \"$MOZ_AUTO_DEPS\"; then\n\
dnl ========================================================\n\
dnl = Use mkdepend instead of $CC -MD for dependency generation\n\
dnl ========================================================\n\
_cpp_md_flag=\n\
MOZ_ARG_DISABLE_BOOL(md,\n\
[  --disable-md            Do not use compiler-based dependencies ],\n\
  [_cpp_md_flag=],\n\
  [_cpp_md_flag=1],\n\
  [dnl Default is to turn on -MD if using GNU-compatible compilers\n\
   if test \"$GNU_CC\" -a \"$GNU_CXX\" -a \"$OS_ARCH\" != \"WINNT\" -a \"$OS_ARCH\" != \"WINCE\"; then\n\
     _cpp_md_flag=1\n\
   fi])\n\
if test \"$_cpp_md_flag\"; then\n\
  COMPILER_DEPEND=1\n\
  if test \"$OS_ARCH\" = \"OpenVMS\"; then\n\
    _DEPEND_CFLAGS=\'$(subst =, ,$(filter-out %/.pp,-MM=-MD=-MF=$(MDDEPDIR)/$(*F).pp))\'\n\
  else\n\
    _DEPEND_CFLAGS=\'$(filter-out %/.pp,-Wp,-MD,$(MDDEPDIR)/$(*F).pp)\'\n\
  fi\n\
else\n\
  COMPILER_DEPEND=\n\
  _USE_CPP_INCLUDE_FLAG=\n\
  _DEFINES_CFLAGS=\'$(ACDEFINES) -D_MOZILLA_CONFIG_H_ -DMOZILLA_CLIENT\'\n\
  _DEFINES_CXXFLAGS=\'$(ACDEFINES) -D_MOZILLA_CONFIG_H_ -DMOZILLA_CLIENT\'\n\
fi\n\
fi # MOZ_AUTO_DEPS\n\
MDDEPDIR=\'.deps\'\n\
AC_SUBST(MOZ_AUTO_DEPS)\n\
AC_SUBST(COMPILER_DEPEND)\n\
AC_SUBST(MDDEPDIR)\n\
\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Static Build Options\n\
dnl =\n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Static build options)\n\
\n\
MOZ_ARG_ENABLE_BOOL(static,\n\
[  --enable-static         Enable building of internal static libs],\n\
    BUILD_STATIC_LIBS=1,\n\
    BUILD_STATIC_LIBS=)\n\
\n\
MOZ_ARG_ENABLE_BOOL(libxul,\n\
[  --enable-libxul         Enable building of libxul],\n\
    MOZ_ENABLE_LIBXUL=1,\n\
    MOZ_ENABLE_LIBXUL=)\n\
\n\
if test -n \"$MOZ_ENABLE_LIBXUL\" -a -n \"$BUILD_STATIC_LIBS\"; then\n\
	AC_MSG_ERROR([--enable-libxul is not compatible with --enable-static])\n\
fi\n\
\n\
if test -n \"$MOZ_ENABLE_LIBXUL\" -a -z \"$MOZ_XUL_APP\"; then\n\
	AC_MSG_ERROR([--enable-libxul is only compatible with toolkit XUL applications.])\n\
fi\n\
\n\
if test -n \"$MOZ_ENABLE_LIBXUL\"; then\n\
    XPCOM_LIBS=\"$LIBXUL_LIBS\"\n\
    AC_DEFINE(MOZ_ENABLE_LIBXUL)\n\
else\n\
    if test -n \"$BUILD_STATIC_LIBS\"; then\n\
        AC_DEFINE(MOZ_STATIC_BUILD)\n\
    fi\n\
    XPCOM_LIBS=\"$DYNAMIC_XPCOM_LIBS\"\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Force JS to be a static lib\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(js-static-build,\n\
[  --enable-js-static-build  Force js to be a static lib],\n\
    JS_STATIC_BUILD=1,\n\
    JS_STATIC_BUILD= )\n\
\n\
AC_SUBST(JS_STATIC_BUILD)\n\
        \n\
if test -n \"$JS_STATIC_BUILD\"; then\n\
    AC_DEFINE(EXPORT_JS_API)\n\
\n\
if test -z \"$BUILD_STATIC_LIBS\"; then\n\
    AC_MSG_ERROR([--enable-js-static-build is only compatible with --enable-static])\n\
fi\n\
\n\
fi\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Standalone module options\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Standalone module options (Not for building Mozilla))\n\
\n\
dnl Check for GLib and libIDL.\n\
dnl ========================================================\n\
case \"$target_os\" in\n\
msvc*|mks*|cygwin*|mingw*|wince*)\n\
    SKIP_IDL_CHECK=\"yes\"\n\
    ;;\n\
*)\n\
    SKIP_IDL_CHECK=\"no\"\n\
    ;;\n\
esac\n\
\n\
if test -z \"$COMPILE_ENVIRONMENT\"; then\n\
    SKIP_IDL_CHECK=\"yes\"\n\
fi\n\
\n\
dnl = Allow users to disable libIDL checking for standalone modules\n\
MOZ_ARG_WITHOUT_BOOL(libIDL,\n\
[  --without-libIDL        Skip check for libIDL (standalone modules only)],\n\
	SKIP_IDL_CHECK=\"yes\")\n\
\n\
if test \"$SKIP_IDL_CHECK\" = \"no\"\n\
then\n\
    _LIBIDL_FOUND=\n\
    if test \"$MACOS_SDK_DIR\"; then \n\
      dnl xpidl, and therefore libIDL, is only needed on the build host.\n\
      dnl Don\'t build it against the SDK, as that causes problems.\n\
      _MACSAVE_CFLAGS=\"$CFLAGS\"\n\
      _MACSAVE_LIBS=\"$LIBS\"\n\
      _MACSAVE_LDFLAGS=\"$LDFLAGS\"\n\
      _MACSAVE_NEXT_ROOT=\"$NEXT_ROOT\"\n\
      changequote(,)\n\
      CFLAGS=`echo $CFLAGS|sed -E -e \"s%((-I|-isystem )${MACOS_SDK_DIR}/usr/(include|lib/gcc)[^ ]*)|-F${MACOS_SDK_DIR}(/System)?/Library/Frameworks[^ ]*|-nostdinc[^ ]*|-isysroot ${MACOS_SDK_DIR}%%g\"`\n\
      LIBS=`echo $LIBS|sed -e \"s?-L${MACOS_SDK_DIR}/usr/lib[^ ]*??g\"`\n\
      LDFLAGS=`echo $LDFLAGS|sed -e \"s?-Wl,-syslibroot,${MACOS_SDK_DIR}??g\"`\n\
      changequote([,])\n\
      unset NEXT_ROOT\n\
    fi\n\
\n\
    if test \"$MOZ_ENABLE_GTK2\"; then\n\
        PKG_CHECK_MODULES(LIBIDL, libIDL-2.0 >= 0.8.0 glib-2.0 gobject-2.0, _LIBIDL_FOUND=1,_LIBIDL_FOUND=)\n\
    fi\n\
    if test \"$MOZ_ENABLE_GTK\"; then\n\
        AM_PATH_LIBIDL($LIBIDL_VERSION,_LIBIDL_FOUND=1)\n\
    fi\n\
    dnl if no gtk/libIDL1 or gtk2/libIDL2 combination was found, fall back\n\
    dnl to either libIDL1 or libIDL2.\n\
    if test -z \"$_LIBIDL_FOUND\"; then\n\
        AM_PATH_LIBIDL($LIBIDL_VERSION,_LIBIDL_FOUND=1)\n\
        if test -z \"$_LIBIDL_FOUND\"; then\n\
            PKG_CHECK_MODULES(LIBIDL, libIDL-2.0 >= 0.8.0,_LIBIDL_FOUND=1)\n\
        fi\n\
    fi\n\
    dnl\n\
    dnl If we don\'t have a libIDL config program & not cross-compiling, \n\
    dnl     look for orbit-config instead.\n\
    dnl\n\
    if test -z \"$_LIBIDL_FOUND\" && test -z \"$CROSS_COMPILE\"; then\n\
        AC_PATH_PROGS(ORBIT_CONFIG, $ORBIT_CONFIG orbit-config)\n\
        if test -n \"$ORBIT_CONFIG\"; then\n\
            AC_MSG_CHECKING([for ORBit libIDL usability])\n\
        	_ORBIT_CFLAGS=`${ORBIT_CONFIG} client --cflags`\n\
    	    _ORBIT_LIBS=`${ORBIT_CONFIG} client --libs`\n\
            _ORBIT_INC_PATH=`${PERL} -e \'{ for $f (@ARGV) { print \"$f \" if ($f =~ m/^-I/); } }\' -- ${_ORBIT_CFLAGS}`\n\
            _ORBIT_LIB_PATH=`${PERL} -e \'{ for $f (@ARGV) { print \"$f \" if ($f =~ m/^-L/); } }\' -- ${_ORBIT_LIBS}`\n\
            LIBIDL_CFLAGS=\"$_ORBIT_INC_PATH\"\n\
            LIBIDL_LIBS=\"$_ORBIT_LIB_PATH -lIDL -lglib\"\n\
            LIBIDL_CONFIG=\n\
            _SAVE_CFLAGS=\"$CFLAGS\"\n\
            _SAVE_LIBS=\"$LIBS\"\n\
            CFLAGS=\"$LIBIDL_CFLAGS $CFLAGS\"\n\
            LIBS=\"$LIBIDL_LIBS $LIBS\"\n\
            AC_TRY_RUN([\n\
#include <stdlib.h>\n\
#include <libIDL/IDL.h>\n\
int main() {\n\
  char *s;\n\
  s=strdup(IDL_get_libver_string());\n\
  if(s==NULL) {\n\
    exit(1);\n\
  }\n\
  exit(0);\n\
}\n\
            ], [_LIBIDL_FOUND=1\n\
                result=\"yes\"],\n\
               [LIBIDL_CFLAGS=\n\
                LIBIDL_LIBS=\n\
                result=\"no\"],\n\
               [_LIBIDL_FOUND=1\n\
                result=\"maybe\"] )\n\
            AC_MSG_RESULT($result)\n\
            CFLAGS=\"$_SAVE_CFLAGS\"\n\
            LIBS=\"$_SAVE_LIBS\"\n\
        fi\n\
    fi\n\
    if test -z \"$_LIBIDL_FOUND\"; then\n\
        AC_MSG_ERROR([libIDL not found.\n\
        libIDL $LIBIDL_VERSION or higher is required.])\n\
    fi\n\
    if test \"$MACOS_SDK_DIR\"; then\n\
      CFLAGS=\"$_MACSAVE_CFLAGS\"\n\
      LIBS=\"$_MACSAVE_LIBS\"\n\
      LDFLAGS=\"$_MACSAVE_LDFLAGS\"\n\
      if test -n \"$_MACSAVE_NEXT_ROOT\" ; then\n\
        export NEXT_ROOT=\"$_MACSAVE_NEXT_ROOT\"\n\
      fi\n\
    fi\n\
fi\n\
\n\
if test -n \"$CROSS_COMPILE\"; then\n\
     if test -z \"$HOST_LIBIDL_CONFIG\"; then\n\
        HOST_LIBIDL_CONFIG=\"$LIBIDL_CONFIG\"\n\
    fi\n\
    if test -n \"$HOST_LIBIDL_CONFIG\" && test \"$HOST_LIBIDL_CONFIG\" != \"no\"; then\n\
        HOST_LIBIDL_CFLAGS=`${HOST_LIBIDL_CONFIG} --cflags`\n\
        HOST_LIBIDL_LIBS=`${HOST_LIBIDL_CONFIG} --libs`\n\
    else\n\
        HOST_LIBIDL_CFLAGS=\"$LIBIDL_CFLAGS\"\n\
        HOST_LIBIDL_LIBS=\"$LIBIDL_LIBS\"\n\
    fi\n\
fi\n\
\n\
if test -z \"$SKIP_PATH_CHECKS\"; then\n\
if test -z \"${GLIB_CFLAGS}\" || test -z \"${GLIB_LIBS}\" ; then\n\
    if test \"$MOZ_ENABLE_GTK2\"; then\n\
        PKG_CHECK_MODULES(GLIB, glib-2.0 >= 1.3.7 gobject-2.0)\n\
    else\n\
        AM_PATH_GLIB(${GLIB_VERSION})\n\
    fi\n\
fi\n\
fi\n\
\n\
if test -z \"${GLIB_GMODULE_LIBS}\" -a -n \"${GLIB_CONFIG}\"; then\n\
    GLIB_GMODULE_LIBS=`$GLIB_CONFIG gmodule --libs`\n\
fi\n\
\n\
AC_SUBST(LIBIDL_CFLAGS)\n\
AC_SUBST(LIBIDL_LIBS)\n\
AC_SUBST(STATIC_LIBIDL)\n\
AC_SUBST(GLIB_CFLAGS)\n\
AC_SUBST(GLIB_LIBS)\n\
AC_SUBST(GLIB_GMODULE_LIBS)\n\
AC_SUBST(HOST_LIBIDL_CONFIG)\n\
AC_SUBST(HOST_LIBIDL_CFLAGS)\n\
AC_SUBST(HOST_LIBIDL_LIBS)\n\
\n\
dnl ========================================================\n\
dnl Check for libart\n\
dnl ========================================================\n\
if test \"$MOZ_SVG_RENDERER_LIBART\"; then\n\
  if test ! -f $topsrcdir/other-licenses/libart_lgpl/Makefile.in; then\n\
    AC_MSG_ERROR([You must check out the mozilla version of libart. Use\n\
mk_add_options MOZ_CO_MODULE=mozilla/other-licenses/libart_lgpl])\n\
  fi\n\
\n\
  dnl libart\'s configure hasn\'t been run yet, but we know what the\n\
  dnl answer should be anyway\n\
  MOZ_LIBART_CFLAGS=\'-I${DIST}/include/libart_lgpl\'\n\
  case \"$target_os\" in\n\
  msvc*|mks*|cygwin*|mingw*|wince*)\n\
      MOZ_LIBART_LIBS=\'$(LIBXUL_DIST)/lib/$(LIB_PREFIX)moz_art_lgpl.$(IMPORT_LIB_SUFFIX)\' \n\
      ;;\n\
  os2*)\n\
      MOZ_LIBART_LIBS=\'-lmoz_art -lm\'\n\
      ;;\n\
  beos*)\n\
      MOZ_LIBART_LIBS=\'-lmoz_art_lgpl -lroot -lbe\'\n\
      ;;\n\
  *)\n\
      MOZ_LIBART_LIBS=\'-lmoz_art_lgpl -lm\'\n\
      ;;\n\
  esac\n\
  AC_FUNC_ALLOCA\n\
fi\n\
\n\
AC_SUBST(MOZ_LIBART_CFLAGS)\n\
AC_SUBST(MOZ_LIBART_LIBS)\n\
\n\
dnl ========================================================\n\
dnl Check for cairo\n\
dnl ========================================================\n\
if test \"$MOZ_SVG\" -o \"$MOZ_ENABLE_CANVAS\" -o \"$MOZ_ENABLE_CAIRO_GFX\" ; then\n\
   MOZ_CAIRO_CFLAGS=\'-I$(LIBXUL_DIST)/include/cairo\'\n\
\n\
   MOZ_TREE_CAIRO=1\n\
   MOZ_ARG_ENABLE_BOOL(system-cairo,\n\
   [ --enable-system-cairo Use system cairo (located with pkgconfig)],\n\
   MOZ_TREE_CAIRO=,\n\
   MOZ_TREE_CAIRO=1 )\n\
\n\
   if test \"$MOZ_TREE_CAIRO\"; then\n\
       # Check for headers defining standard int types.\n\
       AC_CHECK_HEADERS(stdint.h inttypes.h sys/int_types.h)\n\
\n\
       # For now we assume that we will have a uint64_t available through\n\
       # one of the above headers or mozstdint.h.\n\
       AC_DEFINE(HAVE_UINT64_T)\n\
\n\
       # Define macros for cairo-features.h\n\
       if test \"$MOZ_X11\"; then\n\
           XLIB_SURFACE_FEATURE=\"#define CAIRO_HAS_XLIB_SURFACE 1\"\n\
           PS_SURFACE_FEATURE=\"#define CAIRO_HAS_PS_SURFACE 1\"\n\
           PDF_SURFACE_FEATURE=\"#define CAIRO_HAS_PDF_SURFACE 1\"\n\
           FT_FONT_FEATURE=\"#define CAIRO_HAS_FT_FONT 1\"\n\
           MOZ_ENABLE_CAIRO_FT=1\n\
           CAIRO_FT_CFLAGS=\"$FT2_CFLAGS\"\n\
       fi\n\
       if test \"$MOZ_WIDGET_TOOLKIT\" = \"mac\" -o \"$MOZ_WIDGET_TOOLKIT\" = \"cocoa\"; then\n\
           QUARTZ_SURFACE_FEATURE=\"#define CAIRO_HAS_QUARTZ_SURFACE 1\"\n\
           NQUARTZ_SURFACE_FEATURE=\"#define CAIRO_HAS_NQUARTZ_SURFACE 1\"\n\
           ATSUI_FONT_FEATURE=\"#define CAIRO_HAS_ATSUI_FONT 1\"\n\
       fi\n\
       if test \"$MOZ_WIDGET_TOOLKIT\" = \"windows\"; then\n\
           WIN32_SURFACE_FEATURE=\"#define CAIRO_HAS_WIN32_SURFACE 1\"\n\
           WIN32_FONT_FEATURE=\"#define CAIRO_HAS_WIN32_FONT 1\"\n\
           PDF_SURFACE_FEATURE=\"#define CAIRO_HAS_PDF_SURFACE 1\"\n\
       fi\n\
       if test \"$MOZ_WIDGET_TOOLKIT\" = \"os2\"; then\n\
           OS2_SURFACE_FEATURE=\"#define CAIRO_HAS_OS2_SURFACE 1\"\n\
           FT_FONT_FEATURE=\"#define CAIRO_HAS_FT_FONT 1\"\n\
           MOZ_ENABLE_CAIRO_FT=1\n\
           CAIRO_FT_CFLAGS=\"-I${MZFTCFGFT2}/include\"\n\
           CAIRO_FT_LIBS=\"-L${MZFTCFGFT2}/lib -lmozft -lmzfntcfg\"\n\
       fi\n\
       if test \"$MOZ_ENABLE_GLITZ\"; then\n\
           GLITZ_SURFACE_FEATURE=\"#define CAIRO_HAS_GLITZ_SURFACE 1\"\n\
       fi\n\
       if test \"$MOZ_WIDGET_TOOLKIT\" = \"beos\"; then\n\
           PKG_CHECK_MODULES(CAIRO_FT, fontconfig freetype2)\n\
           BEOS_SURFACE_FEATURE=\"#define CAIRO_HAS_BEOS_SURFACE 1\"\n\
           FT_FONT_FEATURE=\"#define CAIRO_HAS_FT_FONT 1\"\n\
           MOZ_ENABLE_CAIRO_FT=1\n\
       fi\n\
       AC_SUBST(MOZ_ENABLE_CAIRO_FT)\n\
       AC_SUBST(CAIRO_FT_CFLAGS)\n\
\n\
       if test \"$MOZ_DEBUG\"; then\n\
         SANITY_CHECKING_FEATURE=\"#define CAIRO_DO_SANITY_CHECKING 1\"\n\
       else\n\
         SANITY_CHECKING_FEATURE=\"#undef CAIRO_DO_SANITY_CHECKING\"\n\
       fi\n\
\n\
       PNG_FUNCTIONS_FEATURE=\"#define CAIRO_HAS_PNG_FUNCTIONS 1\"\n\
\n\
       AC_SUBST(PS_SURFACE_FEATURE)\n\
       AC_SUBST(PDF_SURFACE_FEATURE)\n\
       AC_SUBST(SVG_SURFACE_FEATURE)\n\
       AC_SUBST(XLIB_SURFACE_FEATURE)\n\
       AC_SUBST(QUARTZ_SURFACE_FEATURE)\n\
       AC_SUBST(NQUARTZ_SURFACE_FEATURE)\n\
       AC_SUBST(XCB_SURFACE_FEATURE)\n\
       AC_SUBST(WIN32_SURFACE_FEATURE)\n\
       AC_SUBST(OS2_SURFACE_FEATURE)\n\
       AC_SUBST(BEOS_SURFACE_FEATURE)\n\
       AC_SUBST(GLITZ_SURFACE_FEATURE)\n\
       AC_SUBST(DIRECTFB_SURFACE_FEATURE)\n\
       AC_SUBST(FT_FONT_FEATURE)\n\
       AC_SUBST(WIN32_FONT_FEATURE)\n\
       AC_SUBST(ATSUI_FONT_FEATURE)\n\
       AC_SUBST(PNG_FUNCTIONS_FEATURE)\n\
\n\
       if test \"$_WIN32_MSVC\"; then\n\
           MOZ_CAIRO_LIBS=\'$(DEPTH)/gfx/cairo/cairo/src/mozcairo.lib $(DEPTH)/gfx/cairo/libpixman/src/mozlibpixman.lib\'\n\
           if test \"$MOZ_ENABLE_GLITZ\"; then\n\
               MOZ_CAIRO_LIBS=\"$MOZ_CAIRO_LIBS \"\'$(DEPTH)/gfx/cairo/glitz/src/mozglitz.lib $(DEPTH)/gfx/cairo/glitz/src/wgl/mozglitzwgl.lib\'\n\
           fi\n\
       else\n\
           MOZ_CAIRO_LIBS=\'-L$(DEPTH)/gfx/cairo/cairo/src -lmozcairo -L$(DEPTH)/gfx/cairo/libpixman/src -lmozlibpixman\'\" $CAIRO_FT_LIBS\"\n\
\n\
           if test \"$MOZ_X11\"; then\n\
               MOZ_CAIRO_LIBS=\"$MOZ_CAIRO_LIBS $XLDFLAGS -lXrender -lfreetype -lfontconfig\"\n\
           fi\n\
\n\
           if test \"$MOZ_ENABLE_GLITZ\"; then\n\
               MOZ_CAIRO_LIBS=\"$MOZ_CAIRO_LIBS \"\'-L$(DEPTH)/gfx/cairo/glitz/src -lmozglitz\'\n\
               if test \"$MOZ_X11\"; then\n\
                   MOZ_CAIRO_LIBS=\"$MOZ_CAIRO_LIBS \"\'-L$(DEPTH)/gfx/cairo/glitz/src/glx -lmozglitzglx -lGL\'\n\
               fi\n\
               if test \"$MOZ_WIDGET_TOOLKIT\" = \"windows\"; then\n\
                   MOZ_CAIRO_LIBS=\"$MOZ_CAIRO_LIBS \"\'-L$(DEPTH)/gfx/cairo/glitz/src/wgl -lmozglitzwgl\'\n\
               fi\n\
           fi\n\
       fi\n\
   else\n\
      PKG_CHECK_MODULES(CAIRO, cairo >= $CAIRO_VERSION freetype2 fontconfig)\n\
      MOZ_CAIRO_CFLAGS=$CAIRO_CFLAGS\n\
      MOZ_CAIRO_LIBS=$CAIRO_LIBS\n\
   fi\n\
fi\n\
\n\
AC_SUBST(MOZ_TREE_CAIRO)\n\
AC_SUBST(MOZ_CAIRO_CFLAGS)\n\
AC_SUBST(MOZ_CAIRO_LIBS)\n\
\n\
dnl ========================================================\n\
dnl disable xul\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xul,\n\
[  --disable-xul           Disable XUL],\n\
    MOZ_XUL= )\n\
if test \"$MOZ_XUL\"; then\n\
  AC_DEFINE(MOZ_XUL)\n\
else\n\
  dnl remove extensions that require XUL\n\
  MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s/inspector//\' -e \'s/venkman//\' -e \'s/irc//\' -e \'s/tasks//\'`\n\
fi\n\
\n\
AC_SUBST(MOZ_XUL)\n\
\n\
dnl ========================================================\n\
dnl Two ways to enable Python support:\n\
dnl   --enable-extensions=python # select all available.\n\
dnl    (MOZ_PYTHON_EXTENSIONS contains the list of extensions)\n\
dnl or:\n\
dnl   --enable-extensions=python/xpcom,... # select individual ones\n\
dnl\n\
dnl If either is used, we locate the Python to use.\n\
dnl ========================================================\n\
dnl\n\
dnl If \'python\' appears anywhere in the extensions list, go lookin\'...\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c python` -ne 0; then\n\
    dnl Allow PYTHON to point to the Python interpreter to use\n\
    dnl (note that it must be the python executable - which we\n\
    dnl run to locate the relevant paths etc)\n\
    dnl If not set, we use whatever Python we can find.\n\
    MOZ_PYTHON=$PYTHON\n\
    dnl Ask Python what its version number is\n\
    changequote(,)\n\
    MOZ_PYTHON_VER=`$PYTHON -c \"import sys;print \'%d%d\' % sys.version_info[0:2]\"`\n\
    MOZ_PYTHON_VER_DOTTED=`$PYTHON -c \"import sys;print \'%d.%d\' % sys.version_info[0:2]\"`\n\
    changequote([,])\n\
    dnl Ask for the Python \"prefix\" (ie, home/source dir)\n\
    MOZ_PYTHON_PREFIX=`$PYTHON -c \"import sys; print sys.prefix\"`\n\
    dnl Setup the include and library directories.\n\
    if test \"$OS_ARCH\" = \"WINNT\"; then\n\
        MOZ_PYTHON_PREFIX=`$CYGPATH_W $MOZ_PYTHON_PREFIX | $CYGPATH_S`\n\
        dnl Source trees have \"include\" and \"PC\" for .h, and \"PCbuild\" for .lib\n\
        dnl Binary trees have \"include\" for .h, and \"libs\" for .lib\n\
        dnl We add \'em both - along with quotes, to handle spaces.\n\
        MOZ_PYTHON_DLL_SUFFIX=.pyd\n\
        MOZ_PYTHON_INCLUDES=\"\\\"-I$MOZ_PYTHON_PREFIX/include\\\" \\\"-I$MOZ_PYTHON_PREFIX/PC\\\"\"\n\
        MOZ_PYTHON_LIBS=\"\\\"/libpath:$MOZ_PYTHON_PREFIX/PCBuild\\\" \\\"/libpath:$MOZ_PYTHON_PREFIX/libs\\\"\"\n\
    else\n\
        dnl Non-Windows include and libs\n\
        MOZ_PYTHON_DLL_SUFFIX=$DLL_SUFFIX\n\
        MOZ_PYTHON_INCLUDES=\"-I$MOZ_PYTHON_PREFIX/include/python$MOZ_PYTHON_VER_DOTTED\"\n\
        dnl Check for dynamic Python lib\n\
        dnl - A static Python is no good - multiple dynamic libraries (xpcom\n\
        dnl - core, xpcom loader, pydom etc) all need to share Python.\n\
        dnl - Python 2.3\'s std --enable-shared configure option will\n\
        dnl   create a libpython2.3.so.1.0. We should first try this\n\
        dnl   dotted versioned .so file because this is the one that\n\
        dnl   the PyXPCOM build mechanics tries to link to.\n\
        dnl   XXX Should find a better way than hardcoding \"1.0\".\n\
        dnl - Python developement tree dir layouts are NOT allowed here\n\
        dnl   because the PyXPCOM build just dies on it later anyway.\n\
        dnl - Fixes to the above by Python/*nix knowledgable people welcome!\n\
        if test -f \"$MOZ_PYTHON_PREFIX/lib/libpython$MOZ_PYTHON_VER_DOTTED.so.1.0\"; then\n\
            MOZ_PYTHON_LIBS=\"-L$MOZ_PYTHON_PREFIX/lib -lpython$MOZ_PYTHON_VER_DOTTED\"\n\
        elif test -f \"$MOZ_PYTHON_PREFIX/lib/libpython$MOZ_PYTHON_VER_DOTTED.so\"; then\n\
            MOZ_PYTHON_LIBS=\"-L$MOZ_PYTHON_PREFIX/lib -lpython$MOZ_PYTHON_VER_DOTTED\"\n\
        elif test -f \"$MOZ_PYTHON_PREFIX/libpython$MOZ_PYTHON_VER_DOTTED.so\"; then\n\
            dnl Don\'t Python development tree directory layout.\n\
            MOZ_PYTHON_LIBS=\"-L$MOZ_PYTHON_PREFIX -lpython$MOZ_PYTHON_VER_DOTTED\"\n\
            AC_MSG_ERROR([The Python at $MOZ_PYTHON_PREFIX looks like a dev tree. The PyXPCOM build cannot handle this yet. You must \'make install\' Python and use the installed tree.])\n\
        elif test \"$OS_ARCH\" = \"Darwin\"; then\n\
            dnl We do Darwin last, so if a custom non-framework build of\n\
            dnl python is used on OSX, then it will be picked up first by\n\
            dnl the logic above.\n\
            MOZ_PYTHON_LIBS=\"-framework Python\"\n\
        else\n\
            AC_MSG_ERROR([Could not find build shared libraries for Python at $MOZ_PYTHON_PREFIX.  This is required for PyXPCOM.])\n\
        fi\n\
        if test \"$OS_ARCH\" = \"Linux\"; then\n\
            MOZ_PYTHON_LIBS=\"$MOZ_PYTHON_LIBS -lutil\"\n\
        fi\n\
    fi\n\
    dnl Handle \"_d\" on Windows\n\
    if test \"$OS_ARCH\" = \"WINNT\" && test -n \"$MOZ_DEBUG\"; then\n\
        MOZ_PYTHON_DEBUG_SUFFIX=\"_d\"\n\
    else\n\
        MOZ_PYTHON_DEBUG_SUFFIX=\n\
    fi\n\
    AC_MSG_RESULT(Building Python extensions using python-$MOZ_PYTHON_VER_DOTTED from $MOZ_PYTHON_PREFIX)\n\
fi\n\
\n\
dnl If the user asks for the \'python\' extension, then we add\n\
dnl MOZ_PYTHON_EXTENSIONS to MOZ_EXTENSIONS - but with the leading \'python/\'\n\
dnl Note the careful regex - it must match \'python\' followed by anything\n\
dnl other than a \'/\', including the end-of-string.\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c \'python\\([[^/]]\\|$\\)\'` -ne 0; then\n\
    for pyext in $MOZ_PYTHON_EXTENSIONS; do\n\
        MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS python/$pyext`\n\
    done\n\
fi\n\
dnl Later we may allow MOZ_PYTHON_EXTENSIONS to be specified on the\n\
dnl command-line, but not yet\n\
AC_SUBST(MOZ_PYTHON_EXTENSIONS)\n\
AC_SUBST(MOZ_PYTHON)\n\
AC_SUBST(MOZ_PYTHON_PREFIX)\n\
AC_SUBST(MOZ_PYTHON_INCLUDES)\n\
AC_SUBST(MOZ_PYTHON_LIBS)\n\
AC_SUBST(MOZ_PYTHON_VER)\n\
AC_SUBST(MOZ_PYTHON_VER_DOTTED)\n\
AC_SUBST(MOZ_PYTHON_DEBUG_SUFFIX)\n\
AC_SUBST(MOZ_PYTHON_DLL_SUFFIX)\n\
\n\
dnl ========================================================\n\
dnl disable profile sharing\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(profilesharing,\n\
[  --disable-profilesharing           Disable profile sharing],\n\
    MOZ_PROFILESHARING=,\n\
    MOZ_PROFILESHARING=1 )\n\
if test \"$MOZ_PROFILESHARING\"; then\n\
  MOZ_IPCD=1\n\
  AC_DEFINE(MOZ_PROFILESHARING)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl enable ipc/ipcd\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(ipcd,\n\
[  --enable-ipcd                      Enable IPC daemon],\n\
    MOZ_IPCD=1,\n\
    MOZ_IPCD= )\n\
\n\
dnl ========================================================\n\
dnl disable profile locking\n\
dnl   do no use this in applications that can have more than\n\
dnl   one process accessing the profile directory.\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(profilelocking,\n\
[  --disable-profilelocking           Disable profile locking],\n\
    MOZ_PROFILELOCKING=,\n\
    MOZ_PROFILELOCKING=1 )\n\
if test \"$MOZ_PROFILELOCKING\"; then\n\
  AC_DEFINE(MOZ_PROFILELOCKING)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl disable rdf services\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(rdf,\n\
[  --disable-rdf           Disable RDF],\n\
    MOZ_RDF= )\n\
if test \"$MOZ_RDF\"; then\n\
  AC_DEFINE(MOZ_RDF)\n\
fi\n\
\n\
AC_SUBST(MOZ_RDF)\n\
\n\
dnl ========================================================\n\
dnl necko configuration options\n\
dnl ========================================================\n\
\n\
dnl\n\
dnl option to disable various necko protocols\n\
dnl\n\
MOZ_ARG_ENABLE_STRING(necko-protocols,\n\
[  --enable-necko-protocols[={http,ftp,default,all,none}]\n\
                          Enable/disable specific protocol handlers],\n\
[ for option in `echo $enableval | sed \'s/,/ /g\'`; do\n\
    if test \"$option\" = \"yes\" || test \"$option\" = \"all\"; then\n\
        NECKO_PROTOCOLS=\"$NECKO_PROTOCOLS $NECKO_PROTOCOLS_DEFAULT\"\n\
    elif test \"$option\" = \"no\" || test \"$option\" = \"none\"; then\n\
        NECKO_PROTOCOLS=\"\"\n\
    elif test \"$option\" = \"default\"; then\n\
        NECKO_PROTOCOLS=\"$NECKO_PROTOCOLS $NECKO_PROTOCOLS_DEFAULT\"\n\
    elif test `echo \"$option\" | grep -c \\^-` != 0; then\n\
        option=`echo $option | sed \'s/^-//\'`\n\
        NECKO_PROTOCOLS=`echo \"$NECKO_PROTOCOLS\" | sed \"s/ ${option}//\"`\n\
    else\n\
        NECKO_PROTOCOLS=\"$NECKO_PROTOCOLS $option\"\n\
    fi\n\
done],\n\
    NECKO_PROTOCOLS=\"$NECKO_PROTOCOLS_DEFAULT\")\n\
dnl Remove dupes\n\
NECKO_PROTOCOLS=`${PERL} ${srcdir}/build/unix/uniq.pl ${NECKO_PROTOCOLS}`\n\
AC_SUBST(NECKO_PROTOCOLS)\n\
for p in $NECKO_PROTOCOLS; do\n\
    AC_DEFINE_UNQUOTED(NECKO_PROTOCOL_$p)\n\
done\n\
\n\
dnl\n\
dnl option to disable necko\'s disk cache\n\
dnl\n\
MOZ_ARG_DISABLE_BOOL(necko-disk-cache,\n\
[  --disable-necko-disk-cache\n\
                          Disable necko disk cache],\n\
    NECKO_DISK_CACHE=,\n\
    NECKO_DISK_CACHE=1)\n\
AC_SUBST(NECKO_DISK_CACHE)\n\
if test \"$NECKO_DISK_CACHE\"; then\n\
    AC_DEFINE(NECKO_DISK_CACHE)\n\
fi\n\
\n\
dnl\n\
dnl option to minimize size of necko\'s i/o buffers\n\
dnl\n\
MOZ_ARG_ENABLE_BOOL(necko-small-buffers,\n\
[  --enable-necko-small-buffers\n\
                          Minimize size of necko\'s i/o buffers],\n\
    NECKO_SMALL_BUFFERS=1,\n\
    NECKO_SMALL_BUFFERS=)\n\
AC_SUBST(NECKO_SMALL_BUFFERS)\n\
if test \"$NECKO_SMALL_BUFFERS\"; then\n\
    AC_DEFINE(NECKO_SMALL_BUFFERS)\n\
fi \n\
\n\
dnl\n\
dnl option to disable cookies\n\
dnl\n\
MOZ_ARG_DISABLE_BOOL(cookies,\n\
[  --disable-cookies       Disable cookie support],\n\
    NECKO_COOKIES=,\n\
    NECKO_COOKIES=1)\n\
AC_SUBST(NECKO_COOKIES)\n\
if test \"$NECKO_COOKIES\"; then\n\
    AC_DEFINE(NECKO_COOKIES)\n\
fi\n\
\n\
dnl NECKO_ configuration options are not global\n\
_NON_GLOBAL_ACDEFINES=\"$_NON_GLOBAL_ACDEFINES NECKO_\"\n\
\n\
dnl ========================================================\n\
dnl string api compatibility\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(v1-string-abi,\n\
[  --disable-v1-string-abi   Disable binary compatibility layer for strings],\n\
    MOZ_V1_STRING_ABI=,\n\
    MOZ_V1_STRING_ABI=1)\n\
AC_SUBST(MOZ_V1_STRING_ABI)\n\
if test \"$MOZ_V1_STRING_ABI\"; then\n\
    AC_DEFINE(MOZ_V1_STRING_ABI)\n\
fi\n\
\n\
dnl Only build Mork if it\'s required\n\
AC_SUBST(MOZ_MORK)\n\
if test \"$MOZ_MORK\"; then\n\
  AC_DEFINE(MOZ_MORK)\n\
fi\n\
\n\
dnl Build the lightweight Mork reader if required\n\
AC_SUBST(MOZ_MORKREADER)\n\
if test \"$MOZ_MORKREADER\"; then\n\
  AC_DEFINE(MOZ_MORKREADER)\n\
fi\n\
\n\
dnl ========================================================\n\
if test \"$MOZ_DEBUG\" || test \"$NS_TRACE_MALLOC\"; then\n\
    MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\n\
fi\n\
\n\
if test \"$MOZ_LDAP_XPCOM\"; then\n\
    LDAP_CFLAGS=\'-I${DIST}/public/ldap\'\n\
    if test \"$OS_ARCH\" = \"WINNT\"; then\n\
        if test -n \"$GNU_CC\"; then\n\
            LDAP_LIBS=\'-L$(DIST)/lib -lnsldap32v50 -lnsldappr32v50\'\n\
        else\n\
            LDAP_LIBS=\'$(DIST)/lib/$(LIB_PREFIX)nsldap32v50.${IMPORT_LIB_SUFFIX} $(DIST)/lib/$(LIB_PREFIX)nsldappr32v50.${IMPORT_LIB_SUFFIX}\'\n\
        fi\n\
    elif test \"$VACPP\"; then\n\
            LDAP_LIBS=\'$(DIST)/lib/$(LIB_PREFIX)ldap50.${IMPORT_LIB_SUFFIX} $(DIST)/lib/$(LIB_PREFIX)prldap50.${IMPORT_LIB_SUFFIX}\'\n\
    else\n\
        LDAP_LIBS=\'-L${DIST}/bin -L${DIST}/lib -lldap50 -llber50 -lprldap50\'\n\
    fi\n\
fi\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test \"$SUNCTL\"; then\n\
    dnl older versions of glib do not seem to have gmodule which ctl needs\n\
    _SAVE_CFLAGS=$CFLAGS\n\
    CFLAGS=\"$CFLAGS $GLIB_CFLAGS\"\n\
    AC_LANG_SAVE\n\
    AC_LANG_C\n\
    AC_TRY_COMPILE([#include <gmodule.h>],\n\
        [ int x = 1; x++; ],,\n\
        [AC_MSG_ERROR([Cannot build ctl without gmodule support in glib.])])\n\
    AC_LANG_RESTORE\n\
    CFLAGS=$_SAVE_CFLAGS\n\
    AC_DEFINE(SUNCTL)\n\
fi\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Maintainer debug option (no --enable equivalent)\n\
dnl =\n\
dnl ========================================================\n\
\n\
AC_SUBST(AR)\n\
AC_SUBST(AR_FLAGS)\n\
AC_SUBST(AR_LIST)\n\
AC_SUBST(AR_EXTRACT)\n\
AC_SUBST(AR_DELETE)\n\
AC_SUBST(AS)\n\
AC_SUBST(ASFLAGS)\n\
AC_SUBST(AS_DASH_C_FLAG)\n\
AC_SUBST(LD)\n\
AC_SUBST(RC)\n\
AC_SUBST(RCFLAGS)\n\
AC_SUBST(WINDRES)\n\
AC_SUBST(USE_SHORT_LIBNAME)\n\
AC_SUBST(IMPLIB)\n\
AC_SUBST(FILTER)\n\
AC_SUBST(BIN_FLAGS)\n\
AC_SUBST(NS_USE_NATIVE)\n\
AC_SUBST(MOZ_WIDGET_TOOLKIT)\n\
AC_SUBST(MOZ_GFX_TOOLKIT)\n\
AC_SUBST(MOZ_UPDATE_XTERM)\n\
AC_SUBST(MINIMO)\n\
AC_SUBST(MOZ_AUTH_EXTENSION)\n\
AC_SUBST(MOZ_MATHML)\n\
AC_SUBST(MOZ_PERMISSIONS)\n\
AC_SUBST(MOZ_XTF)\n\
AC_SUBST(MOZ_XMLEXTRAS)\n\
AC_SUBST(MOZ_NO_INSPECTOR_APIS)\n\
AC_SUBST(MOZ_WEBSERVICES)\n\
AC_SUBST(MOZ_PREF_EXTENSIONS)\n\
AC_SUBST(MOZ_SVG)\n\
AC_SUBST(MOZ_SVG_FOREIGNOBJECT)\n\
AC_SUBST(MOZ_XSLT_STANDALONE)\n\
AC_SUBST(MOZ_JS_LIBS)\n\
AC_SUBST(MOZ_PSM)\n\
AC_SUBST(MOZ_DEBUG)\n\
AC_SUBST(MOZ_DEBUG_MODULES)\n\
AC_SUBST(MOZ_PROFILE_MODULES)\n\
AC_SUBST(MOZ_DEBUG_ENABLE_DEFS)\n\
AC_SUBST(MOZ_DEBUG_DISABLE_DEFS)\n\
AC_SUBST(MOZ_DEBUG_FLAGS)\n\
AC_SUBST(MOZ_DEBUG_LDFLAGS)\n\
AC_SUBST(MOZ_DBGRINFO_MODULES)\n\
AC_SUBST(MOZ_EXTENSIONS)\n\
AC_SUBST(MOZ_IMG_DECODERS)\n\
AC_SUBST(MOZ_IMG_ENCODERS)\n\
AC_SUBST(MOZ_JSDEBUGGER)\n\
AC_SUBST(MOZ_OJI)\n\
AC_SUBST(MOZ_NO_XPCOM_OBSOLETE)\n\
AC_SUBST(MOZ_PLUGINS)\n\
AC_SUBST(ENABLE_EAZEL_PROFILER)\n\
AC_SUBST(EAZEL_PROFILER_CFLAGS)\n\
AC_SUBST(EAZEL_PROFILER_LIBS)\n\
AC_SUBST(MOZ_PERF_METRICS)\n\
AC_SUBST(GC_LEAK_DETECTOR)\n\
AC_SUBST(MOZ_LOG_REFCNT)\n\
AC_SUBST(MOZ_LEAKY)\n\
AC_SUBST(MOZ_JPROF)\n\
AC_SUBST(MOZ_XPCTOOLS)\n\
AC_SUBST(MOZ_JSLOADER)\n\
AC_SUBST(MOZ_USE_NATIVE_UCONV)\n\
AC_SUBST(MOZ_INSURE)\n\
AC_SUBST(MOZ_INSURE_DIRS)\n\
AC_SUBST(MOZ_INSURE_EXCLUDE_DIRS)\n\
AC_SUBST(MOZ_QUANTIFY)\n\
AC_SUBST(MOZ_INSURIFYING)\n\
AC_SUBST(MOZ_LDAP_XPCOM)\n\
AC_SUBST(MOZ_LDAP_XPCOM_EXPERIMENTAL)\n\
AC_SUBST(LDAP_CFLAGS)\n\
AC_SUBST(LDAP_LIBS)\n\
AC_SUBST(LIBICONV)\n\
AC_SUBST(MOZ_PLACES)\n\
AC_SUBST(MOZ_PLACES_BOOKMARKS)\n\
AC_SUBST(MOZ_STORAGE)\n\
AC_SUBST(MOZ_FEEDS)\n\
AC_SUBST(NS_PRINTING)\n\
\n\
AC_SUBST(MOZ_JAVAXPCOM)\n\
AC_SUBST(JAVA_INCLUDE_PATH)\n\
AC_SUBST(JAVA)\n\
AC_SUBST(JAVAC)\n\
AC_SUBST(JAR)\n\
\n\
AC_SUBST(MOZ_PROFILESHARING)\n\
AC_SUBST(MOZ_PROFILELOCKING)\n\
\n\
AC_SUBST(MOZ_IPCD)\n\
\n\
AC_SUBST(HAVE_XIE)\n\
AC_SUBST(MOZ_XIE_LIBS)\n\
AC_SUBST(MOZ_XPRINT_CFLAGS)\n\
AC_SUBST(MOZ_XPRINT_LDFLAGS)\n\
AC_SUBST(MOZ_ENABLE_XPRINT)\n\
AC_SUBST(MOZ_ENABLE_POSTSCRIPT)\n\
AC_SUBST(MOZ_XINERAMA_LIBS)\n\
AC_SUBST(MOZ_ENABLE_XINERAMA)\n\
\n\
AC_SUBST(XPCOM_USE_LEA)\n\
AC_SUBST(BUILD_STATIC_LIBS)\n\
AC_SUBST(MOZ_ENABLE_LIBXUL)\n\
AC_SUBST(ENABLE_TESTS)\n\
AC_SUBST(IBMBIDI)\n\
AC_SUBST(SUNCTL)\n\
AC_SUBST(MOZ_UNIVERSALCHARDET)\n\
AC_SUBST(ACCESSIBILITY)\n\
AC_SUBST(MOZ_XPINSTALL)\n\
AC_SUBST(MOZ_VIEW_SOURCE)\n\
AC_SUBST(MOZ_SINGLE_PROFILE)\n\
AC_SUBST(MOZ_SPELLCHECK)\n\
AC_SUBST(MOZ_XPFE_COMPONENTS)\n\
AC_SUBST(MOZ_USER_DIR)\n\
AC_SUBST(MOZ_AIRBAG)\n\
AC_SUBST(MOZ_MOCHITEST)\n\
\n\
AC_SUBST(ENABLE_STRIP)\n\
AC_SUBST(USE_ELF_DYNSTR_GC)\n\
AC_SUBST(USE_PREBINDING)\n\
AC_SUBST(INCREMENTAL_LINKER)\n\
AC_SUBST(MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS)\n\
AC_SUBST(MOZ_COMPONENT_NSPR_LIBS)\n\
AC_SUBST(MOZ_XPCOM_OBSOLETE_LIBS)\n\
\n\
AC_SUBST(MOZ_FIX_LINK_PATHS)\n\
AC_SUBST(XPCOM_LIBS)\n\
AC_SUBST(XPCOM_FROZEN_LDOPTS)\n\
AC_SUBST(XPCOM_GLUE_LDOPTS)\n\
AC_SUBST(XPCOM_STANDALONE_GLUE_LDOPTS)\n\
\n\
AC_SUBST(USE_DEPENDENT_LIBS)\n\
\n\
AC_SUBST(MOZ_BUILD_ROOT)\n\
AC_SUBST(MOZ_OS2_TOOLS)\n\
AC_SUBST(MOZ_OS2_USE_DECLSPEC)\n\
\n\
AC_SUBST(MOZ_POST_DSO_LIB_COMMAND)\n\
AC_SUBST(MOZ_POST_PROGRAM_COMMAND)\n\
AC_SUBST(MOZ_TIMELINE)\n\
AC_SUBST(WINCE)\n\
AC_SUBST(TARGET_DEVICE)\n\
\n\
AC_SUBST(MOZ_APP_NAME)\n\
AC_SUBST(MOZ_APP_DISPLAYNAME)\n\
AC_SUBST(MOZ_APP_VERSION)\n\
AC_SUBST(FIREFOX_VERSION)\n\
AC_SUBST(THUNDERBIRD_VERSION)\n\
AC_SUBST(SUNBIRD_VERSION)\n\
AC_SUBST(SEAMONKEY_VERSION)\n\
\n\
AC_SUBST(MOZ_PKG_SPECIAL)\n\
\n\
AC_SUBST(MOZILLA_OFFICIAL)\n\
AC_SUBST(BUILD_OFFICIAL)\n\
AC_SUBST(MOZ_MILESTONE_RELEASE)\n\
\n\
dnl win32 options\n\
AC_SUBST(MOZ_PROFILE)\n\
AC_SUBST(MOZ_DEBUG_SYMBOLS)\n\
AC_SUBST(MOZ_MAPINFO)\n\
AC_SUBST(MOZ_BROWSE_INFO)\n\
AC_SUBST(MOZ_TOOLS_DIR)\n\
AC_SUBST(CYGWIN_WRAPPER)\n\
AC_SUBST(AS_PERL)\n\
AC_SUBST(WIN32_REDIST_DIR)\n\
AC_SUBST(PYTHON)\n\
\n\
dnl Echo the CFLAGS to remove extra whitespace.\n\
CFLAGS=`echo \\\n\
	$_WARNINGS_CFLAGS \\\n\
	$CFLAGS`\n\
\n\
CXXFLAGS=`echo \\\n\
	$_MOZ_RTTI_FLAGS \\\n\
	$_MOZ_EXCEPTIONS_FLAGS \\\n\
	$_WARNINGS_CXXFLAGS \\\n\
	$CXXFLAGS`\n\
\n\
COMPILE_CFLAGS=`echo \\\n\
    $_DEFINES_CFLAGS \\\n\
	$_DEPEND_CFLAGS \\\n\
    $COMPILE_CFLAGS`\n\
\n\
COMPILE_CXXFLAGS=`echo \\\n\
    $_DEFINES_CXXFLAGS \\\n\
	$_DEPEND_CFLAGS \\\n\
    $COMPILE_CXXFLAGS`\n\
\n\
AC_SUBST(SYSTEM_MAKEDEPEND)\n\
AC_SUBST(SYSTEM_JPEG)\n\
AC_SUBST(SYSTEM_PNG)\n\
AC_SUBST(SYSTEM_ZLIB)\n\
\n\
AC_SUBST(JPEG_CFLAGS)\n\
AC_SUBST(JPEG_LIBS)\n\
AC_SUBST(ZLIB_CFLAGS)\n\
AC_SUBST(ZLIB_LIBS)\n\
AC_SUBST(PNG_CFLAGS)\n\
AC_SUBST(PNG_LIBS)\n\
\n\
AC_SUBST(MOZ_JPEG_CFLAGS)\n\
AC_SUBST(MOZ_JPEG_LIBS)\n\
AC_SUBST(MOZ_ZLIB_CFLAGS)\n\
AC_SUBST(MOZ_ZLIB_LIBS)\n\
AC_SUBST(MOZ_PNG_CFLAGS)\n\
AC_SUBST(MOZ_PNG_LIBS)\n\
\n\
AC_SUBST(NSPR_CFLAGS)\n\
AC_SUBST(NSPR_LIBS)\n\
AC_SUBST(MOZ_NATIVE_NSPR)\n\
\n\
AC_SUBST(NSS_CFLAGS)\n\
AC_SUBST(NSS_LIBS)\n\
AC_SUBST(NSS_DEP_LIBS)\n\
AC_SUBST(MOZ_NATIVE_NSS)\n\
\n\
AC_SUBST(CFLAGS)\n\
AC_SUBST(CXXFLAGS)\n\
AC_SUBST(CPPFLAGS)\n\
AC_SUBST(COMPILE_CFLAGS)\n\
AC_SUBST(COMPILE_CXXFLAGS)\n\
AC_SUBST(LDFLAGS)\n\
AC_SUBST(LIBS)\n\
AC_SUBST(CROSS_COMPILE)\n\
\n\
AC_SUBST(HOST_CC)\n\
AC_SUBST(HOST_CXX)\n\
AC_SUBST(HOST_CFLAGS)\n\
AC_SUBST(HOST_CXXFLAGS)\n\
AC_SUBST(HOST_OPTIMIZE_FLAGS)\n\
AC_SUBST(HOST_AR)\n\
AC_SUBST(HOST_AR_FLAGS)\n\
AC_SUBST(HOST_LD)\n\
AC_SUBST(HOST_RANLIB)\n\
AC_SUBST(HOST_NSPR_MDCPUCFG)\n\
AC_SUBST(HOST_BIN_SUFFIX)\n\
AC_SUBST(HOST_OS_ARCH)\n\
\n\
AC_SUBST(TARGET_CPU)\n\
AC_SUBST(TARGET_VENDOR)\n\
AC_SUBST(TARGET_OS)\n\
AC_SUBST(TARGET_NSPR_MDCPUCFG)\n\
AC_SUBST(TARGET_MD_ARCH)\n\
AC_SUBST(TARGET_XPCOM_ABI)\n\
AC_SUBST(OS_TARGET)\n\
AC_SUBST(OS_ARCH)\n\
AC_SUBST(OS_RELEASE)\n\
AC_SUBST(OS_TEST)\n\
\n\
AC_SUBST(MOZ_DISABLE_JAR_PACKAGING)\n\
AC_SUBST(MOZ_CHROME_FILE_FORMAT)\n\
\n\
AC_SUBST(WRAP_MALLOC_CFLAGS)\n\
AC_SUBST(WRAP_MALLOC_LIB)\n\
AC_SUBST(MKSHLIB)\n\
AC_SUBST(MKCSHLIB)\n\
AC_SUBST(MKSHLIB_FORCE_ALL)\n\
AC_SUBST(MKSHLIB_UNFORCE_ALL)\n\
AC_SUBST(DSO_CFLAGS)\n\
AC_SUBST(DSO_PIC_CFLAGS)\n\
AC_SUBST(DSO_LDOPTS)\n\
AC_SUBST(LIB_PREFIX)\n\
AC_SUBST(DLL_PREFIX)\n\
AC_SUBST(DLL_SUFFIX)\n\
AC_DEFINE_UNQUOTED(MOZ_DLL_SUFFIX, \"$DLL_SUFFIX\")\n\
AC_SUBST(LIB_SUFFIX)\n\
AC_SUBST(OBJ_SUFFIX)\n\
AC_SUBST(BIN_SUFFIX)\n\
AC_SUBST(ASM_SUFFIX)\n\
AC_SUBST(IMPORT_LIB_SUFFIX)\n\
AC_SUBST(USE_N32)\n\
AC_SUBST(CC_VERSION)\n\
AC_SUBST(CXX_VERSION)\n\
AC_SUBST(MSMANIFEST_TOOL)\n\
\n\
if test \"$USING_HCC\"; then\n\
   CC=\'${topsrcdir}/build/hcc\'\n\
   CC=\"$CC \'$_OLDCC\'\"\n\
   CXX=\'${topsrcdir}/build/hcpp\'\n\
   CXX=\"$CXX \'$_OLDCXX\'\"\n\
   AC_SUBST(CC)\n\
   AC_SUBST(CXX)\n\
fi\n\
\n\
dnl Check for missing components\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test \"$MOZ_X11\"; then\n\
    dnl ====================================================\n\
    dnl = Check if X headers exist\n\
    dnl ====================================================\n\
    _SAVE_CFLAGS=$CFLAGS\n\
    CFLAGS=\"$CFLAGS $XCFLAGS\"\n\
    AC_TRY_COMPILE([\n\
        #include <stdio.h>\n\
        #include <X11/Xlib.h>\n\
        #include <X11/Intrinsic.h>\n\
    ],\n\
    [\n\
        Display *dpy = 0;\n\
        if ((dpy = XOpenDisplay(NULL)) == NULL) {\n\
            fprintf(stderr, \": can\'t open %s\\n\", XDisplayName(NULL));\n\
            exit(1);\n\
        }\n\
    ], [], \n\
    [ AC_MSG_ERROR([Could not compile basic X program.]) ])\n\
    CFLAGS=\"$_SAVE_CFLAGS\"\n\
\n\
    if test ! -z \"$MISSING_X\"; then\n\
        AC_MSG_ERROR([ Could not find the following X libraries: $MISSING_X ]);\n\
    fi\n\
\n\
fi # MOZ_X11\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl Set various defines and substitutions\n\
dnl ========================================================\n\
\n\
if test \"$OS_ARCH\" = \"OS2\" -a \"$VACPP\" = \"yes\"; then\n\
      LIBS=\'so32dll.lib tcp32dll.lib\'\n\
elif test \"$OS_ARCH\" = \"BeOS\"; then\n\
  AC_DEFINE(XP_BEOS)\n\
  MOZ_MOVEMAIL=1\n\
elif test \"$OS_ARCH\" = \"Darwin\"; then\n\
  AC_DEFINE(XP_UNIX)\n\
  AC_DEFINE(UNIX_ASYNC_DNS)\n\
elif test \"$OS_ARCH\" = \"OpenVMS\"; then\n\
  AC_DEFINE(XP_UNIX)\n\
elif test \"$OS_ARCH\" != \"WINNT\" -a \"$OS_ARCH\" != \"OS2\" -a \"$OS_ARCH\" != \"WINCE\"; then\n\
  AC_DEFINE(XP_UNIX)\n\
  AC_DEFINE(UNIX_ASYNC_DNS)\n\
  MOZ_MOVEMAIL=1\n\
fi\n\
AC_SUBST(MOZ_MOVEMAIL)\n\
\n\
AC_DEFINE(JS_THREADSAFE)\n\
\n\
if test \"$MOZ_DEBUG\"; then\n\
    AC_DEFINE(MOZ_REFLOW_PERF)\n\
    AC_DEFINE(MOZ_REFLOW_PERF_DSP)\n\
fi\n\
\n\
if test \"$ACCESSIBILITY\" -a \"$MOZ_ENABLE_GTK2\" ; then\n\
    AC_DEFINE(MOZ_ACCESSIBILITY_ATK)\n\
    ATK_FULL_VERSION=`$PKG_CONFIG --modversion atk`\n\
    ATK_MAJOR_VERSION=`echo ${ATK_FULL_VERSION} | $AWK -F\\. \'{ print $1 }\'`\n\
    ATK_MINOR_VERSION=`echo ${ATK_FULL_VERSION} | $AWK -F\\. \'{ print $2 }\'`\n\
    ATK_REV_VERSION=`echo ${ATK_FULL_VERSION} | $AWK -F\\. \'{ print $3 }\'`\n\
    AC_DEFINE_UNQUOTED(ATK_MAJOR_VERSION, $ATK_MAJOR_VERSION)\n\
    AC_DEFINE_UNQUOTED(ATK_MINOR_VERSION, $ATK_MINOR_VERSION)\n\
    AC_DEFINE_UNQUOTED(ATK_REV_VERSION, $ATK_REV_VERSION)\n\
fi\n\
\n\
# Used for LD_LIBRARY_PATH of run_viewer target\n\
LIBS_PATH=\n\
for lib_arg in $NSPR_LIBS $TK_LIBS; do\n\
  case $lib_arg in\n\
    -L* ) LIBS_PATH=\"${LIBS_PATH:+$LIBS_PATH:}\"`expr $lib_arg : \"-L\\(.*\\)\"` ;;\n\
      * ) ;;\n\
  esac\n\
done\n\
AC_SUBST(LIBS_PATH)\n\
\n\
dnl ========================================================\n\
dnl Use cygwin wrapper for win32 builds, except MSYS/MinGW\n\
dnl ========================================================\n\
case \"$host_os\" in\n\
mingw*)\n\
    WIN_TOP_SRC=`cd $srcdir; pwd -W`\n\
    ;;\n\
cygwin*|msvc*|mks*)\n\
    HOST_CC=\"\\$(CYGWIN_WRAPPER) $HOST_CC\"\n\
    HOST_CXX=\"\\$(CYGWIN_WRAPPER) $HOST_CXX\"\n\
    CC=\"\\$(CYGWIN_WRAPPER) $CC\"\n\
    CXX=\"\\$(CYGWIN_WRAPPER) $CXX\"\n\
    CPP=\"\\$(CYGWIN_WRAPPER) $CPP\"\n\
    LD=\"\\$(CYGWIN_WRAPPER) $LD\"\n\
    AS=\"\\$(CYGWIN_WRAPPER) $AS\"\n\
    RC=\"\\$(CYGWIN_WRAPPER) $RC\"\n\
    MIDL=\"\\$(CYGWIN_WRAPPER) $MIDL\"\n\
    CYGDRIVE_MOUNT=`mount -p | awk \'{ if (/^\\//) { print $1; exit } }\'`\n\
    WIN_TOP_SRC=`cygpath -a -w $srcdir | sed -e \'s|\\\\\\\\|/|g\'`\n\
    ;;\n\
esac\n\
\n\
AC_SUBST(CYGDRIVE_MOUNT)\n\
AC_SUBST(WIN_TOP_SRC)\n\
\n\
AC_SUBST(MOZILLA_VERSION)\n\
\n\
. ${srcdir}/config/chrome-versions.sh\n\
AC_DEFINE_UNQUOTED(MOZILLA_LOCALE_VERSION,\"$MOZILLA_LOCALE_VERSION\")\n\
AC_DEFINE_UNQUOTED(MOZILLA_REGION_VERSION,\"$MOZILLA_REGION_VERSION\")\n\
AC_DEFINE_UNQUOTED(MOZILLA_SKIN_VERSION,\"$MOZILLA_SKIN_VERSION\")\n\
\n\
AC_SUBST(ac_configure_args)\n\
\n\
dnl Spit out some output\n\
dnl ========================================================\n\
\n\
dnl The following defines are used by xpcom\n\
_NON_GLOBAL_ACDEFINES=\"$_NON_GLOBAL_ACDEFINES\n\
CPP_THROW_NEW\n\
HAVE_CPP_2BYTE_WCHAR_T\n\
HAVE_CPP_ACCESS_CHANGING_USING\n\
HAVE_CPP_AMBIGUITY_RESOLVING_USING\n\
HAVE_CPP_BOOL\n\
HAVE_CPP_DYNAMIC_CAST_TO_VOID_PTR\n\
HAVE_CPP_EXPLICIT\n\
HAVE_CPP_MODERN_SPECIALIZE_TEMPLATE_SYNTAX\n\
HAVE_CPP_NAMESPACE_STD\n\
HAVE_CPP_NEW_CASTS\n\
HAVE_CPP_PARTIAL_SPECIALIZATION\n\
HAVE_CPP_TROUBLE_COMPARING_TO_ZERO\n\
HAVE_CPP_TYPENAME\n\
HAVE_CPP_UNAMBIGUOUS_STD_NOTEQUAL\n\
HAVE_STATVFS\n\
NEED_CPP_UNUSED_IMPLEMENTATIONS\n\
NEW_H\n\
HAVE_GETPAGESIZE\n\
HAVE_ICONV\n\
HAVE_ICONV_WITH_CONST_INPUT\n\
HAVE_MBRTOWC\n\
HAVE_SYS_MOUNT_H\n\
HAVE_SYS_VFS_H\n\
HAVE_WCRTOMB\n\
MOZ_V1_STRING_ABI\n\
\"\n\
\n\
AC_CONFIG_HEADER(\n\
gfx/gfx-config.h\n\
netwerk/necko-config.h\n\
xpcom/xpcom-config.h\n\
xpcom/xpcom-private.h\n\
)\n\
\n\
# Save the defines header file before autoconf removes it.\n\
# (Do not add AC_DEFINE calls after this line.)\n\
  _CONFIG_TMP=confdefs-tmp.h\n\
  _CONFIG_DEFS_H=mozilla-config.h\n\
\n\
  cat > $_CONFIG_TMP <<\\EOF\n\
/* List of defines generated by configure. Included with preprocessor flag,\n\
 * -include, to avoid long list of -D defines on the compile command-line.\n\
 * Do not edit.\n\
 */\n\
\n\
#ifndef _MOZILLA_CONFIG_H_\n\
#define _MOZILLA_CONFIG_H_\n\
EOF\n\
\n\
_EGREP_PATTERN=\'^#define (\'\n\
if test -n \"$_NON_GLOBAL_ACDEFINES\"; then\n\
    for f in $_NON_GLOBAL_ACDEFINES; do\n\
        _EGREP_PATTERN=\"${_EGREP_PATTERN}$f|\"\n\
    done\n\
fi\n\
_EGREP_PATTERN=\"${_EGREP_PATTERN}dummy_never_defined)\"\n\
 \n\
  sort confdefs.h | egrep -v \"$_EGREP_PATTERN\" >> $_CONFIG_TMP\n\
\n\
  cat >> $_CONFIG_TMP <<\\EOF\n\
\n\
#endif /* _MOZILLA_CONFIG_H_ */\n\
\n\
EOF\n\
\n\
  # Only write mozilla-config.h when something changes (or it doesn\'t exist)\n\
  if cmp -s $_CONFIG_TMP $_CONFIG_DEFS_H; then\n\
    rm $_CONFIG_TMP\n\
  else\n\
    AC_MSG_RESULT(\"creating $_CONFIG_DEFS_H\")\n\
    mv -f $_CONFIG_TMP $_CONFIG_DEFS_H\n\
\n\
    echo ==== $_CONFIG_DEFS_H =================================\n\
    cat $_CONFIG_DEFS_H\n\
  fi\n\
\n\
dnl Probably shouldn\'t call this manually but we always want the output of DEFS\n\
rm -f confdefs.h.save\n\
mv confdefs.h confdefs.h.save\n\
egrep -v \"$_EGREP_PATTERN\" confdefs.h.save > confdefs.h\n\
AC_OUTPUT_MAKE_DEFS()\n\
MOZ_DEFINES=$DEFS\n\
AC_SUBST(MOZ_DEFINES)\n\
rm -f confdefs.h\n\
mv confdefs.h.save confdefs.h\n\
\n\
dnl Load the list of Makefiles to generate.\n\
dnl   To add new Makefiles, edit allmakefiles.sh.\n\
dnl   allmakefiles.sh sets the variable, MAKEFILES.\n\
. ${srcdir}/allmakefiles.sh\n\
dnl \n\
dnl Run a perl script to quickly create the makefiles.\n\
dnl If it succeeds, it outputs a shell command to set CONFIG_FILES\n\
dnl   for the files it cannot handle correctly. This way, config.status\n\
dnl   will handle these files.\n\
dnl If it fails, nothing is set and config.status will run as usual.\n\
dnl\n\
dnl This does not change the $MAKEFILES variable.\n\
dnl\n\
dnl OpenVMS gets a line overflow on the long eval command, so use a temp file.\n\
dnl\n\
if test -z \"${AS_PERL}\"; then\n\
echo $MAKEFILES | ${PERL} $srcdir/build/autoconf/acoutput-fast.pl > conftest.sh\n\
else\n\
echo $MAKEFILES | ${PERL} $srcdir/build/autoconf/acoutput-fast.pl -nowrap --cygwin-srcdir=$srcdir > conftest.sh\n\
fi\n\
. ./conftest.sh\n\
rm conftest.sh\n\
\n\
echo $MAKEFILES > unallmakefiles\n\
\n\
AC_OUTPUT($MAKEFILES)\n\
\n\
dnl ========================================================\n\
dnl = Setup a nice relatively clean build environment for\n\
dnl = sub-configures.\n\
dnl ========================================================\n\
CC=\"$_SUBDIR_CC\" \n\
CXX=\"$_SUBDIR_CXX\" \n\
CFLAGS=\"$_SUBDIR_CFLAGS\" \n\
CPPFLAGS=\"$_SUBDIR_CPPFLAGS\"\n\
CXXFLAGS=\"$_SUBDIR_CXXFLAGS\"\n\
LDFLAGS=\"$_SUBDIR_LDFLAGS\"\n\
HOST_CC=\"$_SUBDIR_HOST_CC\" \n\
HOST_CFLAGS=\"$_SUBDIR_HOST_CFLAGS\"\n\
HOST_LDFLAGS=\"$_SUBDIR_HOST_LDFLAGS\"\n\
RC=\n\
\n\
unset MAKEFILES\n\
unset CONFIG_FILES\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test -z \"$MOZ_NATIVE_NSPR\" || test \"$MOZ_LDAP_XPCOM\"; then\n\
    ac_configure_args=\"$_SUBDIR_CONFIG_ARGS --with-dist-prefix=$MOZ_BUILD_ROOT/dist --with-mozilla\"\n\
    if test -z \"$MOZ_DEBUG\"; then\n\
        ac_configure_args=\"$ac_configure_args --disable-debug\"\n\
    fi\n\
    if test \"$MOZ_OPTIMIZE\" = \"1\"; then\n\
        ac_configure_args=\"$ac_configure_args --enable-optimize\"\n\
    fi\n\
    if test \"$OS_ARCH\" = \"WINNT\" && test \"$NS_TRACE_MALLOC\"; then\n\
       ac_configure_args=\"$ac_configure_args --enable-debug --disable-optimize\"\n\
    fi\n\
    if test -n \"$HAVE_64BIT_OS\"; then\n\
        ac_configure_args=\"$ac_configure_args --enable-64bit\"\n\
    fi\n\
    AC_OUTPUT_SUBDIRS(nsprpub)\n\
    ac_configure_args=\"$_SUBDIR_CONFIG_ARGS\"\n\
fi\n\
\n\
if test -z \"$MOZ_NATIVE_NSPR\"; then\n\
    # Hack to deal with the fact that we use NSPR_CFLAGS everywhere\n\
    AC_MSG_WARN([Recreating autoconf.mk with updated nspr-config output])\n\
    if test ! \"$VACPP\" && test \"$OS_ARCH\" != \"WINNT\" && test \"$OS_ARCH\" != \"WINCE\"; then\n\
       _libs=`./nsprpub/config/nspr-config --prefix=$\\(LIBXUL_DIST\\) --exec-prefix=$\\(DIST\\) --libdir=$\\(LIBXUL_DIST\\)/lib --libs`\n\
       $PERL -pi.bak -e \"s \'^NSPR_LIBS\\\\s*=.*\'NSPR_LIBS = $_libs\'\" config/autoconf.mk\n\
    fi\n\
    if test \"$OS_ARCH\" != \"WINNT\" && test \"$OS_ARCH\" != \"WINCE\" ; then\n\
       _cflags=`./nsprpub/config/nspr-config --prefix=$\\(LIBXUL_DIST\\) --exec-prefix=$\\(DIST\\) --includedir=$\\(LIBXUL_DIST\\)/include/nspr --cflags`\n\
       $PERL -pi.bak -e \"s \'^NSPR_CFLAGS\\\\s*=.*\'NSPR_CFLAGS = $_cflags\'\" config/autoconf.mk\n\
    fi\n\
    rm -f config/autoconf.mk.bak\n\
fi\n\
\n\
# if we\'re building the LDAP XPCOM component, we need to build \n\
# the c-sdk first.  \n\
#\n\
if test \"$MOZ_LDAP_XPCOM\"; then\n\
\n\
    # these subdirs may not yet have been created in the build tree.\n\
    # don\'t use the \"-p\" switch to mkdir, since not all platforms have it\n\
    #\n\
    if test ! -d \"directory\"; then\n\
        mkdir \"directory\"\n\
    fi\n\
    if test ! -d \"directory/c-sdk\"; then\n\
        mkdir \"directory/c-sdk\"    \n\
    fi\n\
    if test ! -d \"directory/c-sdk/ldap\"; then\n\
        mkdir \"directory/c-sdk/ldap\"    \n\
    fi\n\
\n\
    ac_configure_args=\"$_SUBDIR_CONFIG_ARGS --prefix=$MOZ_BUILD_ROOT/dist --with-dist-prefix=$MOZ_BUILD_ROOT/dist --without-nss --with-mozilla\"\n\
    if test -z \"$MOZ_DEBUG\"; then\n\
        ac_configure_args=\"$ac_configure_args --disable-debug\"\n\
    fi\n\
    if test \"$MOZ_OPTIMIZE\" = \"1\"; then\n\
        ac_configure_args=\"$ac_configure_args --enable-optimize\"\n\
    fi\n\
    if test -n \"$HAVE_64BIT_OS\"; then\n\
        ac_configure_args=\"$ac_configure_args --enable-64bit\"\n\
    fi\n\
    AC_OUTPUT_SUBDIRS(directory/c-sdk)\n\
    ac_configure_args=\"$_SUBDIR_CONFIG_ARGS\"\n\
fi\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
\n\
"

#define MOZ_AC_V2 "dnl -*- Mode: Autoconf; tab-width: 4; indent-tabs-mode: nil; -*-\n\
dnl vi: set tabstop=4 shiftwidth=4 expandtab:\n\
dnl ***** BEGIN LICENSE BLOCK *****\n\
dnl Version: MPL 1.1/GPL 2.0/LGPL 2.1\n\
dnl\n\
dnl The contents of this file are subject to the Mozilla Public License Version\n\
dnl 1.1 (the \"License\"); you may not use this file except in compliance with\n\
dnl the License. You may obtain a copy of the License at\n\
dnl http://www.mozilla.org/MPL/\n\
dnl\n\
dnl Software distributed under the License is distributed on an \"AS IS\" basis,\n\
dnl WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License\n\
dnl for the specific language governing rights and limitations under the\n\
dnl License.\n\
dnl\n\
dnl The Original Code is this file as it was released upon August 6, 1998.\n\
dnl\n\
dnl The Initial Developer of the Original Code is\n\
dnl Christopher Seawood.\n\
dnl Portions created by the Initial Developer are Copyright (C) 1998-2001\n\
dnl the Initial Developer. All Rights Reserved.\n\
dnl\n\
dnl Contributor(s):\n\
dnl   Jamie Zawinski <jwz@jwz.org>\n\
dnl   gettimeofday args check\n\
dnl   Christopher Blizzard <blizzard@appliedtheory.com>\n\
dnl   gnomefe update & enable-pthreads\n\
dnl   Ramiro Estrugo <ramiro@netscape.com>\n\
dnl   X11 makedepend support\n\
dnl   Insure support.\n\
dnl   Henry Sobotka <sobotka@axess.com>\n\
dnl   OS/2 support\n\
dnl   Dan Mosedale <dmose@mozilla.org>\n\
dnl   LDAP support\n\
dnl   Seth Spitzer <sspitzer@netscape.com>\n\
dnl   xpctools support\n\
dnl   Benjamin Smedberg <benjamin@smedbergs.us>\n\
dnl   Howard Chu <hyc@symas.com>\n\
dnl   MSYS support\n\
dnl   Mark Mentovai <mark@moxienet.com>:\n\
dnl   Mac OS X 10.4 support\n\
dnl   Giorgio Maone <g.maone@informaction.com>\n\
dnl   MSVC l10n compatible version check\n\
dnl\n\
dnl Alternatively, the contents of this file may be used under the terms of\n\
dnl either the GNU General Public License Version 2 or later (the \"GPL\"), or\n\
dnl the GNU Lesser General Public License Version 2.1 or later (the \"LGPL\"),\n\
dnl in which case the provisions of the GPL or the LGPL are applicable instead\n\
dnl of those above. If you wish to allow use of your version of this file only\n\
dnl under the terms of either the GPL or the LGPL, and not to allow others to\n\
dnl use your version of this file under the terms of the MPL, indicate your\n\
dnl decision by deleting the provisions above and replace them with the notice\n\
dnl and other provisions required by the GPL or the LGPL. If you do not delete\n\
dnl the provisions above, a recipient may use your version of this file under\n\
dnl the terms of any one of the MPL, the GPL or the LGPL.\n\
dnl\n\
dnl ***** END LICENSE BLOCK *****\n\
\n\
dnl Process this file with autoconf to produce a configure script.\n\
dnl ========================================================\n\
\n\
AC_PREREQ(2.13)\n\
AC_INIT(config/config.mk)\n\
AC_CONFIG_AUX_DIR(${srcdir}/build/autoconf)\n\
AC_CANONICAL_SYSTEM\n\
TARGET_CPU=\"${target_cpu}\"\n\
TARGET_VENDOR=\"${target_vendor}\"\n\
TARGET_OS=\"${target_os}\"\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Don\'t change the following two lines.  Doing so breaks:\n\
dnl =\n\
dnl = CFLAGS=\"-foo\" ./configure\n\
dnl =\n\
dnl ========================================================\n\
CFLAGS=\"${CFLAGS=}\"\n\
CPPFLAGS=\"${CPPFLAGS=}\"\n\
CXXFLAGS=\"${CXXFLAGS=}\"\n\
LDFLAGS=\"${LDFLAGS=}\"\n\
HOST_CFLAGS=\"${HOST_CFLAGS=}\"\n\
HOST_CXXFLAGS=\"${HOST_CXXFLAGS=}\"\n\
HOST_LDFLAGS=\"${HOST_LDFLAGS=}\"\n\
\n\
dnl ========================================================\n\
dnl = Preserve certain environment flags passed to configure\n\
dnl = We want sub projects to receive the same flags\n\
dnl = untainted by this configure script\n\
dnl ========================================================\n\
_SUBDIR_CC=\"$CC\"\n\
_SUBDIR_CXX=\"$CXX\"\n\
_SUBDIR_CFLAGS=\"$CFLAGS\"\n\
_SUBDIR_CPPFLAGS=\"$CPPFLAGS\"\n\
_SUBDIR_CXXFLAGS=\"$CXXFLAGS\"\n\
_SUBDIR_LDFLAGS=\"$LDFLAGS\"\n\
_SUBDIR_HOST_CC=\"$HOST_CC\"\n\
_SUBDIR_HOST_CFLAGS=\"$HOST_CFLAGS\"\n\
_SUBDIR_HOST_CXXFLAGS=\"$HOST_CXXFLAGS\"\n\
_SUBDIR_HOST_LDFLAGS=\"$HOST_LDFLAGS\"\n\
_SUBDIR_CONFIG_ARGS=\"$ac_configure_args\"\n\
\n\
dnl Set the version number of the libs included with mozilla\n\
dnl ========================================================\n\
MOZJPEG=62\n\
MOZPNG=10207\n\
MOZZLIB=0x1230\n\
NSPR_VERSION=4\n\
NSS_VERSION=3\n\
\n\
dnl Set the minimum version of toolkit libs used by mozilla\n\
dnl ========================================================\n\
GLIB_VERSION=1.2.0\n\
GTK_VERSION=1.2.0\n\
LIBIDL_VERSION=0.6.3\n\
PERL_VERSION=5.006\n\
QT_VERSION=3.2.0\n\
QT_VERSION_NUM=320\n\
LIBART_VERSION=2.3.4\n\
CAIRO_VERSION=0.9.1\n\
GLITZ_VERSION=0.4.0\n\
GTK2_VERSION=1.3.7\n\
MAKE_VERSION=3.78\n\
WINDRES_VERSION=2.14.90\n\
W32API_VERSION=3.8\n\
GNOMEVFS_VERSION=2.0\n\
GNOMEUI_VERSION=2.2.0\n\
GCONF_VERSION=1.2.1\n\
LIBGNOME_VERSION=2.0\n\
STARTUP_NOTIFICATION_VERSION=0.8\n\
\n\
MSMANIFEST_TOOL=\n\
\n\
dnl Set various checks\n\
dnl ========================================================\n\
MISSING_X=\n\
AC_PROG_AWK\n\
\n\
dnl Initialize the Pthread test variables early so they can be\n\
dnl  overridden by each platform.\n\
dnl ========================================================\n\
USE_PTHREADS=\n\
_PTHREAD_LDFLAGS=\"\"\n\
\n\
dnl Do not allow a separate objdir build if a srcdir build exists.\n\
dnl ==============================================================\n\
_topsrcdir=`cd \\`dirname $0\\`; pwd`\n\
_objdir=`pwd`\n\
if test \"$_topsrcdir\" != \"$_objdir\"\n\
then\n\
  # Check for a couple representative files in the source tree\n\
  _conflict_files=\n\
  for file in $_topsrcdir/Makefile $_topsrcdir/config/autoconf.mk; do\n\
    if test -f $file; then\n\
      _conflict_files=\"$_conflict_files $file\"\n\
    fi\n\
  done\n\
  if test \"$_conflict_files\"; then\n\
    echo \"***\"\n\
    echo \"*   Your source tree contains these files:\"\n\
    for file in $_conflict_files; do\n\
      echo \"*         $file\"\n\
    done\n\
    cat 1>&2 <<-EOF\n\
	*   This indicates that you previously built in the source tree.\n\
	*   A source tree build can confuse the separate objdir build.\n\
	*\n\
	*   To clean up the source tree:\n\
	*     1. cd $_topsrcdir\n\
	*     2. gmake distclean\n\
	***\n\
	EOF\n\
    exit 1\n\
    break\n\
  fi\n\
fi\n\
MOZ_BUILD_ROOT=`pwd`\n\
\n\
dnl Default to MSVC for win32\n\
dnl ==============================================================\n\
if test -z \"$CROSS_COMPILE\"; then\n\
case \"$target\" in\n\
*-cygwin*|*-mingw*|*-msvc*|*-mks*)\n\
    MAKE_VERSION=3.79\n\
    if test -z \"$CC\"; then CC=cl; fi\n\
    if test -z \"$CXX\"; then CXX=cl; fi\n\
    if test -z \"$CPP\"; then CPP=cl; fi\n\
    if test -z \"$LD\"; then LD=link; fi\n\
    if test -z \"$AS\"; then AS=ml; fi\n\
    if test -z \"$MIDL\"; then MIDL=midl; fi\n\
    ;;\n\
esac\n\
fi\n\
\n\
COMPILE_ENVIRONMENT=1\n\
MOZ_ARG_ENABLE_BOOL(compile-environment,\n\
[  --disable-compile-environment\n\
                           Disable compiler/library checks.],\n\
    COMPILE_ENVIRONMENT=1,\n\
    COMPILE_ENVIRONMENT= )\n\
\n\
AC_PATH_PROGS(NSINSTALL_BIN, nsinstall )\n\
if test -z \"$COMPILE_ENVIRONMENT\"; then\n\
if test -z \"$NSINSTALL_BIN\" || test \"$NSINSTALL_BIN\" = \":\"; then\n\
    AC_MSG_ERROR([nsinstall not found in \\$PATH])\n\
fi\n\
fi\n\
AC_SUBST(NSINSTALL_BIN)\n\
\n\
dnl ========================================================\n\
dnl Checks for compilers.\n\
dnl ========================================================\n\
dnl Set CROSS_COMPILE in the environment when running configure\n\
dnl to use the cross-compile setup for now\n\
dnl ========================================================\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
\n\
dnl Do some special WinCE toolchain stuff\n\
case \"$target\" in\n\
*wince)\n\
    echo -----------------------------------------------------------------------------\n\
    echo Building Windows CE Shunt Library and Tool Chain\n\
    echo -----------------------------------------------------------------------------\n\
\n\
    echo -n \"#define TOPSRCDIR \\\"\" > $srcdir/build/wince/tools/topsrcdir.h\n\
    `$srcdir/build/cygwin-wrapper echo -n $_topsrcdir >> $srcdir/build/wince/tools/topsrcdir.h`\n\
    echo -n \\\" >> $srcdir/build/wince/tools/topsrcdir.h\n\
\n\
    make -C  $srcdir/build/wince/tools\n\
    echo -----------------------------------------------------------------------------\n\
    ;;\n\
esac\n\
\n\
if test -n \"$CROSS_COMPILE\" && test \"$target\" != \"$host\"; then\n\
    echo \"cross compiling from $host to $target\"\n\
    cross_compiling=yes\n\
\n\
    _SAVE_CC=\"$CC\"\n\
    _SAVE_CFLAGS=\"$CFLAGS\"\n\
    _SAVE_LDFLAGS=\"$LDFLAGS\"\n\
\n\
    AC_MSG_CHECKING([for host c compiler])\n\
    AC_CHECK_PROGS(HOST_CC, $HOST_CC gcc cc /usr/ucb/cc cl icc, \"\")\n\
    if test -z \"$HOST_CC\"; then\n\
        AC_MSG_ERROR([no acceptable c compiler found in \\$PATH])\n\
    fi\n\
    AC_MSG_RESULT([$HOST_CC])\n\
    AC_MSG_CHECKING([for host c++ compiler])\n\
    AC_CHECK_PROGS(HOST_CXX, $HOST_CXX $CCC c++ g++ gcc CC cxx cc++ cl icc, \"\")\n\
    if test -z \"$HOST_CXX\"; then\n\
        AC_MSG_ERROR([no acceptable c++ compiler found in \\$PATH])\n\
    fi\n\
    AC_MSG_RESULT([$HOST_CXX])\n\
\n\
    if test -z \"$HOST_CFLAGS\"; then\n\
        HOST_CFLAGS=\"$CFLAGS\"\n\
    fi\n\
    if test -z \"$HOST_CXXFLAGS\"; then\n\
        HOST_CXXFLAGS=\"$CXXFLAGS\"\n\
    fi\n\
    if test -z \"$HOST_LDFLAGS\"; then\n\
        HOST_LDFLAGS=\"$LDFLAGS\"\n\
    fi\n\
    AC_CHECK_PROGS(HOST_RANLIB, $HOST_RANLIB ranlib, ranlib, :)\n\
    AC_CHECK_PROGS(HOST_AR, $HOST_AR ar, ar, :)\n\
    CC=\"$HOST_CC\"\n\
    CFLAGS=\"$HOST_CFLAGS\"\n\
    LDFLAGS=\"$HOST_LDFLAGS\"\n\
\n\
    AC_MSG_CHECKING([whether the host c compiler ($HOST_CC $HOST_CFLAGS $HOST_LDFLAGS) works])\n\
    AC_TRY_COMPILE([], [return(0);], \n\
	[ac_cv_prog_hostcc_works=1 AC_MSG_RESULT([yes])],\n\
	AC_MSG_ERROR([installation or configuration problem: host compiler $HOST_CC cannot create executables.]) )\n\
\n\
    CC=\"$HOST_CXX\"\n\
    CFLAGS=\"$HOST_CXXFLAGS\"\n\
\n\
    AC_MSG_CHECKING([whether the host c++ compiler ($HOST_CXX $HOST_CXXFLAGS $HOST_LDFLAGS) works])\n\
    AC_TRY_COMPILE([], [return(0);], \n\
	[ac_cv_prog_hostcxx_works=1 AC_MSG_RESULT([yes])],\n\
	AC_MSG_ERROR([installation or configuration problem: host compiler $HOST_CXX cannot create executables.]) )\n\
    \n\
    CC=$_SAVE_CC\n\
    CFLAGS=$_SAVE_CFLAGS\n\
    LDFLAGS=$_SAVE_LDFLAGS\n\
\n\
    case \"$build:$target\" in\n\
      powerpc-apple-darwin8*:i?86-apple-darwin*)\n\
        dnl The Darwin cross compiler doesn\'t necessarily point itself at a\n\
        dnl root that has libraries for the proper architecture, it defaults\n\
        dnl to the system root.  The libraries in the system root on current\n\
        dnl versions of PPC OS X 10.4 aren\'t fat, so these target compiler\n\
        dnl checks will fail.  Fake a working SDK in that case.\n\
        _SAVE_CFLAGS=$CFLAGS\n\
        _SAVE_CXXFLAGS=$CXXLAGS\n\
        CFLAGS=\"-isysroot /Developer/SDKs/MacOSX10.4u.sdk $CFLAGS\"\n\
        CXXFLAGS=\"-isysroot /Developer/SDKs/MacOSX10.4u.sdk $CXXFLAGS\"\n\
        ;;\n\
    esac\n\
\n\
    AC_CHECK_PROGS(CC, $CC \"${target_alias}-gcc\" \"${target}-gcc\", :)\n\
    unset ac_cv_prog_CC\n\
    AC_PROG_CC\n\
    AC_CHECK_PROGS(CXX, $CXX \"${target_alias}-g++\" \"${target}-g++\", :)\n\
    unset ac_cv_prog_CXX\n\
    AC_PROG_CXX\n\
\n\
    case \"$build:$target\" in\n\
      powerpc-apple-darwin8*:i?86-apple-darwin*)\n\
        dnl Revert the changes made above.  From this point on, the target\n\
        dnl compiler will never be used without applying the SDK to CFLAGS\n\
        dnl (see --with-macos-sdk below).\n\
        CFLAGS=$_SAVE_CFLAGS\n\
        CXXFLAGS=$_SAVE_CXXFLAGS\n\
        ;;\n\
    esac\n\
\n\
    AC_CHECK_PROGS(RANLIB, $RANLIB \"${target_alias}-ranlib\" \"${target}-ranlib\", :)\n\
    AC_CHECK_PROGS(AR, $AR \"${target_alias}-ar\" \"${target}-ar\", :)\n\
    AC_PATH_PROGS(AS, $AS \"${target_alias}-as\" \"${target}-as\", :)\n\
    AC_CHECK_PROGS(LD, $LD \"${target_alias}-ld\" \"${target}-ld\", :)\n\
    AC_CHECK_PROGS(STRIP, $STRIP \"${target_alias}-strip\" \"${target}-strip\", :)\n\
    AC_CHECK_PROGS(WINDRES, $WINDRES \"${target_alias}-windres\" \"${target}-windres\", :)\n\
    AC_DEFINE(CROSS_COMPILE)\n\
else\n\
    AC_PROG_CC\n\
    AC_PROG_CXX\n\
    AC_PROG_RANLIB\n\
    AC_PATH_PROGS(AS, $AS as, $CC)\n\
    AC_CHECK_PROGS(AR, ar, :)\n\
    AC_CHECK_PROGS(LD, ld, :)\n\
    AC_CHECK_PROGS(STRIP, strip, :)\n\
    AC_CHECK_PROGS(WINDRES, windres, :)\n\
    if test -z \"$HOST_CC\"; then\n\
        HOST_CC=\"$CC\"\n\
    fi\n\
    if test -z \"$HOST_CFLAGS\"; then\n\
        HOST_CFLAGS=\"$CFLAGS\"\n\
    fi\n\
    if test -z \"$HOST_CXX\"; then\n\
        HOST_CXX=\"$CXX\"\n\
    fi\n\
    if test -z \"$HOST_CXXFLAGS\"; then\n\
        HOST_CXXFLAGS=\"$CXXFLAGS\"\n\
    fi\n\
    if test -z \"$HOST_LDFLAGS\"; then\n\
        HOST_LDFLAGS=\"$LDFLAGS\"\n\
    fi\n\
    if test -z \"$HOST_RANLIB\"; then\n\
        HOST_RANLIB=\"$RANLIB\"\n\
    fi\n\
    if test -z \"$HOST_AR\"; then\n\
       HOST_AR=\"$AR\"\n\
    fi\n\
fi\n\
\n\
GNU_AS=\n\
GNU_LD=\n\
GNU_CC=\n\
GNU_CXX=\n\
CC_VERSION=\'N/A\'\n\
CXX_VERSION=\'N/A\'\n\
if test \"$GCC\" = \"yes\"; then\n\
    GNU_CC=1\n\
    CC_VERSION=`$CC -v 2>&1 | grep \'gcc version\'`\n\
fi\n\
if test \"$GXX\" = \"yes\"; then\n\
    GNU_CXX=1\n\
    CXX_VERSION=`$CXX -v 2>&1 | grep \'gcc version\'`\n\
fi\n\
if test \"`echo | $AS -v 2>&1 | grep -c GNU`\" != \"0\"; then\n\
    GNU_AS=1\n\
fi\n\
if test \"`echo | $LD -v 2>&1 | grep -c GNU`\" != \"0\"; then\n\
    GNU_LD=1\n\
fi\n\
if test \"$GNU_CC\"; then\n\
    if `$CC -print-prog-name=ld` -v 2>&1 | grep -c GNU >/dev/null; then\n\
        GCC_USE_GNU_LD=1\n\
    fi\n\
fi\n\
\n\
dnl Special win32 checks\n\
dnl ========================================================\n\
case \"$target\" in\n\
*-wince)\n\
    WINVER=400\n\
    ;;\n\
*)\n\
    if test -n \"$GNU_CC\"; then  \n\
        WINVER=501\n\
    else    \n\
        WINVER=500\n\
    fi\n\
    ;;\n\
esac\n\
\n\
MOZ_ARG_WITH_STRING(windows-version,\n\
[  --with-windows-version=WINVER\n\
                          Minimum Windows version (WINVER) to support\n\
                              400: Windows 95\n\
                              500: Windows 2000\n\
                              501: Windows XP],\n\
  WINVER=$withval)\n\
\n\
case \"$WINVER\" in\n\
400|500|501)\n\
    ;;\n\
\n\
*)\n\
    AC_MSG_ERROR([Invalid value --with-windows-version, must be 400, 500 or 501]);\n\
    ;;\n\
\n\
esac\n\
\n\
case \"$target\" in\n\
*-cygwin*|*-mingw*|*-msvc*|*-mks*|*-wince)\n\
    if test \"$GCC\" != \"yes\"; then\n\
        # Check to see if we are really running in a msvc environemnt\n\
        _WIN32_MSVC=1\n\
        AC_CHECK_PROGS(MIDL, midl)\n\
\n\
        # Make sure compilers are valid\n\
        CFLAGS=\"$CFLAGS -TC -nologo\"\n\
        CXXFLAGS=\"$CXXFLAGS -TP -nologo\"\n\
        AC_LANG_SAVE\n\
        AC_LANG_C\n\
        AC_TRY_COMPILE([#include <stdio.h>],\n\
            [ printf(\"Hello World\\n\"); ],,\n\
            AC_MSG_ERROR([\\$(CC) test failed.  You must have MS VC++ in your path to build.]) )\n\
\n\
        AC_LANG_CPLUSPLUS\n\
        AC_TRY_COMPILE([#include <new.h>],\n\
            [ unsigned *test = new unsigned(42); ],,\n\
            AC_MSG_ERROR([\\$(CXX) test failed.  You must have MS VC++ in your path to build.]) )\n\
        AC_LANG_RESTORE\n\
        \n\
        changequote(,)\n\
        _MSVC_VER_FILTER=\'s|.* \\([0-9][0-9]*\\.[0-9][0-9]*\\.[0-9][0-9]*\\).*|\\1|p\'\n\
        changequote([,])\n\
        \n\
        # Determine compiler version\n\
        CC_VERSION=`\"${CC}\" -v 2>&1 | sed -ne \"$_MSVC_VER_FILTER\"`\n\
        _CC_MAJOR_VERSION=`echo ${CC_VERSION} | $AWK -F\\. \'{ print $1 }\'`\n\
        _CC_MINOR_VERSION=`echo ${CC_VERSION} | $AWK -F\\. \'{ print $2 }\'`\n\
        _MSC_VER=${_CC_MAJOR_VERSION}${_CC_MINOR_VERSION}\n\
\n\
        CXX_VERSION=`\"${CXX}\" -v 2>&1 | sed -ne \"$_MSVC_VER_FILTER\"`\n\
        _CXX_MAJOR_VERSION=`echo ${CXX_VERSION} | $AWK -F\\. \'{ print $1 }\'`\n\
\n\
        if test \"$_CC_MAJOR_VERSION\" != \"$_CXX_MAJOR_VERSION\"; then\n\
            AC_MSG_ERROR([The major versions of \\$CC and \\$CXX do not match.])\n\
        fi\n\
        if test \"$_CC_MAJOR_VERSION\" = \"12\"; then\n\
            _CC_SUITE=6\n\
        elif test \"$_CC_MAJOR_VERSION\" = \"13\"; then\n\
            _CC_SUITE=7\n\
        elif test \"$_CC_MAJOR_VERSION\" = \"14\"; then\n\
            _CC_SUITE=8\n\
            CXXFLAGS=\"$CXXFLAGS -Zc:wchar_t-\"\n\
            AC_DEFINE(_CRT_SECURE_NO_DEPRECATE)\n\
            AC_DEFINE(_CRT_NONSTDC_NO_DEPRECATE)\n\
        elif test \"$_CC_MAJOR_VERSION\" = \"15\"; then\n\
            _CC_SUITE=9\n\
            CXXFLAGS=\"$CXXFLAGS -Zc:wchar_t-\"\n\
            AC_DEFINE(_CRT_SECURE_NO_WARNINGS)\n\
            AC_DEFINE(_CRT_NONSTDC_NO_WARNINGS)\n\
        else\n\
            AC_MSG_ERROR([This version of the MSVC compiler, $CC_VERSION , is unsupported.])\n\
        fi\n\
\n\
        _MOZ_RTTI_FLAGS_ON=\'-GR\'\n\
        _MOZ_RTTI_FLAGS_OFF=\'-GR-\'\n\
        _MOZ_EXCEPTIONS_FLAGS_ON=\'-EHsc\'\n\
        _MOZ_EXCEPTIONS_FLAGS_OFF=\'\'\n\
\n\
        if test -n \"$WIN32_REDIST_DIR\"; then\n\
            WIN32_REDIST_DIR=`cd \"$WIN32_REDIST_DIR\" && pwd`\n\
        fi\n\
	\n\
        # bug #249782\n\
        # ensure that mt.exe is Microsoft (R) Manifest Tool and not magnetic tape manipulation utility (or something else)\n\
        if test \"$_CC_SUITE\" -ge \"8\"; then\n\
                MSMT_TOOL=`mt 2>&1|grep \'Microsoft (R) Manifest Tool\'`\n\
                if test -n \"MSMT_TOOL\"; then\n\
                        MSMANIFEST_TOOL_VERSION=`echo ${MSMANIFEST_TOOL}|grep -Po \"(^|\\s)[0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+)?(\\s|$)\"`\n\
                        if test -z \"MSMANIFEST_TOOL_VERSION\"; then\n\
                                AC_MSG_WARN([Unknown version of the Microsoft (R) Manifest Tool.])\n\
                        fi\n\
                        MSMANIFEST_TOOL=1\n\
                        unset MSMT_TOOL\n\
                else\n\
                        AC_MSG_ERROR([Microsoft (R) Manifest Tool must be in your \\$PATH.])\n\
                fi\n\
        fi\n\
\n\
        # Check linker version\n\
        _LD_FULL_VERSION=`\"${LD}\" -v 2>&1 | sed -ne \"$_MSVC_VER_FILTER\"`\n\
        _LD_MAJOR_VERSION=`echo ${_LD_FULL_VERSION} | $AWK -F\\. \'{ print $1 }\'`\n\
        if test \"$_LD_MAJOR_VERSION\" != \"$_CC_SUITE\"; then\n\
            AC_MSG_ERROR([The linker major version, $_LD_FULL_VERSION,  does not match the compiler suite version, $_CC_SUITE.])\n\
        fi\n\
        INCREMENTAL_LINKER=1\n\
\n\
        # Check midl version\n\
        _MIDL_FULL_VERSION=`\"${MIDL}\" -v 2>&1 | sed -ne \"$_MSVC_VER_FILTER\"`\n\
        _MIDL_MAJOR_VERSION=`echo ${_MIDL_FULL_VERSION} | $AWK -F\\. \'{ print $1 }\'`\n\
        _MIDL_MINOR_VERSION=`echo ${_MIDL_FULL_VERSION} | $AWK -F\\. \'{ print $2 }\'`\n\
        _MIDL_REV_VERSION=`echo ${_MIDL_FULL_VERSION} | $AWK -F\\. \'{ print $3 }\'`\n\
         # Add flags if necessary\n\
         AC_MSG_CHECKING([for midl flags])\n\
         if test \\( \"$_MIDL_MAJOR_VERSION\" -gt \"6\" \\) -o \\( \"$_MIDL_MAJOR_VERSION\" = \"6\" -a \"$_MIDL_MINOR_VERSION\" -gt \"0\" \\) -o \\( \"$_MIDL_MAJOR_VERSION\" = \"6\" -a \"$_MIDL_MINOR_VERSION\" = \"00\" -a \"$_MIDL_REV_VERSION\" -gt \"359\" \\); then\n\
             # Starting with MIDL version 6.0.359, the MIDL compiler\n\
             # generates /Oicf /robust stubs by default, which is not\n\
             # compatible with versions of Windows older than Win2k.\n\
             # This switches us back to the old behaviour. When we drop\n\
             # support for Windows older than Win2k, we should remove\n\
             # this.\n\
             MIDL_FLAGS=\"${MIDL_FLAGS} -no_robust\"\n\
             AC_MSG_RESULT([need -no_robust])\n\
         else\n\
             MIDL_FLAGS=\"${MIDL_FLAGS}\"\n\
             AC_MSG_RESULT([none needed])\n\
        fi\n\
        \n\
        unset _MSVC_VER_FILTER\n\
        \n\
    else\n\
        # Check w32api version\n\
        _W32API_MAJOR_VERSION=`echo $W32API_VERSION | $AWK -F\\. \'{ print $1 }\'`\n\
        _W32API_MINOR_VERSION=`echo $W32API_VERSION | $AWK -F\\. \'{ print $2 }\'`\n\
        AC_MSG_CHECKING([for w32api version >= $W32API_VERSION])\n\
        AC_TRY_COMPILE([#include <w32api.h>],\n\
            #if (__W32API_MAJOR_VERSION < $_W32API_MAJOR_VERSION) || \\\n\
                (__W32API_MAJOR_VERSION == $_W32API_MAJOR_VERSION && \\\n\
                 __W32API_MINOR_VERSION < $_W32API_MINOR_VERSION)\n\
                #error \"test failed.\"\n\
            #endif\n\
            , [ res=yes ], [ res=no ])\n\
        AC_MSG_RESULT([$res])\n\
        if test \"$res\" != \"yes\"; then\n\
            AC_MSG_ERROR([w32api version $W32API_VERSION or higher required.])\n\
        fi\n\
        # Check windres version\n\
        AC_MSG_CHECKING([for windres version >= $WINDRES_VERSION])\n\
        _WINDRES_VERSION=`${WINDRES} --version 2>&1 | grep -i windres 2>/dev/null | $AWK \'{ print $3 }\'`\n\
        AC_MSG_RESULT([$_WINDRES_VERSION])\n\
        _WINDRES_MAJOR_VERSION=`echo $_WINDRES_VERSION | $AWK -F\\. \'{ print $1 }\'`\n\
        _WINDRES_MINOR_VERSION=`echo $_WINDRES_VERSION | $AWK -F\\. \'{ print $2 }\'`\n\
        _WINDRES_RELEASE_VERSION=`echo $_WINDRES_VERSION | $AWK -F\\. \'{ print $3 }\'`\n\
        WINDRES_MAJOR_VERSION=`echo $WINDRES_VERSION | $AWK -F\\. \'{ print $1 }\'`\n\
        WINDRES_MINOR_VERSION=`echo $WINDRES_VERSION | $AWK -F\\. \'{ print $2 }\'`\n\
        WINDRES_RELEASE_VERSION=`echo $WINDRES_VERSION | $AWK -F\\. \'{ print $3 }\'`\n\
        if test \"$_WINDRES_MAJOR_VERSION\" -lt \"$WINDRES_MAJOR_VERSION\" -o \\\n\
                \"$_WINDRES_MAJOR_VERSION\" -eq \"$WINDRES_MAJOR_VERSION\" -a \\\n\
                \"$_WINDRES_MINOR_VERSION\" -lt \"$WINDRES_MINOR_VERSION\" -o \\\n\
                \"$_WINDRES_MAJOR_VERSION\" -eq \"$WINDRES_MAJOR_VERSION\" -a \\\n\
                \"$_WINDRES_MINOR_VERSION\" -eq \"$WINDRES_MINOR_VERSION\" -a \\\n\
                \"$_WINDRES_RELEASE_VERSION\" -lt \"$WINDRES_RELEASE_VERSION\"\n\
        then\n\
            AC_MSG_ERROR([windres version $WINDRES_VERSION or higher is required to build.])\n\
        fi\n\
    fi # !GNU_CC\n\
\n\
    AC_DEFINE_UNQUOTED(WINVER,0x$WINVER)\n\
    AC_DEFINE_UNQUOTED(_WIN32_WINNT,0x$WINVER)\n\
    # Require OS features provided by IE 4.0\n\
    AC_DEFINE_UNQUOTED(_WIN32_IE,0x0400)\n\
    ;;\n\
esac\n\
\n\
if test -n \"$_WIN32_MSVC\"; then\n\
    SKIP_PATH_CHECKS=1\n\
    SKIP_COMPILER_CHECKS=1\n\
    SKIP_LIBRARY_CHECKS=1\n\
    AC_CHECK_HEADERS(mmintrin.h)\n\
fi\n\
\n\
dnl Test breaks icc on OS/2 && MSVC\n\
if test \"$CC\" != \"icc\" -a -z \"$_WIN32_MSVC\"; then\n\
    AC_PROG_CC_C_O\n\
    if grep \"NO_MINUS_C_MINUS_O 1\" ./confdefs.h >/dev/null; then\n\
        USING_HCC=1\n\
        _OLDCC=$CC\n\
        _OLDCXX=$CXX\n\
        CC=\"${srcdir}/build/hcc \'$CC\'\"\n\
        CXX=\"${srcdir}/build/hcpp \'$CXX\'\"\n\
    fi\n\
fi\n\
\n\
AC_PROG_CPP\n\
AC_PROG_CXXCPP\n\
\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
AC_SUBST(MIDL_FLAGS)\n\
AC_SUBST(_MSC_VER)\n\
\n\
AC_SUBST(GNU_AS)\n\
AC_SUBST(GNU_LD)\n\
AC_SUBST(GNU_CC)\n\
AC_SUBST(GNU_CXX)\n\
\n\
dnl ========================================================\n\
dnl Checks for programs.\n\
dnl ========================================================\n\
AC_PROG_INSTALL\n\
AC_PROG_LN_S\n\
AC_PATH_PROGS(PERL, $PERL perl5 perl )\n\
if test -z \"$PERL\" || test \"$PERL\" = \":\"; then\n\
    AC_MSG_ERROR([perl not found in \\$PATH])\n\
fi\n\
\n\
if test -z \"$TINDERBOX_SKIP_PERL_VERSION_CHECK\"; then\n\
AC_MSG_CHECKING([for minimum required perl version >= $PERL_VERSION])\n\
_perl_version=`PERL_VERSION=$PERL_VERSION $PERL -e \'print \"$]\"; if ($] >= $ENV{PERL_VERSION}) { exit(0); } else { exit(1); }\' 2>&5`\n\
_perl_res=$?\n\
AC_MSG_RESULT([$_perl_version])\n\
\n\
if test \"$_perl_res\" != 0; then\n\
    AC_MSG_ERROR([Perl $PERL_VERSION or higher is required.])\n\
fi\n\
fi\n\
\n\
AC_MSG_CHECKING([for full perl installation])\n\
_perl_archlib=`$PERL -e \'use Config; if ( -d $Config{archlib} ) { exit(0); } else { exit(1); }\' 2>&5`\n\
_perl_res=$?\n\
if test \"$_perl_res\" != 0; then\n\
    AC_MSG_RESULT([no])\n\
    AC_MSG_ERROR([Cannot find Config.pm or \\$Config{archlib}.  A full perl installation is required.])\n\
else\n\
    AC_MSG_RESULT([yes])    \n\
fi\n\
\n\
AC_PATH_PROGS(PYTHON, $PYTHON python)\n\
if test -z \"$PYTHON\"; then\n\
    AC_MSG_ERROR([python was not found in \\$PATH])\n\
fi\n\
echo PYTHON=\"$PYTHON\"\n\
\n\
AC_PATH_PROG(DOXYGEN, doxygen, :)\n\
AC_PATH_PROG(WHOAMI, whoami, :)\n\
AC_PATH_PROG(AUTOCONF, autoconf, :)\n\
AC_PATH_PROG(UNZIP, unzip, :)\n\
AC_PATH_PROGS(ZIP, zip)\n\
if test -z \"$ZIP\" || test \"$ZIP\" = \":\"; then\n\
    AC_MSG_ERROR([zip not found in \\$PATH])\n\
fi\n\
AC_PATH_PROG(SYSTEM_MAKEDEPEND, makedepend)\n\
AC_PATH_PROG(XARGS, xargs)\n\
if test -z \"$XARGS\" || test \"$XARGS\" = \":\"; then\n\
    AC_MSG_ERROR([xargs not found in \\$PATH .])\n\
fi\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
\n\
dnl ========================================================\n\
dnl = Mac OS X toolchain support\n\
dnl ========================================================\n\
\n\
case \"$target_os\" in\n\
darwin*)\n\
    dnl Current known valid versions for GCC_VERSION are 2.95.2 3.1 3.3 4.0.\n\
    dnl 4.0 identifies itself as 4.0.x, so strip the decidecimal for\n\
    dnl the environment and includedir purposes (when using an SDK, below),\n\
    dnl but remember the full version number for the libdir (SDK).\n\
    changequote(,)\n\
    GCC_VERSION_FULL=`echo $CXX_VERSION | $PERL -pe \'s/^.*gcc version ([^ ]*).*/$1/\'`\n\
    GCC_VERSION=`echo $GCC_VERSION_FULL | $PERL -pe \'(split(/\\./))[0]>=4&&s/(^\\d*\\.\\d*).*/$1/;\'`\n\
    changequote([,])\n\
    if test \"$GCC_VERSION_FULL\" = \"4.0.0\" ; then\n\
        dnl Bug 280479, but this keeps popping up in bug 292530 too because\n\
        dnl 4.0.0/4061 is the default compiler in Tiger.\n\
        changequote(,)\n\
        GCC_BUILD=`echo $CXX_VERSION | $PERL -pe \'s/^.*build ([^ )]*).*/$1/\'`\n\
        changequote([,])\n\
        if test \"$GCC_BUILD\" = \"4061\" ; then\n\
            AC_MSG_ERROR([You are attempting to use Apple gcc 4.0 build 4061.\n\
This compiler was supplied with Xcode 2.0, and contains bugs that prevent it\n\
from building Mozilla.\n\
Either upgrade to Xcode 2.1 or later, or switch the system\'s default compiler\n\
to gcc 3.3 by running \\\"sudo gcc_select 3.3\\\".])\n\
        fi\n\
    fi\n\
\n\
    dnl xcodebuild needs GCC_VERSION defined in the environment, since it\n\
    dnl doesn\'t respect the CC/CXX setting.  With GCC_VERSION set, it will use\n\
    dnl /usr/bin/g(cc|++)-$GCC_VERSION.\n\
    AC_PATH_PROGS(PBBUILD, pbbuild xcodebuild pbxbuild)\n\
\n\
    case \"$PBBUILD\" in\n\
      *xcodebuild*)\n\
        changequote(,)\n\
        XCODEBUILD_VERSION=`$PBBUILD -version 2>/dev/null | sed -e \'s/.*DevToolsCore-\\([0-9]*\\).*/\\1/\'`\n\
        changequote([,])\n\
        if test -n \"$XCODEBUILD_VERSION\" && test \"$XCODEBUILD_VERSION\" -ge 620 ; then\n\
          HAS_XCODE_2_1=1;\n\
        fi\n\
      ;;\n\
    esac\n\
\n\
    dnl sdp was formerly in /Developer/Tools.  As of Mac OS X 10.4 (Darwin 8),\n\
    dnl it has moved into /usr/bin.\n\
    AC_PATH_PROG(SDP, sdp, :, [$PATH:/usr/bin:/Developer/Tools])\n\
    ;;\n\
esac\n\
\n\
AC_SUBST(GCC_VERSION)\n\
AC_SUBST(XCODEBUILD_VERSION)\n\
AC_SUBST(HAS_XCODE_2_1)\n\
\n\
dnl The universal machinery sets UNIVERSAL_BINARY to inform packager.mk\n\
dnl that a universal binary is being produced.\n\
AC_SUBST(UNIVERSAL_BINARY)\n\
\n\
dnl ========================================================\n\
dnl = Mac OS X SDK support\n\
dnl ========================================================\n\
MACOS_SDK_DIR=\n\
NEXT_ROOT=\n\
MOZ_ARG_WITH_STRING(macos-sdk,\n\
[  --with-macos-sdk=dir   Location of platform SDK to use (Mac OS X only)],\n\
    MACOS_SDK_DIR=$withval)\n\
\n\
dnl MACOS_SDK_DIR will be set to the SDK location whenever one is in use.\n\
dnl NEXT_ROOT will be set and exported only if it\'s needed.\n\
AC_SUBST(MACOS_SDK_DIR)\n\
AC_SUBST(NEXT_ROOT)\n\
\n\
if test \"$MACOS_SDK_DIR\"; then\n\
  dnl Sync this section with the ones in NSPR and NSS.\n\
  dnl Changes to the cross environment here need to be accounted for in\n\
  dnl the libIDL checks (below) and xpidl build.\n\
\n\
  if test ! -d \"$MACOS_SDK_DIR\"; then\n\
    AC_MSG_ERROR([SDK not found.  When using --with-macos-sdk, you must\n\
specify a valid SDK.  SDKs are installed when the optional cross-development\n\
tools are selected during the Xcode/Developer Tools installation.])\n\
  fi\n\
\n\
  GCC_VERSION_MAJOR=`echo $GCC_VERSION_FULL | $PERL -pe \'s/(^\\d*).*/$1/;\'`\n\
  if test \"$GCC_VERSION_MAJOR\" -lt \"4\" ; then\n\
    SDK_C_INCLUDE=\"-isystem ${MACOS_SDK_DIR}/usr/include/gcc/darwin/${GCC_VERSION} -isystem ${MACOS_SDK_DIR}/usr/include -F${MACOS_SDK_DIR}/System/Library/Frameworks\"\n\
    if test -d \"${MACOS_SDK_DIR}/Library/Frameworks\" ; then\n\
      SDK_C_INCLUDE=\"$SDK_C_INCLUDE -F${MACOS_SDK_DIR}/Library/Frameworks\"\n\
    fi\n\
    SDK_CXX_INCLUDE=\"-I${MACOS_SDK_DIR}/usr/include/gcc/darwin/${GCC_VERSION}/c++ -I${MACOS_SDK_DIR}/usr/include/gcc/darwin/${GCC_VERSION}/c++/ppc-darwin -I${MACOS_SDK_DIR}/usr/include/gcc/darwin/${GCC_VERSION}/c++/backward\"\n\
\n\
    CFLAGS=\"$CFLAGS -nostdinc ${SDK_C_INCLUDE}\"\n\
    CXXFLAGS=\"$CXXFLAGS -nostdinc -nostdinc++ ${SDK_CXX_INCLUDE} ${SDK_C_INCLUDE}\"\n\
\n\
    dnl CPP/CXXCPP needs to be set for AC_CHECK_HEADER.\n\
    CPP=\"$CPP -nostdinc ${SDK_C_INCLUDE}\"\n\
    CXXCPP=\"$CXXCPP -nostdinc -nostdinc++ ${SDK_CXX_INCLUDE} ${SDK_C_INCLUDE}\"\n\
\n\
    dnl ld support for -syslibroot is compiler-agnostic, but only available\n\
    dnl on Tiger.  Although it\'s possible to switch on the build host\'s\n\
    dnl OS release to use ld -syslibroot when available, ld -syslibroot will\n\
    dnl cause warnings as long as NEXT_ROOT is set.  NEXT_ROOT should be\n\
    dnl set because both the compiler and linker use it.\n\
    LIBS=\"-L${MACOS_SDK_DIR}/usr/lib/gcc/darwin -L${MACOS_SDK_DIR}/usr/lib/gcc/darwin/${GCC_VERSION_FULL} -L${MACOS_SDK_DIR}/usr/lib $LIBS\"\n\
    export NEXT_ROOT=$MACOS_SDK_DIR\n\
\n\
    if test -n \"$CROSS_COMPILE\" ; then\n\
      dnl NEXT_ROOT will be in the environment, but it shouldn\'t be set for\n\
      dnl the build host.  HOST_CXX is presently unused.\n\
      HOST_CC=\"NEXT_ROOT= $HOST_CC\"\n\
      HOST_CXX=\"NEXT_ROOT= $HOST_CXX\"\n\
    fi\n\
  else\n\
    dnl gcc >= 4.0 uses different paths than above, but knows how to find\n\
    dnl them itself.\n\
    CFLAGS=\"$CFLAGS -isysroot ${MACOS_SDK_DIR}\"\n\
    CXXFLAGS=\"$CXXFLAGS -isysroot ${MACOS_SDK_DIR}\"\n\
\n\
    dnl CPP/CXXCPP needs to be set for AC_CHECK_HEADER.\n\
    CPP=\"$CPP -isysroot ${MACOS_SDK_DIR}\"\n\
    CXXCPP=\"$CXXCPP -isysroot ${MACOS_SDK_DIR}\"\n\
\n\
    if test \"$GCC_VERSION_FULL\" = \"4.0.0\" ; then\n\
      dnl If gcc >= 4.0, we\'re guaranteed to be on Tiger, which has an ld\n\
      dnl that supports -syslibroot.  Don\'t set NEXT_ROOT because it will\n\
      dnl be ignored and cause warnings when -syslibroot is specified.\n\
      dnl gcc 4.0.1 will pass -syslibroot to ld automatically based on\n\
      dnl the -isysroot it receives, so this is only needed with 4.0.0.\n\
      LDFLAGS=\"$LDFLAGS -Wl,-syslibroot,${MACOS_SDK_DIR}\"\n\
    fi\n\
  fi\n\
\n\
  AC_LANG_SAVE\n\
  AC_MSG_CHECKING([for valid compiler/Mac OS X SDK combination])\n\
  AC_LANG_CPLUSPLUS\n\
  AC_TRY_COMPILE([#include <new>\n\
                 int main() { return 0; }],\n\
   result=yes,\n\
   result=no)\n\
  AC_LANG_RESTORE\n\
  AC_MSG_RESULT($result)\n\
\n\
  if test \"$result\" = \"no\" ; then\n\
    AC_MSG_ERROR([The selected compiler and Mac OS X SDK are incompatible.])\n\
  fi\n\
fi\n\
\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl Be sure the make we use is GNU make.\n\
dnl on win32, gmake.exe is the generally the wrong version\n\
case \"$host_os\" in\n\
cygwin*|mingw*|mks*|msvc*)\n\
    AC_PATH_PROGS(MAKE, $MAKE make gmake, :)\n\
    ;;\n\
*)\n\
    AC_PATH_PROGS(MAKE, $MAKE gmake make, :)\n\
    ;;\n\
esac\n\
_make_try=`$MAKE --version 2>/dev/null | grep GNU`\n\
if test ! \"$_make_try\"\n\
then\n\
	echo\n\
	echo \"*** $MAKE is not GNU Make.  You will not be able to build Mozilla without GNU Make.\"\n\
	echo\n\
	exit 1\n\
fi\n\
dnl Now exit if version if < MAKE_VERSION\n\
rm -f dummy.mk\n\
echo \'all: ; @echo $(MAKE_VERSION)\' > dummy.mk\n\
_make_vers=`$MAKE --no-print-directory -f dummy.mk all 2>/dev/null`\n\
rm -f dummy.mk\n\
_MAKE_MAJOR_VERSION=`echo $_make_vers | $AWK -F\\. \'{ print $1 }\'`\n\
_MAKE_MINOR_VERSION=`echo $_make_vers | $AWK -F\\. \'{ print $2 }\'`\n\
MAKE_MAJOR_VERSION=`echo $MAKE_VERSION | $AWK -F\\. \'{ print $1 }\'`\n\
MAKE_MINOR_VERSION=`echo $MAKE_VERSION | $AWK -F\\. \'{ print $2 }\'`\n\
if test \"$_MAKE_MAJOR_VERSION\" -lt \"$MAKE_MAJOR_VERSION\" || \\\n\
   test \"$_MAKE_MAJOR_VERSION\" = \"$MAKE_MAJOR_VERSION\" -a \\\n\
        \"$_MAKE_MINOR_VERSION\" -lt \"$MAKE_MINOR_VERSION\"; then\n\
   AC_MSG_ERROR([GNU Make $MAKE_VERSION or higher is required to build Mozilla.])\n\
fi\n\
AC_SUBST(MAKE)\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
\n\
AC_PATH_XTRA\n\
\n\
dnl Check in X11 include directory too.\n\
if test \"$no_x\" != \"yes\"; then\n\
    CPPFLAGS=\"$CPPFLAGS $X_CFLAGS\"\n\
fi\n\
\n\
XCFLAGS=\"$X_CFLAGS\"\n\
\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl ========================================================\n\
dnl set the defaults first\n\
dnl ========================================================\n\
AS_BIN=$AS\n\
AR_FLAGS=\'cr $@\'\n\
AR_LIST=\'$(AR) t\'\n\
AR_EXTRACT=\'$(AR) x\'\n\
AR_DELETE=\'$(AR) d\'\n\
AS=\'$(CC)\'\n\
AS_DASH_C_FLAG=\'-c\'\n\
DLL_PREFIX=lib\n\
LIB_PREFIX=lib\n\
DLL_SUFFIX=.so\n\
OBJ_SUFFIX=o\n\
LIB_SUFFIX=a\n\
ASM_SUFFIX=s\n\
IMPORT_LIB_SUFFIX=\n\
TARGET_MD_ARCH=unix\n\
DIRENT_INO=d_ino\n\
CYGWIN_WRAPPER=\n\
WIN_TOP_SRC=\n\
MOZ_USER_DIR=\".mozilla\"\n\
HOST_AR=\'$(AR)\'\n\
HOST_AR_FLAGS=\'$(AR_FLAGS)\'\n\
\n\
MOZ_JPEG_CFLAGS=\n\
MOZ_JPEG_LIBS=\'$(call EXPAND_LIBNAME_PATH,mozjpeg,$(DEPTH)/jpeg)\'\n\
MOZ_ZLIB_CFLAGS=\n\
MOZ_ZLIB_LIBS=\'$(call EXPAND_LIBNAME_PATH,mozz,$(DEPTH)/modules/zlib/src)\'\n\
MOZ_PNG_CFLAGS=\n\
MOZ_PNG_LIBS=\'$(call EXPAND_LIBNAME_PATH,mozpng,$(DEPTH)/modules/libimg/png)\'\n\
\n\
MOZ_JS_LIBS=\'-L$(LIBXUL_DIST)/bin -lmozjs\'\n\
DYNAMIC_XPCOM_LIBS=\'-L$(LIBXUL_DIST)/bin -lxpcom -lxpcom_core\'\n\
MOZ_FIX_LINK_PATHS=\'-Wl,-rpath-link,$(LIBXUL_DIST)/bin\'\n\
XPCOM_FROZEN_LDOPTS=\'-L$(LIBXUL_DIST)/bin $(MOZ_FIX_LINK_PATHS) -lxpcom\'\n\
LIBXUL_LIBS=\'$(XPCOM_FROZEN_LDOPTS) -lxul\'\n\
XPCOM_GLUE_LDOPTS=\'$(LIBXUL_DIST)/lib/$(LIB_PREFIX)xpcomglue_s.$(LIB_SUFFIX) $(XPCOM_FROZEN_LDOPTS)\'\n\
XPCOM_STANDALONE_GLUE_LDOPTS=\'$(LIBXUL_DIST)/lib/$(LIB_PREFIX)xpcomglue.$(LIB_SUFFIX)\'\n\
\n\
MOZ_COMPONENT_NSPR_LIBS=\'-L$(LIBXUL_DIST)/bin $(NSPR_LIBS)\'\n\
MOZ_XPCOM_OBSOLETE_LIBS=\'-L$(LIBXUL_DIST)/lib -lxpcom_compat\'\n\
\n\
USE_DEPENDENT_LIBS=1\n\
\n\
_PLATFORM_DEFAULT_TOOLKIT=cairo-gtk2\n\
MOZ_GFX_TOOLKIT=\'$(MOZ_WIDGET_TOOLKIT)\'\n\
\n\
MOZ_ENABLE_POSTSCRIPT=1 \n\
\n\
if test -n \"$CROSS_COMPILE\"; then\n\
    OS_TARGET=\"${target_os}\"\n\
    OS_ARCH=`echo $target_os | sed -e \'s|/|_|g\'`\n\
    OS_RELEASE=\n\
    OS_TEST=\"${target_cpu}\"\n\
    case \"${target_os}\" in\n\
        linux*)       OS_ARCH=Linux ;;\n\
        solaris*)     OS_ARCH=SunOS OS_RELEASE=5 ;;\n\
        mingw*)       OS_ARCH=WINNT ;;\n\
        wince*)       OS_ARCH=WINCE ;;\n\
        darwin*)      OS_ARCH=Darwin OS_TARGET=Darwin ;;\n\
    esac\n\
else\n\
    OS_TARGET=`uname -s`\n\
    OS_ARCH=`uname -s | sed -e \'s|/|_|g\'`\n\
    OS_RELEASE=`uname -r`\n\
    OS_TEST=`uname -m`\n\
fi\n\
_COMPILER_PREFIX=\n\
\n\
HOST_OS_ARCH=`echo $host_os | sed -e \'s|/|_|g\'`\n\
\n\
#######################################################################\n\
# Master \"Core Components\" macros for getting the OS target           #\n\
#######################################################################\n\
\n\
#\n\
# Note: OS_TARGET should be specified on the command line for gmake.\n\
# When OS_TARGET=WIN95 is specified, then a Windows 95 target is built.\n\
# The difference between the Win95 target and the WinNT target is that\n\
# the WinNT target uses Windows NT specific features not available\n\
# in Windows 95. The Win95 target will run on Windows NT, but (supposedly)\n\
# at lesser performance (the Win95 target uses threads; the WinNT target\n\
# uses fibers).\n\
#\n\
# When OS_TARGET=WIN16 is specified, then a Windows 3.11 (16bit) target\n\
# is built. See: win16_3.11.mk for lots more about the Win16 target.\n\
#\n\
# If OS_TARGET is not specified, it defaults to $(OS_ARCH), i.e., no\n\
# cross-compilation.\n\
#\n\
\n\
#\n\
# The following hack allows one to build on a WIN95 machine (as if\n\
# s/he were cross-compiling on a WINNT host for a WIN95 target).\n\
# It also accomodates for MKS\'s uname.exe.  If you never intend\n\
# to do development on a WIN95 machine, you don\'t need this hack.\n\
#\n\
case \"$OS_ARCH\" in\n\
WIN95)\n\
    OS_ARCH=WINNT\n\
    OS_TARGET=WIN95\n\
    ;;\n\
Windows_95)\n\
    OS_ARCH=Windows_NT\n\
    OS_TARGET=WIN95\n\
    ;;\n\
Windows_98)\n\
    OS_ARCH=Windows_NT\n\
    OS_TARGET=WIN95\n\
    ;;\n\
CYGWIN_9*|CYGWIN_ME*)\n\
    OS_ARCH=\'CYGWIN_NT-4.0\'\n\
    OS_TARGET=WIN95\n\
    ;;\n\
esac\n\
\n\
#\n\
# Define and override various archtecture-specific variables, including\n\
# HOST_OS_ARCH\n\
# OS_ARCH\n\
# OS_TEST\n\
# OS_TARGET\n\
# OS_RELEASE\n\
# OS_MINOR_RELEASE\n\
#\n\
\n\
case \"$HOST_OS_ARCH\" in\n\
cygwin*|mingw*|mks*|msvc*)\n\
    HOST_OS_ARCH=WINNT\n\
    ;;\n\
linux*)\n\
    HOST_OS_ARCH=Linux\n\
    ;;\n\
solaris*)\n\
    HOST_OS_ARCH=SunOS\n\
    ;;\n\
BSD_386)\n\
    HOST_OS_ARCH=BSD\n\
    ;;\n\
dgux)\n\
    HOST_OS_ARCH=DGUX\n\
    ;;\n\
IRIX64)\n\
    HOST_OS_ARCH=IRIX\n\
    ;;\n\
UNIX_SV)\n\
    if \"`cat /etc/bcheckrc | grep -c NCR 2>/dev/null`\" != \"0\"; then\n\
        HOST_OS_ARCH=NCR\n\
    else\n\
        HOST_OS_ARCH=UNIXWARE\n\
    fi\n\
    ;;\n\
ncr)\n\
    HOST_OS_ARCH=NCR\n\
    ;;\n\
UNIX_SYSTEM_V)\n\
    HOST_OS_ARCH=NEC\n\
    ;;\n\
OSF1)\n\
    ;;\n\
*OpenVMS*)\n\
    HOST_OS_ARCH=OpenVMS\n\
    ;;\n\
OS_2)\n\
    HOST_OS_ARCH=OS2\n\
    ;;\n\
QNX)\n\
    ;;\n\
SCO_SV)\n\
    HOST_OS_ARCH=SCOOS\n\
    ;;\n\
SINIX-N | SINIX-Y | SINIX-Z |ReliantUNIX-M)\n\
    HOST_OS_ARCH=SINIX\n\
    ;;\n\
UnixWare)\n\
    HOST_OS_ARCH=UNIXWARE\n\
    ;;\n\
esac\n\
\n\
case \"$OS_ARCH\" in\n\
WINNT)\n\
    OS_TEST=`uname -p`\n\
    ;;\n\
Windows_NT)\n\
#\n\
# If uname -s returns \"Windows_NT\", we assume that we are using\n\
# the uname.exe in MKS toolkit.\n\
#\n\
# The -r option of MKS uname only returns the major version number.\n\
# So we need to use its -v option to get the minor version number.\n\
# Moreover, it doesn\'t have the -p option, so we need to use uname -m.\n\
#\n\
    OS_ARCH=WINNT\n\
    OS_TARGET=WINNT\n\
    OS_MINOR_RELEASE=`uname -v`\n\
    if test \"$OS_MINOR_RELEASE\" = \"00\"; then\n\
        OS_MINOR_RELEASE=0\n\
    fi\n\
    OS_RELEASE=\"${OS_RELEASE}.${OS_MINOR_RELEASE}\"\n\
    ;;\n\
CYGWIN32_NT|CYGWIN_NT*|MINGW*_NT*)\n\
#\n\
# If uname -s returns \"CYGWIN_NT-4.0\", we assume that we are using\n\
# the uname.exe in the Cygwin tools.\n\
# Prior to the Beta 20 release, Cygwin was called GNU-Win32.\n\
# If uname -s returns \"CYGWIN32/NT\", we assume that we are using\n\
# the uname.exe in the GNU-Win32 tools.\n\
# If uname -s returns MINGW32_NT-5.1, we assume that we are using\n\
# the uname.exe in the MSYS tools.\n\
#\n\
    OS_RELEASE=`expr $OS_ARCH : \'.*NT-\\(.*\\)\'`\n\
    OS_ARCH=WINNT\n\
    OS_TARGET=WINNT\n\
    ;;\n\
AIX)\n\
    OS_RELEASE=`uname -v`.`uname -r`\n\
    OS_TEST=`uname -p`\n\
    ;;\n\
BSD_386)\n\
    OS_ARCH=BSD\n\
    ;;\n\
dgux)\n\
    OS_ARCH=DGUX\n\
    ;;\n\
IRIX64)\n\
    OS_ARCH=IRIX\n\
    ;;\n\
UNIX_SV)\n\
    if \"`cat /etc/bcheckrc | grep -c NCR 2>/dev/null`\" != \"0\"; then\n\
        OS_ARCH=NCR\n\
    else\n\
        OS_ARCH=UNIXWARE\n\
        OS_RELEASE=`uname -v`\n\
    fi\n\
    ;;\n\
ncr)\n\
    OS_ARCH=NCR\n\
    ;;\n\
UNIX_SYSTEM_V)\n\
    OS_ARCH=NEC\n\
    ;;\n\
OSF1)\n\
    case `uname -v` in\n\
    148)\n\
        OS_RELEASE=V3.2C\n\
        ;;\n\
    564)\n\
        OS_RELEASE=V4.0B\n\
        ;;\n\
    878)\n\
        OS_RELEASE=V4.0D\n\
        ;;\n\
    esac\n\
    ;;\n\
*OpenVMS*)\n\
    OS_ARCH=OpenVMS\n\
    OS_RELEASE=`uname -v`\n\
    OS_TEST=`uname -p`\n\
    ;;\n\
OS_2)\n\
    OS_ARCH=OS2\n\
    OS_TARGET=OS2\n\
    OS_RELEASE=`uname -v`\n\
    ;;\n\
QNX)\n\
    if test \"$OS_TARGET\" != \"NTO\"; then\n\
        changequote(,)\n\
        OS_RELEASE=`uname -v | sed \'s/^\\([0-9]\\)\\([0-9]*\\)$/\\1.\\2/\'`\n\
        changequote([,])\n\
    fi\n\
    OS_TEST=x86\n\
    ;;\n\
SCO_SV)\n\
    OS_ARCH=SCOOS\n\
    OS_RELEASE=5.0\n\
    ;;\n\
SINIX-N | SINIX-Y | SINIX-Z |ReliantUNIX-M)\n\
    OS_ARCH=SINIX\n\
    OS_TEST=`uname -p`\n\
    ;;\n\
UnixWare)\n\
    OS_ARCH=UNIXWARE\n\
    OS_RELEASE=`uname -v`\n\
    ;;\n\
WINCE)\n\
    OS_ARCH=WINCE\n\
    OS_TARGET=WINCE\n\
    ;;\n\
Darwin)\n\
    case \"${target_cpu}\" in\n\
    powerpc*)\n\
        OS_TEST=ppc\n\
        ;;\n\
    i*86*)\n\
        OS_TEST=i386 \n\
        ;;\n\
    *)\n\
        if test -z \"$CROSS_COMPILE\" ; then\n\
            OS_TEST=`uname -p`\n\
        fi\n\
        ;;\n\
    esac\n\
    ;;\n\
esac\n\
\n\
if test \"$OS_ARCH\" = \"NCR\"; then\n\
    changequote(,)\n\
    OS_RELEASE=`awk \'{print $3}\' /etc/.relid | sed \'s/^\\([0-9]\\)\\(.\\)\\(..\\)\\(.*\\)$/\\2.\\3/\'`\n\
    changequote([,])\n\
fi\n\
\n\
# Only set CPU_ARCH if we recognize the value of OS_TEST\n\
\n\
case \"$OS_TEST\" in\n\
*86 | i86pc)\n\
    CPU_ARCH=x86\n\
    ;;\n\
\n\
powerpc* | ppc)\n\
    CPU_ARCH=ppc\n\
    ;;\n\
\n\
Alpha | alpha | ALPHA)\n\
    CPU_ARCH=Alpha\n\
    ;;\n\
\n\
sun4u)\n\
    CPU_ARCH=sparc\n\
    ;;\n\
\n\
x86_64 | sparc | ppc | ia64)\n\
    CPU_ARCH=\"$OS_TEST\"\n\
    ;;\n\
esac\n\
\n\
if test -z \"$OS_TARGET\"; then\n\
    OS_TARGET=$OS_ARCH\n\
fi\n\
if test \"$OS_TARGET\" = \"WIN95\"; then\n\
    OS_RELEASE=\"4.0\"\n\
fi\n\
if test \"$OS_TARGET\" = \"WIN16\"; then\n\
    OS_RELEASE=\n\
fi\n\
OS_CONFIG=\"${OS_TARGET}${OS_RELEASE}\"\n\
\n\
dnl ========================================================\n\
dnl GNU specific defaults\n\
dnl ========================================================\n\
if test \"$GNU_CC\"; then\n\
    MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -Wl,-h,$@ -o $@\'\n\
    MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -Wl,-h,$@ -o $@\'\n\
    DSO_LDOPTS=\'-shared\'\n\
    if test \"$GCC_USE_GNU_LD\"; then\n\
        # Don\'t allow undefined symbols in libraries\n\
        DSO_LDOPTS=\"$DSO_LDOPTS -Wl,-z,defs\"\n\
    fi\n\
    DSO_CFLAGS=\'\'\n\
    DSO_PIC_CFLAGS=\'-fPIC\'\n\
    _MOZ_RTTI_FLAGS_ON=${_COMPILER_PREFIX}-frtti\n\
    _MOZ_RTTI_FLAGS_OFF=${_COMPILER_PREFIX}-fno-rtti\n\
    _MOZ_EXCEPTIONS_FLAGS_ON=\'-fhandle-exceptions\'\n\
    _MOZ_EXCEPTIONS_FLAGS_OFF=\'-fno-handle-exceptions\'\n\
\n\
    # Turn on GNU specific features\n\
    # -Wall - turn on all warnings\n\
    # -pedantic - make compiler warn about non-ANSI stuff, and\n\
    #             be a little bit stricter\n\
    # Warnings slamm took out for now (these were giving more noise than help):\n\
    # -Wbad-function-cast - warns when casting a function to a new return type\n\
    # -Wconversion - complained when char\'s or short\'s were used a function args\n\
    # -Wshadow - removed because it generates more noise than help --pete\n\
    _WARNINGS_CFLAGS=\"${_WARNINGS_CFLAGS} -Wall -W -Wno-unused -Wpointer-arith -Wcast-align\"\n\
\n\
    dnl Turn pedantic on but disable the warnings for long long\n\
    _PEDANTIC=1\n\
    _IGNORE_LONG_LONG_WARNINGS=1\n\
\n\
    _DEFINES_CFLAGS=\'-include $(DEPTH)/mozilla-config.h -DMOZILLA_CLIENT\'\n\
    _USE_CPP_INCLUDE_FLAG=1\n\
else\n\
    MKSHLIB=\'$(LD) $(DSO_LDOPTS) -h $@ -o $@\'\n\
    MKCSHLIB=\'$(LD) $(DSO_LDOPTS) -h $@ -o $@\'\n\
\n\
    DSO_LDOPTS=\'-shared\'\n\
    if test \"$GNU_LD\"; then\n\
        # Don\'t allow undefined symbols in libraries\n\
        DSO_LDOPTS=\"$DSO_LDOPTS -z defs\"\n\
    fi\n\
\n\
    DSO_CFLAGS=\'\'\n\
    DSO_PIC_CFLAGS=\'-KPIC\'\n\
    _DEFINES_CFLAGS=\'$(ACDEFINES) -D_MOZILLA_CONFIG_H_ -DMOZILLA_CLIENT\'\n\
fi\n\
\n\
if test \"$GNU_CXX\"; then\n\
    # Turn on GNU specific features\n\
    _WARNINGS_CXXFLAGS=\"${_WARNINGS_CXXFLAGS} -Wall -Wconversion -Wpointer-arith -Wcast-align -Woverloaded-virtual -Wsynth -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor\"\n\
\n\
    _DEFINES_CXXFLAGS=\'-DMOZILLA_CLIENT -include $(DEPTH)/mozilla-config.h\'\n\
    _USE_CPP_INCLUDE_FLAG=1\n\
else\n\
    _DEFINES_CXXFLAGS=\'-DMOZILLA_CLIENT -D_MOZILLA_CONFIG_H_ $(ACDEFINES)\'\n\
fi\n\
\n\
dnl gcc can come with its own linker so it is better to use the pass-thru calls\n\
dnl MKSHLIB_FORCE_ALL is used to force the linker to include all object\n\
dnl files present in an archive. MKSHLIB_UNFORCE_ALL reverts the linker to\n\
dnl normal behavior.\n\
dnl ========================================================\n\
MKSHLIB_FORCE_ALL=\n\
MKSHLIB_UNFORCE_ALL=\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test \"$GNU_CC\"; then\n\
  AC_MSG_CHECKING(whether ld has archive extraction flags)\n\
  AC_CACHE_VAL(ac_cv_mkshlib_force_and_unforce,\n\
   [_SAVE_LDFLAGS=$LDFLAGS; _SAVE_LIBS=$LIBS\n\
    ac_cv_mkshlib_force_and_unforce=\"no\"\n\
    exec 3<&0 <<LOOP_INPUT\n\
	force=\"-Wl,--whole-archive\";   unforce=\"-Wl,--no-whole-archive\"\n\
	force=\"-Wl,-z -Wl,allextract\"; unforce=\"-Wl,-z -Wl,defaultextract\"\n\
	force=\"-Wl,-all\";              unforce=\"-Wl,-none\"\n\
LOOP_INPUT\n\
    while read line\n\
    do\n\
      eval $line\n\
      LDFLAGS=$force\n\
      LIBS=$unforce\n\
      AC_TRY_LINK(,, ac_cv_mkshlib_force_and_unforce=$line; break)\n\
    done\n\
    exec 0<&3 3<&-\n\
    LDFLAGS=$_SAVE_LDFLAGS; LIBS=$_SAVE_LIBS\n\
   ])\n\
  if test \"$ac_cv_mkshlib_force_and_unforce\" = \"no\"; then\n\
    AC_MSG_RESULT(no)\n\
  else\n\
    AC_MSG_RESULT(yes)\n\
    eval $ac_cv_mkshlib_force_and_unforce\n\
    MKSHLIB_FORCE_ALL=$force\n\
    MKSHLIB_UNFORCE_ALL=$unforce\n\
  fi\n\
fi # GNU_CC\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl =================================================================\n\
dnl Set up and test static assertion macros used to avoid AC_TRY_RUN,\n\
dnl which is bad when cross compiling.\n\
dnl =================================================================\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
configure_static_assert_macros=\'\n\
#define CONFIGURE_STATIC_ASSERT(condition) CONFIGURE_STATIC_ASSERT_IMPL(condition, __LINE__)\n\
#define CONFIGURE_STATIC_ASSERT_IMPL(condition, line) CONFIGURE_STATIC_ASSERT_IMPL2(condition, line)\n\
#define CONFIGURE_STATIC_ASSERT_IMPL2(condition, line) typedef int static_assert_line_##line[(condition) ? 1 : -1]\n\
\'\n\
\n\
dnl test that the macros actually work:\n\
AC_MSG_CHECKING(that static assertion macros used in autoconf tests work)\n\
AC_CACHE_VAL(ac_cv_static_assertion_macros_work,\n\
 [AC_LANG_SAVE\n\
  AC_LANG_C\n\
  ac_cv_static_assertion_macros_work=\"yes\"\n\
  AC_TRY_COMPILE([$configure_static_assert_macros],\n\
                 [CONFIGURE_STATIC_ASSERT(1)],\n\
                 ,\n\
                 ac_cv_static_assertion_macros_work=\"no\")\n\
  AC_TRY_COMPILE([$configure_static_assert_macros],\n\
                 [CONFIGURE_STATIC_ASSERT(0)],\n\
                 ac_cv_static_assertion_macros_work=\"no\",\n\
                 )\n\
  AC_LANG_CPLUSPLUS\n\
  AC_TRY_COMPILE([$configure_static_assert_macros],\n\
                 [CONFIGURE_STATIC_ASSERT(1)],\n\
                 ,\n\
                 ac_cv_static_assertion_macros_work=\"no\")\n\
  AC_TRY_COMPILE([$configure_static_assert_macros],\n\
                 [CONFIGURE_STATIC_ASSERT(0)],\n\
                 ac_cv_static_assertion_macros_work=\"no\",\n\
                 )\n\
  AC_LANG_RESTORE\n\
 ])\n\
AC_MSG_RESULT(\"$ac_cv_static_assertion_macros_work\")\n\
if test \"$ac_cv_static_assertion_macros_work\" = \"no\"; then\n\
    AC_MSG_ERROR([Compiler cannot compile macros used in autoconf tests.])\n\
fi\n\
\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl ========================================================\n\
dnl Checking for 64-bit OS\n\
dnl ========================================================\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
AC_LANG_SAVE\n\
AC_LANG_C\n\
AC_MSG_CHECKING(for 64-bit OS)\n\
AC_TRY_COMPILE([$configure_static_assert_macros],\n\
               [CONFIGURE_STATIC_ASSERT(sizeof(long) == 8)],\n\
               result=\"yes\", result=\"no\")\n\
AC_MSG_RESULT(\"$result\")\n\
if test \"$result\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_64BIT_OS)\n\
    HAVE_64BIT_OS=1\n\
fi\n\
AC_SUBST(HAVE_64BIT_OS)\n\
AC_LANG_RESTORE\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl ========================================================\n\
dnl Enable high-memory support on OS/2, disabled by default\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(os2-high-mem,\n\
[  --enable-os2-high-mem Enable high-memory support on OS/2],\n\
    MOZ_OS2_HIGH_MEMORY=1,\n\
    MOZ_OS2_HIGH_MEMORY= )\n\
AC_SUBST(MOZ_OS2_HIGH_MEMORY)\n\
\n\
dnl ========================================================\n\
dnl System overrides of the defaults for host\n\
dnl ========================================================\n\
case \"$host\" in\n\
*-beos*)\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_BEOS -DBeOS -DBEOS -D_POSIX_SOURCE -DNO_X11\"\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_beos.cfg\\\"\'\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O3}\"\n\
    ;;\n\
\n\
*cygwin*|*mingw*|*mks*|*msvc*|*wince)\n\
    if test -n \"$_WIN32_MSVC\"; then\n\
        HOST_AR=lib\n\
        HOST_AR_FLAGS=\'-NOLOGO -OUT:\"$@\"\'\n\
        HOST_CFLAGS=\"$HOST_CFLAGS -TC -nologo -Fd\\$(HOST_PDBFILE)\"\n\
        HOST_RANLIB=\'echo ranlib\'\n\
    else\n\
        HOST_CFLAGS=\"$HOST_CFLAGS -mno-cygwin\"\n\
    fi\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_WIN32 -DXP_WIN -DWIN32 -D_WIN32 -DNO_X11\"\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_winnt.cfg\\\"\'\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O2}\"\n\
    HOST_BIN_SUFFIX=.exe\n\
    case \"$host\" in\n\
    *mingw*)\n\
    dnl MinGW/MSYS does not need CYGWIN_WRAPPER\n\
        ;;\n\
    *)\n\
        CYGWIN_WRAPPER=\"${srcdir}/build/cygwin-wrapper\"\n\
        if test \"`echo ${srcdir} | grep -c ^/ 2>/dev/null`\" = 0; then\n\
            _pwd=`pwd`\n\
            CYGWIN_WRAPPER=\"${_pwd}/${srcdir}/build/cygwin-wrapper\"\n\
        fi\n\
        if test \"`${PERL} -v | grep -c cygwin  2>/dev/null`\" = 0; then\n\
            AS_PERL=1\n\
            PERL=\"${CYGWIN_WRAPPER} $PERL\"\n\
        fi\n\
        PYTHON=\"${CYGWIN_WRAPPER} $PYTHON\"\n\
        ;;\n\
    esac\n\
    ;;\n\
\n\
*-darwin*)\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_UNIX -DXP_MACOSX -DNO_X11\"\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_darwin.cfg\\\"\'\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O3}\"\n\
    MOZ_FIX_LINK_PATHS=\'-Wl,-executable_path,$(LIBXUL_DIST)/bin\'\n\
    LIBXUL_LIBS=\'$(XPCOM_FROZEN_LDOPTS) $(LIBXUL_DIST)/bin/XUL -lobjc\'\n\
    ;;\n\
\n\
*-linux*)\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_UNIX\"\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_linux.cfg\\\"\'\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O3}\"\n\
    ;;\n\
\n\
*os2*)\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_OS2 -DNO_X11 -Zomf\"\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_os2.cfg\\\"\'\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O2}\"\n\
    HOST_BIN_SUFFIX=.exe\n\
    MOZ_FIX_LINK_PATHS=\n\
    ;;\n\
\n\
*-osf*)\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_UNIX\"\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_osf1.cfg\\\"\'\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O2}\"\n\
    ;;\n\
\n\
*)\n\
    HOST_CFLAGS=\"$HOST_CFLAGS -DXP_UNIX\"\n\
    HOST_OPTIMIZE_FLAGS=\"${HOST_OPTIMIZE_FLAGS=-O2}\"\n\
    ;;\n\
esac\n\
\n\
dnl Get mozilla version from central milestone file\n\
MOZILLA_VERSION=`$PERL $srcdir/config/milestone.pl -topsrcdir $srcdir`\n\
\n\
dnl Get version of various core apps from the version files.\n\
FIREFOX_VERSION=`cat $topsrcdir/browser/config/version.txt`\n\
THUNDERBIRD_VERSION=`cat $topsrcdir/mail/config/version.txt`\n\
SUNBIRD_VERSION=`cat $topsrcdir/calendar/sunbird/config/version.txt`\n\
SEAMONKEY_VERSION=`cat $topsrcdir/suite/config/version.txt`\n\
\n\
AC_DEFINE_UNQUOTED(MOZILLA_VERSION,\"$MOZILLA_VERSION\")\n\
AC_DEFINE_UNQUOTED(MOZILLA_VERSION_U,$MOZILLA_VERSION)\n\
\n\
dnl ========================================================\n\
dnl System overrides of the defaults for target\n\
dnl ========================================================\n\
\n\
case \"$target\" in\n\
*-aix*)\n\
    AC_DEFINE(AIX)\n\
    if test ! \"$GNU_CC\"; then\n\
        if test ! \"$HAVE_64BIT_OS\"; then\n\
            # Compiling with Visual Age C++ object model compat is the\n\
            # default. To compile with object model ibm, add \n\
            # AIX_OBJMODEL=ibm to .mozconfig.\n\
            if test \"$AIX_OBJMODEL\" = \"ibm\"; then\n\
                CXXFLAGS=\"$CXXFLAGS -qobjmodel=ibm\"\n\
            else\n\
                AIX_OBJMODEL=compat\n\
            fi\n\
        else\n\
            AIX_OBJMODEL=compat\n\
        fi\n\
        AC_SUBST(AIX_OBJMODEL)\n\
        DSO_LDOPTS=\'-qmkshrobj=1\'\n\
        DSO_CFLAGS=\'-qflag=w:w\'\n\
        DSO_PIC_CFLAGS=\n\
        LDFLAGS=\"$LDFLAGS -Wl,-brtl -blibpath:/usr/lib:/lib\"\n\
        AC_MSG_WARN([Clearing MOZ_FIX_LINK_PATHS till we can fix bug 332075.])\n\
        MOZ_FIX_LINK_PATHS=\n\
        MKSHLIB=\'$(CXX) $(DSO_LDOPTS) -o $@\'\n\
        MKCSHLIB=\'$(CC) $(DSO_LDOPTS) -o $@\'\n\
        if test \"$COMPILE_ENVIRONMENT\"; then\n\
            AC_LANG_SAVE\n\
            AC_LANG_CPLUSPLUS\n\
            AC_MSG_CHECKING([for VisualAge C++ compiler version >= 5.0.2.0])\n\
            AC_TRY_COMPILE([],\n\
                [#if (__IBMCPP__ < 502)\n\
                 #error \"Bad compiler\"\n\
                 #endif],\n\
                _BAD_COMPILER=,_BAD_COMPILER=1)\n\
            if test -n \"$_BAD_COMPILER\"; then\n\
                AC_MSG_RESULT([no])    \n\
                AC_MSG_ERROR([VisualAge C++ version 5.0.2.0 or higher is required to build.])\n\
            else\n\
                AC_MSG_RESULT([yes])    \n\
            fi\n\
            AC_LANG_RESTORE\n\
            TARGET_COMPILER_ABI=\"ibmc\"\n\
            CC_VERSION=`lslpp -Lcq vac.C 2>/dev/null | awk -F: \'{ print $3 }\'`\n\
            CXX_VERSION=`lslpp -Lcq vacpp.cmp.core 2>/dev/null | awk -F: \'{ print $3 }\'`\n\
        fi\n\
    fi\n\
    case \"${target_os}\" in\n\
    aix4.1*)\n\
        DLL_SUFFIX=\'_shr.a\'\n\
        ;;\n\
    esac\n\
    if test \"$COMPILE_ENVIRONMENT\"; then\n\
        AC_CHECK_HEADERS(sys/inttypes.h)\n\
    fi\n\
    AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
    ;;\n\
\n\
*-beos*)\n\
    no_x=yes\n\
    MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_LDOPTS) -Wl,-h,$@ -o $@\'\n\
    _PLATFORM_DEFAULT_TOOLKIT=\"cairo-beos\"\n\
    DSO_LDOPTS=\'-nostart\'\n\
    TK_LIBS=\'-lbe -lroot\'\n\
    LIBS=\"$LIBS -lbe\"\n\
    if test \"$COMPILE_ENVIRONMENT\"; then\n\
        AC_CHECK_LIB(bind,main,LIBS=\"$LIBS -lbind\")\n\
        AC_CHECK_LIB(zeta,main,LIBS=\"$LIBS -lzeta\")\n\
    fi\n\
    _WARNINGS_CFLAGS=\"${_WARNINGS_CFLAGS} -Wno-multichar\"\n\
    _WARNINGS_CXXFLAGS=\"${_WARNINGS_CXXFLAGS} -Wno-multichar\"\n\
    _MOZ_USE_RTTI=1\n\
    USE_DEPENDENT_LIBS=\n\
    MOZ_USER_DIR=\"Mozilla\"\n\
    ;;\n\
\n\
*-bsdi*)\n\
    dnl -pedantic doesn\'t play well with BSDI\'s _very_ modified gcc (shlicc2)\n\
    _PEDANTIC=\n\
    _IGNORE_LONG_LONG_WARNINGS=\n\
    case $OS_RELEASE in\n\
	4.*|5.*)\n\
            STRIP=\"$STRIP -d\"\n\
            ;;\n\
	*)\n\
	    DSO_CFLAGS=\'\'\n\
	    DSO_LDOPTS=\'-r\'\n\
	    _WARNINGS_CFLAGS=\"-Wall\"\n\
	    _WARNINGS_CXXFLAGS=\"-Wall\"\n\
	    # The test above doesn\'t work properly, at least on 3.1.\n\
	    MKSHLIB_FORCE_ALL=\'\'\n\
	    MKSHLIB_UNFORCE_ALL=\'\'\n\
	;;\n\
    esac\n\
    ;;\n\
\n\
*-darwin*) \n\
    MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
    MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
\n\
    _PEDANTIC=\n\
    CFLAGS=\"$CFLAGS -fpascal-strings -no-cpp-precomp -fno-common\"\n\
    CXXFLAGS=\"$CXXFLAGS -fpascal-strings -no-cpp-precomp -fno-common\"\n\
    DLL_SUFFIX=\".dylib\"\n\
    DSO_LDOPTS=\'\'\n\
    STRIP=\"$STRIP -x -S\"\n\
    _PLATFORM_DEFAULT_TOOLKIT=\'cairo-cocoa\'\n\
    MOZ_ENABLE_POSTSCRIPT=\n\
    TARGET_NSPR_MDCPUCFG=\'\\\"md/_darwin.cfg\\\"\'\n\
    # set MACOSX to generate lib/mac/MoreFiles/Makefile\n\
    MACOSX=1\n\
\n\
    dnl check for the presence of the -dead_strip linker flag\n\
    AC_MSG_CHECKING([for -dead_strip option to ld])\n\
    _SAVE_LDFLAGS=$LDFLAGS\n\
    LDFLAGS=\"$LDFLAGS -Wl,-dead_strip\"\n\
    AC_TRY_LINK(,[return 0;],_HAVE_DEAD_STRIP=1,_HAVE_DEAD_STRIP=)\n\
    if test -n \"$_HAVE_DEAD_STRIP\" ; then\n\
        AC_MSG_RESULT([yes])\n\
        MOZ_OPTIMIZE_LDFLAGS=\"-Wl,-dead_strip\"\n\
    else\n\
        AC_MSG_RESULT([no])\n\
    fi\n\
    LDFLAGS=$_SAVE_LDFLAGS\n\
    ;;\n\
\n\
*-freebsd*)\n\
    if test `test -x /usr/bin/objformat && /usr/bin/objformat || echo aout` != \"elf\"; then\n\
	DLL_SUFFIX=\".so.1.0\"\n\
	DSO_LDOPTS=\"-shared\"\n\
    fi\n\
    if test ! \"$GNU_CC\"; then\n\
	DSO_LDOPTS=\"-Bshareable $DSO_LDOPTS\"\n\
    fi\n\
# Can\'t have force w/o an unforce.\n\
#    # Hack for FreeBSD 2.2\n\
#    if test -z \"$MKSHLIB_FORCE_ALL\"; then\n\
#	MKSHLIB_FORCE_ALL=\'-Wl,-Bforcearchive\'\n\
#	MKSHLIB_UNFORCE_ALL=\'\'\n\
#    fi\n\
    ;; \n\
\n\
*-hpux*)\n\
    DLL_SUFFIX=\".sl\"\n\
    if test ! \"$GNU_CC\"; then\n\
    	DSO_LDOPTS=\'-b -Wl,+s\'\n\
    	DSO_CFLAGS=\"\"\n\
    	DSO_PIC_CFLAGS=\"+Z\"\n\
    	MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_LDOPTS) -L$(LIBXUL_DIST)/bin -o $@\'\n\
    	MKCSHLIB=\'$(LD) -b +s -L$(LIBXUL_DIST)/bin -o $@\'\n\
        CXXFLAGS=\"$CXXFLAGS -Wc,-ansi_for_scope,on\"\n\
    else\n\
        DSO_LDOPTS=\'-b -E +s\'\n\
        MKSHLIB=\'$(LD) $(DSO_LDOPTS) -L$(LIBXUL_DIST)/bin -L$(LIBXUL_DIST)/lib -o $@\'\n\
        MKCSHLIB=\'$(LD) $(DSO_LDOPTS) -L$(LIBXUL_DIST)/bin -L$(LIBXUL_DIST)/lib -o $@\'\n\
    fi\n\
    MOZ_POST_PROGRAM_COMMAND=\'chatr +s enable\'\n\
    AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
    ;;\n\
\n\
*-irix5*)\n\
    AC_DEFINE(IRIX)\n\
    DSO_LDOPTS=\'-elf -shared\'\n\
\n\
    if test \"$GNU_CC\"; then\n\
       MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
       MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
       MKSHLIB_FORCE_ALL=\'-Wl,-all\'\n\
       MKSHLIB_UNFORCE_ALL=\'-Wl,-none\'\n\
       CXXFLAGS=\"$CXXFLAGS -D_LANGUAGE_C_PLUS_PLUS\"\n\
    else\n\
       MKSHLIB=\'$(LD) $(DSO_LDOPTS) -o $@\'\n\
       MKCSHLIB=\'$(LD) $(DSO_LDOPTS) -o $@\'\n\
       MKSHLIB_FORCE_ALL=\'-all\'\n\
       MKSHLIB_UNFORCE_ALL=\'-none\'\n\
    fi\n\
    ;;\n\
\n\
*-irix6*)\n\
    AC_DEFINE(IRIX)\n\
    dnl the irix specific xptcinvoke code is written against the n32 ABI so we *must* \n\
    dnl compile and link using -n32\n\
    USE_N32=1\n\
    TARGET_COMPILER_ABI=n32\n\
    DSO_LDOPTS=\'-elf -shared\'\n\
    MKSHLIB=\'$(CCC) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
    MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
    _MOZ_EXCEPTIONS_FLAGS_OFF=\"-LANG:exceptions=OFF\"\n\
    _MOZ_EXCEPTIONS_FLAGS_ON=\"-LANG:exceptions=ON\"\n\
    if test \"$GNU_CC\"; then\n\
       MKSHLIB_FORCE_ALL=\'-Wl,-all\'\n\
       MKSHLIB_UNFORCE_ALL=\'-Wl,-none\'\n\
       _WARNINGS_CFLAGS=\"-Wall\"\n\
       _WARNINGS_CXXFLAGS=\"-Wall\"\n\
       CXXFLAGS=\"$CXXFLAGS -D_LANGUAGE_C_PLUS_PLUS\"\n\
    else\n\
       MKSHLIB_FORCE_ALL=\'-all\'\n\
       MKSHLIB_UNFORCE_ALL=\'-none\'\n\
	   AR_LIST=\"$AR t\"\n\
	   AR_EXTRACT=\"$AR x\"\n\
	   AR_DELETE=\"$AR d\"\n\
	   AR=\'$(CXX) -ar\'\n\
	   AR_FLAGS=\'-o $@\'\n\
       CFLAGS=\"$CFLAGS -woff 3262 -G 4\"\n\
       CXXFLAGS=\"$CXXFLAGS -woff 3262 -G 4\"\n\
       if test -n \"$USE_N32\"; then\n\
	   ASFLAGS=\"$ASFLAGS -n32\"\n\
	   CFLAGS=\"$CFLAGS -n32\"\n\
	   CXXFLAGS=\"$CXXFLAGS -n32\"\n\
	   LDFLAGS=\"$LDFLAGS -n32\"\n\
       fi\n\
       AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
       AC_MSG_WARN([Clearing MOZ_FIX_LINK_PATHS for OSF/1 as fix for bug 333545 (till the reference bug 332075 is fixed.])\n\
       MOZ_FIX_LINK_PATHS=\n\
    fi\n\
    if test -z \"$GNU_CXX\"; then\n\
      MIPSPRO_CXX=1\n\
    fi\n\
    ;;\n\
\n\
*-*linux*)\n\
    TARGET_NSPR_MDCPUCFG=\'\\\"md/_linux.cfg\\\"\'\n\
    MOZ_DEBUG_FLAGS=\"-g -fno-inline\"  # most people on linux use gcc/gdb,\n\
                                      # and that combo is not yet good at\n\
                                      # debugging inlined functions (even\n\
                                      # when using DWARF2 as the debugging\n\
                                      # format)    \n\
\n\
    case \"${target_cpu}\" in\n\
    alpha*)\n\
    	CFLAGS=\"$CFLAGS -mieee\"\n\
    	CXXFLAGS=\"$CXXFLAGS -mieee\"\n\
    ;;\n\
    i*86)\n\
    	USE_ELF_DYNSTR_GC=1\n\
        MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS=1\n\
    ;;\n\
    mips*)\n\
        CFLAGS=\"$CFLAGS -Wa,-xgot\"\n\
        CXXFLAGS=\"$CXXFLAGS -Wa,-xgot\"\n\
    ;;\n\
    esac\n\
    ;;\n\
\n\
*-wince*)\n\
\n\
    MOZ_TOOLS_DIR=`echo $MOZ_TOOLS`\n\
    AR_LIST=\"$AR -list\"\n\
    AR_EXTRACT=\"$AR -extract\"\n\
    AR_DELETE=\"$AR d\"\n\
    AR_FLAGS=\'-OUT:\"$@\"\'\n\
\n\
    DSO_CFLAGS=\n\
    DSO_PIC_CFLAGS=\n\
    DLL_SUFFIX=.dll\n\
    BIN_SUFFIX=\'.exe\'\n\
    RC=rc.exe\n\
    # certain versions of cygwin\'s makedepend barf on the \n\
    # #include <string> vs -I./dist/include/string issue so don\'t use it\n\
    SYSTEM_MAKEDEPEND=\n\
\n\
    HOST_CC=cl\n\
    HOST_CXX=cl\n\
    HOST_LD=link\n\
    HOST_AR=\'lib -OUT:$@\'\n\
    HOST_RANLIB=\'echo ranlib\'\n\
        \n\
	MOZ_OPTIMIZE_FLAGS=\'-O1\'\n\
    AR_FLAGS=\'-NOLOGO -OUT:\"$@\"\'\n\
    ASM_SUFFIX=asm\n\
    CFLAGS=\"$CFLAGS -W3 -Gy -Fd\\$(PDBFILE)\"\n\
    CXXFLAGS=\"$CXXFLAGS -W3 -Gy -Fd\\$(PDBFILE)\"\n\
    DLL_PREFIX=\n\
    DOXYGEN=:\n\
    DSO_LDOPTS=-SUBSYSTEM:WINDOWSCE\n\
    DYNAMIC_XPCOM_LIBS=\'$(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xpcom_core.lib\'\n\
    GARBAGE=\n\
    IMPORT_LIB_SUFFIX=lib\n\
    LIBS=\"$LIBS\"\n\
    LIBXUL_LIBS=\'$(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xul.lib\'\n\
    LIB_PREFIX=\n\
    LIB_SUFFIX=lib \n\
    MKCSHLIB=\'$(LD) -NOLOGO -DLL -OUT:$@ $(DSO_LDOPTS)\'\n\
    MKSHLIB=\'$(LD) -NOLOGO -DLL -OUT:$@ $(DSO_LDOPTS)\'\n\
    MKSHLIB_FORCE_ALL=\n\
    MKSHLIB_UNFORCE_ALL=\n\
    MOZ_COMPONENT_NSPR_LIBS=\'$(NSPR_LIBS)\'\n\
    MOZ_COMPONENT_NSPR_LIBS=\'$(NSPR_LIBS)\'\n\
    MOZ_DEBUG_FLAGS=\'-Zi\'\n\
    MOZ_DEBUG_LDFLAGS=\'-DEBUG -DEBUGTYPE:CV\'\n\
    MOZ_FIX_LINK_PATHS=\n\
    MOZ_JS_LIBS=\'$(LIBXUL_DIST)/lib/js$(MOZ_BITS)$(VERSION_NUMBER).lib\'\n\
    MOZ_OPTIMIZE_FLAGS=\'-O1\'\n\
    MOZ_XPCOM_OBSOLETE_LIBS=\'$(LIBXUL_DIST)/lib/xpcom_compat.lib\'\n\
    OBJ_SUFFIX=obj\n\
    RANLIB=\'echo not_ranlib\'\n\
    STRIP=\'echo not_strip\'\n\
    TARGET_NSPR_MDCPUCFG=\'\\\"md/_wince.cfg\\\"\'\n\
    UNZIP=unzip\n\
    XARGS=xargs\n\
    XPCOM_FROZEN_LDOPTS=\'$(LIBXUL_DIST)/lib/xpcom.lib\'\n\
    ZIP=zip\n\
    LIBIDL_CFLAGS=\"-I$MOZ_TOOLS_DIR/include ${GLIB_CFLAGS}\"\n\
    LIBIDL_LIBS=\"$MOZ_TOOLS_DIR/lib/libidl-0.6_s.lib $MOZ_TOOLS_DIR/lib/glib-1.2_s.lib\"\n\
    STATIC_LIBIDL=1\n\
\n\
    AC_DEFINE(HAVE_SNPRINTF)\n\
    AC_DEFINE(_WINDOWS)\n\
    AC_DEFINE(_WIN32)\n\
    AC_DEFINE(WIN32)\n\
    AC_DEFINE(XP_WIN)\n\
    AC_DEFINE(XP_WIN32)\n\
    AC_DEFINE(HW_THREADS)\n\
    AC_DEFINE(STDC_HEADERS)\n\
    AC_DEFINE(NEW_H, <new>)\n\
    AC_DEFINE(WIN32_LEAN_AND_MEAN)\n\
\n\
    TARGET_MD_ARCH=win32\n\
    _PLATFORM_DEFAULT_TOOLKIT=\'windows\'\n\
    BIN_SUFFIX=\'.exe\'\n\
    USE_SHORT_LIBNAME=1\n\
    MOZ_ENABLE_COREXFONTS=\n\
    MOZ_ENABLE_POSTSCRIPT=\n\
    MOZ_USER_DIR=\"Mozilla\"\n\
;;\n\
\n\
\n\
*-mingw*|*-cygwin*|*-msvc*|*-mks*)\n\
    DSO_CFLAGS=\n\
    DSO_PIC_CFLAGS=\n\
    DLL_SUFFIX=.dll\n\
    RC=rc.exe\n\
    # certain versions of cygwin\'s makedepend barf on the \n\
    # #include <string> vs -I./dist/include/string issue so don\'t use it\n\
    SYSTEM_MAKEDEPEND=\n\
    if test -n \"$GNU_CC\"; then\n\
        CC=\"$CC -mno-cygwin\"\n\
        CXX=\"$CXX -mno-cygwin\"\n\
        CPP=\"$CPP -mno-cygwin\"\n\
        CFLAGS=\"$CFLAGS -mms-bitfields\"\n\
        CXXFLAGS=\"$CXXFLAGS -mms-bitfields\"\n\
        DSO_LDOPTS=\'-shared\'\n\
        MKSHLIB=\'$(CXX) $(DSO_LDOPTS) -o $@\'\n\
        MKCSHLIB=\'$(CC) $(DSO_LDOPTS) -o $@\'\n\
        RC=\'$(WINDRES)\'\n\
        # Use temp file for windres (bug 213281)\n\
        RCFLAGS=\'-O coff --use-temp-file\'\n\
        # mingw doesn\'t require kernel32, user32, and advapi32 explicitly\n\
        LIBS=\"$LIBS -lgdi32 -lwinmm -lwsock32\"\n\
        MOZ_JS_LIBS=\'-L$(LIBXUL_DIST)/lib -ljs$(MOZ_BITS)$(VERSION_NUMBER)\'\n\
        MOZ_FIX_LINK_PATHS=\n\
        DYNAMIC_XPCOM_LIBS=\'-L$(LIBXUL_DIST)/lib -lxpcom -lxpcom_core\'\n\
        XPCOM_FROZEN_LDOPTS=\'-L$(LIBXUL_DIST)/lib -lxpcom\'\n\
        DLL_PREFIX=\n\
        IMPORT_LIB_SUFFIX=dll.a\n\
    else\n\
        TARGET_COMPILER_ABI=msvc\n\
        HOST_CC=\'$(CC)\'\n\
        HOST_CXX=\'$(CXX)\'\n\
        HOST_LD=\'$(LD)\'\n\
        AR=\'lib -NOLOGO -OUT:\"$@\"\'\n\
        AR_FLAGS=\n\
        RANLIB=\'echo not_ranlib\'\n\
        STRIP=\'echo not_strip\'\n\
        XARGS=xargs\n\
        ZIP=zip\n\
        UNZIP=unzip\n\
        DOXYGEN=:\n\
        GARBAGE=\'$(OBJDIR)/vc20.pdb $(OBJDIR)/vc40.pdb\'\n\
        OBJ_SUFFIX=obj\n\
        LIB_SUFFIX=lib\n\
        DLL_PREFIX=\n\
        LIB_PREFIX=\n\
        IMPORT_LIB_SUFFIX=lib\n\
        MKSHLIB=\'$(LD) -NOLOGO -DLL -OUT:$@ -PDB:$(PDBFILE) $(DSO_LDOPTS)\'\n\
        MKCSHLIB=\'$(LD) -NOLOGO -DLL -OUT:$@ -PDB:$(PDBFILE) $(DSO_LDOPTS)\'\n\
        MKSHLIB_FORCE_ALL=\n\
        MKSHLIB_UNFORCE_ALL=\n\
        DSO_LDOPTS=-SUBSYSTEM:WINDOWS\n\
        CFLAGS=\"$CFLAGS -W3 -Gy -Fd\\$(PDBFILE)\"\n\
        CXXFLAGS=\"$CXXFLAGS -W3 -Gy -Fd\\$(PDBFILE)\"\n\
        LIBS=\"$LIBS kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib advapi32.lib\"\n\
        MOZ_DEBUG_FLAGS=\'-Zi\'\n\
        MOZ_DEBUG_LDFLAGS=\'-DEBUG -DEBUGTYPE:CV\'\n\
    	MOZ_OPTIMIZE_FLAGS=\'-O1\'\n\
        MOZ_JS_LIBS=\'$(LIBXUL_DIST)/lib/js$(MOZ_BITS)$(VERSION_NUMBER).lib\'\n\
        MOZ_FIX_LINK_PATHS=\n\
        DYNAMIC_XPCOM_LIBS=\'$(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xpcom_core.lib\'\n\
        XPCOM_FROZEN_LDOPTS=\'$(LIBXUL_DIST)/lib/xpcom.lib\'\n\
        LIBXUL_LIBS=\'$(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xul.lib\'\n\
        MOZ_COMPONENT_NSPR_LIBS=\'$(NSPR_LIBS)\'\n\
        MOZ_XPCOM_OBSOLETE_LIBS=\'$(LIBXUL_DIST)/lib/xpcom_compat.lib\'\n\
    fi\n\
    MOZ_JPEG_LIBS=\'$(call EXPAND_LIBNAME_PATH,jpeg$(MOZ_BITS)$(VERSION_NUMBER),$(DEPTH)/jpeg)\'\n\
    MOZ_PNG_LIBS=\'$(call EXPAND_LIBNAME_PATH,png,$(DEPTH)/modules/libimg/png)\'\n\
    AC_DEFINE(HAVE_SNPRINTF)\n\
    AC_DEFINE(_WINDOWS)\n\
    AC_DEFINE(_WIN32)\n\
    AC_DEFINE(WIN32)\n\
    AC_DEFINE(XP_WIN)\n\
    AC_DEFINE(XP_WIN32)\n\
    AC_DEFINE(HW_THREADS)\n\
    AC_DEFINE(STDC_HEADERS)\n\
    AC_DEFINE(NEW_H, <new>)\n\
    AC_DEFINE(WIN32_LEAN_AND_MEAN)\n\
    TARGET_MD_ARCH=win32\n\
    _PLATFORM_DEFAULT_TOOLKIT=\'cairo-windows\'\n\
    BIN_SUFFIX=\'.exe\'\n\
    USE_SHORT_LIBNAME=1\n\
    MOZ_ENABLE_POSTSCRIPT=\n\
    MOZ_USER_DIR=\"Mozilla\"\n\
\n\
    dnl Hardcode to win95 for now - cls\n\
    TARGET_NSPR_MDCPUCFG=\'\\\"md/_win95.cfg\\\"\'\n\
\n\
    dnl set NO_X11 defines here as the general check is skipped on win32\n\
    no_x=yes\n\
    AC_DEFINE(NO_X11)\n\
\n\
    dnl MinGW/MSYS doesn\'t provide or need cygpath\n\
    case \"$host\" in\n\
    *-mingw*)\n\
	CYGPATH_W=echo\n\
	CYGPATH_S=cat\n\
	MOZ_BUILD_ROOT=`cd $MOZ_BUILD_ROOT && pwd -W`\n\
	;;\n\
    *-cygwin*|*-msvc*|*-mks*)\n\
	CYGPATH_W=\"cygpath -a -w\"\n\
	CYGPATH_S=\"sed -e s|\\\\\\\\|/|g\"\n\
	MOZ_BUILD_ROOT=`$CYGPATH_W $MOZ_BUILD_ROOT | $CYGPATH_S`\n\
	;;\n\
    esac\n\
    case \"$host\" in\n\
    *-mingw*|*-cygwin*|*-msvc*|*-mks*)\n\
\n\
    if test -z \"$MOZ_TOOLS\"; then\n\
        AC_MSG_ERROR([MOZ_TOOLS is not set])\n\
    fi\n\
\n\
    MOZ_TOOLS_DIR=`cd $MOZ_TOOLS && pwd`\n\
    if test \"$?\" != \"0\" || test -z \"$MOZ_TOOLS_DIR\"; then\n\
        AC_MSG_ERROR([cd \\$MOZ_TOOLS failed. MOZ_TOOLS ==? $MOZ_TOOLS])\n\
    fi\n\
    if test `echo ${PATH}: | grep -ic \"$MOZ_TOOLS_DIR/bin:\"` = 0; then\n\
        AC_MSG_ERROR([\\$MOZ_TOOLS\\\\bin must be in your path.])\n\
    fi\n\
    MOZ_TOOLS_DIR=`$CYGPATH_W $MOZ_TOOLS_DIR | $CYGPATH_S`\n\
\n\
    if test -n \"$GLIB_PREFIX\"; then\n\
        _GLIB_PREFIX_DIR=`cd $GLIB_PREFIX && pwd`\n\
        if test \"$?\" = \"0\"; then\n\
            if test `echo ${PATH}: | grep -ic \"$_GLIB_PREFIX_DIR/bin:\"` = 0; then\n\
                AC_MSG_ERROR([GLIB_PREFIX must be in your \\$PATH.])\n\
            fi\n\
            _GLIB_PREFIX_DIR=`$CYGPATH_W $_GLIB_PREFIX_DIR | $CYGPATH_S`\n\
        else\n\
            AC_MSG_ERROR([GLIB_PREFIX is set but \"${GLIB_PREFIX}\" is not a directory.])\n\
        fi\n\
    else\n\
        _GLIB_PREFIX_DIR=$MOZ_TOOLS_DIR\n\
    fi\n\
    if test ! -f \"${_GLIB_PREFIX_DIR}/include/glib.h\"; then\n\
        AC_MSG_ERROR([Cannot find $_GLIB_PREFIX_DIR/include/glib.h .])\n\
    fi\n\
    GLIB_CFLAGS=\"-I${_GLIB_PREFIX_DIR}/include\"\n\
    if test -f \"${_GLIB_PREFIX_DIR}/lib/glib-1.2_s.lib\"; then\n\
        GLIB_LIBS=\"${_GLIB_PREFIX_DIR}/lib/glib-1.2_s.lib\"\n\
    elif test -f \"${_GLIB_PREFIX_DIR}/lib/glib-1.2.lib\"; then\n\
        GLIB_LIBS=\"${_GLIB_PREFIX_DIR}/lib/glib-1.2.lib\"\n\
    else\n\
        AC_MSG_ERROR([Cannot find $_GLIB_PREFIX_DIR/lib/glib-1.2.lib or $_GLIB_PREFIX_DIR/lib/glib-1.2_s.lib])\n\
    fi\n\
\n\
    if test -n \"$LIBIDL_PREFIX\"; then\n\
        _LIBIDL_PREFIX_DIR=`cd $LIBIDL_PREFIX && pwd`\n\
        if test \"$?\" = \"0\"; then\n\
            if test `echo ${PATH}: | grep -ic \"$_LIBIDL_PREFIX_DIR/bin:\"` = 0; then\n\
                AC_MSG_ERROR([LIBIDL_PREFIX must be in your \\$PATH.])\n\
            fi\n\
            _LIBIDL_PREFIX_DIR=`$CYGPATH_W $_LIBIDL_PREFIX_DIR | $CYGPATH_S`\n\
        else\n\
            AC_MSG_ERROR([LIBIDL_PREFIX is set but \"${LIBIDL_PREFIX}\" is not a directory.])\n\
        fi\n\
    else\n\
        _LIBIDL_PREFIX_DIR=$MOZ_TOOLS_DIR\n\
    fi        \n\
    if test ! -f \"${_LIBIDL_PREFIX_DIR}/include/libIDL/IDL.h\"; then\n\
        AC_MSG_ERROR([Cannot find $_LIBIDL_PREFIX_DIR/include/libIDL/IDL.h .])\n\
    fi\n\
    LIBIDL_CFLAGS=\"-I${_LIBIDL_PREFIX_DIR}/include ${GLIB_CFLAGS}\"\n\
    if test -f \"${_LIBIDL_PREFIX_DIR}/lib/libidl-0.6_s.lib\"; then\n\
        LIBIDL_LIBS=\"${_LIBIDL_PREFIX_DIR}/lib/libidl-0.6_s.lib\"\n\
        STATIC_LIBIDL=1\n\
    elif test -f \"${_LIBIDL_PREFIX_DIR}/lib/libidl-0.6.lib\"; then\n\
        LIBIDL_LIBS=\"${_LIBIDL_PREFIX_DIR}/lib/libidl-0.6.lib\"\n\
    else\n\
        AC_MSG_ERROR([Cannot find $_LIBIDL_PREFIX_DIR/lib/libidl-0.6.lib or $_LIBIDL_PREFIX_DIR/lib/libidl-0.6_s.lib])\n\
    fi\n\
    LIBIDL_LIBS=\"${LIBIDL_LIBS} ${GLIB_LIBS}\"\n\
    ;;\n\
\n\
    *) # else cross-compiling\n\
        if test -n \"$GLIB_PREFIX\"; then\n\
            GLIB_CFLAGS=\"-I${GLIB_PREFIX}/include\"\n\
            if test -f \"${GLIB_PREFIX}/lib/glib-1.2_s.lib\"; then\n\
                GLIB_LIBS=\"${GLIB_PREFIX}/lib/glib-1.2_s.lib\"\n\
            elif test -f \"${GLIB_PREFIX}/lib/glib-1.2.lib\"; then\n\
                GLIB_LIBS=\"${GLIB_PREFIX}/lib/glib-1.2.lib\"\n\
            else\n\
                AC_MSG_ERROR([Cannot find $GLIB_PREFIX/lib/glib-1.2.lib or $GLIB_PREFIX/lib/glib-1.2_s.lib])\n\
            fi\n\
        fi\n\
        if test -n \"$LIBIDL_PREFIX\"; then\n\
            LIBIDL_CFLAGS=\"-I${LIBIDL_PREFIX}/include ${GLIB_CFLAGS}\"\n\
            if test -f \"${LIBIDL_PREFIX}/lib/libIDL-0.6_s.lib\"; then\n\
                LIBIDL_LIBS=\"${LIBIDL_PREFIX}/lib/libIDL-0.6_s.lib\"\n\
                STATIC_LIBIDL=1\n\
            elif test -f \"${LIBIDL_PREFIX}/lib/libIDL-0.6.lib\"; then\n\
                LIBIDL_LIBS=\"${LIBIDL_PREFIX}/lib/libIDL-0.6.lib\"\n\
            else\n\
                AC_MSG_ERROR([Cannot find $LIBIDL_PREFIX/lib/libIDL-0.6.lib or $LIBIDL_PREFIX/lib/libIDL-0.6_s.lib])\n\
            fi\n\
        fi\n\
        LIBIDL_LIBS=\"${LIBIDL_LIBS} ${GLIB_LIBS}\"\n\
        ;;\n\
    esac \n\
\n\
    case \"$target\" in\n\
    i*86-*)\n\
    	AC_DEFINE(_X86_)\n\
	;;\n\
    alpha-*)\n\
    	AC_DEFINE(_ALPHA_)\n\
	;;\n\
    mips-*)\n\
    	AC_DEFINE(_MIPS_)\n\
	;;\n\
    *)\n\
    	AC_DEFINE(_CPU_ARCH_NOT_DEFINED)\n\
	;;\n\
    esac\n\
    ;;\n\
\n\
*-netbsd*)\n\
    DSO_CFLAGS=\'\'\n\
    CFLAGS=\"$CFLAGS -Dunix\"\n\
    CXXFLAGS=\"$CXXFLAGS -Dunix\"\n\
    if $CC -E - -dM </dev/null | grep __ELF__ >/dev/null; then\n\
        DLL_SUFFIX=\".so\"\n\
        DSO_PIC_CFLAGS=\'-fPIC -DPIC\'\n\
        DSO_LDOPTS=\'-shared\'\n\
	BIN_FLAGS=\'-Wl,--export-dynamic\'\n\
    else\n\
    	DSO_PIC_CFLAGS=\'-fPIC -DPIC\'\n\
    	DLL_SUFFIX=\".so.1.0\"\n\
    	DSO_LDOPTS=\'-shared\'\n\
    fi\n\
    # This will fail on a.out systems prior to 1.5.1_ALPHA.\n\
    MKSHLIB_FORCE_ALL=\'-Wl,--whole-archive\'\n\
    MKSHLIB_UNFORCE_ALL=\'-Wl,--no-whole-archive\'\n\
    if test \"$LIBRUNPATH\"; then\n\
	DSO_LDOPTS=\"-Wl,-R$LIBRUNPATH $DSO_LDOPTS\"\n\
    fi\n\
    MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -Wl,-soname,lib$(LIBRARY_NAME)$(DLL_SUFFIX) -o $@\'\n\
    MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -Wl,-soname,lib$(LIBRARY_NAME)$(DLL_SUFFIX) -o $@\'\n\
    ;;\n\
\n\
*-nto*) \n\
	AC_DEFINE(NTO)	\n\
	AC_DEFINE(_QNX_SOURCE)\n\
	AC_DEFINE(_i386)\n\
	OS_TARGET=NTO\n\
	MOZ_OPTIMIZE_FLAGS=\"-O\"\n\
	MOZ_DEBUG_FLAGS=\"-gstabs\"\n\
	USE_PTHREADS=1\n\
	_PEDANTIC=\n\
	LIBS=\"$LIBS -lsocket -lstdc++\"\n\
	_DEFINES_CFLAGS=\'-Wp,-include -Wp,$(DEPTH)/mozilla-config.h -DMOZILLA_CLIENT -D_POSIX_C_SOURCE=199506\'\n\
	_DEFINES_CXXFLAGS=\'-DMOZILLA_CLIENT -Wp,-include -Wp,$(DEPTH)/mozilla-config.h -D_POSIX_C_SOURCE=199506\'\n\
	if test \"$with_x\" != \"yes\"\n\
	then\n\
		_PLATFORM_DEFAULT_TOOLKIT=\"photon\"\n\
	    TK_CFLAGS=\'-I/usr/include/photon\'\n\
		TK_LIBS=\'-lph\'\n\
	fi\n\
	case \"${target_cpu}\" in\n\
	ppc*)\n\
	AC_DEFINE(HAVE_VA_LIST_AS_ARRAY)	\n\
	;;\n\
	esac\n\
	case \"${host_cpu}\" in\n\
	i*86)\n\
	USE_ELF_DYNSTR_GC=1\n\
	;;\n\
	esac\n\
	;;\n\
\n\
*-openbsd*)\n\
    DLL_SUFFIX=\".so.1.0\"\n\
    DSO_CFLAGS=\'\'\n\
    DSO_PIC_CFLAGS=\'-fPIC\'\n\
    DSO_LDOPTS=\'-shared -fPIC\'\n\
    if test \"$LIBRUNPATH\"; then\n\
	DSO_LDOPTS=\"-R$LIBRUNPATH $DSO_LDOPTS\"\n\
    fi\n\
    ;;\n\
\n\
*-openvms*) \n\
    AC_DEFINE(NO_PW_GECOS)\n\
    AC_DEFINE(NO_UDSOCK)\n\
    AC_DEFINE(POLL_WITH_XCONNECTIONNUMBER)\n\
    USE_PTHREADS=1\n\
    MKSHLIB_FORCE_ALL=\'-all\'\n\
    MKSHLIB_UNFORCE_ALL=\'-none\'\n\
    AS=\'as\'\n\
    AS_DASH_C_FLAG=\'-Wc/names=as_is\'\n\
    AR_FLAGS=\'c $@\'\n\
    DSO_LDOPTS=\'-shared -auto_symvec\'\n\
    DSO_PIC_CFLAGS=\n\
    MOZ_DEBUG_LDFLAGS=\'-g\'\n\
    COMPAQ_CXX=1\n\
    CC_VERSION=`$CC -V 2>&1 | awk \'/ C / { print $3 }\'`\n\
    CXX_VERSION=`$CXX -V 2>&1 | awk \'/ C\\+\\+ / { print $3 }\'`\n\
    ;;\n\
\n\
\n\
*-os2*)\n\
    MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
    MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -o $@\'\n\
    AC_DEFINE(XP_OS2)\n\
    USE_SHORT_LIBNAME=1\n\
    DLL_PREFIX=\n\
    LIB_PREFIX=\n\
    LIB_SUFFIX=lib\n\
    BIN_SUFFIX=\".exe\"\n\
    DLL_SUFFIX=\".dll\"\n\
    IMPORT_LIB_SUFFIX=lib\n\
    DSO_PIC_CFLAGS=\n\
    TARGET_MD_ARCH=os2\n\
    _PLATFORM_DEFAULT_TOOLKIT=os2\n\
    MOZ_ENABLE_POSTSCRIPT=\n\
    RC=rc.exe\n\
    RCFLAGS=\'-n\'\n\
    MOZ_USER_DIR=\"Mozilla\"\n\
\n\
    if test \"$MOZTOOLS\"; then\n\
        MOZ_TOOLS_DIR=`echo $MOZTOOLS | sed -e \'s|\\\\\\\\|/|g\'`\n\
    else\n\
        AC_MSG_ERROR([MOZTOOLS is not set])\n\
    fi\n\
\n\
    # EMX/GCC build\n\
    if test -n \"$GNU_CC\"; then\n\
        AC_DEFINE(OS2)\n\
        AC_DEFINE(XP_OS2_EMX)\n\
        AC_DEFINE(OS2EMX_PLAIN_CHAR)\n\
        AC_DEFINE(TCPV40HDRS)\n\
        AR=emxomfar\n\
        AR_FLAGS=\'r $@\'\n\
        CFLAGS=\"$CFLAGS -Zomf\"\n\
        CXXFLAGS=\"$CXXFLAGS -Zomf\"\n\
        DSO_LDOPTS=\'-Zdll\'\n\
        BIN_FLAGS=\'-Zlinker /ST:0x100000\'\n\
        IMPLIB=\'emximp -o\'\n\
        FILTER=\'emxexp -o\'\n\
        LDFLAGS=\'-Zmap\'\n\
        MOZ_DEBUG_FLAGS=\"-g -fno-inline\"\n\
        MOZ_OPTIMIZE_FLAGS=\"-O2 -s\"\n\
        MOZ_OPTIMIZE_LDFLAGS=\"-Zlinker /EXEPACK:2 -Zlinker /PACKCODE -Zlinker /PACKDATA\"\n\
        MOZ_XPCOM_OBSOLETE_LIBS=\'-L$(LIBXUL_DIST)/lib $(LIBXUL_DIST)/lib/xpcomct.lib\'\n\
        DYNAMIC_XPCOM_LIBS=\'-L$(LIBXUL_DIST)/lib $(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xpcomcor.lib\'\n\
        LIBXUL_LIBS=\'-L$(LIBXUL_DIST)/lib $(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xul.lib\'\n\
        if test -n \"$MOZ_OS2_HIGH_MEMORY\"; then\n\
          DSO_LDOPTS=\"$DSO_LDOPTS -Zhigh-mem\"\n\
          LDFLAGS=\"$LDFLAGS -Zhigh-mem\"\n\
          MOZ_OPTIMIZE_LDFLAGS=\"$MOZ_OPTIMIZE_LDFLAGS -Zhigh-mem\"\n\
          AC_DEFINE(MOZ_OS2_HIGH_MEMORY)\n\
        fi\n\
\n\
        # GCC for OS/2 currently predefines these, but we don\'t want them\n\
        _DEFINES_CFLAGS=\"$_DEFINES_CFLAGS -Uunix -U__unix -U__unix__\"\n\
        _DEFINES_CXXFLAGS=\"$_DEFINES_CXXFLAGS -Uunix -U__unix -U__unix__\"\n\
\n\
        AC_CACHE_CHECK(for __declspec(dllexport),\n\
           ac_os2_declspec,\n\
           [AC_TRY_COMPILE([__declspec(dllexport) void ac_os2_declspec(void) {}],\n\
                           [return 0;],\n\
                           ac_os2_declspec=\"yes\",\n\
                           ac_os2_declspec=\"no\")])\n\
        if test \"$ac_os2_declspec\" = \"yes\"; then\n\
           FILTER=\'true\'\n\
           MOZ_OS2_USE_DECLSPEC=\'1\'\n\
        fi\n\
        \n\
    # Visual Age C++ build\n\
    elif test \"$VACPP\" = \"yes\"; then\n\
        MOZ_BUILD_ROOT=`pwd -D`\n\
        OBJ_SUFFIX=obj\n\
        AR=-ilib\n\
        AR_FLAGS=\'/NOL /NOI /O:$(subst /,\\\\,$@)\'\n\
        AR_LIST=\'/L\'\n\
        AR_EXTRACT=\'-*\'\n\
        AR_DELETE=\'-\'\n\
        AS=alp\n\
        ASFLAGS=\'-Mb\'\n\
        AS_DASH_C_FLAG=\'\'\n\
        ASM_SUFFIX=asm\n\
        LD=\'-ilink\'\n\
        CFLAGS=\"/Q /qlibansi /Gm+ /Su4 /Mp /Tl9\"\n\
        CXXFLAGS=\"/Q /qlibansi /Gm+ /Su4 /Mp /Tl9 /Gx+\"\n\
        MOZ_DEBUG_FLAGS=\"/Ti+\"\n\
        MOZ_OPTIMIZE_FLAGS=\"/O+ /Gl+ /G5 /qarch=pentium\"\n\
        LDFLAGS=\"/NOL /M\"\n\
        MOZ_DEBUG_LDFLAGS=\"/DE\"\n\
        MOZ_OPTIMIZE_LDFLAGS=\"/OPTFUNC /EXEPACK:2 /PACKCODE /PACKDATA\"\n\
        DSO_LDOPTS=\'\'\n\
        DSO_PIC_CFLAGS=\n\
        IMPLIB=\'implib /NOL /NOI\'\n\
        FILTER=\'cppfilt -q -B -P\'\n\
        AC_DEFINE(NO_ANSI_KEYWORDS)\n\
        AC_DEFINE(OS2,4)\n\
        AC_DEFINE(_X86_)\n\
        AC_DEFINE(XP_OS2_VACPP)\n\
        AC_DEFINE(TCPV40HDRS)\n\
        AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
        AC_DEFINE(STDC_HEADERS)\n\
        MOZ_COMPONENT_NSPR_LIBS=\'$(NSPR_LIBS)\'\n\
        MKSHLIB=\'$(LD) $(DSO_LDOPTS)\'\n\
        MKCSHLIB=\'$(LD) $(DSO_LDOPTS)\'\n\
        MOZ_JS_LIBS=\'$(LIBXUL_DIST)/lib/mozjs.lib\'\n\
        MOZ_XPCOM_OBSOLETE_LIBS=\'$(LIBXUL_DIST)/lib/xpcomct.lib\'\n\
        DYNAMIC_XPCOM_LIBS=\'$(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xpcomcor.lib\'\n\
        LIBXUL_LIBS=\'$(LIBXUL_DIST)/lib/xpcom.lib $(LIBXUL_DIST)/lib/xul.lib\'\n\
    fi\n\
    ;;\n\
\n\
alpha*-*-osf*)\n\
    if test \"$GNU_CC\"; then\n\
      MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -Wl,-soname,$@ -o $@\'\n\
      MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -Wl,-soname,$@ -o $@\'\n\
\n\
    else\n\
	MOZ_DEBUG_FLAGS=\'-g\'\n\
	ASFLAGS=\'-I$(topsrcdir)/xpcom/reflect/xptcall/public -g\'\n\
	CFLAGS=\"$CFLAGS -ieee\"\n\
	CXXFLAGS=\"$CXXFLAGS \"\'-noexceptions -ieee  -ptr $(DIST)/cxx_repository\'\n\
	DSO_LDOPTS=\'-shared -msym -expect_unresolved \\* -update_registry $(DIST)/so_locations\'\n\
	DSO_CFLAGS=\n\
	DSO_PIC_CFLAGS=\n\
	MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -soname $@ -o $@\'\n\
	MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_CFLAGS) $(DSO_LDOPTS) -soname $@ -o $@\'\n\
	MKSHLIB_FORCE_ALL=\'-all\'\n\
	MKSHLIB_UNFORCE_ALL=\'-none\'\n\
	dnl Might fix the libxpcom.so breakage on this platform as well....\n\
	AC_DEFINE(NSCAP_DISABLE_TEST_DONTQUERY_CASES)\n\
	AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
    fi\n\
    if test -z \"$GNU_CXX\"; then\n\
      COMPAQ_CXX=1\n\
    fi\n\
    AC_DEFINE(NEED_USLEEP_PROTOTYPE)\n\
    ;;\n\
\n\
*-qnx*) \n\
    DIRENT_INO=d_stat.st_ino\n\
    dnl Solves the problems the QNX compiler has with nsCOMPtr.h.\n\
    AC_DEFINE(NSCAP_DISABLE_TEST_DONTQUERY_CASES)\n\
    AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
    dnl Explicit set STDC_HEADERS to workaround QNX 6.0\'s failing of std test\n\
    AC_DEFINE(STDC_HEADERS)\n\
    if test \"$no_x\" = \"yes\"; then\n\
	    _PLATFORM_DEFAULT_TOOLKIT=\'photon\'\n\
	    TK_CFLAGS=\'-I/usr/nto/include/photon\'\n\
	    TK_LIBS=\'-lphoton -lphrender\'\n\
    fi\n\
    ;;\n\
\n\
*-sco*) \n\
    AC_DEFINE(NSCAP_DISABLE_TEST_DONTQUERY_CASES)\n\
    AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
    CXXFLAGS=\"$CXXFLAGS -I/usr/include/CC\"\n\
    if test ! \"$GNU_CC\"; then\n\
       DSO_LDOPTS=\'-G\'\n\
    fi\n\
    ;;\n\
\n\
dnl the qsort routine under solaris is faulty\n\
*-solaris*) \n\
    AC_DEFINE(SOLARIS)\n\
    TARGET_NSPR_MDCPUCFG=\'\\\"md/_solaris.cfg\\\"\'\n\
    SYSTEM_MAKEDEPEND=\n\
    # $ORIGIN/.. is for shared libraries under components/ to locate shared\n\
    # libraries one level up (e.g. libnspr4.so)\n\
    LDFLAGS=\"$LDFLAGS -z ignore -R \'\\$\\$ORIGIN:\\$\\$ORIGIN/..\'\"\n\
    if test -z \"$GNU_CC\"; then\n\
       NS_USE_NATIVE=1\n\
       MOZ_FIX_LINK_PATHS=\'-R $(LIBXUL_DIST)/bin\'\n\
       AC_DEFINE(NSCAP_DISABLE_DEBUG_PTR_TYPES)\n\
       CFLAGS=\"$CFLAGS -xstrconst -xbuiltin=%all\"\n\
       CXXFLAGS=\"$CXXFLAGS -xbuiltin=%all -features=tmplife -norunpath\"\n\
       LDFLAGS=\"-xildoff -z lazyload -z combreloc $LDFLAGS\"\n\
       if test -z \"$CROSS_COMPILE\" && test -f /usr/lib/ld/map.noexstk; then\n\
           _SAVE_LDFLAGS=$LDFLAGS\n\
           LDFLAGS=\"-M /usr/lib/ld/map.noexstk $LDFLAGS\" \n\
           AC_TRY_LINK([#include <stdio.h>],\n\
                       [printf(\"Hello World\\n\");],\n\
                       ,\n\
                       [LDFLAGS=$_SAVE_LDFLAGS])\n\
       fi\n\
       MOZ_OPTIMIZE_FLAGS=\"-xO4\"\n\
       MKSHLIB=\'$(CXX) $(CXXFLAGS) $(DSO_PIC_FLAGS) $(DSO_LDOPTS) -h $@ -o $@\'\n\
       MKCSHLIB=\'$(CC) $(CFLAGS) $(DSO_PIC_FLAGS) -G -z muldefs -h $@ -o $@\'\n\
       MKSHLIB_FORCE_ALL=\'-z allextract\'\n\
       MKSHLIB_UNFORCE_ALL=\'\'\n\
       DSO_LDOPTS=\'-G -z muldefs\'\n\
       AR_LIST=\"$AR t\"\n\
       AR_EXTRACT=\"$AR x\"\n\
       AR_DELETE=\"$AR d\"\n\
       AR=\'$(CXX) -xar\'\n\
       AR_FLAGS=\'-o $@\'\n\
       AS=\'/usr/ccs/bin/as\'\n\
       ASFLAGS=\"$ASFLAGS -K PIC -L -P -D_ASM -D__STDC__=0\"\n\
       AS_DASH_C_FLAG=\'\'\n\
       TARGET_COMPILER_ABI=\"sunc\"\n\
        CC_VERSION=`$CC -V 2>&1 | grep \'^cc:\' 2>/dev/null | $AWK -F\\: \'{ print $2 }\'`\n\
        CXX_VERSION=`$CXX -V 2>&1 | grep \'^CC:\' 2>/dev/null | $AWK -F\\: \'{ print $2 }\'`\n\
       AC_MSG_CHECKING([for Forte compiler version >= WS6U2])\n\
       AC_LANG_SAVE\n\
       AC_LANG_CPLUSPLUS\n\
       AC_TRY_COMPILE([],\n\
           [#if (__SUNPRO_CC < 0x530)\n\
           #error \"Denied\"\n\
           #endif],\n\
           _BAD_COMPILER=,_BAD_COMPILER=1)\n\
        if test -n \"$_BAD_COMPILER\"; then\n\
            _res=\"no\"\n\
            AC_MSG_ERROR([Forte version WS6U2 or higher is required to build. Your compiler version is $CC_VERSION .])\n\
        else\n\
            _res=\"yes\"\n\
        fi\n\
        AC_MSG_RESULT([$_res])\n\
        AC_LANG_RESTORE\n\
    else\n\
       ASFLAGS=\"$ASFLAGS -fPIC\"\n\
       DSO_LDOPTS=\'-G\'\n\
       _WARNINGS_CFLAGS=\'\'\n\
       _WARNINGS_CXXFLAGS=\'\'\n\
       if test \"$OS_RELEASE\" = \"5.3\"; then\n\
	  AC_DEFINE(MUST_UNDEF_HAVE_BOOLEAN_AFTER_INCLUDES)\n\
       fi\n\
    fi\n\
    if test \"$OS_RELEASE\" = \"5.5.1\"; then\n\
       AC_DEFINE(NEED_USLEEP_PROTOTYPE)\n\
    fi\n\
    ;;\n\
\n\
*-sunos*) \n\
    DSO_LDOPTS=\'-Bdynamic\'\n\
    MKSHLIB=\'-$(LD) $(DSO_LDOPTS) -o $@\'\n\
    MKCSHLIB=\'-$(LD) $(DSO_LDOPTS) -o $@\'\n\
    AC_DEFINE(SUNOS4)\n\
    AC_DEFINE(SPRINTF_RETURNS_STRING)\n\
    case \"$(target_os)\" in\n\
    sunos4.1*)\n\
        DLL_SUFFIX=\'.so.1.0\'\n\
        ;;\n\
    esac\n\
    ;;\n\
\n\
*-sysv4.2uw7*) \n\
	NSPR_LIBS=\"-lnspr$NSPR_VERSION -lplc$NSPR_VERSION -lplds$NSPR_VERSION -L/usr/ccs/lib -lcrt\"\n\
    ;;\n\
\n\
*-os2*)\n\
    HOST_NSPR_MDCPUCFG=\'\\\"md/_os2.cfg\\\"\'\n\
    ;;\n\
\n\
esac\n\
\n\
dnl Only one oddball right now (QNX), but this gives us flexibility\n\
dnl if any other platforms need to override this in the future.\n\
AC_DEFINE_UNQUOTED(D_INO,$DIRENT_INO)\n\
\n\
dnl ========================================================\n\
dnl Any platform that doesn\'t have MKSHLIB_FORCE_ALL defined\n\
dnl by now will not have any way to link most binaries (tests\n\
dnl as well as viewer, apprunner, etc.), because some symbols\n\
dnl will be left out of the \"composite\" .so\'s by ld as unneeded.\n\
dnl So, by defining NO_LD_ARCHIVE_FLAGS for these platforms,\n\
dnl they can link in the static libs that provide the missing\n\
dnl symbols.\n\
dnl ========================================================\n\
NO_LD_ARCHIVE_FLAGS=\n\
if test -z \"$MKSHLIB_FORCE_ALL\" || test -z \"$MKSHLIB_UNFORCE_ALL\"; then\n\
    NO_LD_ARCHIVE_FLAGS=1\n\
fi\n\
case \"$target\" in\n\
*-os2*)\n\
    NO_LD_ARCHIVE_FLAGS=\n\
    ;;\n\
*-aix4.3*|*-aix5*)\n\
    NO_LD_ARCHIVE_FLAGS=\n\
    ;;\n\
*-openvms*)\n\
    NO_LD_ARCHIVE_FLAGS=\n\
    ;;\n\
*-msvc*|*-mks*|*-mingw*|*-cygwin*|*-wince)\n\
    if test -z \"$GNU_CC\"; then\n\
        NO_LD_ARCHIVE_FLAGS=\n\
    fi\n\
    ;;\n\
esac\n\
AC_SUBST(NO_LD_ARCHIVE_FLAGS)\n\
\n\
dnl\n\
dnl Indicate that platform requires special thread safe \n\
dnl locking when starting up the OJI JVM \n\
dnl (see mozilla/modules/oji/src/nsJVMManager.cpp)\n\
dnl ========================================================\n\
case \"$target\" in\n\
    *-hpux*)      \n\
        AC_DEFINE(MOZ_OJI_REQUIRE_THREAD_SAFE_ON_STARTUP)\n\
        ;;\n\
esac\n\
\n\
dnl ========================================================\n\
dnl = Flags to strip unused symbols from .so components\n\
dnl ========================================================\n\
case \"$target\" in\n\
    *-linux*)\n\
        MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-Wl,--version-script -Wl,$(BUILD_TOOLS)/gnu-ld-scripts/components-version-script\'\n\
        ;;\n\
    *-solaris*)\n\
        if test -z \"$GNU_CC\"; then\n\
         MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-M $(BUILD_TOOLS)/gnu-ld-scripts/components-mapfile\'\n\
        else\n\
         if test -z \"$GCC_USE_GNU_LD\"; then\n\
          MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-Wl,-M -Wl,$(BUILD_TOOLS)/gnu-ld-scripts/components-mapfile\'\n\
         else\n\
          MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-Wl,--version-script -Wl,$(BUILD_TOOLS)/gnu-ld-scripts/components-version-script\'\n\
         fi\n\
        fi\n\
        ;;\n\
    *-nto*) \n\
        MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-Wl,--version-script,$(BUILD_TOOLS)/gnu-ld-scripts/components-version-script\'\n\
        ;;\n\
    *-darwin*)\n\
        AC_MSG_CHECKING(for -exported_symbols_list option to ld)\n\
        AC_CACHE_VAL(ac_cv_exported_symbols_list,\n\
         [\n\
           if $LD -exported_symbols_list 2>&1 | grep \"argument missing\" >/dev/null; then\n\
             ac_cv_exported_symbols_list=true\n\
           else\n\
             ac_cv_exported_symbols_list=false\n\
           fi\n\
         ])\n\
        if test \"$ac_cv_exported_symbols_list\" = true; then\n\
          MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-Wl,-exported_symbols_list -Wl,$(BUILD_TOOLS)/gnu-ld-scripts/components-export-list\'\n\
          AC_MSG_RESULT(yes)\n\
        else\n\
          AC_MSG_RESULT(no)\n\
        fi\n\
        ;;\n\
    *-cygwin*|*-mingw*|*-mks*|*-msvc|*-wince)\n\
        if test -n \"$GNU_CC\"; then\n\
           MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\'-Wl,--version-script,$(BUILD_TOOLS)/gnu-ld-scripts/components-version-script\'\n\
        fi\n\
        ;;\n\
esac\n\
\n\
if test -z \"$COMPILE_ENVIRONMENT\"; then\n\
    SKIP_COMPILER_CHECKS=1\n\
fi\n\
\n\
if test -z \"$SKIP_COMPILER_CHECKS\"; then\n\
dnl Checks for typedefs, structures, and compiler characteristics.\n\
dnl ========================================================\n\
AC_LANG_C\n\
AC_HEADER_STDC\n\
AC_C_CONST\n\
AC_TYPE_MODE_T\n\
AC_TYPE_OFF_T\n\
AC_TYPE_PID_T\n\
AC_TYPE_SIZE_T\n\
AC_STRUCT_ST_BLKSIZE\n\
AC_MSG_CHECKING(for siginfo_t)\n\
AC_CACHE_VAL(ac_cv_siginfo_t,\n\
 [AC_TRY_COMPILE([#define _POSIX_C_SOURCE 199506L\n\
                  #include <signal.h>],\n\
                 [siginfo_t* info;],\n\
                 [ac_cv_siginfo_t=true],\n\
                 [ac_cv_siginfo_t=false])])\n\
if test \"$ac_cv_siginfo_t\" = true ; then\n\
  AC_DEFINE(HAVE_SIGINFO_T)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
\n\
dnl Visual Age for os/2 also defines size_t and off_t in certain \n\
dnl  header files.  These defines make Visual Age use the mozilla\n\
dnl  defines types.\n\
if test \"$VACPP\" = \"yes\"; then\n\
   AC_DEFINE(__size_t)\n\
   AC_DEFINE(__off_t)\n\
fi\n\
\n\
dnl Check for int16_t, int32_t, int64_t, int64, uint, uint_t, and uint16_t.\n\
dnl ========================================================\n\
AC_MSG_CHECKING(for int16_t)\n\
AC_CACHE_VAL(ac_cv_int16_t,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [int16_t foo = 0;],\n\
                 [ac_cv_int16_t=true],\n\
                 [ac_cv_int16_t=false])])\n\
if test \"$ac_cv_int16_t\" = true ; then\n\
  AC_DEFINE(HAVE_INT16_T)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
AC_MSG_CHECKING(for int32_t)\n\
AC_CACHE_VAL(ac_cv_int32_t,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [int32_t foo = 0;],\n\
                 [ac_cv_int32_t=true],\n\
                 [ac_cv_int32_t=false])])\n\
if test \"$ac_cv_int32_t\" = true ; then\n\
  AC_DEFINE(HAVE_INT32_T)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
AC_MSG_CHECKING(for int64_t)\n\
AC_CACHE_VAL(ac_cv_int64_t,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [int64_t foo = 0;],\n\
                 [ac_cv_int64_t=true],\n\
                 [ac_cv_int64_t=false])])\n\
if test \"$ac_cv_int64_t\" = true ; then\n\
  AC_DEFINE(HAVE_INT64_T)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
AC_MSG_CHECKING(for int64)\n\
AC_CACHE_VAL(ac_cv_int64,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [int64 foo = 0;],\n\
                 [ac_cv_int64=true],\n\
                 [ac_cv_int64=false])])\n\
if test \"$ac_cv_int64\" = true ; then\n\
  AC_DEFINE(HAVE_INT64)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
AC_MSG_CHECKING(for uint)\n\
AC_CACHE_VAL(ac_cv_uint,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [uint foo = 0;],\n\
                 [ac_cv_uint=true],\n\
                 [ac_cv_uint=false])])\n\
if test \"$ac_cv_uint\" = true ; then\n\
  AC_DEFINE(HAVE_UINT)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
AC_MSG_CHECKING(for uint_t)\n\
AC_CACHE_VAL(ac_cv_uint_t,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [uint_t foo = 0;],\n\
                 [ac_cv_uint_t=true],\n\
                 [ac_cv_uint_t=false])])\n\
if test \"$ac_cv_uint_t\" = true ; then\n\
  AC_DEFINE(HAVE_UINT_T)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
AC_MSG_CHECKING(for uint16_t)\n\
AC_CACHE_VAL(ac_cv_uint16_t,\n\
 [AC_TRY_COMPILE([#include <stdio.h>\n\
                  #include <sys/types.h>],\n\
                 [uint16_t foo = 0;],\n\
                 [ac_cv_uint16_t=true],\n\
                 [ac_cv_uint16_t=false])])\n\
if test \"$ac_cv_uint16_t\" = true ; then\n\
  AC_DEFINE(HAVE_UINT16_T)\n\
  AC_MSG_RESULT(yes)\n\
else\n\
  AC_MSG_RESULT(no)\n\
fi\n\
\n\
dnl On the gcc trunk (as of 2001-02-09) _GNU_SOURCE, and thus __USE_GNU,\n\
dnl are defined when compiling C++ but not C.  Since the result of this\n\
dnl test is used only in C++, do it in C++.\n\
AC_LANG_CPLUSPLUS\n\
\n\
AC_MSG_CHECKING(for uname.domainname)\n\
AC_CACHE_VAL(ac_cv_have_uname_domainname_field,\n\
    [AC_TRY_COMPILE([#include <sys/utsname.h>],\n\
        [ struct utsname *res; char *domain; \n\
            (void)uname(res);  if (res != 0) { domain = res->domainname; } ],\n\
        [ac_cv_have_uname_domainname_field=true],\n\
        [ac_cv_have_uname_domainname_field=false])])\n\
\n\
if test \"$ac_cv_have_uname_domainname_field\" = \"true\"; then\n\
    AC_DEFINE(HAVE_UNAME_DOMAINNAME_FIELD)\n\
    AC_MSG_RESULT(yes)\n\
else\n\
    AC_MSG_RESULT(no)\n\
fi\n\
\n\
AC_MSG_CHECKING(for uname.__domainname)\n\
AC_CACHE_VAL(ac_cv_have_uname_us_domainname_field,\n\
    [AC_TRY_COMPILE([#include <sys/utsname.h>],\n\
        [ struct utsname *res; char *domain; \n\
            (void)uname(res);  if (res != 0) { domain = res->__domainname; } ],\n\
        [ac_cv_have_uname_us_domainname_field=true],\n\
        [ac_cv_have_uname_us_domainname_field=false])])\n\
\n\
if test \"$ac_cv_have_uname_us_domainname_field\" = \"true\"; then\n\
    AC_DEFINE(HAVE_UNAME_US_DOMAINNAME_FIELD)\n\
    AC_MSG_RESULT(yes)\n\
else\n\
    AC_MSG_RESULT(no)\n\
fi\n\
\n\
AC_LANG_C\n\
\n\
dnl Check for usable wchar_t (2 bytes, unsigned)\n\
dnl (we really don\'t need the unsignedness check anymore)\n\
dnl ========================================================\n\
\n\
AC_CACHE_CHECK(for usable wchar_t (2 bytes, unsigned),\n\
    ac_cv_have_usable_wchar_v2,\n\
    [AC_TRY_COMPILE([#include <stddef.h>\n\
                     $configure_static_assert_macros],\n\
                    [CONFIGURE_STATIC_ASSERT(sizeof(wchar_t) == 2);\n\
                     CONFIGURE_STATIC_ASSERT((wchar_t)-1 > (wchar_t) 0)],\n\
                    ac_cv_have_usable_wchar_v2=\"yes\",\n\
                    ac_cv_have_usable_wchar_v2=\"no\")])\n\
if test \"$ac_cv_have_usable_wchar_v2\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_CPP_2BYTE_WCHAR_T)\n\
    HAVE_CPP_2BYTE_WCHAR_T=1\n\
else\n\
dnl This is really gcc-only\n\
dnl Do this test using CXX only since some versions of gcc\n\
dnl 2.95-2.97 have a signed wchar_t in c++ only and some versions\n\
dnl only have short-wchar support for c++.\n\
dnl Note that we assume that mac & win32 have short wchar (see nscore.h)\n\
\n\
    AC_LANG_SAVE\n\
    AC_LANG_CPLUSPLUS\n\
    _SAVE_CXXFLAGS=$CXXFLAGS\n\
    CXXFLAGS=\"$CXXFLAGS -fshort-wchar\"\n\
\n\
    AC_CACHE_CHECK(for compiler -fshort-wchar option, \n\
        ac_cv_have_usable_wchar_option_v2,\n\
        [AC_TRY_COMPILE([#include <stddef.h>\n\
                         $configure_static_assert_macros],\n\
                        [CONFIGURE_STATIC_ASSERT(sizeof(wchar_t) == 2);\n\
                         CONFIGURE_STATIC_ASSERT((wchar_t)-1 > (wchar_t) 0)],\n\
                        ac_cv_have_usable_wchar_option_v2=\"yes\",\n\
                        ac_cv_have_usable_wchar_option_v2=\"no\")])\n\
\n\
    if test \"$ac_cv_have_usable_wchar_option_v2\" = \"yes\"; then\n\
        AC_DEFINE(HAVE_CPP_2BYTE_WCHAR_T)\n\
        HAVE_CPP_2BYTE_WCHAR_T=1\n\
    else    \n\
        CXXFLAGS=$_SAVE_CXXFLAGS\n\
    fi\n\
    AC_LANG_RESTORE\n\
fi\n\
\n\
dnl Check for .hidden assembler directive and visibility attribute.\n\
dnl Borrowed from glibc configure.in\n\
dnl ===============================================================\n\
if test \"$GNU_CC\"; then\n\
  AC_CACHE_CHECK(for visibility(hidden) attribute,\n\
                 ac_cv_visibility_hidden,\n\
                 [cat > conftest.c <<EOF\n\
                  int foo __attribute__ ((visibility (\"hidden\"))) = 1;\n\
EOF\n\
                  ac_cv_visibility_hidden=no\n\
                  if ${CC-cc} -Werror -S conftest.c -o conftest.s >/dev/null 2>&1; then\n\
                    if egrep \'\\.(hidden|private_extern).*foo\' conftest.s >/dev/null; then\n\
                      ac_cv_visibility_hidden=yes\n\
                    fi\n\
                  fi\n\
                  rm -f conftest.[cs]\n\
                 ])\n\
  if test \"$ac_cv_visibility_hidden\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_VISIBILITY_HIDDEN_ATTRIBUTE)\n\
\n\
    AC_CACHE_CHECK(for visibility(default) attribute,\n\
                   ac_cv_visibility_default,\n\
                   [cat > conftest.c <<EOF\n\
                    int foo __attribute__ ((visibility (\"default\"))) = 1;\n\
EOF\n\
                    ac_cv_visibility_default=no\n\
                    if ${CC-cc} -fvisibility=hidden -Werror -S conftest.c -o conftest.s >/dev/null 2>&1; then\n\
                      if ! egrep \'\\.(hidden|private_extern).*foo\' conftest.s >/dev/null; then\n\
                        ac_cv_visibility_default=yes\n\
                      fi\n\
                    fi\n\
                    rm -f conftest.[cs]\n\
                   ])\n\
    if test \"$ac_cv_visibility_default\" = \"yes\"; then\n\
      AC_DEFINE(HAVE_VISIBILITY_ATTRIBUTE)\n\
\n\
      AC_CACHE_CHECK(for visibility pragma support,\n\
                     ac_cv_visibility_pragma,\n\
                     [cat > conftest.c <<EOF\n\
#pragma GCC visibility push(hidden)\n\
                      int foo_hidden = 1;\n\
#pragma GCC visibility push(default)\n\
                      int foo_default = 1;\n\
EOF\n\
                      ac_cv_visibility_pragma=no\n\
                      if ${CC-cc} -Werror -S conftest.c -o conftest.s >/dev/null 2>&1; then\n\
                        if egrep \'\\.(hidden|extern_private).*foo_hidden\' conftest.s >/dev/null; then\n\
                          if ! egrep \'\\.(hidden|extern_private).*foo_default\' conftest.s > /dev/null; then\n\
                            ac_cv_visibility_pragma=yes\n\
                          fi\n\
                        fi\n\
                      fi\n\
                      rm -f conftest.[cs]\n\
                    ])\n\
      if test \"$ac_cv_visibility_pragma\" = \"yes\"; then\n\
        AC_CACHE_CHECK(For gcc visibility bug with class-level attributes (GCC bug 26905),\n\
                       ac_cv_have_visibility_class_bug,\n\
                       [cat > conftest.c <<EOF\n\
#pragma GCC visibility push(hidden)\n\
struct __attribute__ ((visibility (\"default\"))) TestStruct {\n\
  static void Init();\n\
};\n\
__attribute__ ((visibility (\"default\"))) void TestFunc() {\n\
  TestStruct::Init();\n\
}\n\
EOF\n\
                       ac_cv_have_visibility_class_bug=no\n\
                       if ! ${CXX-g++} ${CXXFLAGS} ${DSO_PIC_CFLAGS} ${DSO_LDOPTS} -S -o conftest.S conftest.c > /dev/null 2>&1 ; then\n\
                         ac_cv_have_visibility_class_bug=yes\n\
                       else\n\
                         if test `grep -c \"@PLT\" conftest.S` = 0; then\n\
                           ac_cv_have_visibility_class_bug=yes\n\
                         fi\n\
                       fi\n\
                       rm -rf conftest.{c,S}\n\
                       ])\n\
\n\
        AC_CACHE_CHECK(For x86_64 gcc visibility bug with builtins (GCC bug 20297),\n\
                       ac_cv_have_visibility_builtin_bug,\n\
                       [cat > conftest.c <<EOF\n\
#pragma GCC visibility push(hidden)\n\
#pragma GCC visibility push(default)\n\
#include <string.h>\n\
#pragma GCC visibility pop\n\
\n\
__attribute__ ((visibility (\"default\"))) void Func() {\n\
  char c[[100]];\n\
  memset(c, 0, sizeof(c));\n\
}\n\
EOF\n\
                       ac_cv_have_visibility_builtin_bug=no\n\
                       if ! ${CC-cc} ${CFLAGS} ${DSO_PIC_CFLAGS} ${DSO_LDOPTS} -O2 -S -o conftest.S conftest.c > /dev/null 2>&1 ; then\n\
                         ac_cv_have_visibility_builtin_bug=yes\n\
                       else\n\
                         if test `grep -c \"@PLT\" conftest.S` = 0; then\n\
                           ac_cv_visibility_builtin_bug=yes\n\
                         fi\n\
                       fi\n\
                       rm -f conftest.{c,S}\n\
                       ])\n\
        if test \"$ac_cv_have_visibility_builtin_bug\" = \"no\" -a \\\n\
                \"$ac_cv_have_visibility_class_bug\" = \"no\"; then\n\
          VISIBILITY_FLAGS=\'-I$(DIST)/include/system_wrappers -include $(topsrcdir)/config/gcc_hidden.h\'\n\
          WRAP_SYSTEM_INCLUDES=1\n\
        else\n\
          VISIBILITY_FLAGS=\'-fvisibility=hidden\'\n\
        fi # have visibility pragma bug\n\
      fi   # have visibility pragma\n\
    fi     # have visibility(default) attribute\n\
  fi       # have visibility(hidden) attribute\n\
fi         # GNU_CC\n\
\n\
AC_SUBST(WRAP_SYSTEM_INCLUDES)\n\
AC_SUBST(VISIBILITY_FLAGS)\n\
\n\
dnl Checks for header files.\n\
dnl ========================================================\n\
AC_HEADER_DIRENT\n\
case \"$target_os\" in\n\
freebsd*)\n\
# for stuff like -lXshm\n\
    CPPFLAGS=\"${CPPFLAGS} ${X_CFLAGS}\"\n\
    ;;\n\
esac\n\
AC_CHECK_HEADERS(sys/byteorder.h compat.h getopt.h)\n\
AC_CHECK_HEADERS(sys/bitypes.h memory.h unistd.h)\n\
AC_CHECK_HEADERS(gnu/libc-version.h nl_types.h)\n\
AC_CHECK_HEADERS(malloc.h)\n\
AC_CHECK_HEADERS(X11/XKBlib.h)\n\
\n\
dnl These are all the places some variant of statfs can be hiding.\n\
AC_CHECK_HEADERS(sys/statvfs.h sys/statfs.h sys/vfs.h sys/mount.h)\n\
\n\
dnl Try for MMX support\n\
dnl NB - later gcc versions require -mmmx for this header to be successfully\n\
dnl included (or another option which implies it, such as -march=pentium-mmx)\n\
AC_CHECK_HEADERS(mmintrin.h)\n\
\n\
dnl Check whether the compiler supports the new-style C++ standard\n\
dnl library headers (i.e. <new>) or needs the old \"new.h\"\n\
AC_LANG_CPLUSPLUS\n\
NEW_H=new.h\n\
AC_CHECK_HEADER(new, [NEW_H=new])\n\
AC_DEFINE_UNQUOTED(NEW_H, <$NEW_H>)\n\
AC_LANG_C\n\
\n\
case $target in\n\
*-aix4.3*|*-aix5*)\n\
	;;\n\
*)\n\
	AC_CHECK_HEADERS(sys/cdefs.h)\n\
	;;\n\
esac\n\
\n\
dnl Checks for libraries.\n\
dnl ========================================================\n\
case $target in\n\
*-hpux11.*)\n\
	;;\n\
*)\n\
	AC_CHECK_LIB(c_r, gethostbyname_r)\n\
	;;\n\
esac\n\
\n\
dnl We don\'t want to link with libdl even if it\'s present on OS X, since\n\
dnl it\'s not used and not part of the default installation.\n\
dnl The same goes for BeOS.\n\
dnl We don\'t want to link against libm or libpthread on Darwin since\n\
dnl they both are just symlinks to libSystem and explicitly linking\n\
dnl against libSystem causes issues when debugging (see bug 299601).\n\
case $target in\n\
*-darwin*)\n\
    ;;\n\
*-beos*)\n\
    ;;\n\
*)\n\
    AC_CHECK_LIB(m, atan)\n\
    AC_CHECK_LIB(dl, dlopen,\n\
    AC_CHECK_HEADER(dlfcn.h, \n\
        LIBS=\"-ldl $LIBS\"\n\
        AC_DEFINE(HAVE_LIBDL)))\n\
    ;;\n\
esac\n\
if test ! \"$GNU_CXX\"; then\n\
\n\
    case $target in\n\
    *-aix*)\n\
	AC_CHECK_LIB(C_r, demangle)\n\
	;;\n\
     *)\n\
	AC_CHECK_LIB(C, demangle)\n\
	;;\n\
     esac\n\
fi\n\
AC_CHECK_LIB(socket, socket)\n\
\n\
XLDFLAGS=\"$X_LIBS\"\n\
XLIBS=\"$X_EXTRA_LIBS\"\n\
\n\
dnl ========================================================\n\
dnl Checks for X libraries.\n\
dnl Ordering is important.\n\
dnl Xt is dependent upon SM as of X11R6\n\
dnl ========================================================\n\
if test \"$no_x\" = \"yes\"; then\n\
    AC_DEFINE(NO_X11)\n\
else\n\
    AC_DEFINE_UNQUOTED(FUNCPROTO,15)\n\
	XLIBS=\"-lX11 $XLIBS\"\n\
	_SAVE_LDFLAGS=\"$LDFLAGS\"\n\
	LDFLAGS=\"$XLDFLAGS $LDFLAGS\"\n\
	AC_CHECK_LIB(X11, XDrawLines, [X11_LIBS=\"-lX11\"],\n\
		[MISSING_X=\"$MISSING_X -lX11\"], $XLIBS)\n\
	AC_CHECK_LIB(Xext, XextAddDisplay, [XEXT_LIBS=\"-lXext\"],\n\
		[MISSING_X=\"$MISSING_X -lXext\"], $XLIBS)\n\
   \n\
     \n\
	AC_CHECK_LIB(Xt, XtFree, [ XT_LIBS=\"-lXt\"], [\n\
        unset ac_cv_lib_Xt_XtFree\n\
	    AC_CHECK_LIB(ICE, IceFlush, [XT_LIBS=\"-lICE $XT_LIBS\"],, $XT_LIBS $XLIBS)\n\
	    AC_CHECK_LIB(SM, SmcCloseConnection, [XT_LIBS=\"-lSM $XT_LIBS\"],, $XT_LIBS $XLIBS) \n\
        AC_CHECK_LIB(Xt, XtFree, [ XT_LIBS=\"-lXt $XT_LIBS\"],\n\
		    [MISSING_X=\"$MISSING_X -lXt\"], $X_PRE_LIBS $XT_LIBS $XLIBS)\n\
        ])\n\
\n\
    # AIX needs the motif library linked before libXt to prevent\n\
    # crashes in plugins linked against Motif - Bug #98892\n\
    case \"${target_os}\" in\n\
    aix*)\n\
        XT_LIBS=\"-lXm $XT_LIBS\"\n\
        ;;\n\
    esac\n\
\n\
    dnl ========================================================\n\
    dnl = Check for Xinerama\n\
    dnl ========================================================\n\
    AC_CHECK_LIB(Xinerama, XineramaIsActive, [MOZ_XINERAMA_LIBS=\"-lXinerama\"],,\n\
        $XLIBS $XEXT_LIBS)\n\
    AC_CHECK_HEADER(X11/extensions/Xinerama.h)\n\
\n\
    dnl ========================================================\n\
    dnl = Check for XShm\n\
    dnl ========================================================\n\
    AC_CHECK_LIB(Xext, XShmCreateImage, _HAVE_XSHM_XEXT=1,,\n\
        $XLIBS $XEXT_LIBS)\n\
    AC_CHECK_HEADER(X11/extensions/XShm.h)\n\
    if test -n \"$ac_cv_header_X11_extensions_XShm_h\" &&\n\
        test -n \"$_HAVE_XSHM_XEXT\"; then\n\
        AC_DEFINE(HAVE_XSHM)\n\
    fi\n\
\n\
    dnl ========================================================\n\
    dnl = Check for XIE\n\
    dnl ========================================================\n\
    AC_CHECK_LIB(XIE, XieFloGeometry, [MOZ_XIE_LIBS=\"-lXIE\"],,\n\
        $XLIBS $XEXT_LIBS)\n\
    AC_CHECK_HEADER(X11/extensions/XIElib.h)\n\
\n\
    if test \"$MOZ_XIE_LIBS\"; then\n\
	dnl ====================================================\n\
	dnl = If XIE is present and is desired, turn it on\n\
	dnl ====================================================\n\
	case $target in\n\
	    *-hpux*)\n\
		;;\n\
	    *)\n\
		HAVE_XIE=1\n\
		;;\n\
	esac\n\
    fi\n\
\n\
	LDFLAGS=\"$_SAVE_LDFLAGS\"\n\
\n\
    AC_CHECK_FT2(6.1.0, [_HAVE_FREETYPE2=1], [_HAVE_FREETYPE2=])\n\
\n\
fi # $no_x\n\
\n\
AC_SUBST(XCFLAGS)\n\
AC_SUBST(XLDFLAGS)\n\
AC_SUBST(XLIBS)\n\
AC_SUBST(XT_LIBS)\n\
\n\
dnl ========================================================\n\
dnl = pthread support\n\
dnl = Start by checking whether the system support pthreads\n\
dnl ========================================================\n\
case \"$target_os\" in\n\
darwin*)\n\
    USE_PTHREADS=1\n\
    ;;\n\
*)\n\
    MOZ_CHECK_PTHREADS(pthreads,\n\
        USE_PTHREADS=1 _PTHREAD_LDFLAGS=\"-lpthreads\",\n\
        MOZ_CHECK_PTHREADS(pthread,\n\
            USE_PTHREADS=1 _PTHREAD_LDFLAGS=\"-lpthread\",\n\
            MOZ_CHECK_PTHREADS(c_r,\n\
                USE_PTHREADS=1 _PTHREAD_LDFLAGS=\"-lc_r\",\n\
                MOZ_CHECK_PTHREADS(c,\n\
                    USE_PTHREADS=1\n\
                )\n\
            )\n\
        )\n\
    )\n\
    ;;\n\
esac\n\
\n\
dnl ========================================================\n\
dnl Check the command line for --with-pthreads \n\
dnl ========================================================\n\
MOZ_ARG_WITH_BOOL(pthreads,\n\
[  --with-pthreads         Force use of system pthread library with NSPR ],\n\
[ if test \"$USE_PTHREADS\"x = x; then\n\
    AC_MSG_ERROR([ --with-pthreads specified for a system without pthread support ]);\n\
fi],\n\
    USE_PTHREADS=\n\
    _PTHREAD_LDFLAGS=\n\
)\n\
\n\
dnl ========================================================\n\
dnl Do the platform specific pthread hackery\n\
dnl ========================================================\n\
if test \"$USE_PTHREADS\"x != x\n\
then\n\
	dnl\n\
	dnl See if -pthread is supported.\n\
	dnl\n\
	rm -f conftest*\n\
	ac_cv_have_dash_pthread=no\n\
	AC_MSG_CHECKING(whether ${CC-cc} accepts -pthread)\n\
	echo \'int main() { return 0; }\' | cat > conftest.c\n\
	${CC-cc} -pthread -o conftest conftest.c > conftest.out 2>&1\n\
	if test $? -eq 0; then\n\
		if test -z \"`egrep -i \'(unrecognize|unknown)\' conftest.out | grep pthread`\" && test -z \"`egrep -i \'(error|incorrect)\' conftest.out`\" ; then\n\
			ac_cv_have_dash_pthread=yes\n\
	        case \"$target_os\" in\n\
	        freebsd*)\n\
# Freebsd doesn\'t use -pthread for compiles, it uses them for linking\n\
                ;;\n\
	        *)\n\
			    CFLAGS=\"$CFLAGS -pthread\"\n\
			    CXXFLAGS=\"$CXXFLAGS -pthread\"\n\
                ;;\n\
	        esac\n\
		fi\n\
	fi\n\
	rm -f conftest*\n\
    AC_MSG_RESULT($ac_cv_have_dash_pthread)\n\
\n\
	dnl\n\
	dnl See if -pthreads is supported.\n\
	dnl\n\
    ac_cv_have_dash_pthreads=no\n\
    if test \"$ac_cv_have_dash_pthread\" = \"no\"; then\n\
	    AC_MSG_CHECKING(whether ${CC-cc} accepts -pthreads)\n\
    	echo \'int main() { return 0; }\' | cat > conftest.c\n\
	    ${CC-cc} -pthreads -o conftest conftest.c > conftest.out 2>&1\n\
    	if test $? -eq 0; then\n\
	    	if test -z \"`egrep -i \'(unrecognize|unknown)\' conftest.out | grep pthreads`\" && test -z \"`egrep -i \'(error|incorrect)\' conftest.out`\" ; then\n\
			    ac_cv_have_dash_pthreads=yes\n\
			    CFLAGS=\"$CFLAGS -pthreads\"\n\
			    CXXFLAGS=\"$CXXFLAGS -pthreads\"\n\
		    fi\n\
	    fi\n\
	    rm -f conftest*\n\
    	AC_MSG_RESULT($ac_cv_have_dash_pthreads)\n\
    fi\n\
\n\
	case \"$target\" in\n\
	    *-*-freebsd*)\n\
			AC_DEFINE(_REENTRANT)\n\
			AC_DEFINE(_THREAD_SAFE)\n\
			dnl -pthread links in -lc_r, so don\'t specify it explicitly.\n\
			if test \"$ac_cv_have_dash_pthread\" = \"yes\"; then\n\
				_PTHREAD_LDFLAGS=\"-pthread\"\n\
			else\n\
				_PTHREAD_LDFLAGS=\"-lc_r\"\n\
			fi\n\
			;;\n\
\n\
	    *-*-openbsd*|*-*-bsdi*)\n\
			AC_DEFINE(_REENTRANT)\n\
			AC_DEFINE(_THREAD_SAFE)\n\
			dnl -pthread links in -lc_r, so don\'t specify it explicitly.\n\
			if test \"$ac_cv_have_dash_pthread\" = \"yes\"; then\n\
                _PTHREAD_LDFLAGS=\"-pthread\"\n\
			fi\n\
			;;\n\
\n\
	    *-*-linux*) \n\
			AC_DEFINE(_REENTRANT) \n\
			;;\n\
\n\
	    *-*-nto*) \n\
			AC_DEFINE(_REENTRANT) \n\
			;;\n\
\n\
	    *-aix4.3*|*-aix5*)\n\
			AC_DEFINE(_REENTRANT) \n\
			;;\n\
\n\
	    *-hpux11.*)\n\
			AC_DEFINE(_REENTRANT) \n\
			;;\n\
\n\
	    alpha*-*-osf*)\n\
			AC_DEFINE(_REENTRANT)\n\
			;;\n\
\n\
	    *-*-solaris*) \n\
    			AC_DEFINE(_REENTRANT) \n\
			if test ! \"$GNU_CC\"; then\n\
				CFLAGS=\"$CFLAGS -mt\" \n\
				CXXFLAGS=\"$CXXFLAGS -mt\" \n\
			fi\n\
			;;\n\
	esac\n\
    LDFLAGS=\"${_PTHREAD_LDFLAGS} ${LDFLAGS}\"\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Check for MacOS deployment target version\n\
dnl ========================================================\n\
\n\
MOZ_ARG_ENABLE_STRING(macos-target,\n\
                      [  --enable-macos-target=VER (default=10.3/ppc, 10.4/x86)\n\
                          Set the minimum MacOS version needed at runtime],\n\
                      [_MACOSX_DEPLOYMENT_TARGET=$enableval])\n\
\n\
case \"$target\" in\n\
*-darwin*)\n\
    if test -n \"$_MACOSX_DEPLOYMENT_TARGET\" ; then\n\
        dnl Use the specified value\n\
        export MACOSX_DEPLOYMENT_TARGET=$_MACOSX_DEPLOYMENT_TARGET\n\
    elif test -z \"$MACOSX_DEPLOYMENT_TARGET\" ; then\n\
        dnl No value specified on the command line or in the environment,\n\
        dnl use the lesser of the application\'s minimum or the architecture\'s\n\
        dnl minimum.\n\
        case \"${target_cpu}\" in\n\
            powerpc*)\n\
                dnl Architecture minimum 10.3\n\
                dnl export MACOSX_DEPLOYMENT_TARGET=10.3\n\
                export MACOSX_DEPLOYMENT_TARGET=10.2\n\
                ;;\n\
            i*86*)\n\
                dnl Architecture minimum 10.4\n\
                export MACOSX_DEPLOYMENT_TARGET=10.4\n\
                ;;\n\
        esac\n\
    fi\n\
    ;;\n\
esac\n\
\n\
AC_SUBST(MACOSX_DEPLOYMENT_TARGET)\n\
\n\
dnl ========================================================\n\
dnl See if mmap sees writes\n\
dnl For cross compiling, just define it as no, which is a safe default\n\
dnl ========================================================\n\
AC_MSG_CHECKING(whether mmap() sees write()s)\n\
\n\
changequote(,)\n\
mmap_test_prog=\'\n\
    #include <stdlib.h>\n\
    #include <unistd.h>\n\
    #include <sys/mman.h>\n\
    #include <sys/types.h>\n\
    #include <sys/stat.h>\n\
    #include <fcntl.h>\n\
\n\
    char fname[] = \"conftest.file\";\n\
    char zbuff[1024]; /* Fractional page is probably worst case */\n\
\n\
    int main() {\n\
	char *map;\n\
	int fd;\n\
	int i;\n\
	unlink(fname);\n\
	fd = open(fname, O_RDWR | O_CREAT, 0660);\n\
	if(fd<0) return 1;\n\
	unlink(fname);\n\
	write(fd, zbuff, sizeof(zbuff));\n\
	lseek(fd, 0, SEEK_SET);\n\
	map = (char*)mmap(0, sizeof(zbuff), PROT_READ, MAP_SHARED, fd, 0);\n\
	if(map==(char*)-1) return 2;\n\
	for(i=0; fname[i]; i++) {\n\
	    int rc = write(fd, &fname[i], 1);\n\
	    if(map[i]!=fname[i]) return 4;\n\
	}\n\
	return 0;\n\
    }\n\
\'\n\
changequote([,])\n\
\n\
AC_TRY_RUN($mmap_test_prog , result=\"yes\", result=\"no\", result=\"yes\")\n\
\n\
AC_MSG_RESULT(\"$result\")\n\
\n\
if test \"$result\" = \"no\"; then\n\
    AC_DEFINE(MMAP_MISSES_WRITES)\n\
fi\n\
\n\
\n\
dnl Checks for library functions.\n\
dnl ========================================================\n\
AC_PROG_GCC_TRADITIONAL\n\
AC_FUNC_MEMCMP\n\
AC_CHECK_FUNCS(random strerror lchown fchmod snprintf statvfs memmove rint stat64 lstat64)\n\
AC_CHECK_FUNCS(flockfile getpagesize)\n\
\n\
dnl localtime_r and strtok_r are only present on MacOS version 10.2 and higher\n\
if test -z \"$MACOS_DEPLOYMENT_TARGET\" || test \"$MACOS_DEPLOYMENT_TARGET\" -ge \"100200\"; then\n\
  AC_CHECK_FUNCS(localtime_r strtok_r)\n\
fi\n\
\n\
dnl check for wcrtomb/mbrtowc\n\
dnl =======================================================================\n\
if test -z \"$MACOS_DEPLOYMENT_TARGET\" || test \"$MACOS_DEPLOYMENT_TARGET\" -ge \"100300\"; then\n\
AC_LANG_SAVE\n\
AC_LANG_CPLUSPLUS\n\
AC_CACHE_CHECK(for wcrtomb,\n\
    ac_cv_have_wcrtomb,\n\
    [AC_TRY_LINK([#include <wchar.h>],\n\
                 [mbstate_t ps={0};wcrtomb(0,\'f\',&ps);],\n\
                 ac_cv_have_wcrtomb=\"yes\",\n\
                 ac_cv_have_wcrtomb=\"no\")])\n\
if test \"$ac_cv_have_wcrtomb\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_WCRTOMB)\n\
fi\n\
AC_CACHE_CHECK(for mbrtowc,\n\
    ac_cv_have_mbrtowc,\n\
    [AC_TRY_LINK([#include <wchar.h>],\n\
                 [mbstate_t ps={0};mbrtowc(0,0,0,&ps);],\n\
                 ac_cv_have_mbrtowc=\"yes\",\n\
                 ac_cv_have_mbrtowc=\"no\")])\n\
if test \"$ac_cv_have_mbrtowc\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_MBRTOWC)\n\
fi\n\
AC_LANG_RESTORE\n\
fi\n\
\n\
AC_CACHE_CHECK(\n\
    [for res_ninit()],\n\
    ac_cv_func_res_ninit,\n\
    [AC_TRY_LINK([\n\
        #ifdef linux\n\
        #define _BSD_SOURCE 1\n\
        #endif\n\
        #include <resolv.h>\n\
        ],\n\
        [int foo = res_ninit(&_res);],\n\
        [ac_cv_func_res_ninit=yes],\n\
        [ac_cv_func_res_ninit=no])\n\
    ])\n\
\n\
if test \"$ac_cv_func_res_ninit\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_RES_NINIT)\n\
dnl must add the link line we do something as foolish as this... dougt\n\
dnl else\n\
dnl    AC_CHECK_LIB(bind, res_ninit, AC_DEFINE(HAVE_RES_NINIT),\n\
dnl        AC_CHECK_LIB(resolv, res_ninit, AC_DEFINE(HAVE_RES_NINIT)))\n\
fi\n\
\n\
AC_LANG_CPLUSPLUS\n\
AC_CACHE_CHECK(\n\
    [for gnu_get_libc_version()],\n\
    ac_cv_func_gnu_get_libc_version,\n\
    [AC_TRY_LINK([\n\
        #ifdef HAVE_GNU_LIBC_VERSION_H\n\
        #include <gnu/libc-version.h>\n\
        #endif\n\
        ],\n\
        [const char *glibc_version = gnu_get_libc_version();],\n\
        [ac_cv_func_gnu_get_libc_version=yes],\n\
        [ac_cv_func_gnu_get_libc_version=no] \n\
        )]\n\
    )\n\
\n\
if test \"$ac_cv_func_gnu_get_libc_version\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_GNU_GET_LIBC_VERSION)\n\
fi\n\
\n\
case $target_os in\n\
    os2*|msvc*|mks*|cygwin*|mingw*|darwin*|wince*|beos*)\n\
        ;;\n\
    *)\n\
    \n\
AC_CHECK_LIB(c, iconv, [_ICONV_LIBS=\"$_ICONV_LIBS\"],\n\
    AC_CHECK_LIB(iconv, iconv, [_ICONV_LIBS=\"$_ICONV_LIBS -liconv\"],\n\
        AC_CHECK_LIB(iconv, libiconv, [_ICONV_LIBS=\"$_ICONV_LIBS -liconv\"])))\n\
_SAVE_LIBS=$LIBS\n\
LIBS=\"$LIBS $_ICONV_LIBS\"\n\
AC_CACHE_CHECK(\n\
    [for iconv()],\n\
    ac_cv_func_iconv,\n\
    [AC_TRY_LINK([\n\
        #include <stdlib.h>\n\
        #include <iconv.h>\n\
        ],\n\
        [\n\
            iconv_t h = iconv_open(\"\", \"\");\n\
            iconv(h, NULL, NULL, NULL, NULL);\n\
            iconv_close(h);\n\
        ],\n\
        [ac_cv_func_iconv=yes],\n\
        [ac_cv_func_iconv=no] \n\
        )]\n\
    )\n\
if test \"$ac_cv_func_iconv\" = \"yes\"; then\n\
    AC_DEFINE(HAVE_ICONV)\n\
    DYNAMIC_XPCOM_LIBS=\"$DYNAMIC_XPCOM_LIBS $_ICONV_LIBS\"\n\
    LIBXUL_LIBS=\"$LIBXUL_LIBS $_ICONV_LIBS\"\n\
    LIBICONV=\"$_ICONV_LIBS\"\n\
    AC_CACHE_CHECK(\n\
        [for iconv() with const input],\n\
        ac_cv_func_const_iconv,\n\
        [AC_TRY_COMPILE([\n\
            #include <stdlib.h>\n\
            #include <iconv.h>\n\
            ],\n\
            [\n\
                const char *input = \"testing\";\n\
                iconv_t h = iconv_open(\"\", \"\");\n\
                iconv(h, &input, NULL, NULL, NULL);\n\
                iconv_close(h);\n\
            ],\n\
            [ac_cv_func_const_iconv=yes],\n\
            [ac_cv_func_const_iconv=no] \n\
            )]\n\
        )\n\
    if test \"$ac_cv_func_const_iconv\" = \"yes\"; then\n\
        AC_DEFINE(HAVE_ICONV_WITH_CONST_INPUT)\n\
    fi\n\
fi\n\
LIBS=$_SAVE_LIBS\n\
\n\
    ;;\n\
esac\n\
\n\
AM_LANGINFO_CODESET\n\
\n\
AC_LANG_C\n\
\n\
dnl **********************\n\
dnl *** va_copy checks ***\n\
dnl **********************\n\
dnl we currently check for all three va_copy possibilities, so we get\n\
dnl all results in config.log for bug reports.\n\
AC_MSG_CHECKING(for an implementation of va_copy())\n\
AC_CACHE_VAL(ac_cv_va_copy,[\n\
    AC_TRY_RUN([\n\
        #include <stdarg.h>\n\
        void f (int i, ...) {\n\
            va_list args1, args2;\n\
            va_start (args1, i);\n\
            va_copy (args2, args1);\n\
            if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)\n\
                exit (1);\n\
            va_end (args1); va_end (args2);\n\
        }\n\
        int main() { f (0, 42); return 0; }],\n\
        ac_cv_va_copy=yes,\n\
        ac_cv_va_copy=no,\n\
        ac_cv_va_copy=no\n\
    )\n\
])\n\
AC_MSG_RESULT($ac_cv_va_copy)\n\
AC_MSG_CHECKING(for an implementation of __va_copy())\n\
AC_CACHE_VAL(ac_cv___va_copy,[\n\
    AC_TRY_RUN([\n\
        #include <stdarg.h>\n\
        void f (int i, ...) {\n\
            va_list args1, args2;\n\
            va_start (args1, i);\n\
            __va_copy (args2, args1);\n\
            if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)\n\
                exit (1);\n\
            va_end (args1); va_end (args2);\n\
        }\n\
        int main() { f (0, 42); return 0; }],\n\
        ac_cv___va_copy=yes,\n\
        ac_cv___va_copy=no,\n\
        ac_cv___va_copy=no\n\
    )\n\
])\n\
AC_MSG_RESULT($ac_cv___va_copy)\n\
AC_MSG_CHECKING(whether va_lists can be copied by value)\n\
AC_CACHE_VAL(ac_cv_va_val_copy,[\n\
    AC_TRY_RUN([\n\
        #include <stdarg.h>\n\
        void f (int i, ...) {\n\
            va_list args1, args2;\n\
            va_start (args1, i);\n\
            args2 = args1;\n\
            if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)\n\
                exit (1);\n\
            va_end (args1); va_end (args2);\n\
        }\n\
        int main() { f (0, 42); return 0; }],\n\
        ac_cv_va_val_copy=yes,\n\
        ac_cv_va_val_copy=no,\n\
        ac_cv_va_val_copy=yes\n\
    )\n\
])\n\
if test \"x$ac_cv_va_copy\" = \"xyes\"; then\n\
    AC_DEFINE(VA_COPY, va_copy)\n\
    AC_DEFINE(HAVE_VA_COPY)\n\
elif test \"x$ac_cv___va_copy\" = \"xyes\"; then\n\
    AC_DEFINE(VA_COPY, __va_copy)\n\
    AC_DEFINE(HAVE_VA_COPY)\n\
fi\n\
\n\
if test \"x$ac_cv_va_val_copy\" = \"xno\"; then\n\
   AC_DEFINE(HAVE_VA_LIST_AS_ARRAY)\n\
fi\n\
AC_MSG_RESULT($ac_cv_va_val_copy)\n\
\n\
dnl Check for dll-challenged libc\'s.\n\
dnl This check is apparently only needed for Linux.\n\
case \"$target\" in\n\
	*-linux*)\n\
	    dnl ===================================================================\n\
	    _curdir=`pwd`\n\
	    export _curdir\n\
	    rm -rf conftest* _conftest\n\
	    mkdir _conftest\n\
	    cat >> conftest.C <<\\EOF\n\
#include <stdio.h>\n\
#include <link.h>\n\
#include <dlfcn.h>\n\
#ifdef _dl_loaded\n\
void __dump_link_map(void) {\n\
  struct link_map *map = _dl_loaded;\n\
  while (NULL != map) {printf(\"0x%08x %s\\n\", map->l_addr, map->l_name); map = map->l_next;}\n\
}\n\
int main() {\n\
  dlopen(\"./conftest1.so\",RTLD_LAZY);\n\
  dlopen(\"./../_conftest/conftest1.so\",RTLD_LAZY);\n\
  dlopen(\"CURDIR/_conftest/conftest1.so\",RTLD_LAZY);\n\
  dlopen(\"CURDIR/_conftest/../_conftest/conftest1.so\",RTLD_LAZY);\n\
  __dump_link_map();\n\
}\n\
#else\n\
/* _dl_loaded isn\'t defined, so this should be either a libc5 (glibc1) system, or a glibc2 system that doesn\'t have the multiple load bug (i.e., RH6.0).*/\n\
int main() { printf(\"./conftest1.so\\n\"); }\n\
#endif\n\
EOF\n\
\n\
	    $PERL -p -i -e \"s/CURDIR/\\$ENV{_curdir}/g;\" conftest.C\n\
\n\
	    cat >> conftest1.C <<\\EOF\n\
#include <stdio.h>\n\
void foo(void) {printf(\"foo in dll called\\n\");}\n\
EOF\n\
	    ${CXX-g++} -fPIC -c -g conftest1.C\n\
	    ${CXX-g++} -shared -Wl,-h -Wl,conftest1.so -o conftest1.so conftest1.o\n\
	    ${CXX-g++} -g conftest.C -o conftest -ldl\n\
	    cp -f conftest1.so conftest _conftest\n\
	    cd _conftest\n\
	    if test `./conftest | grep conftest1.so | wc -l` -gt 1\n\
	    then\n\
		echo\n\
		echo \"*** Your libc has a bug that can result in loading the same dynamic\"\n\
		echo \"*** library multiple times.  This bug is known to be fixed in glibc-2.0.7-32\"\n\
		echo \"*** or later.  However, if you choose not to upgrade, the only effect\"\n\
		echo \"*** will be excessive memory usage at runtime.\"\n\
		echo\n\
	    fi\n\
	    cd ${_curdir}\n\
	    rm -rf conftest* _conftest\n\
	    dnl ===================================================================\n\
	    ;;\n\
esac\n\
\n\
dnl ===================================================================\n\
dnl ========================================================\n\
dnl By default, turn rtti and exceptions off on g++/egcs\n\
dnl ========================================================\n\
if test \"$GNU_CXX\"; then\n\
\n\
  AC_MSG_CHECKING(for C++ exceptions flag)\n\
\n\
  dnl They changed -f[no-]handle-exceptions to -f[no-]exceptions in g++ 2.8\n\
  AC_CACHE_VAL(ac_cv_cxx_exceptions_flags,\n\
  [echo \"int main() { return 0; }\" | cat > conftest.C\n\
\n\
  ${CXX-g++} ${CXXFLAGS} -c -fno-handle-exceptions conftest.C > conftest.out 2>&1\n\
\n\
  if egrep \"warning.*renamed\" conftest.out >/dev/null; then\n\
    ac_cv_cxx_exceptions_flags=${_COMPILER_PREFIX}-fno-exceptions\n\
  else\n\
    ac_cv_cxx_exceptions_flags=${_COMPILER_PREFIX}-fno-handle-exceptions\n\
  fi\n\
\n\
  rm -f conftest*])\n\
\n\
  AC_MSG_RESULT($ac_cv_cxx_exceptions_flags)\n\
  _MOZ_EXCEPTIONS_FLAGS_OFF=$ac_cv_cxx_exceptions_flags\n\
  _MOZ_EXCEPTIONS_FLAGS_ON=`echo $ac_cv_cxx_exceptions_flags | sed \'s|no-||\'`\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Put your C++ language/feature checks below\n\
dnl ========================================================\n\
AC_LANG_CPLUSPLUS\n\
\n\
HAVE_GCC3_ABI=\n\
if test \"$GNU_CC\"; then\n\
  AC_CACHE_CHECK(for gcc 3.0 ABI,\n\
      ac_cv_gcc_three_abi,\n\
      [AC_TRY_COMPILE([],\n\
                      [\n\
#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100 /* G++ V3 ABI */\n\
  return 0;\n\
#else\n\
#error Not gcc3.\n\
#endif\n\
                      ],\n\
                      ac_cv_gcc_three_abi=\"yes\",\n\
                      ac_cv_gcc_three_abi=\"no\")])\n\
  if test \"$ac_cv_gcc_three_abi\" = \"yes\"; then\n\
      TARGET_COMPILER_ABI=\"${TARGET_COMPILER_ABI-gcc3}\"\n\
      HAVE_GCC3_ABI=1\n\
  else\n\
      TARGET_COMPILER_ABI=\"${TARGET_COMPILER_ABI-gcc2}\"\n\
  fi\n\
fi\n\
AC_SUBST(HAVE_GCC3_ABI)\n\
\n\
\n\
AC_CACHE_CHECK(for C++ \\\"explicit\\\" keyword,\n\
               ac_cv_cpp_explicit,\n\
               [AC_TRY_COMPILE(class X {\n\
                               public: explicit X(int i) : i_(i) {}\n\
                               private: int i_;\n\
                               };,\n\
                               X x(3);,\n\
                               ac_cv_cpp_explicit=yes,\n\
                               ac_cv_cpp_explicit=no)])\n\
if test \"$ac_cv_cpp_explicit\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_EXPLICIT)\n\
fi\n\
\n\
AC_CACHE_CHECK(for C++ \\\"typename\\\" keyword,\n\
               ac_cv_cpp_typename,\n\
               [AC_TRY_COMPILE(class param {\n\
                               public:\n\
                                   typedef unsigned long num_type;\n\
                               };\n\
\n\
                               template <class T> class tplt {\n\
                               public:\n\
                                   typedef typename T::num_type t_num_type;\n\
                                   t_num_type foo(typename T::num_type num) {\n\
                                       return num;\n\
                                   }\n\
                               };,\n\
                               tplt<param> A;\n\
                               A.foo(0);,\n\
                               ac_cv_cpp_typename=yes,\n\
                               ac_cv_cpp_typename=no)])\n\
if test \"$ac_cv_cpp_typename\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_TYPENAME)\n\
fi\n\
\n\
dnl Check for support of modern template specialization syntax\n\
dnl Test code and requirement from scc@netscape.com.\n\
dnl Autoconf cut-and-paste job by waterson@netscape.com\n\
AC_CACHE_CHECK(for modern C++ template specialization syntax support,\n\
               ac_cv_cpp_modern_specialize_template_syntax,\n\
               [AC_TRY_COMPILE(template <class T> struct X { int a; };\n\
                               class Y {};\n\
                               template <> struct X<Y> { double a; };,\n\
                               X<int> int_x;\n\
                               X<Y> y_x;,\n\
                               ac_cv_cpp_modern_specialize_template_syntax=yes,\n\
                               ac_cv_cpp_modern_specialize_template_syntax=no)])\n\
if test \"$ac_cv_cpp_modern_specialize_template_syntax\" = yes ; then\n\
  AC_DEFINE(HAVE_CPP_MODERN_SPECIALIZE_TEMPLATE_SYNTAX)\n\
fi\n\
\n\
\n\
dnl Some compilers support only full specialization, and some don\'t.\n\
AC_CACHE_CHECK(whether partial template specialization works,\n\
               ac_cv_cpp_partial_specialization,\n\
               [AC_TRY_COMPILE(template <class T> class Foo {};\n\
                               template <class T> class Foo<T*> {};,\n\
                               return 0;,\n\
                               ac_cv_cpp_partial_specialization=yes,\n\
                               ac_cv_cpp_partial_specialization=no)])\n\
if test \"$ac_cv_cpp_partial_specialization\" = yes ; then\n\
  AC_DEFINE(HAVE_CPP_PARTIAL_SPECIALIZATION)\n\
fi\n\
\n\
dnl Some compilers have limited support for operators with templates;\n\
dnl specifically, it is necessary to define derived operators when a base\n\
dnl class\'s operator declaration should suffice.\n\
AC_CACHE_CHECK(whether operators must be re-defined for templates derived from templates,\n\
               ac_cv_need_derived_template_operators,\n\
               [AC_TRY_COMPILE([template <class T> class Base { };\n\
                                template <class T>\n\
                                Base<T> operator+(const Base<T>& lhs, const Base<T>& rhs) { return lhs; }\n\
                                template <class T> class Derived : public Base<T> { };],\n\
                               [Derived<char> a, b;\n\
                                Base<char> c = a + b;\n\
                                return 0;],\n\
                               ac_cv_need_derived_template_operators=no,\n\
                               ac_cv_need_derived_template_operators=yes)])\n\
if test \"$ac_cv_need_derived_template_operators\" = yes ; then\n\
  AC_DEFINE(NEED_CPP_DERIVED_TEMPLATE_OPERATORS)\n\
fi\n\
\n\
\n\
dnl Some compilers have trouble detecting that a template class\n\
dnl that derives from another template is actually an instance\n\
dnl of the base class. This test checks for that.\n\
AC_CACHE_CHECK(whether we need to cast a derived template to pass as its base class,\n\
               ac_cv_need_cpp_template_cast_to_base,\n\
               [AC_TRY_COMPILE([template <class T> class Base { };\n\
                                template <class T> class Derived : public Base<T> { };\n\
                                template <class T> int foo(const Base<T>&) { return 0; }],\n\
                               [Derived<char> bar; return foo(bar);],\n\
                               ac_cv_need_cpp_template_cast_to_base=no,\n\
                               ac_cv_need_cpp_template_cast_to_base=yes)])\n\
if test \"$ac_cv_need_cpp_template_cast_to_base\" = yes ; then\n\
  AC_DEFINE(NEED_CPP_TEMPLATE_CAST_TO_BASE)\n\
fi\n\
\n\
dnl Some compilers have trouble resolving the ambiguity between two\n\
dnl functions whose arguments differ only by cv-qualifications.\n\
AC_CACHE_CHECK(whether the compiler can resolve const ambiguities for templates,\n\
               ac_cv_can_resolve_const_ambiguity,\n\
               [AC_TRY_COMPILE([\n\
                                template <class T> class ptrClass {\n\
                                  public: T* ptr;\n\
                                };\n\
\n\
                                template <class T> T* a(ptrClass<T> *arg) {\n\
                                  return arg->ptr;\n\
                                }\n\
\n\
                                template <class T>\n\
                                const T* a(const ptrClass<T> *arg) {\n\
                                  return arg->ptr;\n\
                                }\n\
                               ],\n\
                               [ ptrClass<int> i;\n\
                                 a(&i); ],\n\
                               ac_cv_can_resolve_const_ambiguity=yes,\n\
                               ac_cv_can_resolve_const_ambiguity=no)])\n\
if test \"$ac_cv_can_resolve_const_ambiguity\" = no ; then\n\
  AC_DEFINE(CANT_RESOLVE_CPP_CONST_AMBIGUITY)\n\
fi\n\
\n\
dnl\n\
dnl We don\'t do exceptions on unix.  The only reason this used to be here\n\
dnl is that mozilla/xpcom/tests/TestCOMPtr.cpp has a test which uses \n\
dnl exceptions.  But, we turn exceptions off by default and this test breaks.\n\
dnl So im commenting this out until someone writes some artificial \n\
dnl intelligence to detect not only if the compiler has exceptions, but if \n\
dnl they are enabled as well.\n\
dnl \n\
dnl AC_CACHE_CHECK(for C++ \\\"exceptions\\\",\n\
dnl                ac_cv_cpp_exceptions,\n\
dnl                [AC_TRY_COMPILE(class X { public: X() {} };\n\
dnl                                static void F() { throw X(); },\n\
dnl                                try { F(); } catch(X & e) { },\n\
dnl                                ac_cv_cpp_exceptions=yes,\n\
dnl                                ac_cv_cpp_exceptions=no)])\n\
dnl if test $ac_cv_cpp_exceptions = yes ; then\n\
dnl    AC_DEFINE(HAVE_CPP_EXCEPTIONS)\n\
dnl fi\n\
\n\
dnl Some compilers have marginal |using| support; for example, gcc-2.7.2.3\n\
dnl supports it well enough to allow us to use it to change access, but not\n\
dnl to resolve ambiguity. The next two tests determine how well the |using|\n\
dnl keyword is supported.\n\
dnl\n\
dnl Check to see if we can change access with |using|.  Test both a\n\
dnl legal and an illegal example.\n\
AC_CACHE_CHECK(whether the C++ \\\"using\\\" keyword can change access,\n\
               ac_cv_cpp_access_changing_using2,\n\
               [AC_TRY_COMPILE(\n\
                   class A { protected: int foo() { return 0; } };\n\
                   class B : public A { public: using A::foo; };,\n\
                   B b; return b.foo();,\n\
                   [AC_TRY_COMPILE(\n\
                       class A { public: int foo() { return 1; } };\n\
                       class B : public A { private: using A::foo; };,\n\
                       B b; return b.foo();,\n\
                       ac_cv_cpp_access_changing_using2=no,\n\
                       ac_cv_cpp_access_changing_using2=yes)],\n\
                   ac_cv_cpp_access_changing_using2=no)])\n\
if test \"$ac_cv_cpp_access_changing_using2\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_ACCESS_CHANGING_USING)\n\
fi\n\
\n\
dnl Check to see if we can resolve ambiguity with |using|.\n\
AC_CACHE_CHECK(whether the C++ \\\"using\\\" keyword resolves ambiguity,\n\
               ac_cv_cpp_ambiguity_resolving_using,\n\
               [AC_TRY_COMPILE(class X { \n\
                                 public: int go(const X&) {return 3;}\n\
                                         int jo(const X&) {return 3;}\n\
                               };\n\
                               class Y : public X {\n\
                                 public:  int go(int) {return 2;}\n\
                                          int jo(int) {return 2;}\n\
                                          using X::jo;\n\
                                 private: using X::go;\n\
                               };,\n\
                               X x; Y y; y.jo(x);,\n\
                               ac_cv_cpp_ambiguity_resolving_using=yes,\n\
                               ac_cv_cpp_ambiguity_resolving_using=no)])\n\
if test \"$ac_cv_cpp_ambiguity_resolving_using\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_AMBIGUITY_RESOLVING_USING)\n\
fi\n\
\n\
dnl Check to see if the |std| namespace is supported. If so, we\'ll want\n\
dnl to qualify any standard library calls with \"std::\" to ensure that\n\
dnl those functions can be resolved.\n\
AC_CACHE_CHECK(for \\\"std::\\\" namespace,\n\
               ac_cv_cpp_namespace_std,\n\
               [AC_TRY_COMPILE([#include <algorithm>],\n\
                               [return std::min(0, 1);],\n\
                               ac_cv_cpp_namespace_std=yes,\n\
                               ac_cv_cpp_namespace_std=no)])\n\
if test \"$ac_cv_cpp_namespace_std\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_NAMESPACE_STD)\n\
fi\n\
\n\
dnl Older compilers are overly ambitious with respect to using the standard\n\
dnl template library\'s |operator!=()| when |operator==()| is defined. In\n\
dnl which case, defining |operator!=()| in addition to |operator==()| causes\n\
dnl ambiguity at compile-time. This test checks for that case.\n\
AC_CACHE_CHECK(whether standard template operator!=() is ambiguous,\n\
               ac_cv_cpp_unambiguous_std_notequal,\n\
               [AC_TRY_COMPILE([#include <algorithm>\n\
                                struct T1 {};\n\
                                int operator==(const T1&, const T1&) { return 0; }\n\
                                int operator!=(const T1&, const T1&) { return 0; }],\n\
                               [T1 a,b; return a != b;],\n\
                               ac_cv_cpp_unambiguous_std_notequal=unambiguous,\n\
                               ac_cv_cpp_unambiguous_std_notequal=ambiguous)])\n\
if test \"$ac_cv_cpp_unambiguous_std_notequal\" = unambiguous ; then\n\
  AC_DEFINE(HAVE_CPP_UNAMBIGUOUS_STD_NOTEQUAL)\n\
fi\n\
\n\
\n\
AC_CACHE_CHECK(for C++ reinterpret_cast,\n\
               ac_cv_cpp_reinterpret_cast,\n\
               [AC_TRY_COMPILE(struct X { int i; };\n\
                               struct Y { int i; };,\n\
                               X x; X*const z = &x;Y*y = reinterpret_cast<Y*>(z);,\n\
                               ac_cv_cpp_reinterpret_cast=yes,\n\
                               ac_cv_cpp_reinterpret_cast=no)])\n\
if test \"$ac_cv_cpp_reinterpret_cast\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_NEW_CASTS)\n\
fi\n\
\n\
dnl See if a dynamic_cast to void* gives the most derived object.\n\
AC_CACHE_CHECK(for C++ dynamic_cast to void*,\n\
               ac_cv_cpp_dynamic_cast_void_ptr,\n\
               [AC_TRY_RUN([class X { int i; public: virtual ~X() { } };\n\
                            class Y { int j; public: virtual ~Y() { } };\n\
                            class Z : public X, public Y { int k; };\n\
\n\
                            int main() {\n\
                                 Z mdo;\n\
                                 X *subx = (X*)&mdo;\n\
                                 Y *suby = (Y*)&mdo;\n\
                                 return !((((void*)&mdo != (void*)subx) &&\n\
                                           ((void*)&mdo == dynamic_cast<void*>(subx))) ||\n\
                                          (((void*)&mdo != (void*)suby) &&\n\
                                           ((void*)&mdo == dynamic_cast<void*>(suby))));\n\
                            }],\n\
                           ac_cv_cpp_dynamic_cast_void_ptr=yes,\n\
                           ac_cv_cpp_dynamic_cast_void_ptr=no,\n\
                           ac_cv_cpp_dynamic_cast_void_ptr=no)])\n\
if test \"$ac_cv_cpp_dynamic_cast_void_ptr\" = yes ; then\n\
   AC_DEFINE(HAVE_CPP_DYNAMIC_CAST_TO_VOID_PTR)\n\
fi\n\
\n\
\n\
dnl note that this one is reversed - if the test fails, then\n\
dnl we require implementations of unused virtual methods. Which\n\
dnl really blows because it means we\'ll have useless vtable\n\
dnl bloat.\n\
AC_CACHE_CHECK(whether C++ requires implementation of unused virtual methods,\n\
               ac_cv_cpp_unused_required,\n\
               [AC_TRY_LINK(class X {private: virtual void never_called();};,\n\
                               X x;,\n\
                               ac_cv_cpp_unused_required=no,\n\
                               ac_cv_cpp_unused_required=yes)])\n\
if test \"$ac_cv_cpp_unused_required\" = yes ; then\n\
   AC_DEFINE(NEED_CPP_UNUSED_IMPLEMENTATIONS)\n\
fi\n\
\n\
\n\
dnl Some compilers have trouble comparing a constant reference to a templatized\n\
dnl class to zero, and require an explicit operator==() to be defined that takes\n\
dnl an int. This test separates the strong from the weak.\n\
\n\
AC_CACHE_CHECK(for trouble comparing to zero near std::operator!=(),\n\
               ac_cv_trouble_comparing_to_zero,\n\
               [AC_TRY_COMPILE([#include <algorithm>\n\
                                template <class T> class Foo {};\n\
                                class T2;\n\
                                template <class T> int operator==(const T2*, const T&) { return 0; }\n\
                                template <class T> int operator!=(const T2*, const T&) { return 0; }],\n\
                               [Foo<int> f; return (0 != f);],\n\
                               ac_cv_trouble_comparing_to_zero=no,\n\
                               ac_cv_trouble_comparing_to_zero=yes)])\n\
if test \"$ac_cv_trouble_comparing_to_zero\" = yes ; then\n\
  AC_DEFINE(HAVE_CPP_TROUBLE_COMPARING_TO_ZERO)\n\
fi\n\
\n\
\n\
\n\
dnl End of C++ language/feature checks\n\
AC_LANG_C\n\
\n\
dnl ========================================================\n\
dnl =  Internationalization checks\n\
dnl ========================================================\n\
dnl\n\
dnl Internationalization and Locale support is different\n\
dnl on various UNIX platforms.  Checks for specific i18n\n\
dnl features go here.\n\
\n\
dnl check for LC_MESSAGES\n\
AC_CACHE_CHECK(for LC_MESSAGES,\n\
		ac_cv_i18n_lc_messages,\n\
		[AC_TRY_COMPILE([#include <locale.h>],\n\
				[int category = LC_MESSAGES;],\n\
				ac_cv_i18n_lc_messages=yes,\n\
				ac_cv_i18n_lc_messages=no)])\n\
if test \"$ac_cv_i18n_lc_messages\" = yes; then\n\
   AC_DEFINE(HAVE_I18N_LC_MESSAGES)\n\
fi 	\n\
\n\
fi # SKIP_COMPILER_CHECKS\n\
\n\
TARGET_XPCOM_ABI=\n\
if test -n \"${CPU_ARCH}\" -a -n \"${TARGET_COMPILER_ABI}\"; then\n\
    TARGET_XPCOM_ABI=\"${CPU_ARCH}-${TARGET_COMPILER_ABI}\"\n\
fi\n\
\n\
dnl Mozilla specific options\n\
dnl ========================================================\n\
dnl The macros used for command line options\n\
dnl are defined in build/autoconf/altoptions.m4.\n\
\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Check for external package dependencies\n\
dnl =\n\
dnl ========================================================\n\
MOZ_ARG_HEADER(External Packages)\n\
\n\
MOZ_ENABLE_LIBXUL=\n\
\n\
MOZ_ARG_WITH_STRING(libxul-sdk,\n\
[  --with-libxul-sdk=PFX   Use the libXUL SDK at <PFX>],\n\
  LIBXUL_SDK_DIR=$withval)\n\
\n\
if test \"$LIBXUL_SDK_DIR\" = \"yes\"; then\n\
    AC_MSG_ERROR([--with-libxul-sdk must specify a path])\n\
elif test -n \"$LIBXUL_SDK_DIR\" -a \"$LIBXUL_SDK_DIR\" != \"no\"; then\n\
    LIBXUL_SDK=`cd \"$LIBXUL_SDK_DIR\" && pwd`\n\
\n\
    if test ! -f \"$LIBXUL_SDK/sdk/include/xpcom-config.h\"; then\n\
        AC_MSG_ERROR([$LIBXUL_SDK/sdk/include/xpcom-config.h doesn\'t exist])\n\
    fi\n\
\n\
    MOZ_ENABLE_LIBXUL=1\n\
fi\n\
AC_SUBST(LIBXUL_SDK)\n\
\n\
dnl ========================================================\n\
dnl = If NSPR was not detected in the system, \n\
dnl = use the one in the source tree (mozilla/nsprpub)\n\
dnl ========================================================\n\
MOZ_ARG_WITH_BOOL(system-nspr,\n\
[  --with-system-nspr      Use system installed NSPR],\n\
    _USE_SYSTEM_NSPR=1 )\n\
\n\
if test -n \"$_USE_SYSTEM_NSPR\"; then\n\
    AM_PATH_NSPR(4.0.0, [MOZ_NATIVE_NSPR=1], [MOZ_NATIVE_NSPR=])\n\
fi\n\
\n\
if test -z \"$MOZ_NATIVE_NSPR\"; then\n\
    NSPR_CFLAGS=\'`$(DEPTH)/nsprpub/config/nspr-config --prefix=$(LIBXUL_DIST) --includedir=$(LIBXUL_DIST)/include/nspr --cflags`\'\n\
    # explicitly set libs for Visual Age C++ for OS/2\n\
    if test \"$OS_ARCH\" = \"OS2\" -a \"$VACPP\" = \"yes\"; then\n\
        NSPR_LIBS=\'$(LIBXUL_DIST)/lib/nspr\'$NSPR_VERSION\'.lib $(LIBXUL_DIST)/lib/plc\'$NSPR_VERSION\'.lib $(LIBXUL_DIST)/lib/plds\'$NSPR_VERSION\'.lib \'$_PTHREAD_LDFLAGS\'\'\n\
    elif test \"$OS_ARCH\" = \"WINCE\"; then\n\
        NSPR_CFLAGS=\'-I$(LIBXUL_DIST)/include/nspr\'\n\
        NSPR_LIBS=\'$(LIBXUL_DIST)/lib/nspr\'$NSPR_VERSION\'.lib $(LIBXUL_DIST)/lib/plc\'$NSPR_VERSION\'.lib $(LIBXUL_DIST)/lib/plds\'$NSPR_VERSION\'.lib \'\n\
    elif test \"$OS_ARCH\" = \"WINNT\"; then\n\
        NSPR_CFLAGS=\'-I$(LIBXUL_DIST)/include/nspr\'\n\
        if test -n \"$GNU_CC\"; then\n\
            NSPR_LIBS=\"-L\\$(LIBXUL_DIST)/lib -lnspr$NSPR_VERSION -lplc$NSPR_VERSION -lplds$NSPR_VERSION\"\n\
        else\n\
            NSPR_LIBS=\'$(LIBXUL_DIST)/lib/nspr\'$NSPR_VERSION\'.lib $(LIBXUL_DIST)/lib/plc\'$NSPR_VERSION\'.lib $(LIBXUL_DIST)/lib/plds\'$NSPR_VERSION\'.lib \'\n\
        fi\n\
    else\n\
        NSPR_LIBS=\'`$(DEPTH)/nsprpub/config/nspr-config --prefix=$(LIBXUL_DIST) --libdir=$(LIBXUL_DIST)/lib --libs`\'\n\
    fi\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = If NSS was not detected in the system, \n\
dnl = use the one in the source tree (mozilla/security/nss)\n\
dnl ========================================================\n\
\n\
MOZ_ARG_WITH_BOOL(system-nss,\n\
[  --with-system-nss      Use system installed NSS],\n\
    _USE_SYSTEM_NSS=1 )\n\
\n\
if test -n \"$_USE_SYSTEM_NSS\"; then\n\
    AM_PATH_NSS(3.0.0, [MOZ_NATIVE_NSS=1], [MOZ_NATIVE_NSS=])\n\
fi\n\
\n\
if test -n \"$MOZ_NATIVE_NSS\"; then\n\
   NSS_LIBS=\"$NSS_LIBS -lcrmf\"\n\
else\n\
   NSS_CFLAGS=\'-I$(LIBXUL_DIST)/public/nss\'\n\
   NSS_DEP_LIBS=\'\\\\\\\n\
        $(LIBXUL_DIST)/lib/$(LIB_PREFIX)crmf.$(LIB_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(DLL_PREFIX)smime\'$NSS_VERSION\'$(DLL_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(DLL_PREFIX)ssl\'$NSS_VERSION\'$(DLL_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(DLL_PREFIX)nss\'$NSS_VERSION\'$(DLL_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(DLL_PREFIX)softokn\'$NSS_VERSION\'$(DLL_SUFFIX)\'\n\
\n\
   if test -z \"$GNU_CC\" && test \"$OS_ARCH\" = \"WINNT\" -o \"$OS_ARCH\" = \"WINCE\" -o \"$OS_ARCH\" = \"OS2\"; then\n\
       NSS_LIBS=\'\\\\\\\n\
        $(LIBXUL_DIST)/lib/$(LIB_PREFIX)crmf.$(LIB_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(LIB_PREFIX)smime\'$NSS_VERSION\'.$(IMPORT_LIB_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(LIB_PREFIX)ssl\'$NSS_VERSION\'.$(IMPORT_LIB_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(LIB_PREFIX)nss\'$NSS_VERSION\'.$(IMPORT_LIB_SUFFIX) \\\\\\\n\
        $(LIBXUL_DIST)/lib/$(LIB_PREFIX)softokn\'$NSS_VERSION\'.$(IMPORT_LIB_SUFFIX)\'\n\
   else\n\
       NSS_LIBS=\'$(LIBS_DIR)\'\" -lcrmf -lsmime$NSS_VERSION -lssl$NSS_VERSION -lnss$NSS_VERSION -lsoftokn$NSS_VERSION\"\n\
   fi\n\
fi\n\
\n\
if test -z \"$SKIP_LIBRARY_CHECKS\"; then\n\
dnl system JPEG support\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(system-jpeg,\n\
[  --with-system-jpeg[=PFX]\n\
                          Use system libjpeg [installed at prefix PFX]],\n\
    JPEG_DIR=$withval)\n\
\n\
_SAVE_CFLAGS=$CFLAGS\n\
_SAVE_LDFLAGS=$LDFLAGS\n\
_SAVE_LIBS=$LIBS\n\
if test -n \"${JPEG_DIR}\" -a \"${JPEG_DIR}\" != \"yes\"; then\n\
    CFLAGS=\"-I${JPEG_DIR}/include $CFLAGS\"\n\
    LDFLAGS=\"-L${JPEG_DIR}/lib $LDFLAGS\"\n\
fi\n\
if test -z \"$JPEG_DIR\" -o \"$JPEG_DIR\" = no; then\n\
    SYSTEM_JPEG=\n\
else\n\
    AC_CHECK_LIB(jpeg, jpeg_destroy_compress, [SYSTEM_JPEG=1 JPEG_LIBS=\"-ljpeg $JPEG_LIBS\"], SYSTEM_JPEG=, $JPEG_LIBS)\n\
fi\n\
\n\
if test \"$SYSTEM_JPEG\" = 1; then\n\
    LIBS=\"$JPEG_LIBS $LIBS\"\n\
    AC_TRY_COMPILE([ #include <stdio.h>\n\
                     #include <sys/types.h>\n\
                     #include <jpeglib.h> ],\n\
                   [ #if JPEG_LIB_VERSION < $MOZJPEG\n\
                     #error \"Insufficient JPEG library version ($MOZJPEG required).\"\n\
                     #endif ],\n\
                   SYSTEM_JPEG=1,\n\
                   [SYSTEM_JPEG= JPEG_CFLAGS= JPEG_LIBS=]) \n\
fi \n\
CFLAGS=$_SAVE_CFLAGS\n\
LDFLAGS=$_SAVE_LDFLAGS\n\
LIBS=$_SAVE_LIBS\n\
\n\
if test -n \"${JPEG_DIR}\" -a -d \"${JPEG_DIR}\" -a \"$SYSTEM_JPEG\" = 1; then\n\
    JPEG_CFLAGS=\"-I${JPEG_DIR}/include\"\n\
    JPEG_LIBS=\"-L${JPEG_DIR}/lib ${JPEG_LIBS}\"\n\
fi\n\
\n\
dnl system ZLIB support\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(system-zlib,\n\
[  --with-system-zlib[=PFX]\n\
                          Use system libz [installed at prefix PFX]],\n\
    ZLIB_DIR=$withval)\n\
\n\
_SAVE_CFLAGS=$CFLAGS\n\
_SAVE_LDFLAGS=$LDFLAGS\n\
_SAVE_LIBS=$LIBS\n\
if test -n \"${ZLIB_DIR}\" -a \"${ZLIB_DIR}\" != \"yes\"; then\n\
    CFLAGS=\"-I${ZLIB_DIR}/include $CFLAGS\"\n\
    LDFLAGS=\"-L${ZLIB_DIR}/lib $LDFLAGS\"\n\
fi\n\
if test -z \"$ZLIB_DIR\" -o \"$ZLIB_DIR\" = no; then\n\
    SYSTEM_ZLIB=\n\
else\n\
    AC_CHECK_LIB(z, gzread, [SYSTEM_ZLIB=1 ZLIB_LIBS=\"-lz $ZLIB_LIBS\"], \n\
	[SYSTEM_ZLIB= ZLIB_CFLAGS= ZLIB_LIBS=], $ZLIB_LIBS)\n\
fi\n\
if test \"$SYSTEM_ZLIB\" = 1; then\n\
    LIBS=\"$ZLIB_LIBS $LIBS\"\n\
    AC_TRY_COMPILE([ #include <stdio.h>\n\
                     #include <string.h>\n\
                     #include <zlib.h> ],\n\
                   [ #if ZLIB_VERNUM < $MOZZLIB \n\
                     #error \"Insufficient zlib version ($MOZZLIB required).\"\n\
                     #endif ],\n\
                   SYSTEM_ZLIB=1,\n\
                   [SYSTEM_ZLIB= ZLIB_CFLAGS= ZLIB_LIBS=]) \n\
fi\n\
CFLAGS=$_SAVE_CFLAGS\n\
LDFLAGS=$_SAVE_LDFLAGS\n\
LIBS=$_SAVE_LIBS\n\
\n\
if test \"${ZLIB_DIR}\" -a -d \"${ZLIB_DIR}\" -a \"$SYSTEM_ZLIB\" = 1; then\n\
    ZLIB_CFLAGS=\"-I${ZLIB_DIR}/include\"\n\
    ZLIB_LIBS=\"-L${ZLIB_DIR}/lib ${ZLIB_LIBS}\"\n\
fi\n\
\n\
dnl system PNG Support\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(system-png, \n\
[  --with-system-png[=PFX]\n\
                          Use system libpng [installed at prefix PFX]],\n\
    PNG_DIR=$withval)\n\
\n\
_SAVE_CFLAGS=$CFLAGS\n\
_SAVE_LDFLAGS=$LDFLAGS\n\
_SAVE_LIBS=$LIBS\n\
CFLAGS=\"$ZLIB_CFLAGS $CFLAGS\"\n\
LDFLAGS=\"$ZLIB_LIBS -lz $LDFLAGS\"\n\
if test -n \"${PNG_DIR}\" -a \"${PNG_DIR}\" != \"yes\"; then\n\
    CFLAGS=\"-I${PNG_DIR}/include $CFLAGS\"\n\
    LDFLAGS=\"-L${PNG_DIR}/lib $LDFLAGS\"\n\
fi\n\
if test -z \"$PNG_DIR\" -o \"$PNG_DIR\" = no; then\n\
    SYSTEM_PNG=\n\
else\n\
    _SAVE_PNG_LIBS=$PNG_LIBS\n\
    AC_CHECK_LIB(png, png_get_valid, [SYSTEM_PNG=1 PNG_LIBS=\"-lpng $PNG_LIBS\"],\n\
                 SYSTEM_PNG=, $PNG_LIBS)\n\
    AC_CHECK_LIB(png, png_get_acTl, ,\n\
                 [SYSTEM_PNG= PNG_LIBS=$_SAVE_PNG_LIBS], $_SAVE_PNG_LIBS)\n\
fi\n\
if test \"$SYSTEM_PNG\" = 1; then\n\
    LIBS=\"$PNG_LIBS $LIBS\"\n\
    AC_TRY_COMPILE([ #include <stdio.h>\n\
                     #include <sys/types.h>\n\
                     #include <png.h> ],\n\
                   [ #if PNG_LIBPNG_VER < $MOZPNG\n\
                     #error \"Insufficient libpng version ($MOZPNG required).\"\n\
                     #endif\n\
                     #ifndef PNG_UINT_31_MAX\n\
                     #error \"Insufficient libpng version.\"\n\
                     #endif ],\n\
                   SYSTEM_PNG=1,\n\
                   [SYSTEM_PNG= PNG_CFLAGS= PNG_LIBS=]) \n\
fi\n\
CFLAGS=$_SAVE_CFLAGS\n\
LDFLAGS=$_SAVE_LDFLAGS\n\
LIBS=$_SAVE_LIBS\n\
\n\
if test \"${PNG_DIR}\" -a -d \"${PNG_DIR}\" -a \"$SYSTEM_PNG\" = 1; then\n\
    PNG_CFLAGS=\"-I${PNG_DIR}/include\"\n\
    PNG_LIBS=\"-L${PNG_DIR}/lib ${PNG_LIBS}\"\n\
fi\n\
\n\
fi # SKIP_LIBRARY_CHECKS\n\
\n\
dnl check whether to enable glitz\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(glitz,\n\
[  --enable-glitz          Enable Glitz for use with Cairo],\n\
    MOZ_ENABLE_GLITZ=1,\n\
    MOZ_ENABLE_GLITZ= )\n\
if test \"$MOZ_ENABLE_GLITZ\"; then\n\
    AC_DEFINE(MOZ_ENABLE_GLITZ)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Java SDK support\n\
dnl ========================================================\n\
JAVA_INCLUDE_PATH=\n\
MOZ_ARG_WITH_STRING(java-include-path,\n\
[  --with-java-include-path=dir   Location of Java SDK headers],\n\
    JAVA_INCLUDE_PATH=$withval)\n\
\n\
JAVA_BIN_PATH=\n\
MOZ_ARG_WITH_STRING(java-bin-path,\n\
[  --with-java-bin-path=dir   Location of Java binaries (java, javac, jar)],\n\
    JAVA_BIN_PATH=$withval)\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Application\n\
dnl =\n\
dnl ========================================================\n\
\n\
MOZ_ARG_HEADER(Application)\n\
\n\
BUILD_STATIC_LIBS=\n\
ENABLE_TESTS=1\n\
MOZ_ACTIVEX_SCRIPTING_SUPPORT=\n\
MOZ_BRANDING_DIRECTORY=\n\
MOZ_CALENDAR=\n\
MOZ_DBGRINFO_MODULES=\n\
MOZ_ENABLE_CANVAS=1\n\
MOZ_EXTENSIONS_ALL=\" wallet xml-rpc help p3p venkman inspector irc typeaheadfind gnomevfs sroaming datetime finger cview layout-debug tasks sql xforms schema-validation reporter\"\n\
MOZ_FEEDS=1\n\
MOZ_IMG_DECODERS_DEFAULT=\"png gif jpeg bmp xbm icon\"\n\
MOZ_IMG_ENCODERS_DEFAULT=\"png jpeg\"\n\
MOZ_IPCD=\n\
MOZ_JAVAXPCOM=\n\
MOZ_JSDEBUGGER=1\n\
MOZ_JSLOADER=1\n\
MOZ_LDAP_XPCOM=\n\
MOZ_LIBART_CFLAGS=\n\
MOZ_LIBART_LIBS=\n\
MOZ_MAIL_NEWS=\n\
MOZ_MATHML=1\n\
MOZ_MOCHITEST=1\n\
MOZ_MORK=1\n\
MOZ_MORKREADER=\n\
MOZ_AUTH_EXTENSION=1\n\
MOZ_NO_ACTIVEX_SUPPORT=1\n\
MOZ_NO_INSPECTOR_APIS=\n\
MOZ_NO_XPCOM_OBSOLETE=\n\
MOZ_NO_FAST_LOAD=\n\
MOZ_OJI=1\n\
MOZ_PERMISSIONS=1\n\
MOZ_PLACES=\n\
MOZ_PLAINTEXT_EDITOR_ONLY=\n\
MOZ_PLUGINS=1\n\
MOZ_PREF_EXTENSIONS=1\n\
MOZ_PROFILELOCKING=1\n\
MOZ_PROFILESHARING=1\n\
MOZ_PSM=1\n\
MOZ_PYTHON_EXTENSIONS=\"xpcom dom\"\n\
MOZ_PYTHON=\n\
MOZ_PYTHON_DEBUG_SUFFIX=\n\
MOZ_PYTHON_DLL_SUFFIX=\n\
MOZ_PYTHON_INCLUDES=\n\
MOZ_PYTHON_LIBS=\n\
MOZ_PYTHON_PREFIX=\n\
MOZ_PYTHON_VER=\n\
MOZ_PYTHON_VER_DOTTED=\n\
MOZ_RDF=1\n\
MOZ_REFLOW_PERF=\n\
MOZ_SAFE_BROWSING=\n\
MOZ_SINGLE_PROFILE=\n\
MOZ_SPELLCHECK=1\n\
MOZ_STATIC_MAIL_BUILD=\n\
MOZ_STORAGE=1\n\
MOZ_SVG=1\n\
MOZ_TIMELINE=\n\
MOZ_UI_LOCALE=en-US\n\
MOZ_UNIVERSALCHARDET=1\n\
MOZ_URL_CLASSIFIER=\n\
MOZ_USE_NATIVE_UCONV=\n\
MOZ_V1_STRING_ABI=1\n\
MOZ_VIEW_SOURCE=1\n\
MOZ_WEBSERVICES=1\n\
MOZ_XMLEXTRAS=1\n\
MOZ_XPFE_COMPONENTS=1\n\
MOZ_XPINSTALL=1\n\
MOZ_XSLT_STANDALONE=\n\
MOZ_XTF=1\n\
MOZ_XUL=1\n\
NS_PRINTING=1\n\
NECKO_COOKIES=1\n\
NECKO_DISK_CACHE=1\n\
NECKO_PROTOCOLS_DEFAULT=\"about data file ftp gopher http res viewsource\"\n\
NECKO_SMALL_BUFFERS=\n\
SUNCTL=\n\
JS_STATIC_BUILD=\n\
XPC_IDISPATCH_SUPPORT=\n\
\n\
\n\
case \"$target_os\" in\n\
darwin*)\n\
    ACCESSIBILITY=\n\
    ;;\n\
*)\n\
    ACCESSIBILITY=1\n\
    ;;\n\
esac\n\
\n\
case \"$target_os\" in\n\
    msvc*|mks*|cygwin*|mingw*)\n\
        if test -z \"$GNU_CC\"; then \n\
            XPC_IDISPATCH_SUPPORT=1\n\
            MOZ_NO_ACTIVEX_SUPPORT=\n\
            MOZ_ACTIVEX_SCRIPTING_SUPPORT=1\n\
        fi\n\
        ;;\n\
esac\n\
\n\
MOZ_ARG_ENABLE_STRING(application,\n\
[  --enable-application=APP\n\
                          Options include:\n\
                            suite\n\
                            browser (Firefox)\n\
                            mail (Thunderbird)\n\
                            minimo\n\
                            composer\n\
                            calendar (Sunbird)\n\
                            xulrunner\n\
                            camino\n\
                            content/xslt (Standalone Transformiix XSLT)\n\
                            netwerk (Standalone Necko)\n\
                            tools/update-packaging (AUS-related packaging tools)\n\
                            standalone (use this for standalone\n\
                              xpcom/xpconnect or to manually drive a build)],\n\
[ MOZ_BUILD_APP=$enableval ] )\n\
\n\
if test \"$MOZ_BUILD_APP\" = \"macbrowser\"; then\n\
    MOZ_BUILD_APP=camino\n\
fi\n\
\n\
case \"$MOZ_BUILD_APP\" in\n\
minimo)\n\
  MOZ_EMBEDDING_PROFILE=basic\n\
  ;;\n\
\n\
*)\n\
  MOZ_EMBEDDING_PROFILE=default\n\
  ;;\n\
esac\n\
\n\
MOZ_ARG_WITH_STRING(embedding-profile,\n\
[  --with-embedding-profile=default|basic|minimal\n\
                       see http://wiki.mozilla.org/Gecko:Small_Device_Support],\n\
[ MOZ_EMBEDDING_PROFILE=$withval ])\n\
\n\
case \"$MOZ_EMBEDDING_PROFILE\" in\n\
default)\n\
  MOZ_EMBEDDING_LEVEL_DEFAULT=1\n\
  MOZ_EMBEDDING_LEVEL_BASIC=1\n\
  MOZ_EMBEDDING_LEVEL_MINIMAL=1\n\
  AC_DEFINE(MOZ_EMBEDDING_LEVEL_DEFAULT)\n\
  AC_DEFINE(MOZ_EMBEDDING_LEVEL_BASIC)\n\
  AC_DEFINE(MOZ_EMBEDDING_LEVEL_MINIMAL)\n\
  ;;\n\
\n\
basic)\n\
  MOZ_EMBEDDING_LEVEL_DEFAULT=\n\
  MOZ_EMBEDDING_LEVEL_BASIC=1\n\
  MOZ_EMBEDDING_LEVEL_MINIMAL=1\n\
  AC_DEFINE(MOZ_EMBEDDING_LEVEL_BASIC)\n\
  AC_DEFINE(MOZ_EMBEDDING_LEVEL_MINIMAL)\n\
  ENABLE_TESTS=\n\
  MOZ_ACTIVEX_SCRIPTING_SUPPORT=\n\
  MOZ_COMPOSER=\n\
  MOZ_ENABLE_CANVAS=\n\
  MOZ_ENABLE_POSTSCRIPT=\n\
  MOZ_EXTENSIONS_DEFAULT=\" spatialnavigation\"\n\
  MOZ_IMG_DECODERS_DEFAULT=\"png gif jpeg\"\n\
  MOZ_IMG_ENCODERS_DEFAULT=\n\
  MOZ_IMG_ENCODERS=\n\
  MOZ_INSTALLER=\n\
  MOZ_JSDEBUGGER=\n\
  MOZ_LDAP_XPCOM=\n\
  MOZ_MAIL_NEWS=\n\
  MOZ_MATHML=\n\
  MOZ_AUTH_EXTENSION=\n\
  MOZ_NO_ACTIVEX_SUPPORT=1\n\
  MOZ_NO_INSPECTOR_APIS=1\n\
  MOZ_NO_XPCOM_OBSOLETE=1\n\
  MOZ_NO_FAST_LOAD=1\n\
  MOZ_OJI=\n\
  MOZ_PLAINTEXT_EDITOR_ONLY=1\n\
#  MOZ_PLUGINS=\n\
  MOZ_PREF_EXTENSIONS=\n\
  MOZ_PROFILELOCKING=\n\
  MOZ_PROFILESHARING=\n\
  MOZ_SINGLE_PROFILE=1\n\
  MOZ_SPELLCHECK=\n\
  MOZ_SVG=\n\
  MOZ_UNIVERSALCHARDET=\n\
  MOZ_UPDATER=\n\
  MOZ_USE_NATIVE_UCONV=1\n\
  MOZ_V1_STRING_ABI=\n\
  MOZ_VIEW_SOURCE=\n\
  MOZ_XPFE_COMPONENTS=\n\
  MOZ_XPINSTALL=\n\
  MOZ_XTF=\n\
  MOZ_XUL_APP=1\n\
  NECKO_DISK_CACHE=\n\
  NECKO_PROTOCOLS_DEFAULT=\"about data http file res\"\n\
  NECKO_SMALL_BUFFERS=1\n\
  NS_DISABLE_LOGGING=1\n\
  NS_PRINTING=\n\
  JS_STATIC_BUILD=1\n\
  ;;\n\
\n\
minimal)\n\
  MOZ_EMBEDDING_LEVEL_DEFAULT=\n\
  MOZ_EMBEDDING_LEVEL_BASIC=\n\
  MOZ_EMBEDDING_LEVEL_MINIMAL=1\n\
  AC_DEFINE(MOZ_EMBEDDING_LEVEL_MINIMAL)\n\
  ENABLE_TESTS=\n\
  MOZ_ACTIVEX_SCRIPTING_SUPPORT=\n\
  MOZ_COMPOSER=\n\
  MOZ_ENABLE_CANVAS=\n\
  MOZ_ENABLE_POSTSCRIPT=\n\
  MOZ_EXTENSIONS_DEFAULT=\" spatialnavigation\"\n\
  MOZ_IMG_DECODERS_DEFAULT=\"png gif jpeg\"\n\
  MOZ_IMG_ENCODERS_DEFAULT=\n\
  MOZ_IMG_ENCODERS=\n\
  MOZ_INSTALLER=\n\
  MOZ_JSDEBUGGER=\n\
  MOZ_LDAP_XPCOM=\n\
  MOZ_MAIL_NEWS=\n\
  MOZ_MATHML=\n\
  MOZ_AUTH_EXTENSION=\n\
  MOZ_NO_ACTIVEX_SUPPORT=1\n\
  MOZ_NO_INSPECTOR_APIS=1\n\
  MOZ_NO_XPCOM_OBSOLETE=1\n\
  MOZ_NO_FAST_LOAD=1\n\
  MOZ_OJI=\n\
  MOZ_PLAINTEXT_EDITOR_ONLY=1\n\
  MOZ_PLUGINS=\n\
  MOZ_PREF_EXTENSIONS=\n\
  MOZ_PROFILELOCKING=\n\
  MOZ_PROFILESHARING=\n\
  MOZ_SINGLE_PROFILE=1\n\
  MOZ_SPELLCHECK=\n\
  MOZ_STORAGE=\n\
  MOZ_SVG=\n\
  MOZ_UNIVERSALCHARDET=\n\
  MOZ_UPDATER=\n\
  MOZ_USE_NATIVE_UCONV=1\n\
  MOZ_V1_STRING_ABI=\n\
  MOZ_VIEW_SOURCE=\n\
  MOZ_XPFE_COMPONENTS=\n\
  MOZ_XPINSTALL=\n\
  MOZ_XTF=\n\
  MOZ_XUL=\n\
  MOZ_RDF=\n\
  MOZ_XUL_APP=1\n\
  NECKO_DISK_CACHE=\n\
  NECKO_PROTOCOLS_DEFAULT=\"about data http file res\"\n\
  NECKO_SMALL_BUFFERS=1\n\
  NS_DISABLE_LOGGING=1\n\
  NS_PRINTING=\n\
  JS_STATIC_BUILD=1\n\
  ;;\n\
\n\
*)\n\
  AC_MSG_ERROR([Unrecognized value: --with-embedding-profile=$MOZ_EMBEDDING_PROFILE])\n\
  ;;\n\
esac\n\
\n\
AC_SUBST(MOZ_EMBEDDING_LEVEL_DEFAULT)\n\
AC_SUBST(MOZ_EMBEDDING_LEVEL_BASIC)\n\
AC_SUBST(MOZ_EMBEDDING_LEVEL_MINIMAL)\n\
\n\
case \"$MOZ_BUILD_APP\" in\n\
suite)\n\
  MOZ_APP_NAME=seamonkey\n\
  MOZ_APP_DISPLAYNAME=SeaMonkey\n\
  MOZ_MAIL_NEWS=1\n\
  MOZ_LDAP_XPCOM=1\n\
  MOZ_COMPOSER=1\n\
  MOZ_SUITE=1\n\
  MOZ_PROFILESHARING=\n\
  MOZ_APP_VERSION=$SEAMONKEY_VERSION\n\
  if test \"$MOZ_XUL_APP\"; then\n\
    MOZ_EXTENSIONS_DEFAULT=\" wallet xml-rpc p3p venkman inspector irc typeaheadfind gnomevfs sroaming reporter\"\n\
  else\n\
    MOZ_EXTENSIONS_DEFAULT=\" wallet xml-rpc help p3p venkman inspector irc typeaheadfind gnomevfs sroaming reporter\"\n\
  fi\n\
  AC_DEFINE(MOZ_SUITE)\n\
  ;;\n\
\n\
browser)\n\
  MOZ_APP_NAME=firefox\n\
  MOZ_XUL_APP=1\n\
  MOZ_UPDATER=1\n\
  MOZ_PHOENIX=1\n\
  MOZ_PLACES=1\n\
  MOZ_PLACES_BOOKMARKS=\n\
  # always enabled for form history\n\
  MOZ_MORKREADER=1\n\
  MOZ_SAFE_BROWSING=1\n\
  MOZ_APP_VERSION=$FIREFOX_VERSION\n\
  MOZ_NO_XPCOM_OBSOLETE=1\n\
  MOZ_EXTENSIONS_DEFAULT=\" xml-rpc inspector gnomevfs reporter\"\n\
  AC_DEFINE(MOZ_PHOENIX)\n\
  # MOZ_APP_DISPLAYNAME will be set by branding/configure.sh\n\
  ;;\n\
\n\
minimo)\n\
  MOZ_APP_NAME=minimo\n\
  MOZ_APP_DISPLAYNAME=minimo\n\
  AC_DEFINE(MINIMO)\n\
  MINIMO=1\n\
  MOZ_APP_VERSION=`cat $topsrcdir/minimo/config/version.txt`\n\
  ;;\n\
\n\
mail)\n\
  MOZ_APP_NAME=thunderbird\n\
  MOZ_APP_DISPLAYNAME=Thunderbird\n\
  MOZ_XUL_APP=1\n\
  MOZ_UPDATER=1\n\
  MOZ_THUNDERBIRD=1\n\
  MOZ_MATHML=\n\
  MOZ_NO_ACTIVEX_SUPPORT=1\n\
  MOZ_ACTIVEX_SCRIPTING_SUPPORT=\n\
  ENABLE_TESTS=\n\
  MOZ_OJI=\n\
  NECKO_DISK_CACHE=\n\
  NECKO_PROTOCOLS_DEFAULT=\"http file viewsource res data\"\n\
  MOZ_ENABLE_CANVAS=\n\
  MOZ_IMG_DECODERS_DEFAULT=`echo \"$MOZ_IMG_DECODERS_DEFAULT\" | sed \"s/ xbm//\"`\n\
  MOZ_MAIL_NEWS=1\n\
  MOZ_LDAP_XPCOM=1\n\
  MOZ_STATIC_MAIL_BUILD=1\n\
  MOZ_COMPOSER=1\n\
  MOZ_SAFE_BROWSING=1\n\
  MOZ_SVG=\n\
  MOZ_APP_VERSION=$THUNDERBIRD_VERSION\n\
  MOZ_EXTENSIONS_DEFAULT=\" wallet\"\n\
  AC_DEFINE(MOZ_THUNDERBIRD)\n\
  ;;\n\
\n\
composer)\n\
  MOZ_APP_NAME=nvu\n\
  MOZ_APP_DISPLAYNAME=NVU\n\
  MOZ_XUL_APP=1\n\
  MOZ_UPDATER=1\n\
  MOZ_STANDALONE_COMPOSER=1\n\
  MOZ_COMPOSER=1\n\
  MOZ_APP_VERSION=0.17+\n\
  AC_DEFINE(MOZ_STANDALONE_COMPOSER)\n\
  ;;\n\
\n\
calendar)\n\
  MOZ_APP_NAME=sunbird\n\
  MOZ_APP_DISPLAYNAME=Calendar\n\
  MOZ_XUL_APP=1\n\
  MOZ_UPDATER=1\n\
  MOZ_SUNBIRD=1\n\
  MOZ_CALENDAR=1\n\
  MOZ_APP_VERSION=$SUNBIRD_VERSION\n\
  MOZ_PLAINTEXT_EDITOR_ONLY=1\n\
  NECKO_PROTOCOLS_DEFAULT=\"about http ftp file res viewsource\"\n\
  MOZ_NO_ACTIVEX_SUPPORT=1\n\
  MOZ_ACTIVEX_SCRIPTING_SUPPORT=\n\
  MOZ_INSTALLER=\n\
  MOZ_MATHML=\n\
  NECKO_DISK_CACHE=\n\
  MOZ_OJI=\n\
  MOZ_PLUGINS=\n\
  NECKO_COOKIES=\n\
  MOZ_NO_XPCOM_OBSOLETE=1\n\
  MOZ_EXTENSIONS_DEFAULT=\n\
  MOZ_UNIVERSALCHARDET=\n\
  AC_DEFINE(MOZ_SUNBIRD)\n\
  ;;\n\
\n\
xulrunner)\n\
  MOZ_APP_NAME=xulrunner\n\
  MOZ_APP_DISPLAYNAME=XULRunner\n\
  MOZ_XUL_APP=1\n\
  MOZ_UPDATER=1\n\
  MOZ_XULRUNNER=1\n\
  MOZ_ENABLE_LIBXUL=1\n\
  MOZ_APP_VERSION=$MOZILLA_VERSION\n\
  MOZ_JAVAXPCOM=1\n\
  if test \"$MOZ_STORAGE\"; then\n\
    MOZ_PLACES=1\n\
  fi\n\
  MOZ_EXTENSIONS_DEFAULT=\" xml-rpc gnomevfs\"\n\
  MOZ_NO_XPCOM_OBSOLETE=1\n\
  AC_DEFINE(MOZ_XULRUNNER)\n\
\n\
  if test \"$LIBXUL_SDK\"; then\n\
    AC_MSG_ERROR([Building XULRunner --with-libxul-sdk doesn\'t make sense; XULRunner provides the libxul SDK.])\n\
  fi\n\
  ;;\n\
\n\
camino) \n\
  MOZ_APP_NAME=mozilla\n\
  ENABLE_TESTS=\n\
  ACCESSIBILITY=\n\
  MOZ_JSDEBUGGER=\n\
  MOZ_SINGLE_PROFILE=1\n\
  MOZ_PROFILESHARING=\n\
  MOZ_APP_DISPLAYNAME=Mozilla\n\
  MOZ_APP_VERSION=$MOZILLA_VERSION\n\
  MOZ_EXTENSIONS_DEFAULT=\" typeaheadfind\"\n\
# MOZ_XUL_APP=1\n\
  MOZ_PREF_EXTENSIONS=\n\
  MOZ_WEBSERVICES=\n\
  AC_DEFINE(MOZ_MACBROWSER)\n\
  ;;\n\
\n\
content/xslt)\n\
  MOZ_APP_NAME=mozilla\n\
  MOZ_EXTENSIONS_DEFAULT=\"\"\n\
  AC_DEFINE(TX_EXE)\n\
  MOZ_XSLT_STANDALONE=1\n\
  MOZ_APP_VERSION=$MOZILLA_VERSION\n\
  ;;\n\
\n\
netwerk)\n\
  MOZ_APP_NAME=mozilla\n\
  MOZ_EXTENSIONS_DEFAULT=\"\"\n\
  MOZ_APP_VERSION=$MOZILLA_VERSION\n\
  MOZ_NO_XPCOM_OBSOLETE=1\n\
  MOZ_SINGLE_PROFILE=1\n\
  MOZ_PROFILESHARING=\n\
  MOZ_XPINSTALL=\n\
  ;;\n\
\n\
standalone) \n\
  MOZ_APP_NAME=mozilla\n\
  MOZ_APP_DISPLAYNAME=Mozilla\n\
  MOZ_APP_VERSION=$MOZILLA_VERSION\n\
  ;;\n\
\n\
tools/update-packaging)\n\
  MOZ_APP_NAME=mozilla\n\
  MOZ_APP_DISPLAYNAME=Mozilla\n\
  MOZ_APP_VERSION=$MOZILLA_VERSION\n\
  MOZ_UPDATE_PACKAGING=1\n\
  MOZ_UPDATER=1\n\
  ;;\n\
\n\
*)\n\
  if test -z \"$MOZ_BUILD_APP\"; then\n\
    AC_MSG_ERROR([--enable-application=APP is required.])\n\
  else\n\
    AC_MSG_ERROR([--enable-application value not recognized.])\n\
  fi\n\
  ;;\n\
\n\
esac\n\
\n\
AC_SUBST(MOZ_BUILD_APP)\n\
AC_SUBST(MOZ_XUL_APP)\n\
AC_SUBST(MOZ_SUITE)\n\
AC_SUBST(MOZ_PHOENIX)\n\
AC_SUBST(MOZ_THUNDERBIRD)\n\
AC_SUBST(MOZ_STANDALONE_COMPOSER)\n\
AC_SUBST(MOZ_SUNBIRD)\n\
AC_SUBST(MOZ_XULRUNNER)\n\
\n\
AC_DEFINE_UNQUOTED(MOZ_BUILD_APP,$MOZ_BUILD_APP)\n\
\n\
if test \"$MOZ_XUL_APP\"; then\n\
  MOZ_SINGLE_PROFILE=1\n\
  MOZ_PROFILESHARING=\n\
  AC_DEFINE(MOZ_XUL_APP)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = \n\
dnl = Toolkit Options\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Toolkit Options)\n\
\n\
    dnl ========================================================\n\
    dnl = Select the default toolkit\n\
    dnl ========================================================\n\
	MOZ_ARG_ENABLE_STRING(default-toolkit,\n\
	[  --enable-default-toolkit=TK\n\
                          Select default toolkit\n\
                          Platform specific defaults:\n\
                            BeOS - beos\n\
                            Mac OS X - mac (carbon)\n\
                            Neutrino/QNX - photon\n\
                            OS/2 - os2\n\
                            Win32 - windows\n\
                            * - gtk],\n\
    [ _DEFAULT_TOOLKIT=$enableval ],\n\
    [ _DEFAULT_TOOLKIT=$_PLATFORM_DEFAULT_TOOLKIT])\n\
\n\
    if test \"$_DEFAULT_TOOLKIT\" = \"gtk\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"qt\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"gtk2\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"xlib\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"os2\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"beos\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"photon\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"mac\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"windows\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cocoa\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-windows\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-gtk2\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-beos\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-os2\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-xlib\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-mac\" \\\n\
        -o \"$_DEFAULT_TOOLKIT\" = \"cairo-cocoa\"\n\
    then\n\
        dnl nglayout only supports building with one toolkit,\n\
        dnl so ignore everything after the first comma (\",\").\n\
        MOZ_WIDGET_TOOLKIT=`echo \"$_DEFAULT_TOOLKIT\" | sed -e \"s/,.*$//\"`\n\
    else\n\
        if test \"$no_x\" != \"yes\"; then\n\
            AC_MSG_ERROR([Toolkit must be xlib, gtk, gtk2 or qt.])\n\
        else\n\
            AC_MSG_ERROR([Toolkit must be $_PLATFORM_DEFAULT_TOOLKIT or cairo-$_PLATFORM_DEFAULT_TOOLKIT (if supported).])\n\
        fi\n\
    fi\n\
\n\
AC_DEFINE_UNQUOTED(MOZ_DEFAULT_TOOLKIT,\"$MOZ_WIDGET_TOOLKIT\")\n\
\n\
dnl ========================================================\n\
dnl = Enable the toolkit as needed                         =\n\
dnl ========================================================\n\
\n\
case \"$MOZ_WIDGET_TOOLKIT\" in\n\
gtk)\n\
	MOZ_ENABLE_GTK=1\n\
    MOZ_ENABLE_XREMOTE=1\n\
    if test \"$_HAVE_FREETYPE2\"; then\n\
        MOZ_ENABLE_FREETYPE2=1\n\
    fi\n\
    MOZ_ENABLE_XPRINT=1\n\
    TK_CFLAGS=\'$(MOZ_GTK_CFLAGS)\'\n\
    TK_LIBS=\'$(MOZ_GTK_LDFLAGS)\'\n\
	AC_DEFINE(MOZ_WIDGET_GTK)\n\
    ;;\n\
\n\
gtk2)\n\
    MOZ_ENABLE_GTK2=1\n\
    MOZ_ENABLE_XREMOTE=1\n\
    MOZ_ENABLE_COREXFONTS=${MOZ_ENABLE_COREXFONTS-}\n\
    TK_CFLAGS=\'$(MOZ_GTK2_CFLAGS)\'\n\
    TK_LIBS=\'$(MOZ_GTK2_LIBS)\'\n\
    AC_DEFINE(MOZ_WIDGET_GTK2)\n\
    ;;\n\
\n\
xlib)\n\
	MOZ_ENABLE_XLIB=1\n\
    if test \"$_HAVE_FREETYPE2\"; then\n\
        MOZ_ENABLE_FREETYPE2=1\n\
    fi\n\
    MOZ_ENABLE_XPRINT=1\n\
	TK_CFLAGS=\'$(MOZ_XLIB_CFLAGS)\'\n\
	TK_LIBS=\'$(MOZ_XLIB_LDFLAGS)\'\n\
	AC_DEFINE(MOZ_WIDGET_XLIB)\n\
    ;;\n\
\n\
qt)\n\
    MOZ_ENABLE_QT=1\n\
    if test \"$_HAVE_FREETYPE2\"; then\n\
        MOZ_ENABLE_FREETYPE2=1\n\
    fi\n\
    MOZ_ENABLE_XPRINT=1\n\
    TK_CFLAGS=\'$(MOZ_QT_CFLAGS)\'\n\
    TK_LIBS=\'$(MOZ_QT_LDFLAGS)\'\n\
    AC_DEFINE(MOZ_WIDGET_QT)\n\
    ;;\n\
\n\
photon)\n\
	MOZ_ENABLE_PHOTON=1\n\
	AC_DEFINE(MOZ_WIDGET_PHOTON)\n\
    ;;\n\
mac|cocoa)\n\
    TK_LIBS=\'-framework Carbon\'\n\
    TK_CFLAGS=\"-I${MACOS_SDK_DIR}/Developer/Headers/FlatCarbon\"\n\
    CFLAGS=\"$CFLAGS $TK_CFLAGS\"\n\
    CXXFLAGS=\"$CXXFLAGS $TK_CFLAGS\"\n\
    MOZ_USER_DIR=\"Mozilla\"\n\
    AC_DEFINE(XP_MACOSX)\n\
    AC_DEFINE(TARGET_CARBON)\n\
    AC_DEFINE(TARGET_API_MAC_CARBON)\n\
    if test \"$MOZ_WIDGET_TOOLKIT\" = \"cocoa\"; then\n\
        MOZ_ENABLE_COCOA=1\n\
        AC_DEFINE(MOZ_WIDGET_COCOA)\n\
    fi\n\
    ;;\n\
\n\
cairo-windows)\n\
    MOZ_WIDGET_TOOLKIT=windows\n\
    MOZ_GFX_TOOLKIT=cairo\n\
    MOZ_ENABLE_CAIRO_GFX=1\n\
    ;;\n\
\n\
cairo-gtk2)\n\
    MOZ_WIDGET_TOOLKIT=gtk2\n\
    MOZ_GFX_TOOLKIT=cairo\n\
    MOZ_ENABLE_CAIRO_GFX=1\n\
    MOZ_ENABLE_GTK2=1\n\
    MOZ_ENABLE_XREMOTE=1\n\
    TK_CFLAGS=\'$(MOZ_GTK2_CFLAGS) $(MOZ_CAIRO_CFLAGS)\'\n\
    TK_LIBS=\'$(MOZ_GTK2_LIBS) $(MOZ_CAIRO_LIBS)\'\n\
    AC_DEFINE(MOZ_WIDGET_GTK2)\n\
    ;;\n\
cairo-beos)\n\
    MOZ_WIDGET_TOOLKIT=beos\n\
    MOZ_GFX_TOOLKIT=cairo\n\
    MOZ_ENABLE_CAIRO_GFX=1\n\
    TK_CFLAGS=\'$(MOZ_CAIRO_CFLAGS)\'\n\
    TK_LIBS=\'$(MOZ_CAIRO_LIBS)\'\n\
    ;;\n\
\n\
cairo-os2)\n\
    MOZ_WIDGET_TOOLKIT=os2\n\
    MOZ_GFX_TOOLKIT=cairo\n\
    MOZ_ENABLE_CAIRO_GFX=1\n\
    TK_CFLAGS=\'$(MOZ_CAIRO_CFLAGS)\'\n\
    TK_LIBS=\'$(MOZ_CAIRO_LIBS)\'\n\
    ;;\n\
\n\
cairo-xlib)\n\
    MOZ_WIDGET_TOOLKIT=xlib\n\
    MOZ_GFX_TOOLKIT=cairo\n\
    MOZ_ENABLE_CAIRO_GFX=1\n\
	MOZ_ENABLE_XLIB=1\n\
	TK_CFLAGS=\'$(MOZ_XLIB_CFLAGS) $(MOZ_CAIRO_FLAGS)\'\n\
	TK_LIBS=\'$(MOZ_XLIB_LDFLAGS) $(MOZ_CAIRO_LIBS)\'\n\
	AC_DEFINE(MOZ_WIDGET_XLIB)\n\
    ;;\n\
\n\
cairo-mac|cairo-cocoa)\n\
    if test \"$MOZ_WIDGET_TOOLKIT\" = \"cairo-cocoa\"; then\n\
        MOZ_WIDGET_TOOLKIT=cocoa\n\
        AC_DEFINE(MOZ_WIDGET_COCOA)\n\
        MOZ_ENABLE_COCOA=1\n\
    else\n\
        MOZ_WIDGET_TOOLKIT=mac\n\
    fi\n\
    MOZ_ENABLE_CAIRO_GFX=1\n\
    MOZ_GFX_TOOLKIT=cairo\n\
    MOZ_USER_DIR=\"Mozilla\"\n\
    AC_DEFINE(XP_MACOSX)\n\
    AC_DEFINE(TARGET_CARBON)\n\
    AC_DEFINE(TARGET_API_MAC_CARBON)\n\
    TK_LIBS=\'-framework Carbon\'\n\
    TK_CFLAGS=\"-I${MACOS_SDK_DIR}/Developer/Headers/FlatCarbon\"\n\
    CFLAGS=\"$CFLAGS $TK_CFLAGS\"\n\
    CXXFLAGS=\"$CXXFLAGS $TK_CFLAGS\"\n\
    ;;\n\
esac\n\
\n\
if test \"$MOZ_ENABLE_XREMOTE\"; then\n\
    AC_DEFINE(MOZ_ENABLE_XREMOTE)\n\
fi\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test \"$MOZ_ENABLE_GTK\"\n\
then\n\
    AM_PATH_GTK($GTK_VERSION,,\n\
    AC_MSG_ERROR(Test for GTK failed.))\n\
\n\
    MOZ_GTK_LDFLAGS=$GTK_LIBS\n\
    MOZ_GTK_CFLAGS=$GTK_CFLAGS\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_GTK2\"\n\
then\n\
    PKG_CHECK_MODULES(MOZ_GTK2, gtk+-2.0 >= 1.3.7 gdk-x11-2.0 glib-2.0 gobject-2.0)\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_XLIB\"\n\
then\n\
    MOZ_XLIB_CFLAGS=\"$X_CFLAGS\"\n\
    MOZ_XLIB_LDFLAGS=\"$XLDFLAGS\"\n\
    MOZ_XLIB_LDFLAGS=\"$MOZ_XLIB_LDFLAGS $XEXT_LIBS $X11_LIBS\"\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_QT\"\n\
then\n\
    MOZ_ARG_WITH_STRING(qtdir,\n\
    [  --with-qtdir=\\$dir       Specify Qt directory ],\n\
    [ QTDIR=$withval])\n\
\n\
    if test -z \"$QTDIR\"; then\n\
      QTDIR=\"/usr\"\n\
    fi\n\
    QTINCDIR=\"/include/qt\"\n\
    if test ! -d \"$QTDIR$QTINCDIR\"; then\n\
       QTINCDIR=\"/include/X11/qt\"\n\
    fi\n\
    if test ! -d \"$QTDIR$QTINCDIR\"; then\n\
       QTINCDIR=\"/include\"\n\
    fi\n\
\n\
    if test -x \"$QTDIR/bin/moc\"; then\n\
      HOST_MOC=\"$QTDIR/bin/moc\"\n\
    else\n\
      AC_CHECK_PROGS(HOST_MOC, moc, \"\")\n\
    fi\n\
    if test -z \"$HOST_MOC\"; then\n\
      AC_MSG_ERROR([no acceptable moc preprocessor found])\n\
    fi\n\
    MOC=$HOST_MOC\n\
\n\
    QT_CFLAGS=\"-I${QTDIR}${QTINCDIR} -DQT_GENUINE_STR -DQT_NO_STL\"\n\
    if test -z \"$MOZ_DEBUG\"; then\n\
      QT_CFLAGS=\"$QT_CFLAGS -DQT_NO_DEBUG -DNO_DEBUG\"\n\
    fi\n\
    _SAVE_LDFLAGS=$LDFLAGS\n\
    QT_LDFLAGS=-L${QTDIR}/lib\n\
    LDFLAGS=\"$LDFLAGS $QT_LDFLAGS\"\n\
    AC_LANG_SAVE\n\
    AC_LANG_CPLUSPLUS\n\
    AC_CHECK_LIB(qt, main, QT_LIB=-lqt,\n\
        AC_CHECK_LIB(qt-mt, main, QT_LIB=-lqt-mt,\n\
            AC_MSG_ERROR([Cannot find QT libraries.])))\n\
    LDFLAGS=$_SAVE_LDFLAGS\n\
    QT_LIBS=\"-L/usr/X11R6/lib $QT_LDFLAGS $QT_LIB -lXext -lX11\"\n\
\n\
    MOZ_QT_LDFLAGS=$QT_LIBS\n\
    MOZ_QT_CFLAGS=$QT_CFLAGS\n\
\n\
    _SAVE_CXXFLAGS=$CXXFLAGS\n\
    _SAVE_LIBS=$LIBS\n\
\n\
    CXXFLAGS=\"$CXXFLAGS $QT_CFLAGS\"\n\
    LIBS=\"$LIBS $QT_LIBS\"\n\
    \n\
    AC_MSG_CHECKING(Qt - version >= $QT_VERSION)\n\
    AC_TRY_COMPILE([#include <qglobal.h>],\n\
    [\n\
        #if (QT_VERSION < $QT_VERSION_NUM)\n\
            #error  \"QT_VERSION too old\"\n\
        #endif\n\
    ],result=\"yes\",result=\"no\")\n\
\n\
    AC_MSG_RESULT(\"$result\")\n\
    if test \"$result\" = \"no\"; then\n\
        AC_MSG_ERROR([Qt Mozilla requires at least version $QT_VERSION of Qt])\n\
    fi\n\
    CXXFLAGS=$_SAVE_CXXFLAGS\n\
    LIBS=$_SAVE_LIBS\n\
\n\
    AC_LANG_RESTORE\n\
fi\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
AC_SUBST(MOZ_DEFAULT_TOOLKIT)\n\
\n\
dnl ========================================================\n\
dnl = startup-notification support module\n\
dnl ========================================================\n\
\n\
if test \"$MOZ_ENABLE_GTK2\"\n\
then\n\
    MOZ_ENABLE_STARTUP_NOTIFICATION=\n\
\n\
    MOZ_ARG_ENABLE_BOOL(startup-notification,\n\
    [  --enable-startup-notification       Enable startup-notification support (default: disabled) ],\n\
        MOZ_ENABLE_STARTUP_NOTIFICATION=force,\n\
        MOZ_ENABLE_STARTUP_NOTIFICATION=)\n\
    if test \"$MOZ_ENABLE_STARTUP_NOTIFICATION\"\n\
    then\n\
        PKG_CHECK_MODULES(MOZ_STARTUP_NOTIFICATION,\n\
                          libstartup-notification-1.0 >= $STARTUP_NOTIFICATION_VERSION,\n\
        [MOZ_ENABLE_STARTUP_NOTIFICATION=1], [\n\
            if test \"$MOZ_ENABLE_STARTUP_NOTIFICATION\" = \"force\"\n\
            then\n\
                AC_MSG_ERROR([* * * Could not find startup-notification >= $STARTUP_NOTIFICATION_VERSION])\n\
            fi\n\
            MOZ_ENABLE_STARTUP_NOTIFICATION=\n\
        ])\n\
    fi\n\
\n\
    if test \"$MOZ_ENABLE_STARTUP_NOTIFICATION\"; then\n\
        AC_DEFINE(MOZ_ENABLE_STARTUP_NOTIFICATION)\n\
    fi\n\
\n\
    TK_LIBS=\"$TK_LIBS $MOZ_STARTUP_NOTIFICATION_LIBS\"\n\
fi\n\
AC_SUBST(MOZ_ENABLE_STARTUP_NOTIFICATION)\n\
AC_SUBST(MOZ_STARTUP_NOTIFICATION_CFLAGS)\n\
AC_SUBST(MOZ_STARTUP_NOTIFICATION_LIBS)\n\
\n\
AC_SUBST(GTK_CONFIG)\n\
AC_SUBST(TK_CFLAGS)\n\
AC_SUBST(TK_LIBS)\n\
\n\
AC_SUBST(MOZ_ENABLE_GTK)\n\
AC_SUBST(MOZ_ENABLE_XLIB)\n\
AC_SUBST(MOZ_ENABLE_GTK2)\n\
AC_SUBST(MOZ_ENABLE_QT)\n\
AC_SUBST(MOZ_ENABLE_PHOTON)\n\
AC_SUBST(MOZ_ENABLE_COCOA)\n\
AC_SUBST(MOZ_ENABLE_CAIRO_GFX)\n\
AC_SUBST(MOZ_ENABLE_GLITZ)\n\
AC_SUBST(MOZ_ENABLE_XREMOTE)\n\
AC_SUBST(MOZ_GTK_CFLAGS)\n\
AC_SUBST(MOZ_GTK_LDFLAGS)\n\
AC_SUBST(MOZ_GTK2_CFLAGS)\n\
AC_SUBST(MOZ_GTK2_LIBS)\n\
AC_SUBST(MOZ_XLIB_CFLAGS)\n\
AC_SUBST(MOZ_XLIB_LDFLAGS)\n\
AC_SUBST(MOZ_QT_CFLAGS)\n\
AC_SUBST(MOZ_QT_LDFLAGS)\n\
\n\
AC_SUBST(MOC)\n\
\n\
if test \"$MOZ_ENABLE_CAIRO_GFX\"\n\
then\n\
    AC_DEFINE(MOZ_THEBES)\n\
    AC_DEFINE(MOZ_CAIRO_GFX)\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_GTK\" \\\n\
|| test \"$MOZ_ENABLE_QT\" \\\n\
|| test \"$MOZ_ENABLE_XLIB\" \\\n\
|| test \"$MOZ_ENABLE_GTK2\"\n\
then\n\
    AC_DEFINE(MOZ_X11)\n\
    MOZ_X11=1\n\
fi\n\
AC_SUBST(MOZ_X11)\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Components & Features\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Components and Features)\n\
\n\
dnl ========================================================\n\
dnl = Localization\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_STRING(ui-locale,\n\
[  --enable-ui-locale=ab-CD\n\
                          Select the user interface locale (default: en-US)],\n\
    MOZ_UI_LOCALE=$enableval )\n\
AC_SUBST(MOZ_UI_LOCALE)\n\
\n\
dnl =========================================================\n\
dnl = Calendar client\n\
dnl =========================================================\n\
MOZ_ARG_ENABLE_BOOL(calendar,,\n\
    MOZ_OLD_CALENDAR=1,\n\
    MOZ_OLD_CALENDAR= )\n\
\n\
if test \"$MOZ_OLD_CALENDAR\"; then\n\
    AC_MSG_WARN([Building with the calendar extension is no longer supported.])\n\
    if test \"$MOZ_THUNDERBIRD\"; then\n\
        AC_MSG_WARN([Since you\'re trying to build mail, you could try adding])\n\
        AC_MSG_WARN([\'--enable-extensions=default,lightning\' to your mozconfig])\n\
        AC_MSG_WARN([and building WITH A FRESH TREE.])\n\
    fi\n\
    AC_MSG_WARN([For more information, please visit:])\n\
    AC_MSG_ERROR([http://www.mozilla.org/projects/calendar/])\n\
fi\n\
\n\
AC_SUBST(MOZ_CALENDAR)\n\
\n\
dnl =========================================================\n\
dnl = Mail & News\n\
dnl =========================================================\n\
MOZ_ARG_DISABLE_BOOL(mailnews,\n\
[  --disable-mailnews      Disable building of mail & news components],\n\
    MOZ_MAIL_NEWS=,\n\
    MOZ_MAIL_NEWS=1 )\n\
AC_SUBST(MOZ_MAIL_NEWS)\n\
\n\
dnl ========================================================\n\
dnl static mail build off by default \n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(static-mail,\n\
[  --enable-static-mail Enable static mail build support],\n\
    MOZ_STATIC_MAIL_BUILD=1,\n\
    MOZ_STATIC_MAIL_BUILD= )\n\
\n\
if test \"$MOZ_STATIC_MAIL_BUILD\"; then\n\
    AC_DEFINE(MOZ_STATIC_MAIL_BUILD)\n\
fi\n\
\n\
AC_SUBST(MOZ_STATIC_MAIL_BUILD)\n\
\n\
dnl =========================================================\n\
dnl = LDAP\n\
dnl =========================================================\n\
MOZ_ARG_DISABLE_BOOL(ldap,\n\
[  --disable-ldap          Disable LDAP support],\n\
    MOZ_LDAP_XPCOM=,\n\
    MOZ_LDAP_XPCOM=1)\n\
\n\
dnl ========================================================\n\
dnl = Trademarked Branding \n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(official-branding,\n\
[  --enable-official-branding Enable Official mozilla.org Branding\n\
                          Do not distribute builds with\n\
                          --enable-official-branding unless you have\n\
                          permission to use trademarks per\n\
                          http://www.mozilla.org/foundation/trademarks/ .],\n\
[case \"$MOZ_BUILD_APP\" in\n\
browser)\n\
    MOZ_BRANDING_DIRECTORY=other-licenses/branding/firefox\n\
    MOZ_APP_DISPLAYNAME=Firefox\n\
    ;;\n\
\n\
calendar)\n\
    MOZ_BRANDING_DIRECTORY=other-licenses/branding/sunbird\n\
    MOZ_APP_DISPLAYNAME=Sunbird\n\
    ;;\n\
\n\
mail)\n\
    MOZ_BRANDING_DIRECTORY=other-licenses/branding/thunderbird\n\
    ;;\n\
\n\
*)]\n\
    AC_MSG_ERROR([Official branding is only available for Firefox Sunbird and Thunderbird.])\n\
esac\n\
)\n\
\n\
MOZ_ARG_WITH_STRING(branding,\n\
[  --with-branding=dir    Use branding from the specified directory.],\n\
    MOZ_BRANDING_DIRECTORY=$withval)\n\
\n\
REAL_BRANDING_DIRECTORY=\"${MOZ_BRANDING_DIRECTORY}\"\n\
if test -z \"$REAL_BRANDING_DIRECTORY\"; then\n\
  REAL_BRANDING_DIRECTORY=${MOZ_BUILD_APP}/branding/nightly\n\
fi\n\
\n\
if test -f \"$topsrcdir/$REAL_BRANDING_DIRECTORY/configure.sh\"; then\n\
  . \"$topsrcdir/$REAL_BRANDING_DIRECTORY/configure.sh\"\n\
fi\n\
\n\
AC_SUBST(MOZ_BRANDING_DIRECTORY)\n\
\n\
dnl ========================================================\n\
dnl = Distribution ID\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(distribution-id,\n\
[  --with-distribution-id=ID  Set distribution-specific id (default=org.mozilla)],\n\
[ val=`echo $withval`\n\
    MOZ_DISTRIBUTION_ID=\"$val\"])\n\
\n\
if test -z \"$MOZ_DISTRIBUTION_ID\"; then\n\
   MOZ_DISTRIBUTION_ID=\"org.mozilla\"\n\
fi\n\
\n\
AC_DEFINE_UNQUOTED(MOZ_DISTRIBUTION_ID,\"$MOZ_DISTRIBUTION_ID\")\n\
AC_SUBST(MOZ_DISTRIBUTION_ID)\n\
\n\
dnl ========================================================\n\
dnl = FreeType2\n\
dnl = Enable freetype by default if building against X11 \n\
dnl = and freetype is available\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(freetype2,\n\
[  --disable-freetype2     Disable FreeType2 support ],\n\
    MOZ_ENABLE_FREETYPE2=,\n\
    MOZ_ENABLE_FREETYPE2=1)\n\
\n\
if test \"$MOZ_ENABLE_FREETYPE2\" && test -z \"$MOZ_X11\" -o -z \"$_HAVE_FREETYPE2\"; then\n\
    AC_MSG_ERROR([Cannot enable FreeType2 support for non-X11 toolkits or if FreeType2 is not detected.])\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_FREETYPE2\"; then\n\
    AC_DEFINE(MOZ_ENABLE_FREETYPE2)\n\
    _NON_GLOBAL_ACDEFINES=\"$_NON_GLOBAL_ACDEFINES MOZ_ENABLE_FREETYPE2\"\n\
fi\n\
AC_SUBST(MOZ_ENABLE_FREETYPE2)\n\
\n\
dnl ========================================================\n\
dnl = Xft\n\
dnl ========================================================\n\
if test \"$MOZ_ENABLE_GTK2\"; then\n\
    MOZ_ENABLE_XFT=1\n\
fi\n\
\n\
MOZ_ARG_DISABLE_BOOL(xft,\n\
[  --disable-xft           Disable Xft support ],\n\
    MOZ_ENABLE_XFT=,\n\
    MOZ_ENABLE_XFT=1)\n\
\n\
if test \"$MOZ_ENABLE_XFT\" && test -z \"$MOZ_ENABLE_GTK2\"; then\n\
    AC_MSG_ERROR([Cannot enable XFT support for non-GTK2 toolkits.])\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_XFT\" && test \"$MOZ_ENABLE_FREETYPE2\"; then\n\
    AC_MSG_ERROR([Cannot enable XFT and FREETYPE2 at the same time.])\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_XFT\"\n\
then\n\
    AC_DEFINE(MOZ_ENABLE_XFT)\n\
    PKG_CHECK_MODULES(MOZ_XFT, xft)\n\
    PKG_CHECK_MODULES(_PANGOCHK, pango >= 1.1.0)\n\
fi\n\
\n\
AC_SUBST(MOZ_ENABLE_XFT)\n\
AC_SUBST(MOZ_XFT_CFLAGS)\n\
AC_SUBST(MOZ_XFT_LIBS)\n\
\n\
dnl ========================================================\n\
dnl = pango font rendering\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(pango,\n\
[  --enable-pango          Enable Pango font rendering support],\n\
    MOZ_ENABLE_PANGO=1,\n\
    MOZ_ENABLE_PANGO=)\n\
\n\
if test \"$MOZ_ENABLE_PANGO\" && test -z \"$MOZ_ENABLE_CAIRO_GFX\"\n\
then\n\
    AC_DEFINE(MOZ_ENABLE_PANGO)\n\
    PKG_CHECK_MODULES(MOZ_PANGO, pangoxft >= 1.6.0)\n\
\n\
    AC_SUBST(MOZ_ENABLE_PANGO)\n\
    AC_SUBST(MOZ_PANGO_CFLAGS)\n\
    AC_SUBST(MOZ_PANGO_LIBS)\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_GTK2\" && test \"$MOZ_ENABLE_CAIRO_GFX\"\n\
then\n\
    # For gtk2, we require --enable-pango; gtk2 already implies --enable-xft\n\
    if test -z \"$MOZ_ENABLE_PANGO\"\n\
    then\n\
        AC_MSG_WARN([Pango is required for cairo gfx builds, assuming --enable-pango])\n\
        MOZ_ENABLE_PANGO=1\n\
    fi\n\
fi\n\
\n\
if test \"$MOZ_ENABLE_PANGO\" && test \"$MOZ_ENABLE_CAIRO_GFX\"\n\
then\n\
    AC_DEFINE(MOZ_ENABLE_PANGO)\n\
    dnl PKG_CHECK_MODULES(MOZ_PANGO, pango >= 1.10.0 pangocairo >= 1.10.0)\n\
    if test \"$MOZ_X11\"; then\n\
        PKG_CHECK_MODULES(MOZ_PANGO, pango >= 1.6.0 pangoft2 >= 1.6.0 pangoxft >= 1.6.0)\n\
    else\n\
        PKG_CHECK_MODULES(MOZ_PANGO, pango >= 1.6.0 pangoft2 >= 1.6.0)\n\
    fi\n\
\n\
    AC_SUBST(MOZ_ENABLE_PANGO)\n\
    AC_SUBST(MOZ_PANGO_CFLAGS)\n\
    AC_SUBST(MOZ_PANGO_LIBS)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = x11 core font support (default and ability to enable depend on toolkit)\n\
dnl ========================================================\n\
if test \"$MOZ_X11\"\n\
then\n\
    MOZ_ENABLE_COREXFONTS=${MOZ_ENABLE_COREXFONTS-1}\n\
else\n\
    MOZ_ENABLE_COREXFONTS=\n\
fi\n\
if test \"$MOZ_ENABLE_COREXFONTS\"\n\
then\n\
    AC_DEFINE(MOZ_ENABLE_COREXFONTS)\n\
fi\n\
\n\
AC_SUBST(MOZ_ENABLE_COREXFONTS)\n\
\n\
dnl ========================================================\n\
dnl = PostScript print module\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(postscript,\n\
[  --disable-postscript    Disable PostScript printing support ],\n\
    MOZ_ENABLE_POSTSCRIPT=,\n\
    MOZ_ENABLE_POSTSCRIPT=1 )\n\
\n\
dnl ========================================================\n\
dnl = Xprint print module\n\
dnl ========================================================\n\
if test \"$MOZ_X11\"\n\
then\n\
    _SAVE_LDFLAGS=\"$LDFLAGS\"\n\
    LDFLAGS=\"$XLDFLAGS $LDFLAGS\"\n\
    AC_CHECK_LIB(Xp, XpGetPrinterList, [XPRINT_LIBS=\"-lXp\"],\n\
        MOZ_ENABLE_XPRINT=, $XEXT_LIBS $XLIBS)\n\
    LDFLAGS=\"$_SAVE_LDFLAGS\"\n\
\n\
    MOZ_XPRINT_CFLAGS=\"$X_CFLAGS\"\n\
    MOZ_XPRINT_LDFLAGS=\"$XLDFLAGS $XPRINT_LIBS\"\n\
    MOZ_XPRINT_LDFLAGS=\"$MOZ_XPRINT_LDFLAGS $XEXT_LIBS $X11_LIBS\"\n\
\n\
    MOZ_ARG_DISABLE_BOOL(xprint,\n\
    [  --disable-xprint        Disable Xprint printing support ],\n\
        MOZ_ENABLE_XPRINT=,\n\
        MOZ_ENABLE_XPRINT=1 )\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = GnomeVFS support module\n\
dnl ========================================================\n\
\n\
if test \"$MOZ_X11\"\n\
then\n\
    dnl build the gnomevfs extension by default only when the\n\
    dnl GTK2 toolkit is in use.\n\
    if test \"$MOZ_ENABLE_GTK2\"\n\
    then\n\
        MOZ_ENABLE_GNOMEVFS=1\n\
        MOZ_ENABLE_GCONF=1\n\
        MOZ_ENABLE_LIBGNOME=1\n\
    fi\n\
\n\
    MOZ_ARG_DISABLE_BOOL(gnomevfs,\n\
    [  --disable-gnomevfs      Disable GnomeVFS support ],\n\
        MOZ_ENABLE_GNOMEVFS=,\n\
        MOZ_ENABLE_GNOMEVFS=force)\n\
\n\
    if test \"$MOZ_ENABLE_GNOMEVFS\"\n\
    then\n\
        PKG_CHECK_MODULES(MOZ_GNOMEVFS, gnome-vfs-2.0 >= $GNOMEVFS_VERSION gnome-vfs-module-2.0 >= $GNOMEVFS_VERSION,[\n\
            MOZ_GNOMEVFS_LIBS=`echo $MOZ_GNOMEVFS_LIBS | sed \'s/-llinc\\>//\'`\n\
            MOZ_ENABLE_GNOMEVFS=1\n\
        ],[\n\
            if test \"$MOZ_ENABLE_GNOMEVFS\" = \"force\"\n\
            then\n\
                AC_MSG_ERROR([* * * Could not find gnome-vfs-module-2.0 >= $GNOMEVFS_VERSION])\n\
            fi\n\
            MOZ_ENABLE_GNOMEVFS=\n\
        ])\n\
    fi\n\
\n\
    AC_SUBST(MOZ_GNOMEVFS_CFLAGS)\n\
    AC_SUBST(MOZ_GNOMEVFS_LIBS)\n\
\n\
    if test \"$MOZ_ENABLE_GCONF\"\n\
    then\n\
        PKG_CHECK_MODULES(MOZ_GCONF, gconf-2.0 >= $GCONF_VERSION,[\n\
            MOZ_GCONF_LIBS=`echo $MOZ_GCONF_LIBS | sed \'s/-llinc\\>//\'`\n\
            MOZ_ENABLE_GCONF=1\n\
        ],[\n\
            MOZ_ENABLE_GCONF=\n\
        ])\n\
    fi\n\
\n\
    AC_SUBST(MOZ_GCONF_CFLAGS)\n\
    AC_SUBST(MOZ_GCONF_LIBS)\n\
\n\
    if test \"$MOZ_ENABLE_LIBGNOME\"\n\
    then\n\
        PKG_CHECK_MODULES(MOZ_LIBGNOME, libgnome-2.0 >= $LIBGNOME_VERSION,[\n\
            MOZ_LIBGNOME_LIBS=`echo $MOZ_LIBGNOME_LIBS | sed \'s/-llinc\\>//\'`\n\
            MOZ_ENABLE_LIBGNOME=1\n\
        ],[\n\
            MOZ_ENABLE_LIBGNOME=\n\
        ])\n\
    fi\n\
\n\
    AC_SUBST(MOZ_LIBGNOME_CFLAGS)\n\
    AC_SUBST(MOZ_LIBGNOME_LIBS)\n\
\n\
    # The GNOME component is built if gtk2, gconf, gnome-vfs, and libgnome\n\
    # are all available.\n\
\n\
    if test \"$MOZ_ENABLE_GTK2\" -a \"$MOZ_ENABLE_GCONF\" -a \\\n\
            \"$MOZ_ENABLE_GNOMEVFS\" -a \"$MOZ_ENABLE_LIBGNOME\"; then\n\
      MOZ_ENABLE_GNOME_COMPONENT=1\n\
    else\n\
      MOZ_ENABLE_GNOME_COMPONENT=\n\
    fi\n\
\n\
    AC_SUBST(MOZ_ENABLE_GNOME_COMPONENT)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = libgnomeui support module\n\
dnl ========================================================\n\
\n\
if test \"$MOZ_ENABLE_GTK2\"\n\
then\n\
    MOZ_ENABLE_GNOMEUI=1\n\
\n\
    MOZ_ARG_DISABLE_BOOL(gnomeui,\n\
    [  --disable-gnomeui       Disable libgnomeui support (default: auto, optional at runtime) ],\n\
        MOZ_ENABLE_GNOMEUI=,\n\
        MOZ_ENABLE_GNOMEUI=force)\n\
\n\
    if test \"$MOZ_ENABLE_GNOMEUI\"\n\
    then\n\
        PKG_CHECK_MODULES(MOZ_GNOMEUI, libgnomeui-2.0 >= $GNOMEUI_VERSION,\n\
        [\n\
            MOZ_GNOMEUI_LIBS=`echo $MOZ_GNOMEUI_LIBS | sed \'s/-llinc\\>//\'`\n\
            MOZ_ENABLE_GNOMEUI=1\n\
        ],[\n\
            if test \"$MOZ_ENABLE_GNOMEUI\" = \"force\"\n\
            then\n\
                AC_MSG_ERROR([* * * Could not find libgnomeui-2.0 >= $GNOMEUI_VERSION])\n\
            fi\n\
            MOZ_ENABLE_GNOMEUI=\n\
        ])\n\
    fi\n\
\n\
    if test \"$MOZ_ENABLE_GNOMEUI\"; then\n\
        AC_DEFINE(MOZ_ENABLE_GNOMEUI)\n\
    fi\n\
fi\n\
AC_SUBST(MOZ_ENABLE_GNOMEUI)\n\
AC_SUBST(MOZ_GNOMEUI_CFLAGS)\n\
AC_SUBST(MOZ_GNOMEUI_LIBS)\n\
\n\
dnl ========================================================\n\
dnl = dbus support\n\
dnl ========================================================\n\
\n\
if test \"$MOZ_ENABLE_GTK2\"\n\
then\n\
    MOZ_ENABLE_DBUS=1\n\
\n\
    MOZ_ARG_DISABLE_BOOL(dbus,\n\
    [  --disable-dbus       Disable dbus support (default: auto, optional at runtime) ],\n\
        MOZ_ENABLE_DBUS=,\n\
        MOZ_ENABLE_DBUS=force)\n\
\n\
    if test \"$MOZ_ENABLE_DBUS\"\n\
    then\n\
        PKG_CHECK_MODULES(MOZ_DBUS_GLIB, dbus-glib-1,\n\
        [\n\
            MOZ_ENABLE_DBUS=1\n\
        ],[\n\
            if test \"$MOZ_ENABLE_DBUS\" = \"force\"\n\
            then\n\
                AC_MSG_ERROR([* * * Could not find dbus-glib-1])\n\
            fi\n\
            MOZ_ENABLE_DBUS=\n\
        ])\n\
    fi\n\
\n\
    if test \"$MOZ_ENABLE_DBUS\"; then\n\
        AC_DEFINE(MOZ_ENABLE_DBUS)\n\
    fi\n\
fi\n\
AC_SUBST(MOZ_ENABLE_DBUS)\n\
AC_SUBST(MOZ_DBUS_GLIB_CFLAGS)\n\
AC_SUBST(MOZ_DBUS_GLIB_LIBS)\n\
\n\
dnl ========================================================\n\
dnl = Setting MOZ_EXTRA_X11CONVERTERS turns on additional \n\
dnl = converters in intl/uconv that are used only by X11 gfx \n\
dnl = implementations. By default, it\'s undefined so that \n\
dnl = those converters are not built on other platforms/toolkits. \n\
dnl = (see bug 180851)\n\
dnl ========================================================\n\
\n\
if (test \"$MOZ_ENABLE_GTK\"  || test \"$MOZ_ENABLE_GTK2\") \\\n\
&& test \"$MOZ_ENABLE_COREXFONTS\" \\\n\
|| test \"$MOZ_ENABLE_XLIB\" \\\n\
|| test \"$MOZ_ENABLE_XPRINT\" \n\
then\n\
    AC_DEFINE(MOZ_EXTRA_X11CONVERTERS)\n\
    MOZ_EXTRA_X11CONVERTERS=1\n\
fi\n\
AC_SUBST(MOZ_EXTRA_X11CONVERTERS)\n\
\n\
dnl ========================================================\n\
dnl = Build Personal Security Manager\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(crypto,\n\
[  --disable-crypto        Disable crypto support (Personal Security Manager)],\n\
    MOZ_PSM=,\n\
    MOZ_PSM=1 )\n\
\n\
dnl ========================================================\n\
dnl = JS Debugger XPCOM component (js/jsd)\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(jsd,\n\
[  --disable-jsd           Disable JavaScript debug library],\n\
    MOZ_JSDEBUGGER=,\n\
    MOZ_JSDEBUGGER=1)\n\
\n\
\n\
dnl ========================================================\n\
dnl = Disable plugin support\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(plugins,\n\
[  --disable-plugins       Disable plugins support],\n\
    MOZ_PLUGINS=,\n\
    MOZ_PLUGINS=1)\n\
\n\
dnl ========================================================\n\
dnl = Open JVM Interface (OJI) support\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(oji,\n\
[  --disable-oji           Disable Open JVM Integration support],\n\
    MOZ_OJI=,\n\
    MOZ_OJI=1)\n\
if test -n \"$MOZ_OJI\"; then\n\
    AC_DEFINE(OJI)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = This turns on xinerama support.  We just can\'t use the\n\
dnl = autodetection of the libraries since on Red Hat 7 linking with\n\
dnl = Xinerama crashes the dynamic loader.  Make people turn it on\n\
dnl = explicitly.  The autodetection is done above in the Xlib\n\
dnl = detection routines.\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(xinerama,\n\
[  --enable-xinerama       Enable Xinerama support\n\
                           ( not safe for Red Hat 7.0 ) ],\n\
    _ENABLE_XINERAMA=1,\n\
    _ENABLE_XINERAMA= )\n\
\n\
if test -n \"$_ENABLE_XINERAMA\" -a -n \"$MOZ_XINERAMA_LIBS\" -a \\\n\
    -n \"$ac_cv_header_X11_extensions_Xinerama_h\"; then\n\
    MOZ_ENABLE_XINERAMA=1\n\
    AC_DEFINE(MOZ_ENABLE_XINERAMA)\n\
fi\n\
\n\
dnl bi-directional support always on\n\
IBMBIDI=1\n\
AC_DEFINE(IBMBIDI)\n\
\n\
dnl ========================================================\n\
dnl complex text support off by default\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(ctl,\n\
[  --enable-ctl            Enable Thai Complex Script support],\n\
    SUNCTL=1,\n\
    SUNCTL= )\n\
\n\
dnl ========================================================\n\
dnl view source support on by default\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(view-source,\n\
[  --disable-view-source     Disable view source support],\n\
    MOZ_VIEW_SOURCE=,\n\
    MOZ_VIEW_SOURCE=1 )\n\
if test \"$MOZ_VIEW_SOURCE\"; then\n\
    AC_DEFINE(MOZ_VIEW_SOURCE)\n\
fi\n\
\n\
\n\
dnl ========================================================\n\
dnl accessibility support on by default on all platforms \n\
dnl except OS X.\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(accessibility,\n\
[  --disable-accessibility Disable accessibility support (off by default on OS X)],\n\
    ACCESSIBILITY=,\n\
    ACCESSIBILITY=1 )\n\
if test \"$ACCESSIBILITY\"; then\n\
    AC_DEFINE(ACCESSIBILITY)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl xpfe/components on by default\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xpfe-components,\n\
[  --disable-xpfe-components\n\
                          Disable xpfe components],\n\
    MOZ_XPFE_COMPONENTS=,\n\
    MOZ_XPFE_COMPONENTS=1 )\n\
\n\
dnl ========================================================\n\
dnl xpinstall support on by default\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xpinstall,\n\
[  --disable-xpinstall     Disable xpinstall support],\n\
    MOZ_XPINSTALL=,\n\
    MOZ_XPINSTALL=1 )\n\
if test \"$MOZ_XPINSTALL\"; then\n\
    AC_DEFINE(MOZ_XPINSTALL)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Single profile support off by default\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(single-profile,\n\
[  --enable-single-profile Enable single profile support ],\n\
    MOZ_SINGLE_PROFILE=1,\n\
    MOZ_SINGLE_PROFILE= )\n\
\n\
dnl ========================================================\n\
dnl xpcom js loader support on by default\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(jsloader,\n\
[  --disable-jsloader      Disable xpcom js loader support],\n\
    MOZ_JSLOADER=,\n\
    MOZ_JSLOADER=1 )\n\
if test \"$MOZ_JSLOADER\"; then\n\
    AC_DEFINE(MOZ_JSLOADER)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Disable printing\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(printing,\n\
[  --disable-printing  Disable printing support],\n\
    NS_PRINTING=,\n\
    NS_PRINTING=1 )\n\
\n\
if test \"$NS_PRINTING\"; then\n\
    AC_DEFINE(NS_PRINTING)\n\
    AC_DEFINE(NS_PRINT_PREVIEW)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl use native unicode converters\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(native-uconv,\n\
[  --enable-native-uconv   Enable iconv support],\n\
    MOZ_USE_NATIVE_UCONV=1,\n\
    MOZ_USE_NATIVE_UCONV= )\n\
if test \"$MOZ_USE_NATIVE_UCONV\"; then\n\
    AC_DEFINE(MOZ_USE_NATIVE_UCONV)\n\
fi\n\
if test \"$OS_ARCH\" != \"WINCE\" -a \"$OS_ARCH\" != \"WINNT\" -a \"$MOZ_USE_NATIVE_UCONV\" -a \"$ac_cv_func_iconv\" != \"yes\"; then\n\
    AC_MSG_ERROR([iconv() not found.  Cannot enable native uconv support.])\n\
fi\n\
\n\
\n\
dnl ========================================================\n\
dnl Libeditor can be build as plaintext-only,\n\
dnl or as a full html and text editing component.\n\
dnl We build both by default.\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(plaintext-editor-only,\n\
[  --enable-plaintext-editor-only\n\
                          Allow only plaintext editing],\n\
    MOZ_PLAINTEXT_EDITOR_ONLY=1,\n\
    MOZ_PLAINTEXT_EDITOR_ONLY= )\n\
dnl Note the #define is MOZILLA, not MOZ, for compat with the Mac build.\n\
AC_SUBST(MOZ_PLAINTEXT_EDITOR_ONLY)\n\
\n\
dnl ========================================================\n\
dnl Composer is on by default.\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(composer,\n\
[  --disable-composer      Disable building of Composer],\n\
    MOZ_COMPOSER=,\n\
    MOZ_COMPOSER=1 )\n\
AC_SUBST(MOZ_COMPOSER)\n\
\n\
\n\
dnl ========================================================\n\
dnl = Drop XPCOM Obsolete library\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xpcom-obsolete,\n\
[  --disable-xpcom-obsolete           Disable XPCOM Obsolete Library],\n\
    MOZ_NO_XPCOM_OBSOLETE=1,\n\
    MOZ_NO_XPCOM_OBSOLETE=)\n\
\n\
if test -n \"$MOZ_NO_XPCOM_OBSOLETE\"; then\n\
if test -z \"$MOZ_SINGLE_PROFILE\"; then\n\
    AC_MSG_ERROR([Building without xpcom-obsolete isn\'t support when building full profile support.])\n\
fi\n\
    AC_DEFINE(MOZ_NO_XPCOM_OBSOLETE)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Disable Fast Load\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xpcom-fastload,\n\
[  --disable-xpcom-fastload           Disable XPCOM fastload support],\n\
    MOZ_NO_FAST_LOAD=1,\n\
    MOZ_NO_FAST_LOAD=)\n\
\n\
AC_SUBST(MOZ_NO_FAST_LOAD)\n\
\n\
if test -n \"$MOZ_NO_FAST_LOAD\"; then\n\
    AC_DEFINE(MOZ_NO_FAST_LOAD)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Permissions System\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(permissions,\n\
[  --disable-permissions   Disable permissions (popup and cookie blocking)],\n\
    MOZ_PERMISSIONS=,\n\
    MOZ_PERMISSIONS=1\n\
)\n\
\n\
dnl ========================================================\n\
dnl NegotiateAuth\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(negotiateauth,\n\
[  --disable-negotiateauth Disable GSS-API negotiation ],\n\
    MOZ_AUTH_EXTENSION=,\n\
    MOZ_AUTH_EXTENSION=1 )\n\
\n\
dnl ========================================================\n\
dnl XTF\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xtf,\n\
[  --disable-xtf           Disable XTF (pluggable xml tags) support],\n\
    MOZ_XTF=,\n\
    MOZ_XTF=1 )\n\
if test \"$MOZ_XTF\"; then\n\
  AC_DEFINE(MOZ_XTF)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Inspector APIs\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(inspector-apis,\n\
[  --disable-inspector-apis Disable the DOM inspection APIs ],\n\
    MOZ_NO_INSPECTOR_APIS=1,\n\
    MOZ_NO_INSPECTOR_APIS= )\n\
\n\
dnl ========================================================\n\
dnl XMLExtras\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xmlextras,\n\
[  --disable-xmlextras Disable XMLExtras such as XPointer support ],\n\
    MOZ_XMLEXTRAS=,\n\
    MOZ_XMLEXTRAS=1 )\n\
\n\
dnl ========================================================\n\
dnl Webservices\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(webservices,\n\
[  --disable-webservices Disable Webservices/SOAP support ],\n\
    MOZ_WEBSERVICES=,\n\
    MOZ_WEBSERVICES=1 )\n\
\n\
dnl ========================================================\n\
dnl Pref extensions (autoconfig and system-pref)\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(pref-extensions,\n\
[  --disable-pref-extensions\n\
                          Disable pref extensions such as autoconfig and\n\
                          system-pref],\n\
  MOZ_PREF_EXTENSIONS=,\n\
  MOZ_PREF_EXTENSIONS=1 )\n\
\n\
dnl ========================================================\n\
dnl = Universalchardet\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(universalchardet,\n\
[  --disable-universalchardet\n\
                          Disable universal encoding detection],\n\
  MOZ_UNIVERSALCHARDET=,\n\
  MOZ_UNIVERSALCHARDET=1 )\n\
\n\
dnl ========================================================\n\
dnl JavaXPCOM\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(javaxpcom,\n\
[  --enable-javaxpcom\n\
                          Enable Java-XPCOM bridge],\n\
    MOZ_JAVAXPCOM=1,\n\
    MOZ_JAVAXPCOM= )\n\
\n\
if test -n \"${MOZ_JAVAXPCOM}\"; then\n\
  case \"$host_os\" in\n\
    cygwin*|msvc*|mks*)\n\
      if test -n \"$JAVA_HOME\"; then\n\
        JAVA_HOME=`cygpath -u \\`cygpath -m -s \"$JAVA_HOME\"\\``\n\
      fi\n\
      ;;\n\
    *mingw*)\n\
      if test -n \"$JAVA_HOME\"; then\n\
        JAVA_HOME=`cd \"$JAVA_HOME\" && pwd`\n\
      fi\n\
      ;;\n\
  esac\n\
\n\
  if test -n \"${JAVA_INCLUDE_PATH}\"; then\n\
    dnl Make sure jni.h exists in the given include path.\n\
    if test ! -f \"$JAVA_INCLUDE_PATH/jni.h\"; then\n\
      AC_MSG_ERROR([jni.h was not found in given include path $JAVA_INCLUDE_PATH.])\n\
    fi\n\
  else\n\
    case \"$target_os\" in\n\
      darwin*)\n\
        dnl Default to java system location\n\
        JAVA_INCLUDE_PATH=/System/Library/Frameworks/JavaVM.framework/Headers\n\
        ;;\n\
      *)\n\
        dnl Try $JAVA_HOME\n\
        JAVA_INCLUDE_PATH=\"$JAVA_HOME/include\"\n\
        ;;\n\
    esac\n\
    if test ! -f \"$JAVA_INCLUDE_PATH/jni.h\"; then\n\
      AC_MSG_ERROR([The header jni.h was not found.  Set \\$JAVA_HOME to your java sdk directory, use --with-java-bin-path={java-bin-dir}, or reconfigure with --disable-javaxpcom.])\n\
    fi\n\
  fi\n\
\n\
  if test -n \"${JAVA_BIN_PATH}\"; then\n\
    dnl Look for javac and jar in the specified path.\n\
    JAVA_PATH=\"$JAVA_BIN_PATH\"\n\
  else\n\
    dnl No path specified, so look for javac and jar in $JAVA_HOME & $PATH.\n\
    JAVA_PATH=\"$JAVA_HOME/bin:$PATH\"\n\
  fi\n\
\n\
  AC_PATH_PROG(JAVA, java, :, [$JAVA_PATH])\n\
  AC_PATH_PROG(JAVAC, javac, :, [$JAVA_PATH])\n\
  AC_PATH_PROG(JAR, jar, :, [$JAVA_PATH])\n\
  if test -z \"$JAVA\" || test \"$JAVA\" = \":\" || test -z \"$JAVAC\" || test \"$JAVAC\" = \":\" || test -z \"$JAR\" || test \"$JAR\" = \":\"; then\n\
    AC_MSG_ERROR([The programs java, javac and jar were not found.  Set \\$JAVA_HOME to your java sdk directory, use --with-java-bin-path={java-bin-dir}, or reconfigure with --disable-javaxpcom.])\n\
  fi\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Airbag crash reporting (on by default on supported platforms)\n\
dnl ========================================================\n\
\n\
if test \"$OS_ARCH\" = \"WINNT\" -a -z \"$GNU_CC\"; then\n\
   MOZ_AIRBAG=1\n\
fi\n\
\n\
MOZ_ARG_DISABLE_BOOL(airbag,\n\
[  --disable-airbag          Disable airbag crash reporting],\n\
    MOZ_AIRBAG=,\n\
    MOZ_AIRBAG=1)\n\
\n\
if test -n \"$MOZ_AIRBAG\"; then\n\
   AC_DEFINE(MOZ_AIRBAG)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Build mochitest JS/DOM tests (on by default)\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(mochitest,\n\
[  --disable-mochitest        Disable mochitest harness],\n\
    MOZ_MOCHITEST=,\n\
    MOZ_MOCHITEST=1 )\n\
\n\
dnl ========================================================\n\
dnl = Enable compilation of specific extension modules\n\
dnl ========================================================\n\
\n\
MOZ_ARG_ENABLE_STRING(extensions,\n\
[  --enable-extensions     Enable extensions],\n\
[ for option in `echo $enableval | sed \'s/,/ /g\'`; do\n\
    if test \"$option\" = \"yes\" || test \"$option\" = \"all\"; then\n\
        MOZ_EXTENSIONS=\"$MOZ_EXTENSIONS $MOZ_EXTENSIONS_ALL\"\n\
    elif test \"$option\" = \"no\" || test \"$option\" = \"none\"; then\n\
        MOZ_EXTENSIONS=\"\"\n\
    elif test \"$option\" = \"default\"; then\n\
        MOZ_EXTENSIONS=\"$MOZ_EXTENSIONS $MOZ_EXTENSIONS_DEFAULT\"\n\
    elif test `echo \"$option\" | grep -c \\^-` != 0; then\n\
        option=`echo $option | sed \'s/^-//\'`\n\
        MOZ_EXTENSIONS=`echo \"$MOZ_EXTENSIONS\" | sed \"s/ ${option}//\"`\n\
    else\n\
        MOZ_EXTENSIONS=\"$MOZ_EXTENSIONS $option\"\n\
    fi\n\
done],\n\
    MOZ_EXTENSIONS=\"$MOZ_EXTENSIONS_DEFAULT\")\n\
\n\
if test -z \"$MOZ_ENABLE_GNOMEVFS\" && test `echo \"$MOZ_EXTENSIONS\" | grep -c gnomevfs` -ne 0; then\n\
    # Suppress warning on non-X11 platforms\n\
    if test -n \"$MOZ_X11\"; then\n\
        AC_MSG_WARN([Cannot build gnomevfs without required libraries. Removing gnomevfs from MOZ_EXTENSIONS.])\n\
    fi\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|gnomevfs||\'`\n\
fi\n\
\n\
if test -z \"$MOZ_JSDEBUGGER\" && test `echo \"$MOZ_EXTENSIONS\" | grep -c venkman` -ne 0; then\n\
    AC_MSG_WARN([Cannot build venkman without JavaScript debug library. Removing venkman from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|venkman||\'`\n\
fi\n\
\n\
if test \"$MOZ_XUL_APP\" && test `echo \"$MOZ_EXTENSIONS\" | grep -c help` -ne 0; then\n\
    AC_MSG_WARN([Cannot build old help extension with MOZ_XUL_APP set. Removing help from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|help||\'`\n\
fi\n\
\n\
dnl This might be temporary: build tridentprofile only on Windows\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c tridentprofile` -ne 0 && test \"$OS_ARCH\" != \"WINNT\"; then\n\
    AC_MSG_WARN([tridentprofile extension works only on Windows at this time. Removing tridentprofile from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|tridentprofile||\'`\n\
fi\n\
\n\
dnl cookie must be built before tridentprofile. put it at list\'s end.\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c tridentprofile` -ne 0; then\n\
  MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|tridentprofile||\'`\n\
  MOZ_EXTENSIONS=\"$MOZ_EXTENSIONS tridentprofile\"\n\
fi\n\
\n\
dnl xforms requires xtf and webservices and schema-validation\n\
if test -z \"$MOZ_XTF\" && test `echo \"$MOZ_EXTENSIONS\" | grep -c xforms` -ne 0; then\n\
    AC_MSG_WARN([Cannot build XForms without XTF support.  Removing XForms from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|xforms||g\'`\n\
fi\n\
if test -z \"$MOZ_WEBSERVICES\" && test `echo \"$MOZ_EXTENSIONS\" | grep -c xforms` -ne 0; then\n\
    AC_MSG_WARN([Cannot build XForms without webservices.  Removing XForms from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|xforms||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c xforms` -ne 0 && test `echo \"$MOZ_EXTENSIONS\" | grep -c schema-validation` -eq 0; then\n\
    AC_MSG_WARN([Cannot build XForms without schema validation.  Removing XForms from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|xforms||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c auth` -ne 0; then\n\
    AC_MSG_WARN([auth is no longer an extension, use --disable-negotiateauth to disable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|auth||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c xmlextras` -ne 0; then\n\
    AC_MSG_WARN([xmlextras is no longer an extension, use --disable-xmlextras to disable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|xmlextras||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c \'cookie\\|permissions\'` -ne 0; then\n\
    AC_MSG_WARN([cookie and permissions are no longer extensions, use --disable-permissions to disable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|cookie||g; s|permissions||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c webservices` -ne 0; then\n\
    AC_MSG_WARN([webservices is no longer an extension, use --disable-webservices to disable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|webservices||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c pref` -ne 0; then\n\
    AC_MSG_WARN([pref is no longer an extension, use --disable-pref-extensions to disable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|pref||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c universalchardet` -ne 0; then\n\
    AC_MSG_WARN([universalchardet is no longer an extension, use --disable-universalchardet to disable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|universalchardet||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c java` -ne 0; then\n\
    AC_MSG_WARN([java is no longer an extension, use --enable-javaxpcom to enable.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|java||g\'`\n\
fi\n\
\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c spellcheck` -ne 0; then\n\
    AC_MSG_WARN([spellcheck is no longer an extension.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|spellcheck||g\'`\n\
fi\n\
\n\
if test -n \"$MOZ_NO_XPCOM_OBSOLETE\" && test `echo \"$MOZ_EXTENSIONS\" | grep -c sroaming` -ne 0; then\n\
    AC_MSG_WARN([Cannot currently build sroaming without xpcom obsolete -- bug 249343. Removing sroaming from MOZ_EXTENSIONS.])\n\
    MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s|sroaming||\'`\n\
fi\n\
\n\
dnl Remove dupes\n\
MOZ_EXTENSIONS=`${PERL} ${srcdir}/build/unix/uniq.pl ${MOZ_EXTENSIONS}`\n\
\n\
dnl ========================================================\n\
dnl Image decoders\n\
dnl ========================================================\n\
case \"$MOZ_WIDGET_TOOLKIT\" in\n\
beos|windows|os2|mac|cocoa)\n\
    ;;\n\
*)\n\
    if test -z \"$MOZ_ENABLE_GNOMEUI\"; then\n\
       MOZ_IMG_DECODERS_DEFAULT=`echo $MOZ_IMG_DECODERS_DEFAULT | sed -e \'s|icon||\'`                \n\
    fi\n\
    ;;\n\
esac\n\
\n\
MOZ_ARG_ENABLE_STRING(image-decoders,\n\
[  --enable-image-decoders[={mod1,mod2,default,all,none}]\n\
                          Enable specific image decoders],\n\
[ for option in `echo $enableval | sed \'s/,/ /g\'`; do\n\
    if test \"$option\" = \"yes\" || test \"$option\" = \"all\"; then\n\
        MOZ_IMG_DECODERS=\"$MOZ_IMG_DECODERS $MOZ_IMG_DECODERS_DEFAULT\"\n\
    elif test \"$option\" = \"no\" || test \"$option\" = \"none\"; then\n\
        MOZ_IMG_DECODERS=\"\"\n\
    elif test \"$option\" = \"default\"; then\n\
        MOZ_IMG_DECODERS=\"$MOZ_IMG_DECODERS $MOZ_IMG_DECODERS_DEFAULT\"\n\
    elif test `echo \"$option\" | grep -c \\^-` != 0; then\n\
        option=`echo $option | sed \'s/^-//\'`\n\
        MOZ_IMG_DECODERS=`echo \"$MOZ_IMG_DECODERS\" | sed \"s/ ${option}//\"`\n\
    else\n\
        MOZ_IMG_DECODERS=\"$MOZ_IMG_DECODERS $option\"\n\
    fi\n\
done],\n\
    MOZ_IMG_DECODERS=\"$MOZ_IMG_DECODERS_DEFAULT\")\n\
\n\
if test `echo \"$MOZ_IMG_DECODERS\" | grep -c png` -ne 0; then\n\
    MOZ_PNG_CFLAGS=\"$MOZ_PNG_CFLAGS -DMOZ_PNG_READ\"\n\
    AC_MSG_CHECKING([if pnggccrd.c can be compiled without PNG_NO_MMX_CODE])\n\
    AC_TRY_COMPILE([#define MOZ_PNG_READ 1\n\
                    #include \"$_topsrcdir/modules/libimg/png/pnggccrd.c\"],,\n\
                    _results=yes,\n\
                    _results=no)\n\
    AC_MSG_RESULT([$_results])\n\
    if test \"$_results\" = \"no\"; then\n\
        MOZ_PNG_CFLAGS=\"$MOZ_PNG_CFLAGS -DPNG_NO_MMX_CODE\"\n\
    fi\n\
fi\n\
\n\
dnl Dupes are removed in the encoder section because it will also add decoders\n\
\n\
dnl ========================================================\n\
dnl Image encoders\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_STRING(image-encoders,\n\
[  --enable-image-encoders[={mod1,mod2,default,all,none}]\n\
                          Enable specific image encoders],\n\
[ for option in `echo $enableval | sed \'s/,/ /g\'`; do\n\
    if test \"$option\" = \"yes\" || test \"$option\" = \"all\"; then\n\
        addencoder=\"$MOZ_IMG_ENCODERS_DEFAULT\"\n\
    elif test \"$option\" = \"no\" || test \"$option\" = \"none\"; then\n\
        MOZ_IMG_ENCODERS=\"\"\n\
        addencoder=\"\"\n\
    elif test \"$option\" = \"default\"; then\n\
        addencoder=\"$MOZ_IMG_ENCODERS_DEFAULT\"\n\
    elif test `echo \"$option\" | grep -c \\^-` != 0; then\n\
        option=`echo $option | sed \'s/^-//\'`\n\
        addencoder=`echo \"$MOZ_IMG_ENCODERS\" | sed \"s/ ${option}//\"`\n\
    else\n\
        addencoder=\"$option\"\n\
    fi\n\
    MOZ_IMG_ENCODERS=\"$MOZ_IMG_ENCODERS $addencoder\"\n\
done],\n\
    MOZ_IMG_ENCODERS=\"$MOZ_IMG_ENCODERS_DEFAULT\")\n\
\n\
if test `echo \"$MOZ_IMG_ENCODERS\" | grep -c png` -ne 0; then\n\
    MOZ_PNG_CFLAGS=\"$MOZ_PNG_CFLAGS -DMOZ_PNG_WRITE\"\n\
fi\n\
\n\
dnl Remove dupes\n\
MOZ_IMG_DECODERS=`${PERL} ${srcdir}/build/unix/uniq.pl ${MOZ_IMG_DECODERS}`\n\
MOZ_IMG_ENCODERS=`${PERL} ${srcdir}/build/unix/uniq.pl ${MOZ_IMG_ENCODERS}`\n\
\n\
dnl ========================================================\n\
dnl experimental ldap features\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(ldap-experimental,\n\
[  --enable-ldap-experimental\n\
                          Enable LDAP experimental features],\n\
    MOZ_LDAP_XPCOM_EXPERIMENTAL=1,\n\
    MOZ_LDAP_XPCOM_EXPERIMENTAL=)\n\
\n\
dnl ========================================================\n\
dnl MathML on by default\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(mathml,\n\
[  --disable-mathml        Disable MathML support],\n\
    MOZ_MATHML=,\n\
    MOZ_MATHML=1 )\n\
if test \"$MOZ_MATHML\"; then\n\
  AC_DEFINE(MOZ_MATHML)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Canvas\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(canvas,\n\
[  --disable-canvas          Disable html:canvas feature],\n\
    MOZ_ENABLE_CANVAS=,\n\
    MOZ_ENABLE_CANVAS=1 )\n\
if test -n \"$MOZ_ENABLE_CANVAS\"; then\n\
    AC_DEFINE(MOZ_ENABLE_CANVAS)\n\
fi\n\
AC_SUBST(MOZ_ENABLE_CANVAS)\n\
\n\
dnl ========================================================\n\
dnl SVG\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(svg,\n\
[  --disable-svg            Disable SVG support],\n\
    MOZ_SVG=,\n\
    MOZ_SVG=1 )\n\
if test -n \"$MOZ_SVG\"; then\n\
  if test -z \"$MOZ_ENABLE_CAIRO_GFX\"; then\n\
    AC_MSG_ERROR([SVG requires cairo gfx])\n\
  else\n\
    AC_DEFINE(MOZ_SVG)\n\
  fi\n\
fi\n\
\n\
MOZ_SVG_FOREIGNOBJECT=$MOZ_ENABLE_CAIRO_GFX\n\
dnl ========================================================\n\
dnl SVG <foreignObject>\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(svg-foreignobject,\n\
   [  --disable-svg-foreignobject\n\
                        Disable SVG <foreignObject> support],\n\
   MOZ_SVG_FOREIGNOBJECT=,\n\
   MOZ_SVG_FOREIGNOBJECT=1 )\n\
if test \"$MOZ_SVG_FOREIGNOBJECT\"; then\n\
  if test -z \"$MOZ_ENABLE_CAIRO_GFX\"; then\n\
    AC_MSG_ERROR([<foreignobject> requires cairo gfx])\n\
  else\n\
    if test \"$MOZ_SVG\"; then\n\
      AC_DEFINE(MOZ_SVG_FOREIGNOBJECT)\n\
    else\n\
      MOZ_SVG_FOREIGNOBEJCT=\n\
    fi\n\
  fi\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Installer\n\
dnl ========================================================\n\
case \"$target_os\" in\n\
    aix*|solaris*|linux*|msvc*|mks*|cygwin*|mingw*|os2*|wince*)\n\
        MOZ_INSTALLER=1\n\
        ;;\n\
esac\n\
\n\
MOZ_ARG_DISABLE_BOOL(installer,\n\
[  --disable-installer     Disable building of installer],\n\
    MOZ_INSTALLER=,\n\
    MOZ_INSTALLER=1 )\n\
if test -n \"$MOZ_INSTALLER\" -a -n \"$MOZ_XUL_APP\" -a \"$OS_ARCH\" = \"WINNT\"; then\n\
    # Disable installer for Windows builds that use the new toolkit if NSIS\n\
    # isn\'t in the path.\n\
    AC_PATH_PROGS(MAKENSIS, makensis)\n\
    if test -z \"$MAKENSIS\" || test \"$MAKENSIS\" = \":\"; then\n\
        AC_MSG_ERROR([To build the installer makensis is required in your path. To build without the installer reconfigure using --disable-installer.])\n\
    fi\n\
    # The Windows build for NSIS requires the iconv command line utility to\n\
    # convert the charset of the locale files.\n\
    AC_PATH_PROGS(HOST_ICONV, $HOST_ICONV \"iconv\", \"\")\n\
    if test -z \"$HOST_ICONV\"; then\n\
        AC_MSG_ERROR([To build the installer iconv is required in your path. To build without the installer reconfigure using --disable-installer.])\n\
    fi\n\
fi\n\
\n\
# Automatically disable installer if xpinstall isn\'t built\n\
if test -z \"$MOZ_XPINSTALL\"; then\n\
    MOZ_INSTALLER=\n\
fi\n\
AC_SUBST(MOZ_INSTALLER)\n\
\n\
AC_MSG_CHECKING([for tar archiver])\n\
AC_CHECK_PROGS(TAR, gnutar gtar tar, \"\")\n\
if test -z \"$TAR\"; then\n\
    AC_MSG_ERROR([no tar archiver found in \\$PATH])\n\
fi\n\
AC_MSG_RESULT([$TAR])\n\
AC_SUBST(TAR)\n\
\n\
dnl ========================================================\n\
dnl Updater\n\
dnl ========================================================\n\
\n\
MOZ_ARG_DISABLE_BOOL(updater,\n\
[  --disable-updater       Disable building of updater],\n\
    MOZ_UPDATER=,\n\
    MOZ_UPDATER=1 )\n\
# The Windows build requires the iconv command line utility\n\
# in order to build the updater.\n\
case \"$target_os\" in\n\
    msvc*|mks*|cygwin*|mingw*|wince*)\n\
        if test -n \"$MOZ_UPDATER\"; then\n\
            AC_MSG_CHECKING([for iconv])\n\
            AC_CHECK_PROGS(HOST_ICONV, $HOST_ICONV \"iconv\", \"\")\n\
            if test -z \"$HOST_ICONV\"; then\n\
                AC_MSG_ERROR([iconv not found in \\$PATH])\n\
            fi\n\
        fi\n\
        ;;\n\
esac\n\
AC_SUBST(MOZ_UPDATER)\n\
\n\
# app update channel is \'default\' when not supplied.\n\
MOZ_ARG_ENABLE_STRING([update-channel],\n\
[  --enable-update-channel=CHANNEL\n\
                           Select application update channel (default=default)],\n\
    MOZ_UPDATE_CHANNEL=`echo $enableval | tr A-Z a-z`)\n\
\n\
if test -z \"$MOZ_UPDATE_CHANNEL\"; then\n\
    MOZ_UPDATE_CHANNEL=default\n\
fi\n\
AC_DEFINE_UNQUOTED(MOZ_UPDATE_CHANNEL, $MOZ_UPDATE_CHANNEL)\n\
\n\
# tools/update-packaging is not checked out by default.\n\
MOZ_ARG_ENABLE_BOOL(update-packaging,\n\
[  --enable-update-packaging\n\
                           Enable tools/update-packaging],\n\
    MOZ_UPDATE_PACKAGING=1,\n\
    MOZ_UPDATE_PACKAGING= )\n\
AC_SUBST(MOZ_UPDATE_PACKAGING)\n\
\n\
dnl ========================================================\n\
dnl ActiveX\n\
dnl ========================================================\n\
\n\
MOZ_ARG_DISABLE_BOOL(xpconnect-idispatch,\n\
[  --disable-xpconnect-idispatch\n\
                          Disable building of xpconnect support for IDispatch\n\
                          (win32 only)],\n\
    XPC_IDISPATCH_SUPPORT=,\n\
    XPC_IDISPATCH_SUPPORT=1)\n\
AC_SUBST(XPC_IDISPATCH_SUPPORT)\n\
\n\
MOZ_ARG_DISABLE_BOOL(activex,\n\
[  --disable-activex       Disable building of ActiveX control (win32 only)],\n\
    MOZ_NO_ACTIVEX_SUPPORT=1,\n\
    MOZ_NO_ACTIVEX_SUPPORT= )\n\
AC_SUBST(MOZ_NO_ACTIVEX_SUPPORT)\n\
\n\
MOZ_ARG_DISABLE_BOOL(activex-scripting,\n\
[  --disable-activex-scripting\n\
                          Disable building of ActiveX scripting support (win32)],\n\
    MOZ_ACTIVEX_SCRIPTING_SUPPORT=,\n\
    MOZ_ACTIVEX_SCRIPTING_SUPPORT=1)\n\
AC_SUBST(MOZ_ACTIVEX_SCRIPTING_SUPPORT)\n\
\n\
if test -n \"$MOZ_NO_ACTIVEX_SUPPORT\" -a -n \"$MOZ_ACTIVEX_SCRIPTING_SUPPORT\";\n\
then\n\
    AC_MSG_ERROR([Cannot enable ActiveX scripting support when ActiveX support is disabled.])\n\
fi\n\
\n\
dnl ========================================================\n\
dnl leaky\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(leaky,\n\
[  --enable-leaky          Build leaky memory tool],\n\
    MOZ_LEAKY=1,\n\
    MOZ_LEAKY=)\n\
\n\
\n\
dnl ========================================================\n\
dnl xpctools\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(xpctools,\n\
[  --enable-xpctools       Build JS profiling tool],\n\
    MOZ_XPCTOOLS=1,\n\
    MOZ_XPCTOOLS= )\n\
\n\
\n\
dnl ========================================================\n\
dnl build the tests by default\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(tests,\n\
[  --disable-tests         Do not build test libraries & programs],\n\
    ENABLE_TESTS=,\n\
    ENABLE_TESTS=1 )\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Module specific options\n\
dnl =\n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Individual module options)\n\
\n\
dnl ========================================================\n\
dnl = Enable Lea malloc in xpcom. OFF by default.\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(xpcom-lea,\n\
[  --enable-xpcom-lea      Use Lea malloc in xpcom ],\n\
    XPCOM_USE_LEA=1,\n\
    XPCOM_USE_LEA= )\n\
if test -n \"$XPCOM_USE_LEA\"; then\n\
    AC_DEFINE(XPCOM_USE_LEA)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Enable places (new history/bookmarks)\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(places,\n\
[  --enable-places        Enable \'places\' bookmark/history implementation],\n\
    MOZ_PLACES=1,\n\
    MOZ_PLACES= )\n\
if test -n \"$MOZ_PLACES\"; then\n\
    AC_DEFINE(MOZ_PLACES)\n\
    MOZ_MORK=\n\
    MOZ_MORKREADER=1\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Enable places bookmarks (use places for bookmarks)\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(places-bookmarks,\n\
[  --enable-places_bookmarks        Enable \'places\' for bookmarks backend],\n\
    MOZ_PLACES_BOOKMARKS=1,\n\
    MOZ_PLACES_BOOKMARKS= )\n\
if test -n \"$MOZ_PLACES_BOOKMARKS\"; then\n\
    AC_DEFINE(MOZ_PLACES_BOOKMARKS)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Disable feed handling components\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(feeds,\n\
[  --disable-feeds        Disable feed handling and processing components],\n\
    MOZ_FEEDS=,\n\
    MOZ_FEEDS=1 )\n\
if test -n \"$MOZ_FEEDS\"; then\n\
    AC_DEFINE(MOZ_FEEDS)\n\
else\n\
    if test \"$MOZ_BUILD_APP\" = \"browser\"; then\n\
        AC_MSG_ERROR([Cannot build Firefox with --disable-feeds.])\n\
    fi\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Enable mozStorage\n\
dnl = XXX need to implement --with-system-sqlite3 (see bug 263381)\n\
dnl ========================================================\n\
dnl Implicitly enabled by default if building calendar or places\n\
MOZ_ARG_ENABLE_BOOL(storage,\n\
[  --enable-storage        Enable mozStorage module and related components],\n\
    MOZ_STORAGE=1,\n\
    MOZ_STORAGE= )\n\
if test -n \"$MOZ_STORAGE\"; then\n\
    AC_DEFINE(MOZ_STORAGE)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Enable safe browsing (anti-phishing)\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(safe-browsing,\n\
[  --enable-safe-browsing        Enable safe browsing (anti-phishing) implementation],\n\
    MOZ_SAFE_BROWSING=1,\n\
    MOZ_SAFE_BROWSING= )\n\
if test -n \"$MOZ_SAFE_BROWSING\"; then\n\
    AC_DEFINE(MOZ_SAFE_BROWSING)\n\
fi\n\
AC_SUBST(MOZ_SAFE_BROWSING)\n\
\n\
dnl ========================================================\n\
dnl = Enable url-classifier\n\
dnl ========================================================\n\
dnl Implicitly enabled by default if building with safe-browsing\n\
if test -n \"$MOZ_SAFE_BROWSING\"; then\n\
    MOZ_URL_CLASSIFIER=1\n\
fi\n\
MOZ_ARG_ENABLE_BOOL(url-classifier,\n\
[  --enable-url-classifier        Enable url classifier module],\n\
    MOZ_URL_CLASSIFIER=1,\n\
    MOZ_URL_CLASSIFIER= )\n\
if test -n \"$MOZ_URL_CLASSIFIER\"; then\n\
    AC_DEFINE(MOZ_URL_CLASSIFIER)\n\
fi\n\
AC_SUBST(MOZ_URL_CLASSIFIER)\n\
\n\
dnl ========================================================\n\
dnl = Enable Ultrasparc specific optimizations for JS\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(js-ultrasparc,\n\
[  --enable-js-ultrasparc  Use UltraSPARC optimizations in JS],\n\
    JS_ULTRASPARC_OPTS=1,\n\
    JS_ULTRASPARC_OPTS= )\n\
\n\
dnl only enable option for ultrasparcs\n\
if test `echo \"$target_os\" | grep -c \\^solaris 2>/dev/null` = 0 -o \\\n\
    \"$OS_TEST\" != \"sun4u\"; then\n\
    JS_ULTRASPARC_OPTS=\n\
fi\n\
AC_SUBST(JS_ULTRASPARC_OPTS)\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Feature options that require extra sources to be pulled\n\
dnl =\n\
dnl ========================================================\n\
dnl MOZ_ARG_HEADER(Features that require extra sources)\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Debugging Options\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Debugging and Optimizations)\n\
\n\
dnl ========================================================\n\
dnl = Disable building with debug info.\n\
dnl = Debugging is OFF by default\n\
dnl ========================================================\n\
if test -z \"$MOZ_DEBUG_FLAGS\"\n\
then\n\
    case \"$target\" in\n\
    *-irix*)\n\
        if test \"$GNU_CC\"; then\n\
            GCC_VERSION=`$CC -v 2>&1 | awk \'/version/ { print $3 }\'`\n\
            case \"$GCC_VERSION\" in\n\
            2.95.*)\n\
                MOZ_DEBUG_FLAGS=\"\"\n\
                ;;\n\
            *)\n\
                MOZ_DEBUG_FLAGS=\"-g\"\n\
                ;;\n\
            esac\n\
        else\n\
            MOZ_DEBUG_FLAGS=\"-g\"\n\
        fi\n\
        ;;\n\
    *)\n\
    	MOZ_DEBUG_FLAGS=\"-g\"\n\
        ;;\n\
    esac\n\
fi\n\
\n\
MOZ_ARG_ENABLE_STRING(debug,\n\
[  --enable-debug[=DBG]    Enable building with developer debug info\n\
                          (Using compiler flags DBG)],\n\
[ if test \"$enableval\" != \"no\"; then\n\
    MOZ_DEBUG=1\n\
    if test -n \"$enableval\" && test \"$enableval\" != \"yes\"; then\n\
        MOZ_DEBUG_FLAGS=`echo $enableval | sed -e \'s|\\\\\\ | |g\'`\n\
    fi\n\
  else\n\
    MOZ_DEBUG=\n\
  fi ],\n\
  MOZ_DEBUG=)\n\
\n\
MOZ_DEBUG_ENABLE_DEFS=\"-DDEBUG -D_DEBUG\"\n\
 case \"${target_os}\" in\n\
    beos*)\n\
        MOZ_DEBUG_ENABLE_DEFS=\"$MOZ_DEBUG_ENABLE_DEFS -DDEBUG_${USER}\"\n\
        ;;\n\
    msvc*|mks*|cygwin*|mingw*|os2*|wince*)\n\
        MOZ_DEBUG_ENABLE_DEFS=\"$MOZ_DEBUG_ENABLE_DEFS -DDEBUG_`echo ${USERNAME} | sed -e \'s| |_|g\'`\"\n\
        ;;\n\
    *) \n\
        MOZ_DEBUG_ENABLE_DEFS=\"$MOZ_DEBUG_ENABLE_DEFS -DDEBUG_`$WHOAMI`\"\n\
        ;;\n\
  esac\n\
MOZ_DEBUG_ENABLE_DEFS=\"$MOZ_DEBUG_ENABLE_DEFS -DTRACING\"\n\
\n\
MOZ_DEBUG_DISABLE_DEFS=\"-DNDEBUG -DTRIMMED\"\n\
\n\
if test -n \"$MOZ_DEBUG\"; then\n\
    AC_MSG_CHECKING([for valid debug flags])\n\
    _SAVE_CFLAGS=$CFLAGS\n\
    CFLAGS=\"$CFLAGS $MOZ_DEBUG_FLAGS\"\n\
    AC_TRY_COMPILE([#include <stdio.h>], \n\
        [printf(\"Hello World\\n\");],\n\
        _results=yes,\n\
        _results=no)\n\
    AC_MSG_RESULT([$_results])\n\
    if test \"$_results\" = \"no\"; then\n\
        AC_MSG_ERROR([These compiler flags are invalid: $MOZ_DEBUG_FLAGS])\n\
    fi\n\
    CFLAGS=$_SAVE_CFLAGS\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Enable code optimization. ON by default.\n\
dnl ========================================================\n\
if test -z \"$MOZ_OPTIMIZE_FLAGS\"; then\n\
	MOZ_OPTIMIZE_FLAGS=\"-O\"\n\
fi\n\
\n\
MOZ_ARG_ENABLE_STRING(optimize,\n\
[  --disable-optimize      Disable compiler optimization\n\
  --enable-optimize=[OPT] Specify compiler optimization flags [OPT=-O]],\n\
[ if test \"$enableval\" != \"no\"; then\n\
    MOZ_OPTIMIZE=1\n\
    if test -n \"$enableval\" && test \"$enableval\" != \"yes\"; then\n\
        MOZ_OPTIMIZE_FLAGS=`echo \"$enableval\" | sed -e \'s|\\\\\\ | |g\'`\n\
        MOZ_OPTIMIZE=2\n\
    fi\n\
else\n\
    MOZ_OPTIMIZE=\n\
fi ], MOZ_OPTIMIZE=1)\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test -n \"$MOZ_OPTIMIZE\"; then\n\
    AC_MSG_CHECKING([for valid optimization flags])\n\
    _SAVE_CFLAGS=$CFLAGS\n\
    CFLAGS=\"$CFLAGS $MOZ_OPTIMIZE_FLAGS\"\n\
    AC_TRY_COMPILE([#include <stdio.h>], \n\
        [printf(\"Hello World\\n\");],\n\
        _results=yes,\n\
        _results=no)\n\
    AC_MSG_RESULT([$_results])\n\
    if test \"$_results\" = \"no\"; then\n\
        AC_MSG_ERROR([These compiler flags are invalid: $MOZ_OPTIMIZE_FLAGS])\n\
    fi\n\
    CFLAGS=$_SAVE_CFLAGS\n\
fi\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
AC_SUBST(MOZ_OPTIMIZE)\n\
AC_SUBST(MOZ_OPTIMIZE_FLAGS)\n\
AC_SUBST(MOZ_OPTIMIZE_LDFLAGS)\n\
\n\
dnl ========================================================\n\
dnl = Enable/disable debug for specific modules only\n\
dnl =   module names beginning with ^ will be disabled \n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_STRING(debug-modules,\n\
[  --enable-debug-modules  Enable/disable debug info for specific modules],\n\
[ MOZ_DEBUG_MODULES=`echo $enableval| sed \'s/,/ /g\'` ] )\n\
\n\
dnl ========================================================\n\
dnl = Enable/disable generation of debugger info for specific modules only\n\
dnl =    the special module name ALL_MODULES can be used to denote all modules\n\
dnl =    module names beginning with ^ will be disabled\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_STRING(debugger-info-modules,\n\
[  --enable-debugger-info-modules\n\
                          Enable/disable debugger info for specific modules],\n\
[ for i in `echo $enableval | sed \'s/,/ /g\'`; do\n\
      dnl note that the list of module names is reversed as it is copied\n\
      dnl this is important, as it will allow config.mk to interpret stuff like\n\
      dnl \"^ALL_MODULES xpcom\" properly\n\
      if test \"$i\" = \"no\"; then\n\
        i=\"^ALL_MODULES\"\n\
      fi\n\
      if test \"$i\" = \"yes\"; then\n\
        i=\"ALL_MODULES\"\n\
      fi\n\
      MOZ_DBGRINFO_MODULES=\"$i $MOZ_DBGRINFO_MODULES\";\n\
    done ])\n\
\n\
dnl ========================================================\n\
dnl Enable garbage collector\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(boehm,\n\
[  --enable-boehm          Enable the Boehm Garbage Collector],\n\
    GC_LEAK_DETECTOR=1,\n\
    GC_LEAK_DETECTOR= )\n\
if test -n \"$GC_LEAK_DETECTOR\"; then\n\
    AC_DEFINE(GC_LEAK_DETECTOR)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Disable runtime logging checks\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(logging,\n\
[  --disable-logging       Disable logging facilities],\n\
    NS_DISABLE_LOGGING=1,\n\
    NS_DISABLE_LOGGING= )\n\
if test \"$NS_DISABLE_LOGGING\"; then\n\
    AC_DEFINE(NS_DISABLE_LOGGING)\n\
else\n\
    AC_DEFINE(MOZ_LOGGING)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = dnl This will enable logging of addref, release, ctor, dtor.\n\
dnl ========================================================\n\
_ENABLE_LOGREFCNT=42\n\
MOZ_ARG_ENABLE_BOOL(logrefcnt,\n\
[  --enable-logrefcnt      Enable logging of refcounts (default=debug) ],\n\
    _ENABLE_LOGREFCNT=1,\n\
    _ENABLE_LOGREFCNT= )\n\
if test \"$_ENABLE_LOGREFCNT\" = \"1\"; then\n\
    AC_DEFINE(FORCE_BUILD_REFCNT_LOGGING)\n\
elif test -z \"$_ENABLE_LOGREFCNT\"; then\n\
    AC_DEFINE(NO_BUILD_REFCNT_LOGGING)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Use malloc wrapper lib\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(wrap-malloc,\n\
[  --enable-wrap-malloc    Wrap malloc calls (gnu linker only)],\n\
    _WRAP_MALLOC=1,\n\
    _WRAP_MALLOC= )\n\
\n\
if test -n \"$_WRAP_MALLOC\"; then\n\
    if test \"$GNU_CC\"; then\n\
    WRAP_MALLOC_CFLAGS=\"${LDFLAGS} -Wl,--wrap -Wl,malloc -Wl,--wrap -Wl,free -Wl,--wrap -Wl,realloc -Wl,--wrap -Wl,__builtin_new -Wl,--wrap -Wl,__builtin_vec_new -Wl,--wrap -Wl,__builtin_delete -Wl,--wrap -Wl,__builtin_vec_delete -Wl,--wrap -Wl,PR_Free -Wl,--wrap -Wl,PR_Malloc -Wl,--wrap -Wl,PR_Calloc -Wl,--wrap -Wl,PR_Realloc\"\n\
    MKSHLIB=\'$(CXX) $(DSO_LDOPTS) $(WRAP_MALLOC_CFLAGS) -o $@\'\n\
    fi\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Location of malloc wrapper lib\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(wrap-malloc,\n\
[  --with-wrap-malloc=DIR  Location of malloc wrapper library],\n\
    WRAP_MALLOC_LIB=$withval)\n\
\n\
dnl ========================================================\n\
dnl = Use Electric Fence\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(efence,\n\
[  --enable-efence         Link with Electric Fence],\n\
    _ENABLE_EFENCE=1,\n\
    _ENABLE_EFENCE= )\n\
if test -n \"$_ENABLE_EFENCE\"; then\n\
    AC_CHECK_LIB(efence,malloc)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl jprof\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(jprof,\n\
[  --enable-jprof          Enable jprof profiling tool (needs mozilla/tools/jprof)],\n\
    MOZ_JPROF=1,\n\
    MOZ_JPROF= )\n\
if test -n \"$MOZ_JPROF\"; then\n\
    AC_DEFINE(MOZ_JPROF)\n\
fi\n\
\n\
\n\
dnl ========================================================\n\
dnl = Enable stripping of libs & executables\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(strip,\n\
[  --enable-strip          Enable stripping of libs & executables ],\n\
    ENABLE_STRIP=1,\n\
    ENABLE_STRIP= )\n\
\n\
dnl ========================================================\n\
dnl = --enable-elf-dynstr-gc\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(elf-dynstr-gc,\n\
[  --enable-elf-dynstr-gc  Enable elf dynstr garbage collector (opt builds only)],\n\
    USE_ELF_DYNSTR_GC=1,\n\
    USE_ELF_DYNSTR_GC= )\n\
\n\
dnl ========================================================\n\
dnl = --enable-old-abi-compat-wrappers\n\
dnl ========================================================\n\
dnl on x86 linux, the current builds of some popular plugins (notably\n\
dnl flashplayer and real) expect a few builtin symbols from libgcc\n\
dnl which were available in some older versions of gcc.  However,\n\
dnl they\'re _NOT_ available in newer versions of gcc (eg 3.1), so if\n\
dnl we want those plugin to work with a gcc-3.1 built binary, we need\n\
dnl to provide these symbols.  MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS defaults\n\
dnl to true on x86 linux, and false everywhere else.\n\
dnl\n\
\n\
MOZ_ARG_ENABLE_BOOL(old-abi-compat-wrappers,\n\
[  --enable-old-abi-compat-wrappers\n\
                          Support old GCC ABI symbols to ease the pain \n\
                          of the linux compiler change],\n\
    MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS=1,\n\
    MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS= )\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test \"$MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS\"; then\n\
    AC_LANG_SAVE\n\
    AC_LANG_CPLUSPLUS\n\
    AC_CHECK_FUNCS(__builtin_vec_new __builtin_vec_delete __builtin_new __builtin_delete __pure_virtual)\n\
    AC_LANG_RESTORE\n\
    AC_DEFINE(MOZ_ENABLE_OLD_ABI_COMPAT_WRAPPERS)\n\
fi\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl ========================================================\n\
dnl = --enable-prebinding\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(prebinding,\n\
[  --enable-prebinding     Enable prebinding (Mac OS X only)],\n\
    USE_PREBINDING=1,\n\
    USE_PREBINDING= )\n\
\n\
dnl ========================================================\n\
dnl = \n\
dnl = Profiling and Instrumenting\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Profiling and Instrumenting)\n\
\n\
dnl ========================================================\n\
dnl = Enable timeline service, which provides lightweight\n\
dnl = instrumentation of mozilla for performance measurement.\n\
dnl = Timeline is off by default.\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(timeline,\n\
[  --enable-timeline       Enable timeline services ],\n\
    MOZ_TIMELINE=1,\n\
    MOZ_TIMELINE= )\n\
if test -n \"$MOZ_TIMELINE\"; then\n\
    AC_DEFINE(MOZ_TIMELINE)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Turn on reflow counting\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(reflow-perf,\n\
[  --enable-reflow-perf    Enable reflow performance tracing],\n\
    MOZ_REFLOW_PERF=1,\n\
    MOZ_REFLOW_PERF= )\n\
if test -n \"$MOZ_REFLOW_PREF\"; then\n\
    AC_DEFINE(MOZ_REFLOW_PREF)\n\
fi\n\
AC_SUBST(MOZ_REFLOW_PERF)\n\
\n\
dnl ========================================================\n\
dnl Enable performance metrics.\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(perf-metrics,\n\
[  --enable-perf-metrics   Enable performance metrics],\n\
    MOZ_PERF_METRICS=1,\n\
    MOZ_PERF_METRICS= )\n\
if test -n \"$MOZ_PERF_METRICS\"; then\n\
    AC_DEFINE(MOZ_PERF_METRICS)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Enable code size metrics.\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(codesighs,\n\
[  --enable-codesighs      Enable code size analysis tools],\n\
    _ENABLE_CODESIGHS=1,\n\
    _ENABLE_CODESIGHS= )\n\
if test -n \"$_ENABLE_CODESIGHS\"; then\n\
    if test -d $srcdir/tools/codesighs; then \n\
        MOZ_MAPINFO=1\n\
    else\n\
        AC_MSG_ERROR([Codesighs directory $srcdir/tools/codesighs required.])\n\
    fi\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Enable trace malloc\n\
dnl ========================================================\n\
NS_TRACE_MALLOC=${MOZ_TRACE_MALLOC}\n\
MOZ_ARG_ENABLE_BOOL(trace-malloc,\n\
[  --enable-trace-malloc   Enable malloc tracing],\n\
    NS_TRACE_MALLOC=1,\n\
    NS_TRACE_MALLOC= )\n\
if test \"$NS_TRACE_MALLOC\"; then\n\
  # Please, Mr. Linker Man, don\'t take away our symbol names\n\
  MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\n\
  USE_ELF_DYNSTR_GC=\n\
  AC_DEFINE(NS_TRACE_MALLOC)\n\
fi\n\
AC_SUBST(NS_TRACE_MALLOC)\n\
\n\
dnl ========================================================\n\
dnl = Add support for Eazel profiler\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(eazel-profiler-support,\n\
[  --enable-eazel-profiler-support\n\
                          Enable Corel/Eazel profiler support],\n\
    ENABLE_EAZEL_PROFILER=1,\n\
    ENABLE_EAZEL_PROFILER= )\n\
if test -n \"$ENABLE_EAZEL_PROFILER\"; then\n\
    AC_DEFINE(ENABLE_EAZEL_PROFILER)\n\
    USE_ELF_DYNSTR_GC=\n\
    MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\n\
    EAZEL_PROFILER_CFLAGS=\"-g -O -gdwarf-2 -finstrument-functions -D__NO_STRING_INLINES  -D__NO_MATH_INLINES\"\n\
    EAZEL_PROFILER_LIBS=\"-lprofiler -lpthread\"\n\
fi\n\
\n\
MOZ_ARG_ENABLE_STRING(profile-modules,\n\
[  --enable-profile-modules\n\
                          Enable/disable profiling for specific modules],\n\
[ MOZ_PROFILE_MODULES=`echo $enableval| sed \'s/,/ /g\'` ] )\n\
\n\
MOZ_ARG_ENABLE_BOOL(insure,\n\
[  --enable-insure         Enable insure++ instrumentation (linux only)],\n\
    _ENABLE_INSURE=1,\n\
    _ENABLE_INSURE= )\n\
if test -n \"$_ENABLE_INSURE\"; then\n\
    MOZ_INSURE=\"insure\"\n\
    MOZ_INSURIFYING=1\n\
    MOZ_INSURE_DIRS=\".\"\n\
    MOZ_INSURE_EXCLUDE_DIRS=\"config\"\n\
fi\n\
\n\
MOZ_ARG_WITH_STRING(insure-dirs,\n\
[  --with-insure-dirs=DIRS\n\
                          Dirs to instrument with insure ],\n\
    MOZ_INSURE_DIRS=$withval )\n\
\n\
MOZ_ARG_WITH_STRING(insure-exclude-dirs,\n\
[  --with-insure-exclude-dirs=DIRS\n\
                          Dirs to not instrument with insure ],\n\
    MOZ_INSURE_EXCLUDE_DIRS=\"config $withval\" )\n\
\n\
dnl ========================================================\n\
dnl = Support for Quantify (Windows)\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(quantify,\n\
[  --enable-quantify      Enable Quantify support (Windows only) ],\n\
    MOZ_QUANTIFY=1,\n\
    MOZ_QUANTIFY= )\n\
\n\
dnl ========================================================\n\
dnl = Support for demangling undefined symbols\n\
dnl ========================================================\n\
if test -z \"$SKIP_LIBRARY_CHECKS\"; then\n\
    AC_LANG_SAVE\n\
    AC_LANG_CPLUSPLUS\n\
    AC_CHECK_FUNCS(__cxa_demangle, HAVE_DEMANGLE=1, HAVE_DEMANGLE=)\n\
    AC_LANG_RESTORE\n\
fi\n\
\n\
# Demangle only for debug or trace-malloc builds\n\
MOZ_DEMANGLE_SYMBOLS=\n\
if test \"$HAVE_DEMANGLE\" -a \"$HAVE_GCC3_ABI\" && test \"$MOZ_DEBUG\" -o \"$NS_TRACE_MALLOC\"; then\n\
    MOZ_DEMANGLE_SYMBOLS=1\n\
    AC_DEFINE(MOZ_DEMANGLE_SYMBOLS)\n\
fi\n\
AC_SUBST(MOZ_DEMANGLE_SYMBOLS)\n\
\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Misc. Options\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Misc. Options)\n\
\n\
dnl ========================================================\n\
dnl update xterm title\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(xterm-updates,\n\
[  --enable-xterm-updates  Update XTERM titles with current command.],\n\
    MOZ_UPDATE_XTERM=1,\n\
    MOZ_UPDATE_XTERM= )\n\
\n\
dnl =========================================================\n\
dnl = Chrome format\n\
dnl =========================================================\n\
MOZ_ARG_ENABLE_STRING([chrome-format],\n\
[  --enable-chrome-format=jar|flat|both|symlink\n\
                          Select FORMAT of chrome files (default=jar)],\n\
    MOZ_CHROME_FILE_FORMAT=`echo $enableval | tr A-Z a-z`)\n\
\n\
if test -z \"$MOZ_CHROME_FILE_FORMAT\"; then\n\
    MOZ_CHROME_FILE_FORMAT=jar\n\
fi\n\
\n\
if test \"$MOZ_CHROME_FILE_FORMAT\" != \"jar\" && \n\
    test \"$MOZ_CHROME_FILE_FORMAT\" != \"flat\" &&\n\
    test \"$MOZ_CHROME_FILE_FORMAT\" != \"symlink\" &&\n\
    test \"$MOZ_CHROME_FILE_FORMAT\" != \"both\"; then\n\
    AC_MSG_ERROR([--enable-chrome-format must be set to either jar, flat, both, or symlink])\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Define default location for MOZILLA_FIVE_HOME\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(default-mozilla-five-home,\n\
[  --with-default-mozilla-five-home\n\
                          Set the default value for MOZILLA_FIVE_HOME],\n\
[ val=`echo $withval`\n\
  AC_DEFINE_UNQUOTED(MOZ_DEFAULT_MOZILLA_FIVE_HOME,\"$val\") ])\n\
\n\
dnl ========================================================\n\
dnl = Location of the mozilla user directory (default is ~/.mozilla).],\n\
dnl ========================================================\n\
MOZ_ARG_WITH_STRING(user-appdir,\n\
[  --with-user-appdir=DIR  Set user-specific appdir (default=.mozilla)],\n\
[ val=`echo $withval`\n\
if echo \"$val\" | grep \"\\/\" >/dev/null; then\n\
    AC_MSG_ERROR(\"Homedir must be single relative path.\")\n\
else \n\
    MOZ_USER_DIR=\"$val\"\n\
fi])\n\
\n\
AC_DEFINE_UNQUOTED(MOZ_USER_DIR,\"$MOZ_USER_DIR\")\n\
\n\
dnl ========================================================\n\
dnl = Doxygen configuration\n\
dnl ========================================================\n\
dnl Use commas to specify multiple dirs to this arg\n\
MOZ_DOC_INPUT_DIRS=\'./dist/include ./dist/idl\'\n\
MOZ_ARG_WITH_STRING(doc-input-dirs,\n\
[  --with-doc-input-dirs=DIRS\n\
                          Header/idl dirs to create docs from],\n\
[ MOZ_DOC_INPUT_DIRS=`echo \"$withval\" | sed \"s/,/ /g\"` ] )\n\
AC_SUBST(MOZ_DOC_INPUT_DIRS)\n\
\n\
dnl Use commas to specify multiple dirs to this arg\n\
MOZ_DOC_INCLUDE_DIRS=\'./dist/include ./dist/include/nspr\'\n\
MOZ_ARG_WITH_STRING(doc-include-dirs,\n\
[  --with-doc-include-dirs=DIRS  \n\
                          Include dirs to preprocess doc headers],\n\
[ MOZ_DOC_INCLUDE_DIRS=`echo \"$withval\" | sed \"s/,/ /g\"` ] )\n\
AC_SUBST(MOZ_DOC_INCLUDE_DIRS)\n\
\n\
MOZ_DOC_OUTPUT_DIR=\'./dist/docs\'\n\
MOZ_ARG_WITH_STRING(doc-output-dir,\n\
[  --with-doc-output-dir=DIR\n\
                          Dir to generate docs into],\n\
[ MOZ_DOC_OUTPUT_DIR=$withval ] )\n\
AC_SUBST(MOZ_DOC_OUTPUT_DIR)\n\
\n\
if test -z \"$SKIP_COMPILER_CHECKS\"; then\n\
dnl ========================================================\n\
dnl =\n\
dnl = Compiler Options\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Compiler Options)\n\
\n\
dnl ========================================================\n\
dnl Check for gcc -pipe support\n\
dnl ========================================================\n\
AC_MSG_CHECKING([for gcc -pipe support])\n\
if test -n \"$GNU_CC\" && test -n \"$GNU_CXX\" && test -n \"$GNU_AS\"; then\n\
    echo \'#include <stdio.h>\' > dummy-hello.c\n\
    echo \'int main() { printf(\"Hello World\\n\"); exit(0); }\' >> dummy-hello.c\n\
    ${CC} -S dummy-hello.c -o dummy-hello.s 2>&5\n\
    cat dummy-hello.s | ${AS_BIN} -o dummy-hello.S - 2>&5\n\
    if test $? = 0; then\n\
        _res_as_stdin=\"yes\"\n\
    else\n\
        _res_as_stdin=\"no\"\n\
    fi\n\
    if test \"$_res_as_stdin\" = \"yes\"; then\n\
        _SAVE_CFLAGS=$CFLAGS\n\
        CFLAGS=\"$CFLAGS -pipe\"\n\
        AC_TRY_COMPILE( [ #include <stdio.h> ],\n\
            [printf(\"Hello World\\n\");],\n\
            [_res_gcc_pipe=\"yes\"],\n\
            [_res_gcc_pipe=\"no\"] )\n\
        CFLAGS=$_SAVE_CFLAGS\n\
    fi\n\
    if test \"$_res_as_stdin\" = \"yes\" && test \"$_res_gcc_pipe\" = \"yes\"; then\n\
        _res=\"yes\";\n\
        CFLAGS=\"$CFLAGS -pipe\"\n\
        CXXFLAGS=\"$CXXFLAGS -pipe\"\n\
    else\n\
        _res=\"no\"\n\
    fi\n\
    rm -f dummy-hello.c dummy-hello.s dummy-hello.S dummy-hello a.out\n\
    AC_MSG_RESULT([$_res])\n\
else\n\
    AC_MSG_RESULT([no])\n\
fi\n\
\n\
dnl pass -Wno-long-long to the compiler\n\
MOZ_ARG_ENABLE_BOOL(long-long-warning,\n\
[  --enable-long-long-warning \n\
                          Warn about use of non-ANSI long long type],\n\
    _IGNORE_LONG_LONG_WARNINGS=,\n\
    _IGNORE_LONG_LONG_WARNINGS=1)\n\
\n\
if test \"$_IGNORE_LONG_LONG_WARNINGS\"; then\n\
     _SAVE_CFLAGS=\"$CFLAGS\"\n\
     CFLAGS=\"$CFLAGS ${_COMPILER_PREFIX}-Wno-long-long\"\n\
     AC_MSG_CHECKING([whether compiler supports -Wno-long-long])\n\
     AC_TRY_COMPILE([], [return(0);], \n\
	[ _WARNINGS_CFLAGS=\"${_WARNINGS_CFLAGS} ${_COMPILER_PREFIX}-Wno-long-long\"\n\
	  _WARNINGS_CXXFLAGS=\"${_WARNINGS_CXXFLAGS} ${_COMPILER_PREFIX}-Wno-long-long\"\n\
	  result=\"yes\" ], result=\"no\")\n\
     AC_MSG_RESULT([$result])\n\
     CFLAGS=\"$_SAVE_CFLAGS\"\n\
fi\n\
\n\
dnl Test for profiling options\n\
dnl Under gcc 3.3, use -fprofile-arcs/-fbranch-probabilities\n\
dnl Under gcc 3.4+, use -fprofile-generate/-fprofile-use\n\
\n\
_SAVE_CFLAGS=\"$CFLAGS\"\n\
CFLAGS=\"$CFLAGS -fprofile-generate\"\n\
\n\
AC_MSG_CHECKING([whether C compiler supports -fprofile-generate])\n\
AC_TRY_COMPILE([], [return 0;],\n\
               [ PROFILE_GEN_CFLAGS=\"-fprofile-generate\"\n\
                 result=\"yes\" ], result=\"no\")\n\
AC_MSG_RESULT([$result])\n\
\n\
if test $result = \"yes\"; then\n\
  PROFILE_USE_CFLAGS=\"-fprofile-use\"\n\
else\n\
  CFLAGS=\"$_SAVE_CFLAGS -fprofile-arcs\"\n\
  AC_MSG_CHECKING([whether C compiler supports -fprofile-arcs])\n\
  AC_TRY_COMPILE([], [return 0;],\n\
                 [ PROFILE_GEN_CFLAGS=\"-fprofile-arcs\"\n\
                   result=\"yes\" ], result=\"no\")\n\
  AC_MSG_RESULT([$result])\n\
  if test $result = \"yes\"; then\n\
    PROFILE_USE_CFLAGS=\"-fbranch-probabilities\"\n\
  fi\n\
fi\n\
\n\
CFLAGS=\"$_SAVE_CFLAGS\"\n\
\n\
AC_SUBST(PROFILE_GEN_CFLAGS)\n\
AC_SUBST(PROFILE_USE_CFLAGS)\n\
\n\
AC_LANG_CPLUSPLUS\n\
\n\
dnl ========================================================\n\
dnl Test for -pedantic bustage\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(pedantic,\n\
[  --disable-pedantic      Issue all warnings demanded by strict ANSI C ],\n\
_PEDANTIC= )\n\
if test \"$_PEDANTIC\"; then\n\
    _SAVE_CXXFLAGS=$CXXFLAGS\n\
    CXXFLAGS=\"$CXXFLAGS ${_WARNINGS_CXXFLAGS} ${_COMPILER_PREFIX}-pedantic\"\n\
    AC_MSG_CHECKING([whether C++ compiler has -pedantic long long bug])\n\
    AC_TRY_COMPILE([$configure_static_assert_macros],\n\
                   [CONFIGURE_STATIC_ASSERT(sizeof(long long) == 8)],\n\
                   result=\"no\", result=\"yes\" )\n\
    AC_MSG_RESULT([$result])\n\
    CXXFLAGS=\"$_SAVE_CXXFLAGS\"\n\
\n\
    case \"$result\" in\n\
    no)\n\
        _WARNINGS_CFLAGS=\"${_WARNINGS_CFLAGS} ${_COMPILER_PREFIX}-pedantic\"\n\
        _WARNINGS_CXXFLAGS=\"${_WARNINGS_CXXFLAGS} ${_COMPILER_PREFIX}-pedantic\"\n\
        ;;\n\
    yes)\n\
        AC_MSG_ERROR([Your compiler appears to have a known bug where long long is miscompiled when using -pedantic.  Reconfigure using --disable-pedantic. ])\n\
        ;;\n\
    esac\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Test for correct temporary object destruction order\n\
dnl ========================================================\n\
dnl We want to make sure the compiler follows the C++ spec here as \n\
dnl xpcom and the string classes depend on it (bug 235381).\n\
AC_MSG_CHECKING([for correct temporary object destruction order])\n\
AC_TRY_RUN([ class A {\n\
             public:  A(int& x) : mValue(x) {}\n\
                      ~A() { mValue--; }\n\
                      operator char**() { return 0; }\n\
             private:  int& mValue;\n\
             };\n\
             void func(char **arg) {}\n\
             int m=2;\n\
             void test() {\n\
                  func(A(m));\n\
                  if (m==1) m = 0;\n\
             }\n\
             int main() {\n\
                 test();\n\
                 return(m);\n\
             }\n\
             ],\n\
     result=\"yes\", result=\"no\", result=\"maybe\")\n\
AC_MSG_RESULT([$result])\n\
\n\
if test \"$result\" = \"no\"; then\n\
    AC_MSG_ERROR([Your compiler does not follow the C++ specification for temporary object destruction order.])\n\
fi\n\
\n\
dnl ========================================================\n\
dnl Autoconf test for gcc 2.7.2.x (and maybe others?) so that we don\'t\n\
dnl provide non-const forms of the operator== for comparing nsCOMPtrs to\n\
dnl raw pointers in nsCOMPtr.h.  (VC++ has the same bug.)\n\
dnl ========================================================\n\
_SAVE_CXXFLAGS=$CXXFLAGS\n\
CXXFLAGS=\"$CXXFLAGS ${_WARNINGS_CXXFLAGS}\"\n\
AC_CACHE_CHECK(for correct overload resolution with const and templates,\n\
    ac_nscap_nonconst_opeq_bug,\n\
    [AC_TRY_COMPILE([\n\
                      template <class T>\n\
                      class Pointer\n\
                        {\n\
                        public:\n\
                          T* myPtr;\n\
                        };\n\
                      \n\
                      template <class T, class U>\n\
                      int operator==(const Pointer<T>& rhs, U* lhs)\n\
                        {\n\
                          return rhs.myPtr == lhs;\n\
                        }\n\
                      \n\
                      template <class T, class U>\n\
                      int operator==(const Pointer<T>& rhs, const U* lhs)\n\
                        {\n\
                          return rhs.myPtr == lhs;\n\
                        }\n\
                    ],\n\
                    [\n\
                      Pointer<int> foo;\n\
                      const int* bar;\n\
                      return foo == bar;\n\
                    ],\n\
                    ac_nscap_nonconst_opeq_bug=\"no\",\n\
                    ac_nscap_nonconst_opeq_bug=\"yes\")])\n\
CXXFLAGS=\"$_SAVE_CXXFLAGS\"\n\
\n\
if test \"$ac_nscap_nonconst_opeq_bug\" = \"yes\" ; then\n\
    AC_DEFINE(NSCAP_DONT_PROVIDE_NONCONST_OPEQ)\n\
fi\n\
fi # SKIP_COMPILER_CHECKS\n\
\n\
dnl ========================================================\n\
dnl C++ rtti\n\
dnl Should be smarter and check that the compiler does indeed have rtti\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(cpp-rtti,\n\
[  --enable-cpp-rtti       Enable C++ RTTI ],\n\
[ _MOZ_USE_RTTI=1 ],\n\
[ _MOZ_USE_RTTI= ])\n\
\n\
if test \"$_MOZ_USE_RTTI\"; then\n\
   _MOZ_RTTI_FLAGS=$_MOZ_RTTI_FLAGS_ON\n\
else\n\
   _MOZ_RTTI_FLAGS=$_MOZ_RTTI_FLAGS_OFF\n\
fi\n\
\n\
AC_SUBST(_MOZ_RTTI_FLAGS_ON)\n\
\n\
dnl ========================================================\n\
dnl C++ exceptions (g++/egcs only - for now)\n\
dnl Should be smarter and check that the compiler does indeed have exceptions\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(cpp-exceptions,\n\
[  --enable-cpp-exceptions Enable C++ exceptions ],\n\
[ _MOZ_CPP_EXCEPTIONS=1 ],\n\
[ _MOZ_CPP_EXCEPTIONS= ])\n\
\n\
if test \"$_MOZ_CPP_EXCEPTIONS\"; then\n\
    _MOZ_EXCEPTIONS_FLAGS=$_MOZ_EXCEPTIONS_FLAGS_ON\n\
else\n\
    _MOZ_EXCEPTIONS_FLAGS=$_MOZ_EXCEPTIONS_FLAGS_OFF\n\
fi\n\
\n\
# Irix & OSF native compilers do not like exception declarations \n\
# when exceptions are disabled\n\
if test -n \"$MIPSPRO_CXX\" -o -n \"$COMPAQ_CXX\" -o -n \"$VACPP\"; then\n\
    AC_DEFINE(CPP_THROW_NEW, [])\n\
else\n\
    AC_DEFINE(CPP_THROW_NEW, [throw()])\n\
fi\n\
AC_LANG_C\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Build depencency options\n\
dnl =\n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Build dependencies)\n\
\n\
dnl ========================================================\n\
dnl = Do not auto generate dependency info\n\
dnl ========================================================\n\
MOZ_AUTO_DEPS=1\n\
MOZ_ARG_DISABLE_BOOL(auto-deps,\n\
[  --disable-auto-deps     Do not automatically generate dependency info],\n\
    MOZ_AUTO_DEPS=,\n\
    MOZ_AUTO_DEPS=1)\n\
\n\
if test -n \"$MOZ_AUTO_DEPS\"; then\n\
dnl ========================================================\n\
dnl = Use mkdepend instead of $CC -MD for dependency generation\n\
dnl ========================================================\n\
_cpp_md_flag=\n\
MOZ_ARG_DISABLE_BOOL(md,\n\
[  --disable-md            Do not use compiler-based dependencies ],\n\
  [_cpp_md_flag=],\n\
  [_cpp_md_flag=1],\n\
  [dnl Default is to turn on -MD if using GNU-compatible compilers\n\
   if test \"$GNU_CC\" -a \"$GNU_CXX\" -a \"$OS_ARCH\" != \"WINNT\" -a \"$OS_ARCH\" != \"WINCE\"; then\n\
     _cpp_md_flag=1\n\
   fi])\n\
if test \"$_cpp_md_flag\"; then\n\
  COMPILER_DEPEND=1\n\
  if test \"$OS_ARCH\" = \"OpenVMS\"; then\n\
    _DEPEND_CFLAGS=\'$(subst =, ,$(filter-out %/.pp,-MM=-MD=-MF=$(MDDEPDIR)/$(*F).pp))\'\n\
  else\n\
    _DEPEND_CFLAGS=\'$(filter-out %/.pp,-Wp,-MD,$(MDDEPDIR)/$(*F).pp)\'\n\
  fi\n\
else\n\
  COMPILER_DEPEND=\n\
  _USE_CPP_INCLUDE_FLAG=\n\
  _DEFINES_CFLAGS=\'$(ACDEFINES) -D_MOZILLA_CONFIG_H_ -DMOZILLA_CLIENT\'\n\
  _DEFINES_CXXFLAGS=\'$(ACDEFINES) -D_MOZILLA_CONFIG_H_ -DMOZILLA_CLIENT\'\n\
fi\n\
fi # MOZ_AUTO_DEPS\n\
MDDEPDIR=\'.deps\'\n\
AC_SUBST(MOZ_AUTO_DEPS)\n\
AC_SUBST(COMPILER_DEPEND)\n\
AC_SUBST(MDDEPDIR)\n\
\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Static Build Options\n\
dnl =\n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Static build options)\n\
\n\
MOZ_ARG_ENABLE_BOOL(static,\n\
[  --enable-static         Enable building of internal static libs],\n\
    BUILD_STATIC_LIBS=1,\n\
    BUILD_STATIC_LIBS=)\n\
\n\
MOZ_ARG_ENABLE_BOOL(libxul,\n\
[  --enable-libxul         Enable building of libxul],\n\
    MOZ_ENABLE_LIBXUL=1,\n\
    MOZ_ENABLE_LIBXUL=)\n\
\n\
if test -n \"$MOZ_ENABLE_LIBXUL\" -a -n \"$BUILD_STATIC_LIBS\"; then\n\
	AC_MSG_ERROR([--enable-libxul is not compatible with --enable-static])\n\
fi\n\
\n\
if test -n \"$MOZ_ENABLE_LIBXUL\" -a -z \"$MOZ_XUL_APP\"; then\n\
	AC_MSG_ERROR([--enable-libxul is only compatible with toolkit XUL applications.])\n\
fi\n\
\n\
if test -n \"$MOZ_ENABLE_LIBXUL\"; then\n\
    XPCOM_LIBS=\"$LIBXUL_LIBS\"\n\
    AC_DEFINE(MOZ_ENABLE_LIBXUL)\n\
else\n\
    if test -n \"$BUILD_STATIC_LIBS\"; then\n\
        AC_DEFINE(MOZ_STATIC_BUILD)\n\
    fi\n\
    XPCOM_LIBS=\"$DYNAMIC_XPCOM_LIBS\"\n\
fi\n\
\n\
dnl ========================================================\n\
dnl = Force JS to be a static lib\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(js-static-build,\n\
[  --enable-js-static-build  Force js to be a static lib],\n\
    JS_STATIC_BUILD=1,\n\
    JS_STATIC_BUILD= )\n\
\n\
AC_SUBST(JS_STATIC_BUILD)\n\
        \n\
if test -n \"$JS_STATIC_BUILD\"; then\n\
    AC_DEFINE(EXPORT_JS_API)\n\
\n\
if test -z \"$BUILD_STATIC_LIBS\"; then\n\
    AC_MSG_ERROR([--enable-js-static-build is only compatible with --enable-static])\n\
fi\n\
\n\
fi\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Standalone module options\n\
dnl = \n\
dnl ========================================================\n\
MOZ_ARG_HEADER(Standalone module options (Not for building Mozilla))\n\
\n\
dnl Check for GLib and libIDL.\n\
dnl ========================================================\n\
case \"$target_os\" in\n\
msvc*|mks*|cygwin*|mingw*|wince*)\n\
    SKIP_IDL_CHECK=\"yes\"\n\
    ;;\n\
*)\n\
    SKIP_IDL_CHECK=\"no\"\n\
    ;;\n\
esac\n\
\n\
if test -z \"$COMPILE_ENVIRONMENT\"; then\n\
    SKIP_IDL_CHECK=\"yes\"\n\
fi\n\
\n\
dnl = Allow users to disable libIDL checking for standalone modules\n\
MOZ_ARG_WITHOUT_BOOL(libIDL,\n\
[  --without-libIDL        Skip check for libIDL (standalone modules only)],\n\
	SKIP_IDL_CHECK=\"yes\")\n\
\n\
if test \"$SKIP_IDL_CHECK\" = \"no\"\n\
then\n\
    _LIBIDL_FOUND=\n\
    if test \"$MACOS_SDK_DIR\"; then \n\
      dnl xpidl, and therefore libIDL, is only needed on the build host.\n\
      dnl Don\'t build it against the SDK, as that causes problems.\n\
      _MACSAVE_CFLAGS=\"$CFLAGS\"\n\
      _MACSAVE_LIBS=\"$LIBS\"\n\
      _MACSAVE_LDFLAGS=\"$LDFLAGS\"\n\
      _MACSAVE_NEXT_ROOT=\"$NEXT_ROOT\"\n\
      changequote(,)\n\
      CFLAGS=`echo $CFLAGS|sed -E -e \"s%((-I|-isystem )${MACOS_SDK_DIR}/usr/(include|lib/gcc)[^ ]*)|-F${MACOS_SDK_DIR}(/System)?/Library/Frameworks[^ ]*|-nostdinc[^ ]*|-isysroot ${MACOS_SDK_DIR}%%g\"`\n\
      LIBS=`echo $LIBS|sed -e \"s?-L${MACOS_SDK_DIR}/usr/lib[^ ]*??g\"`\n\
      LDFLAGS=`echo $LDFLAGS|sed -e \"s?-Wl,-syslibroot,${MACOS_SDK_DIR}??g\"`\n\
      changequote([,])\n\
      unset NEXT_ROOT\n\
    fi\n\
\n\
    if test \"$MOZ_ENABLE_GTK2\"; then\n\
        PKG_CHECK_MODULES(LIBIDL, libIDL-2.0 >= 0.8.0 glib-2.0 gobject-2.0, _LIBIDL_FOUND=1,_LIBIDL_FOUND=)\n\
    fi\n\
    if test \"$MOZ_ENABLE_GTK\"; then\n\
        AM_PATH_LIBIDL($LIBIDL_VERSION,_LIBIDL_FOUND=1)\n\
    fi\n\
    dnl if no gtk/libIDL1 or gtk2/libIDL2 combination was found, fall back\n\
    dnl to either libIDL1 or libIDL2.\n\
    if test -z \"$_LIBIDL_FOUND\"; then\n\
        AM_PATH_LIBIDL($LIBIDL_VERSION,_LIBIDL_FOUND=1)\n\
        if test -z \"$_LIBIDL_FOUND\"; then\n\
            PKG_CHECK_MODULES(LIBIDL, libIDL-2.0 >= 0.8.0,_LIBIDL_FOUND=1)\n\
        fi\n\
    fi\n\
    dnl\n\
    dnl If we don\'t have a libIDL config program & not cross-compiling, \n\
    dnl     look for orbit-config instead.\n\
    dnl\n\
    if test -z \"$_LIBIDL_FOUND\" && test -z \"$CROSS_COMPILE\"; then\n\
        AC_PATH_PROGS(ORBIT_CONFIG, $ORBIT_CONFIG orbit-config)\n\
        if test -n \"$ORBIT_CONFIG\"; then\n\
            AC_MSG_CHECKING([for ORBit libIDL usability])\n\
        	_ORBIT_CFLAGS=`${ORBIT_CONFIG} client --cflags`\n\
    	    _ORBIT_LIBS=`${ORBIT_CONFIG} client --libs`\n\
            _ORBIT_INC_PATH=`${PERL} -e \'{ for $f (@ARGV) { print \"$f \" if ($f =~ m/^-I/); } }\' -- ${_ORBIT_CFLAGS}`\n\
            _ORBIT_LIB_PATH=`${PERL} -e \'{ for $f (@ARGV) { print \"$f \" if ($f =~ m/^-L/); } }\' -- ${_ORBIT_LIBS}`\n\
            LIBIDL_CFLAGS=\"$_ORBIT_INC_PATH\"\n\
            LIBIDL_LIBS=\"$_ORBIT_LIB_PATH -lIDL -lglib\"\n\
            LIBIDL_CONFIG=\n\
            _SAVE_CFLAGS=\"$CFLAGS\"\n\
            _SAVE_LIBS=\"$LIBS\"\n\
            CFLAGS=\"$LIBIDL_CFLAGS $CFLAGS\"\n\
            LIBS=\"$LIBIDL_LIBS $LIBS\"\n\
            AC_TRY_RUN([\n\
#include <stdlib.h>\n\
#include <libIDL/IDL.h>\n\
int main() {\n\
  char *s;\n\
  s=strdup(IDL_get_libver_string());\n\
  if(s==NULL) {\n\
    exit(1);\n\
  }\n\
  exit(0);\n\
}\n\
            ], [_LIBIDL_FOUND=1\n\
                result=\"yes\"],\n\
               [LIBIDL_CFLAGS=\n\
                LIBIDL_LIBS=\n\
                result=\"no\"],\n\
               [_LIBIDL_FOUND=1\n\
                result=\"maybe\"] )\n\
            AC_MSG_RESULT($result)\n\
            CFLAGS=\"$_SAVE_CFLAGS\"\n\
            LIBS=\"$_SAVE_LIBS\"\n\
        fi\n\
    fi\n\
    if test -z \"$_LIBIDL_FOUND\"; then\n\
        AC_MSG_ERROR([libIDL not found.\n\
        libIDL $LIBIDL_VERSION or higher is required.])\n\
    fi\n\
    if test \"$MACOS_SDK_DIR\"; then\n\
      CFLAGS=\"$_MACSAVE_CFLAGS\"\n\
      LIBS=\"$_MACSAVE_LIBS\"\n\
      LDFLAGS=\"$_MACSAVE_LDFLAGS\"\n\
      if test -n \"$_MACSAVE_NEXT_ROOT\" ; then\n\
        export NEXT_ROOT=\"$_MACSAVE_NEXT_ROOT\"\n\
      fi\n\
    fi\n\
fi\n\
\n\
if test -n \"$CROSS_COMPILE\"; then\n\
     if test -z \"$HOST_LIBIDL_CONFIG\"; then\n\
        HOST_LIBIDL_CONFIG=\"$LIBIDL_CONFIG\"\n\
    fi\n\
    if test -n \"$HOST_LIBIDL_CONFIG\" && test \"$HOST_LIBIDL_CONFIG\" != \"no\"; then\n\
        HOST_LIBIDL_CFLAGS=`${HOST_LIBIDL_CONFIG} --cflags`\n\
        HOST_LIBIDL_LIBS=`${HOST_LIBIDL_CONFIG} --libs`\n\
    else\n\
        HOST_LIBIDL_CFLAGS=\"$LIBIDL_CFLAGS\"\n\
        HOST_LIBIDL_LIBS=\"$LIBIDL_LIBS\"\n\
    fi\n\
fi\n\
\n\
if test -z \"$SKIP_PATH_CHECKS\"; then\n\
if test -z \"${GLIB_CFLAGS}\" || test -z \"${GLIB_LIBS}\" ; then\n\
    if test \"$MOZ_ENABLE_GTK2\"; then\n\
        PKG_CHECK_MODULES(GLIB, glib-2.0 >= 1.3.7 gobject-2.0)\n\
    else\n\
        AM_PATH_GLIB(${GLIB_VERSION})\n\
    fi\n\
fi\n\
fi\n\
\n\
if test -z \"${GLIB_GMODULE_LIBS}\" -a -n \"${GLIB_CONFIG}\"; then\n\
    GLIB_GMODULE_LIBS=`$GLIB_CONFIG gmodule --libs`\n\
fi\n\
\n\
AC_SUBST(LIBIDL_CFLAGS)\n\
AC_SUBST(LIBIDL_LIBS)\n\
AC_SUBST(STATIC_LIBIDL)\n\
AC_SUBST(GLIB_CFLAGS)\n\
AC_SUBST(GLIB_LIBS)\n\
AC_SUBST(GLIB_GMODULE_LIBS)\n\
AC_SUBST(HOST_LIBIDL_CONFIG)\n\
AC_SUBST(HOST_LIBIDL_CFLAGS)\n\
AC_SUBST(HOST_LIBIDL_LIBS)\n\
\n\
dnl ========================================================\n\
dnl Check for libart\n\
dnl ========================================================\n\
if test \"$MOZ_SVG_RENDERER_LIBART\"; then\n\
  if test ! -f $topsrcdir/other-licenses/libart_lgpl/Makefile.in; then\n\
    AC_MSG_ERROR([You must check out the mozilla version of libart. Use\n\
mk_add_options MOZ_CO_MODULE=mozilla/other-licenses/libart_lgpl])\n\
  fi\n\
\n\
  dnl libart\'s configure hasn\'t been run yet, but we know what the\n\
  dnl answer should be anyway\n\
  MOZ_LIBART_CFLAGS=\'-I${DIST}/include/libart_lgpl\'\n\
  case \"$target_os\" in\n\
  msvc*|mks*|cygwin*|mingw*|wince*)\n\
      MOZ_LIBART_LIBS=\'$(LIBXUL_DIST)/lib/$(LIB_PREFIX)moz_art_lgpl.$(IMPORT_LIB_SUFFIX)\' \n\
      ;;\n\
  os2*)\n\
      MOZ_LIBART_LIBS=\'-lmoz_art -lm\'\n\
      ;;\n\
  beos*)\n\
      MOZ_LIBART_LIBS=\'-lmoz_art_lgpl -lroot -lbe\'\n\
      ;;\n\
  *)\n\
      MOZ_LIBART_LIBS=\'-lmoz_art_lgpl -lm\'\n\
      ;;\n\
  esac\n\
  AC_FUNC_ALLOCA\n\
fi\n\
\n\
AC_SUBST(MOZ_LIBART_CFLAGS)\n\
AC_SUBST(MOZ_LIBART_LIBS)\n\
\n\
dnl ========================================================\n\
dnl Check for cairo\n\
dnl ========================================================\n\
if test \"$MOZ_SVG\" -o \"$MOZ_ENABLE_CANVAS\" -o \"$MOZ_ENABLE_CAIRO_GFX\" ; then\n\
   MOZ_CAIRO_CFLAGS=\'-I$(LIBXUL_DIST)/include/cairo\'\n\
\n\
   MOZ_TREE_CAIRO=1\n\
   MOZ_ARG_ENABLE_BOOL(system-cairo,\n\
   [ --enable-system-cairo Use system cairo (located with pkgconfig)],\n\
   MOZ_TREE_CAIRO=,\n\
   MOZ_TREE_CAIRO=1 )\n\
\n\
   if test \"$MOZ_TREE_CAIRO\"; then\n\
       # Check for headers defining standard int types.\n\
       AC_CHECK_HEADERS(stdint.h inttypes.h sys/int_types.h)\n\
\n\
       # For now we assume that we will have a uint64_t available through\n\
       # one of the above headers or mozstdint.h.\n\
       AC_DEFINE(HAVE_UINT64_T)\n\
\n\
       # Define macros for cairo-features.h\n\
       if test \"$MOZ_X11\"; then\n\
           XLIB_SURFACE_FEATURE=\"#define CAIRO_HAS_XLIB_SURFACE 1\"\n\
           PS_SURFACE_FEATURE=\"#define CAIRO_HAS_PS_SURFACE 1\"\n\
           PDF_SURFACE_FEATURE=\"#define CAIRO_HAS_PDF_SURFACE 1\"\n\
           FT_FONT_FEATURE=\"#define CAIRO_HAS_FT_FONT 1\"\n\
           MOZ_ENABLE_CAIRO_FT=1\n\
           CAIRO_FT_CFLAGS=\"$FT2_CFLAGS\"\n\
       fi\n\
       if test \"$MOZ_WIDGET_TOOLKIT\" = \"mac\" -o \"$MOZ_WIDGET_TOOLKIT\" = \"cocoa\"; then\n\
           QUARTZ_SURFACE_FEATURE=\"#define CAIRO_HAS_QUARTZ_SURFACE 1\"\n\
           NQUARTZ_SURFACE_FEATURE=\"#define CAIRO_HAS_NQUARTZ_SURFACE 1\"\n\
           ATSUI_FONT_FEATURE=\"#define CAIRO_HAS_ATSUI_FONT 1\"\n\
       fi\n\
       if test \"$MOZ_WIDGET_TOOLKIT\" = \"windows\"; then\n\
           WIN32_SURFACE_FEATURE=\"#define CAIRO_HAS_WIN32_SURFACE 1\"\n\
           WIN32_FONT_FEATURE=\"#define CAIRO_HAS_WIN32_FONT 1\"\n\
           PDF_SURFACE_FEATURE=\"#define CAIRO_HAS_PDF_SURFACE 1\"\n\
       fi\n\
       if test \"$MOZ_WIDGET_TOOLKIT\" = \"os2\"; then\n\
           OS2_SURFACE_FEATURE=\"#define CAIRO_HAS_OS2_SURFACE 1\"\n\
           FT_FONT_FEATURE=\"#define CAIRO_HAS_FT_FONT 1\"\n\
           MOZ_ENABLE_CAIRO_FT=1\n\
           CAIRO_FT_CFLAGS=\"-I${MZFTCFGFT2}/include\"\n\
           CAIRO_FT_LIBS=\"-L${MZFTCFGFT2}/lib -lmozft -lmzfntcfg\"\n\
       fi\n\
       if test \"$MOZ_ENABLE_GLITZ\"; then\n\
           GLITZ_SURFACE_FEATURE=\"#define CAIRO_HAS_GLITZ_SURFACE 1\"\n\
       fi\n\
       if test \"$MOZ_WIDGET_TOOLKIT\" = \"beos\"; then\n\
           PKG_CHECK_MODULES(CAIRO_FT, fontconfig freetype2)\n\
           BEOS_SURFACE_FEATURE=\"#define CAIRO_HAS_BEOS_SURFACE 1\"\n\
           FT_FONT_FEATURE=\"#define CAIRO_HAS_FT_FONT 1\"\n\
           MOZ_ENABLE_CAIRO_FT=1\n\
       fi\n\
       AC_SUBST(MOZ_ENABLE_CAIRO_FT)\n\
       AC_SUBST(CAIRO_FT_CFLAGS)\n\
\n\
       if test \"$MOZ_DEBUG\"; then\n\
         SANITY_CHECKING_FEATURE=\"#define CAIRO_DO_SANITY_CHECKING 1\"\n\
       else\n\
         SANITY_CHECKING_FEATURE=\"#undef CAIRO_DO_SANITY_CHECKING\"\n\
       fi\n\
\n\
       PNG_FUNCTIONS_FEATURE=\"#define CAIRO_HAS_PNG_FUNCTIONS 1\"\n\
\n\
       AC_SUBST(PS_SURFACE_FEATURE)\n\
       AC_SUBST(PDF_SURFACE_FEATURE)\n\
       AC_SUBST(SVG_SURFACE_FEATURE)\n\
       AC_SUBST(XLIB_SURFACE_FEATURE)\n\
       AC_SUBST(QUARTZ_SURFACE_FEATURE)\n\
       AC_SUBST(NQUARTZ_SURFACE_FEATURE)\n\
       AC_SUBST(XCB_SURFACE_FEATURE)\n\
       AC_SUBST(WIN32_SURFACE_FEATURE)\n\
       AC_SUBST(OS2_SURFACE_FEATURE)\n\
       AC_SUBST(BEOS_SURFACE_FEATURE)\n\
       AC_SUBST(GLITZ_SURFACE_FEATURE)\n\
       AC_SUBST(DIRECTFB_SURFACE_FEATURE)\n\
       AC_SUBST(FT_FONT_FEATURE)\n\
       AC_SUBST(WIN32_FONT_FEATURE)\n\
       AC_SUBST(ATSUI_FONT_FEATURE)\n\
       AC_SUBST(PNG_FUNCTIONS_FEATURE)\n\
\n\
       if test \"$_WIN32_MSVC\"; then\n\
           MOZ_CAIRO_LIBS=\'$(DEPTH)/gfx/cairo/cairo/src/mozcairo.lib $(DEPTH)/gfx/cairo/libpixman/src/mozlibpixman.lib\'\n\
           if test \"$MOZ_ENABLE_GLITZ\"; then\n\
               MOZ_CAIRO_LIBS=\"$MOZ_CAIRO_LIBS \"\'$(DEPTH)/gfx/cairo/glitz/src/mozglitz.lib $(DEPTH)/gfx/cairo/glitz/src/wgl/mozglitzwgl.lib\'\n\
           fi\n\
       else\n\
           MOZ_CAIRO_LIBS=\'-L$(DEPTH)/gfx/cairo/cairo/src -lmozcairo -L$(DEPTH)/gfx/cairo/libpixman/src -lmozlibpixman\'\" $CAIRO_FT_LIBS\"\n\
\n\
           if test \"$MOZ_X11\"; then\n\
               MOZ_CAIRO_LIBS=\"$MOZ_CAIRO_LIBS $XLDFLAGS -lXrender -lfreetype -lfontconfig\"\n\
           fi\n\
\n\
           if test \"$MOZ_ENABLE_GLITZ\"; then\n\
               MOZ_CAIRO_LIBS=\"$MOZ_CAIRO_LIBS \"\'-L$(DEPTH)/gfx/cairo/glitz/src -lmozglitz\'\n\
               if test \"$MOZ_X11\"; then\n\
                   MOZ_CAIRO_LIBS=\"$MOZ_CAIRO_LIBS \"\'-L$(DEPTH)/gfx/cairo/glitz/src/glx -lmozglitzglx -lGL\'\n\
               fi\n\
               if test \"$MOZ_WIDGET_TOOLKIT\" = \"windows\"; then\n\
                   MOZ_CAIRO_LIBS=\"$MOZ_CAIRO_LIBS \"\'-L$(DEPTH)/gfx/cairo/glitz/src/wgl -lmozglitzwgl\'\n\
               fi\n\
           fi\n\
       fi\n\
   else\n\
      PKG_CHECK_MODULES(CAIRO, cairo >= $CAIRO_VERSION freetype2 fontconfig)\n\
      MOZ_CAIRO_CFLAGS=$CAIRO_CFLAGS\n\
      MOZ_CAIRO_LIBS=$CAIRO_LIBS\n\
   fi\n\
fi\n\
\n\
AC_SUBST(MOZ_TREE_CAIRO)\n\
AC_SUBST(MOZ_CAIRO_CFLAGS)\n\
AC_SUBST(MOZ_CAIRO_LIBS)\n\
\n\
dnl ========================================================\n\
dnl disable xul\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(xul,\n\
[  --disable-xul           Disable XUL],\n\
    MOZ_XUL= )\n\
if test \"$MOZ_XUL\"; then\n\
  AC_DEFINE(MOZ_XUL)\n\
else\n\
  dnl remove extensions that require XUL\n\
  MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS | sed -e \'s/inspector//\' -e \'s/venkman//\' -e \'s/irc//\' -e \'s/tasks//\'`\n\
fi\n\
\n\
AC_SUBST(MOZ_XUL)\n\
\n\
dnl ========================================================\n\
dnl Two ways to enable Python support:\n\
dnl   --enable-extensions=python # select all available.\n\
dnl    (MOZ_PYTHON_EXTENSIONS contains the list of extensions)\n\
dnl or:\n\
dnl   --enable-extensions=python/xpcom,... # select individual ones\n\
dnl\n\
dnl If either is used, we locate the Python to use.\n\
dnl ========================================================\n\
dnl\n\
dnl If \'python\' appears anywhere in the extensions list, go lookin\'...\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c python` -ne 0; then\n\
    dnl Allow PYTHON to point to the Python interpreter to use\n\
    dnl (note that it must be the python executable - which we\n\
    dnl run to locate the relevant paths etc)\n\
    dnl If not set, we use whatever Python we can find.\n\
    MOZ_PYTHON=$PYTHON\n\
    dnl Ask Python what its version number is\n\
    changequote(,)\n\
    MOZ_PYTHON_VER=`$PYTHON -c \"import sys;print \'%d%d\' % sys.version_info[0:2]\"`\n\
    MOZ_PYTHON_VER_DOTTED=`$PYTHON -c \"import sys;print \'%d.%d\' % sys.version_info[0:2]\"`\n\
    changequote([,])\n\
    dnl Ask for the Python \"prefix\" (ie, home/source dir)\n\
    MOZ_PYTHON_PREFIX=`$PYTHON -c \"import sys; print sys.prefix\"`\n\
    dnl Setup the include and library directories.\n\
    if test \"$OS_ARCH\" = \"WINNT\"; then\n\
        MOZ_PYTHON_PREFIX=`$CYGPATH_W $MOZ_PYTHON_PREFIX | $CYGPATH_S`\n\
        dnl Source trees have \"include\" and \"PC\" for .h, and \"PCbuild\" for .lib\n\
        dnl Binary trees have \"include\" for .h, and \"libs\" for .lib\n\
        dnl We add \'em both - along with quotes, to handle spaces.\n\
        MOZ_PYTHON_DLL_SUFFIX=.pyd\n\
        MOZ_PYTHON_INCLUDES=\"\\\"-I$MOZ_PYTHON_PREFIX/include\\\" \\\"-I$MOZ_PYTHON_PREFIX/PC\\\"\"\n\
        MOZ_PYTHON_LIBS=\"\\\"/libpath:$MOZ_PYTHON_PREFIX/PCBuild\\\" \\\"/libpath:$MOZ_PYTHON_PREFIX/libs\\\"\"\n\
    else\n\
        dnl Non-Windows include and libs\n\
        MOZ_PYTHON_DLL_SUFFIX=$DLL_SUFFIX\n\
        MOZ_PYTHON_INCLUDES=\"-I$MOZ_PYTHON_PREFIX/include/python$MOZ_PYTHON_VER_DOTTED\"\n\
        dnl Check for dynamic Python lib\n\
        dnl - A static Python is no good - multiple dynamic libraries (xpcom\n\
        dnl - core, xpcom loader, pydom etc) all need to share Python.\n\
        dnl - Python 2.3\'s std --enable-shared configure option will\n\
        dnl   create a libpython2.3.so.1.0. We should first try this\n\
        dnl   dotted versioned .so file because this is the one that\n\
        dnl   the PyXPCOM build mechanics tries to link to.\n\
        dnl   XXX Should find a better way than hardcoding \"1.0\".\n\
        dnl - Python developement tree dir layouts are NOT allowed here\n\
        dnl   because the PyXPCOM build just dies on it later anyway.\n\
        dnl - Fixes to the above by Python/*nix knowledgable people welcome!\n\
        if test -f \"$MOZ_PYTHON_PREFIX/lib/libpython$MOZ_PYTHON_VER_DOTTED.so.1.0\"; then\n\
            MOZ_PYTHON_LIBS=\"-L$MOZ_PYTHON_PREFIX/lib -lpython$MOZ_PYTHON_VER_DOTTED\"\n\
        elif test -f \"$MOZ_PYTHON_PREFIX/lib/libpython$MOZ_PYTHON_VER_DOTTED.so\"; then\n\
            MOZ_PYTHON_LIBS=\"-L$MOZ_PYTHON_PREFIX/lib -lpython$MOZ_PYTHON_VER_DOTTED\"\n\
        elif test -f \"$MOZ_PYTHON_PREFIX/libpython$MOZ_PYTHON_VER_DOTTED.so\"; then\n\
            dnl Don\'t Python development tree directory layout.\n\
            MOZ_PYTHON_LIBS=\"-L$MOZ_PYTHON_PREFIX -lpython$MOZ_PYTHON_VER_DOTTED\"\n\
            AC_MSG_ERROR([The Python at $MOZ_PYTHON_PREFIX looks like a dev tree. The PyXPCOM build cannot handle this yet. You must \'make install\' Python and use the installed tree.])\n\
        elif test \"$OS_ARCH\" = \"Darwin\"; then\n\
            dnl We do Darwin last, so if a custom non-framework build of\n\
            dnl python is used on OSX, then it will be picked up first by\n\
            dnl the logic above.\n\
            MOZ_PYTHON_LIBS=\"-framework Python\"\n\
        else\n\
            AC_MSG_ERROR([Could not find build shared libraries for Python at $MOZ_PYTHON_PREFIX.  This is required for PyXPCOM.])\n\
        fi\n\
        if test \"$OS_ARCH\" = \"Linux\"; then\n\
            MOZ_PYTHON_LIBS=\"$MOZ_PYTHON_LIBS -lutil\"\n\
        fi\n\
    fi\n\
    dnl Handle \"_d\" on Windows\n\
    if test \"$OS_ARCH\" = \"WINNT\" && test -n \"$MOZ_DEBUG\"; then\n\
        MOZ_PYTHON_DEBUG_SUFFIX=\"_d\"\n\
    else\n\
        MOZ_PYTHON_DEBUG_SUFFIX=\n\
    fi\n\
    AC_MSG_RESULT(Building Python extensions using python-$MOZ_PYTHON_VER_DOTTED from $MOZ_PYTHON_PREFIX)\n\
fi\n\
\n\
dnl If the user asks for the \'python\' extension, then we add\n\
dnl MOZ_PYTHON_EXTENSIONS to MOZ_EXTENSIONS - but with the leading \'python/\'\n\
dnl Note the careful regex - it must match \'python\' followed by anything\n\
dnl other than a \'/\', including the end-of-string.\n\
if test `echo \"$MOZ_EXTENSIONS\" | grep -c \'python\\([[^/]]\\|$\\)\'` -ne 0; then\n\
    for pyext in $MOZ_PYTHON_EXTENSIONS; do\n\
        MOZ_EXTENSIONS=`echo $MOZ_EXTENSIONS python/$pyext`\n\
    done\n\
fi\n\
dnl Later we may allow MOZ_PYTHON_EXTENSIONS to be specified on the\n\
dnl command-line, but not yet\n\
AC_SUBST(MOZ_PYTHON_EXTENSIONS)\n\
AC_SUBST(MOZ_PYTHON)\n\
AC_SUBST(MOZ_PYTHON_PREFIX)\n\
AC_SUBST(MOZ_PYTHON_INCLUDES)\n\
AC_SUBST(MOZ_PYTHON_LIBS)\n\
AC_SUBST(MOZ_PYTHON_VER)\n\
AC_SUBST(MOZ_PYTHON_VER_DOTTED)\n\
AC_SUBST(MOZ_PYTHON_DEBUG_SUFFIX)\n\
AC_SUBST(MOZ_PYTHON_DLL_SUFFIX)\n\
\n\
dnl ========================================================\n\
dnl disable profile sharing\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(profilesharing,\n\
[  --disable-profilesharing           Disable profile sharing],\n\
    MOZ_PROFILESHARING=,\n\
    MOZ_PROFILESHARING=1 )\n\
if test \"$MOZ_PROFILESHARING\"; then\n\
  MOZ_IPCD=1\n\
  AC_DEFINE(MOZ_PROFILESHARING)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl enable ipc/ipcd\n\
dnl ========================================================\n\
MOZ_ARG_ENABLE_BOOL(ipcd,\n\
[  --enable-ipcd                      Enable IPC daemon],\n\
    MOZ_IPCD=1,\n\
    MOZ_IPCD= )\n\
\n\
dnl ========================================================\n\
dnl disable profile locking\n\
dnl   do no use this in applications that can have more than\n\
dnl   one process accessing the profile directory.\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(profilelocking,\n\
[  --disable-profilelocking           Disable profile locking],\n\
    MOZ_PROFILELOCKING=,\n\
    MOZ_PROFILELOCKING=1 )\n\
if test \"$MOZ_PROFILELOCKING\"; then\n\
  AC_DEFINE(MOZ_PROFILELOCKING)\n\
fi\n\
\n\
dnl ========================================================\n\
dnl disable rdf services\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(rdf,\n\
[  --disable-rdf           Disable RDF],\n\
    MOZ_RDF= )\n\
if test \"$MOZ_RDF\"; then\n\
  AC_DEFINE(MOZ_RDF)\n\
fi\n\
\n\
AC_SUBST(MOZ_RDF)\n\
\n\
dnl ========================================================\n\
dnl necko configuration options\n\
dnl ========================================================\n\
\n\
dnl\n\
dnl option to disable various necko protocols\n\
dnl\n\
MOZ_ARG_ENABLE_STRING(necko-protocols,\n\
[  --enable-necko-protocols[={http,ftp,default,all,none}]\n\
                          Enable/disable specific protocol handlers],\n\
[ for option in `echo $enableval | sed \'s/,/ /g\'`; do\n\
    if test \"$option\" = \"yes\" || test \"$option\" = \"all\"; then\n\
        NECKO_PROTOCOLS=\"$NECKO_PROTOCOLS $NECKO_PROTOCOLS_DEFAULT\"\n\
    elif test \"$option\" = \"no\" || test \"$option\" = \"none\"; then\n\
        NECKO_PROTOCOLS=\"\"\n\
    elif test \"$option\" = \"default\"; then\n\
        NECKO_PROTOCOLS=\"$NECKO_PROTOCOLS $NECKO_PROTOCOLS_DEFAULT\"\n\
    elif test `echo \"$option\" | grep -c \\^-` != 0; then\n\
        option=`echo $option | sed \'s/^-//\'`\n\
        NECKO_PROTOCOLS=`echo \"$NECKO_PROTOCOLS\" | sed \"s/ ${option}//\"`\n\
    else\n\
        NECKO_PROTOCOLS=\"$NECKO_PROTOCOLS $option\"\n\
    fi\n\
done],\n\
    NECKO_PROTOCOLS=\"$NECKO_PROTOCOLS_DEFAULT\")\n\
dnl Remove dupes\n\
NECKO_PROTOCOLS=`${PERL} ${srcdir}/build/unix/uniq.pl ${NECKO_PROTOCOLS}`\n\
AC_SUBST(NECKO_PROTOCOLS)\n\
for p in $NECKO_PROTOCOLS; do\n\
    AC_DEFINE_UNQUOTED(NECKO_PROTOCOL_$p)\n\
done\n\
\n\
dnl\n\
dnl option to disable necko\'s disk cache\n\
dnl\n\
MOZ_ARG_DISABLE_BOOL(necko-disk-cache,\n\
[  --disable-necko-disk-cache\n\
                          Disable necko disk cache],\n\
    NECKO_DISK_CACHE=,\n\
    NECKO_DISK_CACHE=1)\n\
AC_SUBST(NECKO_DISK_CACHE)\n\
if test \"$NECKO_DISK_CACHE\"; then\n\
    AC_DEFINE(NECKO_DISK_CACHE)\n\
fi\n\
\n\
dnl\n\
dnl option to minimize size of necko\'s i/o buffers\n\
dnl\n\
MOZ_ARG_ENABLE_BOOL(necko-small-buffers,\n\
[  --enable-necko-small-buffers\n\
                          Minimize size of necko\'s i/o buffers],\n\
    NECKO_SMALL_BUFFERS=1,\n\
    NECKO_SMALL_BUFFERS=)\n\
AC_SUBST(NECKO_SMALL_BUFFERS)\n\
if test \"$NECKO_SMALL_BUFFERS\"; then\n\
    AC_DEFINE(NECKO_SMALL_BUFFERS)\n\
fi \n\
\n\
dnl\n\
dnl option to disable cookies\n\
dnl\n\
MOZ_ARG_DISABLE_BOOL(cookies,\n\
[  --disable-cookies       Disable cookie support],\n\
    NECKO_COOKIES=,\n\
    NECKO_COOKIES=1)\n\
AC_SUBST(NECKO_COOKIES)\n\
if test \"$NECKO_COOKIES\"; then\n\
    AC_DEFINE(NECKO_COOKIES)\n\
fi\n\
\n\
dnl NECKO_ configuration options are not global\n\
_NON_GLOBAL_ACDEFINES=\"$_NON_GLOBAL_ACDEFINES NECKO_\"\n\
\n\
dnl ========================================================\n\
dnl string api compatibility\n\
dnl ========================================================\n\
MOZ_ARG_DISABLE_BOOL(v1-string-abi,\n\
[  --disable-v1-string-abi   Disable binary compatibility layer for strings],\n\
    MOZ_V1_STRING_ABI=,\n\
    MOZ_V1_STRING_ABI=1)\n\
AC_SUBST(MOZ_V1_STRING_ABI)\n\
if test \"$MOZ_V1_STRING_ABI\"; then\n\
    AC_DEFINE(MOZ_V1_STRING_ABI)\n\
fi\n\
\n\
dnl Only build Mork if it\'s required\n\
AC_SUBST(MOZ_MORK)\n\
if test \"$MOZ_MORK\"; then\n\
  AC_DEFINE(MOZ_MORK)\n\
fi\n\
\n\
dnl Build the lightweight Mork reader if required\n\
AC_SUBST(MOZ_MORKREADER)\n\
if test \"$MOZ_MORKREADER\"; then\n\
  AC_DEFINE(MOZ_MORKREADER)\n\
fi\n\
\n\
dnl ========================================================\n\
if test \"$MOZ_DEBUG\" || test \"$NS_TRACE_MALLOC\"; then\n\
    MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS=\n\
fi\n\
\n\
if test \"$MOZ_LDAP_XPCOM\"; then\n\
    LDAP_CFLAGS=\'-I${DIST}/public/ldap\'\n\
    if test \"$OS_ARCH\" = \"WINNT\"; then\n\
        if test -n \"$GNU_CC\"; then\n\
            LDAP_LIBS=\'-L$(DIST)/lib -lnsldap32v50 -lnsldappr32v50\'\n\
        else\n\
            LDAP_LIBS=\'$(DIST)/lib/$(LIB_PREFIX)nsldap32v50.${IMPORT_LIB_SUFFIX} $(DIST)/lib/$(LIB_PREFIX)nsldappr32v50.${IMPORT_LIB_SUFFIX}\'\n\
        fi\n\
    elif test \"$VACPP\"; then\n\
            LDAP_LIBS=\'$(DIST)/lib/$(LIB_PREFIX)ldap50.${IMPORT_LIB_SUFFIX} $(DIST)/lib/$(LIB_PREFIX)prldap50.${IMPORT_LIB_SUFFIX}\'\n\
    else\n\
        LDAP_LIBS=\'-L${DIST}/bin -L${DIST}/lib -lldap50 -llber50 -lprldap50\'\n\
    fi\n\
fi\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test \"$SUNCTL\"; then\n\
    dnl older versions of glib do not seem to have gmodule which ctl needs\n\
    _SAVE_CFLAGS=$CFLAGS\n\
    CFLAGS=\"$CFLAGS $GLIB_CFLAGS\"\n\
    AC_LANG_SAVE\n\
    AC_LANG_C\n\
    AC_TRY_COMPILE([#include <gmodule.h>],\n\
        [ int x = 1; x++; ],,\n\
        [AC_MSG_ERROR([Cannot build ctl without gmodule support in glib.])])\n\
    AC_LANG_RESTORE\n\
    CFLAGS=$_SAVE_CFLAGS\n\
    AC_DEFINE(SUNCTL)\n\
fi\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl ========================================================\n\
dnl =\n\
dnl = Maintainer debug option (no --enable equivalent)\n\
dnl =\n\
dnl ========================================================\n\
\n\
AC_SUBST(AR)\n\
AC_SUBST(AR_FLAGS)\n\
AC_SUBST(AR_LIST)\n\
AC_SUBST(AR_EXTRACT)\n\
AC_SUBST(AR_DELETE)\n\
AC_SUBST(AS)\n\
AC_SUBST(ASFLAGS)\n\
AC_SUBST(AS_DASH_C_FLAG)\n\
AC_SUBST(LD)\n\
AC_SUBST(RC)\n\
AC_SUBST(RCFLAGS)\n\
AC_SUBST(WINDRES)\n\
AC_SUBST(USE_SHORT_LIBNAME)\n\
AC_SUBST(IMPLIB)\n\
AC_SUBST(FILTER)\n\
AC_SUBST(BIN_FLAGS)\n\
AC_SUBST(NS_USE_NATIVE)\n\
AC_SUBST(MOZ_WIDGET_TOOLKIT)\n\
AC_SUBST(MOZ_GFX_TOOLKIT)\n\
AC_SUBST(MOZ_UPDATE_XTERM)\n\
AC_SUBST(MINIMO)\n\
AC_SUBST(MOZ_AUTH_EXTENSION)\n\
AC_SUBST(MOZ_MATHML)\n\
AC_SUBST(MOZ_PERMISSIONS)\n\
AC_SUBST(MOZ_XTF)\n\
AC_SUBST(MOZ_XMLEXTRAS)\n\
AC_SUBST(MOZ_NO_INSPECTOR_APIS)\n\
AC_SUBST(MOZ_WEBSERVICES)\n\
AC_SUBST(MOZ_PREF_EXTENSIONS)\n\
AC_SUBST(MOZ_SVG)\n\
AC_SUBST(MOZ_SVG_FOREIGNOBJECT)\n\
AC_SUBST(MOZ_XSLT_STANDALONE)\n\
AC_SUBST(MOZ_JS_LIBS)\n\
AC_SUBST(MOZ_PSM)\n\
AC_SUBST(MOZ_DEBUG)\n\
AC_SUBST(MOZ_DEBUG_MODULES)\n\
AC_SUBST(MOZ_PROFILE_MODULES)\n\
AC_SUBST(MOZ_DEBUG_ENABLE_DEFS)\n\
AC_SUBST(MOZ_DEBUG_DISABLE_DEFS)\n\
AC_SUBST(MOZ_DEBUG_FLAGS)\n\
AC_SUBST(MOZ_DEBUG_LDFLAGS)\n\
AC_SUBST(MOZ_DBGRINFO_MODULES)\n\
AC_SUBST(MOZ_EXTENSIONS)\n\
AC_SUBST(MOZ_IMG_DECODERS)\n\
AC_SUBST(MOZ_IMG_ENCODERS)\n\
AC_SUBST(MOZ_JSDEBUGGER)\n\
AC_SUBST(MOZ_OJI)\n\
AC_SUBST(MOZ_NO_XPCOM_OBSOLETE)\n\
AC_SUBST(MOZ_PLUGINS)\n\
AC_SUBST(ENABLE_EAZEL_PROFILER)\n\
AC_SUBST(EAZEL_PROFILER_CFLAGS)\n\
AC_SUBST(EAZEL_PROFILER_LIBS)\n\
AC_SUBST(MOZ_PERF_METRICS)\n\
AC_SUBST(GC_LEAK_DETECTOR)\n\
AC_SUBST(MOZ_LOG_REFCNT)\n\
AC_SUBST(MOZ_LEAKY)\n\
AC_SUBST(MOZ_JPROF)\n\
AC_SUBST(MOZ_XPCTOOLS)\n\
AC_SUBST(MOZ_JSLOADER)\n\
AC_SUBST(MOZ_USE_NATIVE_UCONV)\n\
AC_SUBST(MOZ_INSURE)\n\
AC_SUBST(MOZ_INSURE_DIRS)\n\
AC_SUBST(MOZ_INSURE_EXCLUDE_DIRS)\n\
AC_SUBST(MOZ_QUANTIFY)\n\
AC_SUBST(MOZ_INSURIFYING)\n\
AC_SUBST(MOZ_LDAP_XPCOM)\n\
AC_SUBST(MOZ_LDAP_XPCOM_EXPERIMENTAL)\n\
AC_SUBST(LDAP_CFLAGS)\n\
AC_SUBST(LDAP_LIBS)\n\
AC_SUBST(LIBICONV)\n\
AC_SUBST(MOZ_PLACES)\n\
AC_SUBST(MOZ_PLACES_BOOKMARKS)\n\
AC_SUBST(MOZ_STORAGE)\n\
AC_SUBST(MOZ_FEEDS)\n\
AC_SUBST(NS_PRINTING)\n\
\n\
AC_SUBST(MOZ_JAVAXPCOM)\n\
AC_SUBST(JAVA_INCLUDE_PATH)\n\
AC_SUBST(JAVA)\n\
AC_SUBST(JAVAC)\n\
AC_SUBST(JAR)\n\
\n\
AC_SUBST(MOZ_PROFILESHARING)\n\
AC_SUBST(MOZ_PROFILELOCKING)\n\
\n\
AC_SUBST(MOZ_IPCD)\n\
\n\
AC_SUBST(HAVE_XIE)\n\
AC_SUBST(MOZ_XIE_LIBS)\n\
AC_SUBST(MOZ_XPRINT_CFLAGS)\n\
AC_SUBST(MOZ_XPRINT_LDFLAGS)\n\
AC_SUBST(MOZ_ENABLE_XPRINT)\n\
AC_SUBST(MOZ_ENABLE_POSTSCRIPT)\n\
AC_SUBST(MOZ_XINERAMA_LIBS)\n\
AC_SUBST(MOZ_ENABLE_XINERAMA)\n\
\n\
AC_SUBST(XPCOM_USE_LEA)\n\
AC_SUBST(BUILD_STATIC_LIBS)\n\
AC_SUBST(MOZ_ENABLE_LIBXUL)\n\
AC_SUBST(ENABLE_TESTS)\n\
AC_SUBST(IBMBIDI)\n\
AC_SUBST(SUNCTL)\n\
AC_SUBST(MOZ_UNIVERSALCHARDET)\n\
AC_SUBST(ACCESSIBILITY)\n\
AC_SUBST(MOZ_XPINSTALL)\n\
AC_SUBST(MOZ_VIEW_SOURCE)\n\
AC_SUBST(MOZ_SINGLE_PROFILE)\n\
AC_SUBST(MOZ_SPELLCHECK)\n\
AC_SUBST(MOZ_XPFE_COMPONENTS)\n\
AC_SUBST(MOZ_USER_DIR)\n\
AC_SUBST(MOZ_AIRBAG)\n\
AC_SUBST(MOZ_MOCHITEST)\n\
\n\
AC_SUBST(ENABLE_STRIP)\n\
AC_SUBST(USE_ELF_DYNSTR_GC)\n\
AC_SUBST(USE_PREBINDING)\n\
AC_SUBST(INCREMENTAL_LINKER)\n\
AC_SUBST(MOZ_COMPONENTS_VERSION_SCRIPT_LDFLAGS)\n\
AC_SUBST(MOZ_COMPONENT_NSPR_LIBS)\n\
AC_SUBST(MOZ_XPCOM_OBSOLETE_LIBS)\n\
\n\
AC_SUBST(MOZ_FIX_LINK_PATHS)\n\
AC_SUBST(XPCOM_LIBS)\n\
AC_SUBST(XPCOM_FROZEN_LDOPTS)\n\
AC_SUBST(XPCOM_GLUE_LDOPTS)\n\
AC_SUBST(XPCOM_STANDALONE_GLUE_LDOPTS)\n\
\n\
AC_SUBST(USE_DEPENDENT_LIBS)\n\
\n\
AC_SUBST(MOZ_BUILD_ROOT)\n\
AC_SUBST(MOZ_OS2_TOOLS)\n\
AC_SUBST(MOZ_OS2_USE_DECLSPEC)\n\
\n\
AC_SUBST(MOZ_POST_DSO_LIB_COMMAND)\n\
AC_SUBST(MOZ_POST_PROGRAM_COMMAND)\n\
AC_SUBST(MOZ_TIMELINE)\n\
AC_SUBST(WINCE)\n\
AC_SUBST(TARGET_DEVICE)\n\
\n\
AC_SUBST(MOZ_APP_NAME)\n\
AC_SUBST(MOZ_APP_DISPLAYNAME)\n\
AC_SUBST(MOZ_APP_VERSION)\n\
AC_SUBST(FIREFOX_VERSION)\n\
AC_SUBST(THUNDERBIRD_VERSION)\n\
AC_SUBST(SUNBIRD_VERSION)\n\
AC_SUBST(SEAMONKEY_VERSION)\n\
\n\
AC_SUBST(MOZ_PKG_SPECIAL)\n\
\n\
AC_SUBST(MOZILLA_OFFICIAL)\n\
AC_SUBST(BUILD_OFFICIAL)\n\
AC_SUBST(MOZ_MILESTONE_RELEASE)\n\
\n\
dnl win32 options\n\
AC_SUBST(MOZ_PROFILE)\n\
AC_SUBST(MOZ_DEBUG_SYMBOLS)\n\
AC_SUBST(MOZ_MAPINFO)\n\
AC_SUBST(MOZ_BROWSE_INFO)\n\
AC_SUBST(MOZ_TOOLS_DIR)\n\
AC_SUBST(CYGWIN_WRAPPER)\n\
AC_SUBST(AS_PERL)\n\
AC_SUBST(WIN32_REDIST_DIR)\n\
AC_SUBST(PYTHON)\n\
\n\
dnl Echo the CFLAGS to remove extra whitespace.\n\
CFLAGS=`echo \\\n\
	$_WARNINGS_CFLAGS \\\n\
	$CFLAGS`\n\
\n\
CXXFLAGS=`echo \\\n\
	$_MOZ_RTTI_FLAGS \\\n\
	$_MOZ_EXCEPTIONS_FLAGS \\\n\
	$_WARNINGS_CXXFLAGS \\\n\
	$CXXFLAGS`\n\
\n\
COMPILE_CFLAGS=`echo \\\n\
    $_DEFINES_CFLAGS \\\n\
	$_DEPEND_CFLAGS \\\n\
    $COMPILE_CFLAGS`\n\
\n\
COMPILE_CXXFLAGS=`echo \\\n\
    $_DEFINES_CXXFLAGS \\\n\
	$_DEPEND_CFLAGS \\\n\
    $COMPILE_CXXFLAGS`\n\
\n\
AC_SUBST(SYSTEM_MAKEDEPEND)\n\
AC_SUBST(SYSTEM_JPEG)\n\
AC_SUBST(SYSTEM_PNG)\n\
AC_SUBST(SYSTEM_ZLIB)\n\
\n\
AC_SUBST(JPEG_CFLAGS)\n\
AC_SUBST(JPEG_LIBS)\n\
AC_SUBST(ZLIB_CFLAGS)\n\
AC_SUBST(ZLIB_LIBS)\n\
AC_SUBST(PNG_CFLAGS)\n\
AC_SUBST(PNG_LIBS)\n\
\n\
AC_SUBST(MOZ_JPEG_CFLAGS)\n\
AC_SUBST(MOZ_JPEG_LIBS)\n\
AC_SUBST(MOZ_ZLIB_CFLAGS)\n\
AC_SUBST(MOZ_ZLIB_LIBS)\n\
AC_SUBST(MOZ_PNG_CFLAGS)\n\
AC_SUBST(MOZ_PNG_LIBS)\n\
\n\
AC_SUBST(NSPR_CFLAGS)\n\
AC_SUBST(NSPR_LIBS)\n\
AC_SUBST(MOZ_NATIVE_NSPR)\n\
\n\
AC_SUBST(NSS_CFLAGS)\n\
AC_SUBST(NSS_LIBS)\n\
AC_SUBST(NSS_DEP_LIBS)\n\
AC_SUBST(MOZ_NATIVE_NSS)\n\
\n\
AC_SUBST(CFLAGS)\n\
AC_SUBST(CXXFLAGS)\n\
AC_SUBST(CPPFLAGS)\n\
AC_SUBST(COMPILE_CFLAGS)\n\
AC_SUBST(COMPILE_CXXFLAGS)\n\
AC_SUBST(LDFLAGS)\n\
AC_SUBST(LIBS)\n\
AC_SUBST(CROSS_COMPILE)\n\
\n\
AC_SUBST(HOST_CC)\n\
AC_SUBST(HOST_CXX)\n\
AC_SUBST(HOST_CFLAGS)\n\
AC_SUBST(HOST_CXXFLAGS)\n\
AC_SUBST(HOST_OPTIMIZE_FLAGS)\n\
AC_SUBST(HOST_AR)\n\
AC_SUBST(HOST_AR_FLAGS)\n\
AC_SUBST(HOST_LD)\n\
AC_SUBST(HOST_RANLIB)\n\
AC_SUBST(HOST_NSPR_MDCPUCFG)\n\
AC_SUBST(HOST_BIN_SUFFIX)\n\
AC_SUBST(HOST_OS_ARCH)\n\
\n\
AC_SUBST(TARGET_CPU)\n\
AC_SUBST(TARGET_VENDOR)\n\
AC_SUBST(TARGET_OS)\n\
AC_SUBST(TARGET_NSPR_MDCPUCFG)\n\
AC_SUBST(TARGET_MD_ARCH)\n\
AC_SUBST(TARGET_XPCOM_ABI)\n\
AC_SUBST(OS_TARGET)\n\
AC_SUBST(OS_ARCH)\n\
AC_SUBST(OS_RELEASE)\n\
AC_SUBST(OS_TEST)\n\
\n\
AC_SUBST(MOZ_DISABLE_JAR_PACKAGING)\n\
AC_SUBST(MOZ_CHROME_FILE_FORMAT)\n\
\n\
AC_SUBST(WRAP_MALLOC_CFLAGS)\n\
AC_SUBST(WRAP_MALLOC_LIB)\n\
AC_SUBST(MKSHLIB)\n\
AC_SUBST(MKCSHLIB)\n\
AC_SUBST(MKSHLIB_FORCE_ALL)\n\
AC_SUBST(MKSHLIB_UNFORCE_ALL)\n\
AC_SUBST(DSO_CFLAGS)\n\
AC_SUBST(DSO_PIC_CFLAGS)\n\
AC_SUBST(DSO_LDOPTS)\n\
AC_SUBST(LIB_PREFIX)\n\
AC_SUBST(DLL_PREFIX)\n\
AC_SUBST(DLL_SUFFIX)\n\
AC_DEFINE_UNQUOTED(MOZ_DLL_SUFFIX, \"$DLL_SUFFIX\")\n\
AC_SUBST(LIB_SUFFIX)\n\
AC_SUBST(OBJ_SUFFIX)\n\
AC_SUBST(BIN_SUFFIX)\n\
AC_SUBST(ASM_SUFFIX)\n\
AC_SUBST(IMPORT_LIB_SUFFIX)\n\
AC_SUBST(USE_N32)\n\
AC_SUBST(CC_VERSION)\n\
AC_SUBST(CXX_VERSION)\n\
AC_SUBST(MSMANIFEST_TOOL)\n\
\n\
if test \"$USING_HCC\"; then\n\
   CC=\'${topsrcdir}/build/hcc\'\n\
   CC=\"$CC \'$_OLDCC\'\"\n\
   CXX=\'${topsrcdir}/build/hcpp\'\n\
   CXX=\"$CXX \'$_OLDCXX\'\"\n\
   AC_SUBST(CC)\n\
   AC_SUBST(CXX)\n\
fi\n\
\n\
dnl Check for missing components\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test \"$MOZ_X11\"; then\n\
    dnl ====================================================\n\
    dnl = Check if X headers exist\n\
    dnl ====================================================\n\
    _SAVE_CFLAGS=$CFLAGS\n\
    CFLAGS=\"$CFLAGS $XCFLAGS\"\n\
    AC_TRY_COMPILE([\n\
        #include <stdio.h>\n\
        #include <X11/Xlib.h>\n\
        #include <X11/Intrinsic.h>\n\
    ],\n\
    [\n\
        Display *dpy = 0;\n\
        if ((dpy = XOpenDisplay(NULL)) == NULL) {\n\
            fprintf(stderr, \": can\'t open %s\\n\", XDisplayName(NULL));\n\
            exit(1);\n\
        }\n\
    ], [], \n\
    [ AC_MSG_ERROR([Could not compile basic X program.]) ])\n\
    CFLAGS=\"$_SAVE_CFLAGS\"\n\
\n\
    if test ! -z \"$MISSING_X\"; then\n\
        AC_MSG_ERROR([ Could not find the following X libraries: $MISSING_X ]);\n\
    fi\n\
\n\
fi # MOZ_X11\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
dnl Set various defines and substitutions\n\
dnl ========================================================\n\
\n\
if test \"$OS_ARCH\" = \"OS2\" -a \"$VACPP\" = \"yes\"; then\n\
      LIBS=\'so32dll.lib tcp32dll.lib\'\n\
elif test \"$OS_ARCH\" = \"BeOS\"; then\n\
  AC_DEFINE(XP_BEOS)\n\
  MOZ_MOVEMAIL=1\n\
elif test \"$OS_ARCH\" = \"Darwin\"; then\n\
  AC_DEFINE(XP_UNIX)\n\
  AC_DEFINE(UNIX_ASYNC_DNS)\n\
elif test \"$OS_ARCH\" = \"OpenVMS\"; then\n\
  AC_DEFINE(XP_UNIX)\n\
elif test \"$OS_ARCH\" != \"WINNT\" -a \"$OS_ARCH\" != \"OS2\" -a \"$OS_ARCH\" != \"WINCE\"; then\n\
  AC_DEFINE(XP_UNIX)\n\
  AC_DEFINE(UNIX_ASYNC_DNS)\n\
  MOZ_MOVEMAIL=1\n\
fi\n\
AC_SUBST(MOZ_MOVEMAIL)\n\
\n\
AC_DEFINE(JS_THREADSAFE)\n\
\n\
if test \"$MOZ_DEBUG\"; then\n\
    AC_DEFINE(MOZ_REFLOW_PERF)\n\
    AC_DEFINE(MOZ_REFLOW_PERF_DSP)\n\
fi\n\
\n\
if test \"$ACCESSIBILITY\" -a \"$MOZ_ENABLE_GTK2\" ; then\n\
    AC_DEFINE(MOZ_ACCESSIBILITY_ATK)\n\
    ATK_FULL_VERSION=`$PKG_CONFIG --modversion atk`\n\
    ATK_MAJOR_VERSION=`echo ${ATK_FULL_VERSION} | $AWK -F\\. \'{ print $1 }\'`\n\
    ATK_MINOR_VERSION=`echo ${ATK_FULL_VERSION} | $AWK -F\\. \'{ print $2 }\'`\n\
    ATK_REV_VERSION=`echo ${ATK_FULL_VERSION} | $AWK -F\\. \'{ print $3 }\'`\n\
    AC_DEFINE_UNQUOTED(ATK_MAJOR_VERSION, $ATK_MAJOR_VERSION)\n\
    AC_DEFINE_UNQUOTED(ATK_MINOR_VERSION, $ATK_MINOR_VERSION)\n\
    AC_DEFINE_UNQUOTED(ATK_REV_VERSION, $ATK_REV_VERSION)\n\
fi\n\
\n\
# Used for LD_LIBRARY_PATH of run_viewer target\n\
LIBS_PATH=\n\
for lib_arg in $NSPR_LIBS $TK_LIBS; do\n\
  case $lib_arg in\n\
    -L* ) LIBS_PATH=\"${LIBS_PATH:+$LIBS_PATH:}\"`expr $lib_arg : \"-L\\(.*\\)\"` ;;\n\
      * ) ;;\n\
  esac\n\
done\n\
AC_SUBST(LIBS_PATH)\n\
\n\
dnl ========================================================\n\
dnl Use cygwin wrapper for win32 builds, except MSYS/MinGW\n\
dnl ========================================================\n\
case \"$host_os\" in\n\
mingw*)\n\
    WIN_TOP_SRC=`cd $srcdir; pwd -W`\n\
    ;;\n\
cygwin*|msvc*|mks*)\n\
    HOST_CC=\"\\$(CYGWIN_WRAPPER) $HOST_CC\"\n\
    HOST_CXX=\"\\$(CYGWIN_WRAPPER) $HOST_CXX\"\n\
    CC=\"\\$(CYGWIN_WRAPPER) $CC\"\n\
    CXX=\"\\$(CYGWIN_WRAPPER) $CXX\"\n\
    CPP=\"\\$(CYGWIN_WRAPPER) $CPP\"\n\
    LD=\"\\$(CYGWIN_WRAPPER) $LD\"\n\
    AS=\"\\$(CYGWIN_WRAPPER) $AS\"\n\
    RC=\"\\$(CYGWIN_WRAPPER) $RC\"\n\
    MIDL=\"\\$(CYGWIN_WRAPPER) $MIDL\"\n\
    CYGDRIVE_MOUNT=`mount -p | awk \'{ if (/^\\//) { print $1; exit } }\'`\n\
    WIN_TOP_SRC=`cygpath -a -w $srcdir | sed -e \'s|\\\\\\\\|/|g\'`\n\
    ;;\n\
esac\n\
\n\
AC_SUBST(CYGDRIVE_MOUNT)\n\
AC_SUBST(WIN_TOP_SRC)\n\
\n\
AC_SUBST(MOZILLA_VERSION)\n\
\n\
. ${srcdir}/config/chrome-versions.sh\n\
AC_DEFINE_UNQUOTED(MOZILLA_LOCALE_VERSION,\"$MOZILLA_LOCALE_VERSION\")\n\
AC_DEFINE_UNQUOTED(MOZILLA_REGION_VERSION,\"$MOZILLA_REGION_VERSION\")\n\
AC_DEFINE_UNQUOTED(MOZILLA_SKIN_VERSION,\"$MOZILLA_SKIN_VERSION\")\n\
\n\
AC_SUBST(ac_configure_args)\n\
\n\
dnl Spit out some output\n\
dnl ========================================================\n\
\n\
dnl The following defines are used by xpcom\n\
_NON_GLOBAL_ACDEFINES=\"$_NON_GLOBAL_ACDEFINES\n\
CPP_THROW_NEW\n\
HAVE_CPP_2BYTE_WCHAR_T\n\
HAVE_CPP_ACCESS_CHANGING_USING\n\
HAVE_CPP_AMBIGUITY_RESOLVING_USING\n\
HAVE_CPP_BOOL\n\
HAVE_CPP_DYNAMIC_CAST_TO_VOID_PTR\n\
HAVE_CPP_EXPLICIT\n\
HAVE_CPP_MODERN_SPECIALIZE_TEMPLATE_SYNTAX\n\
HAVE_CPP_NAMESPACE_STD\n\
HAVE_CPP_NEW_CASTS\n\
HAVE_CPP_PARTIAL_SPECIALIZATION\n\
HAVE_CPP_TROUBLE_COMPARING_TO_ZERO\n\
HAVE_CPP_TYPENAME\n\
HAVE_CPP_UNAMBIGUOUS_STD_NOTEQUAL\n\
HAVE_STATVFS\n\
NEED_CPP_UNUSED_IMPLEMENTATIONS\n\
NEW_H\n\
HAVE_GETPAGESIZE\n\
HAVE_ICONV\n\
HAVE_ICONV_WITH_CONST_INPUT\n\
HAVE_MBRTOWC\n\
HAVE_SYS_MOUNT_H\n\
HAVE_SYS_VFS_H\n\
HAVE_WCRTOMB\n\
MOZ_V1_STRING_ABI\n\
\"\n\
\n\
AC_CONFIG_HEADER(\n\
gfx/gfx-config.h\n\
netwerk/necko-config.h\n\
xpcom/xpcom-config.h\n\
xpcom/xpcom-private.h\n\
)\n\
\n\
# Save the defines header file before autoconf removes it.\n\
# (Do not add AC_DEFINE calls after this line.)\n\
  _CONFIG_TMP=confdefs-tmp.h\n\
  _CONFIG_DEFS_H=mozilla-config.h\n\
\n\
  cat > $_CONFIG_TMP <<\\EOF\n\
/* List of defines generated by configure. Included with preprocessor flag,\n\
 * -include, to avoid long list of -D defines on the compile command-line.\n\
 * Do not edit.\n\
 */\n\
\n\
#ifndef _MOZILLA_CONFIG_H_\n\
#define _MOZILLA_CONFIG_H_\n\
EOF\n\
\n\
_EGREP_PATTERN=\'^#define (\'\n\
if test -n \"$_NON_GLOBAL_ACDEFINES\"; then\n\
    for f in $_NON_GLOBAL_ACDEFINES; do\n\
        _EGREP_PATTERN=\"${_EGREP_PATTERN}$f|\"\n\
    done\n\
fi\n\
_EGREP_PATTERN=\"${_EGREP_PATTERN}dummy_never_defined)\"\n\
 \n\
  sort confdefs.h | egrep -v \"$_EGREP_PATTERN\" >> $_CONFIG_TMP\n\
\n\
  cat >> $_CONFIG_TMP <<\\EOF\n\
\n\
#endif /* _MOZILLA_CONFIG_H_ */\n\
\n\
EOF\n\
\n\
  # Only write mozilla-config.h when something changes (or it doesn\'t exist)\n\
  if cmp -s $_CONFIG_TMP $_CONFIG_DEFS_H; then\n\
    rm $_CONFIG_TMP\n\
  else\n\
    AC_MSG_RESULT(\"creating $_CONFIG_DEFS_H\")\n\
    mv -f $_CONFIG_TMP $_CONFIG_DEFS_H\n\
\n\
    echo ==== $_CONFIG_DEFS_H =================================\n\
    cat $_CONFIG_DEFS_H\n\
  fi\n\
\n\
dnl Probably shouldn\'t call this manually but we always want the output of DEFS\n\
rm -f confdefs.h.save\n\
mv confdefs.h confdefs.h.save\n\
egrep -v \"$_EGREP_PATTERN\" confdefs.h.save > confdefs.h\n\
AC_OUTPUT_MAKE_DEFS()\n\
MOZ_DEFINES=$DEFS\n\
AC_SUBST(MOZ_DEFINES)\n\
rm -f confdefs.h\n\
mv confdefs.h.save confdefs.h\n\
\n\
dnl Load the list of Makefiles to generate.\n\
dnl   To add new Makefiles, edit allmakefiles.sh.\n\
dnl   allmakefiles.sh sets the variable, MAKEFILES.\n\
. ${srcdir}/allmakefiles.sh\n\
dnl \n\
dnl Run a perl script to quickly create the makefiles.\n\
dnl If it succeeds, it outputs a shell command to set CONFIG_FILES\n\
dnl   for the files it cannot handle correctly. This way, config.status\n\
dnl   will handle these files.\n\
dnl If it fails, nothing is set and config.status will run as usual.\n\
dnl\n\
dnl This does not change the $MAKEFILES variable.\n\
dnl\n\
dnl OpenVMS gets a line overflow on the long eval command, so use a temp file.\n\
dnl\n\
if test -z \"${AS_PERL}\"; then\n\
echo $MAKEFILES | ${PERL} $srcdir/build/autoconf/acoutput-fast.pl > conftest.sh\n\
else\n\
echo $MAKEFILES | ${PERL} $srcdir/build/autoconf/acoutput-fast.pl -nowrap --cygwin-srcdir=$srcdir > conftest.sh\n\
fi\n\
. ./conftest.sh\n\
rm conftest.sh\n\
\n\
echo $MAKEFILES > unallmakefiles\n\
\n\
AC_OUTPUT($MAKEFILES)\n\
\n\
dnl ========================================================\n\
dnl = Setup a nice relatively clean build environment for\n\
dnl = sub-configures.\n\
dnl ========================================================\n\
CC=\"$_SUBDIR_CC\" \n\
CXX=\"$_SUBDIR_CXX\" \n\
CFLAGS=\"$_SUBDIR_CFLAGS\" \n\
CPPFLAGS=\"$_SUBDIR_CPPFLAGS\"\n\
CXXFLAGS=\"$_SUBDIR_CXXFLAGS\"\n\
LDFLAGS=\"$_SUBDIR_LDFLAGS\"\n\
HOST_CC=\"$_SUBDIR_HOST_CC\" \n\
HOST_CFLAGS=\"$_SUBDIR_HOST_CFLAGS\"\n\
HOST_LDFLAGS=\"$_SUBDIR_HOST_LDFLAGS\"\n\
RC=\n\
\n\
unset MAKEFILES\n\
unset CONFIG_FILES\n\
\n\
if test \"$COMPILE_ENVIRONMENT\"; then\n\
if test -z \"$MOZ_NATIVE_NSPR\" || test \"$MOZ_LDAP_XPCOM\"; then\n\
    ac_configure_args=\"$_SUBDIR_CONFIG_ARGS --with-dist-prefix=$MOZ_BUILD_ROOT/dist --with-mozilla\"\n\
    if test -z \"$MOZ_DEBUG\"; then\n\
        ac_configure_args=\"$ac_configure_args --disable-debug\"\n\
    fi\n\
    if test \"$MOZ_OPTIMIZE\" = \"1\"; then\n\
        ac_configure_args=\"$ac_configure_args --enable-optimize\"\n\
    fi\n\
    if test \"$OS_ARCH\" = \"WINNT\" && test \"$NS_TRACE_MALLOC\"; then\n\
       ac_configure_args=\"$ac_configure_args --enable-debug --disable-optimize\"\n\
    fi\n\
    if test -n \"$HAVE_64BIT_OS\"; then\n\
        ac_configure_args=\"$ac_configure_args --enable-64bit\"\n\
    fi\n\
    AC_OUTPUT_SUBDIRS(nsprpub)\n\
    ac_configure_args=\"$_SUBDIR_CONFIG_ARGS\"\n\
fi\n\
\n\
if test -z \"$MOZ_NATIVE_NSPR\"; then\n\
    # Hack to deal with the fact that we use NSPR_CFLAGS everywhere\n\
    AC_MSG_WARN([Recreating autoconf.mk with updated nspr-config output])\n\
    if test ! \"$VACPP\" && test \"$OS_ARCH\" != \"WINNT\" && test \"$OS_ARCH\" != \"WINCE\"; then\n\
       _libs=`./nsprpub/config/nspr-config --prefix=$\\(LIBXUL_DIST\\) --exec-prefix=$\\(DIST\\) --libdir=$\\(LIBXUL_DIST\\)/lib --libs`\n\
       $PERL -pi.bak -e \"s \'^NSPR_LIBS\\\\s*=.*\'NSPR_LIBS = $_libs\'\" config/autoconf.mk\n\
    fi\n\
    if test \"$OS_ARCH\" != \"WINNT\" && test \"$OS_ARCH\" != \"WINCE\" ; then\n\
       _cflags=`./nsprpub/config/nspr-config --prefix=$\\(LIBXUL_DIST\\) --exec-prefix=$\\(DIST\\) --includedir=$\\(LIBXUL_DIST\\)/include/nspr --cflags`\n\
       $PERL -pi.bak -e \"s \'^NSPR_CFLAGS\\\\s*=.*\'NSPR_CFLAGS = $_cflags\'\" config/autoconf.mk\n\
    fi\n\
    rm -f config/autoconf.mk.bak\n\
fi\n\
\n\
# if we\'re building the LDAP XPCOM component, we need to build \n\
# the c-sdk first.  \n\
#\n\
if test \"$MOZ_LDAP_XPCOM\"; then\n\
\n\
    # these subdirs may not yet have been created in the build tree.\n\
    # don\'t use the \"-p\" switch to mkdir, since not all platforms have it\n\
    #\n\
    if test ! -d \"directory\"; then\n\
        mkdir \"directory\"\n\
    fi\n\
    if test ! -d \"directory/c-sdk\"; then\n\
        mkdir \"directory/c-sdk\"    \n\
    fi\n\
    if test ! -d \"directory/c-sdk/ldap\"; then\n\
        mkdir \"directory/c-sdk/ldap\"    \n\
    fi\n\
\n\
    ac_configure_args=\"$_SUBDIR_CONFIG_ARGS --prefix=$MOZ_BUILD_ROOT/dist --with-dist-prefix=$MOZ_BUILD_ROOT/dist --without-nss --with-mozilla\"\n\
    if test -z \"$MOZ_DEBUG\"; then\n\
        ac_configure_args=\"$ac_configure_args --disable-debug\"\n\
    fi\n\
    if test \"$MOZ_OPTIMIZE\" = \"1\"; then\n\
        ac_configure_args=\"$ac_configure_args --enable-optimize\"\n\
    fi\n\
    if test -n \"$HAVE_64BIT_OS\"; then\n\
        ac_configure_args=\"$ac_configure_args --enable-64bit\"\n\
    fi\n\
    AC_OUTPUT_SUBDIRS(directory/c-sdk)\n\
    ac_configure_args=\"$_SUBDIR_CONFIG_ARGS\"\n\
fi\n\
fi # COMPILE_ENVIRONMENT\n\
\n\
\n\
"
#if 0
static void u0054_repo_encodings__verify_vhash_has_path(
	SG_context* pCtx,
	const SG_vhash* pvh,
	const char* pszPath
	)
{
	SG_uint32 count;
	SG_uint32 i;
	SG_bool bFound = SG_FALSE;

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &count)  );

	for (i=0; i<count; i++)
	{
		const char* pszgid = NULL;
		const SG_variant* pv = NULL;
		SG_vhash* pvhItem = NULL;
		const char* pszItemPath = NULL;

		SG_ERR_CHECK(  SG_vhash__get_nth_pair(pCtx, pvh, i, &pszgid, &pv)  );
		SG_ERR_CHECK(  SG_variant__get__vhash(pCtx, pv, &pvhItem)  );
		SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, SG_STATUS_ENTRY_KEY__REPO_PATH, &pszItemPath)  );
		if (0 == strcmp(pszPath, pszItemPath))
		{
			bFound = SG_TRUE;
			break;
		}
	}

	if (!bFound)
	{
		SG_ERR_THROW(  SG_ERR_NOT_FOUND  );
	}


fail:
	return;
}
#endif

#if 0
static void u0054_repo_encodings__verify_vhash_sz(
	SG_context* pCtx,
	const SG_vhash* pvh,
	const char* pszKey,
	const char* pszShouldBe
	)
{
	const char* pszIs;

	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvh, pszKey, &pszIs)  );
	if (0 != strcmp(pszIs, pszShouldBe))
	{
		SG_ERR_THROW(  SG_ERR_NOT_FOUND  );
	}

fail:
	return;
}
#endif

static void u0054_repo_encodings__verify_vhash_count(
	SG_context* pCtx,
	const SG_vhash* pvh,
	SG_uint32 count_should_be
	)
{
	SG_uint32 count;

	SG_ERR_CHECK(  SG_vhash__count(pCtx, pvh, &count)  );
	VERIFY_COND("u0054_repo_encodings__verify_vhash_count", (count == count_should_be));

fail:
	return;
}

static void u0054_repo_encodings__verifytree__dir_has_file(
	SG_context* pCtx,
	const SG_vhash* pvh,
	const char* pszName,
    char* buf_hid,
	SG_uint32 lenBuf
	)
{
	SG_vhash* pvhItem = NULL;
	const char* pszType = NULL;
    const char* psz_hid = NULL;

	VERIFY_COND("u0054_repo_encodings__verifytree__dir_has_file", (lenBuf >= SG_HID_MAX_BUFFER_LENGTH));

	SG_ERR_CHECK(  SG_vhash__get__vhash(pCtx, pvh, pszName, &pvhItem)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "type", &pszType)  );
	SG_ERR_CHECK(  SG_vhash__get__sz(pCtx, pvhItem, "hid", &psz_hid)  );

	VERIFY_COND("u0054_repo_encodings__verifytree__dir_has_file", (lenBuf > strlen(psz_hid)));
    SG_ERR_CHECK(  SG_strcpy(pCtx, buf_hid, lenBuf, psz_hid)  );

	VERIFY_COND("u0054_repo_encodings__verifytree__dir_has_file", (0 == strcmp(pszType, "file")));


fail:
	return;
}

static void u0054_repo_encodings__do_get_dir(SG_context* pCtx,SG_repo* pRepo, const char* pszidHidTreeNode, SG_vhash** ppResult)
{
	SG_uint32 count;
	SG_uint32 i;
	SG_treenode* pTreenode = NULL;
	SG_vhash* pvhDir = NULL;
	SG_vhash* pvhSub = NULL;
	SG_vhash* pvhSubDir = NULL;

	SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhDir)  );
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszidHidTreeNode, &pTreenode)  );
	SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenode, &count)  );
	for (i=0; i<count; i++)
	{
		const SG_treenode_entry* pEntry = NULL;
		SG_treenode_entry_type type;
		const char* pszName = NULL;
		const char* pszidHid = NULL;
		const char* pszgid = NULL;

		SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pTreenode, i, &pszgid, &pEntry)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_type(pCtx, pEntry, &type)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_entry_name(pCtx, pEntry, &pszName)  );
		SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pEntry, &pszidHid)  );

		SG_ERR_CHECK(  SG_VHASH__ALLOC(pCtx, &pvhSub)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub, "hid", pszidHid)  );
		SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub, "gid", pszgid)  );

		if (SG_TREENODEENTRY_TYPE_DIRECTORY == type)
		{
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub, "type", "directory")  );

			SG_ERR_CHECK(  u0054_repo_encodings__do_get_dir(pCtx, pRepo, pszidHid, &pvhSubDir)  );
			SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhSub, "dir", &pvhSubDir)  );
		}
		else if (SG_TREENODEENTRY_TYPE_REGULAR_FILE == type)
		{
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub, "type", "file")  );

		}
		else if (SG_TREENODEENTRY_TYPE_SYMLINK == type)
		{
			SG_ERR_CHECK(  SG_vhash__add__string__sz(pCtx, pvhSub, "type", "symlink")  );

		}
		else
		{
			SG_ERR_THROW(  SG_ERR_NOTIMPLEMENTED  );
		}

		SG_ERR_CHECK(  SG_vhash__add__vhash(pCtx, pvhDir, pszName, &pvhSub)  );
	}

	SG_TREENODE_NULLFREE(pCtx, pTreenode);
	pTreenode = NULL;

	*ppResult = pvhDir;

	return;

fail:
	SG_VHASH_NULLFREE(pCtx, pvhDir);
	SG_VHASH_NULLFREE(pCtx, pvhSub);

}

static void u0054_repo_encodings__get_entire_repo_manifest(
	SG_context* pCtx,
	const char* pszRidescName,
	SG_vhash** ppResult
	)
{
	SG_vhash* pvh = NULL;
	SG_repo* pRepo = NULL;
	SG_rbtree* pIdsetLeaves = NULL;
	SG_uint32 count_leaves = 0;
	char* pszidUserSuperRoot = NULL;
	SG_rbtree_iterator* pIterator = NULL;
	SG_bool b = SG_FALSE;
	const char* pszLeaf = NULL;
	SG_treenode* pTreenode = NULL;
	const SG_treenode_entry* pEntry = NULL;
	const char* pszidHidActualRoot = NULL;
	SG_uint32 count = 0;

	/*
	 * Fetch the descriptor by its given name and use it to connect to
	 * the repo.
	 */
	SG_ERR_CHECK(  SG_REPO__OPEN_REPO_INSTANCE(pCtx, pszRidescName, &pRepo)  );

	/*
	 * This routine currently only works if there is exactly one leaf
	 * in the repo.  A smarter version of this (later) will allow the
	 * desired dagnode to be specified either by its hex id or by a
	 * named branch or a label.
	 */
	SG_ERR_CHECK(  SG_repo__fetch_dag_leaves(pCtx, pRepo,SG_DAGNUM__VERSION_CONTROL,&pIdsetLeaves)  );
	SG_ERR_CHECK(  SG_rbtree__count(pCtx, pIdsetLeaves, &count_leaves)  );

	SG_ARGCHECK_RETURN(count_leaves == 1, count_leaves);

	SG_ERR_CHECK(  SG_rbtree__iterator__first(pCtx, &pIterator, pIdsetLeaves, &b, &pszLeaf, NULL)  );
	SG_rbtree__iterator__free(pCtx, pIterator);
	pIterator = NULL;

	/*
	 * Load the desired changeset from the repo so we can look up the
	 * id of its user root directory
	 */
	SG_ERR_CHECK(  SG_changeset__tree__find_root_hid(pCtx, pRepo, pszLeaf, &pszidUserSuperRoot)  );

	/*
	 * What we really want here is not the super-root, but the actual root.
	 * So let's jump down one directory.
	 */
	SG_ERR_CHECK(  SG_treenode__load_from_repo(pCtx, pRepo, pszidUserSuperRoot, &pTreenode)  );
	SG_ERR_CHECK(  SG_treenode__count(pCtx, pTreenode, &count)  );
	VERIFY_COND("count should be 1", (count == 1));

	SG_ERR_CHECK(  SG_treenode__get_nth_treenode_entry__ref(pCtx, pTreenode, 0, NULL, &pEntry)  );
	SG_ERR_CHECK(  SG_treenode_entry__get_hid_blob(pCtx, pEntry, &pszidHidActualRoot)  );

	/*
	 * Retrieve everything.  A new directory will be created in cwd.
	 * Its name will be the name of the descriptor.
	 */
	SG_ERR_CHECK(  u0054_repo_encodings__do_get_dir(pCtx, pRepo, pszidHidActualRoot, &pvh)  );

	SG_TREENODE_NULLFREE(pCtx, pTreenode);
	pTreenode = NULL;

	SG_NULLFREE(pCtx, pszidUserSuperRoot);
	SG_RBTREE_NULLFREE(pCtx, pIdsetLeaves);
	pIdsetLeaves = NULL;
	SG_REPO_NULLFREE(pCtx, pRepo);
	pRepo = NULL;

	*ppResult = pvh;

fail:
	return;
}


static void u0054_repo_encodings__commit_all(
	SG_context* pCtx,
	const SG_pathname* pPathWorkingDir,
    char ** ppsz_hid_cs
	)
{
	SG_ERR_CHECK_RETURN(  unittests_pendingtree__simple_commit(pCtx, pPathWorkingDir, ppsz_hid_cs)  );
}

static void u0054_repo_encodings__create_file__numbers(
	SG_context* pCtx,
	SG_pathname* pPath,
	const char* pszName,
	const SG_uint32 countLines
	)
{
	SG_pathname* pPathFile = NULL;
	SG_uint32 i;
	SG_file* pFile = NULL;

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pPath, pszName)  );
	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFile, SG_FILE_CREATE_NEW | SG_FILE_RDWR, 0600, &pFile)  );
	for (i=0; i<countLines; i++)
	{
		char buf[64];

		VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "%d\n", i)  );
		VERIFY_ERR_CHECK(  SG_file__write(pCtx, pFile, (SG_uint32) strlen(buf), (SG_byte*) buf, NULL)  );
	}
	VERIFY_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);

	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
}

static void u0054_repo_encodings__write_file__string(
	SG_context* pCtx,
	SG_pathname* pPath,
	const char* pszName,
	const char* pszContents
	)
{
	SG_pathname* pPathFile = NULL;
	SG_file* pFile = NULL;

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pPath, pszName)  );
	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFile, SG_FILE_OPEN_OR_CREATE | SG_FILE_RDWR, 0600, &pFile)  );
	VERIFY_ERR_CHECK(  SG_file__write(pCtx, pFile, (SG_uint32) strlen(pszContents), (SG_byte*) pszContents, NULL)  );
	VERIFY_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);

	return;

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
}

static void u0054_repo_encodings__append_to_file__numbers(
	SG_context* pCtx,
	SG_pathname* pPath,
	const char* pszName,
	const SG_uint32 countLines
	)
{
	SG_pathname* pPathFile = NULL;
	SG_uint32 i;
	SG_file* pFile = NULL;
	SG_uint64 len = 0;

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pPath, pszName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__length__pathname(pCtx, pPathFile, &len, NULL)  );
	VERIFY_ERR_CHECK(  SG_file__open__pathname(pCtx, pPathFile, SG_FILE_OPEN_EXISTING | SG_FILE_RDWR, 0600, &pFile)  );
	VERIFY_ERR_CHECK(  SG_file__seek(pCtx, pFile, len)  );
	for (i=0; i<countLines; i++)
	{
		char buf[64];

		VERIFY_ERR_CHECK(  SG_sprintf(pCtx, buf, sizeof(buf), "%d\n", i)  );
		VERIFY_ERR_CHECK(  SG_file__write(pCtx, pFile, (SG_uint32) strlen(buf), (SG_byte*) buf, NULL)  );
	}
	VERIFY_ERR_CHECK(  SG_file__close(pCtx, &pFile)  );

	SG_PATHNAME_NULLFREE(pCtx, pPathFile);

fail:
	return;
}


static int u0054_repo_encodings_test__1(SG_context* pCtx,SG_pathname* pPathTopDir)
{
	char bufName[SG_TID_MAX_BUFFER_LENGTH];
	char bufOtherName[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathWorkingDir = NULL;
	SG_pathname* pPathGetDir = NULL;
	SG_pathname* pPathFile = NULL;
    SG_bool bExists = SG_FALSE;
	SG_vhash* pvh = NULL;
	char buf_hid_1[SG_HID_MAX_BUFFER_LENGTH];
	char buf_hid_2[SG_HID_MAX_BUFFER_LENGTH];
	char bufCSet[SG_HID_MAX_BUFFER_LENGTH];

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufName, sizeof(bufName), 32)  );

	/* create the working dir */
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir, pPathTopDir, bufName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathWorkingDir)  );

    /* add stuff */
	VERIFY_ERR_CHECK(  u0054_repo_encodings__create_file__numbers(pCtx, pPathWorkingDir, "a", 2000)  );

    /* create the repo */
	VERIFY_ERR_CHECK(  _ut_pt__new_repo(pCtx, bufName, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  _ut_pt__addremove(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  u0054_repo_encodings__commit_all(pCtx, pPathWorkingDir, NULL)  );

	/* Load a listing of everything in the repo so we can verify */
	VERIFY_ERR_CHECK(  u0054_repo_encodings__get_entire_repo_manifest(pCtx, bufName, &pvh)  );
	VERIFY_ERR_CHECK(  u0054_repo_encodings__verify_vhash_count(pCtx, pvh, 1)  );
	VERIFY_ERR_CHECK(  u0054_repo_encodings__verifytree__dir_has_file(pCtx, pvh, "a", buf_hid_1, sizeof(buf_hid_1))  );
	SG_VHASH_NULLFREE(pCtx, pvh);

	VERIFY_ERR_CHECK(  u0054_repo_encodings__append_to_file__numbers(pCtx, pPathWorkingDir, "a", 10)  );
	VERIFY_ERR_CHECK(  u0054_repo_encodings__commit_all(pCtx, pPathWorkingDir, NULL)  );

	VERIFY_ERR_CHECK(  _ut_pt__get_baseline(pCtx, pPathWorkingDir, bufCSet, sizeof(bufCSet))  );

	VERIFY_ERR_CHECK(  u0054_repo_encodings__get_entire_repo_manifest(pCtx, bufName, &pvh)  );
	VERIFY_ERR_CHECK(  u0054_repo_encodings__verify_vhash_count(pCtx, pvh, 1)  );
	VERIFY_ERR_CHECK(  u0054_repo_encodings__verifytree__dir_has_file(pCtx, pvh, "a", buf_hid_2, sizeof(buf_hid_2))  );
	SG_VHASH_NULLFREE(pCtx, pvh);

	/* Now get it */
	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufOtherName, sizeof(bufOtherName), 32)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathGetDir, pPathTopDir, bufOtherName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathGetDir)  );
	VERIFY_ERR_CHECK(  unittests_workingdir__do_export(pCtx, bufName, pPathGetDir, bufCSet)  );

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pPathGetDir, "a")  );
	VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathFile, &bExists, NULL, NULL)  );
    printf("%s\n", SG_pathname__sz(pPathFile));
	VERIFY_COND("here", (bExists));

	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathGetDir);

	return 1;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);

	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathGetDir);

	return 0;
}

#if !defined(WINDOWS)
static int u0054_repo_encodings_test__autoconf(SG_context* pCtx,SG_pathname* pPathTopDir)
{
	char bufName[SG_TID_MAX_BUFFER_LENGTH];
	char bufOtherName[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathWorkingDir = NULL;
	SG_pathname* pPathGetDir = NULL;
	SG_pathname* pPathFile = NULL;
    SG_bool bExists = SG_FALSE;
	SG_vhash* pvh = NULL;
	char buf_hid_1[SG_HID_MAX_BUFFER_LENGTH];
	char buf_hid_2[SG_HID_MAX_BUFFER_LENGTH];
	char bufCSet[SG_HID_MAX_BUFFER_LENGTH];

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufName, sizeof(bufName), 32)  );

	/* create the working dir */
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathWorkingDir, pPathTopDir, bufName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathWorkingDir)  );

    /* add stuff */
	VERIFY_ERR_CHECK(  u0054_repo_encodings__write_file__string(pCtx, pPathWorkingDir, "autoconf", MOZ_AC_V1)  );

    /* create the repo */
	VERIFY_ERR_CHECK(  _ut_pt__new_repo(pCtx, bufName, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  _ut_pt__addremove(pCtx, pPathWorkingDir)  );
	VERIFY_ERR_CHECK(  u0054_repo_encodings__commit_all(pCtx, pPathWorkingDir, NULL)  );

	/* Load a listing of everything in the repo so we can verify */
	VERIFY_ERR_CHECK(  u0054_repo_encodings__get_entire_repo_manifest(pCtx, bufName, &pvh)  );
	VERIFY_ERR_CHECK(  u0054_repo_encodings__verify_vhash_count(pCtx, pvh, 1)  );
	VERIFY_ERR_CHECK(  u0054_repo_encodings__verifytree__dir_has_file(pCtx, pvh, "autoconf", buf_hid_1, sizeof(buf_hid_1))  );
	SG_VHASH_NULLFREE(pCtx, pvh);

	VERIFY_ERR_CHECK(  u0054_repo_encodings__write_file__string(pCtx, pPathWorkingDir, "autoconf", MOZ_AC_V2)  );
	VERIFY_ERR_CHECK(  u0054_repo_encodings__commit_all(pCtx, pPathWorkingDir, NULL)  );

	VERIFY_ERR_CHECK(  _ut_pt__get_baseline(pCtx, pPathWorkingDir, bufCSet, sizeof(bufCSet))  );

	VERIFY_ERR_CHECK(  u0054_repo_encodings__get_entire_repo_manifest(pCtx, bufName, &pvh)  );
	VERIFY_ERR_CHECK(  u0054_repo_encodings__verify_vhash_count(pCtx, pvh, 1)  );
	VERIFY_ERR_CHECK(  u0054_repo_encodings__verifytree__dir_has_file(pCtx, pvh, "autoconf", buf_hid_2, sizeof(buf_hid_2))  );
	SG_VHASH_NULLFREE(pCtx, pvh);

	/* Now get it */
	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufOtherName, sizeof(bufOtherName), 32)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathGetDir, pPathTopDir, bufOtherName)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathGetDir)  );
	VERIFY_ERR_CHECK(  unittests_workingdir__do_export(pCtx, bufName, pPathGetDir, bufCSet)  );

	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__PATHNAME_SZ(pCtx, &pPathFile, pPathGetDir, "autoconf")  );
	VERIFY_ERR_CHECK(  SG_fsobj__exists__pathname(pCtx, pPathFile, &bExists, NULL, NULL)  );
	VERIFY_COND("here", (bExists));

	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathGetDir);

	return 1;

fail:
	SG_VHASH_NULLFREE(pCtx, pvh);

	SG_PATHNAME_NULLFREE(pCtx, pPathWorkingDir);
	SG_PATHNAME_NULLFREE(pCtx, pPathFile);
	SG_PATHNAME_NULLFREE(pCtx, pPathGetDir);

	return 0;
}
#endif

TEST_MAIN(u0054_repo_encodings)
{
	char bufTopDir[SG_TID_MAX_BUFFER_LENGTH];
	SG_pathname* pPathTopDir = NULL;

	TEMPLATE_MAIN_START;

	VERIFY_ERR_CHECK(  SG_tid__generate2(pCtx, bufTopDir, sizeof(bufTopDir), 32)  );
	VERIFY_ERR_CHECK(  SG_PATHNAME__ALLOC__SZ(pCtx, &pPathTopDir,bufTopDir)  );
	VERIFY_ERR_CHECK(  SG_fsobj__mkdir__pathname(pCtx, pPathTopDir)  );

	BEGIN_TEST(  u0054_repo_encodings_test__1(pCtx, pPathTopDir)  );
#if !defined(WINDOWS)
	BEGIN_TEST(  u0054_repo_encodings_test__autoconf(pCtx, pPathTopDir)  );
#endif

	/* TODO rm -rf the top dir */

	// fall-thru to common cleanup

fail:
	SG_PATHNAME_NULLFREE(pCtx, pPathTopDir);

	TEMPLATE_MAIN_END;
}

