# ZenLib w/o HTTP Client (see end of file to enable)
# in order to build with qmake just copy this file to the sources
# then open with QtCreator and build
TEMPLATE = lib
CONFIG += staticlib
TARGET = zen
INCLUDEPATH += . .. HTTP_Client

QMAKE_CXXFLAGS += -D_UNICODE

# surpressing way too many warnings here, with a heavy sigh
# these should be looked at one group at a time
QMAKE_CFLAGS_WARN_ON -= -Wall
QMAKE_CXXFLAGS_WARN_ON -= -Wall

!win32-msvc* {
QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter \
-Wno-ignored-qualifiers \
-Wno-missing-braces \
-Wno-parentheses
}

INCLUDEPATH      += \
        ../../Source

# Input
HEADERS += \
        ../../Source/ZenLib/BitStream.h \
        ../../Source/ZenLib/BitStream_Fast.h \
        ../../Source/ZenLib/BitStream_LE.h \
        ../../Source/ZenLib/Conf.h \
        ../../Source/ZenLib/Conf_Internal.h \
        ../../Source/ZenLib/CriticalSection.h \
        ../../Source/ZenLib/Dir.h \
        ../../Source/ZenLib/File.h \
        ../../Source/ZenLib/FileName.h \
        ../../Source/ZenLib/Format/Html/Html_Handler.h \
        ../../Source/ZenLib/Format/Html/Html_Request.h \
        ../../Source/ZenLib/Format/Http/Http_Cookies.h \
        ../../Source/ZenLib/Format/Http/Http_Handler.h \
        ../../Source/ZenLib/Format/Http/Http_Request.h \
        ../../Source/ZenLib/Format/Http/Http_Utils.h \
        ../../Source/ZenLib/InfoMap.h \
        ../../Source/ZenLib/int128s.h \
        ../../Source/ZenLib/int128u.h \
        ../../Source/ZenLib/MemoryDebug.h \
        ../../Source/ZenLib/MemoryUtils.h \
        ../../Source/ZenLib/OS_Utils.h \
        ../../Source/ZenLib/PreComp.h \
        ../../Source/ZenLib/Thread.h \
        ../../Source/ZenLib/Trace.h \
        ../../Source/ZenLib/Translation.h \
        ../../Source/ZenLib/Utils.h \
        ../../Source/ZenLib/Ztring.h \
        ../../Source/ZenLib/ZtringList.h \
        ../../Source/ZenLib/ZtringListList.h \
        ../../Source/ZenLib/ZtringListListF.h

SOURCES += \
        ../../Source/ZenLib/Conf.cpp \
        ../../Source/ZenLib/CriticalSection.cpp \
        ../../Source/ZenLib/Dir.cpp \
        ../../Source/ZenLib/File.cpp \
        ../../Source/ZenLib/FileName.cpp \
        ../../Source/ZenLib/Format/Html/Html_Handler.cpp \
        ../../Source/ZenLib/Format/Html/Html_Request.cpp \
        ../../Source/ZenLib/Format/Http/Http_Cookies.cpp \
        ../../Source/ZenLib/Format/Http/Http_Handler.cpp \
        ../../Source/ZenLib/Format/Http/Http_Request.cpp \
        ../../Source/ZenLib/Format/Http/Http_Utils.cpp \
        ../../Source/ZenLib/HTTP_Client.cpp \
        ../../Source/ZenLib/InfoMap.cpp \
        ../../Source/ZenLib/int128s.cpp \
        ../../Source/ZenLib/int128u.cpp \
        ../../Source/ZenLib/MemoryDebug.cpp \
        ../../Source/ZenLib/OS_Utils.cpp \
        ../../Source/ZenLib/PreComp.cpp \
        ../../Source/ZenLib/Thread.cpp \
        ../../Source/ZenLib/Translation.cpp \
        ../../Source/ZenLib/Utils.cpp \
        ../../Source/ZenLib/Ztring.cpp \
        ../../Source/ZenLib/ZtringList.cpp \
        ../../Source/ZenLib/ZtringListList.cpp \
        ../../Source/ZenLib/ZtringListListF.cpp

# not sure whats up with this guy
#SOURCES += \
#        ../../Source/ZenLib/HTTP_Client/HTTPClient.c \
#        ../../Source/ZenLib/HTTP_Client/HTTPClientAuth.c \
#        ../../Source/ZenLib/HTTP_Client/HTTPClientString.c \
#        ../../Source/ZenLib/HTTP_Client/HTTPClientWrapper.c
#HEADERS += \
#        ../../Source/ZenLib/HTTP_Client/HTTPClient.h \
#        ../../Source/ZenLib/HTTP_Client/HTTPClientAuth.h \
#        ../../Source/ZenLib/HTTP_Client/HTTPClientCommon.h \
#        ../../Source/ZenLib/HTTP_Client/HTTPClientString.h \
#        ../../Source/ZenLib/HTTP_Client/HTTPClientWrapper.h \
#        ../../Source/ZenLib/HTTP_Client.h
