find_path(LIBEVENT2_INCLUDE_DIR event2/event.h)
find_library(LIBEVENT2_LIBRARIES NAMES event)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libevent2 DEFAULT_MSG
  LIBEVENT2_LIBRARIES
  LIBEVENT2_INCLUDE_DIR)

mark_as_advanced(
  LIBEVENT2_INCLUDE_DIR
  LIBEVENT2_LIBRARIES)
