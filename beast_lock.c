#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "beast_lock.h"
#include "beast_log.h"


static inline int __do_lock(beast_locker_t *locker, int type)
{ 
    int ret, tries = 10;
    struct flock lock;

    lock.l_type = type;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 1;
    lock.l_pid = 0;

    do {
        ret = fcntl(locker->fd, F_SETLKW, &lock);
        tries--;
    } while (ret < 0 && tries > 0 &&
             (errno == EINTR || errno == EAGAIN));

    return ret;
}


beast_locker_t *beast_locker_create(char *path)
{
    beast_locker_t *locker;
    int fd;

    locker = malloc(sizeof(beast_locker_t));
    if (!locker) {
        goto failed;
    }

    fd = open(path, O_RDWR|O_CREAT, 0666);
    if (fd == -1) {
        free(locker);
        goto failed;
    }

    locker->fd = fd;
    locker->path = strdup(path);

    if (!locker->path) {
        close(fd);
        free(locker);
        goto failed;
    }

    return locker;

failed:
    beast_write_log(beast_log_notice,
              "beast_locker_create() failed errno(%d)", errno);
    return NULL;
}


int beast_locker_wrlock(beast_locker_t *locker)
{   
    if (__do_lock(locker, F_WRLCK) < 0) {
        beast_write_log(beast_log_notice,
              "beast_locker_wrlock() failed errno(%d)", errno);
        return -1;
    }
    return 0;
}


int beast_locker_rdlock(beast_locker_t *locker)
{   
    if (__do_lock(locker, F_RDLCK) < 0) {
        beast_write_log(beast_log_notice,
              "beast_locker_rdlock() failed errno(%d)", errno);
        return -1;
    }
    return 0;
}


int beast_locker_unlock(beast_locker_t *locker)
{   
    if (__do_lock(locker, F_UNLCK) < 0) {
        beast_write_log(beast_log_notice,
              "beast_locker_unlock() failed errno(%d)", errno);
        return -1;
    }
    return 0;
}


void beast_locker_destroy(beast_locker_t *locker)
{
    close(locker->fd);
    free(locker->path);
    free(locker);
}
