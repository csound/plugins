# Build image opcodes
option(BUILD_IMAGE_OPCODES "Build the image opcodes" ON)

if(${BUILD_IMAGE_OPCODES})
	find_package(PNG)
	check_deps(BUILD_IMAGE_OPCODES PNG_FOUND)

	if(BUILD_IMAGE_OPCODES)
		make_plugin(image imageOpcodes.c)
		target_include_directories(image PRIVATE ${PNG_INCLUDE_DIR})
		target_link_libraries(image ${PNG_LIBRARIES})
		target_compile_definitions(image PRIVATE ${PNG_DEFINITIONS})
	endif()
endif()
