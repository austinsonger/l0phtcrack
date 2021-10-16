CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lc7base)

INCLUDE(../cmake/PCH.cmake)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Help REQUIRED)

SET(lc7base_SOURCES 
	src/stdafx.cpp
	src/CHelpPage.cpp
	src/CLC7BasePlugin.cpp
	src/CQueuePage.cpp
	src/CReportsPage.cpp
	src/CSchedulePage.cpp
	src/CEditTaskDlg.cpp
	src/helpbrowser.cpp
	src/linkage.cpp
)

SET(lc7base_FORMS 
	resources/help.ui
	resources/queue.ui
	resources/reports.ui
	resources/schedule.ui
	resources/edittask.ui
)

SET(lc7base_RESOURCES
	resources/lc7base.qrc
)

SET(lc7base_HEADERS
	include/stdafx.h
	include/helpbrowser.h
	include/CHelpPage.h
	include/CLC7BasePlugin.h
	include/CQueuePage.h
	include/CReportsPage.h
	include/CSchedulePage.h
	include/CEditTaskDlg.h
	include/linkage.h
	include/uuids.h
)

#MESSAGE(lc7base_SOURCES=${lc7base_SOURCES})
#MESSAGE(lc7base_FORMS=${lc7base_FORMS})
#MESSAGE(lc7base_HEADERS=${lc7base_HEADERS})

QT5_WRAP_UI(lc7base_UI_HEADERS
    ${lc7base_FORMS}
)

QT5_ADD_RESOURCES(ResourceSources ${lc7base_RESOURCES})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} "include" "../api/include" "../external/qtpropertybrowser/src")
ADD_LIBRARY(lc7base SHARED ${lc7base_SOURCES} ${lc7base_HEADERS} ${lc7base_UI_HEADERS} ${lc7base_RESOURCES} ${ResourceSources} )
TARGET_LINK_LIBRARIES(lc7base qtpropertybrowser)

qt5_use_modules(lc7base Widgets Network Help)

ADD_PRECOMPILED_HEADER(lc7base stdafx lc7base_SOURCES)
