#define _DEFAULT_SOURCE /* For reallocarray */
#define _POSIX_C_SOURCE 200809L /* For openat, fstatat */

#include <limits.h> /* For SSIZE_MAX */
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "utility.h"

uintmax_t min_unsigned(uintmax_t x, uintmax_t y)
{
    return x < y ? x : y;
}

void* malloc_checked(size_t size)
{
    void *ret = malloc(size);
    if (!ret)
        err(1, "%s failed", "malloc");
    return ret;
}
void reallocarray_checked(void **ptr, size_t nmemb, size_t size)
{
    void *newp = reallocarray(*ptr, nmemb, size);
    if (!newp)
        err(1, "%s failed", "reallocarray");
    *ptr = newp;
}

char* strdup_checked(const char *s)
{
    char *ret = strdup(s);
    if (!ret)
        err(1, "%s failed", "strdup");
    return ret;
}

void msleep(uintmax_t msec)
{
    if (usleep(msec * 1000) && errno == EINVAL)
        err(1, "%s failed", "usleep");
}

int openat_checked(const char *dir, int dirfd, const char *path, int flags)
{
    int fd;
    do {
        fd = openat(dirfd, path, flags);
    } while (fd == -1 && errno == EINTR);

    if (fd == -1)
        err(1, "openat %s%s with %d failed", dir, path, flags);

    return fd;
}

ssize_t read_autorestart(int fd, void *buf, size_t count)
{
    ssize_t ret;
    do {
        ret = read(fd, buf, count);
    } while (ret == -1 && errno == EINTR);

    return ret;
}
ssize_t write_autorestart(int fd, const void *buf, size_t count)
{
    ssize_t ret;
    do {
        ret = write(fd, buf, count);
    } while (ret == -1 && errno == EINTR);

    return ret;
}

ssize_t readall(int fd, void *buffer, size_t len)
{
    size_t bytes = 0;
    for (ssize_t ret; bytes < len; bytes += ret) {
        ret = read_autorestart(fd, (char*) buffer + bytes, min_unsigned(len - bytes, SSIZE_MAX));
        if (ret == 0)
            break;
        if (ret == -1)
            return -1;
    }

    return bytes;
}
ssize_t asreadall(int fd, char **buffer, size_t *len)
{
    size_t bytes = 0;
    for (ssize_t ret; ; bytes += ret) {
        if (bytes == *len) {
            *len += 100;
            reallocarray_checked((void**) buffer, *len, sizeof(char));
        }

        ret = read_autorestart(fd, *buffer + bytes, min_unsigned(*len - bytes, SSIZE_MAX));
        if (ret == 0)
            break;
        if (ret == -1)
            return -1;
    }

    if (bytes == *len) {
        *len += 1;
        reallocarray_checked((void**) buffer, *len, sizeof(char));
    }

    (*buffer)[*len - 1] = '\0';

    return bytes;
}

const char* readall_as_uintmax(int fd, uintmax_t *val)
{
    /*
     * 20 bytes is enough for storing decimal string as large as (uint64_t) -1
     */
    char line[21];
    ssize_t sz = readall(fd, line, sizeof(line));
    if (sz == -1)
        return "read";
    if (sz == 0 || sz == sizeof(line) || line[sz - 1] != '\n') {
        errno = 0;
        return "readall_as_uintmax assumption";
    }
    line[sz - 1] = '\0';

    errno = 0;
    char *endptr;
    uintmax_t ret = strtoumax(line, &endptr, 10);
    if (*endptr != '\0') {
        errno = 0;
        return "readall_as_uintmax assumption";
    }
    if (errno == ERANGE)
        return "strtoumax";

    *val = ret;
    return NULL;
}

int isdir(const char *dir, int dirfd, const char *path)
{
    struct stat dir_stat_v;
    if (fstatat(dirfd, path, &dir_stat_v, 0) < 0)
        err(1, "%s failed on %s%s", "stat", dir, path);

    return S_ISDIR(dir_stat_v.st_mode);
}
