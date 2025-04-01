/*
Do not modify this file.
Make all of your changes to main.cpp instead.
*/

#include "disk.h"

#include <iostream>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

using std::cerr;
using std::endl;

struct disk
{
    int fd;
    int block_size;
    int nblocks;
};

struct disk *disk_open(const char *diskname, int nblocks)
{
    struct disk *d;

    d = new struct disk;
    if (!d)
        return 0;

    d->fd = open(diskname, O_CREAT | O_RDWR, 0777);
    if (d->fd < 0)
    {
        delete d;
        return 0;
    }

    d->block_size = BLOCK_SIZE;
    d->nblocks = nblocks;

    if (ftruncate(d->fd, d->nblocks * d->block_size) < 0)
    {
        close(d->fd);
        delete d;
        return 0;
    }

    return d;
}

void disk_write(struct disk *d, int block, const char *data)
{
    if (block < 0 || block >= d->nblocks)
    {
        cerr << "disk_write: invalid block #" << block << endl;
        abort();
    }

    int actual = pwrite(d->fd, data, d->block_size, block * d->block_size);
    if (actual != d->block_size)
    {
        cerr << "disk_write: failed to write block #" << block << ": " << strerror(errno) << endl;
        abort();
    }
}

void disk_read(struct disk *d, int block, char *data)
{
    if (block < 0 || block >= d->nblocks)
    {
        cerr << "disk_read: invalid block #" << block << endl;
        abort();
    }

    int actual = pread(d->fd, data, d->block_size, block * d->block_size);
    if (actual != d->block_size)
    {
        cerr << "disk_read: failed to read block #" << block << ": " << strerror(errno) << endl;
        abort();
    }
}

int disk_nblocks(struct disk *d)
{
    return d->nblocks;
}

void disk_close(struct disk *d)
{
    close(d->fd);
    delete d;
}
