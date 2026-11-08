/* AmigaOS 3.x m68k compat shims */
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <complex.h>

#define AMIGA_TO_UNIX_DAYS 2922L

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
    (void)tz;
    if (!tv) return -1;
    struct DateStamp ds;
    DateStamp(&ds);
    tv->tv_sec = (ds.ds_Days + AMIGA_TO_UNIX_DAYS) * 86400L
               + ds.ds_Minute * 60L
               + ds.ds_Tick / TICKS_PER_SECOND;
    tv->tv_usec = (ds.ds_Tick % TICKS_PER_SECOND) * (1000000L / TICKS_PER_SECOND);
    return 0;
}
int _gettimeofday(struct timeval *tv, struct timezone *tz) { return gettimeofday(tv,tz); }

#undef timeradd
#undef timersub
void timeradd(const struct timeval *a,const struct timeval *b,struct timeval *res){
    res->tv_sec=a->tv_sec+b->tv_sec; res->tv_usec=a->tv_usec+b->tv_usec;
    if(res->tv_usec>=1000000){res->tv_sec++;res->tv_usec-=1000000;}
}
void timersub(const struct timeval *a,const struct timeval *b,struct timeval *res){
    res->tv_sec=a->tv_sec-b->tv_sec; res->tv_usec=a->tv_usec-b->tv_usec;
    if(res->tv_usec<0){res->tv_sec--;res->tv_usec+=1000000;}
}

int fcntl(int fd,int cmd,...){(void)fd;
  switch(cmd){
    case F_GETFL:case F_SETFL:case F_GETFD:case F_SETFD:return 0;
    /* OWB is the only user of its databases, so SQLite's file locks are always granted. curl's bsdsocket
       sockets also come through here, and clib2's fcntl doesn't know them, so this stays a shim. */
    case F_SETLK:case F_SETLKW:return 0;
    case F_GETLK:{va_list ap;struct flock *l;va_start(ap,cmd);l=va_arg(ap,struct flock *);va_end(ap);if(l)l->l_type=F_UNLCK;return 0;}
    default:errno=EINVAL;return -1;}}
int dup(int fd){return fd;}
long sysconf(int name){(void)name;return 0;}
char *getcwd(char *buf,size_t size){if(buf&&size>0)buf[0]=0;return buf;}
int mkdir(const char *path,mode_t mode){(void)mode;BPTR lock=CreateDir(path);if(!lock)return -1;UnLock(lock);return 0;}
int rmdir(const char *path){return DeleteFile(path)?0:-1;}
int fchmod(int fd,mode_t mode){(void)fd;(void)mode;return 0;}
int fchown(int fd,uid_t owner,gid_t group){(void)fd;(void)owner;(void)group;return 0;}
uid_t geteuid(void){return 0;}
int utimes(const char *filename,const struct timeval *times){(void)filename;(void)times;return 0;}

void __assert_func(const char *file,int line,const char *func,const char *expr){
    if(file&&expr) fprintf(stderr,"Assertion failed: %s, file %s, line %d\n",expr,file,line);
    abort();
}

/* BUILD THIS FILE WITH -O0 -fno-builtin. With optimisation, GCC turns cos(y)+I*sin(y) into a call to
 * cexp(), so these would call themselves until the stack runs out and overwrites the program. */
double complex cexp(double complex z){
    double r=exp(creal(z));
    return r*(cos(cimag(z))+I*sin(cimag(z)));
}
float complex cexpf(float complex z){
    float r=expf(crealf(z));
    return r*(cosf(cimagf(z))+I*sinf(cimagf(z)));
}

char *strndup(const char *s,size_t n){
    size_t l=strlen(s); if(l>n)l=n;
    char *p=(char*)malloc(l+1); if(!p)return NULL;
    memcpy(p,s,l); p[l]=0; return p;
}
char *stpcpy(char *dst,const char *src){
    while((*dst++=*src++)); return dst-1;
}
