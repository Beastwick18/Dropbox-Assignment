#ifndef CSE3320_FILESYSTEM_H
#define CSE3320_FILESYSTEM_H

#include <stdbool.h>

typedef enum {
  R = 0b01,
  H = 0b10,
} ATTRIB;

int fs_createfs(char *disk_image_name);

int fs_savefs();

int fs_setattrib(char *filename, ATTRIB a, bool enabled);

int fs_open(char *image);

int fs_close();

int fs_list(bool show_hidden);

int fs_put(char *filename);

int fs_get(char *filename, char *newfilename);

int fs_del();

int fs_undel();

int fs_df();

#endif
