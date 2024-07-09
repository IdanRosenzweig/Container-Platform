cmake_minimum_required(VERSION 3.28)
project(container_platform)

set(CMAKE_CXX_STANDARD 20)

add_executable(container_platform src/main/run_container.cpp
        src/abstract/features.cpp
        src/abstract/features.h
        src/main/sample_container.h)
