cmake_minimum_required(VERSION 3.23)

project(sum LANGUAGES C)

add_library(sum INTERFACE)
target_include_directories(sum SYSTEM INTERFACE include)
add_library(sum::sum ALIAS sum)
