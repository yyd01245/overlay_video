#ifndef _H_AUDIOVIDEO_STUB_
#define _H_AUDIOVIDEO_STUB_


#include<sys/types.h>
//////////////////////////

#define MAX_VNC_NUM 128

#define MAX_AV_NUM 2

typedef struct _area{
    short x;
    short y; 
    int shmid;//videosurf
    int shmid_aud;//audiofrag
}area;

typedef struct _avstub{
    int on;
    area areas[MAX_AV_NUM];
}avstub;




static int videostub_set_resource(avstub*stub,int shmid,int x,int y,int shmida)
{
    int i;
    for(i=0;i<MAX_AV_NUM;i++){
        if(stub->areas[i].shmid==0)
            break;
    }

    if(i!=MAX_AV_NUM){
        stub->on++;
    
        stub->areas[i].x=x;
        stub->areas[i].y=y;
        stub->areas[i].shmid=shmid;
        stub->areas[i].shmid_aud=shmida;

        return i;
    }

    return -1;
}



static int videostub_unset_resource(avstub*stub,int shmid,int x,int y)
{
    int i;
    for(i=0;i<MAX_AV_NUM;i++){
        if(stub->areas[i].shmid==shmid &&
            stub->areas[i].x==x &&
            stub->areas[i].y==y )
            break;
    }

    if(i!=MAX_AV_NUM){
        stub->on--;
        stub->areas[i].shmid=0;
        stub->areas[i].shmid_aud=0;

        return i;
    }

    return -1;
}



#endif
