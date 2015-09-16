#ifndef _H_ASYNC_QUEUE_
#define _H_ASYNC_QUEUE_


#include"list.h"
#include<pthread.h>

typedef struct _async_q{

    struct list_head qhead;

    pthread_mutex_t lock;
    pthread_cond_t notify;

    int size;

}async_que_t;



typedef struct _que_nd que_nd_t;



//typedef void (*clear_cb)(que_nd_t*nd,void*);

struct _que_nd{
    void*carrier;
    struct list_head list;
};




async_que_t*async_que_new();
int async_que_free(async_que_t*);

int async_que_init(async_que_t*);
void async_que_fini(async_que_t*);

int async_que_enque(async_que_t*,void*);
int async_que_deque(async_que_t*,void**);

int async_que_clear(async_que_t*que);












#endif
