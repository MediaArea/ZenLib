dnl #########################################################################
dnl ### Init
dnl #########################################################################

dnl -------------------------------------------------------------------------
dnl Name and version
dnl
AC_INIT([libzen], [0.4.39])

dnl -------------------------------------------------------------------------
dnl Test if we are at the good place
dnl
AC_CONFIG_SRCDIR([../../../Source/ZenLib/Ztring.cpp])

dnl -------------------------------------------------------------------------
dnl sets build, host, target variables and the same with _alias
dnl
AC_CANONICAL_BUILD
AM_INIT_AUTOMAKE([foreign -Wall])

dnl -------------------------------------------------------------------------
dnl Autotools init
dnl
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_PROG_INSTALL

dnl #########################################################################
dnl ### Options
dnl #########################################################################

dnl -------------------------------------------------------------------------
dnl Enable
dnl
AC_ARG_ENABLE([debug],            [AS_HELP_STRING([--enable-debug],             [Enable full debug])],                          , [enable_debug=no])
AC_ARG_ENABLE([gprof],            [AS_HELP_STRING([--enable-gprof],             [Enable gprof log])],                           , [enable_gprof=no])
AC_ARG_ENABLE([universal_binary], [AS_HELP_STRING([--enable-universal_binary],  [Create Mac PPC/i386 Universal binary])],       , [enable_universal_binary=no])
AC_ARG_ENABLE([arch-ppc],         [AS_HELP_STRING(([--enable-arch-ppc],         [Create Mac PPC architecture]))],               , [enable_arch_ppc=no])
AC_ARG_ENABLE([arch-ppc64],       [AS_HELP_STRING(([--enable-arch-ppc64],       [Create Mac PPC64 architecture]))],             , [enable_arch_ppc64=no])
AC_ARG_ENABLE([arch-i386],        [AS_HELP_STRING(([--enable-arch-i386],        [Create Mac/iPhone i386 architecture]))],       , [enable_arch_i386=no])
AC_ARG_ENABLE([arch-x86_64],      [AS_HELP_STRING(([--enable-arch-x86_64],      [Create Mac/iPhone x86_64 architecture]))],     , [enable_arch_x86_64=no])
AC_ARG_ENABLE([arch-armv7],       [AS_HELP_STRING(([--enable-arch-armv7],       [Create iPhone ARMv7 architecture]))],          , [enable_arch_armv7=no])
AC_ARG_ENABLE([arch-armv7s],      [AS_HELP_STRING(([--enable-arch-armv7s],      [Create iPhone ARMv7s architecture]))],         , [enable_arch_armv7s=no])
AC_ARG_ENABLE([arch-arm64],       [AS_HELP_STRING(([--enable-arch-arm64],       [Create Mac/iPhone ARM64 architecture]))],      , [enable_arch_arm64=no])
AC_ARG_ENABLE([unicode],          [AS_HELP_STRING(([--disable-unicode],         [Disable unicode]))],                           , [enable_unicode=yes])
AC_ARG_ENABLE([wstring_missing],  [AS_HELP_STRING(([--enable-wstring_missing],  [Disable wstring support (old compilers)]))],   , [enable_wstring_missing=no])
AC_ARG_ENABLE([stream_missing],   [AS_HELP_STRING(([--enable-stream_missing],   [Disable stream support (old compilers)]))],    , [enable_stream_missing=no])
AC_ARG_ENABLE([large_files],      [AS_HELP_STRING(([--disable-large_files],     [Disable large files support]))],               , [enable_large_files=yes])
dnl AC_ARG_ENABLE([threads],          [AS_HELP_STRING(([--disable-threads],         [Disable threads support]))],                   , [enable_threads=yes])
 

dnl -------------------------------------------------------------------------
dnl With
dnl
AC_ARG_WITH([macosx-sdk],           [AS_HELP_STRING(([--with-macosx-sdk],           [Force the Mac SDK]))],                         , [with_macosx_sdk=no])
AC_ARG_WITH([macosx-version-min],   [AS_HELP_STRING(([--with-macosx-version-min],   [Force the Mac Version]))],                     , [with_macosx_version_min=no])
AC_ARG_WITH([iphoneos-sdk],         [AS_HELP_STRING(([--with-iphoneos-sdk],         [Force the iPhone SDK]))],                      , [with_iphoneos_sdk=no])
AC_ARG_WITH([iphoneos-version-min], [AS_HELP_STRING(([--with-iphoneos-version-min], [Force the iPhone Version]))],                  , [with_iphoneos_version_min=no])

dnl #########################################################################
dnl ### Compiler specific
dnl #########################################################################

dnl -------------------------------------------------------------------------
case "${build}" in
  powerpc-montavista* )
    CFLAGS="$CFLAGS -D__MONTAVISTA__"
    CXXFLAGS="$CXXFLAGS -D__MONTAVISTA__"
  ;;
esac

dnl -------------------------------------------------------------------------
dnl Mac/iPhone Architecture and SDK
dnl
if test "$enable_universal_binary" = "yes"; then
	enable_arch_ppc=yes
	enable_arch_i386=yes
fi
if test "$enable_arch_ppc" = "yes"; then
	CXXFLAGS="$CXXFLAGS -arch ppc"
	CFLAGS="$CFLAGS -arch ppc"
	LDFLAGS="$LDFLAGS -arch ppc"
fi
if test "$enable_arch_ppc64" = "yes"; then
	CXXFLAGS="$CXXFLAGS -arch ppc64"
	CFLAGS="$CFLAGS -arch ppc64"
	LDFLAGS="$LDFLAGS -arch ppc64"
fi
if test "$enable_arch_i386" = "yes"; then
	CXXFLAGS="$CXXFLAGS -arch i386"
	CFLAGS="$CFLAGS -arch i386"
	LDFLAGS="$LDFLAGS -arch i386"
fi
if test "$enable_arch_x86_64" = "yes"; then
	CXXFLAGS="$CXXFLAGS -arch x86_64"
	CFLAGS="$CFLAGS -arch x86_64"
	LDFLAGS="$LDFLAGS -arch x86_64"
fi
if test "$enable_arch_armv7" = "yes"; then
	CXXFLAGS="$CXXFLAGS -arch armv7"
	CFLAGS="$CFLAGS -arch armv7"
	LDFLAGS="$LDFLAGS -arch armv7"
fi
if test "$enable_arch_armv7s" = "yes"; then
	CXXFLAGS="$CXXFLAGS -arch armv7s"
	CFLAGS="$CFLAGS -arch armv7s"
	LDFLAGS="$LDFLAGS -arch armv7s"
fi
if test "$enable_arch_arm64" = "yes"; then
	CXXFLAGS="$CXXFLAGS -arch arm64"
	CFLAGS="$CFLAGS -arch arm64"
	LDFLAGS="$LDFLAGS -arch arm64"
fi
if test "x$with_macosx_sdk" != xno ; then
	CXXFLAGS="-isysroot $with_macosx_sdk $CXXFLAGS"
	CFLAGS="-isysroot $with_macosx_sdk $CFLAGS"
	LDFLAGS="-isysroot $with_macosx_sdk $LDFLAGS"
fi
if test "x$with_macosx_version_min" != xno ; then
	CXXFLAGS="-mmacosx-version-min=$with_macosx_version_min $CXXFLAGS"
	CFLAGS="-mmacosx-version-min=$with_macosx_version_min $CFLAGS"
	LDFLAGS="-mmacosx-version-min=$with_macosx_version_min $LDFLAGS"
fi
if test "x$with_iphoneos_sdk" != xno ; then
	CXXFLAGS="-isysroot $with_iphoneos_sdk $CXXFLAGS"
	CFLAGS="-isysroot $with_iphoneos_sdk $CFLAGS"
	LDFLAGS="-isysroot $with_iphoneos_sdk $LDFLAGS"
fi
if test "x$with_iphoneos_version_min" != xno ; then
	CXXFLAGS="-miphoneos-version-min=$with_iphoneos_version_min $CXXFLAGS"
	CFLAGS="-miphoneos-version-min=$with_iphoneos_version_min $CFLAGS"
	LDFLAGS="-miphoneos-version-min=$with_iphoneos_version_min $LDFLAGS"
fi

dnl #########################################################################
dnl ### ZenLib flags
dnl #########################################################################

dnl -------------------------------------------------------------------------
dnl Common
dnl
if test "$enable_shared" = "yes"; then
    ZenLib_LIBS="-L$(pwd)/.libs -lzen -lpthread $ZenLib_LIBS"
fi
if test "$enable_static" = "yes"; then
    ZenLib_LIBS_Static="$(pwd)/.libs/libzen.a -lpthread $ZenLib_LIBS_Static"
fi

dnl #########################################################################
dnl ### C/C++ compiler options used to compile
dnl #########################################################################

dnl -------------------------------------------------------------------------
dnl wstring
dnl
if test "$enable_wstring_missing" = "yes"; then
	CXXFLAGS="$CXXFLAGS -DWSTRING_MISSING"
	ZenLib_CXXFLAGS="$ZenLib_CXXFLAGS -DWSTRING_MISSING"
	ZenLib_wstring_missing="yes"
	enable_unicode="no"
else
	ZenLib_wstring_missing="no"
fi

dnl -------------------------------------------------------------------------
dnl Unicode
dnl
if test "$enable_unicode" = "yes"; then
	CXXFLAGS="$CXXFLAGS -DUNICODE"
	ZenLib_CXXFLAGS="$ZenLib_CXXFLAGS -DUNICODE"
	ZenLib_Unicode="yes"
else
	ZenLib_Unicode="no"
fi

dnl -------------------------------------------------------------------------
dnl Large files
dnl
if test "$enable_large_files" = "yes"; then
	CXXFLAGS="$CXXFLAGS -D_LARGE_FILES -D_FILE_OFFSET_BITS=64"
	ZenLib_LargeFiles="yes"
else
	ZenLib_LargeFiles="no"
fi

dnl -------------------------------------------------------------------------
dnl Threads
dnl

dnl -------------------------------------------------------------------------
dnl Check if size_t is long
dnl
AC_LANG([C++])
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
	#include <cstdlib>
	void foo(signed int) {}
	void foo(unsigned int) {}
	int main () {foo(size_t(0)); return 0;}])], [size_t_is_long="no"], [size_t_is_long="yes"])

if test "$size_t_is_long" = "yes"; then
	CXXFLAGS="$CXXFLAGS -DSIZE_T_IS_LONG"
	ZenLib_CXXFLAGS="$ZenLib_CXXFLAGS -DSIZE_T_IS_LONG"
fi

dnl -------------------------------------------------------------------------
dnl Check if stream is available
dnl
AC_LANG([C++])
if test "$enable_unicode" = "yes"; then
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
        #include <sstream>
        int main () {std::wstringstream message; message<<1.0; return 0;}])], , [enable_stream_missing="yes"])
else
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
        #include <sstream>
        int main () {std::stringstream message; message<<1.0; return 0;}])], , [enable_stream_missing="yes"])
fi

if test "$enable_stream_missing" = "yes"; then
	CXXFLAGS="$CXXFLAGS -DSTREAM_MISSING"
fi

dnl -------------------------------------------------------------------------
dnl GNU compiler
dnl
if test "$GXX" = yes ; then
    case "${build}" in
      *mingw* | *cygwin* ) ;;
      *)
        CFLAGS="$CFLAGS -fPIC"
        CXXFLAGS="$CXXFLAGS -fPIC"
        LDFLAGS="$LDFLAGS -fPIC"
      ;;
    esac
	if test "$enable_debug" = "yes"; then
		CFLAGS="$CFLAGS -O0 -g -Wall -Wundef"
		CXXFLAGS="$CXXFLAGS -O0 -g -Wall -W -Wundef -Wcast-qual -Wno-ctor-dtor-privacy"
	else
		CFLAGS="$CFLAGS -O2"
		CXXFLAGS="$CXXFLAGS -O2"
	fi
	if test "$enable_gprof" = "yes"; then
		CFLAGS="$CFLAGS -pg -g"
		CXXFLAGS="$CXXFLAGS -pg -g"
	fi
fi

dnl -------------------------------------------------------------------------
dnl External libs
dnl
LDFLAGS="$LDFLAGS -lpthread"

dnl -------------------------------------------------------------------------
dnl Check if thread safe variants of time functions are available
dnl
AC_CHECK_FUNCS(gmtime_r localtime_r)

dnl #########################################################################
dnl ### Output
dnl #########################################################################

AC_SUBST(ZenLib_CXXFLAGS)
AC_SUBST(ZenLib_LIBS)
AC_SUBST(ZenLib_LIBS_Static)
AC_SUBST(ZenLib_Unicode)
AC_SUBST(ZenLib_wstring_missing)
AC_SUBST(ZenLib_LargeFiles)
AC_CONFIG_FILES(libzen-config, [chmod u+x libzen-config])
AC_CONFIG_FILES(libzen.pc)

AC_OUTPUT(Makefile)

dnl #########################################################################
dnl ### Report how we have been configured
dnl #########################################################################

echo ""
echo "Configured ZenLib for '$build'"
echo ""
echo "  Unicode?                                                $enable_unicode"
echo "  Large files support?                                    $enable_large_files"
echo "  wstring missing support?                                $enable_wstring_missing"
echo "  stream missing support?                                 $enable_stream_missing"
echo ""
echo "  Create static lib?                                      $enable_static"
echo "  Create shared lib?                                      $enable_shared"
echo ""
echo "  CXXFLAGS: $CXXFLAGS"
echo "  LIBS: $LIBS"
