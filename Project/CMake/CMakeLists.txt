project(zenlib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../ZenLib/Source)

add_definitions(-DUNICODE)

set(zenlib_SRCS
	ZenLib/Conf.cpp
	ZenLib/CriticalSection.cpp
	ZenLib/Dir.cpp
	ZenLib/File.cpp
	ZenLib/FileName.cpp
	ZenLib/HTTP_Client.cpp
	ZenLib/InfoMap.cpp
	ZenLib/int128s.cpp
	ZenLib/int128u.cpp
	ZenLib/MemoryDebug.cpp
	ZenLib/OS_Utils.cpp
	ZenLib/Translation.cpp
	ZenLib/Thread.cpp
	ZenLib/Utils.cpp
	ZenLib/Ztring.cpp
	ZenLib/ZtringList.cpp
	ZenLib/ZtringListList.cpp
	ZenLib/ZtringListListF.cpp
	ZenLib/Format/Html/Html_Handler.cpp
	ZenLib/Format/Html/Html_Request.cpp
	ZenLib/Format/Http/Http_Cookies.cpp
	ZenLib/Format/Http/Http_Handler.cpp
	ZenLib/Format/Http/Http_Request.cpp
	ZenLib/Format/Http/Http_Utils.cpp
)

add_library(zenlib STATIC ${zenlib_SRCS})