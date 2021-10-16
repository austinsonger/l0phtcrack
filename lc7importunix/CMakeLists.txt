CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lc7importunix)

INCLUDE(../cmake/PCH.cmake)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	SET(PLATFORM_LIBS Shlwapi.lib netapi32.lib crypt32.lib ws2_32.lib)
	SET(PLATFORM_INCLUDE_DIRS "")
	SET(PLATFORM_LIBRARY_DIRS "")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	SET(PLATFORM_LIBS "")
	SET(PLATFORM_INCLUDE_DIRS "")
	SET(PLATFORM_LIBRARY_DIRS "")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	SET(PLATFORM_LIBS "")
	SET(PLATFORM_DEFINITIONS "")
	SET(PLATFORM_INCLUDE_DIRS "/usr/local/include")
	SET(PLATFORM_LIBRARY_DIRS "")
ENDIF()


SET(lc7importunix_SOURCES 
	src/stdafx.cpp
	src/ShadowImporter.cpp
	src/UnixSSHImporter.cpp
	src/CImportShadow.cpp
	src/CImportShadowGUI.cpp
	src/CImportUnixSSH.cpp
	src/CImportUnixSSHGUI.cpp
	src/CLC7ImportUnixPlugin.cpp
	src/importshadowconfig.cpp
	src/importunixsshconfig.cpp
	src/linkage.cpp
	src/CLC7UnixImporter.cpp
	src/CLC7UnixImporter_OLDPASSWD.cpp
	src/CLC7UnixImporter_LINUX.cpp
	src/CLC7UnixImporter_BSD.cpp
	src/CLC7UnixImporter_SOLARIS.cpp
	src/CLC7UnixImporter_AIX.cpp
	
)

SET(lc7importunix_FORMS 
	resources/importshadowconfig.ui
	resources/importunixsshconfig.ui
)

#SET(lc7importunix_RESOURCES
#	resources/lc7importunix.qrc
#)

SET(lc7importunix_HEADERS
	include/ShadowImporter.h
	include/UnixSSHImporter.h
	include/CImportShadow.h
	include/CImportShadowGUI.h
	include/CImportUnixSSH.h
	include/CImportUnixSSHGUI.h
	include/CLC7ImportUnixPlugin.h
	include/importshadowconfig.h
	include/importunixsshconfig.h
	include/linkage.h
	include/stdafx.h
	include/uuids.h
	include/ILC7UnixImporter.h
	include/CLC7UnixImporter.h
	include/CLC7UnixImporter_OLDPASSWD.h
	include/CLC7UnixImporter_LINUX.h
	include/CLC7UnixImporter_BSD.h
	include/CLC7UnixImporter_SOLARIS.h
	include/CLC7UnixImporter_AIX.h
)

#MESSAGE(lc7importunix_SOURCES=${lc7importunix_SOURCES})
#MESSAGE(lc7importunix_FORMS=${lc7importunix_FORMS})
#MESSAGE(lc7importunix_HEADERS=${lc7importunix_HEADERS})

QT5_WRAP_UI(lc7importunix_UI_HEADERS
    ${lc7importunix_FORMS}
)

QT5_ADD_RESOURCES(ResourceSources ${lc7importunix_RESOURCES})

ADD_DEFINITIONS(-DUNICODE=1 ${PLATFORM_DEFINITIONS})

LINK_DIRECTORIES( 
	${PLATFORM_LIBRARY_DIRS}
)

INCLUDE_DIRECTORIES("include" "../api/include" "../external/libssh2/include" ${PLATFORM_INCLUDE_DIRS})
ADD_LIBRARY(lc7importunix SHARED ${lc7importunix_SOURCES} ${lc7importunix_HEADERS} ${lc7importunix_UI_HEADERS} ${lc7importunix_RESOURCES} ${ResourceSources})
TARGET_LINK_LIBRARIES(lc7importunix ${PLATFORM_LIBS} libssh2)

qt5_use_modules(lc7importunix Widgets)

ADD_PRECOMPILED_HEADER(lc7importunix stdafx lc7importunix_SOURCES)
