char*  initSuperBlock();
void initFS(int nBytes);

struct file_entry; 
struct file_entry* file_table; //files that are open in the mounted filesystem
struct free_block* freeblock_head;
int nextFD = 0; //used to assign the nextFD
int total_files = 0; //total number of files stored in the file system
int free_blocks = 0;
int disk_num;

typedef struct free_block {
   int block_number;
   struct free_block* next; //pointer to enxt free block in chain
} free_block;

//holds information for each open file in the file table
typedef struct file_entry {
   int fd; //file descriptor of the open file
   int inode_block; //block number of the inode in the file_system
   int file_block; //block number of the file_extent in the file_system
   int open;
   int file_offset; //file pointer used in seek & readByte
   char name[9];
} file_entry;



/******************************* Required Functions for TinyFS *******************************/
int tfs_mkfs(char *filename, int nBytes);

int tfs_mount(char *filename);

int tfs_unmount(void);

fileDescriptor tfs_openFile(char *name);

int tfs_closeFile(fileDescriptor FD);

int tfs_writeFile(fileDescriptor FD,char *buffer, int size);

int tfs_deleteFile(fileDescriptor FD);

int tfs_readByte(fileDescriptor FD, char *buffer);

int tfs_seek(fileDescriptor FD, int offset);   
