# Build the CUDA GPU opcodes (requires the CUDA toolkit).
# Not available on macOS: NVIDIA dropped CUDA support after macOS 10.13.

option(BUILD_CUDA_OPCODES "Build the CUDA GPU opcodes" OFF)

if(BUILD_CUDA_OPCODES)
    if(APPLE)
        message(STATUS "CUDA is not supported on macOS; disabling BUILD_CUDA_OPCODES")
        set(BUILD_CUDA_OPCODES OFF)
    elseif(CMAKE_VERSION VERSION_LESS 3.18)
        message(STATUS "CUDA requires CMake 3.18 or later; disabling BUILD_CUDA_OPCODES")
        set(BUILD_CUDA_OPCODES OFF)
    else()
        include(CheckLanguage)
        check_language(CUDA)
        if(CMAKE_CUDA_COMPILER)
            enable_language(CUDA)
            find_package(CUDAToolkit)
        endif()
        check_deps(BUILD_CUDA_OPCODES CMAKE_CUDA_COMPILER CUDAToolkit_FOUND)
    endif()
endif()

if(BUILD_CUDA_OPCODES)
    message(STATUS "Building the CUDA opcodes")

    # A broad set of compute capabilities; override with
    # -DCMAKE_CUDA_ARCHITECTURES="..." (e.g. "native").
    if(NOT CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES 75 80 86 89)
    endif()
    set(CMAKE_CUDA_STANDARD 14)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)

    function(make_cuda_plugin libname srcs)
        make_plugin(${libname} ${srcs})
        target_include_directories(${libname} PRIVATE ${CSOUND_INCLUDE_DIRS})
        target_compile_options(${libname} PRIVATE
            "$<$<COMPILE_LANGUAGE:CUDA>:-use_fast_math>")
    endfunction()

    # cudaop1: cudasynth  (additive synthesis driven by a PVS signal)
    make_cuda_plugin(cudaop1 adsyn.cu)
    target_link_libraries(cudaop1 PRIVATE CUDA::cudart)

    # cudaop2: cudasynth, cudanal  (PVS analysis/synthesis)
    make_cuda_plugin(cudaop2 pvsops.cu)
    target_link_libraries(cudaop2 PRIVATE CUDA::cudart CUDA::cufft)

    # cudaop3: cudasliding  (sliding DFT)
    make_cuda_plugin(cudaop3 slidingm.cu)
    target_link_libraries(cudaop3 PRIVATE CUDA::cudart)

    # cudaop4: cudaconv  (direct convolution)
    make_cuda_plugin(cudaop4 conv.cu)
    target_link_libraries(cudaop4 PRIVATE CUDA::cudart)

    # cudaop5: cudapconv  (partitioned convolution)
    make_cuda_plugin(cudaop5 pconv.cu)
    target_link_libraries(cudaop5 PRIVATE CUDA::cudart CUDA::cufft)
else()
    message(STATUS "Not building the CUDA opcodes")
endif()
