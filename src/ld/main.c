#define _POSIX_C_SOURCE 200809L
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define PAGE_SIZE 4096U
#define BASE32 0x08048000U
#define BASE64 0x400000ULL

static unsigned char *read_file(const char *path,size_t *size)
{
    FILE *f=fopen(path,"rb"); if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}
    long n=ftell(f); if(n<0||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
    unsigned char *p=malloc((size_t)n); if(!p){fclose(f);return NULL;}
    if(fread(p,1,(size_t)n,f)!=(size_t)n){free(p);fclose(f);return NULL;}
    fclose(f);*size=(size_t)n;return p;
}

static int extract_text(const unsigned char *data,size_t size,int is64,const unsigned char **text,size_t *text_size)
{
    if(is64) {
        if(size<sizeof(Elf64_Ehdr))return -1;
        const Elf64_Ehdr *h=(const Elf64_Ehdr*)data;
        if(h->e_type!=ET_REL||h->e_shentsize!=sizeof(Elf64_Shdr)||!h->e_shnum||h->e_shstrndx>=h->e_shnum)return -1;
        if(h->e_shoff>size||h->e_shnum>(size-h->e_shoff)/sizeof(Elf64_Shdr))return -1;
        const Elf64_Shdr *s=(const Elf64_Shdr*)(data+h->e_shoff);
        const Elf64_Shdr *names=&s[h->e_shstrndx];
        if(names->sh_offset>size||names->sh_size>size-names->sh_offset)return -1;
        const char *str=(const char*)data+names->sh_offset;
        for(unsigned i=0;i<h->e_shnum;i++) if(s[i].sh_name<names->sh_size&&!strcmp(str+s[i].sh_name,".text")) {
            if(s[i].sh_offset>size||s[i].sh_size>size-s[i].sh_offset)return -1;
            *text=data+s[i].sh_offset;*text_size=(size_t)s[i].sh_size;return 0;
        }
    } else {
        if(size<sizeof(Elf32_Ehdr))return -1;
        const Elf32_Ehdr *h=(const Elf32_Ehdr*)data;
        if(h->e_type!=ET_REL||h->e_shentsize!=sizeof(Elf32_Shdr)||!h->e_shnum||h->e_shstrndx>=h->e_shnum)return -1;
        if(h->e_shoff>size||h->e_shnum>(size-h->e_shoff)/sizeof(Elf32_Shdr))return -1;
        const Elf32_Shdr *s=(const Elf32_Shdr*)(data+h->e_shoff);
        const Elf32_Shdr *names=&s[h->e_shstrndx];
        if(names->sh_offset>size||names->sh_size>size-names->sh_offset)return -1;
        const char *str=(const char*)data+names->sh_offset;
        for(unsigned i=0;i<h->e_shnum;i++) if(s[i].sh_name<names->sh_size&&!strcmp(str+s[i].sh_name,".text")) {
            if(s[i].sh_offset>size||s[i].sh_size>size-s[i].sh_offset)return -1;
            *text=data+s[i].sh_offset;*text_size=(size_t)s[i].sh_size;return 0;
        }
    }
    return -1;
}

static int link_file(const char *input,const char *output,int is64)
{
    size_t in_size=0,code_size=0;
    unsigned char *in=read_file(input,&in_size); if(!in){perror(input);return -1;}
    const unsigned char *code=NULL;
    if(in_size<EI_NIDENT||memcmp(in,ELFMAG,SELFMAG)||in[EI_CLASS]!=(is64?ELFCLASS64:ELFCLASS32)||in[EI_DATA]!=ELFDATA2LSB||extract_text(in,in_size,is64,&code,&code_size)) {
        fprintf(stderr,"mcc-ld: %s is not a supported MCC ELF relocatable object\n",input);free(in);return -1;
    }
    FILE *f=fopen(output,"wb"); if(!f){perror(output);free(in);return -1;}
    int ok=0;
    if(is64) {
        Elf64_Ehdr h={0}; Elf64_Phdr p={0};
        h.e_ident[EI_MAG0]=ELFMAG0;h.e_ident[EI_MAG1]=ELFMAG1;h.e_ident[EI_MAG2]=ELFMAG2;h.e_ident[EI_MAG3]=ELFMAG3;
        h.e_ident[EI_CLASS]=ELFCLASS64;h.e_ident[EI_DATA]=ELFDATA2LSB;h.e_ident[EI_VERSION]=EV_CURRENT;
        h.e_type=ET_EXEC;h.e_machine=EM_X86_64;h.e_version=EV_CURRENT;h.e_entry=BASE64+PAGE_SIZE;
        h.e_phoff=sizeof(h);h.e_ehsize=sizeof(h);h.e_phentsize=sizeof(p);h.e_phnum=1;
        p.p_type=PT_LOAD;p.p_flags=PF_R|PF_X;p.p_offset=0;p.p_vaddr=BASE64;p.p_paddr=BASE64;
        p.p_filesz=PAGE_SIZE+code_size;p.p_memsz=p.p_filesz;p.p_align=PAGE_SIZE;
        ok=fwrite(&h,sizeof(h),1,f)==1&&fwrite(&p,sizeof(p),1,f)==1;
        if(ok) { unsigned char zero[PAGE_SIZE]={0}; size_t head=sizeof(h)+sizeof(p); ok=fwrite(zero,1,PAGE_SIZE-head,f)==PAGE_SIZE-head; }
        if(ok)ok=fwrite(code,1,code_size,f)==code_size;
    } else {
        Elf32_Ehdr h={0}; Elf32_Phdr p={0};
        h.e_ident[EI_MAG0]=ELFMAG0;h.e_ident[EI_MAG1]=ELFMAG1;h.e_ident[EI_MAG2]=ELFMAG2;h.e_ident[EI_MAG3]=ELFMAG3;
        h.e_ident[EI_CLASS]=ELFCLASS32;h.e_ident[EI_DATA]=ELFDATA2LSB;h.e_ident[EI_VERSION]=EV_CURRENT;
        h.e_type=ET_EXEC;h.e_machine=EM_386;h.e_version=EV_CURRENT;h.e_entry=BASE32+PAGE_SIZE;
        h.e_phoff=sizeof(h);h.e_ehsize=sizeof(h);h.e_phentsize=sizeof(p);h.e_phnum=1;
        p.p_type=PT_LOAD;p.p_offset=0;p.p_vaddr=BASE32;p.p_paddr=BASE32;p.p_filesz=PAGE_SIZE+(uint32_t)code_size;
        p.p_memsz=p.p_filesz;p.p_flags=PF_R|PF_X;p.p_align=PAGE_SIZE;
        ok=fwrite(&h,sizeof(h),1,f)==1&&fwrite(&p,sizeof(p),1,f)==1;
        if(ok) { unsigned char zero[PAGE_SIZE]={0}; size_t head=sizeof(h)+sizeof(p); ok=fwrite(zero,1,PAGE_SIZE-head,f)==PAGE_SIZE-head; }
        if(ok)ok=fwrite(code,1,code_size,f)==code_size;
    }
    if(fclose(f))ok=0;
    free(in);
    if(!ok)return -1;
    return chmod(output,0755);
}

int main(int argc,char **argv)
{
    const char *input=NULL,*output=NULL,*target="x86_64";
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"-o")&&i+1<argc)output=argv[++i];
        else if(!strcmp(argv[i],"--target")&&i+1<argc)target=argv[++i];
        else if(!strcmp(argv[i],"--help")){puts("mcc-ld --target <i386|x86_64> -o output.elf input.o");return 0;}
        else if(argv[i][0]=='-'){fprintf(stderr,"mcc-ld: unknown option: %s\n",argv[i]);return 2;}
        else if(!input)input=argv[i];
        else {fprintf(stderr,"mcc-ld: this initial linker accepts one input object\n");return 2;}
    }
    if(!input||!output||(strcmp(target,"i386")&&strcmp(target,"x86_64))){fprintf(stderr,"usage: mcc-ld --target <i386|x86_64> -o output.elf input.o\n");return 2;}
    if(link_file(input,output,!strcmp(target,"x86_64"))){fprintf(stderr,"mcc-ld: link failed\n");return 1;}
    return 0;
}
