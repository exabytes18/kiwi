find_path(SNAPPY_INCLUDE_DIR snappy.h)
find_library(SNAPPY_LIBRARIES snappy)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(snappy DEFAULT_MSG
  SNAPPY_LIBRARIES
  SNAPPY_INCLUDE_DIR)

mark_as_advanced(
  SNAPPY_INCLUDE_DIR
  SNAPPY_LIBRARIES)
