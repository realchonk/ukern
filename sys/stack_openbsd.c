#include <sys/mman.h>
#include <unistd.h>

void *stack_alloc (size_t size)
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
void stack_free (void *stack, size_t size)
{
	munmap (stack, size);
}

