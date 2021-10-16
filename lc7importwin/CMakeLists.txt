CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(lc7importwin)

INCLUDE(../cmake/PCH.cmake)
IF(MSVC)
	INCLUDE(../cmake/IDL.cmake)
ENDIF()

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(OpenSSL REQUIRED)
FIND_PACKAGE(libssh2 REQUIRED)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	SET(PLATFORM_LIBS Shlwapi.lib netapi32.lib crypt32.lib esent.lib rpcrt4.lib Secur32.lib Activeds.lib Adsiid.lib ws2_32.lib)
	SET(PLATFORM_INCLUDE_DIRS "")
	SET(PLATFORM_LIBRARY_DIRS "")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	SET(PLATFORM_LIBS "")
	SET(PLATFORM_INCLUDE_DIRS "")
	SET(PLATFORM_LIBRARY_DIRS "")
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	FIND_LIBRARY(LIBWINE "wine.1.0" /usr/local/lib)

	SET(PLATFORM_LIBS ${LIBWINE})
#	SET(PLATFORM_DEFINITIONS "-D_WIN32=1 -DWIN32 -fshort-wchar")
	SET(PLATFORM_INCLUDE_DIRS "/usr/local/include" "/usr/local/include/wine/windows")
	SET(PLATFORM_LIBRARY_DIRS "")
ENDIF()

SET(lc7importwin_SOURCES 
	src/stdafx.cpp
	src/CImportWindows.cpp
	src/PWDumpImporter.cpp
	src/SAMImporter.cpp
	src/NTDSImporter.cpp
	src/DRSRImporter.cpp
	src/CImportPWDump.cpp
	src/CImportPWDumpGUI.cpp
	src/CImportSAM.cpp
	src/CImportSAMGUI.cpp
	src/CImportNTDS.cpp
	src/CImportNTDSGUI.cpp
	src/CImportWindowsLocal.cpp
	src/CImportWindowsLocalGUI.cpp
	src/CImportWindowsRemote.cpp
	src/CImportWindowsRemoteGUI.cpp
	src/CLC7ImportWinPlugin.cpp
	src/importpwdumpconfig.cpp
	src/importsamconfig.cpp
	src/importntdsconfig.cpp
	src/importlocalconfig.cpp
	src/importremoteconfig.cpp
	src/linkage.cpp
	src/slow_des.cpp
	src/slow_md5.cpp
	src/slow_rc4.cpp
	src/ntreg.cpp
	src/ntds.cpp
	src/crypt.cpp
	src/utils.cpp
	src/midl_alloc.cpp
	src/drsr_addons.cpp
	src/drsr_alloc.cpp
	src/windows_abstraction.cpp
	src/GenerateRemoteAgentDlg.cpp
	src/CWindowsImportSettings.cpp
	../win-agent/lc7dump/PubKeyFile.cpp
)

SET(lc7importwin_FORMS 
	resources/importlocalconfig.ui
	resources/importremoteconfig.ui
	resources/importpwdumpconfig.ui
	resources/importsamconfig.ui
	resources/importntdsconfig.ui
	resources/generateremoteagent.ui
)

#SET(lc7importwin_RESOURCES
#	resources/lc7importwin.qrc
#)

set(lc7importwin_IDL
	${CMAKE_CURRENT_SOURCE_DIR}/idl/drsr.idl
)

SET(lc7importwin_HEADERS
	include/CImportWindows.h
	include/PWDumpImporter.h
	include/SAMImporter.h
	include/NTDSImporter.h
	include/DRSRImporter.h
	include/CImportPWDump.h
	include/CImportPWDumpGUI.h
	include/CImportSAM.h
	include/CImportSAMGUI.h
	include/CImportNTDS.h
	include/CImportNTDSGUI.h
	include/CImportWindowsLocal.h
	include/CImportWindowsLocalGUI.h
	include/CImportWindowsRemote.h
	include/CImportWindowsRemoteGUI.h
	include/CLC7ImportWinPlugin.h
	include/importlocalconfig.h
	include/importpwdumpconfig.h
	include/importsamconfig.h
	include/importntdsconfig.h
	include/importremoteconfig.h
	include/linkage.h
	include/stdafx.h
	include/uuids.h
	include/slow_md5.h
	include/slow_des.h
	include/slow_rc4.h
	include/ntreg.h
	include/ntds.h
	include/utils.h
	include/crypt.h
	include/midl_alloc.h
	include/drsr_addons.h
	include/drsr_alloc.h
	include/windows_abstraction.h
	include/GenerateRemoteAgentDlg.h
	include/CWindowsImportSettings.h
	../win-agent/lc7dump/PubKeyFile.h
)

#MESSAGE(lc7importwin_SOURCES=${lc7importwin_SOURCES})
#MESSAGE(lc7importwin_FORMS=${lc7importwin_FORMS})
#MESSAGE(lc7importwin_HEADERS=${lc7importwin_HEADERS})

QT5_WRAP_UI(lc7importwin_UI_HEADERS
    ${lc7importwin_FORMS}
)

QT5_ADD_RESOURCES(ResourceSources ${lc7importwin_RESOURCES})

ADD_DEFINITIONS(-DUNICODE=1 ${PLATFORM_DEFINITIONS})

LINK_DIRECTORIES( 
	${PLATFORM_LIBRARY_DIRS}
)

INCLUDE_DIRECTORIES("include" "idl" "${CMAKE_CURRENT_BINARY_DIR}" "../api/include" "../win-agent/lc7dump" ${OPENSSL_INCLUDE_DIR} ${PLATFORM_INCLUDE_DIRS})


# Build idl files

MACRO_ADD_INTERFACES(GENERATED_FILES_IDL ${lc7importwin_IDL})

SOURCE_GROUP("Source Files" FILES ${lc7importwin_SOURCES})
SOURCE_GROUP("Header Files" FILES ${lc7importwin_HEADERS})
SOURCE_GROUP("IDL" FILES ${GENERATED_FILES_IDL} ${lc7importwin_IDL})

################

ADD_LIBRARY(lc7importwin SHARED 
	${lc7importwin_SOURCES} ${lc7importwin_HEADERS} 
	${lc7importwin_UI_HEADERS} 
	${lc7importwin_IDL} ${GENERATED_FILES_IDL}
	${lc7importwin_RESOURCES}
	${ResourceSources}
)

TARGET_LINK_LIBRARIES(lc7importwin ${OPENSSL_LIBRARIES} ${PLATFORM_LIBS} )

qt5_use_modules(lc7importwin Widgets)

ADD_PRECOMPILED_HEADER(lc7importwin stdafx lc7importwin_SOURCES)

