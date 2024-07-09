#!/bin/bash

FS=/home/user/Downloads/test_fs/
CONTAINER_DIR=/mnt/newroot

# make container directory (if not exists)
mkdir -p $CONTAINER_DIR

# allocate local file system and copy content
mount -n -t tmpfs -o size=500M none $CONTAINER_DIR
cp -rf $FS/* $CONTAINER_DIR

# cd into the container
cd $CONTAINER_DIR

echo "unshare -m"
unshare -m

echo "switching root"
pwd
switch_root . bin/sh