#include <iostream>
#include <cstdio>
#include <cstring>
using namespace std;

#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include "container.h"
extern struct container_t contaier;

#define ERR (-1)

bool run_container() {
    // make the container directory (if not exists)
    puts("making the container dir");
    char command[PATH_MAX];
    snprintf(command, PATH_MAX, "mkdir -p %s", contaier.top_dir);
    system(command);

    // seperate namespaces using the unshare syscall
    puts("seperating namespaces");
    if (unshare(
        0
        | CLONE_NEWNS // vfs mounts namesapce
        | CLONE_NEWPID // process namespace
        | CLONE_NEWUTS // uname namespace
        | CLONE_NEWIPC // ipc namespace
        | CLONE_NEWUSER // user namespace
        | CLONE_NEWNET // network amespace
        | CLONE_NEWTIME // time namespace
    ) == ERR) {
        perror("unshare");
        return false;
    }

    // because of the way PID namespace works, we must fork the current process to be able to execute another program

    pid_t child_pid = fork();
    if (child_pid == 0) {

        // make mounts private
        if (mount("none", "/", nullptr, MS_REC | MS_PRIVATE, nullptr) == ERR)  {
            perror("mout / private");
            return false;
        }

        // virtually mount the file system to the container directory
        puts("virtually mounting the file system");
        if (mount(contaier.fs_path, contaier.top_dir, nullptr, MS_BIND | MS_PRIVATE | MS_REC, nullptr) == ERR)  {
            perror("virtually mount fs");
            return false;
        }

        // switch to the root directory of the cotainer
        puts("changing the root directory for the container");
        if (chroot(contaier.top_dir) == ERR)  {
            perror("chroot");
            return false;
        }
        if (chdir("/") == ERR)  {
            perror("chdir");
            return false;
        }

        // remount some linux-provided virtual file systems
        // mount the procfs again
        puts("remounting procfs");
        mkdir("/proc", 0755);
        if (mount("proc", "/proc", "proc", 0, nullptr) == ERR) {
            perror("procfs mount");
            return false;
        }
        // mount the sysfs again
        puts("remounting sysfs");
        mkdir("/sys", 0755);
        if (mount("sysfs", "/sys", "sysfs", 0, nullptr) == ERR)  {
            perror("sysfs mount");
            return false;
        }

        // set hostname
#define HOST_NAME "your container"
        puts("setting container hostname");
        if (sethostname(HOST_NAME, strlen(HOST_NAME)) == ERR)  {
            perror("sethostname");
            return false;
        }

        // run a shell program
        puts("runnig shell program");
#define SHELL_PATH "/bin/sh"
        if (execl(SHELL_PATH, SHELL_PATH, nullptr) == ERR)  {
            perror("execl");
            return false;
        }

        // (shouldn't reach here)
        exit(1);
    }

    while (true) {
        int status;
        waitpid(child_pid, &status, 0);
        if (WIFEXITED(status)) break;
    }
    return true;
}

int main() {
    pid_t container_proc = fork();
    if (container_proc == 0) {
        puts("starting contianer");
        run_container();
        exit(0);
    }

    while (true) {
        int status;
        waitpid(container_proc, &status, 0);
        if (WIFEXITED(status)) break;
    }
    puts("container finished");
}
