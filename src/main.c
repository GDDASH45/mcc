#include <elf.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int link_file(const char *input, const char *output, int is64)
{
    FILE *in = fopen(input, "rb");
    if (!in) {
        perror(input);
        return 1;
    }

    if (fseek(in, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(in);
        return 1;
    }

    long size = ftell(in);
    if (size < 0 || fseek(in, 0, SEEK_SET) != 0) {
        perror("input size");
        fclose(in);
        return 1;
    }

    unsigned char *data = malloc((size_t)size);
    if (!data) {
        perror("malloc");
        fclose(in);
        return 1;
    }

    if (fread(data, 1, (size_t)size, in) != (size_t)size) {
        fprintf(stderr, "mcc-ld: failed to read input\n");
        free(data);
        fclose(in);
        return 1;
    }
    fclose(in);

    if (size < EI_NIDENT || memcmp(data, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "mcc-ld: invalid ELF object\n");
        free(data);
        return 1;
    }

    if (data[EI_CLASS] != (is64 ? ELFCLASS64 : ELFCLASS32)) {
        fprintf(stderr, "mcc-ld: input architecture does not match target\n");
        free(data);
        return 1;
    }

    uint64_t text_offset;
    uint64_t text_size;

    if (is64) {
        if ((size_t)size < sizeof(Elf64_Ehdr)) {
            free(data);
            return 1;
        }

        Elf64_Ehdr *eh = (Elf64_Ehdr *)data;
        if (eh->e_shentsize != sizeof(Elf64_Shdr) ||
            eh->e_shoff > (uint64_t)size ||
            eh->e_shnum > ((uint64_t)size - eh->e_shoff) / sizeof(Elf64_Shdr)) {
            fprintf(stderr, "mcc-ld: invalid section table\n");
            free(data);
            return 1;
        }

        Elf64_Shdr *sh = (Elf64_Shdr *)(data + eh->e_shoff);
        if (eh->e_shstrndx >= eh->e_shnum) {
            fprintf(stderr, "mcc-ld: invalid section string table\n");
            free(data);
            return 1;
        }

        Elf64_Shdr *strings = &sh[eh->e_shstrndx];
        if (strings->sh_offset > (uint64_t)size ||
            strings->sh_size > (uint64_t)size - strings->sh_offset) {
            fprintf(stderr, "mcc-ld: invalid section string table bounds\n");
            free(data);
            return 1;
        }

        const char *names = (const char *)(data + strings->sh_offset);
        int found = 0;

        for (unsigned int i = 0; i < eh->e_shnum; i++) {
            if (sh[i].sh_name >= strings->sh_size)
                continue;

            if (strcmp(names + sh[i].sh_name, ".text") == 0) {
                text_offset = sh[i].sh_offset;
                text_size = sh[i].sh_size;
                found = 1;
                break;
            }
        }

        if (!found || text_offset > (uint64_t)size ||
            text_size > (uint64_t)size - text_offset) {
            fprintf(stderr, "mcc-ld: missing or invalid .text section\n");
            free(data);
            return 1;
        }
    } else {
        if ((size_t)size < sizeof(Elf32_Ehdr)) {
            free(data);
            return 1;
        }

        Elf32_Ehdr *eh = (Elf32_Ehdr *)data;
        if (eh->e_shentsize != sizeof(Elf32_Shdr) ||
            eh->e_shoff > (uint32_t)size ||
            eh->e_shnum > ((uint32_t)size - eh->e_shoff) / sizeof(Elf32_Shdr)) {
            fprintf(stderr, "mcc-ld: invalid section table\n");
            free(data);
            return 1;
        }

        Elf32_Shdr *sh = (Elf32_Shdr *)(data + eh->e_shoff);
        if (eh->e_shstrndx >= eh->e_shnum) {
            fprintf(stderr, "mcc-ld: invalid section string table\n");
            free(data);
            return 1;
        }

        Elf32_Shdr *strings = &sh[eh->e_shstrndx];
        if (strings->sh_offset > (uint32_t)size ||
            strings->sh_size > (uint32_t)size - strings->sh_offset) {
            fprintf(stderr, "mcc-ld: invalid section string table bounds\n");
            free(data);
            return 1;
        }

        const char *names = (const char *)(data + strings->sh_offset);
        int found = 0;

        for (unsigned int i = 0; i < eh->e_shnum; i++) {
            if (sh[i].sh_name >= strings->sh_size)
                continue;

            if (strcmp(names + sh[i].sh_name, ".text") == 0) {
                text_offset = sh[i].sh_offset;
                text_size = sh[i].sh_size;
                found = 1;
                break;
            }
        }

        if (!found || text_offset > (uint64_t)size ||
            text_size > (uint64_t)size - text_offset) {
            fprintf(stderr, "mcc-ld: missing or invalid .text section\n");
            free(data);
            return 1;
        }
    }

    FILE *out = fopen(output, "wb");
    if (!out) {
        perror(output);
        free(data);
        return 1;
    }

    const uint64_t base = is64 ? 0x400000 : 0x08048000;
    const uint64_t code_offset = 4096;

    if (is64) {
        Elf64_Ehdr eh = {0};
        Elf64_Phdr ph = {0};

        memcpy(eh.e_ident, ELFMAG, SELFMAG);
        eh.e_ident[EI_CLASS] = ELFCLASS64;
        eh.e_ident[EI_DATA] = ELFDATA2LSB;
        eh.e_ident[EI_VERSION] = EV_CURRENT;
        eh.e_ident[EI_OSABI] = ELFOSABI_SYSV;
        eh.e_type = ET_EXEC;
        eh.e_machine = EM_X86_64;
        eh.e_version = EV_CURRENT;
        eh.e_entry = base + code_offset;
        eh.e_phoff = sizeof(Elf64_Ehdr);
        eh.e_ehsize = sizeof(Elf64_Ehdr);
        eh.e_phentsize = sizeof(Elf64_Phdr);
        eh.e_phnum = 1;

        ph.p_type = PT_LOAD;
        ph.p_flags = PF_R | PF_X;
        ph.p_offset = 0;
        ph.p_vaddr = base;
        ph.p_paddr = base;
        ph.p_filesz = code_offset + text_size;
        ph.p_memsz = ph.p_filesz;
        ph.p_align = 4096;

        fwrite(&eh, sizeof(eh), 1, out);
        fwrite(&ph, sizeof(ph), 1, out);
    } else {
        Elf32_Ehdr eh = {0};
        Elf32_Phdr ph = {0};

        memcpy(eh.e_ident, ELFMAG, SELFMAG);
        eh.e_ident[EI_CLASS] = ELFCLASS32;
        eh.e_ident[EI_DATA] = ELFDATA2LSB;
        eh.e_ident[EI_VERSION] = EV_CURRENT;
        eh.e_ident[EI_OSABI] = ELFOSABI_SYSV;
        eh.e_type = ET_EXEC;
        eh.e_machine = EM_386;
        eh.e_version = EV_CURRENT;
        eh.e_entry = (Elf32_Addr)(base + code_offset);
        eh.e_phoff = sizeof(Elf32_Ehdr);
        eh.e_ehsize = sizeof(Elf32_Ehdr);
        eh.e_phentsize = sizeof(Elf32_Phdr);
        eh.e_phnum = 1;

        ph.p_type = PT_LOAD;
        ph.p_flags = PF_R | PF_X;
        ph.p_offset = 0;
        ph.p_vaddr = (Elf32_Addr)base;
        ph.p_paddr = (Elf32_Addr)base;
        ph.p_filesz = (Elf32_Word)(code_offset + text_size);
        ph.p_memsz = ph.p_filesz;
        ph.p_align = 4096;

        fwrite(&eh, sizeof(eh), 1, out);
        fwrite(&ph, sizeof(ph), 1, out);
    }

    if (fseek(out, (long)code_offset, SEEK_SET) != 0 ||
        fwrite(data + text_offset, 1, (size_t)text_size, out) != (size_t)text_size) {
        fprintf(stderr, "mcc-ld: failed to write executable\n");
        fclose(out);
        free(data);
        return 1;
    }

    fclose(out);
    free(data);
    return 0;
}

int main(int argc, char **argv)
{
    const char *input = NULL;
    const char *output = NULL;
    const char *target = "x86_64";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) {
            target = argv[++i];
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("usage: mcc-ld --target <i386|x86_64> -o output.elf input.o\n");
            return 0;
        } else {
            input = argv[i];
        }
    }

    if (!input || !output ||
        (strcmp(target, "i386") != 0 && strcmp(target, "x86_64") != 0)) {
        fprintf(stderr, "usage: mcc-ld --target <i386|x86_64> -o output.elf input.o\n");
        return 2;
    }

    return link_file(input, output, strcmp(target, "x86_64") == 0);
}
