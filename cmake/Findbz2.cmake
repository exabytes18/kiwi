find_path(BZ2_INCLUDE_DIR bzlib.h)
find_library(BZ2_LIBRARIES bz2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(bz2 DEFAULT_MSG
  BZ2_LIBRARIES
  BZ2_INCLUDE_DIR)

mark_as_advanced(
  BZ2_INCLUDE_DIR
  BZ2_LIBRARIES)
