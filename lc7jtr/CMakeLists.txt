CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lc7jtr)

INCLUDE(../cmake/PCH.cmake)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)

SET(lc7jtr_SOURCES 
	src/stdafx.cpp
	src/CLC7CharsetEditor.cpp
	src/CLC7CharsetEditorDlg.cpp
	src/CLC7ExecuteJTR.cpp
	src/CLC7JTRConsole.cpp
	src/CLC7JTR.cpp
	src/CLC7JTRGPUManager.cpp
	src/CLC7JTRPlugin.cpp
	src/CSystemJTR.cpp
	src/CTechniqueJTR.cpp
	src/CTechniqueJTRSingleGUI.cpp
	src/CTechniqueJTRBruteGUI.cpp
	src/CTechniqueJTRDictionaryGUI.cpp
	src/jtrnodeworker.cpp
	src/jtrworker.cpp
	src/linkage.cpp
	src/techniquebruteconfig.cpp
	src/techniquedictionaryconfig.cpp
	src/CLC7JTRDLL.cpp
	src/CLC7JTREXE.cpp
	src/CLC7JTRPasswordEngine.cpp
	src/turbo_linecount.cpp
)

SET(lc7jtr_FORMS 
	resources/techniquebruteconfig.ui
	resources/techniquedictionaryconfig.ui
	resources/charseteditor.ui
	resources/charseteditordlg.ui
	resources/jtrconsole.ui
)

SET(lc7jtr_RESOURCES
	resources/lc7jtr.qrc
)

SET(lc7jtr_HEADERS
	include/stdafx.h
	include/CLC7CharsetEditor.h
	include/CLC7CharsetEditorDlg.h
	include/CLC7ExecuteJTR.h
	include/CLC7JTRConsole.h
	include/CLC7JTR.h
	include/CLC7JTRGPUManager.h
	include/CLC7JTRPlugin.h
	include/CSystemJTR.h
	include/CTechniqueJTR.h
	include/CTechniqueJTRSingleGUI.h
	include/CTechniqueJTRBruteGUI.h
	include/CTechniqueJTRDictionaryGUI.h
	include/jtrnodeworker.h
	include/jtrworker.h
	include/jtrexe_crashdump.h
	include/linkage.h
	include/techniquebruteconfig.h
	include/techniquedictionaryconfig.h
	include/uuids.h
	include/CLC7JTRDLL.h
	include/CLC7JTREXE.h
	include/CLC7JTRPasswordEngine.h
	include/turbo_linecount.h
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} "include" "../api/include" "../external/jtrdll/jtrdll")

QT5_WRAP_UI(lc7jtr_UI_HEADERS
    ${lc7jtr_FORMS}
)

IF(MSVC)
	ADD_DEFINITIONS( -D_SCL_SECURE_NO_WARNINGS=1 )
ENDIF()

ADD_LIBRARY(lc7jtr SHARED ${lc7jtr_SOURCES} ${lc7jtr_HEADERS} ${lc7jtr_UI_HEADERS} )
#TARGET_LINK_LIBRARIES(lc7jtr)

qt5_use_modules(lc7jtr Widgets)

ADD_PRECOMPILED_HEADER(lc7jtr stdafx lc7jtr_SOURCES)
