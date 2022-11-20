/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "exec_parser.h"

static so_exec_t *exec;

static int fd;
static struct sigaction old_sa;

/**
 * @brief Find the page fault address in the exec structure
 *
 * @param exec Executable structure
 * @param addr Address of the page fault
 * @return so_seg_t* Segment that contains the page fault address or NULL if
 * the address is not in any segment
 */
so_seg_t* get_segment(so_exec_t *exec, void *addr)
{
	for (int i = 0; i < exec->segments_no; i++) {
		// Cast the address to uintptr_t
		uintptr_t addr_cast = (uintptr_t)addr;

		// Check if the address is inside the segment
		if (addr_cast >= exec->segments[i].vaddr &&
			addr_cast < exec->segments[i].vaddr + exec->segments[i].mem_size)
			return &exec->segments[i];
	}

	// The segment is not found
	return NULL;
}

/**
 * @brief Find the page fault address in the page table of the segment
 *
 * @param segment The segment that contains the page fault address
 * @param addr Address of the page fault
 * @return int 1 if the page is found, 0 otherwise
 */
int get_page(so_seg_t *segment, void *addr)
{
	// Alloc the page table if it is not allocated yet (first page fault)
	if (segment->data == NULL) {
		segment->data = calloc(segment->mem_size / getpagesize() +
							   (segment->mem_size % getpagesize() != 0 ? 1 : 0),
							   sizeof(int));
	}
	// Get the offset of the page
	size_t segment_offset = (uintptr_t)(addr) - segment->vaddr -
							(((uintptr_t)(addr) - segment->vaddr) %
							getpagesize());

	// Check if the page is already mapped
	if (((int *) segment->data)[segment_offset / getpagesize()] == 1)
		return 1;

	// The page is not mapped
	return 0;
}

/**
 * @brief Map the page fault address
 *
 * @param segment The segment that contains the page fault address
 * @param fault_addr Address of the page fault
 * @return void* Address of the mapped page
 */
void *map(so_seg_t *segment, void *fault_addr)
{
	// Get the offset of the page
	size_t segment_offset = (uintptr_t)(fault_addr) - segment->vaddr -
							(((uintptr_t)(fault_addr) - segment->vaddr) %
							getpagesize());
	// Change the page table
	((int *) segment->data)[segment_offset / getpagesize()] = 1;
	
	// Map the page
	void *page = mmap((void *)segment->vaddr + segment_offset, getpagesize(),
					  PERM_W, MAP_FIXED | MAP_SHARED | MAP_ANONYMOUS,
					  -1, 0);

	return page;
}

/**
 * @brief Copy the data from the file to the mapped page
 *
 * @param segment The segment that contains the page fault address
 * @param fault_addr Address of the page fault
 * @param page Address of the mapped page
 */
void copy_segment_data(so_seg_t *segment, void *fault_addr, void *page)
{
	// Get the offset of the page
	size_t segment_offset = (uintptr_t)(fault_addr) - segment->vaddr -
							(((uintptr_t)(fault_addr) - segment->vaddr) %
							getpagesize());

	// Move the file pointer to the offset
	lseek(fd, segment->offset + segment_offset, SEEK_SET);
	// Read the data from the file according to the file size
	if (segment_offset + getpagesize() <= segment->file_size) {
		// Case 1: The page is completely inside the file
		read(fd, page, getpagesize());
	} else if (segment_offset < segment->file_size) {
		// Case 2: The page is partially inside the file
		read(fd, page, segment->file_size - segment_offset);
		memset(page + segment->file_size - segment_offset, 0,
				getpagesize() - segment->file_size + segment_offset);
	} else {
		// Case 3: The page is completely outside the file
		memset(page, 0, getpagesize());
	}
}

/**
 * @brief Page fault handler
 *
 * @param signum Signal number
 * @param info Signal information
 * @param context Signal context
 */
static void segv_handler(int signum, siginfo_t *info, void *context)
{
	// Check if the fault is inside a segment
	so_seg_t *segment = get_segment(exec, info->si_addr);

	if (segment == NULL)
		old_sa.sa_sigaction(signum, info, context);

	// Check if the segment is mapped
	if (get_page(segment, info->si_addr) == 1)
		old_sa.sa_sigaction(signum, info, context);

	// Map the segment
	void *page = map(segment, info->si_addr);

	// Copy the data from the file to the mapped page
	copy_segment_data(segment, info->si_addr, page);

	// Set the permissions
	mprotect(page, getpagesize(), segment->perm);
}

int so_init_loader(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_SIGINFO; // use sa_sigaction for handler
	sa.sa_sigaction = segv_handler;

	if (sigaction(SIGSEGV, &sa, &old_sa) == -1)
		return -1;

	return 0;
}

int so_execute(char *path, char *argv[])
{
	fd = open(path, O_RDONLY); // find the file descriptor
	if (fd == -1)
		return -1;

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
