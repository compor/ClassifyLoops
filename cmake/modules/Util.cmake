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


function(attach_compilation_db_command trgt)
  if(NOT TARGET ${trgt})
    fatal("cannot attach custom command to non-target: ${trgt}")
  endif()

  set(file "compile_commands.json")

  add_custom_command(TARGET ${trgt} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E copy_if_different ${file} "${CMAKE_SOURCE_DIR}/${file}"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    VERBATIM)
endfunction()
