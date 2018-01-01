cmake_minimum_required (VERSION 2.6)

project (kiwi)
set (kiwi_VERSION_MAJOR 1)
set (kiwi_VERSION_MINOR 0)

file(GLOB SOURCES src/*.cc)

include_directories("${PROJECT_SOURCE_DIR}/src")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-g -O3")
add_definitions(-Wall -Wextra -pedantic -std=c++14)

# declare our executable
add_executable(kiwidb ${SOURCES})

# check for epoll or kqueue
include(CheckSymbolExists)
check_symbol_exists(epoll_create "sys/epoll.h" HAVE_EPOLL)
check_symbol_exists(kqueue "sys/event.h" HAVE_KQUEUE)
if (NOT HAVE_EPOLL AND NOT HAVE_KQUEUE)
    message (FATAL_ERROR "You must have either epoll or kqueue")
endif ()

# pthreads
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
target_link_libraries(kiwidb ${CMAKE_THREAD_LIBS_INIT})

# yaml-cpp
find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})
target_link_libraries(kiwidb ${YAML_CPP_LIBRARIES})

# rocksdb
find_package(rocksdb REQUIRED)
include_directories(${ROCKSDB_INCLUDE_DIR})
target_link_libraries(kiwidb ${ROCKSDB_LIBRARIES})

# bz2 (needed for rocksdb)
find_package(bz2 REQUIRED)
target_link_libraries(kiwidb ${BZ2_LIBRARIES})

# lz4 (needed for rocksdb)
find_package(lz4 REQUIRED)
target_link_libraries(kiwidb ${LZ4_LIBRARIES})

# snappy (needed for rocksdb)
find_package(snappy REQUIRED)
target_link_libraries(kiwidb ${SNAPPY_LIBRARIES})

# zlib (needed for rocksdb)
find_package(zlib REQUIRED)
target_link_libraries(kiwidb ${ZLIB_LIBRARIES})

# zstd (needed for rocksdb)
find_package(zstd REQUIRED)
target_link_libraries(kiwidb ${ZSTD_LIBRARIES})

configure_file (
  "${PROJECT_SOURCE_DIR}/src/config.h.in"
  "${PROJECT_SOURCE_DIR}/src/config.h"
)
