#include<stdio.h>

#include<unistd.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/types.h>

#include<stdlib.h>


typedef struct _va{

//    unsigned char on;
    _Bool on;
    int x,y;
    int id;

}va;


int main(int argc,char**argv)
{

    int shmid=atoi(argv[1]);


    void*p=shmat(shmid,NULL,0);

    if(p==(void*)-1){
        fprintf(stderr,"shmat() Failed..\n");
    }

    va*pv=p;

    int i;
    for(i=0;i<256;i++){
        
        fprintf(stderr,"on:%d, [%dx%d] id:%d\n",pv->on,pv->x,pv->y,pv->id);
        pv++;
    }

}
