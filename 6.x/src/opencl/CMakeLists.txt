option(BUILD_OPENCL_OPCODES "Build the OpenCL parallel opcodes" OFF)

find_library (OpenCL_library NAMES OpenCL)
check_deps(BUILD_OPENCL_OPCODES OpenCL_FOUND)

if(BUILD_OPENCL_OPCODES AND OpenCL_FOUND)
    function(make_opencl_plugin libname srcs)
        if(APPLE)
            add_library(${libname} SHARED ${srcs})
        else()
            add_library(${libname} MODULE ${srcs})
        endif()

        set(i 2)
        while( ${i} LESS ${ARGC} )
            target_link_libraries(${libname} ${ARGV${i}})
            math(EXPR i "${i}+1")
        endwhile()

        target_include_directories(${libname} PRIVATE "${CMAKE_SOURCE_DIR}/csound")

        install(TARGETS ${libname}
            LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}"
        )
    endfunction()

    make_opencl_plugin(clop cladsynth.c)
    target_link_libraries(clop PRIVATE ${OpenCL_library})
endif()
