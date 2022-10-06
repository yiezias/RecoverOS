#include "wait_exit.h"
#include "../fs/file.h"
#include "../fs/fs.h"
#include "../kernel/debug.h"
#include "../pipe/pipe.h"

static bool find_hanging_child(struct list_elem *elem, void *ppid) {
	struct task_struct *pthread =
		elem2entry(struct task_struct, all_list_tag, elem);
	if (pthread->parent_pid == *(pid_t *)ppid
	    && pthread->stat == TASK_HANGING) {
		return true;
	}
	return false;
}

static bool find_child(struct list_elem *elem, void *ppid) {
	struct task_struct *pthread =
		elem2entry(struct task_struct, all_list_tag, elem);
	if (pthread->parent_pid == *(pid_t *)ppid) {
		return true;
	}
	return false;
}

static bool init_adopt_a_child(struct list_elem *pelem, void *pid) {
	struct task_struct *pthread =
		elem2entry(struct task_struct, all_list_tag, pelem);
	if (pthread->parent_pid == *(pid_t *)pid) {
		pthread->parent_pid = 1;
	}
	return false;
}

static void release_prog_resource(struct task_struct *release_thread) {
	release_prog_mem(release_thread);

	for (int local_fd = 3; local_fd != MAX_FILES_OPEN_PER_PROC;
	     ++local_fd) {
		if (release_thread->fd_table[local_fd] != -1) {
			if (is_pipe(local_fd)) {
				uint32_t global_fd = fd_local2global(local_fd);
				if (--file_table[global_fd].fd_pos == 0) {
					free_pages(
						&k_v_pool,
						file_table[global_fd].fd_inode,
						1);
					file_table[global_fd].fd_inode = NULL;
				}
			} else {
				sys_close(local_fd);
			}
		}
	}
}

pid_t sys_wait(int *status) {
	struct task_struct *parent_thread = running_thread();

	while (1) {
		struct list_elem *child_elem =
			list_traversal(&all_threads_list, find_hanging_child,
				       &parent_thread->pid);
		if (child_elem != NULL) {
			struct task_struct *child_thread = elem2entry(
				struct task_struct, all_list_tag, child_elem);
			*status = child_thread->exit_status;
			pid_t child_pid = child_thread->pid;
			thread_exit(child_thread, false);
			return child_pid;
		}
		child_elem = list_traversal(&all_threads_list, find_child,
					    &parent_thread->pid);
		if (child_elem == NULL) {
			return -1;
		} else {
			thread_block(TASK_WAITING);
		}
	}
}


void sys_exit(int status) {
	struct task_struct *child_thread = running_thread();
	child_thread->exit_status = status;
	ASSERT(child_thread->parent_pid != -1);

	list_traversal(&all_threads_list, init_adopt_a_child,
		       &child_thread->pid);

	release_prog_resource(child_thread);

	struct task_struct *parent_thread =
		pid2thread(child_thread->parent_pid);
	if (parent_thread->stat == TASK_WAITING) {
		thread_unblock(parent_thread);
	}
	thread_block(TASK_HANGING);
}
