cmake_minimum_required(VERSION 3.12...3.23)

project(
    kilo
    LANGUAGES C
    VERSION 0.1.0
    DESCRIPTION "A tiny text editor"
    HOMEPAGE_URL "https://github.com/Phytolizer/kilo"
)

add_executable(kilo kilo.c)
target_compile_features(kilo PRIVATE c_std_11)
