// Steven Culwell
// 1001783662

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "filesystem.h"

#define BLOCK_SIZE      8192

#define NUM_DATA_BLOCKS 1250
#define NUM_BLOCKS      4226

#define MAX_FILENAME    32
#define MAX_FILES       125
#define MAX_FILE_SIZE   BLOCK_SIZE*NUM_DATA_BLOCKS

typedef uint8_t inode_ptr;
typedef uint16_t block_ptr;

typedef struct {
  char filename[MAX_FILENAME];        // Copy of filename from corresponding dir_entry
  int used_blocks;                    // Counts number of used blocks for the inode
  uint32_t bytes;                     // Total size of the file in bytes
  uint8_t attrib;                     // Bit field containing bit for each attribute
  time_t time_added;                  // The time the file was added to the filesystem
  block_ptr blocks[NUM_DATA_BLOCKS];  // Direct blocks for the inode
} inode;

typedef struct {
  char filename[MAX_FILENAME];        // The filename of the dir entry
  inode_ptr inode;                    // The corresponding inode of the dir entry
  bool valid;                         // True if the dir entry is currently being used, else false
} dir_entry;

static dir_entry *dir_entries[MAX_FILES];
static inode *inodes[MAX_FILES];
static uint8_t *free_inode_map;
static uint8_t *free_block_map;

static uint8_t filesystem[NUM_BLOCKS][BLOCK_SIZE];

static char *disk_image_name;

static bool opened = false;

// Find first index of block marked as "free" (1)
// in free_block_map
int find_next_free_block() {
  for(int i = 0; i < NUM_BLOCKS; i++)
    if(free_block_map[i])
      return i;
  return -1;
}

// Find first index of inode marked as "free" (1)
// in free_inode_map
int find_next_free_inode() {
  for(int i = 0; i < MAX_FILES; i++)
    if(free_inode_map[i])
      return i;
  return -1;
}

// Find first index of directory entry marked as invalid
// in dir_entries
int find_next_free_dir_entry() {
  for(int i = 0; i < MAX_FILES; i++)
    if(dir_entries[i]->valid == 0)
      return i;
  return -1;
}

// Find directory entry with a certain filename.
// This dir_entry must also be either valid or indvalid
// depending on the given "valid" argument
int find_dir_entry(char *filename, bool valid) {
  for(int i = 0; i < MAX_FILES; i++)
    if(strncmp(dir_entries[i]->filename, filename, MAX_FILENAME) == 0 &&
        dir_entries[i]->valid == valid)
      return i;
  return -1;
}

// Check if all blocks in a given free inode are free
bool all_blocks_free(inode_ptr n) {
  for(int i = 0; i < inodes[n]->used_blocks; i++) {
    // Return false if block i is not free
    if(!free_block_map[inodes[n]->blocks[i]])
      return false;
  }
  // Return true now that we know all blocks must be free
  return true;
}

// Initialize the file system, including dir_entries array, inode
// array, free_inode_map, free_block_map, and disk_image_name
int fs_createfs(char *name) {
  if(opened) {
    printf("createfs error: Another file system is already open\n");
    return -1;
  }
  
  disk_image_name = strndup(name, MAX_FILENAME+1);
  
  // Initialize all data in blocks to 0
  for(int i = 0; i < NUM_BLOCKS; i++)
    memset(filesystem[i], 0, BLOCK_SIZE);
  
  size_t size = sizeof(dir_entry);
  for(int i = 0; i < MAX_FILES; i++) {
    dir_entries[i] = (dir_entry *) &filesystem[0][size * i];
    inodes[i] = (inode *) filesystem[i+5];
  }
  
  // Set all inodes and blocks to free (1)
  memset(filesystem[2], 1, MAX_FILES);
  memset(filesystem[3], 1, NUM_BLOCKS);
  
  // Set blocks 0-130 to used (0)
  memset(filesystem[3], 0, MAX_FILES+5);
  
  free_inode_map = filesystem[2];
  free_block_map = filesystem[3];
  
  // printf("%ld\n", BLOCK_SIZE/sizeof(dir_entry));
  
  opened = true;
  return 0;
}

// Save the currently opened filesystem in the current
// directory with the value of disk_image_name as its name
int fs_savefs() {
  if(!opened) {
    printf("savefs error: No file system is currently open\n");
    return -1;
  }

  // Now, open the output file that we are going to write the data to.
  FILE *ofp;
  ofp = fopen(disk_image_name, "w");

  if(ofp == NULL) {
    printf("savefs error: Could not open file \"%s\": ", disk_image_name);
    fflush(stdout);
    perror("");
    return -1;
  }

  // Initialize our offsets and pointers just we did above when reading from the file.
  block_ptr block_index = 0;
  int copy_size = BLOCK_SIZE * NUM_BLOCKS;
  int offset = 0;

  printf("Writing %d bytes to %s\n", copy_size, disk_image_name);

  // Using copy_size as a count to determine when we've copied enough bytes to the output file.
  // Each time through the loop, we will copy BLOCK_SIZE number of bytes from  our stored data
  // to the file fp, then we will increment the offset into the file we are writing to.
  while(copy_size > 0) {
    // Write BLOCK_SIZE number of bytes from our data array into our output file.
    fwrite(filesystem[block_index], BLOCK_SIZE, 1, ofp);

    // Reduce the amount of bytes remaining to copy, increase the offset into the file
    // and increment the block_index to move us to the next data block.
    copy_size -= BLOCK_SIZE;
    offset    += BLOCK_SIZE;
    block_index++;

    // Since we've copied from the point pointed to by our current file pointer, increment
    // offset number of bytes so we will be ready to copy to the next area of our output file.
    fseek(ofp, offset, SEEK_SET);
  }

  // Close the output file, we're done. 
  fclose(ofp);

  return 0;
}

// Set attribute (a) in file with filename (filename) to either
// enabled or disabled
int fs_setattrib(char *filename, attrib a, bool enabled) {
  if(!opened) {
    printf("attrib error: No file system is currently open\n");
    return -1;
  }
  if(strnlen(filename, MAX_FILENAME+1) > MAX_FILENAME) {
    printf("attrib error: File name too long\n");
    return -1;
  }
  
  int idx = find_dir_entry(filename, true);
  if(idx == -1) {
    printf("attrib error: Could not find file with name \"%s\"\n", filename);
    return -1;
  }
  
  int inode_idx = dir_entries[idx]->inode;
  if(enabled)
    inodes[inode_idx]->attrib |=  a & 0b11;
  else
    inodes[inode_idx]->attrib &= ~a & 0b11;
  
  return 0;
}

// Open file on system with name filename as current filesystem
int fs_open(char *filename) {
  if(opened) {
    printf("open error: Another file system is already open\n");
    return -1;
  }
  fs_createfs(filename);
  
  if(strnlen(filename, MAX_FILENAME+1) > MAX_FILENAME) {
    printf("open error: File name too long\n");
    return -1;
  }
  
  int    status;                   // Hold the status of all return values.
  struct stat buf;                 // stat struct to hold the returns from the stat call

  // Call stat with out input filename to verify that the file exists.  It will also 
  // allow us to get the file size. We also get interesting file system info about the
  // file such as inode number, block size, and number of blocks.  For now, we don't 
  // care about anything but the filesize.
  status =  stat(filename, &buf); 

  // If stat did return -1 then we know the input file doesn't exists or we can't use it.
  if(status == -1) {
    printf("open error: Failed to read file\n");
    return -1;
  }
  
  // Save off the size of the input file since we'll use it in a couple of places and 
  // also initialize our index variables to zero. 
  int copy_size   = buf.st_size;
  
  if(copy_size != NUM_BLOCKS * BLOCK_SIZE) {
    printf("open error: Image is not correct size\n");
    return -1;
  }
  
  // Open the input file read-only 
  FILE *ofp = fopen(filename, "r"); 
  printf("Reading %d bytes from %s\n", copy_size, filename);


  // We want to copy and write in chunks of BLOCK_SIZE. So to do this 
  // we are going to use fseek to move along our file stream in chunks of BLOCK_SIZE.
  // We will copy bytes, increment our file pointer by BLOCK_SIZE and repeat.
  int offset      = 0;               

  // We are going to copy and store our file in BLOCK_SIZE chunks instead of one big 
  // memory pool. Why? We are simulating the way the file system stores file data in
  // blocks of space on the disk. block_index will keep us pointing to the area of
  // the area that we will read from or write to.
  int block_index = 0;
  
  // copy_size is initialized to the size of the input file so each loop iteration we
  // will copy BLOCK_SIZE bytes from the file then reduce our copy_size counter by
  // BLOCK_SIZE number of bytes. When copy_size is less than or equal to zero we know
  // we have copied all the data from the input file.
  while(copy_size > 0) {

    // Index into the input file by offset number of bytes.  Initially offset is set to
    // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We 
    // then increase the offset by BLOCK_SIZE and continue the process.  This will
    // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
    fseek(ofp, offset, SEEK_SET);

    // Read BLOCK_SIZE number of bytes from the input file and store them in our
    // data array. 
    int bytes  = fread(filesystem[block_index], BLOCK_SIZE, 1, ofp);

    // If bytes == 0 and we haven't reached the end of the file then something is 
    // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
    // It means we've reached the end of our input file.
    if(bytes == 0 && !feof(ofp)) {
      printf("open error: An error occured reading from the input file\n");
      fclose(ofp);
      return -1;
    }

    // Clear the EOF file flag.
    clearerr(ofp);

    // Reduce copy_size by the BLOCK_SIZE bytes.
    copy_size -= BLOCK_SIZE;
    
    // Increase the offset into our input file by BLOCK_SIZE.  This will allow
    // the fseek at the top of the loop to position us to the correct spot.
    offset    += BLOCK_SIZE;

    // Increment the index into the block array 
    block_index++;
  }

  // We are done copying from the input file so close it out.
  fclose(ofp);

  
  return 0;
}

// Close the currently opened filesystem
int fs_close() {
  if(!opened)
    return -1;
  
  free(disk_image_name);
  opened = false;
  
  return 0;
}

// List all files not marked as deleted. Only show hidden files
// if show_hidden is true
int fs_list(bool show_hidden) {
  if(!opened) {
    printf("list error: No file system is currently open\n");
    return -1;
  }
  
  for(int i = 0; i < MAX_FILES; i++) {
    int inode_idx = dir_entries[i]->inode;
    if(dir_entries[i]->valid && (!(inodes[inode_idx]->attrib & H) || show_hidden)) {
      char h = inodes[inode_idx]->attrib & H ? 'H' : '-';
      char r = inodes[inode_idx]->attrib & R ? 'R' : '-';
      char *time_str = ctime(&inodes[inode_idx]->time_added);
      time_str[strlen(time_str)-1] = 0; // Remove newline character from string
      printf("%c%c %8d %s %s\n", h, r, inodes[inode_idx]->bytes, time_str, dir_entries[i]->filename);
    }
  }
  
  return 0;
}

// Put a file currently on the system into the filesystem
int fs_put(char *filename) {
  if(!opened) {
    printf("put error: No file system is currently open\n");
    return -1;
  }
  
  char *base = basename(filename);
  if(!base || base[0] == 0) {
    printf("put error: Could not extract valid basename from given filename\n");
    return -1;
  }
  
  if(strnlen(base, MAX_FILENAME+1) > MAX_FILENAME) {
    printf("put error: File name too long\n");
    return -1;
  }
  
  int valid_idx = find_dir_entry(base, true);
  if(valid_idx != -1) {
    printf("put error: Another file with the same name already exists\n");
    return -1;
  }
  
  int    status;                   // Hold the status of all return values.
  struct stat buf;                 // stat struct to hold the returns from the stat call

  // Call stat with out input filename to verify that the file exists.  It will also 
  // allow us to get the file size. We also get interesting file system info about the
  // file such as inode number, block size, and number of blocks.  For now, we don't 
  // care about anything but the filesize.
  status =  stat(filename, &buf); 

  // If stat did return -1 then we know the input file doesn't exists or we can't use it.
  if(status == -1) {
    printf("put error: Failed to read file\n");
    return -1;
  }
  
  // Save off the size of the input file since we'll use it in a couple of places and 
  // also initialize our index variables to zero. 
  int copy_size   = buf.st_size;
  
  // If the size of the file is greater than the available space left,
  // we cannot fit this file. Return failure.
  if(copy_size > fs_df()) {
    printf("put error: Not enough disk space\n");
    return -1;
  }
  
  // If the size of the file is greater than the maximum file size,
  // we cannot fit this file. Return failure.
  if(copy_size > MAX_FILE_SIZE) {
    printf("put error: File size is greater than maximum file size: %d\n", MAX_FILE_SIZE);
    return -1;
  }
  
  int dir_entry_idx, inode_idx;
  
  // If a deleted file with the same name exists, we will overwrite it
  // so that there is no naming conflict when undeleting by having two
  // files with same name. Otherwise, we will just use the next available
  // dir entry and inode.
  int invalid_idx = find_dir_entry(base, false);
  if(invalid_idx != -1) {
    dir_entry_idx = invalid_idx;
    inode_idx = dir_entries[dir_entry_idx]->inode;
    
    // If the inode corresponding to the dir entry we are overwritting
    // is not free (another dir entry is using it), then find another
    // one that is free.
    if(!free_inode_map[inode_idx]) {
      inode_idx = find_next_free_inode();
    }
  } else {
    dir_entry_idx = find_next_free_dir_entry();
    inode_idx = find_next_free_inode();
  }
  
  if(dir_entry_idx == -1) {
    printf("put error: Maximum amount of files has been reached (%d)\n", MAX_FILES);
    return -1;
  }
  if(inode_idx == -1) {
    printf("put error: Maximum amount of inodes has been reached (%d)\n", MAX_FILES);
    return -1;
  }
  
  // Clear all values in both the dir entry and inode
  memset(dir_entries[dir_entry_idx], 0, sizeof(dir_entry));
  memset(inodes[inode_idx], 0, sizeof(inode));
  
  strncpy(dir_entries[dir_entry_idx]->filename, base, MAX_FILENAME);
  dir_entries[dir_entry_idx]->inode = inode_idx;
  dir_entries[dir_entry_idx]->valid = true;
  
  strncpy(inodes[inode_idx]->filename, base, MAX_FILENAME);
  inodes[inode_idx]->bytes = copy_size;
  inodes[inode_idx]->time_added = time(NULL);
  inodes[inode_idx]->attrib = 0;
  
  // Open the input file read-only 
  FILE *ifp = fopen(filename, "r"); 
  printf("Reading %d bytes from %s\n", copy_size, filename);


  // We want to copy and write in chunks of BLOCK_SIZE. So to do this 
  // we are going to use fseek to move along our file stream in chunks of BLOCK_SIZE.
  // We will copy bytes, increment our file pointer by BLOCK_SIZE and repeat.
  int offset      = 0;               

  // We are going to copy and store our file in BLOCK_SIZE chunks instead of one big 
  // memory pool. Why? We are simulating the way the file system stores file data in
  // blocks of space on the disk. block_index will keep us pointing to the area of
  // the area that we will read from or write to.
  int block_index = find_next_free_block();
  
  free_inode_map[inode_idx] = 0;
  int direct_data_block_idx = 0;

  // copy_size is initialized to the size of the input file so each loop iteration we
  // will copy BLOCK_SIZE bytes from the file then reduce our copy_size counter by
  // BLOCK_SIZE number of bytes. When copy_size is less than or equal to zero we know
  // we have copied all the data from the input file.
  while(copy_size > 0) {

    // Index into the input file by offset number of bytes.  Initially offset is set to
    // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We 
    // then increase the offset by BLOCK_SIZE and continue the process.  This will
    // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
    fseek(ifp, offset, SEEK_SET);

    // Read BLOCK_SIZE number of bytes from the input file and store them in our
    // data array. 
    int bytes  = fread(filesystem[block_index], BLOCK_SIZE, 1, ifp);

    // If bytes == 0 and we haven't reached the end of the file then something is 
    // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
    // It means we've reached the end of our input file.
    if(bytes == 0 && !feof(ifp)) {
      printf("put error: An error occured reading from the input file\n");
      fclose(ifp);
      return -1;
    }

    // Clear the EOF file flag.
    clearerr(ifp);

    // Reduce copy_size by the BLOCK_SIZE bytes.
    copy_size -= BLOCK_SIZE;
    
    // Increase the offset into our input file by BLOCK_SIZE.  This will allow
    // the fseek at the top of the loop to position us to the correct spot.
    offset    += BLOCK_SIZE;

    free_block_map[block_index] = 0;
    inodes[inode_idx]->blocks[direct_data_block_idx] = block_index;
    inodes[inode_idx]->used_blocks++;
    direct_data_block_idx++;
    
    // Increment the index into the block array 
    block_index = find_next_free_block();
  }

  // We are done copying from the input file so close it out.
  fclose(ifp);
  return 0;
}

// Get a file on the filesystem and move in onto the system
int fs_get(char *filename, char *newfilename) {
  if(!opened) {
    printf("get error: No file system is currently open\n");
    return -1;
  }
  
  int dir_idx = find_dir_entry(filename, true);
  if(dir_idx == -1 || !dir_entries[dir_idx]->valid) {
    printf("get error: Unable to find file \"%s\"\n", filename);
    return -1;
  }
  inode_ptr inode_idx = dir_entries[dir_idx]->inode;
  if(inode_idx >= MAX_FILES) {
    printf("get error: File has invalid inode index\n");
    return -1;
  }
  
  // Now, open the output file that we are going to write the data to.
  FILE *ofp;
  ofp = fopen(newfilename, "w");

  if(ofp == NULL) {
    printf("get error: Could not open file \"%s\": ", disk_image_name);
    fflush(stdout);
    perror("");
    return -1;
  }

  // Initialize our offsets and pointers just we did above when reading from the file.
  block_ptr direct_block_idx = 0;
  int copy_size   = inodes[inode_idx]->bytes;
  int offset      = 0;

  printf("Writing %d bytes to %s\n", copy_size, newfilename);

  // Using copy_size as a count to determine when we've copied enough bytes to the output file.
  // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
  // our stored data to the file fp, then we will increment the offset into the file we are writing to.
  // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
  // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
  // last iteration we'd end up with gibberish at the end of our file. 
  while(copy_size > 0) {

    int num_bytes;

    // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
    // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
    // end up with garbage at the end of the file.
    if(copy_size < BLOCK_SIZE) {
      num_bytes = copy_size;
    } else {
      num_bytes = BLOCK_SIZE;
    }

    // Write num_bytes number of bytes from our data array into our output file.
    uint8_t *block = filesystem[inodes[inode_idx]->blocks[direct_block_idx]];
    fwrite(block, num_bytes, 1, ofp); 

    // Reduce the amount of bytes remaining to copy, increase the offset into the file
    // and increment the block_index to move us to the next data block.
    copy_size -= BLOCK_SIZE;
    offset    += BLOCK_SIZE;
    direct_block_idx++;

    // Since we've copied from the point pointed to by our current file pointer, increment
    // offset number of bytes so we will be ready to copy to the next area of our output file.
    fseek(ofp, offset, SEEK_SET);
  }

  // Close the output file, we're done. 
  fclose(ofp);
  return 0;
}

int fs_del(char *filename) {
  if(!opened) {
    printf("del error: No file system is currently open\n");
    return -1;
  }
  
  int dir_idx = find_dir_entry(filename, true);
  if(dir_idx == -1) {
    printf("del error: Unable to find file \"%s\"\n", filename);
    return -1;
  }
  
  int inode_idx = dir_entries[dir_idx]->inode;
  if(inode_idx >= MAX_FILES) {
    printf("del error: File has invalid inode index\n");
    return -1;
  }
  dir_entries[dir_idx]->valid = false;
  free_inode_map[inode_idx] = 1;
  
  // Mark all blocks corresponding to inode as free
  for(int i = 0; i < inodes[inode_idx]->used_blocks; i++) {
    free_block_map[inodes[inode_idx]->blocks[i]] = 1;
  }
  
  return 0;
}

int fs_undel(char *filename) {
  if(!opened) {
    printf("undel error: No file system is currently open\n");
    return -1;
  }
  
  int dir_idx = find_dir_entry(filename, false);
  if(dir_idx == -1) {
    printf("undel error: Unable to find file \"%s\"\n", filename);
    return -1;
  }
  
  int inode_idx = dir_entries[dir_idx]->inode;
  if(inode_idx >= MAX_FILES) {
    printf("undel error: File has invalid inode index\n");
    return -1;
  }
  // Check that inodes filename matches dir entries filename so that we know they
  // should correspond to each other
  if(strncmp(inodes[inode_idx]->filename, dir_entries[dir_idx]->filename, MAX_FILENAME) != 0) {
    printf("del error: The entries inode has been claimed by another file.\n");
    return -1;
  }
  if(!all_blocks_free(inode_idx)) {
    printf("del error: One or more of the data blocks corresponding to the file have \
been overwritten by another file.\n");
    return -1;
  }
  
  dir_entries[dir_idx]->valid = true;
  free_inode_map[inode_idx] = 0;
  // Mark all blocks corresponding to inode as no longer free
  for(int i = 0; i < inodes[inode_idx]->used_blocks; i++) {
    free_block_map[inodes[inode_idx]->blocks[i]] = 0;
  }
  
  return 0;
}

int fs_df() {
  int count = 0;
  for(int i = 0; i < NUM_BLOCKS; i++)
    if(free_block_map[i])
      count++;
  return count * BLOCK_SIZE;
}

int fs_cat(char *filename) {
  int idx = find_dir_entry(filename, true);
  if(idx == -1) {
    printf("cat error: Could not find file \"%s\"\n", filename);
    return -1;
  }
  
  int inode_idx = dir_entries[idx]->inode;
  int remaining = inodes[inode_idx]->bytes;
  for(int i = 0; i < inodes[inode_idx]->used_blocks; i++) {
    int s = BLOCK_SIZE;
    if(remaining < BLOCK_SIZE)
      s = remaining;
    
    printf("%.*s", s, filesystem[inodes[inode_idx]->blocks[i]]);
    remaining -= BLOCK_SIZE;
  }
  return 0;
}
