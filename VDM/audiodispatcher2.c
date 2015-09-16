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

#include"vdmHelper.h"
#include"circBuffer.h"
#include"avstub.h"
#include"avshm.h"

#define AUDFRAG_SIZ 4608
static char buffer[AUDFRAG_SIZ];
static char wbuffer[9216];
static vdm_struct vdm_main;
static audbuff aud_buffer;

static int latency = 80000; // start latency in micro seconds
static int sampleoffs = 0;
static pa_buffer_attr bufattr;
static int underflows = 0;
static pa_sample_spec ss;

void pa_state_cb(pa_context *c, void *userdata) {
  pa_context_state_t state;
  int *pa_ready = userdata;
  state = pa_context_get_state(c);
  switch  (state) {
    // These are just here for reference
  case PA_CONTEXT_UNCONNECTED:
  case PA_CONTEXT_CONNECTING:
  case PA_CONTEXT_AUTHORIZING:
  case PA_CONTEXT_SETTING_NAME:
  default:
    break;
  case PA_CONTEXT_FAILED:
  case PA_CONTEXT_TERMINATED:
    *pa_ready = 2;
    break;
  case PA_CONTEXT_READY:
    *pa_ready = 1;
    break;
  }
}



static void stream_request_cb(pa_stream *s, size_t length, void *userdata) {
  pa_usec_t usec;
  int neg,nr;

  pa_stream_get_latency(s,&usec,&neg);
  fprintf(stderr,"  latency %8d us\n request length:%zu\n",(int)usec,length);


//  usleep(1000000);
    nr=length;
    nr=audbuff_get(&aud_buffer,wbuffer,length);
    fprintf(stderr,"after audbuff_get(%d)\n",nr);

  pa_stream_write(s, wbuffer, nr, NULL, 0LL, PA_SEEK_RELATIVE);
//  printf("  latency %8d us\n request length:%zu\n read stdin:%d\n",(int)usec,length,nr);

}

static void stream_underflow_cb(pa_stream *s, void *userdata) {
  // We increase the latency by 25% if we get 6 underflows and latency is under 2s
  // This is very useful for over the network playback that can't handle low latencies
  printf("underflow\n");
  underflows++;
  if (underflows >= 6 && latency < 2000000) {
    latency = (latency*5)/4;
    bufattr.maxlength = pa_usec_to_bytes(latency,&ss);
    bufattr.tlength = pa_usec_to_bytes(latency,&ss);  
    pa_stream_set_buffer_attr(s, &bufattr, NULL, NULL);
    underflows = 0;
    printf("latency increased to %d\n", latency);
//    usleep(30000);
  }
}




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
                    usleep(5000);
                    goto retry;
                }

                lastts=ts;
                //fprintf(stderr,"write audio[%d:%p,%d]..\n",wfd,buffer,nr);
//                write(wfd,buffer,nr);
                fprintf(stderr,"index:%d(uid:%d,sink:%d) - cached ts:%"PRIu64"  ..nr=%d audcnt:%d(%d) \n",index,uid,sink,ts,nr,audbuff_size(&aud_buffer),audbuff_space(&aud_buffer));
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

  pa_mainloop *pa_ml;
  pa_mainloop_api *pa_mlapi;
  pa_context *pa_ctx;
  pa_stream *playstream;
  int r;
  int pa_ready = 0;
  int retval = 0;
  unsigned int a;
  double amp;


    int err;

    char aud_name[128];

    pthread_t audfetchid;

    vdm_init(&vdm_main,index);

    audbuff_init(&aud_buffer,0x80000);


    pthread_create(&audfetchid,NULL,audio_fetch,&index);
//    fprintf(stderr,"AudioDispatcher(%d)[%s_%s]",argc,argv[0],argv[1]);

    usleep(100000);

    index1(index,&sink,&uid,8);

    snprintf(aud_name,sizeof aud_name,"audDispatcher.%d(uid:%d,sink:%d)",index,uid,sink);



  // Create a mainloop API and connection to the default server
  pa_ml = pa_mainloop_new();
  pa_mlapi = pa_mainloop_get_api(pa_ml);
  pa_ctx = pa_context_new(pa_mlapi, "Audio Dispatcher2 for VDM");
  pa_context_connect(pa_ctx, NULL, 0, NULL);

  // If there's an error, the callback will set pa_ready to 2
  pa_context_set_state_callback(pa_ctx, pa_state_cb, &pa_ready);

  // We can't do anything until PA is ready, so just iterate the mainloop
  // and continue
  while (pa_ready == 0) {
    pa_mainloop_iterate(pa_ml, 1, NULL);
  }
  if (pa_ready == 2) {
    retval = -1;
    goto exit;
  }

  ss.rate = 48000;
  ss.channels = 2;
  ss.format = PA_SAMPLE_S16NE;
  playstream = pa_stream_new(pa_ctx, "Playback", &ss, NULL);
  if (!playstream) {
    fprintf(stderr,"pa_stream_new failed\n");
  }

  pa_stream_set_write_callback(playstream, stream_request_cb, NULL);
  pa_stream_set_underflow_callback(playstream, stream_underflow_cb, NULL);
  bufattr.fragsize = (uint32_t)-1;
  bufattr.maxlength = pa_usec_to_bytes(latency,&ss);
  bufattr.minreq = pa_usec_to_bytes(0,&ss);
  bufattr.prebuf = (uint32_t)-1;
  bufattr.tlength = pa_usec_to_bytes(latency,&ss);
  r = pa_stream_connect_playback(playstream, NULL, &bufattr,
                                 PA_STREAM_INTERPOLATE_TIMING
                                 |PA_STREAM_ADJUST_LATENCY
                                 |PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL);

  if (r < 0) {
    fprintf(stderr,"pa_stream_connect_playback failed\n");
    retval = -1;
    goto exit;
  }

  // Iterate the main loop and go again.  The second argument is whether
  // or not the iteration should block until something is ready to be
  // done.  Set it to zero for non-blocking.
  while (1) {
    pa_mainloop_iterate(pa_ml, 1, NULL);
  }

exit:
  // clean up and disconnect
  pa_context_disconnect(pa_ctx);
  pa_context_unref(pa_ctx);
  pa_mainloop_free(pa_ml);
  return retval;


    
}


