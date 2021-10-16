CMAKE_MINIMUM_REQUIRED(VERSION 3.10 FATAL_ERROR)

PROJECT(qtsingleapplication)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)

SET(qtsingleapplication_SOURCES 
	src/qtlocalpeer.cpp
	#src/qtlockedfile.cpp
	#src/qtlockedfile_unix.cpp
	#src/qtlockedfile_win.cpp
	src/qtsingleapplication.cpp
	#src/qtsinglecoreapplication.cpp
)

SET(qtsingleapplication_HEADERS
	src/qtlocalpeer.h
	#src/qtlockedfile.h
	src/qtsingleapplication.h
	#src/qtsinglecoreapplication.h
)

#MESSAGE(qtsingleapplication_SOURCES=${qtsingleapplication_SOURCES})
#MESSAGE(qtsingleapplication_HEADERS=${qtsingleapplication_HEADERS})

ADD_DEFINITIONS(-DQT_QTSINGLEAPPLICATION_EXPORT=1 -DUNICODE=1)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} "src")
ADD_LIBRARY(qtsingleapplication SHARED ${qtsingleapplication_SOURCES} ${qtsingleapplication_HEADERS} )
qt5_use_modules(qtsingleapplication Widgets Network)
#TARGET_LINK_LIBRARIES(qtsingleapplication)
