find_path(ZLIB_INCLUDE_DIR zlib.h)
find_library(ZLIB_LIBRARIES z)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zlib DEFAULT_MSG
  ZLIB_LIBRARIES
  ZLIB_INCLUDE_DIR)

mark_as_advanced(
  ZLIB_INCLUDE_DIR
  ZLIB_LIBRARIES)
