# ELF binary on-demand loader in Linux
---
#### Assignment Link: https://ocw.cs.pub.ro/courses/so/teme/tema-3 (in romanian)
---

## Introduction
The implementation of the loader is in `loader.c`. The loader will provide two methods, defined in the `loader.h` header:

- so_init_loader() will be called before any other function from the loader. It should initialize the loader and return 0 on success, or -1 on error.

- so_execute() will be called after so_init_loader(). It should load the binary located in path and execute it with the required arguments. It should return the exit code of the binary on success and -1 on error.

The initialization of the loader is based on inregistration of the signal handler for SIGSEGV. The handler will be called when the loader will try to access a page that is not mapped. The handler will map the page and return to the instruction that caused the SIGSEGV.


## The implementation of the `signal handler`
```
The segv_handler() function is the signal handler for SIGSEGV. It will be called when the loader will try to access a page that is not mapped. These are the steps that the handler will do:
```
- check if the address that caused the SIGSEGV is in one of the segments of the binary that is being executed. If it is not, then the handler will return segmentation fault. The check is done by iterating through the segments of the binary and checking if the address is in the range of the segment. If the address is in the range of the segment, the function will return the segment, otherwise it will return NULL.

- check if the segment was already mapped. If it was, then the handler will return segmentation fault. In the segment->data field, the loader stores a vector of zeros and ones, having the same size as the segment. If the value at the index of the page that caused the SIGSEGV is 1, then the page was already mapped. If the value is 0, then the page was not mapped. The check is done by dividing the offset of the address that caused the SIGSEGV by the page size and checking the value at the index of the vector.

- if the segment was not mapped, then the handler will map the segment using mmap() and set the value at the index of the vector to 1. It is very important to set the value to 1 before the mapping, because the handler can be called again while the mapping is in progress. If the value is set to 1, then the handler will not try to map the segment again. Moreover, when we map, we set the permission to write, because we will need to write the values of the segment in the mapped page.

- when the mapping is done, we copy the values of the segment in the mapped page. We do this by reading the values of the segment from the file descriptor of the binary and writing them in the mapped page. The offset of the segment is the offset of the page that caused the SIGSEGV, so we need to add the offset of the segment to the offset of the page. The size of the segment is the size of the page, so we need to read and write the size of the page. We need to be careful if we have the segment bewteen file size and memory size, because we need to read only the values that are in the file and write 0 in the rest of the page.

- finally, we set the permission of the page to permission of the segment. We do this by using mprotect().

The segmentation fault is returned by appelating the old signal (old_sa).

### Documentation
https://man7.org/linux/man-pages/man2/sigaction.2.html
https://pubs.opengroup.org/onlinepubs/007904875/functions/sigaction.html
https://man7.org/linux/man-pages/man2/mmap.2.html