# Find Faust2
include(FindPackageHandleStandardArgs)

# When ON, link against the self-contained static library that bundles LLVM
# (libfaustwithllvm on Linux/macOS, libfaustwithllvm.lib on Windows). This
# produces a plugin with no runtime dependency on libfaust.
option(FAUST_STATIC "Link against a static libfaust that bundles LLVM" OFF)

find_path(FAUST_INCLUDE_DIR faust/dsp/llvm-dsp.h
  HINTS
    C:/Program Files/Faust/include/
    /opt/lib/faust/architecture/
    /usr/lib/faust/architecture/
    /usr/local/lib/faust/architecture/
    "${FAUST_INCLUDE_DIR_HINT}"
)

if(FAUST_STATIC)
    find_library(FAUST_LIBRARY
        NAMES
            libfaustwithllvm.lib
            libfaustwithllvm.a
            faustwithllvm
        HINTS
            "${FAUST_LIB_DIR_HINT}"
    )
else()
    find_library(FAUST_LIBRARY
        NAMES
            libfaust.so
            libfaust.dylib
            faust.dll
            faust
            libfaust
        HINTS
            "${FAUST_LIB_DIR_HINT}"
    )
endif()

# Locate the Faust standard library (containing stdfaust.lib). It is shipped
# as share/faust in the binary packages and as libraries/ in the source tree.
find_path(FAUST_STDLIB_DIR stdfaust.lib
    HINTS
        "${FAUST_STDLIB_DIR_HINT}"
        "${FAUST_LIB_DIR_HINT}/../share/faust"
        "${FAUST_INCLUDE_DIR_HINT}/../share/faust"
        "${FAUST_LIB_DIR_HINT}/../libraries"
        "${FAUST_INCLUDE_DIR_HINT}/../libraries"
        "${FAUST_INCLUDE_DIR_HINT}/../share/faust"
)

find_package_handle_standard_args(FAUST FAUST_INCLUDE_DIR FAUST_LIBRARY)

if(FAUST_FOUND)
    set(FAUST_INCLUDE_DIRS ${FAUST_INCLUDE_DIR})

    if(FAUST_STATIC)
        # libfaustwithllvm bundles LLVM, so only platform libraries are needed.
        if(WIN32)
            set(FAUST_LIBRARIES ${FAUST_LIBRARY}
                ws2_32 crypt32 dbghelp ole32 shell32 psapi bcrypt
                version uuid user32 advapi32 ntdll)
        elseif(APPLE)
            set(FAUST_LIBRARIES ${FAUST_LIBRARY} z)
        else()
            set(_faust_syslibs dl pthread z)
            find_library(FAUST_TINFO_LIBRARY NAMES tinfo ncurses)
            if(FAUST_TINFO_LIBRARY)
                list(APPEND _faust_syslibs ${FAUST_TINFO_LIBRARY})
            endif()
            set(FAUST_LIBRARIES ${FAUST_LIBRARY} ${_faust_syslibs})
        endif()
    else()
        set(FAUST_LIBRARIES ${FAUST_LIBRARY})

        if("${FAUST_LIBRARY}" MATCHES ".*\\.a")
            # This is a static build of faust without LLVM, hence
            # we have to add all the LLVM flags...

            find_program(LLVM_CONFIG llvm-config
                HINTS
                    /usr/bin
                    /usr/local/bin
                    /usr/local/opt/llvm/bin
            )

            if(NOT LLVM_CONFIG)
                message(WARNING "Using a static Faust library requires LLVM tooling to be present in the path.")
                UNSET(FAUST_FOUND)
            else()
              exec_program(${LLVM_CONFIG} ARGS --includedir OUTPUT_VARIABLE LLVM_DIR)
              exec_program(${LLVM_CONFIG} ARGS --libs OUTPUT_VARIABLE LLVM_LIBS)
              exec_program(${LLVM_CONFIG} ARGS --version OUTPUT_VARIABLE LLVM_VERSION)
              exec_program(${LLVM_CONFIG} ARGS --ldflags OUTPUT_VARIABLE LLVM_LDFLAGS)

              set(LLVM_VERSION LLVM_${LLVM_VERSION_MAJOR}${LLVM_VERSION_MINOR})

              find_package(OpenSSL REQUIRED)
              set(FAUST_LIBRARIES ${FAUST_LIBRARIES} dl ${OPENSSL_LIBRARIES} ncurses z ${LLVM_LDFLAGS} ${LLVM_LIBS} )
            endif()
          endif()
    endif()
else()
    set(FAUST_LIBRARIES)
    set(FAUST_INCLUDE_DIRS)
endif()
