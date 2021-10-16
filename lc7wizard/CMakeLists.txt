CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lc7wizard)

INCLUDE(../cmake/PCH.cmake)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)

SET(lc7wizard_SOURCES 
	src/stdafx.cpp
	src/linkage.cpp
	src/CLC7WizardPlugin.cpp
	src/CLC7WizardComponent.cpp
	src/CLC7Wizard.cpp
	src/CLC7WizardPage.cpp
	src/CLabelClickable.cpp
	src/CLabelCheckBox.cpp
	src/CLabelRadioButton.cpp
	src/CPageAuditType.cpp
	src/CPageIntro.cpp
	src/CPageReporting.cpp
	src/CPageScheduling.cpp
	src/CPageSummary.cpp
	src/CPageUnixImport.cpp
	src/CPageUnixImportFile.cpp
	src/CPageUnixImportSSH.cpp
	src/CPageWindowsImport.cpp
	src/CPageWindowsImportLocal.cpp
	src/CPageWindowsImportPWDump.cpp
	src/CPageWindowsImportRemoteSMB.cpp
	src/CPageWindowsOrUnix.cpp
)

SET(lc7wizard_FORMS 
	
)

SET(lc7wizard_RESOURCES
	resources/lc7wizard.qrc
)

SET(lc7wizard_HEADERS
	include/stdafx.h
	include/linkage.h
	include/uuids.h
	include/CLC7WizardPlugin.h
	include/CLC7WizardComponent.h
	include/CLC7Wizard.h
	include/CLC7WizardPage.h
	include/CLabelClickable.h
	include/CLabelCheckBox.h
	include/CLabelRadioButton.h
	include/CPageAuditType.h
	include/CPageIntro.h
	include/CPageReporting.h
	include/CPageScheduling.h
	include/CPageSummary.h
	include/CPageUnixImport.h
	include/CPageUnixImportFile.h
	include/CPageUnixImportSSH.h
	include/CPageWindowsImport.h
	include/CPageWindowsImportLocal.h
	include/CPageWindowsImportPWDump.h
	include/CPageWindowsImportRemoteSMB.h
	include/CPageWindowsOrUnix.h
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} "include" "../api/include")

QT5_WRAP_UI(lc7wizard_UI_HEADERS
    ${lc7wizard_FORMS}
)

QT5_ADD_RESOURCES(ResourceSources ${lc7wizard_RESOURCES})

ADD_LIBRARY(lc7wizard SHARED ${lc7wizard_SOURCES} ${lc7wizard_HEADERS} ${lc7wizard_UI_HEADERS} ${lc7wizard_RESOURCES} ${ResourceSources})
qt5_use_modules(lc7wizard Widgets)
#TARGET_LINK_LIBRARIES(lc7wizard)

ADD_PRECOMPILED_HEADER(lc7wizard stdafx lc7wizard_SOURCES)
