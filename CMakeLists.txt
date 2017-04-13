# cmake file

# build tool configuration

cmake_minimum_required(VERSION 3.2.0)

# add project cmake module path
set(PROJECT_CMAKE_MODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
list(APPEND CMAKE_MODULE_PATH ${PROJECT_CMAKE_MODULE_DIR})

include(Util)

get_version(PROJECT_VERSION)

set(PROJECT_NAME SimplifyLoopExitsOptPass)

# Standalone build or part of LLVM?
set(PROJECT_STANDALONE_BUILD FALSE)

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  project(${PROJECT_NAME} C CXX)
  set(PROJECT_STANDALONE_BUILD TRUE)
endif()

msg(STATUS "configuring as external LLVM project: ${PROJECT_STANDALONE_BUILD}")

# TODO need to move this to top-level cmake list file
# CMP0056 is required for any libc++abi dependency of the try_compile() command
# this bumps required version to 3.2
# otherwise LD_LIBRARY_DIR needs to be set
set(PROJECT_POLICIES CMP0056)

foreach(PROJECT_POLICY ${PROJECT_POLICIES})
  if(POLICY ${PROJECT_POLICY})
    cmake_policy(GET ${PROJECT_POLICY} POLICY_VALUE)
    msg(STATUS "policy ${PROJECT_POLICY}: ${POLICY_VALUE}")
  endif()
endforeach()

include(CMakePackageConfigHelpers)

# requirements

if(${PROJECT_STANDALONE_BUILD})
  msg(STATUS "detecting LLVM dependency")

  # find_package in CONFIG mode will lookup and include LLVMConfig.cmake
  find_package(LLVM REQUIRED CONFIG)

  if(${LLVM_FOUND})
    msg(STATUS "found LLVM ${LLVM_PACKAGE_VERSION}")
    msg(STATUS "using LLVMConfig.cmake in: ${LLVM_DIR}")
  else()
    msg(FATAL_ERROR "Could not find LLVM")
  endif()
endif()

# These are required if functions are used:
# - add_llvm_loadable_module()
# - add_llvm_library
if(${PROJECT_STANDALONE_BUILD})
  list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
else()
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/share/llvm/cmake/")
endif()

include(AddLLVM)
include(HandleLLVMOptions)

#

msg(STATUS "detecting Python and LLVM lit/FileCheck dependency")

include(Python)

find_python_module(lit REQUIRED)

#if(NOT EXISTS ${LLVM_TOOLS_BINARY_DIR}/FileCheck)
  #msg(FATAL_ERROR "Could not find FileCheck. Please configure LLVM properly.")
#endif()


#

msg(STATUS "detecting Doxygen dependency")

find_package(Doxygen)

if(${DOXYGEN_FOUND})
  msg(STATUS "found Doxygen ${DOXYGEN_VERSION}")

  set(DOXYGEN_CFG_FILE_TEMPLATE
    "${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen.cfg.in")
  set(DOXYGEN_CFG_FILE "${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen.cfg")

  configure_file(${DOXYGEN_CFG_FILE_TEMPLATE} ${DOXYGEN_CFG_FILE} @ONLY)

  add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CFG_FILE}
    WORKING_DIR ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "generating documentation" VERBATIM)
else()
  msg(STATUS "could not find Doxygen")
endif()

# project configuration

#

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

if(NOT ${LLVM_ENABLE_RTTI})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

msg(STATUS "RTTI support: ${LLVM_ENABLE_RTTI}")

if(NOT ${LLVM_ENABLE_EH})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
endif()

msg(STATUS "EH support: ${LLVM_ENABLE_EH}")


add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})

if(${LLVM_PACKAGE_VERSION} VERSION_LESS "3.8.0")
  set(LLVM_RUNTIME_OUTPUT_INTDIR "${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INT_DIR}")
  set(LLVM_RUNTIME_OUTPUT_INTDIR "${CMAKE_BINARY_DIR}/lib/${CMAKE_CFG_INT_DIR}")
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/")

set(LIB_NAME "LLVMSimplifyLoopExitsPass")
set(LIB_SOURCES 
  "${CMAKE_CURRENT_SOURCE_DIR}/lib/SimplifyLoopExits.cpp")

if(NOT ${USE_LLVM_INTERNAL_MODULE_FUNCTION})
  add_library(${LIB_NAME} MODULE ${LIB_SOURCES})
else()
  # this follows the suggestion documented in
  # http://llvm.org/docs/CMake.html#cmake-out-of-source-pass
  # for allowing future inclusion of the plugin in the LLVM source tree
  # by using LLVM's internal cmake functions and utilities
  set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/bin)
  set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/lib)

  add_llvm_loadable_module(${LIB_NAME} ${LIB_SOURCES})
endif()

# according to
# http://llvm.org/docs/CMake.html
# we do not need the below library dependencies since the plugin will be loaded
# via opt or clang which already have those libs in their dependencies
# comment out the following two statements
# to test against static lib LLVM build
#llvm_map_components_to_libnames(llvm_libs core support passes)
#target_link_libraries(${LIB_NAME} ${llvm_libs})

#

target_compile_definitions(${LIB_NAME} PUBLIC 
  VERSION_STRING=${PROJECT_VERSION})

# TODO better way to deduce library name, especially when using llvm cmake
# functions
get_target_property(PROJECT_LIB_NAMES ${LIB_NAME} NAME)
set(PROJECT_LIB_NAMES "${PROJECT_LIB_NAMES} ${LIB_NAME}.so lib${LIB_NAME}.so")

configure_file(${PROJECT_CMAKE_MODULE_DIR}/SimplifyLoopExitsConfig.cmake.in
  "share/${PROJECT_NAME}/cmake/${PROJECT_NAME}Config.cmake"
  @ONLY)

# TODO parse project version from vcs
write_basic_package_version_file(
  "share/${PROJECT_NAME}/cmake/${PROJECT_NAME}ConfigVersion.cmake"
  VERSION "0.0.0"
  COMPATIBILITY SameMajorVersion)

# installation

install(TARGETS ${LIB_NAME} LIBRARY DESTINATION "lib")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION "include")
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/share" DESTINATION .)
