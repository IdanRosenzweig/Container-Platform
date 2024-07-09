//
// Created by user on 7/9/24.
//

#include "features.h"

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "stdlib.h"

#include "sched.h"

#define ERR (-1)

void isolate_fs() {
}

void isolate_processes() {

    system("mount -t proc proc /proc");
}

void isolate_ipc() {
}

void isolate_users() {
}

void isolate_network() {
}

void isolate_hostname() {
    if (unshare(CLONE_NEWUTS) == ERR) perror("unshare");

    const char* new_hostname = "idan host";
    if (sethostname(new_hostname, strlen(new_hostname)) == ERR) perror("sethostname");
}
