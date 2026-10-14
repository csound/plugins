cmake_minimum_required(VERSION 3.16)
project(CsoundWasmUnwind C)

# LLVM 21's main build also selects assembly files that do not support WASM.
# The VM unwinds the stack; this source supplies the C++ exception interface.
add_library(unwind STATIC "${LIBUNWIND_SOURCE_DIR}/src/Unwind-wasm.c")
set_target_properties(unwind PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(unwind PRIVATE c_std_11)
target_compile_options(unwind PRIVATE -fwasm-exceptions)
target_compile_definitions(unwind PRIVATE _LIBUNWIND_HIDE_SYMBOLS)
target_include_directories(unwind PRIVATE "${LIBUNWIND_SOURCE_DIR}/include")
install(TARGETS unwind ARCHIVE DESTINATION lib)
