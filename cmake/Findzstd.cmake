find_path(ZSTD_INCLUDE_DIR zstd.h)
find_library(ZSTD_LIBRARIES zstd)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zstd DEFAULT_MSG
  ZSTD_LIBRARIES
  ZSTD_INCLUDE_DIR)

mark_as_advanced(
  ZLIB_INCLUDE_DIR
  ZSTD_INCLUDE_DIR)
