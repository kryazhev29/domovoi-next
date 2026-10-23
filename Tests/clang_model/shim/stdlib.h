#ifndef SHIM_STDLIB_H
#define SHIM_STDLIB_H
#include <stddef.h>
int abs(int); long labs(long); int atoi(const char *); long atol(const char *);
long strtol(const char *, char **, int); unsigned long strtoul(const char *, char **, int);
void *malloc(size_t); void free(void *); int rand(void); void srand(unsigned);
#endif
