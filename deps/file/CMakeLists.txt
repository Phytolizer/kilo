cmake_minimum_required(VERSION 3.23)

project(file LANGUAGES C)

add_library(file src/file.c)
target_link_libraries(file PUBLIC str::str sum::sum)
target_include_directories(file SYSTEM PUBLIC include)
add_library(file::file ALIAS file)
