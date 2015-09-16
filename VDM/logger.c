
#include"logger.h"

#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

#include"check.h"
//#include"rzut/defs.h"
#include"sysutils.h"
//#include"rsm_utils.h"

#include<unistd.h>
#include<dirent.h>
#include<signal.h>
#include<sys/types.h>
#include<fcntl.h>


static logger_t the_log;


static struct {

    int level;
    const char* lv_str;
}level2str[]={
    {LOG_LEVEL_0,"[0]"},
    {LOG_LEVEL_INFO,"[INFO]"},
    {LOG_LEVEL_DEBUG,"[DEBUG]"},
    {LOG_LEVEL_MSG,"[MESSAGE]"},
    {LOG_LEVEL_WARN,"[WARN]"},
    {LOG_LEVEL_ERROR,"[ERROR]"},
};


#define BUFFLEN 8192

#define LOG_FMT "<COL_TS><TIMESTAMP><COL_Reset> <COL_LV><LEVEL><COL_Reset> <COL_MOD><<MODULE>><COL_Reset>:: <COL_MSG><MESSAGE><COL_Reset>"

#define COL_TS     "\033[31m"
#define COL_LV     "\033[32m"
#define COL_MOD    "\033[34;1m"
#define COL_MSG    " " // "\033[33m"
#define COL_Reset  "\033[0m"




static struct _logdata{

    char logdir[MAX_FILELEN];
    char logname_link[MAX_FILELEN];//without path 
    char logname_real[MAX_FILELEN];//without path 
    int logfd;

    int cursiz;//current log size
    int maxsiz;//max allowed log size

    time_t file_ts;//sec
    int dayofyear;

    time_t clean_ts;
    time_t keep_inv;


}my_logdata;



static int logger_clean(logger_t*log)
{

    time_t now;
    struct tm tmnow;
    time(&now);
    localtime_r(&now,&tmnow);

    char entpath[128];

    DIR*d=opendir(my_logdata.logdir);

    struct dirent*ent;

    while(ent=readdir(d)){

        char logname[128];

        int n;
        int dY,dM,dD;
        int tH,tM,tS,tss;
        n=sscanf(ent->d_name,"%[^.].log.%4d%2d%2d.%2d%2d%2d.%d",logname,&dY,&dM,&dD,&tH,&tM,&tS,&tss);
//        fprintf(stderr,"(%d):<%s>\n",n,ent->d_name); 
        if(n!=8)
            continue;

        //skip
        if(!strcmp(ent->d_name,my_logdata.logname_link))
            continue;

        strncat(logname,".log",sizeof logname);
//        fprintf(stderr,"++++++++logname[%s]\n",logname);

        //not my logfile,skip over
        if(strcmp(logname,my_logdata.logname_link))
            continue;
        //skip current logfile 
        if(!strcmp(ent->d_name,my_logdata.logname_real))
            continue;


//        int year=tmnow.tm_year+1900;
//        int dayoy=tmnow.tm_yday+1;
//        fprintf(stderr,"-Year[%d] dayOfYear[%d]\n",year,dayoy);
//        fprintf(stderr,"+Year[%d] dayOfYear[%d]\n",dY,doy);


        if(now-tss>my_logdata.keep_inv){
            snprintf(entpath,sizeof entpath,"%s/%s",my_logdata.logdir,ent->d_name);
//            fprintf(stderr,"TO REMOVE FILE{%s}\n",entpath);
            unlink(entpath); 
        }

    }

    closedir(d);

//    log->cleandate=tmnow.tm_yday;
    my_logdata.clean_ts=now;

    return 1;
}



static void my_log_output(logger_t* log,char*msg,void*data)
{   

    struct _logdata*logdata=(struct _logdata*)data;

    
    int mlen=strlen(msg);

    struct tm tmnow;
    time_t now;
    time(&now);
    localtime_r(&now,&tmnow);

    int doy=tmnow.tm_yday;
    int diff_clean=now-my_logdata.clean_ts;
    int new_size=my_logdata.cursiz+mlen;

//    fprintf(stderr,"diff_file:=%d \n",diff_file);
//    fprintf(stderr,"diff_clean:=%d \n",diff_clean);

    if(doy!=my_logdata.dayofyear || (new_size>my_logdata.maxsiz && my_logdata.maxsiz!=0)){
        
        logger_user_reset_logfile(log,NULL);
    }

    if(my_logdata.keep_inv!=0 && diff_clean>my_logdata.keep_inv){
        
        logger_clean(log);
    }

    my_logdata.cursiz+=mlen+1;


    write(my_logdata.logfd,msg,strlen(msg));
    write(my_logdata.logfd,"\n",1);
    fsync(my_logdata.logfd);

}


void logger_user_init(logger_t*log,const char*logpath)
{
    
    my_logdata.cursiz=0;
    my_logdata.maxsiz=0;//4*1024*1024;

    my_logdata.file_ts=0;//sec
    my_logdata.dayofyear=0;//sec

    my_logdata.clean_ts=0;
    my_logdata.keep_inv=0;

    my_logdata.logfd=0;



    logger_user_reset_logfile(log,logpath);
    logger_clean(log);

    logger_set_logging_func(log,my_log_output, &my_logdata);
}


void logger_user_fini(logger_t*log,void*data)
{
    if(my_logdata.logfd)
        close(my_logdata.logfd);
}


static void* log_queue_thr(logger_t*log)
{
    pthread_setname_np(pthread_self(),"logger");

    int ret;

    sigset_t set,oset;
    sigfillset(&set);
    ret=pthread_sigmask(SIG_BLOCK,&set,&oset);

    char*pmsg;


    while(1){

        ret=async_que_deque(&log->msgq,(void**)&pmsg);

        if(log->log_func)
            log->log_func(log,pmsg,log->log_data);


        free(pmsg);

    }

}





int logger_init(logger_t*log)
{
    int ret;
    return_val_if_fail(log!=NULL,-1);
    bzero(log,sizeof(logger_t));

    log->b_stderr=1;
    log->stlevel=LOG_LEVEL_INFO;


    ret=async_que_init(&log->msgq);
    _CHK_BEGIN(ret<0,"Init AsyncQueue Failed.")
        return -2;
    _CHK_END


#if 0
    pthread_mutexattr_t mtxattr;
    pthread_mutexattr_init(&mtxattr);
    pthread_mutexattr_settype(&mtxattr,PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&log->lock,&mtxattr);
    pthread_mutexattr_destroy(&mtxattr);
#else
    pthread_mutex_init(&log->lock,NULL);

#endif


    pthread_attr_t tattr;
    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED);

    ret=pthread_create(&log->logthr,&tattr,(void*(*)(void*))log_queue_thr,log);
    pthread_attr_destroy(&tattr);
    _CHK_BEGIN_error(ret!=0,"Create Logging Thread Failed..")
        goto fail0;
    _CHK_END


    return 0;

fail0:
    async_que_fini(&log->msgq);

    return -3;
}



int logger_user_reset_logfile(logger_t*log,const char*logpath)
{
//    fprintf(stderr,"======\n");
    return_val_if_fail(log!=NULL,-2);
    

    time_t now;
    struct tm tmnow;
    time(&now);
    localtime_r(&now,&tmnow);
    
//    log->date=tmnow.tm_yday;
    my_logdata.file_ts=now;
    my_logdata.dayofyear=tmnow.tm_yday;
    my_logdata.cursiz=0;

    //save file's prefix for later uses;
    if(logpath){
        strlcpy(my_logdata.logdir,logpath,sizeof(my_logdata.logdir));

        char*p=strrchr(my_logdata.logdir,'/');
        if(!p){
            strlcpy(my_logdata.logdir,"./",sizeof(my_logdata.logdir));
//            p=my_logdata.logdir+strlen(my_logdata.logdir);
            p=(char*)logpath;
        }else{
            *p=0;    
            p++;
        }

        strlcpy(my_logdata.logname_link,p,sizeof my_logdata.logname_link);
        if(!strstr(my_logdata.logname_link,".log"))
            strncat(my_logdata.logname_link,".log",sizeof my_logdata.logname_link);

    }


    char timestr[32];
    strftime(timestr,sizeof(timestr),"%Y%m%d.%H%M%S.%s",&tmnow);

    snprintf(my_logdata.logname_real,sizeof(my_logdata.logname_real),"%s.%s",my_logdata.logname_link,timestr);

//    fprintf(stderr,"log_dir     [%s]\n",my_logdata.logdir);
//    fprintf(stderr,"logname_link[%s]\n",my_logdata.logname_link);
//    fprintf(stderr,"logname_real[%s]\n",my_logdata.logname_real);

    if(my_logdata.logfd)
        close(my_logdata.logfd);

    char fullpath_real[MAX_FILELEN];
    char fullpath_link[MAX_FILELEN];
    snprintf(fullpath_real,sizeof fullpath_real,"%s/%s",my_logdata.logdir,my_logdata.logname_real);
    snprintf(fullpath_link,sizeof fullpath_link,"%s/%s",my_logdata.logdir,my_logdata.logname_link);

    my_logdata.logfd=open_file(fullpath_real,O_CREAT|O_WRONLY|O_APPEND,0666);
    _CHK_BEGIN_error(my_logdata.logfd<0,"Open Log Failed..")
        return -1;
    _CHK_END

    if(!access(fullpath_link,F_OK)){
        unlink(fullpath_link);
    }

    symlink(my_logdata.logname_real,fullpath_link);
 
    return 0;
}

void logger_user_set_maxsize(logger_t*log,int maxsize)
{
//    fprintf(stderr,"MAXSIZE::%d\n",maxsize);
    return_if_fail(log!=NULL);
    if(maxsize<0){
        maxsize=0;
    }

    my_logdata.maxsiz=maxsize;
}

void logger_user_set_cleantime(logger_t*log,int nday)
{
    return_if_fail(log!=NULL);

    if(nday<=0){
        my_logdata.keep_inv=0;//sec
        return;
    }
        
//    my_logdata.keep_inv=nday;//sec
    my_logdata.keep_inv=nday*24*60*60;//sec
    

}


void logger_fini(logger_t*log)
{
    return_if_fail(NULL!=log);

    pthread_cancel(log->logthr);
    async_que_fini(&log->msgq);

    if(log->log_data==&my_logdata && my_logdata.logfd ){
        close(my_logdata.logfd);
    }
    pthread_mutex_destroy(&log->lock);

}




void logger_set_level(logger_t*log,int lv)
{
    return_if_fail(NULL!=log);

    log->stlevel=lv;
}



void logger_set_logging_func(logger_t*log,logging_func func, void* data)
{
    return_if_fail(NULL!=log);

    log->log_data=data;
    log->log_func=func;

}



static void format_output2(char*buffout,char*bufferr,char*msg,size_t siz,int level,const char*modstr)
{
     
    int ret;
    time_t now;
    struct tm tmnow;
    time(&now);
    localtime_r(&now,&tmnow);

    char timestr[48];
//    ret=strftime(timestr,sizeof(timestr),"%Y/%m/%d-%H:%M:%S",&tmnow);
    get_time(timestr,48);

    char levelstr[16];
    ret=snprintf(levelstr,sizeof(levelstr),"%s",level2str[level].lv_str);

//    char modstr[24];
//    ret=get_current_module_name(modstr,sizeof(modstr));

    strlcpy(buffout,LOG_FMT,siz);
    strlcpy(bufferr,LOG_FMT,siz);

    strrepl(buffout,"<TIMESTAMP>",timestr);
    strrepl(bufferr,"<TIMESTAMP>",timestr);

    strrepl(buffout,"<LEVEL>",levelstr);
    strrepl(bufferr,"<LEVEL>",levelstr);

    strrepl(buffout,"<MODULE>",modstr);
    strrepl(bufferr,"<MODULE>",modstr);


    strrepl(buffout,"<COL_LV>","");
    strrepl(buffout,"<COL_MOD>","");
    strrepl(buffout,"<COL_TS>","");
    strrepl(buffout,"<COL_MSG>","");
    strrepl(buffout,"<COL_Reset>","");
  

    strrepl(bufferr,"<COL_LV>",COL_LV);
    strrepl(bufferr,"<COL_MOD>",COL_MOD);
    strrepl(bufferr,"<COL_TS>",COL_TS);
    strrepl(bufferr,"<COL_MSG>",COL_MSG);
    strrepl(bufferr,"<COL_Reset>",COL_Reset);
  
    strrepl(buffout,"<MESSAGE>",msg);
    strrepl(bufferr,"<MESSAGE>",msg);

}





void logger_logv(logger_t*log,int lv,const char*mod,const char*fmt,va_list ap)
{
    if(lv<log->stlevel)
        return;

    static char msg[BUFFLEN]={0,};
    static char buff[BUFFLEN]={0,};
    static char bufferr[BUFFLEN]={0,};

    pthread_mutex_lock(&log->lock);//protect static buffer above

    vsnprintf(msg,sizeof(msg),fmt,ap);

    format_output2(buff,bufferr,msg,BUFFLEN,lv,mod);

    if(log->b_stderr){
        fprintf(stderr,"%s\n",bufferr);
        fflush(stderr);
    }

    async_que_enque(&log->msgq,strdup(buff));

    pthread_mutex_unlock(&log->lock);

}



void logger_log(logger_t*log,int lv,const char*mod,const char*fmt,...)
{
    va_list ap;
    va_start(ap,fmt);

    logger_logv(log,lv,mod,fmt,ap);

    va_end(ap);
}




