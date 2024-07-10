cmake_minimum_required(VERSION 3.25)
project(container_platform LANGUAGES C)

set(CMAKE_CXX_STANDARD 20)

# include project root directory
include_directories(${PROJECT_SOURCE_DIR})

add_executable(container_platform
        src/main.c
        samples/sample_container.h
)

# add the container struct
set_source_files_properties(${CONTAINER_PATH} PROPERTIES LANGUAGE C)
target_sources(container_platform PRIVATE ${CONTAINER_PATH})