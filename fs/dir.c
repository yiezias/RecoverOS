#include "dir.h"
#include "../kernel/debug.h"
#include "../kernel/memory.h"
#include "../lib/stdio.h"
#include "../lib/string.h"

struct dir *root_dir;
struct inode *root_dir_inode;

struct inode *dir_look_up(struct inode *parent_inode, const char *name,
			  size_t *poff) {
	ASSERT(parent_inode->f_type == FT_DIRECTORY);

	struct dir_entry de;
	for (size_t off = 0; off != parent_inode->i_size;
	     off += sizeof(struct dir_entry)) {
		if (inode_read(parent_inode, &de, sizeof(struct dir_entry), off)
		    == -1) {
			PANIC("dir_look_up: inode_read failedn");
		}
		if (strcmp(name, de.filename) == 0) {
			*poff = off;
			return inode_open(cur_part, de.i_no);
		}
	}
	return NULL;
}

bool dir_link(struct inode *parent_inode, struct dir_entry dir_e) {
	size_t off;
	struct inode *cinode;
	if ((cinode = dir_look_up(parent_inode, dir_e.filename, &off))
	    != NULL) {
		printk("dir_link: dir_entry already exists\n");
		inode_close(cinode);
		return false;
	}

	struct dir_entry de;
	for (size_t off = 0; off != parent_inode->i_size;
	     off += sizeof(struct dir_entry)) {
		if (inode_read(parent_inode, &de, sizeof(struct dir_entry), off)
		    == -1) {
			PANIC("dir_look_up: inode_read failedn");
		}
		if (de.f_type == FT_UNKNOWN) {
			if (inode_write(parent_inode, &dir_e,
					sizeof(struct dir_entry), off)
			    == -1) {
				return false;
			} else {
				return true;
			}
		}
	}
	if (inode_write(parent_inode, &dir_e, sizeof(struct dir_entry),
			parent_inode->i_size)
	    == -1) {
		return false;
	} else {
		return true;
	}
}

bool dir_unlink(struct inode *parent_inode, const char *name) {
	struct inode *cinode;
	size_t off;
	if ((cinode = dir_look_up(parent_inode, name, &off)) == NULL) {
		printk("no such file or directory\n");
		return false;
	}
	inode_close(cinode);
	struct dir_entry dir_e = { { 0 }, 0, FT_UNKNOWN };
	inode_write(parent_inode, &dir_e, sizeof(struct dir_entry), off);
	return true;
}

/*
struct dir *dir_open(struct partition *part, size_t inode_no) {
	struct dir *pdir = (struct dir *)sys_malloc(sizeof(struct dir));
	pdir->inode = inode_open(part, inode_no);
	pdir->dir_pos = 0;
	return pdir;
}*/
/*
void dir_close(struct dir *pdir) {
	if (pdir->inode == root_dir_inode) {
		return;
	}
	inode_close(pdir->inode);
	sys_free(pdir);
}

void dir_read(struct dir *pdir, struct dir_entry *dir_e) {
	inode_read(pdir->inode, dir_e, pdir->dir_pos,pdir->dir_pos);
}
*/
