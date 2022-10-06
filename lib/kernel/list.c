#include "list.h"
#include "../../kernel/intr.h"

void list_init(struct list *list) {
	list->head.prev = NULL;
	list->head.next = &list->tail;
	list->tail.prev = &list->head;
	list->tail.next = NULL;
}

void list_insert(struct list_elem *before, struct list_elem *elem) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	elem->prev = before->prev;
	elem->next = before;
	elem->prev->next = elem;
	before->prev = elem;
	set_intr_stat(old_stat);
}

void list_remove(struct list_elem *elem) {
	enum intr_stat old_stat = set_intr_stat(intr_off);
	elem->prev->next = elem->next;
	elem->next->prev = elem->prev;
	set_intr_stat(old_stat);
}

bool elem_find(struct list *plist, struct list_elem *pelem) {
	struct list_elem *elem = plist->head.next;
	while (elem != &plist->tail) {
		if (elem == pelem) {
			return true;
		}
		elem = elem->next;
	}
	return false;
}

void list_push(struct list *plist, struct list_elem *elem) {
	list_insert(plist->head.next, elem);
}

void list_append(struct list *plist, struct list_elem *elem) {
	list_insert(&plist->tail, elem);
}

struct list_elem *list_pop(struct list *plist) {
	struct list_elem *elem = plist->head.next;
	list_remove(elem);
	return elem;
}

bool list_empty(struct list *plist) {
	return plist->head.next == &plist->tail;
}

struct list_elem *list_traversal(struct list *plist, function func, void *arg) {
	struct list_elem *elem = plist->head.next;
	while (elem != &plist->tail) {
		if (func(elem, arg)) {
			return elem;
		}
		elem = elem->next;
	}
	return NULL;
}

size_t list_len(struct list *plist) {
	struct list_elem *elem = plist->head.next;
	size_t len = 0;
	while (elem != &plist->tail) {
		++len;
		elem = elem->next;
	}

	return len;
}
