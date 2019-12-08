/*
 *  Copyright (C) 2019 CS416 Spring 2019
 *	
 *	Tiny File System
 *
 *	File:	tfs.c
 *  Author: Yujie REN
 *	Date:	April 2019
 *
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <libgen.h>
#include <limits.h>

#include "block.h"
#include "tfs.h"

char diskfile_path[PATH_MAX];
char *disk_path = "./testdisk8";
// Declare your in-memory data structures heree
bitmap_t inode_bit_map;
bitmap_t data_bit_map;
struct superblock *sb;

int numBlocksForInodes;

void printInodeBitMap();
void printDataBitMap();

/* 
 * Get available inode number from bitmap
 */
int get_avail_ino() {
	printf("|----------------------------\n");
	printf("|--- starting get_avail_ino()\n");
	printf("|----------------------------\n");
	if(inode_bit_map == NULL){
	 	printf("inode_bit_map is NULL.\n");
	 	return -1;
	}
	printInodeBitMap();
	// Step 1: Traverse inode bitmap to find an available slot		
	
	uint8_t indexOfAvailableInode;
	//int indexOfAvailableInode = -1;
	printf("Traversing bitmap to find avaiable slot...\n");
	uint8_t i;
	for(i = 0; i < numBlocksForInodes; i++){
		uint8_t inodeBitmapIndex = get_bitmap(inode_bit_map, i);		
		if(inodeBitmapIndex == 0){
			printf("- current inodeBitmapIndex == %u ~ found available slot here\n\n",inodeBitmapIndex);
			indexOfAvailableInode = i;
			break;
		}
		printf("- current inodeBitmapIndex == %u\n",inodeBitmapIndex);
		// printf("index %d | value at inodebitmap: ",i);
		// printf("%u\n",bitindex);		
	}
	printf("indexOfAvailableInode: %u\n",indexOfAvailableInode);
	
	
	// Step 2 ver 2: Read inode bitmap from disk


	// Step 2: Read inode bitmap from disk
	int indexOfAvailableInodeInFile = sb->i_start_blk + indexOfAvailableInode;	
	
	// Step 3: Update inode bitmap and write to disk
	printf("setting index %u of inodebitmap...\n", indexOfAvailableInode);
	set_bitmap(inode_bit_map, indexOfAvailableInode);	
	printInodeBitMap();
	//write new bitmap to disk
	printf("Updating disk with new bitmap...\n");
	bio_write(sb->i_bitmap_blk, inode_bit_map);
	printf("Updated\n");
	
	printf("indexOfAvailableInodeInFile: %u\nReturning index %u as next free index\n",indexOfAvailableInodeInFile, indexOfAvailableInodeInFile);
	printf("|--- get_avail_ino() is done.\n\n");	

	return indexOfAvailableInodeInFile;	
	//return freeInodeLocation;	
}

void printInodeBitMap(){
	uint8_t i;
	printf("Printing inode bitmap...\n");
	for(i = 0; i < numBlocksForInodes; i++){
		uint8_t inodeBitmapIndex = get_bitmap(inode_bit_map, i);
		printf("%u",inodeBitmapIndex);
	}
	printf("\n");
}

/* 
 * Get available data block number from bitmap
 */
int get_avail_blkno() {
	printf("|------------------------------\n");
	printf("|--- starting get_avail_blkno()\n");
	printf("|------------------------------\n");
	if(data_bit_map == NULL){
	 	printf("data_bit_map is NULL.\n");
	 	return -1;
	}
	printDataBitMap();
	// Step 1: Traverse data block bitmap to find an available slot

	uint8_t indexOfAvailableDataBlock;
	int i;
	for(i = 0; i < MAX_DNUM; i++){
		uint8_t dataBitmapIndex = get_bitmap(data_bit_map, i);		
		if(dataBitmapIndex == 0){
			printf("dataBitmapIndex == %u ~ found available slot here\n\n",dataBitmapIndex);
			indexOfAvailableDataBlock = i;
			break;
		}
		printf("dataBitmapIndex == %u\n",dataBitmapIndex);
		// printf("index %d | value at inodebitmap: ",i);
		// printf("%u\n",bitindex);		
	}
	printf("indexOfAvailableDataBlock: %u\n",indexOfAvailableDataBlock);

	// Step 2: Read data block bitmap from disk
	int indexOfAvailableDataBlockInFile = sb->d_start_blk + indexOfAvailableDataBlock;
	
	// Step 3: Update data block bitmap and write to disk
	printf("setting index %u of databitmap...\n", indexOfAvailableDataBlock);	
	set_bitmap(data_bit_map, indexOfAvailableDataBlock);
	printDataBitMap();
	//write new bit map to disk
	printf("Updating disk with new bitmap...\n");
	bio_write(sb->d_bitmap_blk, data_bit_map);
	printf("Updated\n");


	printf("indexOfAvailableDataBlockInFile: %u\nReturning index %u as next free index\n",indexOfAvailableDataBlockInFile, indexOfAvailableDataBlockInFile);
	printf("|--- get_avail_blkno() is done.\n\n");

	return indexOfAvailableDataBlockInFile;	

}

void printDataBitMap(){
	uint8_t i;
	printf("Printing data bitmap...\n");
	
	for(i = 0; i < 64; i++){
		uint8_t dataBitmapIndex = get_bitmap(data_bit_map, i);
		printf("%u",dataBitmapIndex);
	}
	printf("...MAX_DNUM\n");
}

/* 
 * inode operations
 */
int readi(uint16_t ino, struct inode *inode) {

  // Step 1: Get the inode's on-disk block number
	//scan bit map for ino, keep track of this index
	uint16_t i;
	uint16_t j = 0;
	uint16_t inodeDiskBlockNumber;
	for(i = sb->i_start_blk; j < numBlocksForInodes; i++, j++){
		struct inode *temp = malloc(sizeof(struct inode));
		bio_read(i,temp);
		if(temp->ino == ino){
			inodeDiskBlockNumber = i;
			bio_read(inodeDiskBlockNumber, inode);
			return 0;
			//inodeDiskBlockNumber = sb->i_start_blk + i;
		}
	}		
	/*
	int block = (ino * sizeof(struct inode))/BLOCK_SIZE;
	int sector = ((block * BLOCK_SIZE) + inodeStardAddr)/512;
	*/
  // Step 2: Get offset of the inode in the inode on-disk block
	//

  // Step 3: Read the block from disk and then copy into inode structure
	//bio_read(inodeDiskBlockNumber, inode);
	return 0;
}

int writei(uint16_t ino, struct inode *inode) {

	// Step 1: Get the block number where this inode resides on disk
	uint16_t i;
	uint16_t j = 0;
	uint16_t inodeDiskBlockNumber;
	for(i = sb->i_start_blk; j < numBlocksForInodes; i++, j++){
		struct inode *temp = malloc(sizeof(struct inode));
		bio_read(i,temp);
		if(temp->ino == ino){
			inodeDiskBlockNumber = i;
			bio_write(inodeDiskBlockNumber, inode);
			return 0;
			//inodeDiskBlockNumber = sb->i_start_blk + i;
		}
	}	
	
	// Step 2: Get the offset in the block where this inode resides on disk

	// Step 3: Write inode to disk 
	//bio_write(inodeDiskBlockNumber, inode);
	return 0;
}


/* 
 * directory operations
 */
int dir_find(uint16_t ino, const char *fname, size_t name_len, struct dirent *dirent) {

  // Step 1: Call readi() to get the inode using ino (inode number of current directory)
  	struct inode *inode = malloc(sizeof(struct inode));  	
	//this is the inode you're working with
	readi(ino, inode);

	//Now, inode should be the 'starting' directory	

  // Step 2: Get data block of current directory from inode		

  // Step 3: Read directory's data block and check each directory entry.
  //If the name matches, then copy directory entry to dirent structure
	int i;
	for(i = 0; i < 16; i++){
		//if you encounter a valid file/dir linked to this inode
		if(inode->direct_ptr[i] != 0){		
			struct dirent *directoryEntry = malloc(sizeof(struct dirent));
			
			bio_read((sb->d_start_blk + inode->direct_ptr[i]), directoryEntry);

			if(strcmp(fname, directoryEntry->name) == 0){
				dirent->ino = directoryEntry->ino;
				dirent->valid = directoryEntry->valid;
				strcpy(dirent->name, directoryEntry->name);
				return 0;		
			}
		}
	}

	return -1;
}

int dir_add(struct inode dir_inode, uint16_t f_ino, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and check each directory entry of dir_inode	
		
	// Step 2: Check if fname (directory name) is already used in other entries								
	int i;
	for(i = 0; i < 16; i++){
		//if you encounter a valid file/dir linked to this inode
		if(dir_inode.direct_ptr[i] != 0){		
			struct dirent *directoryEntry = malloc(sizeof(struct dirent));
			
			bio_read((sb->d_start_blk + dir_inode.direct_ptr[i]), directoryEntry);

			if(strcmp(fname, directoryEntry->name) == 0){
				//if there already exists an entry, return
				printf("File name already exsists.\n");		
				return 0;
			}
		}
	}

	// Step 3: Add directory entry in dir_inode's data block and write to disk
	for(i = 0; i < 16; i++){
		//if you encounter a valid file/dir linked to this inode
		if(dir_inode.direct_ptr[i] == 0){		
			dir_inode.direct_ptr[i] = f_ino;			
			writei(f_ino, &dir_inode);
		}
	}

	// Allocate a new data block for this directory if it does not exist	
	int freeBlock = get_avail_blkno();
	struct dirent *newEntry = malloc(sizeof(struct dirent));
	newEntry->ino = f_ino;
	newEntry->valid = 1;
	strcpy(newEntry->name, fname);
	bio_write(freeBlock, newEntry);

	// Update directory inode	

	// Write directory entry

	return 0;
}

int dir_remove(struct inode dir_inode, const char *fname, size_t name_len) {

	// Step 1: Read dir_inode's data block and checks each directory entry of dir_inode	
	
	// Step 2: Check if fname exist
	int fileExsists = 0;
	int indexOfFile;

	int i;
	for(i = 0; i < 16; i++){
		//if you encounter a valid file/dir linked to this inode
		if(dir_inode.direct_ptr[i] != 0){		
			struct dirent *directoryEntry = malloc(sizeof(struct dirent));
			
			bio_read((sb->d_start_blk + dir_inode.direct_ptr[i]), directoryEntry);

			if(strcmp(fname, directoryEntry->name) == 0){
				//if there already exists an entry, set file exsists to true	
				indexOfFile = i;				
				fileExsists = 1;
			}
		}
	}

	if(fileExsists == 0){
		printf("file doesn't exsist\n");
		return 0;
	}

	// Step 3: If exist, then remove it from dir_inode's data block and write to disk
		//TODO: May need to change the default values in direct_ptr's to -1
	dir_inode.direct_ptr[indexOfFile] = 0;
	writei(dir_inode.ino, &dir_inode);
	return 0;
}

/* 
 * namei operation
 */
int get_node_by_path(const char *path, uint16_t ino, struct inode *inode) {
	
	// Step 1: Resolve the path name, walk through path, and finally, find its inode.
	// Note: You could either implement it in a iterative way or recursive way

	return 0;
}

/* 
 * Make file system
 */
int tfs_mkfs() {
	printf("|-----------------------\n");
	printf("|--- Starting tfs_mkfs()\n");
	printf("|-----------------------\n");
	// Call dev_init() to initialize (Create) Diskfile
		dev_init(disk_path);
	// write superblock information
		int spaceNeededForInodes = (sizeof(struct inode) * MAX_INUM);
		numBlocksForInodes = spaceNeededForInodes / BLOCK_SIZE;

		// [sb][inodebitmap][databitmap][inodeblock] ... [datab] ... 
		//struct superblock *sb = malloc(sizeof(struct superblock));
		printf("Creating superblock...\n");
		sb = malloc(sizeof(struct superblock));
		sb->magic_num = MAGIC_NUM;
		sb->max_inum = MAX_INUM;
		sb->max_dnum = MAX_INUM;
		sb->i_bitmap_blk = 1;
		sb->d_bitmap_blk = 2;
		sb->i_start_blk = 3;
		sb->d_start_blk = (3 + numBlocksForInodes) + 1; 

		dev_open(disk_path);
		//int num = 14;
		//char *string = "test string";
		printf("Writing superblock to disk...\n");
		bio_write(0, sb);

	// initialize inode bitmap	
		printf("Creating inode bitmap...\n");	
		inode_bit_map = malloc(numBlocksForInodes * sizeof(bitmap_t));

	// initialize data block bitmap
		printf("Creating data bitmap...\n");	
		data_bit_map = malloc(MAX_DNUM * sizeof(bitmap_t));						

	// update bitmap information for root directory
		printf("Setting the 0th index in inode bit map (for root)...\n");	
		set_bitmap(inode_bit_map, 0); 
		printf("Setting the 0th index in data bit map (for root)...\n");	
		set_bitmap(data_bit_map, 0); 		

	// update inode for root directory	(in inode table)		
		//write bitmaps to disk
		//dev_open(disk_path);
			printf("Writing inode bitmap to disk...\n");	
			bio_write(sb->i_bitmap_blk, inode_bit_map);
			printf("Writing data bitmap to disk...\n");	
			bio_write(sb->d_bitmap_blk, data_bit_map);

			//create inode for root, write it to the file (first block in inode table)
			printf("Creating inode for root...\n");	
			// just worry about filling in main basic ones like st_ino, st_mode, st_size, st_blksize, st_blocks
			struct inode *root = malloc(sizeof(struct inode));
			root->ino = 0;
			root->valid = 1;
			// root->size = 			
			// root->type =
			// root->link = 
			//root->direct_ptr = ;
			root->vstat.st_ino = 0;
			root->vstat.st_mode = 0755;
			root->vstat.st_size = -1;
			root->vstat.st_blksize = BLOCK_SIZE;
			root->vstat.st_blocks = -1;
			
			//COMMENTING OUT cause root would be empty at first -- reading data location into root?

			//printf("? - reading next available data location into root->direct_ptr[0]\n");						
			//bio_read(sb->d_start_blk, &root->direct_ptr[0]);			
			
			printf("Writing inode root to the 0th block in inode table on disk...\n");	
			bio_write(sb->i_start_blk, &root);			
									
		//write the rest of the empty inodes into inode table section on disk
		
		printf("Filling inode table with empty inodes...\n");	
		
		uint8_t i;
		for(i = 1; i < numBlocksForInodes; i++){
			//printf("%d\n",i);			
			struct inode *newInode = malloc(sizeof(struct inode));
			newInode->ino = i;
			newInode->valid = 0;			
			newInode->vstat.st_ino = i;
			newInode->vstat.st_mode = 0666;
			newInode->vstat.st_size = -1;
			newInode->vstat.st_blksize = BLOCK_SIZE;
			newInode->vstat.st_blocks = -1;			
			bio_write(sb->i_start_blk+i, &newInode);			
		}	
		
		printf("|--- tfs_mkfs() is done.\n\n");
		
	return 0;
}


/* 
 * FUSE file operations
 */
static void *tfs_init(struct fuse_conn_info *conn) {

	// Step 1a: If disk file is not found, call mkfs
	if( access(disk_path, F_OK ) == -1 ) {
    	// file doesn't exsist
		tfs_mkfs();
	} 

  // Step 1b: If disk file is found, just initialize in-memory data structures
  // and read superblock from disk

	//**Handled in tfs_mkfs()

	return NULL;
}

static void tfs_destroy(void *userdata) {

	// Step 1: De-allocate in-memory data structures
		//deallocate inode bitmap
		free(inode_bit_map);
		//deallocate data bitmap
		free(data_bit_map);
		//deallocate superblock
		free(sb);

	// Step 2: Close diskfile
	dev_close();

}

static int tfs_getattr(const char *path, struct stat *stbuf) {

	// Step 1: call get_node_by_path() to get inode from path

	// Step 2: fill attribute of file into stbuf from inode

		stbuf->st_mode   = S_IFDIR | 0755;
		stbuf->st_nlink  = 2;
		time(&stbuf->st_mtime);

	return 0;
}

static int tfs_opendir(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

    return 0;
}

static int tfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: Read directory entries from its data blocks, and copy them to filler

	return 0;
}


static int tfs_mkdir(const char *path, mode_t mode) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target directory to parent directory

	// Step 5: Update inode for target directory

	// Step 6: Call writei() to write inode to disk
	

	return 0;
}

static int tfs_rmdir(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target directory name

	// Step 2: Call get_node_by_path() to get inode of target directory

	// Step 3: Clear data block bitmap of target directory

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target directory in its parent directory

	return 0;
}

static int tfs_releasedir(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of parent directory

	// Step 3: Call get_avail_ino() to get an available inode number

	// Step 4: Call dir_add() to add directory entry of target file to parent directory

	// Step 5: Update inode for target file

	// Step 6: Call writei() to write inode to disk

	return 0;
}

static int tfs_open(const char *path, struct fuse_file_info *fi) {

	// Step 1: Call get_node_by_path() to get inode from path

	// Step 2: If not find, return -1

	return 0;
}

static int tfs_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {

	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: copy the correct amount of data from offset to buffer

	// Note: this function should return the amount of bytes you copied to buffer
	return 0;
}

static int tfs_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	// Step 1: You could call get_node_by_path() to get inode from path

	// Step 2: Based on size and offset, read its data blocks from disk

	// Step 3: Write the correct amount of data from offset to disk

	// Step 4: Update the inode info and write it to disk

	// Note: this function should return the amount of bytes you write to disk
	return size;
}

static int tfs_unlink(const char *path) {

	// Step 1: Use dirname() and basename() to separate parent directory path and target file name

	// Step 2: Call get_node_by_path() to get inode of target file

	// Step 3: Clear data block bitmap of target file

	// Step 4: Clear inode bitmap and its data block

	// Step 5: Call get_node_by_path() to get inode of parent directory

	// Step 6: Call dir_remove() to remove directory entry of target file in its parent directory

	return 0;
}

static int tfs_truncate(const char *path, off_t size) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_release(const char *path, struct fuse_file_info *fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
	return 0;
}

static int tfs_flush(const char * path, struct fuse_file_info * fi) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}

static int tfs_utimens(const char *path, const struct timespec tv[2]) {
	// For this project, you don't need to fill this function
	// But DO NOT DELETE IT!
    return 0;
}


static struct fuse_operations tfs_ope = {
	.init		= tfs_init,
	.destroy	= tfs_destroy,

	.getattr	= tfs_getattr,
	.readdir	= tfs_readdir,
	.opendir	= tfs_opendir,
	.releasedir	= tfs_releasedir,
	.mkdir		= tfs_mkdir,
	.rmdir		= tfs_rmdir,

	.create		= tfs_create,
	.open		= tfs_open,
	.read 		= tfs_read,
	.write		= tfs_write,
	.unlink		= tfs_unlink,

	.truncate   = tfs_truncate,
	.flush      = tfs_flush,
	.utimens    = tfs_utimens,
	.release	= tfs_release
};


// int main(int argc, char *argv[]) {
// 	int fuse_stat;

// 	getcwd(diskfile_path, PATH_MAX);
// 	strcat(diskfile_path, "/DISKFILE");
	
// 	fuse_stat = fuse_main(argc, argv, &tfs_ope, NULL);

// 	return fuse_stat;
// }

int main(int argc, char *argv[]) {
	printf("Starting...\n");
	int fuse_stat;
	fuse_stat = fuse_main(argc, argv, &tfs_ope, NULL);
	printf("%d\n\n",fuse_stat);

	tfs_mkfs();
	int temp = get_avail_ino();	
	int temp2 = get_avail_blkno();
	printf("Done. Index of next available inode: %d\n",temp);
	printf("Done. Index of next available data block: %d\n",temp2);
	//tfs_mkfs();

	return 0;
}

