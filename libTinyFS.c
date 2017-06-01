#include <stdio.h>
#include <stdlib.h>
#include "libDisk.h"
#include "libTinyFS.h"
#include "tinyFS.h"

/* Makes a blank TinyFS file system of size nBytes on the file specified by ‘filename’. This function should use the emulated disk library to open the specified file, and upon success, format the file to be mountable. This includes initializing all data to 0x00, setting magic numbers, initializing and writing the superblock and inodes, etc. Must return a specified success/error code. */
int tfs_mkfs(char *filename, int nBytes){
   int file;
   file = openDisk(filename, nBytes);
   //check to see if opendisk was a success, error code for failure
   if (file < 0) {
      return file;
   }

   char *superblock = initSuperBlock();
   writeBlock(file, 0, superblock); //writes the superblock to the file
   
   initFS(file, nBytes);


   return -1;
}


/*Initializes all blocks in FS except for Superblock*/
void initFS(int file, int nBytes) {
   int num_blocks;
   char* newblock;


   num_blocks = nBytes/BLOCKSIZE;
   newblock = (char *)calloc(1, BLOCKSIZE);
   for (int idx = 1; idx < num_blocks; idx++) {
      *newblock = FREEBLOCK;
      *(newblock + 1) = 0x45;

      if (idx + 1 < num_blocks) {
         *(newblock + 2) = idx + 1; //make this the pointer to the next block
      }
      else {
         *(newblock + 2) = 0; //last block points to 0x00
      }

      //byte 3 remains empty
      writeBlock(file, idx, newblock);
   }

   free(newblock);
}


/*Initializes the Superblock for the file system*/
char* initSuperBlock() {
   char* superblock;


   superblock = (char *)calloc(1, BLOCKSIZE); 
   *superblock = SUPERBLOCK; //Byte 0 is block type, using superblock macro
   *(superblock + 1) = 0x45; //Byte 1 is magic byte for status check
   *(superblock + 2) = 1; //byte 2 holds the next free block, initially block 1
   //NOTE: Byte 2 is a pointer to another block, Byte 3 is an empty bytes as per spec
   
   return superblock;
}


/* tfs_mount(char *filename) “mounts” a TinyFS file system located within ‘filename’. tfs_unmount(void) “unmounts” the currently mounted file system. As part of the mount operation, tfs_mount should verify the file system is the correct type. Only one file system may be mounted at a time. Use tfs_unmount to cleanly unmount the currently mounted file system. Must return a specified success/error code. */
int tfs_mount(char *filename){
   //open the disk
   int disk_num;

   disk_num = openDisk(filename, 0); 
   if (disk_num < 0) {
      //return a mounting error
   }

   //CHANGE THE 1 IN CALLOC TO THE NUMBER OF FILES WE HAVE ON DISK
   file_table = (file_entry *)calloc(sizeof(file_entry), 1);
   //read in inodes and create resource table of existing files

   return -1;
}

int tfs_unmount() {
   return -1;
}
 
/* Opens a file for reading and writing on the currently mounted file system. Creates a dynamic resource table entry for the file, and returns a file descriptor (integer) that can be used to reference this file while the filesystem is mounted. */
fileDescriptor tfs_openFile(char *name){
   //create an inode for the file, if not existing
   //create a file extent for the file, if not existing
   //link the inode to the file, if not existing
   //add file as a dynamic resource table entry
   return -1;
}
 
/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD) {
   //remove dynamic resource table entry
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
