cmake_minimum_required(VERSION 3.12...3.22)
project(kilo LANGUAGES C DESCRIPTION "A tiny text editor")

add_executable(kilo src/main.c)
target_compile_features(kilo PRIVATE c_std_23)
