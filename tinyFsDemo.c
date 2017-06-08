#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "tinyFS.h"
#include "tinyFS_errno.h"
#include "libDisk.h"
#include "libTinyFS.h"

int main() {
   int file, intstring;
   char* buffer;
   char* mystring;
   char temp;
   timestamp* timer;

   printf("Making filesystem in test.txt of size 12800\n");
   tfs_mkfs("test.txt", 12800);
   printf("\n");

   printf("Mounting filesystem test.txt\n");
   tfs_mount("test.txt");
   printf("\n");
   
   printf("Opening file: test1\n");
   fileDescriptor test1 = tfs_openFile("test1");
   
   time_t currtime = time(0);
   while(time(0) < currtime + 2);

   printf("Opening file: test2\n");
   fileDescriptor test2 = tfs_openFile("test2");
   
   printf("Opening file: test3\n");
   fileDescriptor test3 = tfs_openFile("test3");
   printf("\n");

   printf("Writing 'MISSION CHAUNCETOBERFEST:   CLASSIFIED' to file: test1\n");
   buffer = (char *) calloc(100, 1);
   mystring = "MISSION CHAUNCETOBERFEST:   CLASSIFIED\n"; 
   memcpy(buffer, mystring, strlen(mystring) + 1);
   tfs_writeFile(test1, buffer, strlen(mystring)); 
   printf("\n");
   
   printf("Read string byte by byte back from the file test1\n");
   for (int idx = 0; idx < strlen(mystring) + 1; idx++) {
      tfs_readByte(test1, &temp);
      printf("%c", temp);
   }
   printf("\n");

   printf("Setting file pointer in test1 back to 2 and reading character\n");
   tfs_seek(test1, 2);
   tfs_readByte(test1, &temp);
   printf("%c\n", temp);
   printf("\n");

   printf("Modify file using writeByte and read string byte by byte back from the file test1\n");
   printf("File header changed from classified to unclassified\n");
   tfs_seek(test1, 26);
   tfs_writeByte(test1, 'U');
   tfs_writeByte(test1, 'N');
   while(time(0) < currtime + 4);
   tfs_seek(test1, 0);
   for (int idx = 0; idx < strlen(mystring) + 1; idx++) {
      tfs_readByte(test1, &temp);
      printf("%c", temp);
   }
   printf("\n");
   
   printf("Checking creation, modification, and access times for test1\n");
   timer = tfs_readFileInfo(test1);
   struct tm* time_info;
   char timebuffer[26];
   time_info = localtime(&(timer->creation));
   strftime(timebuffer, 26, "%Y-%m-%d %H:%M:%S\n", time_info);
   printf("Creation time: %s", timebuffer);
   time_info = localtime(&(timer->modification));
   strftime(timebuffer, 26, "%Y-%m-%d %H:%M:%S\n", time_info);
   printf("Modification time: %s", timebuffer);
   time_info = localtime(&(timer->access));
   strftime(timebuffer, 26, "%Y-%m-%d %H:%M:%S\n", time_info);
   printf("Access time: %s", timebuffer);
   printf("\n");
   
   tfs_readdir();
   printf("\n");

   printf("Rename test1 to Chaunce\n");
   tfs_readdir();
   tfs_rename("Chaunce", "test1");
   tfs_readdir();
   printf("\n");

   printf("deleting file test2\n");
   //printf("Freeblocks before: %d\n", free_blocks);
   tfs_deleteFile(test2);
   //printf("Freeblocks after: %d\n", free_blocks);
   printf("\n");

   tfs_readdir();
   printf("\n");

   printf("Closing file Chaunce\n");
   tfs_closeFile(test1);
   printf("\n");

   printf("Trying to read from test1, closed file\n");
   printf("Demo asserts that return result is an error, if not then prints the byte\n");
   int result = tfs_readByte(test1, &temp);
   if (result == ERROR_BADFILE) {
      printf("File not available\n");
   }
   else {
      printf("%c\n", temp);
   }
   printf("\n");


   printf("Unmounting filesystem test.txt\n");
   tfs_unmount();
   printf("\n");

   printf("Mounting and Unmounting file second.txt\n");
   tfs_mkfs("second.txt", 1024);
   tfs_mount("second.txt");
   tfs_unmount();
   printf("\n");

   printf("Mounting filesystem test.txt\n");
   tfs_mount("test.txt");
   printf("\n");
   
   printf("Opening file Chaunce");
   test1 = tfs_openFile("Chaunce");
   printf("\n");

   printf("Read string byte by byte back from the file test1\n");
   for (int idx = 0; idx < strlen(mystring) + 1; idx++) {
      tfs_readByte(test1, &temp);
      printf("%c", temp);
   }
   printf("\n");

   printf("Open and make test3 Read-Only\n");
   test3 = tfs_openFile("test3");
   tfs_makeRO("test3");
   result = tfs_writeFile(test3, "TEST123", strlen("TEST123") + 1);
   if (result == NO_WRITE_ACCESS) {
      printf("No Write Access\n");
   }
   printf("\n");

   printf("Make test3 Read Write\n");
   tfs_makeRW("test3");
   printf("Writing TEST123 to test3\n");
   result = tfs_writeFile(test3, "TEST123", strlen("TEST123") + 1);
   printf("Reading first byte from test3\n");
   tfs_readByte(test3, &temp);
   printf("%c", temp);
   printf("\n");
   
   printf("Unmounting test.txt\n");
   tfs_unmount();
}
