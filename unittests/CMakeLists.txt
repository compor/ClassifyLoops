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

target_sources(${PRJ_TEST_NAME} PUBLIC ${TEST_SOURCES})

target_include_directories(${PRJ_TEST_NAME} PUBLIC ${GTEST_INCLUDE_DIRS})

target_link_libraries(${PRJ_TEST_NAME} PUBLIC ${GTEST_BOTH_LIBRARIES})  
target_link_libraries(${PRJ_TEST_NAME} PUBLIC ${CMAKE_THREAD_LIBS_INIT})  

