// Steven Culwell
// 1001783662

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "filesystem.h"

#define BLOCK_SIZE      8192

#define MAX_FILENAME    32
#define MAX_FILES       125
#define MAX_FILE_SIZE   10240000

#define NUM_DATA_BLOCKS 1250
#define NUM_BLOCKS      4226

// #define BYTES_TO_BLOCKS(bytes) (((bytes)+BLOCK_SIZE-1)/BLOCK_SIZE);

typedef uint8_t inode_ptr;
typedef uint16_t block_ptr;

typedef struct {
  int index;                           // The index of the inode
  int used_blocks;                     // Counts number of used blocks for the inode
  block_ptr direct_data_blocks[1250];  // Direct blocks for the inode
} inode;

typedef struct {
  char filename[MAX_FILENAME];
  inode_ptr inode;
  time_t time_added;
  uint8_t attrib;
  uint32_t bytes;
  uint8_t in_use;
} dir_entry;

static dir_entry *dir_entries[MAX_FILES];
static inode *inodes[MAX_FILES];

static uint8_t filesystem[NUM_BLOCKS][BLOCK_SIZE];
static char *disk_image_name;
static uint8_t *free_inode_map;
static uint8_t *free_block_map;

static bool opened = false;

int find_next_free_block() {
  for(int i = 0; i < NUM_BLOCKS; i++)
    if(free_block_map[i])
      return i;
  return -1;
}

int find_next_free_inode() {
  for(int i = 0; i < MAX_FILES; i++)
    if(free_inode_map[i])
      return i;
  return -1;
}

int find_next_free_dir_entry() {
  for(int i = 0; i < MAX_FILES; i++)
    if(dir_entries[i]->in_use == 0)
      return i;
  return -1;
}

int find_dir_entry(char *filename) {
  for(int i = 0; i < MAX_FILES; i++)
    if(strncmp(dir_entries[i]->filename, filename, MAX_FILENAME) == 0)
      return i;
  return -1;
}

// Initialize the file system
int fs_createfs(char *name) {
  if(!name)
    return -1;
  
  if(opened) {
    printf("Error: Another file system is already open\n");
    return -1;
  }
  
  disk_image_name = strndup(name, MAX_FILENAME+1);
  
  // Initialize all data in blocks to 0
  for(int i = 0; i < NUM_BLOCKS; i++)
    memset(filesystem[i], 0, BLOCK_SIZE);
  
  size_t size = sizeof(dir_entry);
  for(int i = 0; i <= MAX_FILES; i++) {
    dir_entries[i] = (dir_entry *) &filesystem[0][size * i];
    inodes[i] = (inode *) filesystem[i+5];
  }
  
  // Set all inodes and blocks to free (1)
  memset(filesystem[2], 1, MAX_FILES);
  memset(filesystem[3], 1, NUM_BLOCKS);
  
  // Set blocks 0-130 to used (0)
  memset(filesystem[3], 0, MAX_FILES+6);
  
  free_inode_map = filesystem[2];
  free_block_map = filesystem[3];
  
  printf("%ld\n", BLOCK_SIZE/sizeof(dir_entry));
  
  opened = true;
  return 0;
}

int fs_savefs() {
  if(!opened) {
    printf("savefs error: No file system currently open\n");
    return -1;
  }

  // Now, open the output file that we are going to write the data to.
  FILE *ofp;
  ofp = fopen(disk_image_name, "w");

  if( ofp == NULL ) {
    printf("Could not open output file: %s\n", disk_image_name );
    perror("Opening output file returned");
    return -1;
  }

  // Initialize our offsets and pointers just we did above when reading from the file.
  block_ptr block_index = 0;
  int copy_size = BLOCK_SIZE * NUM_BLOCKS;
  int offset = 0;

  printf("Writing %d bytes to %s\n", copy_size, disk_image_name );

  // Using copy_size as a count to determine when we've copied enough bytes to the output file.
  // Each time through the loop, we will copy BLOCK_SIZE number of bytes from  our stored data
  // to the file fp, then we will increment the offset into the file we are writing to.
  while( copy_size > 0 ) {
    // Write BLOCK_SIZE number of bytes from our data array into our output file.
    fwrite( filesystem[block_index], BLOCK_SIZE, 1, ofp );

    // Reduce the amount of bytes remaining to copy, increase the offset into the file
    // and increment the block_index to move us to the next data block.
    copy_size -= BLOCK_SIZE;
    offset    += BLOCK_SIZE;
    block_index++;

    // Since we've copied from the point pointed to by our current file pointer, increment
    // offset number of bytes so we will be ready to copy to the next area of our output file.
    fseek( ofp, offset, SEEK_SET );
  }

  // Close the output file, we're done. 
  fclose( ofp );

  return 0;
}

int fs_setattrib(char *filename, ATTRIB a, bool enabled) {
  if(!opened)
    return -1;
  if(strnlen(filename, MAX_FILENAME+1) > MAX_FILENAME) {
    printf("attrib error: File name too long.\n");
    return -1;
  }
  
  int idx = find_dir_entry(filename);
  if(idx == -1) {
    return -1;
  }
  
  if(enabled)
    dir_entries[idx]->attrib |=  a & 0b11;
  else
    dir_entries[idx]->attrib &= ~a & 0b11;
  
  return 0;
}

int fs_open(char *filename) {
  if(opened) {
    printf("Error: File system already opened. You must close it first\n");
    return -1;
  }
  fs_createfs(filename);
  
  if(strnlen(filename, MAX_FILENAME+1) > MAX_FILENAME) {
    printf("open error: File name too long.\n");
    return -1;
  }
  
  int    status;                   // Hold the status of all return values.
  struct stat buf;                 // stat struct to hold the returns from the stat call

  // Call stat with out input filename to verify that the file exists.  It will also 
  // allow us to get the file size. We also get interesting file system info about the
  // file such as inode number, block size, and number of blocks.  For now, we don't 
  // care about anything but the filesize.
  status =  stat( filename, &buf ); 

  // If stat did return -1 then we know the input file doesn't exists or we can't use it.
  if( status == -1 )
  {
    printf("open error: Failed to read file.\n");
    return -1;
  }
  
  // Save off the size of the input file since we'll use it in a couple of places and 
  // also initialize our index variables to zero. 
  int copy_size   = buf . st_size;
  
  if(copy_size != NUM_BLOCKS * BLOCK_SIZE) {
    printf("open error: Image is not correct size.\n");
    return -1;
  }
  
  // Open the input file read-only 
  FILE *ofp = fopen ( filename, "r" ); 
  printf("Reading %d bytes from %s\n", copy_size, filename );


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
  while( copy_size > 0 )
  {

    // Index into the input file by offset number of bytes.  Initially offset is set to
    // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We 
    // then increase the offset by BLOCK_SIZE and continue the process.  This will
    // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
    fseek( ofp, offset, SEEK_SET );

    // Read BLOCK_SIZE number of bytes from the input file and store them in our
    // data array. 
    int bytes  = fread( filesystem[block_index], BLOCK_SIZE, 1, ofp );

    // If bytes == 0 and we haven't reached the end of the file then something is 
    // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
    // It means we've reached the end of our input file.
    if( bytes == 0 && !feof( ofp ) )
    {
      printf("An error occured reading from the input file.\n");
      fclose(ofp);
      return -1;
    }

    // Clear the EOF file flag.
    clearerr( ofp );

    // Reduce copy_size by the BLOCK_SIZE bytes.
    copy_size -= BLOCK_SIZE;
    
    // Increase the offset into our input file by BLOCK_SIZE.  This will allow
    // the fseek at the top of the loop to position us to the correct spot.
    offset    += BLOCK_SIZE;

    // Increment the index into the block array 
    block_index++;
  }

  // We are done copying from the input file so close it out.
  fclose( ofp );

  
  return 0;
}

int fs_close() {
  if(!opened)
    return -1;
  
  free(disk_image_name);
  opened = false;
  
  return 0;
}

int fs_list(bool show_hidden) {
  if(!opened)
    return -1;
  
  char time_buf[20] = { 0 };
  
  for(int i = 0; i < MAX_FILES; i++) {
    if(dir_entries[i]->in_use && (!(dir_entries[i]->attrib & H) || show_hidden)) {
      char h = dir_entries[i]->attrib & H ? 'H' : '-';
      char r = dir_entries[i]->attrib & R ? 'R' : '-';
      char *time_str = asctime(localtime(&dir_entries[i]->time_added));
      time_str[strlen(time_str)-1] = 0; // Remove newline character from string
      printf("%c%c %8d %s %s\n", h, r, dir_entries[i]->bytes, time_str, dir_entries[i]->filename);
    }
  }
  
  return 0;
}

int fs_put(char *filename) {
  if(!opened)
    return -1;
  
  if(strnlen(filename, MAX_FILENAME+1) > MAX_FILENAME) {
    printf("put error: File name too long.\n");
    return -1;
  }
  
  int    status;                   // Hold the status of all return values.
  struct stat buf;                 // stat struct to hold the returns from the stat call

  // Call stat with out input filename to verify that the file exists.  It will also 
  // allow us to get the file size. We also get interesting file system info about the
  // file such as inode number, block size, and number of blocks.  For now, we don't 
  // care about anything but the filesize.
  status =  stat( filename, &buf ); 

  // If stat did return -1 then we know the input file doesn't exists or we can't use it.
  if( status == -1 ) {
    printf("put error: Failed to read file.\n");
    return -1;
  }
  
  // Save off the size of the input file since we'll use it in a couple of places and 
  // also initialize our index variables to zero. 
  int copy_size   = buf . st_size;
  
  // If the size of the file is greater than the available space left,
  // we cannot fit this file. Return failure.
  if(copy_size > fs_df()) {
    printf("put error: Not enough disk space.\n");
    return -1;
  }
  
  // If the size of the file is greater than the maximum file size,
  // we cannot fit this file. Return failure.
  if(copy_size > MAX_FILE_SIZE) {
    printf("put error: File size is greater than maximum file size: %d\n", MAX_FILE_SIZE);
    return -1;
  }
  
  int dir_entry_idx = find_next_free_dir_entry();
  if(dir_entry_idx == -1) {
    printf("put error: Maximum amount of files has been reached (%d).\n", MAX_FILES);
    return -1;
  }
  int inode_idx = find_next_free_inode();
  if(inode_idx == -1) {
    printf("put error: Maximum amount of inodes has been reached (%d).\n", MAX_FILES);
    return -1;
  }
  
  strncpy(dir_entries[dir_entry_idx]->filename, filename, MAX_FILENAME);
  dir_entries[dir_entry_idx]->bytes = copy_size;
  dir_entries[dir_entry_idx]->time_added = time(NULL);
  dir_entries[dir_entry_idx]->in_use = 1;
  dir_entries[dir_entry_idx]->attrib = 0;
  dir_entries[dir_entry_idx]->inode = inode_idx;
  
  // Open the input file read-only 
  FILE *ifp = fopen ( filename, "r" ); 
  printf("Reading %d bytes from %s\n", copy_size, filename );


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
  while( copy_size > 0 ) {

    // Index into the input file by offset number of bytes.  Initially offset is set to
    // zero so we copy BLOCK_SIZE number of bytes from the front of the file.  We 
    // then increase the offset by BLOCK_SIZE and continue the process.  This will
    // make us copy from offsets 0, BLOCK_SIZE, 2*BLOCK_SIZE, 3*BLOCK_SIZE, etc.
    fseek( ifp, offset, SEEK_SET );

    // Read BLOCK_SIZE number of bytes from the input file and store them in our
    // data array. 
    int bytes  = fread( filesystem[block_index], BLOCK_SIZE, 1, ifp );

    // If bytes == 0 and we haven't reached the end of the file then something is 
    // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
    // It means we've reached the end of our input file.
    if( bytes == 0 && !feof( ifp ) ) {
      printf("An error occured reading from the input file.\n");
      fclose(ifp);
      return -1;
    }

    // Clear the EOF file flag.
    clearerr( ifp );

    // Reduce copy_size by the BLOCK_SIZE bytes.
    copy_size -= BLOCK_SIZE;
    
    // Increase the offset into our input file by BLOCK_SIZE.  This will allow
    // the fseek at the top of the loop to position us to the correct spot.
    offset    += BLOCK_SIZE;

    free_block_map[block_index] = 0;
    inodes[inode_idx]->direct_data_blocks[direct_data_block_idx] = block_index;
    direct_data_block_idx++;
    
    // Increment the index into the block array 
    block_index = find_next_free_block();
  }

  // We are done copying from the input file so close it out.
  fclose( ifp );
  return 0;
}

int fs_get(char *filename, char *newfilename) {
  int    status;                   // Hold the status of all return values.
  struct stat buf;                 // stat struct to hold the returns from the stat call

  // Call stat with out input filename to verify that the file exists.  It will also 
  // allow us to get the file size. We also get interesting file system info about the
  // file such as inode number, block size, and number of blocks.  For now, we don't 
  // care about anything but the filesize.
  status =  stat( filename, &buf ); 

  // If stat did not return -1 then we know the input file exists and we can use it.
  if( status != -1 ) {
    printf("get error: Unable to write file\n");
    return -1;
  }
  
  int dir_entry_idx = find_dir_entry(filename);
  if(dir_entry_idx == -1 || !dir_entries[dir_entry_idx]->in_use) {
    printf("get error: Unable to find file \"%s\".\n", filename);
    return -1;
  }
  int inode_idx = dir_entries[dir_entry_idx]->inode;
  
  // Now, open the output file that we are going to write the data to.
  FILE *ofp;
  ofp = fopen(filename, "w");

  if( ofp == NULL ) {
    printf("Could not open output file: %s\n", filename );
    perror("Opening output file returned");
    return -1;
  }

  // Initialize our offsets and pointers just we did above when reading from the file.
  block_ptr direct_block_idx = 0;
  int copy_size   = dir_entries[dir_entry_idx]->bytes;
  int offset      = 0;

  printf("Writing %d bytes to %s\n", copy_size, filename );

  // Using copy_size as a count to determine when we've copied enough bytes to the output file.
  // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
  // our stored data to the file fp, then we will increment the offset into the file we are writing to.
  // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
  // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
  // last iteration we'd end up with gibberish at the end of our file. 
  while( copy_size > 0 ) {

    int num_bytes;

    // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
    // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
    // end up with garbage at the end of the file.
    if( copy_size < BLOCK_SIZE ) {
      num_bytes = copy_size;
    } else {
      num_bytes = BLOCK_SIZE;
    }

    // Write num_bytes number of bytes from our data array into our output file.
    uint8_t *block = filesystem[inodes[inode_idx]->direct_data_blocks[direct_block_idx]];
    fwrite( block, num_bytes, 1, ofp ); 

    // Reduce the amount of bytes remaining to copy, increase the offset into the file
    // and increment the block_index to move us to the next data block.
    copy_size -= BLOCK_SIZE;
    offset    += BLOCK_SIZE;
    direct_block_idx++;

    // Since we've copied from the point pointed to by our current file pointer, increment
    // offset number of bytes so we will be ready to copy to the next area of our output file.
    fseek( ofp, offset, SEEK_SET );
  }

  // Close the output file, we're done. 
  fclose( ofp );
  return 0;
}

int fs_del() {
  
}

int fs_undel() {
  
}

int fs_df() {
  int count = 0;
  for(int i = 0; i < NUM_BLOCKS; i++)
    if(free_block_map[i])
      count++;
  return count * BLOCK_SIZE;
}
