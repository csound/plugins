# Try to find the Csound library.
# Once done this will define:
#  CSOUND_FOUND - System has the Csound library
#  CSOUND_INCLUDE_DIRS - The Csound include directories.
#  CSOUND_LIBRARIES - The libraries needed to use the Csound library.

if(APPLE)
    # Hints are tried in order: explicit overrides, local builds in ~/Library,
    # the Csound 7 installer, then the system framework.
    set(CMAKE_FIND_FRAMEWORK LAST)
    find_path(CSOUND_INCLUDE_DIR csound.h
        HINTS
            ${CSOUND_INCLUDE_DIR_HINT}
            "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework/Headers"
            "$ENV{HOME}/Library/Frameworks/CsoundLib.framework/Headers"
            /Applications/csound/CsoundLib64.framework/Headers
            /Library/Frameworks/CsoundLib64.framework/Headers
            /Library/Frameworks/CsoundLib.framework/Headers
    )

    find_path(CSOUND_FRAMEWORK CsoundLib64
        HINTS
            ${CSOUND_FRAMEWORK_DIR_HINT}
            "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework"
            /Applications/csound/CsoundLib64.framework
            /Library/Frameworks/CsoundLib64.framework
    )

    find_path(CSOUND_FRAMEWORK32 CsoundLib
        HINTS
            ${CSOUND_FRAMEWORK32_DIR_HINT}
            "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework"
            "$ENV{HOME}/Library/Frameworks/CsoundLib.framework"
            /Applications/csound/CsoundLib64.framework
            /Library/Frameworks/CsoundLib64.framework
            /Library/Frameworks/CsoundLib.framework
    )
else()
    if(WIN32)
        find_path(CSOUND_INCLUDE_DIR csound.h
            PATH_SUFFIXES csound
            HINTS
                ${CSOUND_INCLUDE_DIR_HINT}
                "$ENV{ProgramFiles}/Csound7/include"
                "$ENV{ProgramW6432}/Csound7/include"
                "$ENV{ProgramFiles\(x86\)}/Csound7/include"
        )
    else()
        find_path(CSOUND_INCLUDE_DIR csound.h
            PATH_SUFFIXES csound
            HINTS ${CSOUND_INCLUDE_DIR_HINT}
        )
    endif()
endif()

if(APPLE)
    find_file(CSOUND_LIBRARY
        NAMES CsoundLib64
        NO_DEFAULT_PATH
        HINTS
            ${CSOUND_LIBRARY_DIR_HINT}
            "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework"
            /Applications/csound/CsoundLib64.framework
            /Library/Frameworks/CsoundLib64.framework/
    )
else()
    # Csound 7 on Windows is named csound7_64 (double) or csound7 (single).
    if(WIN32)
        set(_csound_library_names csound7_64 csound7 csound64 csound)
    else()
        set(_csound_library_names csound64 csound)
    endif()
    if(WIN32)
        find_library(CSOUND_LIBRARY
            NAMES ${_csound_library_names}
            HINTS
                ${CSOUND_LIBRARY_DIR_HINT}
                "$ENV{ProgramFiles}/Csound7/lib64"
                "$ENV{ProgramW6432}/Csound7/lib64"
        )
    else()
        find_library(CSOUND_LIBRARY
            NAMES ${_csound_library_names}
            HINTS ${CSOUND_LIBRARY_DIR_HINT}
        )
    endif()
    unset(_csound_library_names)
endif()

if(CSOUND_INCLUDE_DIR AND EXISTS "${CSOUND_INCLUDE_DIR}/version.h")
    file(STRINGS "${CSOUND_INCLUDE_DIR}/version.h" _csound_major
        REGEX "^#define[ \t]+CS_VERSION[ \t]+")
    file(STRINGS "${CSOUND_INCLUDE_DIR}/version.h" _csound_minor
        REGEX "^#define[ \t]+CS_SUBVER[ \t]+")
    file(STRINGS "${CSOUND_INCLUDE_DIR}/version.h" _csound_patch
        REGEX "^#define[ \t]+CS_PATCHLEVEL[ \t]+")

    string(REGEX REPLACE "^#define[ \t]+CS_VERSION[ \t]+\\(?([0-9]+)\\)?.*" "\\1"
        CSOUND_VERSION_MAJOR "${_csound_major}")
    string(REGEX REPLACE "^#define[ \t]+CS_SUBVER[ \t]+\\(?([0-9]+)\\)?.*" "\\1"
        CSOUND_VERSION_MINOR "${_csound_minor}")
    string(REGEX REPLACE "^#define[ \t]+CS_PATCHLEVEL[ \t]+\\(?([0-9]+)\\)?.*" "\\1"
        CSOUND_VERSION_PATCH "${_csound_patch}")

    if(CSOUND_VERSION_MAJOR MATCHES "^[0-9]+$")
        if(NOT CSOUND_VERSION_MINOR MATCHES "^[0-9]+$")
            set(CSOUND_VERSION_MINOR 0)
        endif()
        if(NOT CSOUND_VERSION_PATCH MATCHES "^[0-9]+$")
            set(CSOUND_VERSION_PATCH 0)
        endif()
        set(CSOUND_VERSION
            "${CSOUND_VERSION_MAJOR}.${CSOUND_VERSION_MINOR}.${CSOUND_VERSION_PATCH}")
    endif()
endif()

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set CSOUND_FOUND to TRUE
# if all listed variables are TRUE and the version satisfies the request
if(WASM)
    # Side modules call the browser engine through its API table.
    set(_csound_required_vars CSOUND_INCLUDE_DIR)
else()
    set(_csound_required_vars CSOUND_LIBRARY CSOUND_INCLUDE_DIR)
endif()
find_package_handle_standard_args(CSOUND
    REQUIRED_VARS ${_csound_required_vars}
    VERSION_VAR CSOUND_VERSION
)

mark_as_advanced(CSOUND_INCLUDE_DIR CSOUND_LIBRARY)

set(CSOUND_INCLUDE_DIRS ${CSOUND_INCLUDE_DIR})
set(CSOUND_LIBRARIES ${CSOUND_LIBRARY})
