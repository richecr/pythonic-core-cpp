cmake_minimum_required(VERSION 3.4...3.18)
project(pythonic_core)

add_subdirectory(pybind11)

# EXAMPLE_VERSION_INFO is defined by setup.py and passed into the C++ code as a
# define (VERSION_INFO) here.
target_compile_definitions(src/pythonic_core.cpp
                           PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})