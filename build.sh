#!/bin/bash

PROJECT_DIR=.
TARGET_BUILD_DIR=$PROJECT_DIR/build
CMAKE_BUILD_DIR=$PROJECT_DIR/cmake_build

if [ $# != 1 ]; then
  echo "usage: ./build.sh [container_path]"
  exit 1
fi

CONTAINER=$(realpath $1)
echo $CONTAINER

if ! test -f $CONTAINER; then
  echo "file doesn't exist"
  exit 1
fi

# build target
cmake -S $PROJECT_DIR -B $CMAKE_BUILD_DIR -DCONTAINER_PATH=$CONTAINER
cmake --build $CMAKE_BUILD_DIR --target container_platform

# make build dir if not exists
if ! test -d $TARGET_BUILD_DIR; then
  mkdir -p $TARGET_BUILD_DIR
fi

# copy output to build dir
cp $CMAKE_BUILD_DIR/container_platform $TARGET_BUILD_DIR/

exit 0