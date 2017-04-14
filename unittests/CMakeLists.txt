# cmake file

find_package(GTest)

if(NOT GTEST_FOUND)
  message(WARNING "Could not find GoogleTest. Skipping unit test configuration")

  return()
endif()

