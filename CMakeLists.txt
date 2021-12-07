cmake_minimum_required(VERSION 3.12...3.22)
project(
	kilo
	LANGUAGES C
	VERSION "0.0.1"
	DESCRIPTION "A tiny text editor"
)

configure_file("config.h.cin" "${PROJECT_BINARY_DIR}/config.h")

add_executable(kilo kilo.c)
target_compile_features(kilo PRIVATE c_std_23)
target_include_directories(kilo PRIVATE "${PROJECT_BINARY_DIR}")
