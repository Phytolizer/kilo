cmake_minimum_required(VERSION 3.23)

project(
  hedley
  LANGUAGES C
  DESCRIPTION "A C/C++ header to help move #ifdefs out of your code"
  HOMEPAGE_URL "https://github.com/nemequ/hedley"
)

add_library(hedley INTERFACE)
target_include_directories(hedley SYSTEM INTERFACE include)
add_library(hedley::hedley ALIAS hedley)
