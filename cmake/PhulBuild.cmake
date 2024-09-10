function(phul_add_component component_name)
	add_library(${component_name}_targets OBJECT)
	target_compile_definitions(${component_name}_targets PRIVATE IS_PHUL_BUILDING)

	add_library(${component_name}_static STATIC $<TARGET_PROPERTY:${component_name}_targets,SOURCES>)
	target_link_libraries(${component_name}_static PUBLIC $<TARGET_PROPERTY:${component_name}_targets,LINK_LIBRARIES>)
	target_compile_definitions(${component_name}_static PUBLIC $<TARGET_PROPERTY:${component_name}_targets,COMPILE_DEFINITIONS>)
	set_target_properties(${component_name}_static PROPERTIES CXX_STANDARD 17)

	add_library(${component_name}_shared SHARED $<TARGET_PROPERTY:${component_name}_targets,SOURCES>)
	target_link_libraries(${component_name}_shared PUBLIC $<TARGET_PROPERTY:${component_name}_targets,LINK_LIBRARIES>)
	target_compile_definitions(${component_name}_shared PUBLIC $<TARGET_PROPERTY:${component_name}_targets,COMPILE_DEFINITIONS>)
	set_target_properties(${component_name}_shared PROPERTIES CXX_STANDARD 17)
endfunction()

function(phul_static_component_link_library component_name library_name)
	target_link_libraries(${component_name}_static PUBLIC ${library_name})
endfunction()

function(phul_shared_component_link_library component_name library_name)
	target_link_libraries(${component_name}_shared PUBLIC ${library_name})
endfunction()

function(phul_component_link_component component_name link_component_name)
	target_link_libraries(${component_name}_static PUBLIC ${link_component_name}_static)
	target_link_libraries(${component_name}_shared PUBLIC ${link_component_name}_shared)
endfunction()
