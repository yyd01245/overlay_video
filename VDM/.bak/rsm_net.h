
#ifndef _H_NET_VDM_
#define _H_NET_VDM_


#define MAXQ 10

#define MAXFD 100




#include<poll.h>
#include<sys/select.h>
#include<sys/socket.h>

#include<netinet/in.h>

#define CONN_ONCE 0
#define CONN_KEEP 1
#define CONN_FAIL -1
#define CONN_CLOSE -1

/*
 * FOR TCP
 *  return:
 *      0: Success, Connect Once,OR, Should be closed,After process.
 *      1: Success, not close.
 *     -1: Failed, Should close fd.
 * 
 */
typedef int (*evtworker_cb)(int fd,void*data);

typedef struct _poller{


    struct sockaddr_in polladdr;
    int pollfd;

    struct pollfd allfds[MAXFD];
    int maxfd; 

//    void*recv_buff;
//    size_t recv_buffsiz;

    evtworker_cb worker_cb;
    void*worker_data;

//    struct list_head clihead;

}rsm_httplistener_t;




typedef struct _listener{


    struct sockaddr_in servaddr;
    int servfd;


    evtworker_cb worker_cb;
    void*worker_data;


}rsm_netlistener_t;



struct _rsm_conn_event{
    
    int conn_fd;
    struct sockaddr_in peer_addr;
//    evtworker_cb worker_cb;
//    void* worker_data;
};

typedef struct _rsm_conn_event rsm_netconn_t;




rsm_netconn_t*rsm_netconn_new(const char*ip,int port);
//int rsm_netconn_reconnect(rsm_netconn_t*conn);

void rsm_netconn_free(rsm_netconn_t*conn);

int rsm_netconn_shot(rsm_netconn_t*conn,void*buff,int len,void*obuff,int olen);



rsm_netlistener_t*rsm_netlistener_new(const char*ip,int port);
void rsm_netlistener_free(rsm_netlistener_t*evt);
void rsm_netlistener_set_worker(rsm_netlistener_t*pr,evtworker_cb cb,void*ud);
int rsm_netlistener_listen(rsm_netlistener_t*evt);


rsm_httplistener_t*rsm_httplistener_new(const char*ip,int port);
void rsm_httplistener_free(rsm_httplistener_t*evt);
void rsm_httplistener_set_worker(rsm_httplistener_t*pr,evtworker_cb cb,void*ud);
int rsm_httplistener_listen(rsm_httplistener_t*evt);






#endif
