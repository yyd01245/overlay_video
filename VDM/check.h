#ifndef _H_CHECK_
#define _H_CHECK_


#include"macros.h"
#include"colors.h"

#include<string.h>
#include<stdio.h>
#include<time.h>

#include<errno.h>




static char* _logtime(char*buff,int len){

    time_t now;
    struct tm tmnow;
    struct timespec ts;
    time(&now);

    localtime_r(&now,&tmnow);
    clock_gettime(CLOCK_REALTIME,&ts);

    long msec=(ts.tv_nsec+500000)/1000000;

    strftime(buff,len,"%Y/%m/%d %H:%M:%S",&tmnow);
    snprintf(buff+strlen(buff),len-strlen(buff),".%03ld",msec);

    return buff;
}



#define _ALERT(format,arg...) \
        char _timebuf[64];\
        char _fmtbuf[1024];\
        snprintf(_fmtbuf,sizeof _fmtbuf, _FG_RED "%s::" _FG_CYAN "**>>"_FG_MAGENTA "{%s}"_RESET": "_FG_WHITE"%s\n" ,_logtime(_timebuf,sizeof _timebuf),strerror(errno),format);\
        fprintf(stderr,_fmtbuf,##arg);




#define _CHK_BEGIN_error(expr,format,arg...)  \
    if(_UNLIKELY(expr)){            \
        char _timebuf[64];\
        char _fmtbuf[1024];\
        snprintf(_fmtbuf,sizeof _fmtbuf, _FG_RED "%s::" _FG_CYAN "!((" _FG_BROWN #expr _FG_CYAN  "))"_FG_MAGENTA "{%s}"_RESET": "_FG_WHITE"%s\n" ,_logtime(_timebuf,sizeof _timebuf),strerror(errno),format);\
        fprintf(stderr,_fmtbuf,##arg);




#define _CHK_BEGIN(expr,format,arg...)  \
    if(_UNLIKELY(expr)){            \
        char _timebuf[64];\
        char _fmtbuf[1024];\
        snprintf(_fmtbuf,sizeof _fmtbuf, _FG_RED "%s::" _FG_CYAN "!((" _FG_BROWN #expr _FG_CYAN  ")): "_FG_WHITE"%s\n" ,_logtime(_timebuf,sizeof _timebuf),format);\
        fprintf(stderr,_fmtbuf,##arg);


#define _CHK_ELSE }else{

#define _CHK_END }





#define return_val_if_fail(expr,val)    \
    do{ \
        if(_UNLIKELY(!(expr)))         \
            return (val);              \
    }while(false)



#define return_if_fail(expr)            \
    do{ \
        if(_UNLIKELY(!(expr)))         \
            return ;                   \
    }while(false)













#endif
