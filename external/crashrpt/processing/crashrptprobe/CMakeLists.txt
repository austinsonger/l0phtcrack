project(CrashRptProbe)

# Create the list of source files
aux_source_directory( . source_files )
file( GLOB header_files *.h )

list(APPEND source_files ./CrashRptProbe.rc ./CrashRptProbe.def ${CRASHRPT_ROOT}/reporting/crashrpt/Utility.cpp
			${CRASHRPT_ROOT}/reporting/crashsender/md5.cpp)

# Enable usage of precompiled header
set(srcs_using_precomp ${source_files})
list(REMOVE_ITEM srcs_using_precomp  ./CrashRptProbe.rc ./CrashRptProbe.def ./stdafx.cpp ${CRASHRPT_ROOT}/reporting/crashsender/md5.cpp)
add_msvc_precompiled_header(stdafx.h ./stdafx.cpp srcs_using_precomp)

# Define _UNICODE (use wide-char encoding)
add_definitions(-D_UNICODE)

fix_default_compiler_settings_()

# Add include dir
include_directories( ${CRASHRPT_ROOT}/include 
			${CRASHRPT_ROOT}/reporting/crashrpt
			${CRASHRPT_ROOT}/reporting/crashsender
			${CRASHRPT_ROOT}/thirdparty/wtl
			${CRASHRPT_ROOT}/thirdparty/zlib
			${CRASHRPT_ROOT}/thirdparty/minizip
			${CRASHRPT_ROOT}/thirdparty/tinyxml
			${CRASHRPT_ROOT}/thirdparty/dbghelp/include)
			
# Add library build target
if(CRASHRPT_BUILD_SHARED_LIBS)	
	add_library(CrashRptProbe SHARED ${source_files} ${header_files})
else(CRASHRPT_BUILD_SHARED_LIBS)
	add_library(CrashRptProbe STATIC ${source_files} ${header_files})	
endif(CRASHRPT_BUILD_SHARED_LIBS)

if(CMAKE_CL_64)
	link_directories( ${CRASHRPT_ROOT}/thirdparty/dbghelp/lib/amd64 )	
	target_link_libraries(CrashRptProbe ${CRASHRPT_ROOT}/thirdparty/dbghelp/lib/amd64/dbghelp.lib)
else(CMAKE_CL_64)
	link_directories( ${CRASHRPT_ROOT}/thirdparty/dbghelp/lib )
	target_link_libraries(CrashRptProbe ${CRASHRPT_ROOT}/thirdparty/dbghelp/lib/dbghelp.lib)
endif(CMAKE_CL_64)

target_link_libraries(CrashRptProbe zlib minizip tinyxml Rpcrt4.lib shell32.lib gdi32.lib version.lib psapi.lib)

if(CRASHRPT_BUILD_SHARED_LIBS)
	set_target_properties(CrashRptProbe PROPERTIES 
									DEBUG_POSTFIX ${CRASHRPT_VER}d 
									BETA_POSTFIX ${CRASHRPT_VER}
									RELEASE_POSTFIX ${CRASHRPT_VER}
									RELWITHDEBINFO_POSTFIX ${CRASHRPT_VER})
else(CRASHRPT_BUILD_SHARED_LIBS)
	set_target_properties(CrashRptProbe PROPERTIES 
									DEBUG_POSTFIX LIBd 
									BETA_POSTFIX LIB
									RELEASE_POSTFIX LIB
									RELWITHDEBINFO_POSTFIX LIB )
endif(CRASHRPT_BUILD_SHARED_LIBS)

