find_path(LZ4_INCLUDE_DIR lz4.h)
find_library(LZ4_LIBRARIES lz4)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(lz4 DEFAULT_MSG
  LZ4_LIBRARIES
  LZ4_INCLUDE_DIR)

mark_as_advanced(
  LZ4_INCLUDE_DIR
  LZ4_LIBRARIES)
