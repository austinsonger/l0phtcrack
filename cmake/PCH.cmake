
MACRO(ADD_PRECOMPILED_HEADER target pch_name sources)
	
	IF(MSVC)
    
		# Find PCH compile file in sources list
		FOREACH(source ${${sources}})
			IF(${source} MATCHES ".*${pch_name}.cpp$")
				SET(precompiled_source ${source})
			ENDIF()
		ENDFOREACH()
		SET(precompiled_header ${pch_name}.h)
		SET(precompiled_binary "${CMAKE_CURRENT_BINARY_DIR}/${pch_name}_$<CONFIG>.pch")
		FILE(TO_NATIVE_PATH "${precompiled_binary}" precompiled_binary_native)
			
		
		# Move PCH compile to front of list
		LIST(REMOVE_ITEM ${sources} ${precompiled_source})
		LIST(INSERT ${sources} 0 ${precompiled_source})
	
		SET_TARGET_PROPERTIES(${target}
							  PROPERTIES COMPILE_OPTIONS "/Yu${precompiled_header};/FI${precompiled_header};/Fp${precompiled_binary_native}"
							  OBJECT_DEPENDS "${precompiled_binary}")  
		SET_SOURCE_FILES_PROPERTIES(${precompiled_source}
								    PROPERTIES COMPILE_FLAGS "/Yc${precompiled_header}"
									OBJECT_OUTPUTS "${precompiled_binary}")
	ENDIF(MSVC)

ENDMACRO(ADD_PRECOMPILED_HEADER)

