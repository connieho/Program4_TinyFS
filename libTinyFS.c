#include <stdio.h>
#include <stdlib.h>
#include "libDisk.h"
#include "libTinyFS.h"
#define SUPERBLOCK 1
/* Makes a blank TinyFS file system of size nBytes on the file specified by ‘filename’. This function should use the emulated disk library to open the specified file, and upon success, format the file to be mountable. This includes initializing all data to 0x00, setting magic numbers, initializing and writing the superblock and inodes, etc. Must return a specified success/error code. */
int tfs_mkfs(char *filename, int nBytes){
   int file;
   file = openDisk(filename, nBytes);
   //check to see if opendisk was a success, error code for failure
   if (file < 0) {
      return file;
   }

   char *superblock = initSuperBlock(nBytes);
   writeBlock(file, 0, superblock); //writes the superblock to the file

   return -1;
}
 
/*Initializes the Superblock for the file system*/

char * initSuperBlock(int nBytes) {
   int size_disk;
   char* superblock;

   size_disk = nBytes/BLOCKSIZE;
   superblock = (char *)calloc(1, BLOCKSIZE) 
   *superblock = SUPERBLOCK; //Byte 0 is block type, using superblock macro
   *(superblock + 1) = 0x45; //Byte 1 is magic byte for status check
   //NOTE: Byte 2 is a pointer to another block, Byte 3 is an empty bytes as per spec
   return superblock;
}


/* tfs_mount(char *filename) “mounts” a TinyFS file system located within ‘filename’. tfs_unmount(void) “unmounts” the currently mounted file system. As part of the mount operation, tfs_mount should verify the file system is the correct type. Only one file system may be mounted at a time. Use tfs_unmount to cleanly unmount the currently mounted file system. Must return a specified success/error code. */
int tfs_mount(char *filename){
   return -1;
}
int tfs_unmount() {
   return -1;
}
 
/* Opens a file for reading and writing on the currently mounted file system. Creates a dynamic resource table entry for the file, and returns a file descriptor (integer) that can be used to reference this file while the filesystem is mounted. */
fileDescriptor tfs_openFile(char *name){

}
 
/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD) {
   return -1;
}
 
/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, to the file system. Sets the file pointer to 0 (the start of file) when done. Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size){
   return -1;
}
 
/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD){
   return -1;
}
 
/* reads one byte from the file and copies it to buffer, using the current file pointer location and incrementing it by one upon success. If the file pointer is already at the end of the file then tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer) {
   return -1;
}
 
/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset) {
   return -1;
}
