FUNCTION(make_dist target dist_dir templates libraries plugins utilities extras)
#	MESSAGE(target=${target})
#	MESSAGE(dist_dir=${dist_dir})
#	MESSAGE(templates=${${templates}})
#	MESSAGE(libraries=${${libraries}})
#	MESSAGE(plugins=${${plugins}})
#	MESSAGE(utilities=${${utilities}})
#	MESSAGE(extras=${${extras}})
    	
	IF (MSVC)
		SET(TYPE "$<$<CONFIG:Debug>:d>")
		SET(DEBUGCOPY "$<$<CONFIG:Debug>:${CMAKE_COMMAND}>$<$<NOT:$<CONFIG:Debug>>:echo>")
		
		FOREACH(qtlib ${${libraries}})
			ADD_CUSTOM_COMMAND(TARGET ${target} PRE_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_BINARY_DIR}/${qtlib}${TYPE}.dll ${dist_dir}
				COMMENT "Copying ${qtlib}${TYPE}.dll to ${dist_dir}")
			
			ADD_CUSTOM_COMMAND(TARGET ${target} PRE_BUILD
				COMMAND ${DEBUGCOPY} -E copy_if_different ${QT_BINARY_DIR}/${qtlib}${TYPE}.pdb ${dist_dir}
				COMMENT "$<$<CONFIG:Debug>:Copying ${qtlib}${TYPE}.pdb to ${dist_dir}>")
		
		ENDFOREACH(qtlib)
		
		FOREACH(qtplug ${${plugins}})
			ADD_CUSTOM_COMMAND(TARGET ${target} PRE_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_PLUGINS_DIR}/${qtplug}${TYPE}.dll ${dist_dir}/${qtplug}${TYPE}.dll
				COMMENT "Copying ${qtplug}${TYPE}.dll to ${dist_dir}/${qtplug}${TYPE}.dll")
			
			ADD_CUSTOM_COMMAND(TARGET ${target} PRE_BUILD
				COMMAND ${DEBUGCOPY} -E copy_if_different ${QT_PLUGINS_DIR}/${qtplug}${TYPE}.pdb ${dist_dir}/${qtplug}${TYPE}.pdb 
				COMMENT "$<$<CONFIG:Debug>:Copying ${qtplug}${TYPE}.pdb to ${dist_dir}/${qtplug}${TYPE}.pdb>")
				
		ENDFOREACH(qtplug)

		FOREACH(qtutil ${${utilities}})
			ADD_CUSTOM_COMMAND(TARGET ${target} PRE_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${QT_BINARY_DIR}/${qtutil} ${dist_dir}/${qtutil}
				COMMENT "Copying ${QT_BINARY_DIR}/${qtutil} to ${dist_dir}/${qtutil}")
		ENDFOREACH(qtutil)
		
		
		FOREACH(template ${${templates}})
			FILE(TO_NATIVE_PATH ${template} NATIVE_TEMPLATE)
			FILE(TO_NATIVE_PATH ${dist_dir} NATIVE_DIST_DIR)
						
			ADD_CUSTOM_COMMAND(TARGET ${target} PRE_BUILD
				COMMAND xcopy "${NATIVE_TEMPLATE}" "${NATIVE_DIST_DIR}" /s /d /y
				COMMENT xcopy "${NATIVE_TEMPLATE}" "${NATIVE_DIST_DIR}" /s /d /y)
		ENDFOREACH(template)

		#MESSAGE(${extras})
		#MESSAGE(${${extras}}) 
		
		FOREACH(extra ${${extras}})
			#MESSAGE(">>>> ${extra}")
			STRING(REPLACE "," ";" extra "${extra}" )
			LIST(GET extra 0 FROMFILE)
			LIST(GET extra 1 TOFILE)
			
			#MESSAGE("Extra ${FROMFILE} -> ${dist_dir}/${TOFILE}")
			
			ADD_CUSTOM_COMMAND(TARGET ${target} PRE_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FROMFILE}" "${dist_dir}/${TOFILE}"
				COMMENT "Copying ${FROMFILE} to ${dist_dir}/${TOFILE}")					
		ENDFOREACH(extra)
		
	ENDIF()
ENDFUNCTION(make_dist)
