#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "tinyFS.h"
#include "tinyFS_errno.h"
#include "libDisk.c"
#include "libTinyFS.c"
int main() {
   int file;
   file = openDisk("test.txt", 5096);
   tfs_mkfs("test.txt", 5096);
   closeDisk(file);
}
