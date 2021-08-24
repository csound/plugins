# Try to find the Csound library.
# Once done this will define:
#  CSOUND_FOUND - System has the Csound library
#  CSOUND_INCLUDE_DIRS - The Csound include directories.
#  CSOUND_LIBRARIES - The libraries needed to use the Csound library.

if(APPLE)
find_path(CSOUND_INCLUDE_DIR csound.h HINTS 
"$ENV{HOME}/Library/Frameworks/CsoundLib64.framework/Headers"
 /Library/Frameworks/CsoundLib64.framework/Headers
 "$ENV{HOME}/Library/Frameworks/CsoundLib.framework/Headers"
 /Library/Frameworks/CsoundLib.framework/Headers
  ${CSOUND_INCLUDE_DIR_HINT})
find_path(CSOUND_FRAMEWORK CsoundLib64 HINTS 
"$ENV{HOME}/Library/Frameworks/CsoundLib64.framework"
 /Library/Frameworks/CsoundLib64.framework
  ${CSOUND_FRAMEWORK_DIR_HINT})
find_path(CSOUND_FRAMEWORK32 CsoundLib HINTS 
"$ENV{HOME}/Library/Frameworks/CsoundLib64.framework"
 /Library/Frameworks/CsoundLib64.framework 
 "$ENV{HOME}/Library/Frameworks/CsoundLib.framework"
 /Library/Frameworks/CsoundLib.framework
  ${CSOUND_FRAMEWORK32_DIR_HINT}) 
else()
find_path(CSOUND_INCLUDE_DIR csound.h PATH_SUFFIXES csound HINTS ${CSOUND_INCLUDE_DIR_HINT})
endif()

if(APPLE)
find_library(CSOUND_LIBRARY NAMES CsoundLib64 HINTS /Library/Frameworks/CsoundLib64.framework/
"$ENV{HOME}/Library/Frameworks/CsoundLib64.framework" ${CSOUND_LIBRARY_DIR_HINT})
else()
find_library(CSOUND_LIBRARY NAMES csound64 csound HINTS ${CSOUND_LIBRARY_DIR_HINT})
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CSOUND_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(CSOUND
                                  CSOUND_LIBRARY CSOUND_INCLUDE_DIR)
mark_as_advanced(CSOUND_INCLUDE_DIR CSOUND_LIBRARY)

set(CSOUND_INCLUDE_DIRS ${CSOUND_INCLUDE_DIR})
set(CSOUND_LIBRARIES ${CSOUND_LIBRARY} )
