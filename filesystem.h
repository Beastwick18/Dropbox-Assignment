#ifndef CSE3320_FILESYSTEM_H
#define CSE3320_FILESYSTEM_H

int fs_createfs(char *disk_image_name);

int fs_savefs();

int fs_setattrib();

int fs_open(char *filename);

int fs_close();

int fs_list();

int fs_put();

int fs_get();

int fs_del();

int fs_undel();

int fs_df();

#endif
