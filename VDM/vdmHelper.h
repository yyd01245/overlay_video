#ifndef _H_VDM_HELPER_
#define _H_VDM_HELPER_

#include"avshm.h"
#include"avstub.h"

#ifndef AVSTUB_KEY
#define AVSTUB_KEY 0xfefe1010
#endif

/////////////////////////////////////////
typedef struct _box16{
    int16_t x1,y1,x2,y2;
}box_t;



typedef struct _vdmc{

    int shmid;
    int shmsize;
    int nrecord;

    avstub*avs;

    //on or off
    int status[MAX_AV_NUM];

//video resource
    int savshmidvid[MAX_AV_NUM];
    videosurf*vidsurf[MAX_AV_NUM];
    box_t rects[MAX_AV_NUM];

//audio resource
    int savshmidaud[MAX_AV_NUM];
    audiofrag*audfrag[MAX_AV_NUM];

    int on,last_on;

}vdm_struct;


#define VDM_FOREACH_BEGIN(vdm,vs,af) \
    int _on=(vdm)->on;\
    int _i;\
    for(_i=0;_i<MAX_AV_NUM&&_on;_i++){\
        if((vdm)->status[_i]==0){continue;}\
        _on--;\
        vs=(vdm)->vidsurf[_i];\
        af=(vdm)->audfrag[_i];\
//        area*va=&(vdm)->avs->areas[_i];



#define VDM_FOREACH_END }
   

#include<stdio.h>
#include<string.h>
#include<sys/ipc.h>
#include<sys/shm.h>

static void vdm_init(vdm_struct*vdm,int index)
{

    memset(vdm,0,sizeof *vdm);

    vdm->shmsize=sizeof(avstub)*MAX_VNC_NUM;
    vdm->nrecord=MAX_VNC_NUM;

    vdm->shmid=shmget(AVSTUB_KEY,vdm->shmsize,IPC_CREAT|0666);

    vdm->avs=((avstub*)shmat(vdm->shmid,NULL,0))+index;

    fprintf(stderr,"index:%d, shmid:%d\n",index,vdm->shmid);


}

static void vdm_fini(vdm_struct*vdm)
{
    shmdt(vdm->avs);
    shmctl(vdm->shmid,IPC_RMID,NULL);
}


static int vdm_check_videosurface(vdm_struct*vdm)
{
    //save 
    vdm->on=vdm->avs->on;
    if(!vdm->on){
        //no video area
        if(vdm->on!=vdm->last_on){
            //full flush 
            vdm->last_on=vdm->on;//0
            return -1;
        }
//        fprintf(stderr,"Skip check_videoarea!!\n");
        return 0;
    }

    
    int on=vdm->on;
    memset(&vdm->rects,0,sizeof(box_t)*MAX_AV_NUM);

    int i;
    for(i=0;i<MAX_AV_NUM&&on;i++){

        area*va=&vdm->avs->areas[i];
        if(va->shmid==0){
            vdm->status[i]=0;
            fprintf(stderr,"skip area[%d]\n",i);
            continue;
        }
        vdm->status[i]=1;
        on--;

        videosurf*vf=vdm->vidsurf[i];    

//        fprintf(stderr,"area[%d] va:%p\n",i,va);
        if(va->shmid!=vdm->savshmidvid[i]){
            fprintf(stderr,":shmid_vid changed:\n");
            if(vf)
                shmdt(vf);

            vdm->savshmidvid[i]=va->shmid;
            vdm->vidsurf[i]=(videosurf*)shmat(va->shmid,NULL,0);
            vf=vdm->vidsurf[i];
            fprintf(stderr,"vdm->vidsurf[%d]=%p\n",i,vf);
        }


        vdm->rects[i].x1=va->x;
        vdm->rects[i].x2=vf->width+vdm->rects[i].x1;
        vdm->rects[i].y1=va->y;
        vdm->rects[i].y2=vf->height+vdm->rects[i].y1;

    }
/*
    fprintf(stderr,".. x:%d,y:%d,W:%d,H:%d\n",vdm_main.rect.x0,vdm_main.rect.y0,
            vdm_main.rect.w,vdm_main.rect.h);
*/

    if(vdm->on!=vdm->last_on){
        vdm->last_on=vdm->on;
        return -2;//new /delete videoarea
    }
    return vdm->on;

}




static int vdm_check_audiofragment(vdm_struct*vdm)
{
    
    vdm->on=vdm->avs->on;
    if(!vdm->on){
//        fprintf(stderr,"Skip check_videoarea!!\n");
        return 0;
    }
    
    int on=vdm->on;
 
    int i;
    for(i=0;i<MAX_AV_NUM&&on;i++){

        area*va=&vdm->avs->areas[i];

        if(va->shmid_aud==0){
            vdm->status[i]=0;
            fprintf(stderr,"skip area[%d]\n",i);
            continue;
        }
        vdm->status[i]=1;
        on--;


        audiofrag*af=vdm->audfrag[i];    
//        fprintf(stderr,"area[%d] va:%p\n",i,va);
        if(va->shmid_aud!=vdm->savshmidaud[i]){
            fprintf(stderr,":shmid_aud __+++changed:\n");
            if(af)
                shmdt(af);

            fprintf(stderr,":-------:0\n");
            vdm->savshmidaud[i]=va->shmid_aud;
            fprintf(stderr,":-------:1\n");
            vdm->audfrag[i]=(audiofrag*)shmat(va->shmid_aud,NULL,0);
            fprintf(stderr,":-------:2\n");
            fprintf(stderr,"vdm->audfrag[%d]=%p\n",i,vdm->audfrag[i]);
//            af=vdm->audfrag[i];
        }

    }
/*
    if(vdm->on!=vdm->last_on){
        vdm->last_on=vdm->on;
        return -2;//new /delete videoarea
    }
*/
    return vdm->on;

}





#endif
