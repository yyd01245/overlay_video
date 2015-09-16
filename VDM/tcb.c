#ifndef _H_CIRC_BUFFER_
#define _H_CIRC_BUFFER_

//#include"circ_buf.h"
/*
 * See Documentation/circular-buffers.txt for more information.
 */
#ifndef _LINUX_CIRC_BUF_H
#define _LINUX_CIRC_BUF_H 1

struct circ_buf {
	char *buf;
	int head;
	int tail;
};

/* Return count in buffer.  */
#define CIRC_CNT(head,tail,size) (((head) - (tail)) & ((size)-1))

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has head == tail, which is the same as
   empty.  */
#define CIRC_SPACE(head,tail,size) CIRC_CNT((tail),((head)+1),(size))



#define CIRC_ADD(pos,inc,size) ((pos+inc)&((size)-1))



/* Return count up to the end of the buffer.  Carefully avoid
   accessing head and tail more than once, so they can change
   underneath us without returning inconsistent results.  */
#define CIRC_CNT_TO_END(head,tail,size) \
	({int end = (size) - (tail); \
	  int n = ((head) + end) & ((size)-1); \
	  n < end ? n : end;})

/* Return space available up to the end of the buffer.  */
#define CIRC_SPACE_TO_END(head,tail,size) \
	({int end = (size) - 1 - (head); \
	  int n = (end + (tail)) & ((size)-1); \
	  n <= end ? n : end+1;})

#endif /* _LINUX_CIRC_BUF_H  */




//#include<unistd.h>
//#include<time.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<pthread.h>
//#include<string.h>

typedef struct _audbuff{

    pthread_mutex_t lock;
    struct circ_buf cb;
    int capability;
    uint8_t *data;
}audbuff;

int audbuff_init(audbuff*ab,int cap){

    if((cap&(cap-1))!=0)
        return -1;
    ab->cb.buf=calloc(cap,1);
    ab->data=ab->cb.buf;

    ab->capability=cap;
    ab->cb.head=0;
    ab->cb.tail=0;

    pthread_mutex_init(&ab->lock,NULL);


    return 0;

}

void audbuff_fini(audbuff*ab){
    if(!ab)
        return;
    free(ab->data);

}



void audbuff_put(audbuff*ab,void*data,int len){
    
    void*p=data;
    int asiz;
    pthread_mutex_lock(&ab->lock);
    while(len){

        asiz=CIRC_SPACE_TO_END(ab->cb.head,ab->cb.tail,ab->capability);
//        fprintf(stderr,"space_to_end:%d\n",asiz);
        if(asiz>0){

            asiz=asiz<len?asiz:len;
            memcpy(ab->data+ab->cb.head,p,asiz);
            ab->cb.head=CIRC_ADD(ab->cb.head,asiz,ab->capability);
            len-=asiz;
            p+=asiz;
        }else{//asiz==0,no more space
            asiz=CIRC_CNT_TO_END(ab->cb.head,ab->cb.tail,ab->capability);
            fprintf(stderr,"overwrite:%d\n",asiz);
            int bsiz=asiz<len?asiz:len;
            ab->cb.tail=CIRC_ADD(ab->cb.tail,bsiz,ab->capability);
        }

//        usleep(500000);
    }

    pthread_mutex_unlock(&ab->lock);
}


int audbuff_get(audbuff*ab,void*data,int len){
    
    void*p=data;
    int asiz;
    int glen=len;

    pthread_mutex_lock(&ab->lock);
    while(len){

        asiz=CIRC_CNT_TO_END(ab->cb.head,ab->cb.tail,ab->capability);
//        fprintf(stderr,"cnt_to_end:%d\n",asiz);
        if(asiz>0){

            asiz=asiz<len?asiz:len;
            memcpy(p,ab->data+ab->cb.tail,asiz);
            ab->cb.tail=CIRC_ADD(ab->cb.tail,asiz,ab->capability);
            len-=asiz;
            p+=asiz;
        }else{//asiz==0,no more space

            pthread_mutex_unlock(&ab->lock);
            fprintf(stderr,"get less:\n");
            return glen-len;
        }

    }
    pthread_mutex_unlock(&ab->lock);
    return glen;
}

#if 1

static int make_nums(char*nums,int len,int startval){
    int i;
    int n;
    for(i=0;i<len;i++){

        n='a'+(startval++)%26;

        nums[i]=n;
    }

    return startval;
}

static void print_buff(char*buff,int len){

    int i;
    for(i=0;i<len;i++)
        fprintf(stderr,"%c",buff[i]);
    fprintf(stderr,"\n");

}

static void print_audbuff(audbuff*ab){


    int i;
    for(i=0;i<ab->capability;i++)
        fprintf(stderr,"%c",ab->data[i]);

    fprintf(stderr,"\n");

    for(i=0;i<ab->capability;i++){


        if(ab->cb.tail==i&& ab->cb.head==i){
            
            fprintf(stderr,"X");
        }else
        if(ab->cb.tail==i){
            fprintf(stderr,"T");
        }else if(ab->cb.head==i){
            fprintf(stderr,"H");
        }else{
            fprintf(stderr," ");
        }
    }

    fprintf(stderr,"\n");
}


int main(int argc,char**argv)
{
    
    srand(time(NULL));
    audbuff ab;

    int ret=audbuff_init(&ab,32);
    if(ret<0){

        fprintf(stderr,"audbuff_init failed\n");
        return -1;
    }


    int i=0;
    int wl,rl;

    int val=0;

    char nums[32];
    char dumm[32];

    while(1){


        wl=(rand()%6)+1;

        if((wl%2==0) && (i%2==0)){
            rl=wl;
            fprintf(stderr,"G(%d)\n",rl);
            int nn=audbuff_get(&ab,dumm,rl);
            print_buff(dumm,nn);
        }else{

            val=make_nums(nums,wl,val);

            fprintf(stderr,"P(%d)\n",wl);
            audbuff_put(&ab,nums,wl);
    
        }
        
        print_audbuff(&ab);

        usleep(500000);
        i++;
    }

    return 0;
}

#endif

#endif
