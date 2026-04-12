# Try to find the STK library.
# Once done this will define:
#  STK_FOUND - System has the STK library.
#  STK_INCLUDE_DIRS - The STK include directories.
#  STK_LIBRARIES - The libraries needed to use the STK.

find_path(STK_INCLUDE_DIR Stk.h PATH_SUFFIXES stk HINTS "${STK_INCLUDE_DIR_HINT}")
find_library(STK_LIBRARY NAMES stk HINTS "${STK_LIBRARY_DIR_HINT}")

set(STK_INCLUDE_DIRS ${STK_INCLUDE_DIR})
set(STK_LIBRARIES ${STK_LIBRARY} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set STK_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(STK DEFAULT_MSG
                                  STK_LIBRARY STK_INCLUDE_DIR)

mark_as_advanced(STK_INCLUDE_DIR STK_LIBRARY)
