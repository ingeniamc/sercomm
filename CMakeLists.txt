#-------------------------------------------------------------------------------
# libsercomm - Multiplatform Serial Communications Library.
#
# MIT License
#
# Copyright (c) 2017 Ingenia-CAT S.L.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

#-------------------------------------------------------------------------------
# General

cmake_minimum_required(VERSION 3.0)
project(sercomm LANGUAGES C VERSION 1.3.2)

set(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake
)

#-------------------------------------------------------------------------------
# Build options

option(WITH_GITINFO    "Include Git commit hash in version"      OFF)
option(WITH_EXAMPLES   "Build library usage example apps"        OFF)
option(WITH_DOCS       "Build library public API documentation"  OFF)
option(WITH_ERRDESC    "Support for detailed error descriptions" ON)
option(WITH_DEVMON     "Support for device listing/monitoring"   ON)
option(WITH_PIC        "Generate position independent code"      OFF)

if(WITH_GITINFO)
  find_package(Git REQUIRED)

  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE PROJECT_VERSION_EXTRA
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

# Build a shared library by default
if(NOT DEFINED BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ON)
endif()

if(NOT BUILD_SHARED_LIBS)
  set(SER_STATIC ON)
endif()

configure_file("config.h.in" "${CMAKE_BINARY_DIR}/config.h")

#-------------------------------------------------------------------------------
# Compiler options

if(MSVC)
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /nologo /W4 /D_CRT_SECURE_NO_WARNINGS")
   set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi")
elseif(${CMAKE_C_COMPILER_ID} MATCHES "(GNU|Clang)")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wall -Wextra -pedantic")
endif()

#-------------------------------------------------------------------------------
# Library

# Sources (common)
set(sercomm_srcs
  sercomm/base.c
  sercomm/err.c
)

# Sources (POSIX/Linux)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND sercomm_srcs
    sercomm/posix/base.c
    sercomm/posix/comms.c
    sercomm/posix/time.c
  )

  if(WITH_DEVMON)
    list(APPEND sercomm_srcs
      sercomm/posix/dev_linux.c
    )
  endif()
# Sources (POSIX/macOS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  list(APPEND sercomm_srcs
    sercomm/posix/base.c
    sercomm/posix/comms.c
    sercomm/posix/time.c
  )

  if(WITH_DEVMON)
    list(APPEND sercomm_srcs
      sercomm/posix/dev_macos.c
    )
  endif()
# Sources (Windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  list(APPEND sercomm_srcs
    sercomm/win/base.c
    sercomm/win/comms.c
    sercomm/win/err.c
   )

  if(WITH_DEVMON)
    list(APPEND sercomm_srcs
      sercomm/win/dev.c
    )
  endif()
else()
  message(FATAL "libsercomm does not suppor your system")
endif()

add_library(sercomm ${sercomm_srcs})

# Options
if(WITH_PIC)
  set_target_properties(sercomm PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# Includes
target_include_directories(
  sercomm
  PRIVATE
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/public>"
    "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:include>"
)

# Definitions (common)
target_compile_definitions(sercomm PRIVATE SER_BUILDING)

# Definitions (conditional)
if(WITH_ERRDESC)
  target_compile_definitions(sercomm PRIVATE SER_WITH_ERRDESC)
endif()

# Linked libraries (Linux)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # FIX: disable TRY_RUN for pthreads when cross-compiling
  if(CMAKE_CROSSCOMPILING)
    set(THREADS_PTHREAD_ARG OFF)
  endif()
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)

  target_link_libraries(sercomm PRIVATE ${CMAKE_THREAD_LIBS_INIT})

  if(WITH_DEVMON)
    if(NOT DEFINED UDEV_FOUND)
      find_package(PkgConfig)

      if(PKG_CONFIG_FOUND)
        pkg_check_modules(UDEV libudev)

        if(NOT UDEV_FOUND)
          set(WITH_DEVMON OFF CACHE BOOL "Device listing/monitoring" FORCE)
          message(WARNING "Device listing/monitoring turned off (udev missing)")
        endif()
      else()
        message(WARNING "Pkg-Config is required to find libudev")
      endif()
    endif()

    if(UDEV_FOUND)
      target_link_libraries(sercomm PRIVATE ${UDEV_LIBRARIES})
    endif()
  endif()
# Linked libraries (macOS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)

  target_link_libraries(sercomm PRIVATE ${CMAKE_THREAD_LIBS_INIT})

  if(WITH_DEVMON)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(FOUNDATION_LIBRARY Foundation)

    target_link_libraries(
      sercomm
      PRIVATE
        ${FOUNDATION_LIBRARY}
        ${IOKIT_LIBRARY}
    )
  endif()
# Linked libraries (Windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_link_libraries(sercomm PRIVATE setupapi advapi32)
endif()

#-------------------------------------------------------------------------------
# Examples

if(WITH_EXAMPLES)
  add_subdirectory(examples)
endif()

#-------------------------------------------------------------------------------
# Documentation

if(WITH_DOCS)
  find_package(Doxygen REQUIRED)

  # Doxyfile variables
  set(DOCS_TMPL_DIR ${CMAKE_SOURCE_DIR}/docs/template)
  set(DOCS_OUT_DIR ${CMAKE_BINARY_DIR}/docs)

  set(DOCS_EXAMPLES_DIR ${CMAKE_SOURCE_DIR}/examples)
  set(DOCS_SRCS_LST
    ${CMAKE_SOURCE_DIR}/README.md
    ${CMAKE_SOURCE_DIR}/include/public
    ${CMAKE_SOURCE_DIR}/examples
  )

  string(REPLACE ";" " " DOCS_SRCS "${DOCS_SRCS_LST}")

  # Generate Doxyfile
  configure_file(
    "docs/template/Doxyfile.in"
    "${CMAKE_BINARY_DIR}/Doxyfile"
  )

  # Add 'docs' target
  add_custom_target(
    docs
    COMMAND
      ${DOXYGEN_EXECUTABLE}
    COMMENT "Building documentation" VERBATIM
  )
endif()

#-------------------------------------------------------------------------------
# Installation

# CMake files
include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${CMAKE_SOURCE_DIR}/cmake/Config.cmake.in"
  "${CMAKE_BINARY_DIR}/sercommConfig.cmake"
  INSTALL_DESTINATION "lib/cmake/sercomm"
)

write_basic_package_version_file(
  "${CMAKE_BINARY_DIR}/sercommConfigVersion.cmake"
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    "${CMAKE_BINARY_DIR}/sercommConfig.cmake"
    "${CMAKE_BINARY_DIR}/sercommConfigVersion.cmake"
  DESTINATION
    "lib/cmake/sercomm"
  COMPONENT cmake
)

install(
  EXPORT "sercommTargets"
  DESTINATION "lib/cmake/sercomm"
  COMPONENT cmake
)

# Library
install(
  TARGETS sercomm
  EXPORT "sercommTargets"
  LIBRARY DESTINATION "lib" COMPONENT libraries
  ARCHIVE DESTINATION "lib" COMPONENT archive
  RUNTIME DESTINATION "bin" COMPONENT binaries
  INCLUDES DESTINATION "include"
)

# Headers
install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/include/public/sercomm"
  DESTINATION "include"
  COMPONENT headers
)

install(
  FILES "${CMAKE_BINARY_DIR}/config.h"
  DESTINATION "include/sercomm"
  COMPONENT headers
)

# Documentation
if(WITH_DOCS)
  if(EXISTS ${DOCS_OUT_DIR})
    install(
      DIRECTORY ${DOCS_OUT_DIR}
      DESTINATION "share/sercomm"
      COMPONENT documentation
    )
  endif()
endif()

#-------------------------------------------------------------------------------
# Packaging

if(WITH_GITINFO)
  set(PACKAGE_VERSION_EXTRA "-${PROJECT_VERSION_EXTRA}")
endif()

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}${PACKAGE_VERSION_EXTRA}")

if(WIN32)
  set(CPACK_GENERATOR "ZIP")
else()
  set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)
