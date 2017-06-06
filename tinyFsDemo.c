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
#include "libTinyFS.h"
int main() {
   int file;
   tfs_mkfs("test.txt", 5096);
   tfs_mount("test.txt");
   fileDescriptor tester = tfs_openFile("myfile");
   char buffer[100];
   buffer[0] = 0xDE;
   buffer[1] = 0xAD;
   tfs_writeFile(tester, buffer, sizeof(buffer)); 
   char h = 0x04;
   tfs_readByte(tester, &h);
   printf("%x\n", h);
   tfs_deleteFile(tester);
   tfs_unmount();
}
