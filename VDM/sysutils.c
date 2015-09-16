
#include"sysutils.h"
#include<errno.h>
#include<unistd.h>
#include<time.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<string.h>
#include<signal.h>
#include<fcntl.h>
#include<sys/stat.h>
#include"check.h"


#define NS_PER_SEC 1000000000UL
#define US_PER_SEC 1000000UL
#define MS_PER_SEC 1000UL


static void tmspec_add( struct timespec*ots,const struct timespec*ts0,const struct timespec* ts1)
{

    long carry=0;
    long ns=ts0->tv_nsec+ts1->tv_nsec;

    if(ns>=NS_PER_SEC){
        carry=1;
        ns-=NS_PER_SEC;
    }

    ots->tv_sec=ts0->tv_sec+ts1->tv_sec+carry;
    ots->tv_nsec=ns;

}


static void tmspec_diff( struct timespec*ots,const struct timespec*ts0,const struct timespec* ts1)
{

    long carry=0;
    long ns=ts1->tv_nsec-ts0->tv_nsec;
    if(ns<0){
        ns+=NS_PER_SEC;
        carry=1;
    }
    

    ots->tv_sec=ts1->tv_sec-ts0->tv_sec-carry;
    ots->tv_nsec=ns;

}







void timespec_str(const struct timespec*ts,char*outstr,int bufsiz)
{
    snprintf(outstr,bufsiz,"timespec{ .tv_sec=%ld\t.tv_nsec=%ld }",ts->tv_sec,ts->tv_nsec);

}



int absleep(const struct timespec*req)
{

    int ret;
    struct timespec ts;

    ret=clock_gettime(CLOCK_REALTIME,&ts);
    tmspec_add(&ts,&ts,req);

restart:

    ret=clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&ts,NULL);
    if(ret<0 && errno==EINTR){
        fprintf(stderr,"EINTR...\n");
        goto restart;
    }

    return ret;

}






int register_signal(int signum,sighandler_ft hndlr)
{
    
    int ret;
    struct sigaction sa,osa;

    memset(&sa,0,sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);

    sa.sa_flags=0;
    sa.sa_handler=hndlr;


    ret=sigaction(signum,&sa,&osa);
    
    return ret;

}





static int str_pos(const char*p)
{

    int off=0;;
    const char*pp=p;
    while(*pp){
        if(*pp=='\n' || *pp=='\0')
            return off;
        off++;
        pp++;
    }

}


int popen_out(const char *command, const char *type,char* buff,size_t buff_siz)
{
    
    FILE*fp=popen(command,type);
    size_t siz=buff_siz;
    int offset;
    char*pbuff=buff;
    char*p;

    while(fgets(pbuff,siz,fp)){
        p=strchr(pbuff,'\n');
        if(!p)
            break;
        offset=p-pbuff+1;
        pbuff+=offset;
        siz-=offset;
        if(siz<=0)break;
    }
    buff[buff_siz-1]=0;

    pclose(fp);
    
    return offset;
}



char* strupper(char*pstr)
{

    int i;
    for(i=0;pstr[i];i++){
        pstr[i]&=~0x20;
    }

    return pstr;
}



char* strlower(char*pstr)
{

    int i;
    for(i=0;pstr[i];i++){
        pstr[i]|=0x20;
    }

    return pstr;
}




int strrepl(char*pstr,const char*pat,const char*rep)
{
    int nmatch=0;
    char*pl;

    int p_len=strlen(pstr);
    int pat_len=strlen(pat);
    int rep_len=strlen(rep);

    int mlen;
    char*ppat,*prep;

    while(pl=strstr(pstr,pat)){
        if(!pl)
            break;
        nmatch++;

        ppat=pl+pat_len;
        prep=pl+rep_len;

        mlen=p_len-(ppat-pstr);

        memmove(prep,ppat,mlen+1);
        memmove(pl,rep,rep_len);
    }

    return nmatch;
}



int snprintf_append(char*buff,size_t buffsiz,const char*fmt,...)
{
    va_list va;
    int ret;

    int pos=strlen(buff);

    if(pos>=buffsiz-1){
        return -1;
    }

    va_start(va,fmt);

    ret=vsnprintf(buff+pos,buffsiz-pos,fmt,va);

    va_end(va);

    return ret;
}




int open_file(const char*path,int flags,mode_t mode)
{

    int ret;
    int plen=strlen(path);

    int fd;

    char* ppath=calloc(1,plen+1);
    memcpy(ppath,path,plen+1);

    char*str,*curit;

    if(ppath[0]=='/'){
        str=&ppath[1];
    }else{
        str=&ppath[0];
    }

    while(*str && (curit=strstr(str,"/"))){

        *curit=0;
        ret=mkdir(ppath,0777);
        _CHK_BEGIN_error(ret<0&&errno!=EEXIST,"MakeDirectory.")
                ret=-2;
                goto fail;
        _CHK_END

        *curit='/';
        str=curit+1;
    }

    if(*str){
//        fprintf(stderr,"___________________________:::[%s]<%s>\n",ppath,str);
        fd=open(ppath,flags,mode);
        _CHK_BEGIN_error(fd<0,"OpenFile.")
                ret=-3;
                goto fail;
        _CHK_END
    }else{
        fd=0;
    }
    free(ppath);

    return fd;

fail:
    free(ppath);
    return ret;

}


void get_time(char*buff,int siz)
{
    char lbuf[32];
    struct tm tmnow;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME,&ts);
    localtime_r(&ts.tv_sec,&tmnow);
    strftime(lbuf,31,"%Y/%m/%d-%H:%M:%S",&tmnow);
    snprintf(buff,siz,"%s.%03ld",lbuf,(ts.tv_nsec+500000)/1000000);

}





int become_daemon(void)
{
    

    switch(fork()){
        case -1:
            return -1;
        case 0:
            break;
        default:
            _exit(EXIT_SUCCESS);
    }


    if(setsid()==-1)
        return -1;


    switch(fork()){
        case -1:
            return -1;
        case 0:
            break;
        default:
            _exit(EXIT_SUCCESS);
    }

    umask(0);
   
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
//    close(STDERR_FILENO);

    int fd=open("/dev/null",O_RDWR);
    if(fd!=STDIN_FILENO)
        return -1;

    if(STDOUT_FILENO!=dup2(STDIN_FILENO,STDOUT_FILENO))
        return -1;

//    if(STDERR_FILENO!=dup2(STDIN_FILENO,STDERR_FILENO))
//        return -1;


    return 0;
}







