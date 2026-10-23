/* Заглушки IAR для проверки clang thumbv7m: только синтаксис и размер, не запуск. */
#ifndef IAR_SHIM_H
#define IAR_SHIM_H
#undef __arm
#define __arm static
#define __no_operation() __asm volatile("nop")
#define __disable_interrupt() __asm volatile("cpsid i")
#define __enable_interrupt() __asm volatile("cpsie i")
#define __root __attribute__((used))
#define __weak __attribute__((weak))
#define __irq
#define __no_init
#define __ramfunc
#define __packed __attribute__((packed))
#endif
#define asm __asm
#define __get_CPSR() 0u
#define __set_CPSR(x) ((void)(x))
/* Стек и секции IAR для main.c; __ramfunc задаёт app_update.c сам. */
#define __stackless __attribute__((noinline))
extern char domovoi_cstack_base[] __asm__("CSTACK$$Base");
extern char domovoi_cstack_limit[] __asm__("CSTACK$$Limit");
#define __section_begin(name) ((void *)domovoi_cstack_base)
#define __section_end(name) ((void *)domovoi_cstack_limit)
#undef __ramfunc
