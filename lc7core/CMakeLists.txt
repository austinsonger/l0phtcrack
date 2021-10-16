CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(lc7core)

INCLUDE(../cmake/PCH.cmake)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Sql REQUIRED)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
		SET(NVAPI_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/nvapi/amd64")
		SET(NVAPI_LIB "nvapi64.lib")
	ELSE()
		SET(NVAPI_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/nvapi/x86")
		SET(NVAPI_LIB "nvapi.lib")
	ENDIF()
	SET(PLATFORM_LIBS Shlwapi.lib netapi32.lib crypt32.lib secur32.lib mstask.lib pdh.lib PowrProf.lib ${NVAPI_LIB})
	SET(PLATFORM_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../external/adl_sdk10/include"  "${CMAKE_CURRENT_SOURCE_DIR}/../external/nvapi")
	SET(PLATFORM_LIBRARY_DIRS "${NVAPI_LIBRARY_DIR}" )
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	SET(PLATFORM_LIBS )
	SET(PLATFORM_INCLUDE_DIRS )
	SET(PLATFORM_LIBRARY_DIRS )
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	SET(PLATFORM_LIBS )
#	SET(PLATFORM_DEFINITIONS "-D_WIN32=1 -DWIN32 -fshort-wchar")
	SET(PLATFORM_INCLUDE_DIRS )
	SET(PLATFORM_LIBRARY_DIRS )
ENDIF()

SET(lc7core_SOURCES 
	src/stdafx.cpp
	src/CLC7Action.cpp
	src/CLC7ActionCategory.cpp
	src/CLC7CommandControl.cpp
	src/CLC7Controller.cpp
	src/CLC7HistoricalData.cpp
	src/CLC7Linkage.cpp
	src/CLC7PluginLibrary.cpp
	src/CLC7PluginRegistry.cpp
	src/CLC7Settings.cpp
	src/CLC7WorkQueue.cpp
	src/CLC7WorkQueueFactory.cpp
	src/CLC7Task.cpp
	src/CLC7TaskScheduler.cpp
	src/CLC7ThermalWatchdog.cpp
	src/CLC7CoreSettings.cpp
	src/CLC7Preset.cpp
	src/CLC7PresetGroup.cpp
	src/CLC7PresetManager.cpp
	src/CLC7CPUInformation.cpp
	src/CLC7SecureStringSerializer.cpp
	src/CLC7SystemMonitor.cpp
)

SET(lc7core_HEADERS
	include/stdafx.h
	include/CLC7Action.h
	include/CLC7ActionCategory.h
	include/CLC7CommandControl.h
	include/CLC7Controller.h
	include/CLC7CoreSettings.h
	include/CLC7CPUInformation.h
	include/CLC7HistoricalData.h
	include/CLC7Linkage.h
	include/CLC7PluginLibrary.h
	include/CLC7PluginRegistry.h
	include/CLC7Settings.h
	include/CLC7SystemMonitor.h
	include/CLC7ThermalWatchdog.h
	include/CLC7Task.h
	include/CLC7TaskScheduler.h
	include/CLC7WorkQueue.h
	include/CLC7WorkQueueFactory.h
	include/CLC7Preset.h
	include/CLC7PresetGroup.h
	include/CLC7PresetManager.h
	include/CLC7SecureStringSerializer.h
	include/ILC7Controller.h
	include/ITaskScheduler_BASE.h
	include/platform_specific.h
)


IF(MSVC)
	LIST(APPEND lc7core_SOURCES
		src/CTaskScheduler_WIN32v1.cpp
		src/CTaskScheduler_WIN32v2.cpp
	)
	LIST(APPEND lc7core_HEADERS 
		include/CTaskScheduler_WIN32v1.h
		include/CTaskScheduler_WIN32v2.h
	)
ENDIF()

#MESSAGE(lc7core_SOURCES=${lc7core_SOURCES})
#MESSAGE(lc7core_HEADERS=${lc7core_HEADERS})

ADD_DEFINITIONS(${PLATFORM_DEFINITIONS})

IF(CMAKE_COMPILER_IS_GNUCC)
  SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
ELSEIF(MSVC)
  SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
ENDIF()

LINK_DIRECTORIES( 
	${PLATFORM_LIBRARY_DIRS}
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} "include" "../api/include" "../external/qtkeychain" "../external/quazip" "../external/quazip/zlib" ${PLATFORM_INCLUDE_DIRS})

ADD_LIBRARY(lc7core SHARED ${lc7core_SOURCES} ${lc7core_HEADERS} ${lc7core_UI_HEADERS} )
TARGET_LINK_LIBRARIES(lc7core qt5keychain quazip ${PLATFORM_LIBS})

qt5_use_modules(lc7core Widgets Sql)

ADD_PRECOMPILED_HEADER(lc7core stdafx lc7core_SOURCES)
