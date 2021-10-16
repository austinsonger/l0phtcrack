CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lc7reports)

INCLUDE(../cmake/PCH.cmake)
INCLUDE(../cmake/MakeDist.cmake)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5PrintSupport REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)

SET(lc7reports_SOURCES 
	src/stdafx.cpp
	src/CLC7ReportsPlugin.cpp
	src/CReports.cpp
	src/CReportsGUI.cpp
	src/linkage.cpp
	src/reportsconfig.cpp
)

SET(lc7reports_FORMS 
	resources/reportsconfig.ui
)

SET(lc7reports_RESOURCES
	resources/lc7reports.qrc
)

SET(lc7reports_HEADERS
	include/CLC7ReportsPlugin.h
	include/CReports.h
	include/CReportsGUI.h
	include/linkage.h
	include/reportsconfig.h
	include/stdafx.h
	include/uuids.h
)

#MESSAGE(lc7reports_SOURCES=${lc7reports_SOURCES})
#MESSAGE(lc7reports_FORMS=${lc7reports_FORMS})
#MESSAGE(lc7reports_HEADERS=${lc7reports_HEADERS})

QT5_WRAP_UI(lc7reports_UI_HEADERS
    ${lc7reports_FORMS}
)

QT5_ADD_RESOURCES(ResourceSources ${lc7reports_RESOURCES})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} "include" "../api/include" "../external/qtpropertybrowser/src")
ADD_LIBRARY(lc7reports SHARED ${lc7reports_SOURCES} ${lc7reports_HEADERS} ${lc7reports_UI_HEADERS} ${lc7reports_RESOURCES} ${ResourceSources})
qt5_use_modules(lc7reports Widgets PrintSupport)
TARGET_LINK_LIBRARIES(lc7reports qtpropertybrowser)

ADD_PRECOMPILED_HEADER(lc7reports stdafx lc7reports_SOURCES)

IF(MSVC)
	SET (TEMPLATEDIRS "${CMAKE_CURRENT_SOURCE_DIR}/../dist_lc7reports/${LC7_PLATFORM}/common" "${CMAKE_CURRENT_SOURCE_DIR}/../dist_lc7reports/${LC7_PLATFORM}/$<CONFIG>")
	SET (EXTRAS
	)
ENDIF()

LIST(APPEND TEMPLATEDIRS "${CMAKE_CURRENT_SOURCE_DIR}/../dist_lc7reports/common/common" "${CMAKE_CURRENT_SOURCE_DIR}/../dist_lc7reports/common/$<CONFIG>")

IF(DEFINED CMAKE_BUILD_TYPE)
	SET(LC7REPORTS_DIST "${CMAKE_CURRENT_BINARY_DIR}/../dist_lc7reports")
ELSE()
	SET(LC7REPORTS_DIST "${CMAKE_CURRENT_BINARY_DIR}/../dist_lc7reports/$<CONFIG>")
ENDIF()

MAKE_DIST(lc7reports "${LC7REPORTS_DIST}" TEMPLATEDIRS "" "" "" EXTRAS)

