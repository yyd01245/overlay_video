#include"circ_buf.h"

#include<unistd.h>
#include<time.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>

typedef struct _audbuff{

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
    while(len){

        asiz=CIRC_SPACE_TO_END(ab->cb.head,ab->cb.tail,ab->capability);
        fprintf(stderr,"space_to_end:%d\n",asiz);
        if(asiz>0){

            asiz=asiz<len?asiz:len;
            memcpy(ab->data+ab->cb.head,p,asiz);
            ab->cb.head=CIRC_ADD(ab->cb.head,asiz,ab->capability);
            len-=asiz;
            p+=asiz;
        }else{//asiz==0,no more space
            asiz=CIRC_CNT_TO_END(ab->cb.head,ab->cb.tail,ab->capability);
            int bsiz=asiz<len?asiz:len;
            ab->cb.tail=CIRC_ADD(ab->cb.tail,bsiz,ab->capability);
        }

        usleep(500000);
    }

}


int audbuff_get(audbuff*ab,void*data,int len){
    
    void*p=data;
    int asiz;
    int glen=len;
    while(len){

        asiz=CIRC_CNT_TO_END(ab->cb.head,ab->cb.tail,ab->capability);
        fprintf(stderr,"cnt_to_end:%d\n",asiz);
        if(asiz>0){

            asiz=asiz<len?asiz:len;
            memcpy(p,ab->data+ab->cb.tail,asiz);
            ab->cb.tail=CIRC_ADD(ab->cb.tail,asiz,ab->capability);
            len-=asiz;
            p+=asiz;
        }else{//asiz==0,no more space
            return glen-len;
        }

    }

}



static int make_nums(char*nums,int len,int startval){
    int i;
    int n;
    for(i=0;i<len;i++){

        n='a'+(startval++)%26;

        nums[i]=n;
    }

    return startval;
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


        wl=(rand()%64)+1;

        if((wl%2==0) && (i%2==0)){
            rl=wl;
            fprintf(stderr,"G(%2d).",rl);
            audbuff_get(&ab,dumm,rl);
        }else{

            val=make_nums(nums,wl,val);

            fprintf(stderr,"P(%2d).",wl);
            audbuff_put(&ab,nums,wl);
    
        }
        
        print_audbuff(&ab);

        usleep(500000);
        i++;
    }















    return 0;
}

