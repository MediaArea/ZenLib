dnl #########################################################################
dnl ### Init
dnl #########################################################################

dnl -------------------------------------------------------------------------
dnl Test if we are at the good place
dnl
AC_INIT(../../../Source/ZenLib/Ztring.cpp)

dnl -------------------------------------------------------------------------
dnl sets build, host, target variables and the same with _alias
dnl
AC_CANONICAL_BUILD

dnl -------------------------------------------------------------------------
dnl Name and version
dnl
AM_INIT_AUTOMAKE(ZenLib,0)

dnl -------------------------------------------------------------------------
dnl Autotools init
dnl
CFLAGS="$CFLAGS"
CXXFLAGS="$CXXFLAGS"
AC_PROG_CC
AC_PROG_CXX
AC_DISABLE_SHARED
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
AC_PROG_INSTALL

dnl #########################################################################
dnl Configure
dnl #########################################################################

Home=`pwd`

dnl #########################################################################
dnl ### Options
dnl #########################################################################

dnl -------------------------------------------------------------------------
dnl Enable
dnl
AC_ARG_ENABLE(debug,            AC_HELP_STRING([--enable-debug],            [Enable full debug]),                         , enable_debug=no)
AC_ARG_ENABLE(gprof,            AC_HELP_STRING([--enable-gprof],            [Enable gprof log]),                          , enable_gprof=no)
AC_ARG_ENABLE(universal_binary, AC_HELP_STRING([--enable-universal_binary], [Create Mac PPC/i386 Universal binary]),      , enable_universal_binary)
AC_ARG_ENABLE(arch-ppc,         AC_HELP_STRING([--enable-arch-ppc],         [Create Mac PPC architecture]),               , enable_arch_ppc=no)
AC_ARG_ENABLE(arch-ppc64,       AC_HELP_STRING([--enable-arch-ppc64],       [Create Mac PPC64 architecture]),             , enable_arch_ppc64=no)
AC_ARG_ENABLE(arch-i386,        AC_HELP_STRING([--enable-arch-i386],        [Create Mac i386 architecture]),              , enable_arch_i386=no)
AC_ARG_ENABLE(arch-x86_64,      AC_HELP_STRING([--enable-arch-x86_64],      [Create Mac x86_64 architecture]),            , enable_arch_x86_64=no)
AC_ARG_ENABLE(unicode,          AC_HELP_STRING([--disable-unicode],         [Disable unicode]),                           , enable_unicode=yes)
AC_ARG_ENABLE(wstringmissing,   AC_HELP_STRING([--enable-wstringmissing],   [Disable wstring support (old compilers)]),   , enable_wstringmissing=no)
AC_ARG_ENABLE(large_files,      AC_HELP_STRING([--disable-large-files],     [Disable large files support]),               , enable_large_files=yes)
dnl AC_ARG_ENABLE(threads,          AC_HELP_STRING([--disable-threads],         [Disable threads support]),                   , enable_threads=yes)

dnl -------------------------------------------------------------------------
dnl With
dnl
AC_ARG_WITH(macosx-sdk,         AC_HELP_STRING([--with-macosx-sdk],         [Force the Mac SDK]),                         , with_macosx_sdk=no)
AC_ARG_WITH(macosx-version-min, AC_HELP_STRING([--with-macosx-version-min], [Force the Mac Version]),                     , with_macosx_version_min=no)
AC_ARG_WITH(wxwidgets,      AC_HELP_STRING([--with-wxwidgets],      [Use WxWidgets]),                           , with_wxwidgets=no)
AC_ARG_WITH(wx_gui,         AC_HELP_STRING([--with-wx-gui],         [Use WxWidgets with a GUI]),                , with_wx_gui=no)
AC_ARG_WITH(wx_static,      AC_HELP_STRING([--with-wx-static],      [Use WxWidgets static libs]),               , with_wx_static=no)

dnl -------------------------------------------------------------------------
dnl Third party
dnl
m4_include(wxwin.m4)
AM_OPTIONS_WXCONFIG

dnl #########################################################################
dnl ### Parsing options
dnl #########################################################################

dnl -------------------------------------------------------------------------
dnl WxWidgets (v2)
dnl

dnl Download and compile
WxBuiltInCompile()
{
	if test -e ../../../../Shared/Project/WxWidgets/Compile.sh; then
		cd ../../../../Shared/Project/WxWidgets
		chmod u+x Compile.sh
		./Compile.sh --enable-unicode=$enable_unicode --enable-static=yes --enable-gui=$with_wx_gui --host=$host
		cd $Home
		if test -e ../../../../Shared/Source/WxWidgets/Compile/wx-config; then
			if test "x$(../../../../Shared/Source/WxWidgets/Compile/wx-config --unicode=$enable_unicode --version)" = "x"; then
				with_wxwidgets="no"
			else
				dnl Compiled builtin lib found!
				with_wxwidgets="builtin"
			fi
		else
			dnl Problem while compiling
			with_wxwidgets="no"
		fi
	else
		with_wxwidgets="no"
	fi
}

dnl Search for a good build, else built it
WxBuiltInSearchGoodBuild()
{
	if test -e ../../../../Shared/Source/WxWidgets/Compile/wx-config; then
		dnl There is already a compiled version
		if test "x$(../../../../Shared/Source/WxWidgets/Compile/wx-config --unicode=$enable_unicode --static=$with_wx_static --version)" = "x"; then
			if test "x$(../../../../Shared/Source/WxWidgets/Compile/wx-config --unicode=$enable_unicode --version)" = "x"; then
				dnl Not well compiled, try to compile again
				WxBuiltInCompile
			else
				dnl Already compiled builtin lib found!
				with_wxwidgets="builtin"
			fi
		else
			dnl Already compiled builtin lib found!
			with_wxwidgets="builtin"
		fi
	else
		dnl Not yet compiled, try to compile
		WxBuiltInCompile
	fi
}

dnl The test
if test "$with_wxwidgets" = "yes"; then
    dnl Searching system libs
    AM_PATH_WXCONFIG(2.8.0, WxSystemLibs=1, WxSystemLibs=0, [base], --unicode=$enable_unicode --static=$with_wx_static)
    if test "$WxSystemLibs" != 1; then
        dnl Searching builtin libs
		WxBuiltInSearchGoodBuild
	else
        dnl System lib found!
		with_wxwidgets="system"
	fi
	
	if test "$with_wxwidgets" = "no"; then
		AC_MSG_ERROR([
			wxWidgets must be installed on your system.
	 
			Please check that wx-config is in path, the directory
			where wxWidgets libraries are installed (returned by
			'wx-config --libs' or 'wx-config --static --libs' command)
			is in LD_LIBRARY_PATH or equivalent variable and
			wxWidgets version is 2.8.0 or above, and the compilation 
			is compatible with 
			--unicode=$enable_unicode --static=$with_wx_static
			])
	fi
	if test "$with_wxwidgets" = "system"; then
		if test "${with_wx_config+set}" = set; then
			ZenLib_Wx_Config=$with_wx_config;
		else
			ZenLib_Wx_Config="wx-config"
		fi
	fi
	if test "$with_wxwidgets" = "builtin"; then
       ZenLib_Wx_Config="../../../../Shared/Source/WxWidgets/Compile/wx-config"
	fi
fi

dnl #########################################################################
dnl ### Compiler specific
dnl #########################################################################

dnl -------------------------------------------------------------------------
case "${host}" in
  powerpc-montavista* )
    CFLAGS="$CFLAGS -D__MONTAVISTA__"
    CXXFLAGS="$CXXFLAGS -D__MONTAVISTA__"
  ;;
esac

dnl -------------------------------------------------------------------------
dnl Mac Architecture and SDK
dnl
if test "$enable_universal_binary" = "yes"; then
	enable_arch_ppc=yes
	enable_arch_i386=yes
	if test "$with_macosx_sdk" = no ; then
		with_macosx_sdk="/Developer/SDKs/MacOSX10.4u.sdk"
	fi
	if test "$with_macosx_version_min" = no ; then
		with_macosx_version_min="10.4"
	fi
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
if test "$with_macosx_sdk" = "yes" ; then
	CXXFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk $CXXFLAGS"
	CFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk $CFLAGS"
	LDFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk $LDFLAGS"
elif test "x$with_macosx_sdk" != xno ; then
	CXXFLAGS="-isysroot $with_macosx_sdk $CXXFLAGS"
	CFLAGS="-isysroot $with_macosx_sdk $CFLAGS"
	LDFLAGS="-isysroot $with_macosx_sdk $LDFLAGS"
fi
if test "x$with_macosx_version_min" != xno ; then
	CXXFLAGS="-mmacosx-version-min=$with_macosx_version_min $CXXFLAGS"
	CFLAGS="-mmacosx-version-min=$with_macosx_version_min $CFLAGS"
	LDFLAGS="-mmacosx-version-min=$with_macosx_version_min $LDFLAGS"
fi

dnl #########################################################################
dnl ### ZenLib flags
dnl #########################################################################

dnl -------------------------------------------------------------------------
dnl WxWidgets
dnl
if test "$with_wxwidgets" != "no"; then
	CFLAGS="$CFLAGS $($ZenLib_Wx_Config --unicode=$enable_unicode --cflags)"
	CXXFLAGS="$CXXFLAGS $($ZenLib_Wx_Config --unicode=$enable_unicode --cxxflags)"
	ZenLib_LIBS="$ZenLib_LIBS $($ZenLib_Wx_Config --unicode=$enable_unicode --libs)"
	ZenLib_LIBS_Static="$ZenLib_LIBS_Static $($ZenLib_Wx_Config --unicode=$enable_unicode --libs)"
fi

dnl -------------------------------------------------------------------------
dnl Common
dnl
ZenLib_LIBS="-L$(pwd) -lzen -lpthread $ZenLib_LIBS"
ZenLib_LIBS_Static="$(pwd)/.libs/libzen.a -lpthread $ZenLib_LIBS_Static"

dnl #########################################################################
dnl ### C/C++ compiler options used to compile
dnl #########################################################################

dnl -------------------------------------------------------------------------
dnl wstring
dnl
if test "$enable_wstringmissing" = "yes"; then
	CXXFLAGS="$CXXFLAGS -DWSTRING_MISSING"
	ZenLib_CXXFLAGS="$ZenLib_CXXFLAGS -DWSTRING_MISSING"
	ZenLib_WstringMissing="yes"
	enable_unicode="no"
else
	ZenLib_WstringMissing="no"
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
dnl GNU compiler
dnl
if test "$GXX" = yes ; then
	CFLAGS="$CFLAGS -fPIC"
	CXXFLAGS="$CXXFLAGS -fPIC"
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

dnl #########################################################################
dnl ### Output
dnl #########################################################################

AC_SUBST(ZenLib_CXXFLAGS)
AC_SUBST(ZenLib_LIBS)
AC_SUBST(ZenLib_LIBS_Static)
AC_SUBST(ZenLib_Unicode)
AC_SUBST(ZenLib_WstringMissing)
AC_SUBST(ZenLib_LargeFiles)
AC_CONFIG_FILES(libzen-config, [chmod u+x libzen-config])
AC_CONFIG_FILES(libzen.pc)

AC_OUTPUT(Makefile)

dnl #########################################################################
dnl ### Report how we have been configured
dnl #########################################################################

echo ""
echo "Configured ZenLib for '$host'"
echo ""
echo "  Unicode?                                                $enable_unicode"
echo "  wstring missing support?                                $ZenLib_WstringMissing"
echo "  Large files support?                                    $enable_large_files"
echo "  Using WxWidgets?                                        $with_wxwidgets"
echo ""
echo "  Create static lib?                                      $enable_static"
echo "  Create shared lib?                                      $enable_shared"
echo ""
echo "  CXXFLAGS: $CXXFLAGS"
echo "  LIBS: $LIBS"
