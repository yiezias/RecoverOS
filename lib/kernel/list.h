#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H
#include "../../kernel/global.h"
#include "../stdint.h"


#define elem2entry(struct_name, member_name, member_ptr) \
	(struct_name *)((uint64_t)member_ptr             \
			- ((uint64_t) & ((struct_name *)0)->member_name))
struct list_elem {
	struct list_elem *prev;
	struct list_elem *next;
};

struct list {
	struct list_elem head;
	struct list_elem tail;
};

typedef bool(function)(struct list_elem *, void *arg);

void list_init(struct list *list);

void list_insert(struct list_elem *before, struct list_elem *elem);
void list_remove(struct list_elem *elem);

void list_push(struct list *plist, struct list_elem *elem);
void list_append(struct list *plist, struct list_elem *elem);
struct list_elem *list_pop(struct list *plist);

bool elem_find(struct list *plist, struct list_elem *pelem);
bool list_empty(struct list *plist);
struct list_elem *list_traversal(struct list *plist, function func, void *arg);
size_t list_len(struct list *plist);

#endif
