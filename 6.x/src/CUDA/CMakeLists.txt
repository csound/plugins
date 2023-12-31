option(BUILD_CUDA_OPCODES "Build the CUDA GPU opcodes" OFF)

function(make_cuda_plugin libname srcs)
    SET(LIB_TYPE_DYNAMIC) #set the lib type
    CUDA_ADD_LIBRARY(${libname} ${LIB_TYPE} ${srcs})

    set(i 2)
    while( ${i} LESS ${ARGC} )
        target_link_libraries(${libname} ${ARGV${i}})
        math(EXPR i "${i}+1")
    endwhile()

    target_include_directories(${libname} PRIVATE "${CMAKE_SOURCE_DIR}/csound")

    # set_target_properties(${libname} PROPERTIES
    #     RUNTIME_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
    #     LIBRARY_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
    #     ARCHIVE_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR})

    install(TARGETS ${libname} LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}" )
endfunction()

if(BUILD_CUDA_OPCODES)
    find_package(CUDA REQUIRED)
    check_deps(BUILD_CUDA_OPCODES CUDA_FOUND)
    message(STATUS "Building the CUDA opcodes")

    include_directories(${CMAKE_HOME_DIRECTORY}/include)
    include_directories(/usr/local/include)
    LIST(APPEND CUDA_NVCC_FLAGS "-use_fast_math")

    if(APPLE)
        if(CMAKE_OSX_DEPLOYMENT_TARGET MATCHES 10.6)
            LIST(APPEND CUDA_NVCC_FLAGS "-arch=sm_11")
            make_cuda_plugin(cudaop1 adsyn11.cu)
            make_cuda_plugin(cudaop2 pvsopsf.cu)
            make_cuda_plugin(cudaop3 slidingm.cu)
            make_cuda_plugin(cudaop4 convf.cu)
            make_cuda_plugin(cudaop5 pconv11.cu)
        else()
            LIST(APPEND CUDA_NVCC_FLAGS "-ccbin /usr/bin/clang -arch=sm_30 -Xcompiler -stdlib=libstdc++")
            make_cuda_plugin(cudaop1 adsyn.cu)
            make_cuda_plugin(cudaop2 pvsops.cu)
            make_cuda_plugin(cudaop3 slidingm.cu)
            make_cuda_plugin(cudaop4 conv.cu)
            make_cuda_plugin(cudaop5 pconv.cu)
        endif()
    else()
        if(CUDA_VERSION VERSION_LESS 9.0)
            LIST(APPEND CUDA_NVCC_FLAGS "-arch=sm_20")
        else()
            LIST(APPEND CUDA_NVCC_FLAGS "-arch=sm_75")
        endif()

        make_cuda_plugin(cudaop1 adsyn.cu)
        make_cuda_plugin(cudaop2 pvsops.cu)
        # make_cuda_plugin(cudaop3 slidingm.cu)
        make_cuda_plugin(cudaop4 conv.cu)
        make_cuda_plugin(cudaop5 pconv.cu)
    endif()

    CUDA_ADD_CUFFT_TO_TARGET(cudaop2)
    CUDA_ADD_CUFFT_TO_TARGET(cudaop5)

    SET(CUDA_VERBOSE_BUILD ON CACHE BOOL "nvcc verbose" FORCE)
else()
    message(STATUS "Not building the CUDA opcodes")
endif()
