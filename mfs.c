// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "filesystem.h"

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

// put <filename: Copy the local file to the filesystem image
int put_cmd(char **token, int token_count) {
  if(token_count < 3) {
    printf("Error: Not enough arguments. Expected `put <filename>`\n");
    return 1;
  }
  if(token_count > 3) {
    printf("Error: Too many arguments. Expected `put <filename>`\n");
    return 1;
  }
  
  char *filename = token[1];
  if(!filename) {
    printf("Error: File name must not be empty\n");
    return 1;
  }
  
  // Stuff
  
  return 0;
}

// get <filename>: Retrieve the file from the filesystem image
// get <filename> <newfilename>: Retrieve the file form the file
// system image and place it in the file named <newfilename>
int get_cmd(char **token, int token_count) {
  if(token_count < 3) {
    printf("Error: Not enough arguments. Expected `get <filename>`\n");
    return 1;
  }
  if(token_count > 3) {
    printf("Error: Too many arguments. Expected `get <filename>`\n");
    return 1;
  }
  
  char *filename = token[1];
  if(!filename) {
    printf("Error: File name must not be empty\n");
  }
  
  // Stuff
  
  return 0;
}

// del <filename>: Delete the file
int del_cmd(char **token, int token_count) {
  if(token_count < 3) {
    printf("Error: Not enough arguments. Expected `del <filename>`\n");
    return 1;
  }
  if(token_count > 3) {
    printf("Error: Too many arguments. Expected `del <filename>`\n");
    return 1;
  }
  
  char *filename = token[1];
  if(!filename) {
    printf("Error: File name must not be empty\n");
  }
  
  // Stuff
  
  return 0;
}

// undel <filename>: Undelete the file if found in the directory
int undel_cmd(char **token, int token_count) {
  if(token_count < 3) {
    printf("Error: Not enough arguments. Expected `undel <filename>`\n");
    return 1;
  }
  if(token_count > 3) {
    printf("Error: Too many arguments. Expected `undel <filename>`\n");
    return 1;
  }
  
  char *filename = token[1];
  if(!filename) {
    printf("Error: File name must not be empty\n");
  }
  
  // Stuff
  
  return 0;
}

// list [-h]: List the files in the file system image
int list_cmd(char **token, int token_count) {
  
}

// df: Display the amount of disk space left in the file system
int df_cmd(char **token, int token_count) {
  
}

// open <file image name>: Open a file system image
int open_cmd(char **token, int token_count) {
  if(token_count < 3) {
    printf("Error: Not enough arguments. Expected `open <file image name>`\n");
    return 1;
  }
  if(token_count > 3) {
    printf("Error: Too many arguments. Expected `open <file image name>`\n");
    return 1;
  }
  
  char *file_image_name = token[1];
  if(!file_image_name) {
    printf("Error: File image name must not be empty\n");
  }
  
  // Stuff
  
  return 0;
}

// close: Close the currently opened file system
int close_cmd(char **token, int token_count) {
  
}

// createfs <disk image name>: Create a new file system image
int createfs_cmd(char **token, int token_count) {
  if(token_count < 3) {
    printf("Error: Not enough arguments. Expected `createfs <disk image name>`\n");
    return 1;
  }
  if(token_count > 3) {
    printf("Error: Too many arguments. Expected `createfs <disk image name>`\n");
    return 1;
  }
  
  char *disk_image_name = token[1];
  if(!disk_image_name) {
    printf("Error: Disk image name must not be empty\n");
  }
  
  // Stuff
  
  return 0;
}

// savefs: Save the current file system image
int savefs_cmd(char **token, int token_count) {
  
}

// attrib [+attribute] [ -attribute] <filename>: Set or remove the attribute for the file
int attrib_cmd(char **token, int token_count) {
  
}

int main()
{

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  int running = 1;
  
  while( running )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( running && ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
      token_count++;
    }

    if(strncmp("quit", token[0], MAX_COMMAND_SIZE) == 0 ||
        strncmp("exit", token[0], MAX_COMMAND_SIZE) == 0) {
      running = 0;
    } else if(strncmp("put", token[0], MAX_COMMAND_SIZE) == 0) {
      put_cmd(token, token_count);
    } else if(strncmp("get", token[0], MAX_COMMAND_SIZE) == 0) {
      get_cmd(token, token_count);
    } else if(strncmp("del", token[0], MAX_COMMAND_SIZE) == 0) {
      del_cmd(token, token_count);
    } else if(strncmp("undel", token[0], MAX_COMMAND_SIZE) == 0) {
      undel_cmd(token, token_count);
    } else if(strncmp("list", token[0], MAX_COMMAND_SIZE) == 0) {
      list_cmd(token, token_count);
    } else if(strncmp("df", token[0], MAX_COMMAND_SIZE) == 0) {
      df_cmd(token, token_count);
    } else if(strncmp("open", token[0], MAX_COMMAND_SIZE) == 0) {
      open_cmd(token, token_count);
    } else if(strncmp("close", token[0], MAX_COMMAND_SIZE) == 0) {
      close_cmd(token, token_count);
    } else if(strncmp("createfs", token[0], MAX_COMMAND_SIZE) == 0) {
      createfs_cmd(token, token_count);
    } else if(strncmp("savefs", token[0], MAX_COMMAND_SIZE) == 0) {
      savefs_cmd(token, token_count);
    } else if(strncmp("attrib", token[0], MAX_COMMAND_SIZE) == 0) {
      attrib_cmd(token, token_count);
    } else {
      printf("Error: Unknown command: `%s`", token[0]);
    }

    // int token_index  = 0;
    // for( token_index = 0; token_index < token_count; token_index ++ ) 
    // {
    //   printf("token[%d] = %s\n", token_index, token[token_index] );  
    // }

    free( working_root );

  }
  return 0;
}
