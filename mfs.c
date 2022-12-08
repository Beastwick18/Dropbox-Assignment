// Steven Culwell
// 1001783662

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

// put <filename>: Copy the local file to the filesystem image
int put_cmd(char **token, int token_count) {
  if(token_count != 3) {
    printf("put error: Expected `put <filename>`\n");
    return -1;
  }
  
  char *filename = token[1];
  if(!filename) {
    printf("put error: File name must not be empty\n");
    return -1;
  }
  
  return fs_put(filename);
}

// get <filename>: Retrieve the file from the filesystem image
// get <filename> <newfilename>: Retrieve the file form the file
// system image and place it in the file named <newfilename>
int get_cmd(char **token, int token_count) {
  if(token_count != 3 && token_count != 4) {
    printf("geterror: Expected `get <filename>` or `get <filename> <newfilename>`\n");
    return -1;
  }
  
  char *filename = token[1];
  if(!filename) {
    printf("get error: File name must not be empty\n");
    return -1;
  }
  
  char *newfilename = filename;
  if(token_count == 4)
    newfilename = token[2];
  
  if(!newfilename) {
    printf("get error: New file name must not be empty\n");
    return -1;
  }
  
  return fs_get(filename, newfilename);
  
}

// del <filename>: Delete the file
int del_cmd(char **token, int token_count) {
  if(token_count != 3) {
    printf("del error: Expected `del <filename>`\n");
    return -1;
  }
  
  char *filename = token[1];
  if(!filename) {
    printf("del error: File name must not be empty\n");
    return -1;
  }
  
  return fs_del(filename);
}

// undel <filename>: Undelete the file if found in the directory
int undel_cmd(char **token, int token_count) {
  if(token_count != 3) {
    printf("undel error: Expected `undel <filename>`\n");
    return -1;
  }
  
  char *filename = token[1];
  if(!filename) {
    printf("undel error: File name must not be empty\n");
    return -1;
  }
  
  return fs_undel(filename);
}

// list [-h]: List the files in the file system image
int list_cmd(char **token, int token_count) {
  if(token_count != 3 && token_count != 2) {
    printf("list error: Expected `list [-h]`\n");
    return -1;
  }
  
  bool show_hidden = token_count == 3 && token[1] && strncmp("-h", token[1], 3) == 0;
  return fs_list(show_hidden);
}

// df: Display the amount of disk space left in the file system
int df_cmd(char **token, int token_count) {
  if(token_count != 2) {
    printf("df error: Expected `df`\n");
    return -1;
  }
  
  int df = fs_df();
  printf("%d bytes free\n", df);
  
  return 0;
}

// open <file image name>: Open a file system image
int open_cmd(char **token, int token_count) {
  if(token_count != 3) {
    printf("open error: Expected `open <file image name>`\n");
    return -1;
  }
  
  char *file_image_name = token[1];
  if(!file_image_name) {
    printf("open error: File image name must not be empty\n");
    return -1;
  }
  
  return fs_open(file_image_name);
}

// close: Close the currently opened file system
int close_cmd(char **token, int token_count) {
  // Command should only have 1 token
  if(token_count != 2) {
    printf("close error: Expected `close`\n");
    return -1;
  }
  
  int result = fs_close();
  if(result == -1)
    printf("close error: No file system is currently open\n");
  
  return result;
}

// createfs <disk image name>: Create a new file system image
int createfs_cmd(char **token, int token_count) {
  // Command should have 2 tokens
  if(token_count != 3) {
    printf("createfs error: Expected `createfs <disk image name>`\n");
    return 1;
  }
  
  char *disk_image_name = token[1];
  if(!disk_image_name) {
    printf("createfs error: Disk image name must not be empty\n");
    return -1;
  }
  
  return fs_createfs(disk_image_name);
}

// savefs: Save the current file system image
int savefs_cmd(char **token, int token_count) {
  // Command should only have 1 token
  if(token_count != 2) {
    printf("savefs error: Expected `savefs`\n");
    return -1;
  }
  return fs_savefs();
}

// attrib [+attribute] [ -attribute] <filename>: Set or remove the attribute for the file
int attrib_cmd(char **token, int token_count) {
  // Command should only have 3 tokens
  if(token_count != 4) {
    printf("attrib error: Expected `attrib [+attribute] [-attribute] <file>`\n");
    return -1;
  }
  // Validate first argument is properly formatted by checking if the argument is 2
  // characters, the first character is either a '+' or '-' and the second character
  // is either an 'r' or an 'h'
  if(!token[1] || strnlen(token[1], 3) != 2 || (token[1][0] != '+' && token[1][0] != '-') ||
      (token[1][1] != 'r' && token[1][1] != 'h')) {
    printf("attrib error: Expected `attrib [+attribute] [-attribute] <file>` where attribute \
is either 'r' or 'h'\n");
    return -1;
  }
  if(!token[2]) {
    printf("attrib error: File name must not be empty\n");
    return -1;
  }
  
  bool enabled = token[1][0] == '+';
  attrib a = token[1][1] == 'r' ? R : H;
  return fs_setattrib(token[2], a, enabled);
}

int main() {

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  int running = 1;
  
  while( running ) {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS] = { NULL };

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
              (token_count<MAX_NUM_ARGUMENTS)) {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 ) {
        free(token[token_count]);
        token[token_count] = NULL;
      }
      token_count++;
    }

    if(token_count == 1 || token[0] == NULL) {
      free(working_root);
      continue;
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
      printf("mfs error: Unknown command: `%s`\n", token[0]);
    }

    for(int i = 0; i < token_count; i++) {
      if(token[i])
        free(token[i]);
    }
    free( working_root );
  }
  
  free(cmd_str);
  fs_close();
  return 0;
}
