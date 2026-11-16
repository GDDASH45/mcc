#include <stdlib.h>

static int is_space(char c)
{
    return c == ' ' || (c >= '\t' && c <= '\r');
}

static int digit_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    }

    if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    }

    return -1;
}

int abs(int value)
{
    return value < 0 ? -value : value;
}

long labs(long value)
{
    return value < 0 ? -value : value;
}

long strtol(const char *str, char **endptr, int base)
{
    const char *p = str;
    unsigned long value = 0;
    int negative = 0;
    int digit;
    int any = 0;

    while (is_space(*p)) {
        ++p;
    }

    if (*p == '+' || *p == '-') {
        negative = (*p == '-');
        ++p;
    }

    if (base == 0) {
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
            base = 16;
            p += 2;
        } else if (p[0] == '0') {
            base = 8;
        } else {
            base = 10;
        }
    } else if (base == 16 && p[0] == '0' &&
               (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    }

    if (base < 2 || base > 36) {
        if (endptr != NULL) {
            *endptr = (char *)str;
        }
        return 0;
    }

    while ((digit = digit_value(*p)) >= 0 && digit < base) {
        value = value * (unsigned int)base + (unsigned int)digit;
        ++p;
        any = 1;
    }

    if (endptr != NULL) {
        *endptr = (char *)(any ? p : str);
    }

    if (!any) {
        return 0;
    }

    return negative ? -(long)value : (long)value;
}

int atoi(const char *str)
{
    return (int)strtol(str, NULL, 10);
}

long atol(const char *str)
{
    return strtol(str, NULL, 10);
}
