#include <sys/mman.h>
#include <unistd.h>

#ifdef __OpenBSD__
void *alloc_stack (size_t size)
{
	void *ptr = mmap (
		NULL,
		size,
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE | MAP_STACK,
		-1,
		0
	);

	return ptr != MAP_FAILED ? ptr : NULL;
}
void free_stack (void *stack, size_t size)
{
	munmap (stack, size);
}
#else
# error Unsupported Operating System
#endif

