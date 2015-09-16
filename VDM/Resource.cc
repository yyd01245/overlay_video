#include"Resource.h"
#include<arpa/inet.h>
#include<iostream>
#include<pthread.h>
#include<signal.h>
#include<sys/types.h>
#include<pwd.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<signal.h>
#include<fcntl.h>

#include"cJSON/cJSON.h"
#include"check.h"
#include"vdm.h"
#include"nvmlHelper.h"
#include"sysutils.h"



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



static int switch_user_by_index(int index)
{
    int ret=-1;
    char unam[16];

    int sink,uid;
    index1(index,&sink,&uid,8);

    snprintf(unam,sizeof(unam),"x%.2d",uid);

    struct passwd*urec=getpwnam(unam);
    _CHK_BEGIN_error(!urec,"getpwnam() failed")
        ret=-1; 
    _CHK_ELSE
        ret=setuid(urec->pw_uid);
        _CHK_BEGIN_error(ret<0,"Setuid() Failed..") 
            ret=-2;    
        _CHK_ELSE
            ret=setenv("HOME",urec->pw_dir,1);
        _CHK_END
 
    _CHK_END

    return ret;

}





//////////////////////////////////////
//////////////////////////////////////
int VideoResource::_s_resid=1000;

VideoResource::VideoResource(int index,int x,int y)
{
    _index=index;
    _x=x;
    _y=y;
    _vncstub=new VideoStub(index);
    _vresid=_s_resid++;

    _apid=0;

    VDR_LOG_DEBUG("New Video Resource:[%d|%d:%d]",index,x,y);
}


VideoResource::~VideoResource()
{

//    stop_audio();

    VDR_LOG_DEBUG("Delete Video Resource:[%d|%d:%d]",_index,_x,_y);

    delete _vncstub;

}


bool VideoResource::operator==(VideoResource&other)
{
    bool eq=_index==other._index &&
        _x==other._x &&
        _y==other._y;

    return eq;
    
}



void VideoResource::start_audio()
{
    
    VDR_LOG_DEBUG("Start Audio(%d)..",_apid);
    if(_apid!=0){
        return;
    }


    int ret;
    int pid;


    int sink;
    int uid;
    index1(_index,&sink,&uid,8);



    pid=fork();
    _CHK_BEGIN_error(pid<0,"Fork Audio Process Failed")
    _CHK_END


    if(pid==0){
        //audio process;
   
//        char aud_str[12];
        char index_str[12];
        char sink_str[16];
        char uid_str[16];
//        snprintf(aud_str,sizeof aud_str,PROC_AUDIO_PATH);
        snprintf(index_str,sizeof index_str,"%d",_index);

        const char*argv[10]={0,};
        int i=0;
        argv[i++]=PROC_AUDIO_PATH;
        argv[i++]=index_str;
        argv[i++]=0;

        snprintf(sink_str,sizeof sink_str,"%d",sink);
        snprintf(uid_str,sizeof uid_str,"%d",uid);

        fprintf(stderr,"Index:%d||Uid:%d|Sink:%d\n",_index,uid,sink);
        setenv("PULSE_SINK",sink_str,1);
        setenv("PULSE_LATENCY_MSEC","0",1);
        switch_user_by_index(_index);

#if 0
        execvp(PROC_AUDIO_PATH,(char*const*)argv);
#endif
        sleep(100000);

    }else{
        //parent        

        _apid=pid;
        VDR_LOG_DEBUG("AudioPlayer.%d :Pid[%d] Uid[%d] Sink[%d]..",_index,pid,uid,sink);

    }


}


void VideoResource::stop_audio()
{
    if(_apid){
        VDR_LOG_DEBUG("Stop Audio(%d)..",_apid);
//        DecodeResource*dr=get_decode();
        int ret=kill(_apid,SIGKILL);
        _apid=0;

    }
}



//////////////////////////////////////

int DecodeResource::generate_audiopath(string url){

    const char*p=url.c_str();

    const char*pc=p;
    int key=0;

    while(*pc){
        key=key*0xf+(uint32_t(*pc)*137)%(~0xf0000000);
        pc++;
    }

//    snprintf(_audfrag_path,sizeof _audfrag_path,"/tmp/vdm-audio-%06X",key);
    _audfrag_key=(key_t)key;

    VDR_LOG_MSG("Audio Key:[0x%08x]",_audfrag_key);
    return 0;
}



int DecodeResource::generate_videokey(string url,int width,int height){

    const char*p=url.c_str();

    const char*pc=p;
    int key=0;
    int co=width*height/2;

    while(*pc){
        key=key*0xf+(int(*pc)*co)%(~0xf0000000);
        pc++;
    }

    VDR_LOG_MSG("Video ShmKey:[0x%08x]",key);
//    fprintf(stderr,"Key:[0x%08x]\n",key);
    _vidsurf_key=(key_t)key;
    return 0;

}



DecodeResource::DecodeResource(string url,int width,int height)
{
#ifdef _ENABLE_REDUCE_DECODER_
    static int deccnt=0;
    char suffix[10];
    snprintf(suffix,sizeof suffix,"#%d",deccnt);
    url+=suffix;
    VDR_LOG_DEBUG("New DecodeResource:[%dx%d|%s]",width,height,url.c_str());
    deccnt++;
#else

    VDR_LOG_DEBUG("New DecodeResource:[%dx%d|%s]",width,height,url.c_str());
#endif
//    cerr<<"New DecodeResource()"<<endl;
    _res_url=url;
    _res_width=width;
    _res_height=height;

    _dec_pid=0;
    _audio_thr=false;
    create_ipc();

}


DecodeResource::~DecodeResource()
{
    VDR_LOG_DEBUG("Destroy DecodeResource:[%dx%d|%s]",_res_width,_res_height,_res_url.c_str());
//
    if(_dec_pid>0){
        VDR_LOG_DEBUG("To Kill Decode Process[%d]..",_dec_pid);

        kill(_dec_pid,SIGKILL);
    }

    if(_audio_thr){
        VDR_LOG_DEBUG("To Cancel Audio-Transfer Thread..");
        pthread_cancel(_audio_tid);
        _audio_thr=0;
    }

    VDR_LOG_DEBUG("To Delete VideoResources,total:%d",_vidresources.size());
    list<VideoResource*>::iterator it=_vidresources.begin();
    for(;it!=_vidresources.end();++it){
        _vidresources.erase(it);
        delete *it;
    }


    destroy_ipc();

}




int DecodeResource::create_ipc(){
    
    generate_videokey(_res_url,_res_width,_res_height);
    size_t shmsiz=sizeof(videosurf)+_res_width*_res_height*3/2;//yuv420
    _vidsurf_shmid=shmget(_vidsurf_key,shmsiz,IPC_CREAT|0666);
    _CHK_BEGIN_error(_vidsurf_shmid<0,"shmget() failed")
        return -1;
    _CHK_END

    generate_audiopath(_res_url);
    shmsiz=sizeof(audiofrag)+AUDBUFF_SIZE;//
    _audfrag_shmid=shmget(_audfrag_key,shmsiz,IPC_CREAT|0666);
    _CHK_BEGIN_error(_audfrag_shmid<0,"shmget() failed")
        return -1;
    _CHK_END
//

    _paudfrag=(audiofrag*)shmat(_audfrag_shmid,NULL,0);
    memset(_paudfrag,0,sizeof(audiofrag)+AUDBUFF_SIZE);
    _paudfrag->len=AUDBUFF_SIZE;
    _paudfrag->ts=0;

////////
    VDR_LOG_MSG("VideoKey :%d(shmid:%d)\n",_vidsurf_key,_vidsurf_shmid);
    VDR_LOG_MSG("AudioKey :%d(shmid:%d)\n",_audfrag_key,_audfrag_shmid);

    return 0;
}


void DecodeResource::destroy_ipc()
{
    shmctl(_vidsurf_shmid,IPC_RMID,NULL);
    shmctl(_audfrag_shmid,IPC_RMID,NULL);
//    close(_audfrag_fd);
//    unlink(_audfrag_path);
}


bool DecodeResource::operator==(DecodeResource&other)
{

    bool eq=this->_res_url==other._res_url &&
        this->_res_width==other._res_width &&
        this->_res_height==other._res_height;

    return eq;

}


void DecodeResource::print_videos(char*buff,int siz)
{

    snprintf(buff,siz,"\tDecodeResource:<url:%s|%dx%d> {\n",_res_url.c_str(),_res_width,_res_height);

    list<VideoResource*>::const_iterator it=_vidresources.begin();
    char vb[128];

    int i=0;
//    cerr<<"\tDecodeResource("<<_vidresources.size()<<") ["<<_res_url<<","<<_res_width<<"x"<<_res_height<<""<<"] [:"<<desc<<endl;
    for(;it!=_vidresources.end();++it){
        (*it)->present(vb,sizeof vb);
        snprintf_append(buff,siz,"\t\t%d: %s\n",i++,vb) ;
    }
    snprintf_append(buff,siz,"\t}\n");
    cerr<<buff<<endl;
}


VideoResource* DecodeResource::attach_video(VideoResource* vnc)
{
    int x,y;
    int ret;
    vnc->get_position(x,y);
    ret=vnc->get_stub()->set_resource(this,x,y);
    if(ret<0){
        return NULL; 
    }

    _vidresources.push_back(vnc);
    vnc->set_decode(this);

//    vnc->start_audio();

    return vnc;
}


VideoResource* DecodeResource::detach_video(VideoResource*vnc)
{
//    this->print_videos("Before Detach video");
    if(!vnc)
        return NULL;

    list<VideoResource*>::iterator it=_vidresources.begin();
    for(;it!=_vidresources.end();it++){
        if((*it)==vnc){
            break;
        }
    }
    if(it!=_vidresources.end()){
        
        _vidresources.erase(it);
        int x,y;
        vnc->get_position(x,y);
        (*it)->get_stub()->unset_resource(this,x,y);
//        vnc->stop_audio();
    }

//    this->print_videos("After Detach video");

    return vnc;
    
}


VideoResource* DecodeResource::seek_video_by_resid(int resid)
{
    list<VideoResource*>::const_iterator it=_vidresources.begin();
    for(;it!=_vidresources.end();it++){
        if((*it)->get_resid()==resid ){
            return *it;
        }
    }
    return NULL;
}



VideoResource*DecodeResource::seek_video(int index,int x,int y)
{

    VideoResource*vr=NULL;
    list<VideoResource*>::const_iterator it=_vidresources.begin();
    for(;it!=_vidresources.end();it++){
        int _x,_y;
        (*it)->get_position(_x,_y);
        if((*it)->get_index()==index &&
                            _x==x &&
                            _y==y ){

            vr=*it;
            break;
        }
    }

    return vr;
    
}



list<VideoResource*>DecodeResource::seek_video_by_index(int index)
{

    list<VideoResource*>vncs;
    VideoResource*vnc=NULL;

    list<VideoResource*>::const_iterator it=_vidresources.begin();
    for(;it!=_vidresources.end();it++){
        if((*it)->get_index()==index){
            vncs.push_back(*it);
        }
    }
    
    return vncs;
}



void*transfer_audio(void*arg)
{

    DecodeResource*dr=(DecodeResource*)arg;
    int nr;
    uint8_t audbuff[AUDBUFF_SIZE];
    uint64_t ts=1;
    int ntry=100;
retry:
    int fd=open(dr->get_audiopath(),O_RDONLY);
    if(fd<0){
        VDR_LOG_ERROR("Could not open %s for read {%s}..\n",dr->get_audiopath(),strerror(errno));
        usleep(500000);

        if(ntry){
            ntry--;    
            goto retry;
        }
        return NULL;
    }
    int offset=0;

    for(;;ts++){

        nr=read(fd,audbuff,AUDBUFF_SIZE);
        //fprintf(stderr,"---read pipe size=%d \n",nr);
        if(nr==0)
            break;
        if(nr<0 && errno==EAGAIN){
            usleep(10000);
            continue;
        }
        if(nr<AUDBUFF_SIZE){
            VDR_LOG_DEBUG("read audio less than AUDBUFF_SIZE fram fifo[%s]:{%s}",dr->get_audiopath(),strerror(errno));
            usleep(20000);
            continue;
        }
        //fprintf(stderr,"---put audio data ts=%d \n",ts);
        shmaudio_put(dr->get_audiofrag(),audbuff,AUDBUFF_SIZE,ts);
    }

    return NULL;
}


void DecodeResource::dispatch()
{

    if(_dec_pid==0){
        //create decoder...

        const char*argv[20];
        
        char url_str[1024];
        char audio_str[128];
        char vidkey_str[16];
        char audkey_str[16];
        char width_str[8];
        char height_str[8];
        char coreid_str[8];

        snprintf(vidkey_str,sizeof vidkey_str,"%d",_vidsurf_key);
        snprintf(audkey_str,sizeof audkey_str,"%d",_audfrag_key);
        snprintf(width_str,sizeof width_str,"%d",_res_width);
        snprintf(height_str,sizeof height_str,"%d",_res_height);
//        snprintf(audkey_str,sizeof audkey_str,"%d",_audfrag_key);

        int coreid=get_next_core();                                                                                                                                 
        snprintf(coreid_str,sizeof coreid_str,"%d",coreid);


        int i=0;
        argv[i++]=PROC_AVDECODER_PATH;
        argv[i++]="--inputurl";
//        argv[i++]="udp://@:12346";
#ifdef _ENABLE_REDUCE_DECODER_
        char realurl[128];
        strcpy(realurl,_res_url.c_str());
        char*p=strstr(realurl,"#");
        if(p)
            *p=0;
        argv[i++]=realurl;
#else
        argv[i++]=_res_url.c_str();
#endif
        argv[i++]="--videokey";
        argv[i++]=vidkey_str;
        argv[i++]="--audiokey";
        argv[i++]=audkey_str;
        argv[i++]="--coreid";
        argv[i++]=coreid_str;
        argv[i++]="--width";
        argv[i++]=width_str;
        argv[i++]="--height";
        argv[i++]=height_str;
        argv[i++]=0;


        for(i=0;argv[i];i++){
            fprintf(stderr, " %s",argv[i]);
        }
        fprintf(stderr, "\n");

        int pid;
        pid=fork();
        _CHK_BEGIN_error(pid<0,"fork() failed")
        _CHK_END

        if(pid==0){
            //child            
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
//            close(STDERR_FILENO);
            int ret=execvp(PROC_AVDECODER_PATH,(char*const*)argv);
            _CHK_BEGIN_error(ret<0,"execvp() failed")
            _CHK_END
            exit(188);
        }else{
            
            if(pid<0)
                pid=0;
            _dec_pid=pid;
        }
    }

    if(!_audio_thr){

//        pthread_create(&_audio_tid,NULL,transfer_audio,this);
//        _audio_thr=true;
    }






}




/////////////////////////////

int VideoStub::_nvnc=0;
int VideoStub::_nref=0;
bool VideoStub::_inited=false;
int VideoStub::_vidshmid=0;
int VideoStub::_vidshmsize=0;
key_t VideoStub::_vidshmkey=0;
void* VideoStub::_pvidshm=NULL;
/*
int VideoStub::_audshmid=0;
int VideoStub::_audshmsize=0;
key_t VideoStub::_audshmkey=0;
void* VideoStub::_pvidshm=NULL;
*/


void VideoStub::cleanup()
{
    memset(_pvidshm,0,_vidshmsize);
//    memset(_paudshm,0,_audshmsize);
//    __sync_synchronize();   
}


VideoStub::VideoStub(int index)
{
    if(!_inited){

        _nvnc=MAX_VNC_NUM;
        _vidshmkey=AVSTUB_KEY;
        _vidshmsize=_nvnc*sizeof(avstub);
        _vidshmid=shmget(_vidshmkey,_vidshmsize,IPC_CREAT|0666);
        _CHK_BEGIN_error(_vidshmid<0,"shmget() failed.")
        
            VR_LOG_MSG("VideoStub Init Failed{shmget}");
            return;
        _CHK_END
        _pvidshm=shmat(_vidshmid,NULL,0);
        _CHK_BEGIN_error(_pvidshm==(void*)-1,"shmat() failed.")

            VR_LOG_MSG("VideoStub Init Failed{shmat}");
            return;
        _CHK_END
        memset(_pvidshm,0,_vidshmsize);
//        __sync_synchronize();   

        
        VR_LOG_MSG("VideoStub Key   :[0x%08x]",_vidshmkey);
        VR_LOG_MSG("VideoStub shmid :[%d]",_vidshmid);
        VR_LOG_MSG("VideoStub addr  :[%p]",_pvidshm);
//---------------------------------------------
/*
        _audshmkey=AUDSTUB_KEY;
        _audshmsize=_nvnc*sizeof(struct audiostub);
        _audshmid=shmget(_audshmkey,_audshmsize,IPC_CREAT|0666);
        _CHK_BEGIN_error(_audshmid<0,"shmget() failed.")
        
            VR_LOG_MSG("VideoStub Init Failed{shmget}");
            return;
        _CHK_END
        _paudshm=shmat(_audshmid,NULL,0);
        _CHK_BEGIN_error(_paudshm==(void*)-1,"shmat() failed.")

            VR_LOG_MSG("VideoStub Init Failed{shmat}");
            return;
        _CHK_END
        memset(_paudshm,0,_audshmsize);
//        __sync_synchronize();   
        
        VR_LOG_MSG("AudioStub Key   :[0x%08x]",_audshmkey);
        VR_LOG_MSG("AudioStub shmid :[%d]",_audshmid);
        VR_LOG_MSG("AudioStub addr  :[%p]",_paudshm);
*/

        _inited=true;
    }

    _areas=(avstub*)_pvidshm+index;
    _area_idx=index;
//    _channs=(struct audiostub*)_paudshm+index;
//    _chann_idx=index;

    _nref++;



    VR_LOG_MSG("+VideoStub :%d@[%p] .nref:%d",_area_idx,_areas,_nref);

}


VideoStub::~VideoStub()
{
    _nref--;

    VR_LOG_MSG("-VideoStub :%d@[%p] .nref:%d",_area_idx,_areas,_nref);

    if(_nref==0){
        VR_LOG_MSG("To Cleanup VideoStub");
        shmdt(_areas);
        _inited=false;
    }
}

int VideoStub::set_resource(DecodeResource*res,int x,int y)
{

    int ret;
    fprintf(stderr,"Set_Resource:(%d,%d)\n",res->get_videoshmid(),res->get_audioshmid());
    ret=videostub_set_resource(_areas,res->get_videoshmid(),x,y,res->get_audioshmid());
//    ret+=audiostub_set_resource(_channs,res->get_audioshmid());

    return ret;
}

int VideoStub::unset_resource(DecodeResource*res,int x,int y)
{
    int ret;
    ret=videostub_unset_resource(_areas,res->get_videoshmid(),x,y);
    return ret;
    /*
    if(!ret)
        return false;

    return audiostub_unset_resource(_channs,res->get_audioshmid())?true:false;
    */
}



void VideoStub::print()
{
    cerr<<"VncArea["<<_area_idx<<"]:("<<_areas->on<<") ";
    for(int i=0;i<MAX_AV_NUM;i++){
        cerr<<"shmid:"<<_areas->areas[i].shmid<<":: x:"<<_areas->areas[i].x<<",y:"<<_areas->areas[i].y<<".."<<endl;
    }

}




