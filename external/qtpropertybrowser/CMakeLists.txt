CMAKE_MINIMUM_REQUIRED(VERSION 3.10 FATAL_ERROR)

PROJECT(qtpropertybrowser)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)

SET(qtpropertybrowser_SOURCES 
	src/qtbuttonpropertybrowser.cpp
	src/qteditorfactory.cpp
	src/qtgroupboxpropertybrowser.cpp
	src/qtpropertybrowser.cpp
	src/qtpropertybrowserutils.cpp
	src/qtpropertymanager.cpp
	src/qttreepropertybrowser.cpp
	src/qtvariantproperty.cpp
)

SET(qtpropertybrowser_RESOURCES
	src/qtpropertybrowser.qrc
)

SET(qtpropertybrowser_HEADERS
	src/qtbuttonpropertybrowser.h
	src/qteditorfactory.h
	src/qtgroupboxpropertybrowser.h
	src/qtpropertybrowser.h
	src/qtpropertybrowserutils_p.h
	src/qtpropertymanager.h
	src/qttreepropertybrowser.h
	src/qtvariantproperty.h
)

#MESSAGE(qtpropertybrowser_SOURCES=${qtpropertybrowser_SOURCES})
#MESSAGE(qtpropertybrowser_HEADERS=${qtpropertybrowser_HEADERS})

QT5_ADD_RESOURCES(ResourceSources ${qtpropertybrowser_RESOURCES})

ADD_DEFINITIONS(-DQT_DISABLE_DEPRECATED_BEFORE=0 -DQT_QTPROPERTYBROWSER_EXPORT=1)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} "src")
ADD_LIBRARY(qtpropertybrowser SHARED ${qtpropertybrowser_SOURCES} ${qtpropertybrowser_HEADERS}  ${qtpropertybrowser_RESOURCES} ${ResourceSources})
qt5_use_modules(qtpropertybrowser Widgets)
# TARGET_LINK_LIBRARIES(qtpropertybrowser)
