#include <string.h>
#include <stdint.h>
#include "task.h"

/* CPU context */
struct cpu_context {
	/* 0x00 */ uint64_t rbx;
	/* 0x08 */ uint64_t rbp;
	/* 0x10 */ uint64_t r12;
	/* 0x18 */ uint64_t r13;
	/* 0x20 */ uint64_t r14;
	/* 0x28 */ uint64_t r15;
	/* 0x30 */ uint64_t rip;
};

/* context of a new task */
struct cpu_new_context {
	/* 0x00 */ struct cpu_context ctx;
	/* 0x38 */ uint64_t arg;
	/* 0x40 */ uint64_t entry;

	/* stack must be 16-byte aligned */
	/* 0x48 */ uint64_t pad;
};

__attribute__((naked))
static void
ctx_init (void)
{
	__asm __volatile__ (
		// clear out caller-saved registers
		"xor %rax, %rax\n"
		"xor %rcx, %rcx\n"
		"xor %rdx, %rdx\n"
		"xor %rsi, %rsi\n"
		"xor %r8, %r8\n"
		"xor %r9, %r9\n"
		"xor %r10, %r10\n"
		"xor %r11, %r11\n"

		// get new_context::arg
		"pop %rdi\n"

		// jump to new_context::entry
		"ret\n"
	);
}

__attribute__((naked))
void
ctx_enter (struct cpu_context *ctx)
{
	__asm __volatile__ (
		/* load new context */
		"mov %rdi, %rsp\n"

		/* restore callee-saved registers */
		"pop %rbx\n"
		"pop %rbp\n"
		"pop %r12\n"
		"pop %r13\n"
		"pop %r14\n"
		"pop %r15\n"
		"call sys_unblock\n"
		"ret"
	);
}

__attribute__((naked))
void
ctx_switch (struct cpu_context **old, struct cpu_context *ctx)
{
	__asm __volatile__ (
		// save callee-saved registers
		"push %r15\n"
		"push %r14\n"
		"push %r13\n"
		"push %r12\n"
		"push %rbp\n"
		"push %rbx\n"

		// store old context
		"movq %rsp, (%rdi)\n"

		// load new context
		"mov %rsi, %rsp\n"

		// restore callee-saved registers
		"pop %rbx\n"
		"pop %rbp\n"
		"pop %r12\n"
		"pop %r13\n"
		"pop %r14\n"
		"pop %r15\n"
		"call sys_unblock\n"
		"ret"
	);
}

struct cpu_context *
ctx_new (
	void *stack,
	size_t stack_size,
	void(*entry)(void *),
	void *arg
) {
	struct cpu_new_context *ctx;

	ctx = (void *)((uint64_t)stack + stack_size - sizeof (struct cpu_new_context));

	memset (ctx, 0, sizeof (*ctx));
	ctx->ctx.rip = (uint64_t)ctx_init;
	ctx->entry = (uint64_t)entry;
	ctx->arg = (uint64_t)arg;
	return &ctx->ctx;
}

