CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lc7)

INCLUDE(../cmake/PCH.cmake)
INCLUDE(../cmake/FindQt5.cmake)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)

SET(lc7_SOURCES 
	src/stdafx.cpp
	src/CLC7ColorManager.cpp
	src/CLC7GUILinkage.cpp
	src/CLC7MenuWidget.cpp
	src/CLC7NavBar.cpp
	src/CLC7NavBarGroup.cpp
	src/CLC7NavBarHeader.cpp
	src/CLC7NavBarItem.cpp
	src/CLC7NavBarWidget.cpp
	src/CLC7ProgressBox.cpp
	src/CLC7StartupWidget.cpp
	src/CAnimatedBarChart.cpp
	src/CLC7WorkQueueWidget.cpp
	src/CLC7PresetWidget.cpp
	src/CLC7Updater.cpp
	src/CLC7UpdateDialog.cpp
	src/CLC7FileDownloader.cpp
	src/CLC7ModifiedListWidget.cpp
	src/CSystemOptions.cpp
	src/CSystemPage.cpp
	src/LC7Main.cpp
	src/main.cpp
	src/trdebug.cpp
	src/lc7app.cpp
	src/Attach.cpp
	src/CAttachServer.cpp
	src/CAttachClient.cpp
	src/CIOProcessor.cpp
	src/CLC7ExitWarningDlg.cpp
	src/CLC7AutosaveDlg.cpp
	src/CLC7LoginDlg.cpp
	resources/lc7.rc
)

SET(lc7_FORMS 
	resources/LC7Main.ui
	resources/LC7MenuWidget.ui
	resources/LC7ProgressDialog.ui
	resources/LC7Startup.ui
	resources/LC7WorkQueueWidget.ui
	resources/LC7ExitWarningDlg.ui
	resources/LC7UpdateDialog.ui
	resources/system.ui
	resources/presetwidget.ui
	resources/presetbuttons.ui
	resources/LC7AutosaveDlg.ui
	resources/LC7LoginDlg.ui
)

SET(lc7_RESOURCES
	resources/darkstyle.qrc
	resources/lc7.qrc
	resources/qtpropertymanager.qrc
)

SET(lc7_HEADERS
	include/stdafx.h
	include/appversion.h
	include/CLC7ColorManager.h
	include/CLC7GUILinkage.h
	include/CLC7MenuWidget.h
	include/CLC7NavBar.h
	include/CLC7NavBarGroup.h
	include/CLC7NavBarHeader.h
	include/CLC7NavBarItem.h
	include/CLC7NavBarWidget.h
	include/CLC7ProgressBox.h
	include/CLC7StartupWidget.h
	include/CAnimatedBarChart.h
	include/CLC7WorkQueueWidget.h
	include/CLC7PresetWidget.h
	include/CLC7Updater.h
	include/CLC7UpdateDialog.h
	include/CLC7FileDownloader.h
	include/CLC7ModifiedListWidget.h
	include/ILC7GUIComponent.h
	include/CSystemOptions.h
	include/CSystemPage.h
	include/LC7Main.h
	include/lc7app.h
	include/Attach.h
	include/CIOProcessor.h
	include/CAttachServer.h
	include/CAttachClient.h
	include/CLC7ExitWarningDlg.h
	include/CLC7AutosaveDlg.h
	include/CLC7LoginDlg.h
)

#MESSAGE(lc7_SOURCES=${lc7_SOURCES})
#MESSAGE(lc7_FORMS=${lc7_FORMS})
#MESSAGE(lc7_HEADERS=${lc7_HEADERS})


IF(MSVC)
	SET(CRASHREPORTERLIB CrashRpt)
	SET(CRASHREPORTERINCLUDE  "../external/crashrpt/include")
ELSE()
	SET(CRASHREPORTERLIB "")
	SET(CRASHREPORTERINCLUDE "")
ENDIF()


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} "include" "../api/include" "../external/qtsingleapplication/src" "../external/qtpropertybrowser/src" ${CRASHREPORTERINCLUDE})
LINK_DIRECTORIES(${CRASHREPORTERLIBDIR})

QT5_WRAP_UI(lc7_UI_HEADERS
    ${lc7_FORMS}
)

QT5_ADD_RESOURCES(ResourceSources ${lc7_RESOURCES})
ADD_DEFINITIONS(-DDISABLE_UPDATER)	# Remove this when updates are working again
ADD_EXECUTABLE(lc7 WIN32 ${lc7_SOURCES} ${lc7_HEADERS} ${lc7_UI_HEADERS} ${lc7_RESOURCES} ${ResourceSources})
TARGET_LINK_LIBRARIES(lc7 qtsingleapplication qtpropertybrowser qtpropertybrowser ${CRASHREPORTERLIB})

qt5_use_modules(lc7 Widgets)

ADD_PRECOMPILED_HEADER(lc7 stdafx lc7_SOURCES)

#IF(MSVC)
#	SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /level='requireAdministrator' /uiAccess='false' /MANIFEST /MANIFEST:EMBED" )
#ENDIF()
