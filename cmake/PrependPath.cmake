macro(prepend_path PATHS PREFIX OUT_VARIABLE)
  foreach(_file ${${PATHS}})
    cmake_path(
      ABSOLUTE_PATH _file BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${PREFIX}
      OUTPUT_VARIABLE _file_rel
    )
    list(APPEND ${OUT_VARIABLE} ${_file_rel})
  endforeach()
endmacro()
