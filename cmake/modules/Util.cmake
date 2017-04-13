# cmake file

function(fatal message_txt)
  message(FATAL_ERROR "${message_txt}")
endfunction()


function(debug message_txt)
  message(STATUS "[DEBUG] ${message_txt}")
endfunction()


macro(msg mode)
  message(${mode} "${PROJECT_NAME} ${ARGN}")
endmacro()


function(get_version version)
  execute_process(COMMAND git describe --tags --long --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE ver
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(${version} "${ver}" PARENT_SCOPE)
endfunction()

