# ZenLib w/o HTTP Client (see end of file to enable)
# in order to build with qmake just copy this file to the sources
# then open with QtCreator and build
TEMPLATE = lib
CONFIG += staticlib c++11
TARGET = ZenLib
INCLUDEPATH += . .. HTTP_Client
DESTDIR = ../Library

QMAKE_CXXFLAGS += -D_UNICODE

# surpressing way too many warnings here, with a heavy sigh
# these should be looked at one group at a time
QMAKE_CFLAGS_WARN_ON -= -Wall
QMAKE_CXXFLAGS_WARN_ON -= -Wall

!win32-msvc* {
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter \
-Wno-unused-private-field \
-Wno-unused-const-variable \
-Wno-ignored-qualifiers \
-Wno-missing-braces \
-Wno-pointer-sign \
-Wno-invalid-source-encoding \
-Wno-pointer-sign \
-Wno-parentheses
}

# Input
HEADERS += \
        BitStream.h \
        BitStream_Fast.h \
        BitStream_LE.h \
        Conf.h \
        Conf_Internal.h \
        CriticalSection.h \
        Dir.h \
        File.h \
        FileName.h \
        Format/Html/Html_Handler.h \
        Format/Html/Html_Request.h \
        Format/Http/Http_Cookies.h \
        Format/Http/Http_Handler.h \
        Format/Http/Http_Request.h \
        Format/Http/Http_Utils.h \
        InfoMap.h \
        int128s.h \
        int128u.h \
        MemoryDebug.h \
        MemoryUtils.h \
        OS_Utils.h \
        PreComp.h \
        Thread.h \
        Trace.h \
        Translation.h \
        Utils.h \
        Ztring.h \
        ZtringList.h \
        ZtringListList.h \
        ZtringListListF.h

SOURCES += \
        Conf.cpp \
        CriticalSection.cpp \
        Dir.cpp \
        File.cpp \
        FileName.cpp \
        Format/Html/Html_Handler.cpp \
        Format/Html/Html_Request.cpp \
        Format/Http/Http_Cookies.cpp \
        Format/Http/Http_Handler.cpp \
        Format/Http/Http_Request.cpp \
        Format/Http/Http_Utils.cpp \
        HTTP_Client.cpp \
        InfoMap.cpp \
        int128s.cpp \
        int128u.cpp \
        MemoryDebug.cpp \
        OS_Utils.cpp \
        PreComp.cpp \
        Thread.cpp \
        Translation.cpp \
        Utils.cpp \
        Ztring.cpp \
        ZtringList.cpp \
        ZtringListList.cpp \
        ZtringListListF.cpp

# not sure whats up with this guy
#SOURCES += \
#        HTTP_Client/HTTPClient.c \
#        HTTP_Client/HTTPClientAuth.c \
#        HTTP_Client/HTTPClientString.c \
#        HTTP_Client/HTTPClientWrapper.c
#HEADERS += \
#        HTTP_Client/HTTPClient.h \
#        HTTP_Client/HTTPClientAuth.h \
#        HTTP_Client/HTTPClientCommon.h \
#        HTTP_Client/HTTPClientString.h \
#        HTTP_Client/HTTPClientWrapper.h \
#        HTTP_Client.h
