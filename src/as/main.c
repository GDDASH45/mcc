#define _POSIX_C_SOURCE 200809L
#include <elf.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct buffer { unsigned char *data; size_t size, capacity; };

static int append(struct buffer *b, const void *data, size_t n)
{
    if (b->size + n > b->capacity) {
        size_t cap = b->capacity ? b->capacity : 128;
        while (cap < b->size + n) cap *= 2;
        void *p = realloc(b->data, cap);
        if (!p) return -1;
        b->data = p; b->capacity = cap;
    }
    memcpy(b->data + b->size, data, n);
    b->size += n;
    return 0;
}

static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t') ++s;
    size_t n = strlen(s);
    while (n && (s[n-1] == ' ' || s[n-1] == '\t' || s[n-1] == '\r' || s[n-1] == '\n')) s[--n] = 0;
    return s;
}

static int emit_instruction(struct buffer *b, char *line, int is64)
{
    static const unsigned char nop[] = {0x90}, ret[] = {0xc3}, syscall[] = {0x0f,0x05};
    static const unsigned char int80[] = {0xcd,0x80};
    static const unsigned char xor_eax[] = {0x31,0xc0};
    static const unsigned char xor_ebx[] = {0x31,0xdb};
    static const unsigned char xor_edi[] = {0x31,0xff};
    if (!strcmp(line,"nop")) return append(b,nop,sizeof(nop));
    if (!strcmp(line,"ret")) return append(b,ret,sizeof(ret));
    if (!strcmp(line,"syscall") && is64) return append(b,syscall,sizeof(syscall));
    if (!strcmp(line,"int 0x80") || !strcmp(line,"int $0x80")) return append(b,int80,sizeof(int80));
    if (!strcmp(line,"xor eax, eax")) return append(b,xor_eax,sizeof(xor_eax));
    if (!strcmp(line,"xor ebx, ebx")) return append(b,xor_ebx,sizeof(xor_ebx));
    if (!strcmp(line,"xor edi, edi") && is64) return append(b,xor_edi,sizeof(xor_edi));
    const char *regs[] = {"mov eax,","mov ebx,","mov edi,","mov rax,"};
    const unsigned char opcodes[] = {0xb8,0xbb,0xbf,0xb8};
    for (size_t r=0;r<4;r++) {
        size_t n=strlen(regs[r]);
        if ((r==2 || r==3) && !is64) continue;
        if (!strncmp(line,regs[r],n)) {
            char *end;
            unsigned long value=strtoul(trim(line+n),&end,0);
            if (*trim(end) || end==line+n || value>0xffffffffUL) return -1;
            unsigned char code[5]={opcodes[r],(unsigned char)value,(unsigned char)(value>>8),(unsigned char)(value>>16),(unsigned char)(value>>24)};
            return append(b,code,sizeof(code));
        }
    }
    if (!strncmp(line,".byte",5) && (line[5]==' ' || line[5]=='\t')) {
        char *p=line+5;
        while (*p) {
            p=trim(p);
            if (!*p) break;
            char *end;
            unsigned long v=strtoul(p,&end,0);
            if (end==p || v>255) return -1;
            unsigned char byte=(unsigned char)v;
            if (append(b,&byte,1)) return -1;
            p=trim(end);
            if (*p==',') ++p;
            else if (*p) return -1;
        }
        return 0;
    }
    return -1;
}

static int write_object(const char *path, struct buffer *text, int is64)
{
    static const char names[]="\0.text\0.shstrtab\0";
    FILE *f=fopen(path,"wb"); if(!f) return -1;
    int ok=0;
    if(is64) {
        Elf64_Ehdr h={0}; Elf64_Shdr s[3]={{0}};
        h.e_ident[EI_MAG0]=ELFMAG0; h.e_ident[EI_MAG1]=ELFMAG1; h.e_ident[EI_MAG2]=ELFMAG2; h.e_ident[EI_MAG3]=ELFMAG3;
        h.e_ident[EI_CLASS]=ELFCLASS64; h.e_ident[EI_DATA]=ELFDATA2LSB; h.e_ident[EI_VERSION]=EV_CURRENT;
        h.e_type=ET_REL; h.e_machine=EM_X86_64; h.e_version=EV_CURRENT; h.e_ehsize=sizeof(h);
        h.e_shoff=sizeof(h)+text->size; h.e_shentsize=sizeof(Elf64_Shdr); h.e_shnum=3; h.e_shstrndx=2;
        s[1].sh_name=1; s[1].sh_type=SHT_PROGBITS; s[1].sh_flags=SHF_ALLOC|SHF_EXECINSTR; s[1].sh_offset=sizeof(h); s[1].sh_size=text->size; s[1].sh_addralign=16;
        s[2].sh_name=7; s[2].sh_type=SHT_STRTAB; s[2].sh_offset=h.e_shoff+sizeof(Elf64_Shdr)*3; s[2].sh_size=sizeof(names); s[2].sh_addralign=1;
        ok=fwrite(&h,sizeof(h),1,f)==1 && fwrite(text->data,text->size,1,f)==1 && fwrite(s,sizeof(s),1,f)==1 && fwrite(names,sizeof(names),1,f)==1;
    } else {
        Elf32_Ehdr h={0}; Elf32_Shdr s[3]={{0}};
        h.e_ident[EI_MAG0]=ELFMAG0; h.e_ident[EI_MAG1]=ELFMAG1; h.e_ident[EI_MAG2]=ELFMAG2; h.e_ident[EI_MAG3]=ELFMAG3;
        h.e_ident[EI_CLASS]=ELFCLASS32; h.e_ident[EI_DATA]=ELFDATA2LSB; h.e_ident[EI_VERSION]=EV_CURRENT;
        h.e_type=ET_REL; h.e_machine=EM_386; h.e_version=EV_CURRENT; h.e_ehsize=sizeof(h);
        h.e_shoff=sizeof(h)+text->size; h.e_shentsize=sizeof(Elf32_Shdr); h.e_shnum=3; h.e_shstrndx=2;
        s[1].sh_name=1; s[1].sh_type=SHT_PROGBITS; s[1].sh_flags=SHF_ALLOC|SHF_EXECINSTR; s[1].sh_offset=sizeof(h); s[1].sh_size=text->size; s[1].sh_addralign=16;
        s[2].sh_name=7; s[2].sh_type=SHT_STRTAB; s[2].sh_offset=h.e_shoff+sizeof(Elf32_Shdr)*3; s[2].sh_size=sizeof(names); s[2].sh_addralign=1;
        ok=fwrite(&h,sizeof(h),1,f)==1 && fwrite(text->data,text->size,1,f)==1 && fwrite(s,sizeof(s),1,f)==1 && fwrite(names,sizeof(names),1,f)==1;
    }
    if(fclose(f)) ok=0;
    return ok?chmod(path,0644):-1;
}

int main(int argc,char **argv)
{
    const char *input=NULL,*output=NULL,*target="x86_64";
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"-o") && i+1<argc) output=argv[++i];
        else if(!strcmp(argv[i],"--target") && i+1<argc) target=argv[++i];
        else if(!strcmp(argv[i],"--help")) { puts("mcc-as --target <i386|x86_64> -o output.o input.s"); return 0; }
        else if(argv[i][0]=='-') { fprintf(stderr,"mcc-as: unknown option: %s\n",argv[i]); return 2; }
        else input=argv[i];
    }
    if(!input||!output|| (strcmp(target,"i386")&&strcmp(target,"x86_64"))) { fprintf(stderr,"usage: mcc-as --target <i386|x86_64> -o output.o input.s\n"); return 2; }
    FILE *f=fopen(input,"r"); if(!f){perror(input);return 1;}
    struct buffer text={0}; char *line=NULL; size_t cap=0; ssize_t len; unsigned lineno=0; int failed=0;
    while((len=getline(&line,&cap,f))!=-1) {
        (void)len; ++lineno;
        char *p=trim(line);
        char *comment=strchr(p,';'); if(comment)*comment=0;
        p=trim(p); if(!*p)continue;
        if(p[strlen(p)-1]==':' || !strncmp(p,".global ",8) || !strncmp(p,".globl ",7) || !strcmp(p,".text") || !strcmp(p,".section .text") || !strcmp(p,".intel_syntax noprefix")) continue;
        if(emit_instruction(&text,p,!strcmp(target,"x86_64"))) { fprintf(stderr,"%s:%u: mcc-as: unsupported or invalid instruction: %s\n",input,lineno,p); failed=1; break; }
    }
    free(line); fclose(f);
    if(!failed && !text.size) { fprintf(stderr,"mcc-as: input contains no instructions\n"); failed=1; }
    if(!failed && write_object(output,&text,!strcmp(target,"x86_64"))) { perror(output); failed=1; }
    free(text.data);
    return failed?1:0;
}
