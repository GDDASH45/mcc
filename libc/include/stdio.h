#ifndef MCC_STDIO_H
#define MCC_STDIO_H

#include <stdarg.h>

int printf(const char *restrict format, ...);
int vprintf(const char *restrict format, va_list args);

#endif
