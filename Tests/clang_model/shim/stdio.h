#ifndef SHIM_STDIO_H
#define SHIM_STDIO_H
#include <stddef.h>
#include <stdarg.h>
int sprintf(char *, const char *, ...); int snprintf(char *, size_t, const char *, ...);
int vsprintf(char *, const char *, va_list); int vsnprintf(char *, size_t, const char *, va_list);
int sscanf(const char *, const char *, ...); int printf(const char *, ...);
#endif
