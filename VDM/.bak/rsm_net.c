
#include<stdio.h>
#include<stdlib.h>
#include"rsm_net.h"


#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<signal.h>
#include<pthread.h>
#include<fcntl.h>

#include"rzut/chks.h"
#include"rzut/defs.h"




#if 0
int connect_timeout(int fd,const struct sockaddr*addr,socklen_t addrlen,struct timeval to){

    int ret;
    int val;
    socklen_t lon;
//1. make socket nonblock
    long flag=fcntl(fd,F_GETFL,NULL);
    CHK_EXPRe(flag<0,"Fcntl() Fail")
        return -1;
    END_CHK_EXPRe

    flag|=O_NONBLOCK;
    fcntl(fd,F_SETFL,flag);

//2. connect..
    ret=connect(fd,addr,addrlen);
    if(ret<0) {
        if(errno==EINPROGRESS){
            //need to wait

            fd_set wset;
            int maxfd;
            FD_ZERO(&wset);
            FD_SET(fd,&wset);
            maxfd=fd+1;
            ret=select(maxfd,NULL,&wset,NULL,&to);
            if(ret>0){
                lon=sizeof(val); 
                getsockopt(fd,SOL_SOCKET,SO_ERROR,&val,&lon) ;
                if(val){
                    fprintf(stderr,"Error in connect():%d:{%s}",val,strerror(errno));
                    return -2;
                }
            }else{
                return -4;//select failed
            }

        }else{
            return -3;//fail with other error;
        }

    }

    flag=fcntl(fd,F_GETFL,NULL);
    flag&=(~O_NONBLOCK);
    fcntl(fd,F_SETFL,flag);


    return 0;

}

#endif



rsm_httplistener_t*rsm_httplistener_new(const char*ipstr,int port)
{

    return_val_if_fail(port>0,NULL);

    rsm_httplistener_t*pr=calloc(1,sizeof(rsm_httplistener_t));


    int ret;

    if(!ipstr)
        ipstr="0.0.0.0";

    pr->polladdr.sin_family=AF_INET;
    ret=inet_pton(AF_INET,ipstr,&pr->polladdr.sin_addr);
    CHK_RUNe(ret<0,"inet_pton() Failed..",goto fail0);

    pr->polladdr.sin_port=htons(port);

    pr->pollfd=socket(PF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);
    CHK_RUNe(pr->pollfd<0,"Socket() Failed..",goto fail0);

    int val=1;
    ret=setsockopt(pr->pollfd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));
    CHK_EXPRe_ONLY(ret<0,"Set FD Reuseable.");
#if 1
    int flags;
    flags = fcntl(pr->pollfd, F_GETFL, NULL);
    CHK_EXPRe_ONLY(flags<0,"Get FD FL.");
    ret=fcntl(pr->pollfd,F_SETFL,flags|O_NONBLOCK);
    CHK_EXPRe_ONLY(ret<0,"Set FD Nonblock.");
#endif

#if 0
    struct timeval timeout={0,50000};//50ms
    setsockopt(sender->sock_fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof timeout);
#endif

    ret=bind(pr->pollfd,(struct sockaddr*)&pr->polladdr,sizeof(pr->polladdr));
    CHK_RUNe(ret<0,"Bind() Failed..",goto fail1);

    ret=listen(pr->pollfd,MAXQ);
    CHK_RUNe(ret<0,"Listen() Failed..",goto fail1);


    pr->allfds[0].fd=pr->pollfd;
    pr->allfds[0].events=POLLRDNORM;

    pr->maxfd=1;

    int i;
    for(i=1;i<MAXFD;i++){
        pr->allfds[i].fd=-1;
    }


    return pr;

fail1:
    close(pr->pollfd);

fail0:
    free(pr);

    return NULL;
}

void rsm_httplistener_free(rsm_httplistener_t*pr){

    return_if_fail(pr!=NULL);
    close(pr->allfds[0].fd);
    free(pr);

}


int rsm_httplistener_listen(rsm_httplistener_t*pr)
{
    int ret; 
    int nready;

    return_val_if_fail(pr!=NULL,-1);

    while(1){
        int i;
        int connfd;
        struct sockaddr_in cliaddr;
        socklen_t clilen=sizeof(cliaddr);


        nready=poll(pr->allfds,MAXFD,-1) ;   
        CHK_EXPRe(nready<0,"poll()  Continue..")
                continue;
        END_CHK_EXPRe
//            fprintf(stderr,"\n\033[32m Poll NReady:%d\033[0m\n",nready);

        if(pr->allfds[0].revents&POLLRDNORM){
            
            connfd=accept(pr->pollfd,(struct sockaddr*)&cliaddr,&clilen);
//            CHK_EXPRe(connfd<0,"accept().")
//                continue;
//            END_CHK_EXPRe
//            fprintf(stderr,"\033[32m-%d)accepted(%d)\033[0m\n",pr->pollfd,connfd);
            
            for(i=1;i<MAXFD;i++){
                if(pr->allfds[i].fd<0){
                    pr->allfds[i].fd=connfd;
                    pr->allfds[i].events=POLLRDNORM;
                    break;
                }
            }

            if(i>pr->maxfd)
                pr->maxfd=i;

            if(--nready<=0)
                continue;//no more requests;

        }

        int cfd;
        short cre;
        for(i=1;i<=pr->maxfd;i++){
            int nr;
            cfd=pr->allfds[i].fd;
            cre=pr->allfds[i].revents;
            if(cfd<0)
                continue;///skip
//            fprintf(stderr,"\t\033[32m :%d(%d)\t\033[0m",cfd,cre);

            if(cre&POLLNVAL){
//                fprintf(stderr,"XXXXXX closed..");
                pr->allfds[i].fd=-1;
                continue;
            }
#if 0
            if(cre&(POLLERR|POLLHUP)){
            
                fprintf(stderr,"XXXXXX Error..");
                close(cfd); 
                pr->allfds[i].fd=-1;
                continue;
            }
#endif
            if(cre&(POLLRDNORM|POLLERR)){
//            if(cre&(POLLRDNORM)){
//                fprintf(stderr,"\033[32m To Work\033[0m\n");
                ret=pr->worker_cb(cfd,pr->worker_data);
                if(ret==CONN_CLOSE){
                    close(cfd); 
                    pr->allfds[i].fd=-1;
                }

                if(--nready<=0)
                    break;//no more requests;

            }

        }

    }

    return 0;
}


void rsm_httplistener_set_worker(rsm_httplistener_t*pr,evtworker_cb cb,void*ud)
{
    return_if_fail(pr!=NULL);

    pr->worker_cb=cb;
    pr->worker_data=ud;

}

///////////////////////////////////////////////////////////


rsm_netlistener_t*rsm_netlistener_new(const char*ipstr,int port)
{

    return_val_if_fail(port>0,NULL);

    rsm_netlistener_t*pr=calloc(1,sizeof(rsm_netlistener_t));


    int ret;

    if(!ipstr)
        ipstr="0.0.0.0";

    pr->servaddr.sin_family=AF_INET;
    ret=inet_pton(AF_INET,ipstr,&pr->servaddr.sin_addr);
    CHK_RUNe(ret<0,"inet_pton() Failed..",goto fail0);

    pr->servaddr.sin_port=htons(port);

    pr->servfd=socket(PF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);
    CHK_RUNe(pr->servfd<0,"Socket() Failed..",goto fail0);

    int val=1;
    ret=setsockopt(pr->servfd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));
    CHK_EXPRe_ONLY(ret<0,"Set FD Reuseable.");

//    struct timeval timeout={0,500000};//0.5s
////    setsockopt(pr->servfd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof timeout);
//    setsockopt(pr->servfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof timeout);


    ret=bind(pr->servfd,(struct sockaddr*)&pr->servaddr,sizeof(pr->servaddr));
    CHK_RUNe(ret<0,"Bind() Failed..",goto fail1);


    ret=listen(pr->servfd,MAXQ);
    CHK_RUNe(ret<0,"Listen() Failed..",goto fail1);


    return pr;

fail1:
    close(pr->servfd);

fail0:
    free(pr);

    return NULL;
}



void rsm_netlistener_free(rsm_netlistener_t*pr){

    return_if_fail(pr!=NULL);
    close(pr->servfd);
    free(pr);

}


int rsm_netlistener_listen(rsm_netlistener_t*pr)
{
    int ret; 
    int nready;

    return_val_if_fail(pr!=NULL,-1);

    int i;
    int connfd;
    struct sockaddr_in cliaddr;
    socklen_t clilen=sizeof(cliaddr);

    while(1){
            
        connfd=accept(pr->servfd,(struct sockaddr*)&cliaddr,&clilen);
        CHK_EXPRe(connfd<0,"Accept() Failed")

            continue;
        END_CHK_EXPRe

        struct timeval timeout={0,150000};//150ms
//        setsockopt(connfd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof timeout);
        setsockopt(connfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof timeout);
    
//        fprintf(stderr,"[%p]:(%d)++++++++++++++++++++++++++++\n",pr,connfd);

        ret=pr->worker_cb(connfd,pr->worker_data);
        if(ret==CONN_CLOSE){
            close(connfd); 
        }
            
    }

    return 0;
}


void rsm_netlistener_set_worker(rsm_netlistener_t*pr,evtworker_cb cb,void*ud)
{
    return_if_fail(pr!=NULL);

    pr->worker_cb=cb;
    pr->worker_data=ud;

}


///////////////////////////////////////////////////////////





rsm_netconn_t*rsm_netconn_new(const char*ip,int port)
{

    int ret;
    return_val_if_fail((NULL!=ip && port>0),NULL);
    rsm_netconn_t*conn=calloc(1,sizeof(rsm_netconn_t));
    CHK_RUN(!conn,"Allocate EventNode.",
            return NULL);

    ret=inet_pton(AF_INET,ip,&conn->peer_addr.sin_addr);
    CHK_RUN(ret<=0,"INET_PTON",
            goto cleanup0);

    conn->peer_addr.sin_family=AF_INET;
    conn->peer_addr.sin_port=htons(port);
#if 0
    conn->conn_fd=socket(PF_INET,SOCK_STREAM,0);
    CHK_RUNe(conn->conn_fd<0,"Make Socket",
            goto cleanup0);

    struct timeval timeout={2,0};//3s
//    setsockopt(conn->conn_fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof timeout);
    setsockopt(conn->conn_fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof timeout);
#endif

    return conn;
cleanup0:
    free(conn);
    return NULL;

}

#if 0
int rsm_netconn_connect(rsm_netconn_t*conn)
{
    int ret=0; 
    return_val_if_fail(NULL!=conn,-1);

    ret=connect(conn->conn_fd,(struct sockaddr*)&conn->peer_addr,sizeof(conn->peer_addr));
    CHK_RUNe(ret<0,"Connect to Server..",
            ret=-2);
    return ret;

}

#endif


void rsm_netconn_free(rsm_netconn_t*conn)
{

    return_if_fail(NULL!=conn);

    close(conn->conn_fd);
    free(conn);

}


int rsm_netconn_shot(rsm_netconn_t*conn,void*buff,int len,void*obuff,int olen)
{
    int rv;
    return_val_if_fail(conn!=NULL&&buff!=NULL&&len>0,-1);
//    fprintf(stderr,"<<%d>>",conn->conn_fd);

#if 1
    conn->conn_fd=socket(PF_INET,SOCK_STREAM|SOCK_CLOEXEC,0);
    CHK_RUNe(conn->conn_fd<0,"Make Socket",
            rv=-5;
            goto fail0);


    struct timeval timeout={0,150000};//150ms
//    setsockopt(conn->conn_fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof timeout);
    setsockopt(conn->conn_fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof timeout);
#endif

    //TODO add timeout stuff for connect, through "select(),EINPROGRESS,O_NONBLOCK"
    rv=connect(conn->conn_fd,(struct sockaddr*)&conn->peer_addr,sizeof(conn->peer_addr));
    CHK_RUNe(rv<0,"Connect to Server..",
            rv=-2;
            goto fail);

    rv=send(conn->conn_fd,buff,len,MSG_DONTWAIT|MSG_NOSIGNAL);
    CHK_RUNe(rv<0,"Send to Server..",
            rv=-3;
            goto fail);

    rv=recv(conn->conn_fd,obuff,olen,0);
    CHK_RUNe(rv<0,"Recv from Server..",
            rv=-4;
            goto fail);


fail:
    close(conn->conn_fd);
fail0:
    return rv;
}





