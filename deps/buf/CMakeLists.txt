cmake_minimum_required(VERSION 3.23)

project(
  buf
  LANGUAGES C
  DESCRIPTION "Dynamic array library"
)

add_library(buf INTERFACE)
target_include_directories(buf SYSTEM INTERFACE include)
add_library(buf::buf ALIAS buf)
