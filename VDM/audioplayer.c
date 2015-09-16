#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<pwd.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>

#include<pulse/pulseaudio.h>
#include<pulse/simple.h>

pa_simple *s;
pa_sample_spec ss;
#define AUD_FRAGSIZ 4608

static char buffer[AUD_FRAGSIZ];

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

/*
typedef struct _buf{
   
    char*buf;
    int rdcount;
    int wrcount;
    int size;

}lbuff;


void lbuff_init(lbuff*b,int size){
    
    b->buf=calloc(1,size);
    b->rdcount=0;
    b->wrcount=0;
    b->size=size;
}


int lbuff_read(lbuff*b,int fd,int len){
   
    read(fd,b->buf+)


}
*/

/*
static int switch_user_by_index(int index)
{
    int ret;
    char unam[16];

    int sink,uid;
    index1(index,&sink,&uid,8);

    snprintf(unam,sizeof(unam),"x%.2d",uid);

    struct passwd*urec=getpwnam(unam);
    if(urec){
        ret=setuid(urec->pw_uid);

        ret=setenv("HOME",urec->pw_dir,1);
    }else{
        fprintf(stderr,"No such user..\n");
        sink=-1;
//        cerr<<"No such User["<<unam<<"]"<<endl;
    }

    return sink;

}

*/


int main(int argc,char**argv)
{

    if(argc<2)
        exit(-1);

    int index=atoi(argv[1]);
    int sink;
    int uid;

    int err;

    char aud_name[128];

/*
    index1(index,&sink,&uid,8);
    int ret=switch_user_by_index(index);
    if(ret<0){
        exit(-2);
    }
*/

    fprintf(stderr,"start audioplay %d..\n",index) ;

    index1(index,&sink,&uid,8);

    snprintf(aud_name,sizeof aud_name,"audPlayer.%d(uid:%d,sink:%d)",index,uid,sink);

    ss.format = PA_SAMPLE_S16NE;
    ss.channels = 2;
    ss.rate = 48000;

    s = pa_simple_new(NULL,aud_name,PA_STREAM_PLAYBACK,NULL,"dispatch audio for VDM",&ss,NULL,NULL,NULL);
    if(!s){
        fprintf(stderr,"pa_simple_new() failed :%s\n",strerror(errno));
        return -1;
    }

//    fprintf(stderr,"Latency is:%ld\n",pa_simple_get_latency(s,NULL));

#if 1
    int rfd=open("/dev/urandom",O_RDONLY);

    close(STDERR_FILENO);
    dup2(rfd,STDIN_FILENO);
#endif


//    int wwfd=open("cap.pcm",O_CREAT|O_TRUNC|O_WRONLY,0666);


    while(1){

        int nr,nw;

        nr=read(STDIN_FILENO,buffer,sizeof buffer);
        if(nr<0){
            fprintf(stderr,"read pcm from stdin fail..\n") ;
            continue;
        }
        if(nr==0){
            fprintf(stderr,"read EOF\n") ;
            break; 
        }
/*        if(nr!=sizeof buffer){
            fprintf(stderr,"read less from stdin..\n") ;
            continue;
        }
*/
//        write(wwfd,buffer,nr);

//        fprintf(stderr,"-(%d)\n",nr);

#if 0

        nw=write(STDOUT_FILENO,buffer,nr);
        fprintf(stderr,"read:%d, write:%d\n",nr,nw);
        fprintf(stderr,"[%s]",buffer);
#else
        nw=pa_simple_write(s,buffer,nr,&err);
        if(nw<0){
            fprintf(stderr, " pa_simple_write() failed: %s\n", pa_strerror(err));
            goto finish;
        }
//        fprintf(stderr,"pa_simple_write(%d)\n",nr);

#endif
    }

    if (pa_simple_drain(s, &err) < 0) {
        fprintf(stderr, " pa_simple_drain() failed: %s\n", pa_strerror(err));
        goto finish;
                            
    }


finish:
    pa_simple_free(s);


    return 0;
}


