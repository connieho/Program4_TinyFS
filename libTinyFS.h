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
} file_extent; 
   
