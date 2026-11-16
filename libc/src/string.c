#include <stddef.h>
#include <string.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;

    for (size_t i = 0; i < n; ++i) {
        d[i] = s[i];
    }

    return dest;
}

void *memmove(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;

    if (d < s) {
        for (size_t i = 0; i < n; ++i) {
            d[i] = s[i];
        }
    } else if (d > s) {
        while (n != 0) {
            --n;
            d[n] = s[n];
        }
    }

    return dest;
}

void *memset(void *dest, int value, size_t n)
{
    unsigned char *d = dest;

    for (size_t i = 0; i < n; ++i) {
        d[i] = (unsigned char)value;
    }

    return dest;
}

int memcmp(const void *lhs, const void *rhs, size_t n)
{
    const unsigned char *a = lhs;
    const unsigned char *b = rhs;

    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            return (int)a[i] - (int)b[i];
        }
    }

    return 0;
}

size_t strlen(const char *str)
{
    size_t len = 0;

    while (str[len] != '\0') {
        ++len;
    }

    return len;
}

int strcmp(const char *lhs, const char *rhs)
{
    while (*lhs != '\0' && *lhs == *rhs) {
        ++lhs;
        ++rhs;
    }

    return (int)(unsigned char)*lhs - (int)(unsigned char)*rhs;
}

int strncmp(const char *lhs, const char *rhs, size_t n)
{
    while (n != 0 && *lhs != '\0' && *lhs == *rhs) {
        ++lhs;
        ++rhs;
        --n;
    }

    if (n == 0) {
        return 0;
    }

    return (int)(unsigned char)*lhs - (int)(unsigned char)*rhs;
}

char *strcpy(char *restrict dest, const char *restrict src)
{
    char *result = dest;

    while ((*dest++ = *src++) != '\0') {
    }

    return result;
}

char *strncpy(char *restrict dest, const char *restrict src, size_t n)
{
    size_t i = 0;

    while (i < n && src[i] != '\0') {
        dest[i] = src[i];
        ++i;
    }

    while (i < n) {
        dest[i++] = '\0';
    }

    return dest;
}
