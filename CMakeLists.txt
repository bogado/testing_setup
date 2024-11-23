cmake_minimum_required(VERSION 3.30)
project(test_parser VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(test_setup INTERFACE)

target_compile_features(test_setup INTERFACE cxx_std_23)
target_compile_options(test_setup INTERFACE -Wall -Wextra -Werror)
target_precompile_headers(test_setup INTERFACE all.hpp)

file(GLOB SOURCES LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

message(DEBUG "Found sources ${SOURCES}")
foreach(SOURCE IN LISTS SOURCES)
    string(REGEX REPLACE "(.*)\.cpp$" "\\1" NAME ${SOURCE})
    message(STATUS "Adding target ${NAME}")
    add_executable("${NAME}" "${SOURCE}")
    target_compile_features("${NAME}" PRIVATE cxx_std_23)
    target_link_libraries("${NAME}" PRIVATE test_setup)
endforeach()
