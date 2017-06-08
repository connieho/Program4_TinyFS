#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "libDisk.h"
#include "libTinyFS.h"
#include "tinyFS.h"
#include "tinyFS_errno.h"

//TODO
//CURRENTLY MOUNTED, WRITE IF NOT ENOUGH FREE BLOCKS TO WRITE, OPENFILE IF NOT ENOUGH FREEBLOCKS,
//all functions if disk is not mounted
/* Makes a blank TinyFS file system of size nBytes on the file specified by ‘filename’. This function should use the emulated disk library to open the specified file, and upon success, format the file to be mountable. This includes initializing all data to 0x00, setting magic numbers, initializing and writing the superblock and inodes, etc. Must return a specified success/error code. */
int tfs_mkfs(char *filename, int nBytes){
   // Get the disk number where filename is reside in
   disk_num = openDisk(filename, nBytes);

   if(mounted)
      return ERROR_ALREADY_MOUNTED;

   //check to see if opendisk was a success, error code for failure
   if (disk_num < 0) {
      return ERROR_OPENDISK;
   }

   // Initialize superBlock
   char *superblock = initSuperBlock(nBytes);
   // Write SB to the file
   writeBlock(disk_num, 0, superblock);
   free(superblock);
   
   // Initialize File System
   initFS(nBytes);
   disk_num = -1;

   return MAKEFS_SUCCESS;
}


/*Initializes all blocks in FS except for Superblock*/
void initFS(int nBytes) {
   int num_blocks, idx;
   char* newblock = (char *)calloc(1, BLOCKSIZE);

   // Find number of blocks needed
   num_blocks = nBytes/BLOCKSIZE;

   // Initialize the block as a free block linked list and update if needed
   for (idx = 1; idx < num_blocks; idx++) {
      *newblock = FREEBLOCK;
      *(newblock + 1) = 0x45;

      if ((idx + 1) < num_blocks) {
         *(newblock + 2) = idx + 1; //make this the pointer to the next block
      }
      else {
         *(newblock + 2) = 0; //last block points to 0x00
      }

      // byte 3 remains empty
      // Write the newBlock onto disk
      writeBlock(disk_num, idx, newblock);
   }

   free(newblock);
}


/*Initializes the Superblock for the file system*/
char* initSuperBlock(int nBytes) {
   char* superblock = (char *)calloc(1, BLOCKSIZE); 
   int num_blocks = nBytes/BLOCKSIZE;

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
   char sb_buffer[BLOCKSIZE];
   char inode_buffer[BLOCKSIZE];
   char free_buffer[BLOCKSIZE];
   int idx = 0;
   nextFD = 0;


   if(mounted)
      return ERROR_ALREADY_MOUNTED;

   //open the disk, return BAD_MOUNT if error occurs
   disk_num = openDisk(filename, 0); 
   if (disk_num < 0) {
      return BAD_MOUNT;
   }

   //read in the superblock to sb_buffer
   readBlock(disk_num, 0, sb_buffer);
   total_files =  sb_buffer[6];
   free_blocks = sb_buffer[5];

   file_table = (file_entry *)calloc(sizeof(file_entry), total_files);
   //start reading in inodes at byte offset 8
   //each byte holds block number for inode
   for (idx = 0; idx < total_files; idx++) {
      readBlock(disk_num, sb_buffer[idx + 8], inode_buffer);
      file_table[idx].fd = 0;
      file_table[idx].open = 0;
      file_table[idx].inode_block = sb_buffer[idx + 8];
      // Store the first file block number in byte2
      file_table[idx].file_block = inode_buffer[2];
      memcpy(file_table[idx].name, inode_buffer + 5, 9); 
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

   // update the last freeblock to indicate it is the end.
   for (idx = 1; idx < free_blocks; idx++) {
      readBlock(disk_num, last->block_number, free_buffer);
      current = (free_block*)calloc(sizeof(free_block), 1);
      current->block_number = free_buffer[2];
      current->next = NULL;
      last->next = current;
      last = current;
   }

   mounted = 1;

   return MOUNT_SUCCESS;
}

// Cleanly unmount the current mounted file system 
int tfs_unmount() {
   char sb_buffer[BLOCKSIZE];

   if(disk_num < 0)
      return ERROR_UNMOUNT_FAIL;

   // Free the file_table
   free(file_table);

   // Read in the disk block to sb_buffer
   readBlock(disk_num, 0, sb_buffer);
   // Update all the fields to original starting point
   sb_buffer[5] = free_blocks;
   sb_buffer[6] = total_files;
   sb_buffer[2] = freeblock_head->block_number;
   // Write back the buffer to disk (reinitialize)
   writeBlock(disk_num, 0, sb_buffer);

   // Create an empty linked list
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

   closeDisk(disk_num);
   disk_num = -1;
   mounted = 0;

   return UNMOUNT_SUCCESS;
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
   char *freeBuffer = (char *) calloc(1, BLOCKSIZE);
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
   
   if(!file_table[idx].open) {
      return FILE_NOT_OPEN;
   }
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
   char *freeBuffer = (char *)calloc(1, BLOCKSIZE);
   free_block* freeEntry;

   freeBuffer[0] = FREEBLOCK;
   freeBuffer[1] = 0x45;
   idx = 0; 
   while (idx < total_files && file_table[idx].fd != FD) {
      ++idx;
      if (idx >= total_files) {
         return ERROR_BADFILE;
      }
   }

   // Check if the file open for operation
   if(!file_table[idx].open) {
      return FILE_NOT_OPEN;
   }
   
   readBlock(disk_num, file_table[idx].inode_block, readBuffer);
   // Check the RW access for the file, return if READ_ONLY
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
   return DELETE_SUCCESS;
}
 
/* reads one byte from the file and copies it to buffer, using the current file pointer location and incrementing it by one upon success. If the file pointer is already at the end of the file then tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer) {
   int idx, filesize, success, blockNum;
   char readBuffer[BLOCKSIZE];
   idx = 0;
   while (file_table[idx].fd != FD) {
      ++idx;
      if (idx >= total_files)
         return ERROR_BADFILE;
   }
   if (file_table[idx].open == 0) {
      return FILE_NOT_OPEN;
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
   while (file_table[idx].fd != FD) {
      idx++;
      if(idx >= total_files)
         return ERROR_BADFILE;
   }
   if(!file_table[idx].open) {
      return FILE_NOT_OPEN;
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

// Rename the old file name to newName
int tfs_rename(char *newName, char *oldName) {
   int idx = 0;
   char buffer[BLOCKSIZE];

   // Check if newName is greater than 8 (support size)
   if (strlen(newName) > 8)
      return ERROR_RENAME_FAILURE;

   // Check if the oldName is root directory or not. If so, cannot change
   if (strcmp("/", oldName) == 0)
      return ERROR_RENAME_FAILURE;

   if (disk_num < 0)
      return ERROR_BADREAD; 

   // Since we change the filename, modification and access time will be
   // updated 
   modifyFile(file_table[idx].inode_block);
   // Find the file in the system with oldName
   while (strcmp(file_table[idx].name, oldName) != 0) {
      idx++;
      if(idx >= total_files)
         return ERROR_BADFILE;
   }
   // Return FILE_NOT_OPEN if file is not open for write
   if(!file_table[idx].open) {
      return FILE_NOT_OPEN;
   }   

   // Change the oldname in file_table to newName
   strcpy(file_table[idx].name, newName);
   // Read the inodeBlock to buffer
   readBlock(disk_num, file_table[idx].inode_block, buffer);
   // If READ Only, returns NO_WRITE_ACCESS
   // FileName will not modify
   if (buffer[RW] != 0x03) {
      free(buffer);
      return NO_WRITE_ACCESS;
   }
 
   // Push the changes in buffer back to inode block.  
   memcpy(buffer + 5, newName, strlen(newName) + 1);
   writeBlock(disk_num, file_table[idx].inode_block, buffer);
     
   return RENAME_SUCCESS;
}

// Print out a list of directories and files of the file system
int tfs_readdir() {
   int idx = 0;

   printf("********** List of Files and Directories **********\n");
   // Loop through the file_table and print all the files/ directories' names
   while (idx < total_files) {
      printf("%s\n", file_table[idx++].name);
   }
   
   printf("**********            Done               **********\n");
   // Return success when finished
   return READDIR_SUCCESS;
}
 
/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset) {
   int file_size, code, currentBlock, idx = 0; 
   char *freeBuffer = (char *)calloc(1, BLOCKSIZE);

   //inode byte 3 will be file pointer
   while(file_table[idx].fd != FD) {
      idx++;
      if (idx >= total_files) {
         return ERROR_BADFILE;        
      }
   }
   
   readBlock(disk_num, file_table[idx].inode_block, freeBuffer);
   // Check if offset is greater than the size * 252 byte
   // If so, return BADFILE, since offset cannot be greater than the file size
   // else set file_offset to the offset that was passed in
   if (offset < freeBuffer[3] * 252) {
      code = 0;
      file_table[idx].file_offset = offset;
   } 
   else {
      offset = 0;
      code = ERROR_BADFILE;
   }
   free(freeBuffer);

   // return 0 if success, BADFILE if error occurs.
   return code;
}

// Change the file READRITE ACCESS to Read Only
int tfs_makeRO(char *name) {
   int idx, existing = 0;
   char* buffer = (char *) calloc(1, BLOCKSIZE);

   // Loop through the file system to find the file with corresponding name
   for (idx = 0; idx < total_files; idx++) {
      if(idx < total_files) {
         if (strcmp(file_table[idx].name, name) == 0) {
            existing = 1;
            // Read inode block into buffer
            readBlock(disk_num, file_table[idx].inode_block, buffer);
            // Change the READWRITE Byte to READ only
            buffer[14] = 0x01;
            //Write buffer back to the inode block
            writeBlock(disk_num, file_table[idx].inode_block, buffer);
         }
      } else {
         return ERROR_BADFILE;
      }
   }
   // Return BADFILE if file never exist
   if (existing == 0) {
      return ERROR_BADFILE;
   }
   return 0;
}

// Change the file READWRITE Access to Read and Write
int tfs_makeRW(char *name) {
   int existing = 0;
   char* buffer = (char *) calloc(1, BLOCKSIZE);

   // Loop through all the file in the system to find matching file name
   // return BADFILE if file never found
   for (int idx = 0; idx < total_files; idx++) {
      if(idx < total_files) {
         if (strcmp(file_table[idx].name, name) == 0) {
            existing = 1;
            
            // Read the inode block to buffer
            readBlock(disk_num, file_table[idx].inode_block, buffer);
            // Change the Readwrite byte to RW
            buffer[14] = 0x03;
            // Write the buffer back to inode Block
            writeBlock(disk_num, file_table[idx].inode_block, buffer);
         }
      } else {
         return ERROR_BADFILE;
      }
   }

   // if file never exist, return BADFILE
   if (existing == 0) {
      return ERROR_BADFILE;
   }
   return 0;

}

//tfs_readFileInfo returns a timestamp struct with  creation time or all info 
timestamp* tfs_readFileInfo(fileDescriptor FD) {
   //Initialization
   int idx = 0; 
   char *buffer = (char *)calloc(1, BLOCKSIZE);
   timestamp* time = (timestamp *) calloc(1, sizeof(timestamp));

   // Find the corresponding file with FD, return BADFILE if not found
   while(file_table[idx].fd != FD) {
      idx++;
      if(idx >= total_files)
         break;
   }

   // Get the inode block and put in buffer
   readBlock(disk_num, file_table[idx].inode_block, buffer);

   // Get the all the timestamp(create, access, modification)
   memcpy(time, buffer + 15, sizeof(timestamp));
   free(buffer);

   return time;
}

void accessFile(int inode) {
   // Initialization
   char* buffer = (char *)calloc(BLOCKSIZE, 1);
   timestamp* filetime = (timestamp *)calloc(sizeof(timestamp), 1);

   // Read the inode block that specify in the parameter to buffer
   readBlock(disk_num, inode, buffer);

   // copy the file times to filetime struct
   memcpy(filetime, buffer + 15, sizeof(timestamp));

   // Update file access time;
   filetime->access = time(NULL);
   
   // Write back the update structure to buffer and write to inodeBlock
   memcpy(buffer + 15, filetime, sizeof(timestamp));
   writeBlock(disk_num, inode, buffer);

   // clean up
   free(buffer);
   free(filetime);
}

void modifyFile(int inode) {
   char* buffer = (char *)calloc(BLOCKSIZE, 1);
   timestamp* filetime = (timestamp *)calloc(sizeof(timestamp), 1);

   // find the inode block using the inode number pass in and write to buffer
   readBlock(disk_num, inode, buffer);

   // Get current times from buffer to filetime struct
   memcpy(filetime, buffer + 15, sizeof(timestamp));

   // Update modification and access time
   filetime->modification = time(NULL);
   filetime->access = filetime->modification;

   // Copy back to buffer and write back to inode block
   memcpy(buffer + 15, filetime, sizeof(timestamp));
   writeBlock(disk_num, inode, buffer);

   // clean up
   free(buffer);
   free(filetime);
}
