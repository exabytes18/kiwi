find_path(ROCKSDB_INCLUDE_DIR rocksdb/db.h)
find_library(ROCKSDB_LIBRARIES rocksdb)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(rocksdb DEFAULT_MSG
  ROCKSDB_LIBRARIES
  ROCKSDB_INCLUDE_DIR)

mark_as_advanced(
  ROCKSDB_INCLUDE_DIR
  ROCKSDB_LIBRARIES)
