#include <stdio.h>
#include <string.h>

#include <mcc/elf.h>
#include <mcc/target.h>

static void usage(const char *program)
{
    printf("MCC - Mini Compiler Collection\n");
    printf("Usage: %s --target <i386|x86_64> -o <output>\n", program);
    printf("       %s --help\n", program);
    printf("       %s --version\n", program);
}

int main(int argc, char **argv)
{
    const char *output = NULL;
    enum mcc_arch arch = MCC_ARCH_X86_64;
    int target_seen = 0;

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        usage(argv[0]);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        puts("MCC 0.1.0");
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--target") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "mcc: missing argument for --target\n");
                return 2;
            }

            target_seen = 1;

            if (strcmp(argv[i], "i386") == 0) {
                arch = MCC_ARCH_I386;
            } else if (strcmp(argv[i], "x86_64") == 0) {
                arch = MCC_ARCH_X86_64;
            } else {
                fprintf(stderr, "mcc: unsupported target '%s'\n", argv[i]);
                return 2;
            }
        } else if (strncmp(argv[i], "--target=", 9) == 0) {
            const char *target = argv[i] + 9;

            target_seen = 1;

            if (strcmp(target, "i386") == 0) {
                arch = MCC_ARCH_I386;
            } else if (strcmp(target, "x86_64") == 0) {
                arch = MCC_ARCH_X86_64;
            } else {
                fprintf(stderr, "mcc: unsupported target '%s'\n", target);
                return 2;
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            if (++i >= argc) {
                fprintf(stderr, "mcc: missing argument for -o\n");
                return 2;
            }

            output = argv[i];
        } else {
            fprintf(stderr, "mcc: unknown argument '%s'\n", argv[i]);
            usage(argv[0]);
            return 2;
        }
    }

    if (output == NULL) {
        fprintf(stderr, "mcc: no output file specified\n");
        usage(argv[0]);
        return 2;
    }

    if (!target_seen) {
        fprintf(stderr, "mcc: no target specified; defaulting to x86_64\n");
    }

    if (mcc_elf_write_executable(output, arch) != 0) {
        fprintf(stderr, "mcc: failed to write '%s'\n", output);
        return 1;
    }

    printf("MCC: wrote %s ELF executable to %s\n",
           arch == MCC_ARCH_I386 ? "i386" : "x86_64", output);
    return 0;
}
