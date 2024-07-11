#ifndef CONTAINER_H
#define CONTAINER_H

#include <linux/limits.h>
#include <limits.h>

struct container_t {
    // the top directory to put the container in
    char top_dir[PATH_MAX];

    // path to the container file system
    char fs_path[PATH_MAX];

    // hostname for the container
    char hostame[HOST_NAME_MAX];

    // the initial program to execute in the container
    char init_program[PATH_MAX];
};

#endif //CONTAINER_H
