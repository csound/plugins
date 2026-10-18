# Match platform/wasm-wasi/src/plugin_example*.nix in csound/csound.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(USE_COMPILER_OPTIMIZATIONS OFF CACHE BOOL "" FORCE)
set(REQUIRE_PTHREADS OFF CACHE BOOL "" FORCE)
set(USE_DOUBLE ON CACHE BOOL "" FORCE)
if(NOT WASM_MAX_MEMORY)
    message(FATAL_ERROR "WASM_MAX_MEMORY must match the shared-memory Csound engine")
endif()
execute_process(COMMAND ${CMAKE_C_COMPILER} --print-libgcc-file-name
    OUTPUT_VARIABLE WASM_BUILTINS OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)

function(configure_wasm_plugin target)
    set_target_properties(${target} PROPERTIES SUFFIX ".wasm")
    target_compile_features(${target} PRIVATE c_std_11)
    target_compile_definitions(${target} PRIVATE
        USE_DOUBLE=1 _WASI_EMULATED_SIGNAL _WASI_EMULATED_MMAN)
    target_compile_options(${target} PRIVATE
        -fPIC -pthread "SHELL:-mllvm -wasm-enable-sjlj"
        "SHELL:-mllvm -wasm-use-legacy-eh=false"
        "$<$<COMPILE_LANGUAGE:C>:-std=gnu11>"
        "$<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>"
        "$<$<COMPILE_LANGUAGE:CXX>:-isystem${WASM_CXX_INCLUDE_DIR}>")
    if(WASM_CXX_EXCEPTIONS)
        target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-fwasm-exceptions>")
    else()
        target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>")
    endif()
    target_link_options(${target} PRIVATE
        -shared -nostdlib -nostartfiles
        "LINKER:-Bsymbolic"
        "LINKER:--experimental-pic"
        "LINKER:--no-entry"
        "LINKER:--import-table"
        "LINKER:--import-memory"
        "LINKER:--shared-memory"
        "LINKER:--max-memory=${WASM_MAX_MEMORY}"
        "LINKER:--allow-undefined"
        "LINKER:--export=__wasm_init_tls"
        "LINKER:--export=__tls_size"
        "LINKER:--export=__tls_align"
        "LINKER:--export=__wasm_call_ctors"
        "LINKER:--export-if-defined=__wasm_apply_data_relocs"
        "LINKER:--export-if-defined=csound_opcode_init"
        "LINKER:--export-if-defined=csoundModuleInfo"
        "LINKER:--export-if-defined=csoundModuleInit"
        "LINKER:--export-if-defined=csoundModuleCreate"
        "LINKER:--export-if-defined=csoundModuleDestroy")
    target_link_libraries(${target} PRIVATE
        "${WASM_LIBCXX}" "${WASM_LIBC}" "${WASM_SETJMP}" "${WASM_BUILTINS}")
endfunction()
