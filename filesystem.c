// Steven Culwell
// 1001783662

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "filesystem.h"

#define MAX_FILENAME    32
#define MAX_FILES       125
#define NUM_BLOCKS      4226
#define BLOCK_SIZE      8192
#define MAX_FILE_SIZE   10240000
#define NUM_DATA_BLOCKS 1250

#define NULL_FILE     0x0

#define BYTES_TO_BLOCKS(bytes) (((bytes)+BLOCK_SIZE-1)/BLOCK_SIZE);

typedef uint8_t inode_ptr;
typedef uint16_t block_ptr;

// 2501B
typedef struct {
  block_ptr direct_data_blocks[1250];
  bool deleted;
} inode;

// 38B
typedef struct {
  char filename[MAX_FILENAME];
  inode_ptr inode;
  uint8_t attrib;
} dir_entry;

static dir_entry *dir_entries[MAX_FILES];
static inode *inodes[MAX_FILES];
// inodes[0] = (inode *) (&filesystem[0 + 5]) // from 0-125
static uint8_t filesystem[NUM_BLOCKS][BLOCK_SIZE];
static char *disk_image_name;

static bool created = false;

int find_dir_entry(char *filename) {
  for(int i = 0; i < MAX_FILES; i++) {
    if(strncmp(dir_entries[i]->filename, filename, MAX_FILENAME) == 0) {
      return i;
    }
  }
  return -1;
}

// Initialize the file system
int fs_createfs(char *name) {
  if(!disk_image_name) {
    return -1;
  }
  disk_image_name = strndup(disk_image_name, MAX_FILENAME+1);
  
  // Initialize all data in blocks to 0
  for(int i = 0; i < NUM_BLOCKS; i++) {
    memset(filesystem[i], 0, BLOCK_SIZE);
  }
  
  for(int i = 0; i < MAX_FILES; i++) {
    // dir_entries[i].attrib = 0;
    // memset(dir_entries[i].filename, 0, MAX_FILENAME);
    // dir_entries[i].inode = 0;
    
    // memset(inodes[i].direct_data_blocks, 0, 1250);
    // inodes[i].deleted = false;
  }
  
  created = true;
  return 0;
}

int fs_savefs() {
  //
  // The following chunk of code demonstrates similar functionality of your put command
  //

  int    status;                   // Hold the status of all return values.
  struct stat buf;                 // stat struct to hold the returns from the stat call

  // Call stat with out input filename to verify that the file exists.  It will also 
  // allow us to get the file size. We also get interesting file system info about the
  // file such as inode number, block size, and number of blocks.  For now, we don't 
  // care about anything but the filesize.
  status =  stat( disk_image_name, &buf );

  // If stat did not return -1 then we know the input file exists and we can use it.
  if(status == -1)
  {
    printf("Unable to open file: %s\n", disk_image_name );
    perror("Opening the input file returned: ");
    return -1;
  }
  // The following chunk of code demonstrates similar functionality to your get command
  //

  // Now, open the output file that we are going to write the data to.
  FILE *ofp;
  ofp = fopen(disk_image_name, "w");

  if( ofp == NULL )
  {
    printf("Could not open output file: %s\n", disk_image_name );
    perror("Opening output file returned");
    return -1;
  }

  // Initialize our offsets and pointers just we did above when reading from the file.
  block_ptr block_index = 0;
  int copy_size   = buf . st_size;
  int offset      = 0;

  printf("Writing %d bytes to %s\n", (int) buf . st_size, disk_image_name );

  // Using copy_size as a count to determine when we've copied enough bytes to the output file.
  // Each time through the loop, except the last time, we will copy BLOCK_SIZE number of bytes from
  // our stored data to the file fp, then we will increment the offset into the file we are writing to.
  // On the last iteration of the loop, instead of copying BLOCK_SIZE number of bytes we just copy
  // how ever much is remaining ( copy_size % BLOCK_SIZE ).  If we just copied BLOCK_SIZE on the
  // last iteration we'd end up with gibberish at the end of our file. 
  while( copy_size > 0 )
  { 

    int num_bytes;

    // If the remaining number of bytes we need to copy is less than BLOCK_SIZE then
    // only copy the amount that remains. If we copied BLOCK_SIZE number of bytes we'd
    // end up with garbage at the end of the file.
    if( copy_size < BLOCK_SIZE )
    {
      num_bytes = copy_size;
    }
    else 
    {
      num_bytes = BLOCK_SIZE;
    }

    // Write num_bytes number of bytes from our data array into our output file.
    fwrite( filesystem[block_index], num_bytes, 1, ofp ); 

    // Reduce the amount of bytes remaining to copy, increase the offset into the file
    // and increment the block_index to move us to the next data block.
    copy_size -= BLOCK_SIZE;
    offset    += BLOCK_SIZE;
    block_index ++;

    // Since we've copied from the point pointed to by our current file pointer, increment
    // offset number of bytes so we will be ready to copy to the next area of our output file.
    fseek( ofp, offset, SEEK_SET );
  }

  // Close the output file, we're done. 
  fclose( ofp );

  return 0;
}

int fs_setattrib(char *filename, ATTRIB a, bool enabled) {
  if(!created)
    return -1;
  
  int idx = find_dir_entry(filename);
  if(idx == -1)
    return -1;
  
  if(enabled)
    dir_entries[idx].attrib |=  a & 0b11;
  else
    dir_entries[idx].attrib &= ~a & 0b11;
  
  return 0;
}

int fs_open(char *filename) {
}

int fs_close() {
  
}

int fs_list() {
  
}

int fs_put(char *filename) {
  int status;
  
  struct stat buf;
  status =  stat( filename, &buf ); 

  // If stat did not return -1 then we know the input file exists and we can use it.
  if( status == -1 ) {
    printf("Error: Unable to open file\n");
    return -1;
  }
    // Open the input file read-only 
  FILE *ifp = fopen ( filename, "r" ); 
  printf("Reading %d bytes from %s\n", (int) buf . st_size, filename );

  int copy_size = buf.st_size;

  int offset = 0;               

  int block_index = 0;

  while( copy_size > 0 ) {
    fseek( ifp, offset, SEEK_SET );

    int bytes = fread( filesystem[block_index], BLOCK_SIZE, 1, ifp );

    // If bytes == 0 and we haven't reached the end of the file then something is 
    // wrong. If 0 is returned and we also have the EOF flag set then that is OK.
    // It means we've reached the end of our input file.
    if( bytes == 0 && !feof( ifp ) ) {
      printf("An error occured reading from the input file.\n");
      return -1;
    }

    // Clear the EOF file flag.
    clearerr( ifp );

    // Reduce copy_size by the BLOCK_SIZE bytes.
    copy_size -= BLOCK_SIZE;
    
    // Increase the offset into our input file by BLOCK_SIZE.  This will allow
    // the fseek at the top of the loop to position us to the correct spot.
    offset += BLOCK_SIZE;

    // Increment the index into the block array 
    block_index++;
  }

  // We are done copying from the input file so close it out.
  fclose( ifp );
  
  return 0;
  
}

int fs_get() {
  
}

int fs_del() {
  
}

int fs_undel() {
  
}

int main( int argc, char *argv[] )
{

};
