#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<pwd.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>
#include<time.h>
#include<pulse/pulseaudio.h>
#include<pulse/simple.h>

#include"vdmHelper.h"
#include"circBuffer.h"
#include"avstub.h"
#include"avshm.h"

pa_simple *s;
pa_sample_spec ss;
#define AUDFRAG_SIZ 4608
static char buffer[AUDFRAG_SIZ];
static char wbuffer[9216];
static vdm_struct vdm_main;
static audbuff aud_buffer;




//calculate sink number`{1..8}` & userid`X{00..}` from index
static void index1(int index,int*osink,int*ouid,int pa_amount)
{

    int ret;
    int s;

    *ouid=(index-1)/pa_amount;
    s=index%pa_amount;
    if(s==0)
        s=pa_amount;
        
    *osink=s;

}




void pr_ts(const char*msg){
    
    struct timespec ts;                                                                                                                                            
    clock_gettime(CLOCK_REALTIME,&ts);
    
    fprintf(stderr,"\033[32m[%zu:%zu]\033[0m:%s",ts.tv_sec,ts.tv_nsec/1000000,msg);

}




static void*audio_fetch(void *arg){

    int index=*(int*)arg;
    int sink,uid;
    index1(index,&sink,&uid,8);
#if 0
    int wfd=open("cap.pcm",O_CREAT|O_TRUNC|O_WRONLY,0666);
    if(wfd<0){
        fprintf(stderr,"open cap.pcm{%s}\n",strerror(errno));
    }

#endif

    int nr,nw;
    int ret;
    uint64_t ts=0,lastts=0;

    while(1){


        ret=vdm_check_audiofragment(&vdm_main);
        if(ret==0){
            fprintf(stderr,"No Audio data Ready..\n");
            usleep(20000);
            continue;
        }
        audiofrag*af=NULL;
        videosurf*vs=NULL;

    
        if(ret>0){
    
            //FIXME only one way audio available..
            VDM_FOREACH_BEGIN(&vdm_main,vs,af)
                 
    retry:
                nr=AUDFRAG_SIZ;
                ret=shmaudio_get(af,buffer,&nr,&ts);
    
               // fprintf(stderr,"ts:%"PRIu64" lastts:%"PRIu64" ..nr=%d \n",ts,lastts,nr);
               //
                if(ts-lastts>1){
                    fprintf(stderr,"audio(%d): ts check error(ts:%"PRIu64",lastts:%"PRIu64")..\n",_i,ts,lastts);
//                    usleep(1000);
//                    goto retry;
                }else if(ts-lastts==0){
                    usleep(15000);
                    goto retry;
                }

                lastts=ts;
                //fprintf(stderr,"write audio[%d:%p,%d]..\n",wfd,buffer,nr);
//                write(wfd,buffer,nr);
                fprintf(stderr,"index:%d(uid:%d,sink:%d) - cached ts:%"PRIu64"  ..nr=%d audspace:%d \n",index,uid,sink,ts,nr,audbuff_space(&aud_buffer));
                audbuff_put(&aud_buffer,buffer,nr);
                
            VDM_FOREACH_END
    
    
        }else{

            fprintf(stderr,"fill null audio data...\n");
            memset(buffer,0,sizeof buffer);
            nr=sizeof buffer;
        }


    }

    return NULL;
}

int main(int argc,char**argv)
{

    if(argc<2)
        exit(-1);

    int index=atoi(argv[1]);
    int sink;
    int uid;

    int err;

    char aud_name[128];

    pthread_t audfetchid;

    vdm_init(&vdm_main,index);

    audbuff_init(&aud_buffer,0x80000);


//    fprintf(stderr,"AudioDispatcher(%d)[%s_%s]",argc,argv[0],argv[1]);


    index1(index,&sink,&uid,8);

    snprintf(aud_name,sizeof aud_name,"audDispatcher.%d(uid:%d,sink:%d)",index,uid,sink);

    ss.format = PA_SAMPLE_S16NE;
    ss.channels = 2;
    ss.rate = 48000;

    s = pa_simple_new(NULL,aud_name,PA_STREAM_PLAYBACK,NULL,"dispatch audio for VDM",&ss,NULL,NULL,NULL);
    if(!s){
        fprintf(stderr,"pa_simple_new() failed :%s\n",strerror(errno));
        return -1;
    }
#if 0
    int pfd=open("cappa.pcm",O_CREAT|O_TRUNC|O_WRONLY,0666);
    if(pfd<0){
        fprintf(stderr,"open cap.pcm{%s}\n",strerror(errno));
    }

#endif


//    fprintf(stderr,"Latency is:%ld\n",pa_simple_get_latency(s,NULL));

    pthread_create(&audfetchid,NULL,audio_fetch,&index);
    
    //let audio_fetch run first...
//    usleep(50000);
    int nr,nw;
    int ret;
    int bsleep=0;
    uint64_t ts=0,lastts=0;
    while(1){


        nr=audbuff_get(&aud_buffer,wbuffer,sizeof wbuffer);
        
//        fprintf(stderr,"get audbuff %d..\n",nr);
        if(nr==0){
            usleep(5000);
            continue;
        }

        if(nr<sizeof buffer){
            fprintf(stderr,"get faster than put..\n");
            bsleep=1;
        }
       // pr_ts("Before write audio\n");

        nw=pa_simple_write(s,wbuffer,nr,&err);
        if(nw<0){
            fprintf(stderr, " pa_simple_write(%d) failed: %s\n",nr, pa_strerror(err));
            goto finish;
        }

       // pr_ts("After write audio\n");
        fprintf(stderr,"pa_simple_write(%d)\n",nr);
//        usleep(20000);
#if 0
        ret=write(pfd,wbuffer,nr);
        if(ret<0){
            fprintf(stderr,"save pcm{%s}\n",strerror(errno));
        }
#endif
        if(bsleep){
            bsleep=0; 
            usleep(10000);
        }
    }

    if (pa_simple_drain(s, &err) < 0) {
        fprintf(stderr, " pa_simple_drain() failed: %s\n", pa_strerror(err));
        goto finish;
                            
    }


finish:
    pa_simple_free(s);


    return 0;
}


