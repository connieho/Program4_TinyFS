#include <stdio.h>
#include <string.h>
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

   char *superblock = initSuperBlock(nBytes);
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
char* initSuperBlock(int nBytes) {
   char* superblock;
   int num_blocks = nBytes/BLOCKSIZE;

   superblock = (char *)calloc(1, BLOCKSIZE); 
   *superblock = SUPERBLOCK; //Byte 0 is block type, using superblock macro
   *(superblock + 1) = 0x45; //Byte 1 is magic byte for status check
   *(superblock + 2) = 1; //Byte 2: next free block, can change
   //NOTE: Byte 3 is an empty bytes as per spec
   *(superblock + 4) = num_blocks; //Byte 4: total number of blocks
   *(superblock + 5) = num_blocks - 1; //Byte 5: total number of free blocks
   *(superblock + 6) = 0; //Byte 6: total number of files
   return superblock;
}


/* tfs_mount(char *filename) “mounts” a TinyFS file system located within ‘filename’. tfs_unmount(void) “unmounts” the currently mounted file system. As part of the mount operation, tfs_mount should verify the file system is the correct type. Only one file system may be mounted at a time. Use tfs_unmount to cleanly unmount the currently mounted file system. Must return a specified success/error code. */
int tfs_mount(char *filename){
   //open the disk
   char sb_buffer[BLOCKSIZE];
   char inode_buffer[BLOCKSIZE];

   disk_num = openDisk(filename, 0); 
   if (disk_num < 0) {
      //return a mounting error
   }

   readBlock(disk_num, 0, sb_buffer);
   total_files =  sb_buffer[6];
   free_blocks = sb_buffer[5];
   file_table = (file_entry *)calloc(sizeof(file_entry), total_files);
   //start reading in inodes at byte offset 8
   for (int idx = 0; idx < total_files; idx++) {
      readBlock(disk_num, sb_buffer[idx + 8], inode_buffer);
      file_table[idx].fd = -1;
      file_table[idx].open = 0;
      file_table[idx].inode_block = sb_buffer[idx + 8];
      file_table[idx].file_block = inode_buffer[2]; //store the first file block number in byte 2
      memcpy(file_table[idx].name,"test", strlen("test")); //EDIT AFTER WE decide how we will store this in inode
      file_table[idx].file_offset = 0;
   }

   //create the necessary table for free_block inforation
   for (int idx = 0; idx < free_blocks; idx++) {

   }


   return -1;
}

int tfs_unmount() {
   return -1;
}
 
/* Opens a file for reading and writing on the currently mounted file system. Creates a dynamic resource table entry for the file, and returns a file descriptor (integer) that can be used to reference this file while the filesystem is mounted. */
fileDescriptor tfs_openFile(char *name){
   int existing = 0;

   for (int idx = 0; idx < total_files; idx++) {
      if (strcmp(file_table[idx].name, name) == 0) {
         existing = 1;
         if (file_table[idx].open == 0) {
            file_table[idx].open = 1;
            file_table[idx].fd = nextFD++;
         }
         return file_table[idx].fd;
      }
   }

   if (existing == 0) {
      free_block* inode = freeblock_head;
      free_block* file_extent = freeblock_head->next;
      freeblock_head = freeblock_head->next->next;
      
      file_table = realloc(file_table, sizeof(file_entry) * total_files);
      file_table[total_files - 1].open = 1;
      file_table[total_files - 1].fd = nextFD++;
      file_table[total_files - 1].inode_block = inode->block_number;
      file_table[total_files - 1].file_block = file_extent->block_number;
      memcpy(file_table[total_files - 1].name, name, strlen(name) + 1);
      
      return file_table[total_files - 1].fd;
   }

   //check to see if file is existing
   //if not existing, use freeblock to create inode, filextent for file
   //add inode block # to superblock
   //add file as a dynamic resource table entry that holds inode? and fd
   return ERROR_BADFILEOPEN;
}
 
/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD) {
   //remove dynamic resource table entry
   for (int idx = 0; idx < total_files; idx++) {
      if (file_table[idx].fd == FD) {
         if (file_table[idx].open == 1) {
            file_table[idx].open = 0;
            return 0;
         }
         else {
            return ERROR_BADFILECLOSE;
         }
      }
   }
   return ERROR_BADFILECLOSE;
}
 
/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, to the file system. Sets the file pointer to 0 (the start of file) when done. Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size){
   char *freeBuffer = (char *) calloc(1, BLOCKSIZE);
   int current_block_num, tempSize, numBlock, file_ext_num, inode, idx = 0;
   int next_block_num;
   free_block *newFile;

   // Find the corresponding fd that exist in file_table
   // return ERROR_BADFILE if FD is not found
   while(file_table[idx].fd != FD) {
      idx++;
      if (idx > total_files) {
         return ERROR_BADFILE;        
      }
   }
   
   // Find the inode block corresponding to the inode number
   readBlock(disk_num, file_table[idx].inode_block, freeBuffer);
   cuurent_block_num = freeBuffer[2];
   // Find the file extent corresponding to the one in inode_block
   readBlock(disk_num, freeBuffer[2], freeBuffer);

   //Empty File extent, write into the block
   if(!freeBuffer[2]) {
      freeBuffer[0] = FILE_EXTENT;
      freeBuffer[1] = 0x45;
      if (size <= 252) {
         memcpy(freeBuffer + 4, buffer, size);
         freeBuffer[2] = 0;
         writeBlock(disk_num, current_block_num, freeBuffer);
      } else {
         numBlock = ceil((double) size / 252.0) - 1;
         

         // grab the second free block
         for (idx = 0; idx < numBlock; idx++) {
            memcpy(freeBuffer + 4, buffer, size);
            tempSize = size - 252;
            free_block *temp = freeblock_head;
            freeblock_head = freeblock_head->next;
            freeBuffer[2] = temp->block_number;
            writeBlock(disk_num, current_block_num, freeBuffer);
         }
         memcpy(freeBuffer + 4, buffer, tempSize);
         freeBuffer[2] = 0;
         writeBlock(disk_num, current_block_num, freeBuffer);
      }
   } else {
        
   }
   
   
   
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
