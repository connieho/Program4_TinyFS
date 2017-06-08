#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "libDisk.h"
#include "libTinyFS.h"
#include "tinyFS.h"
#include "tinyFS_errno.h"

//TODO WRITEFILE SIZE ERRORS - MKFS IF DISK ALREADY MOUNTED, MOUNT IF DISK ALREADY MOUNTED, UNMOUNT IF NO DISK
//CURRENTLY MOUNTED, WRITE IF NOT ENOUGH FREE BLOCKS TO WRITE, OPENFILE IF NOT ENOUGH FREEBLOCKS,
//all functions if disk is not mounted
/* Makes a blank TinyFS file system of size nBytes on the file specified by ‘filename’. This function should use the emulated disk library to open the specified file, and upon success, format the file to be mountable. This includes initializing all data to 0x00, setting magic numbers, initializing and writing the superblock and inodes, etc. Must return a specified success/error code. */
int tfs_mkfs(char *filename, int nBytes){
   disk_num = openDisk(filename, nBytes);
   //check to see if opendisk was a success, error code for failure
   if (disk_num < 0) {
      return ERROR_OPENDISK;
   }

   char *superblock = initSuperBlock(nBytes);
   writeBlock(disk_num, 0, superblock); //writes the superblock to the file
   free(superblock);
   
   initFS(nBytes);

   return 0;
}


/*Initializes all blocks in FS except for Superblock*/
void initFS(int nBytes) {
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
      writeBlock(disk_num, idx, newblock);
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
   char free_buffer[BLOCKSIZE];
   nextFD = 0;

   disk_num = openDisk(filename, 0); 
   if (disk_num < 0) {
      //return a mounting error
   }

   //read in the superblock to sb_buffer
   readBlock(disk_num, 0, sb_buffer);
   total_files =  sb_buffer[6];
   free_blocks = sb_buffer[5];
   file_table = (file_entry *)calloc(sizeof(file_entry), total_files);
   //start reading in inodes at byte offset 8
   //each byte holds block number for inode
   for (int idx = 0; idx < total_files; idx++) {
      readBlock(disk_num, sb_buffer[idx + 8], inode_buffer);
      file_table[idx].fd = 0;
      file_table[idx].open = 0;
      file_table[idx].inode_block = sb_buffer[idx + 8];
      file_table[idx].file_block = inode_buffer[2]; //store the first file block number in byte 2
      memcpy(file_table[idx].name, inode_buffer + 5, 9); //EDIT AFTER WE decide how we will store this in inode
      file_table[idx].file_offset = 0;
   }

   //create the freeblock linked list
   free_block* current;
   free_block* last;
   if (free_blocks > 0) {
      current = (free_block*)calloc(sizeof(free_block), 1);
      current->block_number = sb_buffer[2];
      current->next = NULL;
      freeblock_head = current;
      last = current;
   }

   for (int idx = 1; idx < free_blocks; idx++) {
      readBlock(disk_num, last->block_number, free_buffer);
      current = (free_block*)calloc(sizeof(free_block), 1);
      current->block_number = free_buffer[2];
      current->next = NULL;
      last->next = current;
      last = current;
   }

   return 0;
}

int tfs_unmount() {
   char sb_buffer[BLOCKSIZE];
   free(file_table);
   readBlock(disk_num, 0, sb_buffer);
   sb_buffer[5] = free_blocks;
   sb_buffer[6] = total_files;
   sb_buffer[2] = freeblock_head->block_number;
   writeBlock(disk_num, 0, sb_buffer);

   free_block* temp;
   free_block* curr;
   curr = freeblock_head;
   while (curr != NULL) {
      temp = curr;
      curr = curr->next;
      free(temp);
   }
   free_blocks = 0;
   total_files = 0;
   disk_num = -1;

   closeDisk(disk_num);
   return 0;
}
 
/* Opens a file for reading and writing on the currently mounted file system. Creates a dynamic resource table entry for the file, and returns a file descriptor (integer) that can be used to reference this file while the filesystem is mounted. */
fileDescriptor tfs_openFile(char *name){
   char* buffer;
   int existing = 0;
   timestamp* filetime;

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
      ++total_files;
      
      free_block* inode = freeblock_head;
      free_block* file_extent = freeblock_head->next;
      freeblock_head = freeblock_head->next->next;
      free_blocks -= 2;

      file_table = realloc(file_table, sizeof(file_entry) * total_files);
      file_table[total_files - 1].open = 1;
      file_table[total_files - 1].fd = nextFD++;
      file_table[total_files - 1].inode_block = inode->block_number;
      file_table[total_files - 1].file_block = file_extent->block_number;
      memcpy(file_table[total_files - 1].name, name, strlen(name) + 1);
      
      buffer = (char *)calloc(BLOCKSIZE, 1);
      filetime = (timestamp *)calloc(1, sizeof(timestamp));
      buffer[0] = INODE;
      buffer[1] = 0x45;
      buffer[2] = file_extent->block_number;
      buffer[3] = 0x00;
      memcpy(buffer + 5, name, strlen(name) + 1);
      
      buffer[14] = 0x03;


      filetime->creation = time(NULL);
      filetime->modification = filetime->creation;
      filetime->access = filetime->creation;
      memcpy(buffer + 15, filetime, sizeof(timestamp));
      writeBlock(disk_num, inode->block_number, buffer);
      
      readBlock(disk_num, 0, buffer);
      buffer[total_files + 7] = inode->block_number;
      buffer[5] = free_blocks; 
      writeBlock(disk_num, 0, buffer);
      
      free(buffer);
      free(filetime);
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
            accessFile(file_table[idx].inode_block);
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
   // Error checking: RW access, disk open, have enough freeBlock
   char *freeBuffer;
   int current_block_num, tempSize, numBlock, file_ext_num, inode, idx = 0;
   int i, next_block_num;
   free_block *newFile;

   numBlock = ceil((double) size / 252.0) - 1;

   // Find the corresponding fd that exist in file_table
   // return ERROR_BADFILE if FD is not found
   while(file_table[idx].fd != FD) {
      idx++;
      if (idx > total_files) {
         return ERROR_BADFILE;        
      }
   }
   
   freeBuffer = (char *) calloc(1, BLOCKSIZE);
   // Find the inode block corresponding to the inode number
   readBlock(disk_num, file_table[idx].inode_block, freeBuffer);

   if (freeBuffer[RW] != 0x03) {
      free(freeBuffer);
      return NO_WRITE_ACCESS;
   }

   current_block_num = freeBuffer[2];
   //modification time
   modifyFile(file_table[idx].inode_block);
   
   freeBuffer[3] = numBlock + 1;
   writeBlock(disk_num, file_table[idx].inode_block, freeBuffer);
   // Find the file extent corresponding to the one in inode_block
   readBlock(disk_num, freeBuffer[2], freeBuffer);

   //Empty File extent, write into the block
   i = numBlock;
   while (freeBuffer[2] != 0) {
      readBlock(disk_num, i++, freeBuffer);
   }

   freeBuffer[0] = FILE_EXTENT;
   freeBuffer[1] = 0x45;
   if (size <= 252) {
      memcpy(freeBuffer + 4, buffer, size);
      freeBuffer[2] = 0;
      writeBlock(disk_num, current_block_num, freeBuffer);
   } else {
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
   file_table[idx].file_offset = 0;
   
   return WRITE_SUCCESS;
} 
/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD){
   int idx, last_head, current_block;
   char readBuffer[BLOCKSIZE];
   char *freeBuffer;
   free_block* freeEntry;
   freeBuffer = (char *)calloc(1, BLOCKSIZE);
   freeBuffer[0] = FREEBLOCK;
   freeBuffer[1] = 0x45;
   idx = 0; 
   while (idx < total_files && file_table[idx].fd != FD) {
      ++idx;
   }
   if (idx >= total_files) {
      return ERROR_BADFILE;
   }
   
   readBlock(disk_num, file_table[idx].inode_block, readBuffer);
   if (readBuffer[RW] != 0x03) {
      return NO_WRITE_ACCESS;
   }
   
   while (readBuffer[2] != 0) {
      current_block = readBuffer[2];
      freeEntry = (free_block *)calloc(1, sizeof(file_entry));
      freeEntry->next = freeblock_head;
      last_head = freeblock_head->block_number;
      freeEntry->block_number = current_block;
      freeblock_head = freeEntry;
      freeBuffer[2] = last_head;
      readBlock(disk_num, current_block, readBuffer);
      writeBlock(disk_num, current_block, freeBuffer);
      ++free_blocks;
   }
   
   current_block = file_table[idx].inode_block;
   freeEntry = (free_block *)calloc(1, sizeof(file_entry));
   freeEntry->next = freeblock_head;
   last_head = freeblock_head->block_number;
   freeEntry->block_number = current_block;
   freeblock_head = freeEntry;
   freeBuffer[2] = last_head;
   writeBlock(disk_num, current_block, freeBuffer);
   ++free_blocks;

   readBlock(disk_num, 0, readBuffer);
   for (int sbIdx = 8; sbIdx < total_files + 8; sbIdx++) {
      if (readBuffer[sbIdx] == current_block) {
         readBuffer[sbIdx] = readBuffer[total_files + 7];
         readBuffer[total_files + 7] = 0x00;
      }
   }

   --total_files;
   readBuffer[6] = total_files;
   readBuffer[5] = free_blocks;
   readBuffer[2] = current_block;
   writeBlock(disk_num, 0, readBuffer);
   free(freeBuffer);
  
   memcpy(file_table + idx, file_table + total_files, sizeof(file_entry));
   file_table = realloc(file_table, sizeof(file_entry) * total_files);

   //remove file from table
   return 0;
}
 
/* reads one byte from the file and copies it to buffer, using the current file pointer location and incrementing it by one upon success. If the file pointer is already at the end of the file then tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer) {
   int idx, filesize, success, blockNum;
   char readBuffer[BLOCKSIZE];
   idx = 0;
   while (idx < total_files && file_table[idx].fd != FD) {
      ++idx;
   }
   readBlock(disk_num, file_table[idx].inode_block, readBuffer);
   filesize = readBuffer[3] * 252;
   if (file_table[idx].file_offset <= filesize) {
      blockNum = floor((double) file_table[idx].file_offset / 252.0);
      while(blockNum-- >= 0) {
         readBlock(disk_num, readBuffer[2], readBuffer);
      }
      *buffer = readBuffer[4 + file_table[idx].file_offset++];
      accessFile(file_table[idx].inode_block);
      success = 0;
   }
   else {
      success =  END_OF_FILE;
   }
   return success;
}

int tfs_writeByte(fileDescriptor FD, unsigned char data) {
   int idx, filesize, success, blockNum, current_block;
   char readBuffer[BLOCKSIZE];
   idx = 0;
   while (idx < total_files && file_table[idx].fd != FD) {
      ++idx;
   }
   readBlock(disk_num, file_table[idx].inode_block, readBuffer);
   if (readBuffer[RW] != 0x03) {
      return NO_WRITE_ACCESS;
   }
   filesize = readBuffer[3] * 252;
   if (file_table[idx].file_offset <= filesize) {
      blockNum = floor((double) file_table[idx].file_offset / 252.0);
      while(blockNum-- >= 0) {
         current_block = readBuffer[2];
         readBlock(disk_num, readBuffer[2], readBuffer);
      }
      readBuffer[4 + file_table[idx].file_offset++] = data;
      modifyFile(file_table[idx].inode_block);
      writeBlock(disk_num, current_block, readBuffer);
      success = 0;
   }
   else {
      success =  END_OF_FILE;
   }
   return success;
}

int tfs_rename(char *newName, char *oldName) {
   int idx = 0;
   char buffer[BLOCKSIZE];

   if (strlen(newName) > 8)
      return ERROR_RENAME_FAILURE;

   if (strcmp("/", oldName) == 0)
      return ERROR_RENAME_FAILURE;

   if (disk_num < 0)
      return ERROR_BADREAD; 

   modifyFile(file_table[idx].inode_block);
   while (strcmp(file_table[idx].name, oldName) != 0) {
      idx++;
      if(idx > total_files)
         return ERROR_BADFILE;
   }

   strcpy(file_table[idx].name, newName);
   readBlock(disk_num, file_table[idx].inode_block, buffer);
   
   memcpy(buffer + 5, newName, strlen(newName) + 1);
   writeBlock(disk_num, file_table[idx].inode_block, buffer);
     
   return RENAME_SUCCESS;
}

int tfs_readdir() {
   int idx = 0;

   printf("********** List of Files and Directories **********\n");
   while (idx < total_files) {
      printf("%s\n", file_table[idx++].name);
   }
   
   printf("**********            Done               **********\n");
   return READDIR_SUCCESS;
}
 
/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset) {
   int file_size, code, currentBlock, idx = 0; 

   //inode byte 3 will be file pointer
   while(file_table[idx].fd != FD) {
      idx++;
      if (idx > total_files) {
         return ERROR_BADFILE;        
      }
   }
   char *freeBuffer = (char *)calloc(1, BLOCKSIZE);
   
   readBlock(disk_num, file_table[idx].inode_block, freeBuffer);
   if (offset < freeBuffer[3] * 252) {
      code = 0;
      file_table[idx].file_offset = offset;
   } 
   else {
      offset = 0;
      code = ERROR_BADFILE;
   }
   free(freeBuffer);

   return SEEK_SUCCESS;
}

int tfs_makeRO(char *name) {
   int existing;
   existing = 0;
   char* buffer;
   buffer = (char *) calloc(1, BLOCKSIZE);
   for (int idx = 0; idx < total_files; idx++) {
      if (strcmp(file_table[idx].name, name) == 0) {
         existing = 1;
         readBlock(disk_num, file_table[idx].inode_block, buffer);
         buffer[14] = 0x01;
         writeBlock(disk_num, file_table[idx].inode_block, buffer);
      }
   }
   if (existing == 0) {
      return ERROR_BADFILE;
   }
   return 0;
}

int tfs_makeRW(char *name) {
   int existing;
   existing = 0;
   char* buffer;
   buffer = (char *) calloc(1, BLOCKSIZE);
   for (int idx = 0; idx < total_files; idx++) {
      if (strcmp(file_table[idx].name, name) == 0) {
         existing = 1;
         readBlock(disk_num, file_table[idx].inode_block, buffer);
         buffer[14] = 0x03;
         writeBlock(disk_num, file_table[idx].inode_block, buffer);
      }
   }
   if (existing == 0) {
      return ERROR_BADFILE;
   }
   return 0;

}

timestamp* tfs_readFileInfo(fileDescriptor FD) {
   int idx = 0; 

   while(file_table[idx].fd != FD) {
      idx++;
      if (idx > total_files) {
         return NULL;        
      }
   }
   char *buffer = (char *)calloc(1, BLOCKSIZE);
   timestamp* time = (timestamp *) calloc(1, sizeof(timestamp));
   readBlock(disk_num, file_table[idx].inode_block, buffer);
   memcpy(time, buffer + 15, sizeof(timestamp));
   free(buffer);
   return time;
}

void accessFile(int inode) {
   char* buffer;
   timestamp* filetime;
   buffer = (char *)calloc(BLOCKSIZE, 1);
   readBlock(disk_num, inode, buffer);
   filetime = (timestamp *)calloc(sizeof(timestamp), 1);
   memcpy(filetime, buffer + 15, sizeof(timestamp));
   filetime->access = time(NULL);
   memcpy(buffer + 15, filetime, sizeof(timestamp));
   writeBlock(disk_num, inode, buffer);
   free(buffer);
   free(filetime);
}

void modifyFile(int inode) {
   char* buffer;
   timestamp* filetime;
   buffer = (char *)calloc(BLOCKSIZE, 1);
   readBlock(disk_num, inode, buffer);
   filetime = (timestamp *)calloc(sizeof(timestamp), 1);
   memcpy(filetime, buffer + 15, sizeof(timestamp));
   filetime->modification = time(NULL);
   filetime->access = filetime->modification;
   memcpy(buffer + 15, filetime, sizeof(timestamp));
   writeBlock(disk_num, inode, buffer);
   free(buffer);
   free(filetime);
}
