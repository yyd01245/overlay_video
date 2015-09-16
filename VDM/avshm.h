#ifndef _H_AUDIOVIDEO_SHM__
#define _H_AUDIOVIDEO_SHM__

#include<stdint.h>
#include<unistd.h>
#include<string.h>

typedef struct _audiofrag{

    int flag;
    int len;
    uint64_t ts;
    unsigned char data[0];
    
}audiofrag;


typedef struct _videosurf{

    int flag;
    int width,height;
//    int len;
    unsigned char data[0];
    
}videosurf;



static inline int shmvideo_put(videosurf*vs,int w,int h,unsigned char*yuv,int len){

    if(len<=0)
        return -1;

    int retry=10;

    while(!__sync_bool_compare_and_swap(&vs->flag,0,1) && retry){
        retry--;
        usleep(5000);
    }

    if(!retry)//times up
        return -2;

    vs->width=w;
    vs->height=h;
    memcpy(vs->data,yuv,len);

    vs->flag=0;
    __sync_synchronize();


    return 0;
}


static inline int shmvideo_get(videosurf*vs,int*w,int*h,unsigned char*oyuv,int*len){

    if(*len<=0)
        return -1;

    int retry=10;

    while(!__sync_bool_compare_and_swap(&vs->flag,0,1) && retry){
        retry--;
        usleep(5000);
    }

    if(!retry)//times up
        return -2;

    *w=vs->width;
    *h=vs->height;
    int newlen=vs->width*vs->height*3/2;//yuv420
    if(*len<newlen)
        newlen=*len;

    memcpy(oyuv,vs->data,newlen);
    *len=newlen;

    vs->flag=0;
    __sync_synchronize();


    return 0;

}



static inline int shmaudio_put(audiofrag*af,unsigned char*audf,int len,uint64_t ts){

    if(len<=0)
        return -1;

    int retry=10;

    while(!__sync_bool_compare_and_swap(&af->flag,0,1) && retry){
        retry--;
        usleep(2000);
    }
    if(!retry)
        return -2;


    memcpy(af->data,audf,len);

    af->flag=0;
    af->ts=ts;
    af->len=len;
    __sync_synchronize();


    return 0;
}



static inline int shmaudio_get(audiofrag*af,unsigned char*audf,int*len,uint64_t*ots){

    if(*len<=0)
        return -1;

    int retry=10;

    while(!__sync_bool_compare_and_swap(&af->flag,0,1) && retry){
        retry--;
        usleep(2000);
    }
    if(!retry)
        return -2;

    int newlen=af->len;
    if(*len<newlen)
        newlen=*len;

    memcpy(audf,af->data,newlen);
    *len=newlen;
    *ots=af->ts;
    af->flag=0;
    __sync_synchronize();


    return 0;

}


static inline int shmaudio_check_ts(audiofrag*af,uint64_t ts){

    if(af->ts!=ts)
        return 0;
    return 1;
}



#endif //__SHM_AUDIO_H__

