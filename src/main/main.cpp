#include <iostream>
using namespace std;

#include <unistd.h>
#include <sys/wait.h>

#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "stdlib.h"
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/syscall.h>

#include "sample_container.h"

#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/param.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>
#include <getopt.h>

#ifndef MS_MOVE
#define MS_MOVE 8192
#endif

#ifndef MNT_DETACH
#define MNT_DETACH       0x00000002	/* Just detach from the tree */
#endif

/* remove all files/directories below dirName -- don't cross mountpoints */
static int recursiveRemove(int fd) {
    struct stat rb;
    DIR *dir;
    int rc = -1;
    int dfd;

    if (!(dir = fdopendir(fd))) {
        throw;
        goto done;
    }

    /* fdopendir() precludes us from continuing to use the input fd */
    dfd = dirfd(dir);
    if (fstat(dfd, &rb)) {
        throw;
        goto done;
    }

    while (1) {
        struct dirent *d;
        int isdir = 0;

        errno = 0;
        if (!(d = readdir(dir))) {
            if (errno) {
                throw;
                goto done;
            }
            break; /* end of directory */
        }

        if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
            continue;
#ifdef _DIRENT_HAVE_D_TYPE
        if (d->d_type == DT_DIR || d->d_type == DT_UNKNOWN)
#endif
        {
            struct stat sb;

            if (fstatat(dfd, d->d_name, &sb, AT_SYMLINK_NOFOLLOW)) {
                throw;
                continue;
            }

            /* skip if device is not the same */
            if (sb.st_dev != rb.st_dev)
                continue;

            /* remove subdirectories */
            if (S_ISDIR(sb.st_mode)) {
                int cfd;

                cfd = openat(dfd, d->d_name, O_RDONLY);
                if (cfd >= 0)
                    recursiveRemove(cfd); /* it closes cfd too */
                isdir = 1;
            }
        }

        if (unlinkat(dfd, d->d_name, isdir ? AT_REMOVEDIR : 0))
            throw;
    }

    rc = 0; /* success */
done:
    if (dir)
        closedir(dir);
    else
        close(fd);
    return rc;
}

static int switchroot(const char *newroot) {
    /*  Don't try to unmount the old "/", there's no way to do it. */
    const char *umounts[] = {"/dev", "/proc", "/sys", "/run", NULL};
    int i;
    int cfd = -1;
    struct stat newroot_stat, oldroot_stat, sb;

    if (stat("/", &oldroot_stat) != 0) {
        throw;
        return -1;
    }

    if (stat(newroot, &newroot_stat) != 0) {
        throw;
        return -1;
    }

    for (i = 0; umounts[i] != NULL; i++) {
        char newmount[PATH_MAX];

        snprintf(newmount, sizeof(newmount), "%s%s", newroot, umounts[i]);

        if ((stat(umounts[i], &sb) == 0) && sb.st_dev == oldroot_stat.st_dev) {
            /* mount point to move seems to be a normal directory or stat failed */
            continue;
        }

        if ((stat(newmount, &sb) != 0) || (sb.st_dev != newroot_stat.st_dev)) {
            /* mount point seems to be mounted already or stat failed */
            umount2(umounts[i], MNT_DETACH);
            continue;
        }

        if (mount(umounts[i], newmount, NULL, MS_MOVE, NULL) < 0) {
            throw;
            umount2(umounts[i], MNT_FORCE);
        }
    }

    if (chdir(newroot)) {
        throw;
        return -1;
    }

    cfd = open("/", O_RDONLY);
    if (cfd < 0) {
        throw;
        goto fail;
    }

    if (mount(newroot, "/", NULL, MS_MOVE, NULL) < 0) {
        throw;
        goto fail;
    }

    if (chroot(".")) {
        throw;
        goto fail;
    }

    if (chdir("/")) {
        throw;
        goto fail;
    }

    return 0;

    // 	switch (fork()) {
    // 	case 0: /* child */
    // 	{
    // 		struct statfs stfs;
    //
    // 		if (fstatfs(cfd, &stfs) == 0 &&
    // 		    (F_TYPE_EQUAL(stfs.f_type, STATFS_RAMFS_MAGIC) ||
    // 		     F_TYPE_EQUAL(stfs.f_type, STATFS_TMPFS_MAGIC)))
    // 			recursiveRemove(cfd);
    // 		else {
    // 			throw;
    // 			close(cfd);
    // 		}
    // 		exit(EXIT_SUCCESS);
    // 	}
    // 	case -1: /* error */
    // 		break;
    //
    // 	default: /* parent */
    // 		close(cfd);
    // 		return 0;
    // 	}
    //
fail:
    if (cfd >= 0)
        close(cfd);
    return -1;
}

void run_container() {
    // make container directory
    system("mkdir -p " CONTAINER_DIR);

    // make mounts private
    mount("none", "/", nullptr, MS_REC | MS_PRIVATE, nullptr);

    // mount the fs into the container directory
    system("mount --bind " FS " " CONTAINER_DIR);

    // unshare
    unshare(
        0
        | CLONE_NEWNS
        // | CLONE_NEWPID
    );


    // switch to the actual container root
    switchroot(CONTAINER_DIR);

    // run shell program
#define SHELL_PATH "/bin/bash"
    execl(SHELL_PATH, SHELL_PATH, nullptr);

    cout << "shouldn't reach here" << endl;
    exit(1);
}

int main() {
    pid_t container_pid = fork();
    if (container_pid == 0) {
        run_container();
    }

    while (true) {
        int status;
        waitpid(container_pid, &status, 0);
        if (WIFEXITED(status)) break;
    }
    cout << "container finished" << endl;
}
