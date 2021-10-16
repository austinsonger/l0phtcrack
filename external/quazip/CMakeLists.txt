CMAKE_MINIMUM_REQUIRED(VERSION 3.10 FATAL_ERROR)

PROJECT(quazip)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Core REQUIRED)

SET(quazip_SOURCES 
	JlCompress.cpp
	qioapi.cpp
	quaadler32.cpp
	quacrc32.cpp
	quagzipfile.cpp
	quaziodevice.cpp
	quazip.cpp
	quazipdir.cpp
	quazipfile.cpp
	quazipfileinfo.cpp
	quazipnewinfo.cpp
	zip.c
	unzip.c
	zlib/adler32.c
	zlib/compress.c
	zlib/crc32.c
	zlib/deflate.c
	zlib/gzclose.c
	zlib/gzlib.c
	zlib/gzread.c
	zlib/gzwrite.c
	zlib/infback.c
	zlib/inffast.c
	zlib/inflate.c
	zlib/inftrees.c
	zlib/trees.c
	zlib/uncompr.c
	zlib/zutil.c
)

SET(quazip_HEADERS
	crypt.h
	ioapi.h
	JlCompress.h
	quaadler32.h
	quachecksum32.h
	quacrc32.h
	quagzipfile.h
	quaziodevice.h
	quazip.h
	quazipdir.h
	quazipfile.h
	quazipfileinfo.h
	quazipnewinfo.h
	quazip_global.h
	unzip.h
	zip.h
	zlib/crc32.h
	zlib/deflate.h
	zlib/gzguts.h
	zlib/inffast.h
	zlib/inffixed.h
	zlib/inflate.h
	zlib/inftrees.h
	zlib/trees.h
	zlib/zconf.h
	zlib/zlib.h
	zlib/zutil.h
)

ADD_DEFINITIONS(-DQUAZIP_BUILD=1 -DUNICODE=1)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
ADD_LIBRARY(quazip SHARED ${quazip_SOURCES} ${quazip_HEADERS} )
qt5_use_modules(quazip Widgets)

#TARGET_LINK_LIBRARIES(quazip)
