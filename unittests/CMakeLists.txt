# cmake file

# requirements

find_package(GTest)

if(NOT GTEST_FOUND)
  message(WARNING "Could not find GTest; skipping unit tests")

  return()
endif()

find_package(Threads REQUIRED)


# configuration

set(PRJ_TEST_NAME TestLoopExitCategorization)

set(TEST_SOURCES "${PRJ_TEST_NAME}.cpp")

add_executable(${PRJ_TEST_NAME} ${TEST_SOURCES})

target_compile_definitions(${PRJ_TEST_NAME} PUBLIC ${LLVM_DEFINITIONS})

target_include_directories(${PRJ_TEST_NAME} PUBLIC ${GTEST_INCLUDE_DIRS})
target_include_directories(${PRJ_TEST_NAME} PUBLIC ${LLVM_INCLUDE_DIRS})

target_link_libraries(${PRJ_TEST_NAME} PUBLIC ${GTEST_BOTH_LIBRARIES})  
target_link_libraries(${PRJ_TEST_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT})  

llvm_map_components_to_libnames(llvm_libs 
  core support asmparser analysis passes)

target_link_libraries(${PRJ_TEST_NAME} PUBLIC ${llvm_libs})

# setup dependency between unit test targets and their production code
add_dependencies(${PRJ_TEST_NAME} ${LIB_NAME})

# exclude unit test targets from main build
set_target_properties(${PRJ_TEST_NAME} PROPERTIES
  EXCLUDE_FROM_ALL TRUE)


# aggregate unit test targets under a pseudo-target
add_custom_target(unittests)

add_dependencies(unittests ${PRJ_TEST_NAME})


