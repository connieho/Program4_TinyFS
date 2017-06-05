#ifndef TINYFS_H
#define TINYFS_H

#define BLOCKSIZE 256
#define DEFAULT_DISK_SIZE 10240 
#define DEFAULT_DISK_NAME “tinyFSDisk”    
#define SUPERBLOCK 1 
#define INODE 2
#define FILE_EXTENT 3
#define FREEBLOCK 4

typedef int fileDescriptor;
#endif
