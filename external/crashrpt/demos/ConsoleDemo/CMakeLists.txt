project(ConsoleDemo)

# Create the list of source files
aux_source_directory( . source_files )
file( GLOB header_files *.h )

# Define _UNICODE (use wide-char encoding)
add_definitions(-D_UNICODE )

fix_default_compiler_settings_()

# Add include dir
include_directories(${CRASHRPT_ROOT}/include 
					${CRASHRPT_ROOT}/reporting/CrashRpt
					${CRASHRPT_ROOT}/thirdparty/wtl 
					${CRASHRPT_ROOT}/thirdparty/zlib
					${CRASHRPT_ROOT}/thirdparty/minizip  
					${CRASHRPT_ROOT}/thirdparty/jpeg 
					${CRASHRPT_ROOT}/thirdparty/libpng 
					${CRASHRPT_ROOT}/thirdparty/tinyxml )

# Add executable build target
add_executable(ConsoleDemo ${source_files} ${header_files})

# Add input link libraries
target_link_libraries(ConsoleDemo CrashRpt)

set_target_properties(ConsoleDemo PROPERTIES DEBUG_POSTFIX d )
