#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <mcc/elf.h>

#define MCC_ELF_BASE32 0x08048000U
#define MCC_ELF_BASE64 0x0000000000400000ULL
#define MCC_ELF_ALIGN  0x1000U

static const unsigned char exit_code_i386[] = {
    0xb8, 0x01, 0x00, 0x00, 0x00,
    0x31, 0xdb,
    0xcd, 0x80
};

static const unsigned char exit_code_x86_64[] = {
    0xb8, 0x3c, 0x00, 0x00, 0x00,
    0x31, 0xff,
    0x0f, 0x05
};

static void elf_ident(unsigned char *ident, unsigned char elf_class)
{
    memset(ident, 0, 16);
    ident[0] = 0x7f;
    ident[1] = 'E';
    ident[2] = 'L';
    ident[3] = 'F';
    ident[4] = elf_class;
    ident[5] = MCC_ELFDATA2LSB;
    ident[6] = MCC_ELF_VERSION;
}

static int write_i386(FILE *file)
{
    struct mcc_elf32_ehdr ehdr = {0};
    struct mcc_elf32_phdr phdr = {0};
    const uint32_t code_size = (uint32_t)sizeof(exit_code_i386);
    const uint32_t code_offset =
        (uint32_t)(sizeof(ehdr) + sizeof(phdr));
    const uint32_t file_size = code_offset + code_size;

    elf_ident(ehdr.e_ident, MCC_ELFCLASS32);
    ehdr.e_type = MCC_ET_EXEC;
    ehdr.e_machine = MCC_EM_386;
    ehdr.e_version = MCC_ELF_VERSION;
    ehdr.e_entry = MCC_ELF_BASE32 + code_offset;
    ehdr.e_phoff = sizeof(ehdr);
    ehdr.e_ehsize = sizeof(ehdr);
    ehdr.e_phentsize = sizeof(phdr);
    ehdr.e_phnum = 1;

    phdr.p_type = MCC_PT_LOAD;
    phdr.p_offset = 0;
    phdr.p_vaddr = MCC_ELF_BASE32;
    phdr.p_paddr = MCC_ELF_BASE32;
    phdr.p_filesz = file_size;
    phdr.p_memsz = file_size;
    phdr.p_flags = MCC_PF_R | MCC_PF_X;
    phdr.p_align = MCC_ELF_ALIGN;

    return fwrite(&ehdr, sizeof(ehdr), 1, file) == 1 &&
           fwrite(&phdr, sizeof(phdr), 1, file) == 1 &&
           fwrite(exit_code_i386, sizeof(exit_code_i386), 1, file) == 1
           ? 0 : -1;
}

static int write_x86_64(FILE *file)
{
    struct mcc_elf64_ehdr ehdr = {0};
    struct mcc_elf64_phdr phdr = {0};
    const uint64_t code_size = (uint64_t)sizeof(exit_code_x86_64);
    const uint64_t code_offset =
        (uint64_t)(sizeof(ehdr) + sizeof(phdr));
    const uint64_t file_size = code_offset + code_size;

    elf_ident(ehdr.e_ident, MCC_ELFCLASS64);
    ehdr.e_type = MCC_ET_EXEC;
    ehdr.e_machine = MCC_EM_X86_64;
    ehdr.e_version = MCC_ELF_VERSION;
    ehdr.e_entry = MCC_ELF_BASE64 + code_offset;
    ehdr.e_phoff = sizeof(ehdr);
    ehdr.e_ehsize = sizeof(ehdr);
    ehdr.e_phentsize = sizeof(phdr);
    ehdr.e_phnum = 1;

    phdr.p_type = MCC_PT_LOAD;
    phdr.p_flags = MCC_PF_R | MCC_PF_X;
    phdr.p_offset = 0;
    phdr.p_vaddr = MCC_ELF_BASE64;
    phdr.p_paddr = MCC_ELF_BASE64;
    phdr.p_filesz = file_size;
    phdr.p_memsz = file_size;
    phdr.p_align = MCC_ELF_ALIGN;

    return fwrite(&ehdr, sizeof(ehdr), 1, file) == 1 &&
           fwrite(&phdr, sizeof(phdr), 1, file) == 1 &&
           fwrite(exit_code_x86_64, sizeof(exit_code_x86_64), 1, file) == 1
           ? 0 : -1;
}

int mcc_elf_write_executable(const char *path, enum mcc_arch arch)
{
    FILE *file;
    int result;

    file = fopen(path, "wb");
    if (file == NULL) {
        return -1;
    }

    if (arch == MCC_ARCH_I386) {
        result = write_i386(file);
    } else if (arch == MCC_ARCH_X86_64) {
        result = write_x86_64(file);
    } else {
        result = -1;
    }

    if (fclose(file) != 0) {
        result = -1;
    }

    if (result == 0 && chmod(path, 0755) != 0) {
        result = -1;
    }

    return result;
}

_Static_assert(sizeof(struct mcc_elf32_ehdr) == 52,
               "ELF32 header must be 52 bytes");
_Static_assert(sizeof(struct mcc_elf32_phdr) == 32,
               "ELF32 program header must be 32 bytes");
_Static_assert(sizeof(struct mcc_elf64_ehdr) == 64,
               "ELF64 header must be 64 bytes");
_Static_assert(sizeof(struct mcc_elf64_phdr) == 56,
               "ELF64 program header must be 56 bytes");
