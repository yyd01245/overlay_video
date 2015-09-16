#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<math.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<fcntl.h>
#include<pthread.h>


int vidindex;
int decode_width=0;
int decode_height=0;

key_t shmkey=0;
int shmid=0;
void *video_data;
int video_size;

#include"avstub.h"
#include"avshm.h"




void make_audio(int sfd,void*p,int len){

    read(sfd,p,len);
}

void make_image(void*p,int w,int h){
 
    int y_siz=w*h;
    int u_siz=w*h/4;
    int v_siz=w*h/4;
    static int val=0xe;
    static int cu=30;
    static int cv=60;
    int i;
    char*y=p;
    char*u=y+y_siz;
    char*v=u+u_siz;

        for(i=0;i<y_siz;i++){
//            y[i]=val; 
            y[i]=val++; 
        }

        for(i=0;i<u_siz;i++){
//            u[i]=(val+i)*cos(cu); 
            u[i]=0x80+i;
        }

        for(i=0;i<v_siz;i++){
//            v[i]=(val+i)*sin(cv); 
            v[i]=0x80+i;
        }
    cu+=10;
    cv+=3;
    val++;
//    fprintf(stderr,"val:%d:%d:%d\n",val,cu,cv);
}



void * send_audio(void*arg){

    int afd=(*(int *)arg);

    int audsrc=open("/dev/urandom",O_RDONLY);

    char buff[1024];

    while(1){
        int nr=read(audsrc,buff,sizeof buff);
        write(afd,buff,nr);
    }

    return NULL;
}


int main(int argc,char**argv)
{

    int i;

    char*audio_path;

    for(i=1;i<argc;i++){
        if(!strcmp(argv[i],"--videokey")){
            shmkey=atoi(argv[i+1]);
            break;
        }
    }

    for(i=1;i<argc;i++){
        if(!strcmp(argv[i],"--width")){
            decode_width=atoi(argv[i+1]);
            break;
        }
    }

    for(i=1;i<argc;i++){
        if(!strcmp(argv[i],"--height")){
            decode_height=atoi(argv[i+1]);
            break;
        }
    }


    for(i=1;i<argc;i++){
        if(!strcmp(argv[i],"--index")){
            vidindex=atoi(argv[i+1]);
            break;
        }
    }

    for(i=1;i<argc;i++){
        if(!strcmp(argv[i],"--audiofile")){
            audio_path=argv[i+1];
            break;
        }
    }

    int audfd=open(audio_path,O_CREAT|O_WRONLY,0666);
    if(audfd<0){
        fprintf(stderr,"Open audio file Failed...\n");
    }

    videosurf* vidsurf;
    video_size=decode_width*decode_height*3/2;
    video_data=malloc(video_size);


    fprintf(stderr,"Shmkey:[%d] %dx%d\n",shmkey,decode_width,decode_height);

    if(shmkey==0){
        fprintf(stderr,"No shmkey specified.\n");
        return -1;
    }

    shmid=shmget(shmkey,video_size+sizeof(videosurf),IPC_CREAT|0666);
    if(shmid<0){
        fprintf(stderr,"shmget() Failed..\n");
        return -2;
    }

    vidsurf=shmat(shmid,NULL,0);

    
    char abuff[2048];

    pthread_t audtid;

    pthread_create(&audtid,NULL,send_audio,&audfd);

    while(1){
        

        make_image(video_data,decode_width,decode_height);
        shmvideo_put(vidsurf,decode_width,decode_height,video_data,video_size);

//        make_audio(audsrc,abuff,sizeof abuff);
//        write(audfd,abuff,sizeof  abuff);

        usleep(39000);
    }

    
    return 0;

}



