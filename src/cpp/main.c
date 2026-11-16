#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define MAX_MACROS 256
#define MAX_INCLUDE 16
struct macro { char *name; char *value; };
static struct macro macros[MAX_MACROS];
static size_t macro_count;
static char *include_dirs[MAX_INCLUDE];
static size_t include_count;

static char *trim(char *s)
{
    while(isspace((unsigned char)*s))++s;
    size_t n=strlen(s);
    while(n&&isspace((unsigned char)s[n-1]))s[--n]=0;
    return s;
}
static int macro_index(const char *name)
{
    for(size_t i=0;i<macro_count;i++)if(!strcmp(macros[i].name,name))return (int)i;
    return -1;
}
static int define_macro(const char *name,const char *value)
{
    int i=macro_index(name);
    if(i<0){if(macro_count==MAX_MACROS)return -1;i=(int)macro_count++;macros[i].name=strdup(name);}
    free(macros[i].value);macros[i].value=strdup(value);
    return macros[i].name&&macros[i].value?0:-1;
}
static void undef_macro(const char *name)
{
    int i=macro_index(name);if(i<0)return;
    free(macros[i].name);free(macros[i].value);
    macros[i]=macros[--macro_count];
}
static char *expand(const char *line)
{
    size_t cap=strlen(line)*4+64,len=0;char *out=malloc(cap);if(!out)return NULL;
    for(size_t i=0;line[i];) {
        if(isalpha((unsigned char)line[i])||line[i]=='_') {
            size_t start=i++;while(isalnum((unsigned char)line[i])||line[i]=='_')++i;
            char id[256];size_t n=i-start;if(n>=sizeof(id))n=sizeof(id)-1;
            memcpy(id,line+start,n);id[n]=0;
            int m=macro_index(id);const char *v=m>=0?macros[m].value:id;size_t vn=strlen(v);
            if(len+vn+1>cap){cap=(len+vn+1)*2;char*p=realloc(out,cap);if(!p){free(out);return NULL;}out=p;}
            memcpy(out+len,v,vn);len+=vn;
        } else {
            if(len+2>cap){cap*=2;char*p=realloc(out,cap);if(!p){free(out);return NULL;}out=p;}
            out[len++]=line[i++];
        }
    }
    out[len]=0;return out;
}
static FILE *open_include(const char *name,const char *current)
{
    char path[4096];
    if(current){const char *slash=strrchr(current,'/');if(slash){size_t n=(size_t)(slash-current+1);if(n+strlen(name)<sizeof(path)){memcpy(path,current,n);strcpy(path+n,name);FILE*f=fopen(path,"r");if(f)return f;}}}
    for(size_t i=0;i<include_count;i++){if(snprintf(path,sizeof(path),"%s/%s",include_dirs[i],name)<(int)sizeof(path)){FILE*f=fopen(path,"r");if(f)return f;}}
    return fopen(name,"r");
}
static int process_file(const char *path,unsigned depth)
{
    if(depth>32){fprintf(stderr,"mcc-cpp: include nesting too deep\n");return -1;}
    FILE *f=fopen(path,"r");if(!f){perror(path);return -1;}
    char *line=NULL;size_t cap=0;ssize_t n;unsigned lineno=0;
    struct {int parent,condition,seen_else;} cond[64];int level=0,active=1,failed=0;
    while((n=getline(&line,&cap,f))!=-1) {
        (void)n;++lineno;char *p=trim(line);
        if(*p=='#') {
            p=trim(p+1);char *arg=p;while(*p&&!isspace((unsigned char)*p))++p;
            if(*p)*p++=0;p=trim(p);
            if(!strcmp(arg,"define")&&active) {
                char *name=p;while(*p&&(isalnum((unsigned char)*p)||*p=='_'))++p;
                if(p==name){fprintf(stderr,"%s:%u: invalid #define\n",path,lineno);failed=1;break;}
                char saved=*p;*p=0;char *value=saved?trim(p+1):"1";
                if(define_macro(name,value)){failed=1;break;}
            } else if(!strcmp(arg,"undef")&&active) undef_macro(p);
            else if(!strcmp(arg,"ifdef")||!strcmp(arg,"ifndef")) {
                if(level==64){failed=1;break;}
                int yes=macro_index(p)>=0;if(!strcmp(arg,"ifndef"))yes=!yes;
                cond[level].parent=active;cond[level].condition=yes;cond[level].seen_else=0;
                active=active&&yes;++level;
            } else if(!strcmp(arg,"else")) {
                if(!level||cond[level-1].seen_else){fprintf(stderr,"%s:%u: unmatched #else\n",path,lineno);failed=1;break;}
                cond[level-1].seen_else=1;active=cond[level-1].parent&&!cond[level-1].condition;
            } else if(!strcmp(arg,"endif")) {
                if(!level){fprintf(stderr,"%s:%u: unmatched #endif\n",path,lineno);failed=1;break;}
                active=cond[--level].parent;
            } else if(!strcmp(arg,"include")&&active) {
                if((*p=='"'||*p=='<')) {
                    char close=*p=='"'?'"':'>';char *end=strchr(p+1,close);
                    if(!end){fprintf(stderr,"%s:%u: malformed #include\n",path,lineno);failed=1;break;}
                    *end=0;FILE *inc=open_include(p+1,path);
                    if(!inc){fprintf(stderr,"%s:%u: include not found: %s\n",path,lineno,p+1);failed=1;break;}
                    char tmp[]="/tmp/mcc-cpp-XXXXXX";int fd=mkstemp(tmp);
                    if(fd<0){fclose(inc);failed=1;break;}
                    FILE *out=fdopen(fd,"w");char buf[4096];size_t got;
                    while((got=fread(buf,1,sizeof(buf),inc))>0)fwrite(buf,1,got,out);
                    fclose(inc);fclose(out);
                    if(process_file(tmp,depth+1))failed=1;
                    remove(tmp);if(failed)break;
                } else {fprintf(stderr,"%s:%u: malformed #include\n",path,lineno);failed=1;break;}
            } else if(!strcmp(arg,"error")&&active) {
                fprintf(stderr,"%s:%u: #error %s\n",path,lineno,p);failed=1;break;
            }
        } else if(active) {
            char *expanded=expand(line);if(!expanded){failed=1;break;}
            fputs(expanded,stdout);free(expanded);
        }
    }
    if(level){fprintf(stderr,"%s: unterminated conditional directive\n",path);failed=1;}
    free(line);fclose(f);return failed?-1:0;
}
int main(int argc,char **argv)
{
    const char *input=NULL;
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"-D")&&i+1<argc){char *eq=strchr(argv[++i],'=');if(eq){*eq=0;if(define_macro(argv[i],eq+1))return 1;}else if(define_macro(argv[i],"1"))return 1;}
        else if(!strncmp(argv[i],"-D",2)){char *v=argv[i]+2,*eq=strchr(v,'=');if(eq){*eq=0;if(define_macro(v,eq+1))return 1;}else if(define_macro(v,"1"))return 1;}
        else if(!strcmp(argv[i],"-U")&&i+1<argc)undef_macro(argv[++i]);
        else if(!strcmp(argv[i],"-I")&&i+1<argc){if(include_count==MAX_INCLUDE)return 2;include_dirs[include_count++]=argv[++i];}
        else if(!strncmp(argv[i],"-I",2)){if(include_count==MAX_INCLUDE)return 2;include_dirs[include_count++]=argv[i]+2;}
        else if(!strcmp(argv[i],"--help")){puts("mcc-cpp [-Dname[=value]] [-Uname] [-Idir] input.c");return 0;}
        else if(argv[i][0]=='-'){fprintf(stderr,"mcc-cpp: unknown option: %s\n",argv[i]);return 2;}
        else input=argv[i];
    }
    if(!input){fprintf(stderr,"usage: mcc-cpp [-Dname[=value]] [-Uname] [-Idir] input.c\n");return 2;}
    return process_file(input,0)?1:0;
}
