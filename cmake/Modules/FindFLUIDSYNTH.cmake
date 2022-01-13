# - Find fluidsynth
# Find the native fluidsynth includes and library
#
#  FLUIDSYNTH_INCLUDE_DIR - where to find fluidsynth.h
#  FLUIDSYNTH_LIBRARIES   - List of libraries when using fluidsynth.
#  FLUIDSYNTH_FOUND       - True if fluidsynth found.

if (FLUIDSYNTH_INCLUDE_DIR AND FLUIDSYNTH_LIBRARIES)
  # Already in cache, be silent
  set (FluidSynth_FIND_QUIETLY TRUE)
endif ()

if(APPLE)
find_path(FLUIDSYNTH_INCLUDE_DIR fluidsynth.h HINTS 
 /usr/local/include
  ${FLUIDSYNTH_INCLUDE_DIR_HINT})
else()
find_path (FLUIDSYNTH_INCLUDE_DIR fluidsynth.h)
endif()

if(APPLE)
find_library(FLUIDSYNTH_LIBRARY NAMES Fluidsynth HINTS /Library/Frameworks/Fluidsynth.framework/
 ${FLUIDSYNTH_LIBRARY_DIR_HINT})
else()
find_library (FLUIDSYNTH_LIBRARIES NAMES fluidsynth libfluidsynth)
endif()

mark_as_advanced (FLUIDSYNTH_LIBRARIES FLUIDSYNTH_INCLUDE_DIR)

# handle the QUIETLY and REQUIRED arguments and set FLUIDSYNTH_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FluidSynth DEFAULT_MSG FLUIDSYNTH_LIBRARIES FLUIDSYNTH_INCLUDE_DIR)
