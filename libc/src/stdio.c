#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define STDIO_BUFFER_SIZE 256

struct output {
    char buffer[STDIO_BUFFER_SIZE];
    size_t used;
    size_t count;
};

enum length_modifier {
    LENGTH_DEFAULT,
    LENGTH_LONG,
    LENGTH_LLONG,
    LENGTH_SIZE
};

static long syscall_write(const char *buffer, size_t count)
{
#if defined(__x86_64__)
    long result;

    __asm__ volatile (
        "syscall"
        : "=a"(result)
        : "0"(1L), "D"(1L), "S"(buffer), "d"(count)
        : "rcx", "r11", "memory"
    );

    return result;
#elif defined(__i386__)
    long result;

    __asm__ volatile (
        "int $0x80"
        : "=a"(result)
        : "0"(4L), "b"(1L), "c"(buffer), "d"(count)
        : "memory"
    );

    return result;
#else
#error "mcclib stdio supports only i386 and x86_64"
#endif
}

static int flush_output(struct output *out)
{
    size_t written = 0;

    while (written < out->used) {
        long result = syscall_write(
            out->buffer + written,
            out->used - written
        );

        if (result <= 0) {
            return -1;
        }

        written += (size_t)result;
    }

    out->used = 0;
    return 0;
}

static int output_char(struct output *out, char c)
{
    if (out->used == STDIO_BUFFER_SIZE && flush_output(out) < 0) {
        return -1;
    }

    out->buffer[out->used++] = c;
    ++out->count;

    return 0;
}

static int output_string(struct output *out, const char *str)
{
    if (str == NULL) {
        str = "(null)";
    }

    while (*str != '\0') {
        if (output_char(out, *str++) < 0) {
            return -1;
        }
    }

    return 0;
}

static int output_unsigned(
    struct output *out,
    unsigned long long value,
    unsigned int base,
    int uppercase
)
{
    char digits[65];
    size_t count = 0;

    const char *alphabet = uppercase
        ? "0123456789ABCDEF"
        : "0123456789abcdef";

    do {
        digits[count++] = alphabet[value % base];
        value /= base;
    } while (value != 0);

    while (count != 0) {
        if (output_char(out, digits[--count]) < 0) {
            return -1;
        }
    }

    return 0;
}

static int output_signed(struct output *out, long long value)
{
    unsigned long long magnitude;

    if (value < 0) {
        if (output_char(out, '-') < 0) {
            return -1;
        }

        magnitude = 0ULL - (unsigned long long)value;
    } else {
        magnitude = (unsigned long long)value;
    }

    return output_unsigned(out, magnitude, 10, 0);
}

static long long get_signed(va_list *args, enum length_modifier length)
{
    switch (length) {
    case LENGTH_LONG:
        return va_arg(*args, long);

    case LENGTH_LLONG:
        return va_arg(*args, long long);

    case LENGTH_SIZE:
        return va_arg(*args, ptrdiff_t);

    default:
        return va_arg(*args, int);
    }
}

static unsigned long long get_unsigned(
    va_list *args,
    enum length_modifier length
)
{
    switch (length) {
    case LENGTH_LONG:
        return va_arg(*args, unsigned long);

    case LENGTH_LLONG:
        return va_arg(*args, unsigned long long);

    case LENGTH_SIZE:
        return va_arg(*args, size_t);

    default:
        return va_arg(*args, unsigned int);
    }
}

int vprintf(const char *restrict format, va_list args)
{
    struct output out = {
        .used = 0,
        .count = 0
    };

    va_list ap;
    va_copy(ap, args);

    while (*format != '\0') {
        if (*format != '%') {
            if (output_char(&out, *format++) < 0) {
                va_end(ap);
                return -1;
            }

            continue;
        }

        ++format;

        if (*format == '\0') {
            break;
        }

        if (*format == '%') {
            if (output_char(&out, '%') < 0) {
                va_end(ap);
                return -1;
            }

            ++format;
            continue;
        }

        enum length_modifier length = LENGTH_DEFAULT;

        if (*format == 'l') {
            ++format;

            if (*format == 'l') {
                ++format;
                length = LENGTH_LLONG;
            } else {
                length = LENGTH_LONG;
            }
        } else if (*format == 'z') {
            ++format;
            length = LENGTH_SIZE;
        }

        int result = 0;

        switch (*format) {
        case 'c':
            result = output_char(&out, (char)va_arg(ap, int));
            break;

        case 's':
            result = output_string(&out, va_arg(ap, const char *));
            break;

        case 'd':
        case 'i':
            result = output_signed(&out, get_signed(&ap, length));
            break;

        case 'u':
            result = output_unsigned(
                &out, get_unsigned(&ap, length), 10, 0
            );
            break;

        case 'x':
            result = output_unsigned(
                &out, get_unsigned(&ap, length), 16, 0
            );
            break;

        case 'X':
            result = output_unsigned(
                &out, get_unsigned(&ap, length), 16, 1
            );
            break;

        case 'o':
            result = output_unsigned(
                &out, get_unsigned(&ap, length), 8, 0
            );
            break;

        case 'p':
            result = output_string(&out, "0x");

            if (result == 0) {
                result = output_unsigned(
                    &out,
                    (uintptr_t)va_arg(ap, void *),
                    16,
                    0
                );
            }
            break;

        default:
            result = output_char(&out, '%');

            if (result == 0) {
                result = output_char(&out, *format);
            }
            break;
        }

        if (result < 0) {
            va_end(ap);
            return -1;
        }

        ++format;
    }

    va_end(ap);

    if (flush_output(&out) < 0) {
        return -1;
    }

    return (int)out.count;
}

int printf(const char *restrict format, ...)
{
    va_list args;

    va_start(args, format);
    int result = vprintf(format, args);
    va_end(args);

    return result;
}
