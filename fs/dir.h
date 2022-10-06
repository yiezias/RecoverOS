#ifndef __FS_DIR_H
#define __FS_DIR_H
#include "../lib/stdint.h"
#include "fs.h"
#include "inode.h"

#define MAX_FILE_NAME_LEN 16
struct dir_entry {
	char filename[MAX_FILE_NAME_LEN];
	size_t i_no;
	enum file_types f_type;
};

struct dir {
	struct inode *inode;
	size_t dir_pos;
	struct dir_entry dir_buf;
};


extern struct inode *root_dir_inode;
struct inode *dir_look_up(struct inode *parent_inode, const char *name,
			  size_t *poff);
bool dir_link(struct inode *parent_inode, struct dir_entry dir_e);
bool dir_unlink(struct inode *parent_inode, const char *name);
#endif
