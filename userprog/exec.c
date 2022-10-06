#include "exec.h"
#include "../fs/fs.h"
#include "../kernel/global.h"
#include "../kernel/memory.h"
#include "../lib/stdio.h"
#include "../lib/string.h"
#include "../thread/thread.h"

extern void system_ret(void);

/*下面都是从 elf.h 复制过来的*/

#define EI_NIDENT (16)
/* Type for a 16-bit quantity.  */
typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf32_Word;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf64_Word;
typedef int32_t Elf64_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf32_Xword;
typedef int64_t Elf32_Sxword;
typedef uint64_t Elf64_Xword;
typedef int64_t Elf64_Sxword;

/* Type of addresses.  */
typedef uint32_t Elf32_Addr;
typedef uint64_t Elf64_Addr;

/* Type of file offsets.  */
typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Off;

#define ET_EXEC 2    /* Executable file */
#define EM_X86_64 62 /* AMD x86-64 architecture */

typedef struct {
	unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
	Elf64_Half e_type;		  /* Object file type */
	Elf64_Half e_machine;		  /* Architecture */
	Elf64_Word e_version;		  /* Object file version */
	Elf64_Addr e_entry;		  /* Entry point virtual address */
	Elf64_Off e_phoff;		  /* Program header table file offset */
	Elf64_Off e_shoff;		  /* Section header table file offset */
	Elf64_Word e_flags;		  /* Processor-specific flags */
	Elf64_Half e_ehsize;		  /* ELF header size in bytes */
	Elf64_Half e_phentsize;		  /* Program header table entry size */
	Elf64_Half e_phnum;		  /* Program header table entry count */
	Elf64_Half e_shentsize;		  /* Section header table entry size */
	Elf64_Half e_shnum;		  /* Section header table entry count */
	Elf64_Half e_shstrndx; /* Section header string table index */
} Elf64_Ehdr;

typedef struct {
	Elf64_Word p_type;    /* Segment type */
	Elf64_Word p_flags;   /* Segment flags */
	Elf64_Off p_offset;   /* Segment file offset */
	Elf64_Addr p_vaddr;   /* Segment virtual address */
	Elf64_Addr p_paddr;   /* Segment physical address */
	Elf64_Xword p_filesz; /* Segment size in file */
	Elf64_Xword p_memsz;  /* Segment size in memory */
	Elf64_Xword p_align;  /* Segment alignment */
} Elf64_Phdr;

#define PT_LOAD 1 /* Loadable program segment */

/*上面复制自 elf.h*/


static bool segment_load(int32_t fd, size_t offset, size_t filesz,
			 size_t vaddr) {
	uint64_t vaddr_first_page = vaddr & 0xfffffffffffff000;
	uint32_t size_in_first_page = PG_SIZE - (vaddr & 0xfff);
	uint32_t occupy_pages = 0;

	if (filesz > size_in_first_page) {
		uint32_t left_size = filesz - size_in_first_page;
		occupy_pages = DIV_ROUND_UP(left_size, PG_SIZE) + 1;
	} else {
		occupy_pages = 1;
	}

	uint64_t page_idx = 0;
	uint64_t vaddr_page = vaddr_first_page;
	while (page_idx < occupy_pages) {
		if (map_stat((void *)vaddr_page) != 4) {
			if (get_pages(vaddr_page, 1) == NULL) {
				return false;
			}
			struct virt_pool *vpool = &running_thread()->u_v_pool;
			bitmap_set(&vpool->pool_bitmap,
				   (vaddr_page - vpool->start) / PG_SIZE, 1);
		}
		vaddr_page += PG_SIZE;
		++page_idx;
	}
	sys_lseek(fd, offset, SEEK_SET);
	sys_read(fd, (void *)vaddr, filesz);
	return true;
}

static int64_t load(const char *pathname) {
	int64_t ret = -1;
	Elf64_Ehdr elf_header;
	Elf64_Phdr prog_header;
	memset(&elf_header, 0, sizeof(Elf64_Ehdr));

	int64_t fd = sys_open(pathname, O_RDONLY);
	if (fd == -1) {
		printk("load: open failed\n");
		return -1;
	}

	if (sys_read(fd, &elf_header, sizeof(Elf64_Ehdr))
	    != sizeof(Elf64_Ehdr)) {
		printk("load: read elf header failed\n");
		ret = -1;
		goto done;
	}

	if (memcmp(elf_header.e_ident, "\177ELF\2\1\1", 7)
	    || elf_header.e_type != ET_EXEC || elf_header.e_machine != EM_X86_64
	    || elf_header.e_version != 1
	    || elf_header.e_phentsize != sizeof(Elf64_Phdr)) {
		printk("load: seems not a elf file\n");
		ret = -1;
		goto done;
	}

	Elf64_Off prog_header_offset = elf_header.e_phoff;
	Elf64_Half prog_header_size = elf_header.e_phentsize;

	uint64_t prog_idx = 0;
	while (prog_idx < elf_header.e_phnum) {
		memset(&prog_header, 0, prog_header_size);

		sys_lseek(fd, prog_header_offset, SEEK_SET);

		if (sys_read(fd, &prog_header, prog_header_size)
		    != prog_header_size) {
			printk("load: read prog failed\n");
			ret = -1;
			goto done;
		}

		if (PT_LOAD == prog_header.p_type) {
			if (!segment_load(fd, prog_header.p_offset,
					  prog_header.p_filesz,
					  prog_header.p_vaddr)) {
				printk("load: segment_load failed\n");
				ret = -1;
				goto done;
			}
		}

		prog_header_offset += elf_header.e_phentsize;
		++prog_idx;
	}

	ret = elf_header.e_entry;

done:
	sys_close(fd);
	return ret;
}

int32_t sys_execv(const char *path, const char *argv[]) {
	uint64_t rbp;
	asm volatile("movq (%%rbp),%0" : "=a"(rbp));

	uint64_t argc = 0;
	while (argv[argc]) {
		++argc;
	}
	int64_t entry_point = load(path);
	if (entry_point == -1) {
		printk("load failed\n");
		return -1;
	}

	struct task_struct *cur = running_thread();
	char *name = strrchr(path, '/');
	strcpy(cur->name, name ? name + 1 : path);
	//	memcpy(cur->name, path, TASK_NAME_LEN);
	//	cur->name[TASK_NAME_LEN - 1] = 0;

	*(uint64_t *)(rbp - 16) = entry_point;

	asm volatile("movq %0,%%rdi;movq %1,%%rsi" ::"g"(argc), "g"(argv)
		     : "memory");
	asm volatile("movq %0,%%rbp;jmp system_ret" ::"g"(rbp) : "memory");

	return 0;
}
