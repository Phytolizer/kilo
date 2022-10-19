cmake_minimum_required(VERSION 3.23)

project(
  str
  LANGUAGES C
  DESCRIPTION "Sane string library for C"
)

include(PrependPath)

set(STR_SRC_REL str.c strtox.c)
prepend_path(STR_SRC_REL src/ STR_SRC)
add_library(str ${STR_SRC})
target_link_libraries(str PUBLIC hedley::hedley)
target_include_directories(str SYSTEM PUBLIC include)
add_library(str::str ALIAS str)
