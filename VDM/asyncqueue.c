#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include"asyncqueue.h"
#include"check.h"

#include<unistd.h>
#include<signal.h>


/**
 *
 */

static que_nd_t*new_queue_node(void*data)
{
    que_nd_t*nd=(que_nd_t*)calloc(1,sizeof(que_nd_t));
    _CHK_BEGIN(nd==NULL,"New Queue Node faile")
        return NULL;
    _CHK_END

    nd->carrier=data;

    return nd;
}


static void free_queue_node(que_nd_t*node)
{
    return_if_fail(node!=NULL);
    free(node);
}



// statically initialize a object
int async_que_init(async_que_t*que)
{
    return_val_if_fail(que!=NULL,-1);

    memset(que,0,sizeof(async_que_t));

    pthread_mutex_init(&que->lock,NULL);
    pthread_cond_init(&que->notify,NULL);

    INIT_LIST_HEAD(&que->qhead);


    return 0;
}



async_que_t*async_que_new()
{

    async_que_t*que=(async_que_t*)calloc(1,sizeof(async_que_t));
    _CHK_BEGIN_error(que==NULL,"Allocate Queue.")
        return NULL;
    _CHK_END

    async_que_init(que);
    
    return que;
}


//int async_que_enque_full(async_que_t*que,void*ptr,clear_cb callback,void*cbarg )
//
int async_que_enque(async_que_t*que,void*ptr)
{
 
    return_val_if_fail(que!=NULL,-1);
    return_val_if_fail(ptr!=NULL,-2);

   
    que_nd_t*nd=new_queue_node(ptr);
    _CHK_BEGIN_error(nd==NULL,"New Que Node.")
        return -4;
    _CHK_END

    pthread_mutex_lock(&que->lock);
    list_add_tail(&que->qhead,&nd->list);
    que->size++;
    pthread_mutex_unlock(&que->lock);
    pthread_cond_signal(&que->notify);
    
    return 0;

}




int async_que_deque(async_que_t*que,void**pptr)
{
    return_val_if_fail(que!=NULL,-1);
    return_val_if_fail(pptr!=NULL,-2);
    
    pthread_mutex_lock(&que->lock);
    while(list_empty(&que->qhead)){

        pthread_cond_wait(&que->notify,&que->lock);
    }
    
    struct list_head* pos=que->qhead.next;
    que_nd_t*qnd=list_entry(pos,que_nd_t,list);

    list_del(pos);
    que->size--;
  

    pthread_mutex_unlock(&que->lock);

    *pptr=qnd->carrier;

    free_queue_node(qnd);

    return 0;

}




static void async_que_print(async_que_t*que)
{

    if(list_empty(&que->qhead))
        return;

    que_nd_t*qnd;

    list_for_each_entry(qnd,&que->qhead,list){

        printf("(%d)>>{%s}<<\n",que->size,(char*)qnd->carrier);
    }

}






int async_que_clear(async_que_t*que)
{
 
    return_val_if_fail(que!=NULL,-1);

    que_nd_t* pos;
    que_nd_t* savpos;

    pthread_mutex_lock(&que->lock);

    list_for_each_entry_safe(pos, savpos, &que->qhead, list){
       
        list_del(&pos->list);
        free_queue_node(pos);
        que->size--;
    }

    pthread_mutex_unlock(&que->lock);

    return 0;
}




void async_que_fini(async_que_t*que)
{
   
    int ret;

    ret=async_que_clear(que);

    pthread_mutex_destroy(&que->lock);
    pthread_cond_destroy(&que->notify);

}


int async_que_free(async_que_t*que)
{
    int ret=0;
    return_val_if_fail(que!=NULL,-1);

    async_que_fini(que);

    free(que);

    return ret;
}




