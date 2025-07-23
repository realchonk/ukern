#include <sys/mman.h>
#include <unistd.h>

void *stack_alloc (size_t size)
{
	int flags = MAP_ANON | MAP_PRIVATE;
#ifdef __OpenBSD__
	flags |= MAP_STACK;
#endif
	void *ptr = mmap (
		NULL,
		size,
		PROT_READ | PROT_WRITE,
		flags,
		-1,
		0
	);

	return ptr != MAP_FAILED ? ptr : NULL;
}
void stack_free (void *stack, size_t size)
{
	munmap (stack, size);
}

