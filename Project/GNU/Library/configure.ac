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
AC_ARG_ENABLE(unicode,      AC_HELP_STRING([--disable-unicode],     [Disable Unicode]),                         , enable_unicode=yes)
AC_ARG_ENABLE(large_files,  AC_HELP_STRING([--disable-large-files], [Disable Large files support]),             , enable_large_files=yes)
AC_ARG_ENABLE(debug,        AC_HELP_STRING([--enable-debug],        [Enable full debug]),                       , enable_debug=no)
AC_ARG_ENABLE(gprof,        AC_HELP_STRING([--enable-gprof],        [Enable gprof log]),                        , enable_gprof=no)

dnl -------------------------------------------------------------------------
dnl With
dnl
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

dnl #########################################################################
dnl ### ZenLib flags
dnl #########################################################################

dnl -------------------------------------------------------------------------
dnl WxWidgets
dnl
if test "$with_wxwidgets" != "no"; then
	CFLAGS="$CFLAGS $($ZenLib_Wx_Config --unicode=$enable_unicode --cflags)"
	CXXFLAGS="$CXXFLAGS $($ZenLib_Wx_Config --unicode=$enable_unicode --cxxflags)"
	LIBS="$LIBS $($ZenLib_Wx_Config --unicode=$enable_unicode --libs)"
	ZenLib_LIBS="$ZenLib_LIBS $($ZenLib_Wx_Config --unicode=$enable_unicode --libs)"
fi

dnl -------------------------------------------------------------------------
dnl Common
dnl
ZenLib_LIBS="-L$(pwd) -lzen $ZenLib_LIBS"

dnl #########################################################################
dnl ### C/C++ compiler options used to compile
dnl #########################################################################

dnl -------------------------------------------------------------------------
dnl Unicode
dnl
if test "$enable_unicode" = "yes"; then
	AC_DEFINE(UNICODE)
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
dnl Disable lib flags if we don't plan to create executables
dnl
if test "$enable_shared" != "yes"; then
	LIBS=""
fi

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

dnl #########################################################################
dnl ### Output
dnl #########################################################################

AC_SUBST(ZenLib_CXXFLAGS)
AC_SUBST(ZenLib_LIBS)
AC_SUBST(ZenLib_Unicode)
AC_SUBST(ZenLib_LargeFiles)
AC_CONFIG_FILES(ZenLib-config, [chmod u+x ZenLib-config])

AC_OUTPUT(Makefile)

dnl #########################################################################
dnl ### Report how we have been configured
dnl #########################################################################

echo ""
echo "Configured ZenLib for '$build'"
echo ""
echo "  Unicode?                                                $enable_unicode"
echo "  Large files support?                                    $enable_large_files"
echo "  Using WxWidgets?                                        $with_wxwidgets"
echo ""
echo "  Create static lib?                                      $enable_static"
echo "  Create shared lib?                                      $enable_shared"
echo ""
echo "  CXXFLAGS: $CXXFLAGS"
echo "  LIBS: $LIBS"