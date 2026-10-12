# Try to find the Csound library.
# Once done this will define:
#  CSOUND_FOUND - System has the Csound library
#  CSOUND_INCLUDE_DIRS - The Csound include directories.
#  CSOUND_LIBRARIES - The libraries needed to use the Csound library.

if(APPLE)
    find_path(CSOUND_INCLUDE_DIR csound.h
        HINTS
            "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework/Headers"
            "$ENV{HOME}/Library/Frameworks/CsoundLib.framework/Headers"
            /Library/Frameworks/CsoundLib64.framework/Headers
            /Library/Frameworks/CsoundLib.framework/Headers
            ${CSOUND_INCLUDE_DIR_HINT}
    )

    find_path(CSOUND_FRAMEWORK CsoundLib64
        HINTS
            "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework"
            /Library/Frameworks/CsoundLib64.framework
            ${CSOUND_FRAMEWORK_DIR_HINT}
    )

    find_path(CSOUND_FRAMEWORK32 CsoundLib
        HINTS
            "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework"
            "$ENV{HOME}/Library/Frameworks/CsoundLib.framework"
            /Library/Frameworks/CsoundLib64.framework
            /Library/Frameworks/CsoundLib.framework
            ${CSOUND_FRAMEWORK32_DIR_HINT}
    )
else()
    find_path(CSOUND_INCLUDE_DIR csound.h
        PATH_SUFFIXES csound
        HINTS ${CSOUND_INCLUDE_DIR_HINT}
    )
endif()

if(APPLE)
    find_library(CSOUND_LIBRARY
        NAMES CsoundLib64
        HINTS
            /Library/Frameworks/CsoundLib64.framework/
            "$ENV{HOME}/Library/Frameworks/CsoundLib64.framework"
            ${CSOUND_LIBRARY_DIR_HINT}
    )
else()
    find_library(CSOUND_LIBRARY
        NAMES csound64 csound
        HINTS ${CSOUND_LIBRARY_DIR_HINT}
    )
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
find_package_handle_standard_args(CSOUND
    REQUIRED_VARS CSOUND_LIBRARY CSOUND_INCLUDE_DIR
    VERSION_VAR CSOUND_VERSION
)

mark_as_advanced(CSOUND_INCLUDE_DIR CSOUND_LIBRARY)

set(CSOUND_INCLUDE_DIRS ${CSOUND_INCLUDE_DIR})
set(CSOUND_LIBRARIES ${CSOUND_LIBRARY})
