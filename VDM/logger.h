#ifndef _H_LOG_RSM_
#define _H_LOG_RSM_

#ifdef __cplusplus
extern "C"{
#endif

#include<stdarg.h>
#include<stdio.h>

#include"asyncqueue.h"

typedef enum _stlevel{

    LOG_LEVEL_0=0,
    LOG_LEVEL_INFO=1,
    LOG_LEVEL_DEBUG=2,
    LOG_LEVEL_MSG=3,
    LOG_LEVEL_WARN=4,
    LOG_LEVEL_ERROR=5,

}log_lv_t;



#define MAX_FILELEN 128


typedef struct _logger logger_t;

typedef void (*logging_func)(logger_t*,char*msg,void*data);


struct _logger{

    int stlevel;

    logging_func log_func;
    void*        log_data;

    pthread_mutex_t lock;
    pthread_t logthr;
    async_que_t msgq;

    int b_stderr;//output to stderr
};




int logger_init(logger_t*log);
void logger_fini(logger_t*log);
void logger_set_level(logger_t*log,int lv);

void logger_set_logging_func(logger_t*log,logging_func func, void* data);

void logger_user_init(logger_t*log,const char*filename);
void logger_user_fini(logger_t*log,void*data);

int  logger_user_reset_logfile(logger_t*log,const char*filename);
void logger_user_set_maxsize(logger_t*log,int maxsize);
void logger_user_set_cleantime(logger_t*log,int nday);



void logger_logv(logger_t*log,int lv,const char*modname,const char*fmt,va_list ap);
void logger_log(logger_t*log,int lv,const char*modname,const char*fmt,...);






#ifdef __cplusplus
}
#endif


#endif
