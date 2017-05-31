char*  initSuperBlock(int nBytes);
typedef struct superblock {
   const int MAGIC 0x45; //detect when disk is not in correct format, second byte of every block
   int inode_root; //block number of the root inode
   struct free_block* free_head; //pointer to list of freeblock
} superblock;

typedef struct free_block {
   int block_number;
   struct free_block* next; //pointer to enxt free block in chain
} free_block;

typedef struct inode {
   char* filename;
   int size;
   struct file_extent* data_head; //head of file_extent LL
};

typedef struct file_extent {
   char[BLOCKSIZE] file_data; 
   struct file_extent* next; //pointer to next data block
<<<<<<< HEAD
} file_extent 

/******************************* Required Functions for TinyFS *******************************/

/* Makes a blank TinyFS file system of size nBytes on the file specified by ‘filename’. This
 * function should use the emulated disk library to open the specified file, and upon success,
 * format the file to be mountable. This includes initializing all data to 0x00, setting magic
 * numbers, initializing and writing the superblock and inodes, etc. Must return a specified 
 * success/error code.
 */
int tfs_mkfs(char *filename, int nBytes);

/* tfs_mount(char *filename) “mounts” a TinyFS file system located within ‘filename’.
 * tfs_unmount(void) “unmounts” the currently mounted file system. As part of the mount
 * operation, tfs_mount should verify the file system is the correct type. Only one file
 * system may be mounted at a time. Use tfs_unmount to cleanly unmount the currently mounted
 * file system. Must return a specified success/error code.
 */
int tfs_mount(char *filename);
int tfs_unmount(void);

/* Opens a file for reading and writing on the currently mounted file system. Creates a dynamic
 * resource table entry for the file, and returns a file descriptor (integer) that can be used
 * to reference this file while the filesystem is mounted.
 */
fileDescriptor tfs_openFile(char *name);

/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD);

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, to the
 * file system. Sets the file pointer to 0 (the start of file) when done. Returns success/error
 * codes.
 */
int tfs_writeFile(fileDescriptor FD,char *buffer, int size);

/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD);

/* reads one byte from the file and copies it to buffer, using the current file pointer
 * location and incrementing it by one upon success. If the file pointer is already at the
 * end of the file then tfs_readByte() should return an error and not increment 
 * the file pointer. 
 */
int tfs_readByte(fileDescriptor FD, char *buffer);

/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset);   
 
