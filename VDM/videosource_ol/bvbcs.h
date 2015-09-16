#ifndef _H_BVBCS_

#define _H_BVBCS_
#include<unistd.h>

#define SHMHDR_CLEAR 0
#define SHMHDR_DIRTY 1


struct _shm_head{
//should never access directly 
    int count;//count after last clear oper
    int flag;//indicate state
#ifdef _ENABLE_LOCK
    int lock;//for interaction
#endif
    int x;
    int y;//top left
    int x1;
    int y1;//bottom right
    int w;
    int h;
    
};

typedef struct _shm_head bvbcs_shmhdr;

static inline void shmhdr_clear(bvbcs_shmhdr*hdr){
    hdr->flag = SHMHDR_CLEAR;
    hdr->count=0;
    hdr->x  =hdr->y  =0;
    hdr->x1 =hdr->y1 =0;
    hdr->w  =hdr->h  =0;
}

static inline int shmhdr_get_extents(bvbcs_shmhdr*hdr,int*x,int*y,int*w,int*h,int*x1,int*y1){
//LOCK
    int ret=0;
#ifdef _ENABLE_LOCK
    while(!__sync_bool_compare_and_swap(&hdr->lock,0,1))
        usleep(1000);
#endif
    
    if(hdr->count==0)//no update occured.
        goto skip;

    if(x)
        *x=hdr->x;
    if(y)
        *y=hdr->y;
    if(x1)
        *x1=hdr->x1;
    if(y1)
        *y1=hdr->y1;
    
    if(w)
        *w=hdr->w;
    if(h)
        *h=hdr->h;
    ret=1;
    shmhdr_clear(hdr);

skip:
#ifdef _ENABLE_LOCK
//UNLOCK
    hdr->lock=0;
    __sync_synchronize();
#endif
    return ret;
}



#endif //_H_BVBCS_
