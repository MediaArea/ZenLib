AUTOMAKE_OPTIONS = foreign subdir-objects

lib_LTLIBRARIES = libzen.la
libzen_la_SOURCES = \
                       ../../../Source/ZenLib/Conf.cpp \
                       ../../../Source/ZenLib/CriticalSection.cpp \
                       ../../../Source/ZenLib/Dir.cpp \
                       ../../../Source/ZenLib/File.cpp \
                       ../../../Source/ZenLib/FileName.cpp \
                       ../../../Source/ZenLib/InfoMap.cpp \
                       ../../../Source/ZenLib/int128s.cpp \
                       ../../../Source/ZenLib/int128u.cpp \
                       ../../../Source/ZenLib/MemoryDebug.cpp \
                       ../../../Source/ZenLib/OS_Utils.cpp \
                       ../../../Source/ZenLib/Translation.cpp \
                       ../../../Source/ZenLib/Thread.cpp \
                       ../../../Source/ZenLib/Utils.cpp \
                       ../../../Source/ZenLib/Ztring.cpp \
                       ../../../Source/ZenLib/ZtringList.cpp \
                       ../../../Source/ZenLib/ZtringListList.cpp \
                       ../../../Source/ZenLib/ZtringListListF.cpp \
                       ../../../Source/ZenLib/Format/Html/Html_Handler.cpp \
                       ../../../Source/ZenLib/Format/Html/Html_Request.cpp \
                       ../../../Source/ZenLib/Format/Http/Http_Cookies.cpp \
                       ../../../Source/ZenLib/Format/Http/Http_Handler.cpp \
                       ../../../Source/ZenLib/Format/Http/Http_Request.cpp \
                       ../../../Source/ZenLib/Format/Http/Http_Utils.cpp

libzen_la_LDFLAGS     = -no-undefined -version-info 0:0:0

zenincludedir = $(includedir)/ZenLib
zeninclude_HEADERS = \
 ../../../Source/ZenLib/BitStream.h \
 ../../../Source/ZenLib/BitStream_Fast.h \
 ../../../Source/ZenLib/BitStream_LE.h \
 ../../../Source/ZenLib/Conf.h \
 ../../../Source/ZenLib/Conf_Internal.h \
 ../../../Source/ZenLib/CriticalSection.h \
 ../../../Source/ZenLib/Dir.h \
 ../../../Source/ZenLib/File.h \
 ../../../Source/ZenLib/FileName.h \
 ../../../Source/ZenLib/HTTP_Client.h \
 ../../../Source/ZenLib/InfoMap.h \
 ../../../Source/ZenLib/int128s.h \
 ../../../Source/ZenLib/int128u.h \
 ../../../Source/ZenLib/MemoryDebug.h \
 ../../../Source/ZenLib/OS_Utils.h \
 ../../../Source/ZenLib/PreComp.h \
 ../../../Source/ZenLib/Thread.h \
 ../../../Source/ZenLib/Trace.h \
 ../../../Source/ZenLib/Translation.h \
 ../../../Source/ZenLib/Utils.h \
 ../../../Source/ZenLib/Ztring.h \
 ../../../Source/ZenLib/ZtringList.h \
 ../../../Source/ZenLib/ZtringListListF.h \
 ../../../Source/ZenLib/ZtringListList.h

zen_httpclientdir = $(zenincludedir)/HTTP_Client
zen_httpclient_HEADERS = \
 ../../../Source/ZenLib/HTTP_Client/HTTPClient.h \
 ../../../Source/ZenLib/HTTP_Client/HTTPClientCommon.h \
 ../../../Source/ZenLib/HTTP_Client/HTTPClientString.h \
 ../../../Source/ZenLib/HTTP_Client/HTTPClientAuth.h \
 ../../../Source/ZenLib/HTTP_Client/HTTPClientWrapper.h

zen_format_htmldir = $(zenincludedir)/Format/Html
zen_format_html_HEADERS = \
 ../../../Source/ZenLib/Format/Html/Html_Handler.h \
 ../../../Source/ZenLib/Format/Html/Html_Request.h

zen_format_httpdir = $(zenincludedir)/Format/Http
zen_format_http_HEADERS = \
 ../../../Source/ZenLib/Format/Http/Http_Utils.h \
 ../../../Source/ZenLib/Format/Http/Http_Request.h \
 ../../../Source/ZenLib/Format/Http/Http_Handler.h \
 ../../../Source/ZenLib/Format/Http/Http_Cookies.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libzen.pc

AM_TESTS_FD_REDIRECT = 9>&2

TESTS = \
 test/filename.sh \
 Tests/zen_bitstream

check_PROGRAMS = \
 test/zen_filename \
 Tests/zen_bitstream
test_zen_filename_SOURCES = test/Source/filename.cpp
test_zen_filename_CXXFLAGS = $(AM_CXXFLAGS) -Wall -Wextra
test_zen_filename_LDADD = libzen.la

Tests_zen_bitstream_SOURCES = Tests/BitStream.cpp
Tests_zen_bitstream_CXXFLAGS = $(AM_CXXFLAGS) -Wall -Wextra
Tests_zen_bitstream_LDADD = libzen.la

AM_CXXFLAGS = -std=c++11
AM_CPPFLAGS = -I../../../Source
