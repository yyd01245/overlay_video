#ifndef _H_SYS_UTILS_
#define _H_SYS_UTILS_


#include<time.h>

#include<sys/types.h>

#ifdef __cplusplus
extern "C"{
#endif


int absleep(const struct timespec*req);



typedef void (*sighandler_ft)(int);


int register_signal(int signum,sighandler_ft hndlr );


int popen_out(const char *command, const char *type,char* buff,size_t buff_siz);




int strrepl(char*pstr,const char*pat,const char*rep);

int snprintf_append(char*buff,size_t buffsiz,const char*fmt,...);

char* strlower(char*pstr);
char* strupper(char*pstr);



int open_file(const char*path,int flags,mode_t mode);

void get_time(char*buff,int siz);


int become_daemon(void);


#ifdef __cplusplus
}
#endif



#endif
