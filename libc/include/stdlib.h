#ifndef MCC_STDLIB_H
#define MCC_STDLIB_H

#include <stddef.h>

int abs(int value);
long labs(long value);
int atoi(const char *str);
long atol(const char *str);
long strtol(const char *str, char **endptr, int base);

#endif
