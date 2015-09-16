
#include"VDManager.h"
#include"cJSON/cJSON.h"
#include"check.h"
#include"sysutils.h"

#include"vdm.h"

#include<signal.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<pthread.h>

/////////////////////////////////////////////////////


static cJSON*add_decoder(cJSON*json,void*arg){

//    cerr<<"Add_Decoder()"<<endl;
    VDM_LOG_DEBUG("do Add Decoder");

    VDManager*dm=reinterpret_cast<VDManager*>(arg);



    char buffer[128];
    cJSON*pit;
    int width=-1,height=-1,x=-1,y=-1,vncindex=-1;
    const char*url;
    pit=cJSON_GetObjectItem(json,"addr");
    if(pit){
        url=pit->valuestring; 
    }
    pit=cJSON_GetObjectItem(json,"width");
    if(pit){
        width=atoi(pit->valuestring); 
    }
    pit=cJSON_GetObjectItem(json,"height");
    if(pit){
        height=atoi(pit->valuestring); 
    }
    pit=cJSON_GetObjectItem(json,"x");
    if(pit){
        x=atoi(pit->valuestring); 
    }
    pit=cJSON_GetObjectItem(json,"y");
    if(pit){
        y=atoi(pit->valuestring); 
    }
    pit=cJSON_GetObjectItem(json,"vncindex");
    if(pit){
        vncindex=atoi(pit->valuestring); 
    }
    
//    fprintf(stderr,"ppppppppppppppppppp\n");

    _CHK_BEGIN(x==-1&&y==-1&&width==-1&&height==-1&&vncindex==-1,"Malformed json")
        VDM_LOG_ERROR("Malformed JSON");
        strcpy(buffer,"{\"cmd\":\"adddecoder\",\"retcode\":\"-1\"}");
        return cJSON_Parse(buffer);
    _CHK_END

    if(x%2||y%2){
        VDM_LOG_WARN("VideoArea's Position not align to 2..");
        x=x&(~1); 
        y=y&(~1); 
    }

    VDM_LOG_WARN("VideoArea's Position @ (x:%d,y:%d)",x,y);

    int retcode=0;

    VideoResource*vr;
    DecodeResource*res;

    dm->lock();

    list<DecodeResource*> ldr=dm->get_decode_by_index(vncindex);
    list<DecodeResource*>::const_iterator cit=ldr.begin();
    bool exist=false;
    for(;cit!=ldr.end();++cit){
        vr=(*cit)->seek_video(vncindex,x,y);
        if(vr){
            exist=true;
            break;
        }
    }

    if(exist){
        VDM_LOG_WARN("VideoResource<idx:%d,x:%d,y:%d> already Exists..",vncindex,x,y); 
        retcode=-88;

    }else{
    
        VDM_LOG_MSG("New VideoResource<idx:%d,x:%d,y:%d>..",vncindex,x,y); 
    
        //create new DecodeResource or return an already created DecodeResource
        res=dm->obtain_decode(url,width,height);
    
        vr=new VideoResource(vncindex,x,y);
        //attach current vnc to specified DecodeResource
        res->attach_video(vr);

    }
    dm->unlock();

    res->dispatch();

    dm->print_manager("After Add Decoder");
/////
    snprintf(buffer,sizeof buffer,"{\"cmd\":\"adddecoder\",\"retcode\":\"%d\",\"id\":\"%d\"}",retcode,vr->get_resid());
    return cJSON_Parse(buffer);

}

static cJSON*del_decoder(cJSON*json,void*arg){

    VDM_LOG_DEBUG("do Delete Decoder");

    VDManager*dm=reinterpret_cast<VDManager*>(arg);

    list<DecodeResource*>dres;
    char buffer[128];
    int vncindex=-1;
    int resid;
    int retcode=0;

    cJSON*pit;


    pit=cJSON_GetObjectItem(json,"resid");
    if(pit){

        resid=atoi(pit->valuestring); 
        VDM_LOG_DEBUG("Found resid(%d)..",resid);

        DecodeResource*dr=dm->get_decode_by_resid(resid);
        _CHK_BEGIN(!dr,"No Such DecodeResource has VideoResource of resid(%d)",resid)
            VDM_LOG_DEBUG("No Such DecodeResources resid:%d",resid);
            retcode=-8;
            goto fail;
        _CHK_END

        VideoResource*dv=dr->seek_video_by_resid(resid);
        _CHK_BEGIN(!dv,"No Such VideoSource of resid(%d)",resid)
            VDM_LOG_DEBUG("No VideoResource has resid:%d",resid);
            retcode=-18;
            goto fail;
        _CHK_END
    
        dr->detach_video(dv);
        if(dr->get_video_count()==0){
            dm->release_decode(dr);
            delete dr;
        }
        delete dv;

    }else{
        
        pit=cJSON_GetObjectItem(json,"vncindex");
        if(pit){
        
            vncindex=atoi(pit->valuestring); 
            
            dres=dm->get_decode_by_index(vncindex);

            VDM_LOG_DEBUG("%d DecodeResource has index %d",dres.size(),vncindex);
        
            dm->lock();
            for(list<DecodeResource*>::iterator it=dres.begin();it!=dres.end();++it){
                DecodeResource*s=*it;
        
                list<VideoResource*>vnclist=s->seek_video_by_index(vncindex);
                if(vnclist.empty())
                    continue;
        
                while(!vnclist.empty()){
                    VideoResource*vnc=vnclist.front();
            
                    s->detach_video(vnc);
                    if(s->get_video_count()==0){
                        VDM_LOG_DEBUG("To Release DecodeResource");
//                        cerr<<"To Release Decode"<<endl;
                        dm->release_decode(s);
                        delete s;
                    }
            
                    vnclist.pop_front();
                    delete vnc;
                }
            }
            dm->unlock();

        }else{
            retcode=-3;
            goto fail;
        }

    }

////////////////////////////////

fail:

    dm->print_manager("After Del Decoder");

    snprintf(buffer,sizeof buffer,"{\"cmd\":\"deldecoder\",\"retcode\":\"%d\"}",retcode);
    return cJSON_Parse(buffer);

}



static void
my_write_cb(struct bufferevent *bev, void *ctx)
{
    
//        fprintf(stderr,"my_write_cb() called\n");
        bufferevent_free(bev);
}


static void
echo_read_cb(struct bufferevent *bev, void *ctx)
{

    VDManager*dm=reinterpret_cast<VDManager*>(ctx);

        /* This callback is invoked when there is data to read on bev. */
        struct evbuffer *input = bufferevent_get_input(bev);
        struct evbuffer *output = bufferevent_get_output(bev);

        char buffer[1024];
        char rbuffer[1024];
        strcpy(rbuffer,"{}XXEE");

        cJSON*pjson,*retjson,*pit;

        int nr=evbuffer_remove(input,buffer,sizeof buffer);
        buffer[nr]=0;

//        cerr<<"\nRecv message[\n"<<buffer<<"\n] from client..."<<endl;


#if 1

        char*strjson=strstr(buffer,"\r\n\r\n{");
        if(strjson){
        //filter out http header
            strjson+=4;
        }else{
            strjson=buffer;
        }

#else
        char*strjson=buffer;

#endif

        VDM_LOG_DEBUG("Recv Message [\n%s\n]",strjson);

        
        char*pdelimiter=strstr(buffer,"XXEE");
        if(!pdelimiter){
//            cerr<<"no XXEE found!!"<<endl;
            VDM_LOG_DEBUG("No `XXEE' found!!");
            goto fail;
        }
        *pdelimiter=0;

        pjson=cJSON_Parse(strjson);
        if(!pjson){
            VDM_LOG_DEBUG("Could not Parse[%s]!!",strjson);
            goto fail;
        }

        pit=cJSON_GetObjectItem(pjson,"cmd");
        _CHK_BEGIN(!pit,"No `cmd' field found")
            VDM_LOG_DEBUG("No `cmd' field found");
            cJSON_Delete(pjson);
            goto fail;
        _CHK_END



        if(!strcmp(pit->valuestring,"adddecoder")){
            
            retjson=add_decoder(pjson,ctx);

        }else
        if(!strcmp(pit->valuestring,"deldecoder")){

            retjson=del_decoder(pjson,ctx);

        }else{
            retjson=NULL; 
            VDM_LOG_DEBUG("Unknown CMD[%s]",pit->valuestring);
        }

        cJSON_Delete(pjson);

        if(retjson){
            snprintf(rbuffer,sizeof rbuffer,"%sXXEE",cJSON_PrintUnformatted(retjson));
            cJSON_Delete(retjson);
        }

fail:
        // {}XXEE
        evbuffer_add(output,rbuffer,strlen(rbuffer));

        bufferevent_flush(bev,EV_WRITE,BEV_FINISHED);
//        int fd = bufferevent_getfd(bev);
//        evutil_closesocket(fd);

        bufferevent_setcb(bev, NULL, my_write_cb, NULL, ctx);

        /* Copy all the data from the input buffer to the output buffer. */
//        evbuffer_add_buffer(output, input);
}

static void
echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
    fprintf(stderr,"echo_event_cb() called\n");
        if (events & BEV_EVENT_ERROR)
                perror("Error from bufferevent");
        if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
                bufferevent_free(bev);
        }
}

static void
accept_conn(struct evconnlistener *listener,
    evutil_socket_t fd, struct sockaddr *address, int socklen,
    void *ctx)
{
        /* We got a new connection! Set up a bufferevent for it. */
        struct event_base *base = evconnlistener_get_base(listener);
        struct bufferevent *bev = bufferevent_socket_new(
                base, fd, BEV_OPT_CLOSE_ON_FREE);

        bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, ctx);

        bufferevent_enable(bev, EV_READ|EV_WRITE);
//        bufferevent_set_timeouts(bev,NULL,&to);

}



static void
accept_error(struct evconnlistener *listener, void *ctx)
{
        struct event_base *base = evconnlistener_get_base(listener);
        int err = EVUTIL_SOCKET_ERROR();
        fprintf(stderr, "Got an error %d (%s) on the listener. "
                "Shutting down.\n", err, evutil_socket_error_to_string(err));

        event_base_loopexit(base, NULL);
}

logger_t* g_log;

VDManager::VDManager(string ip,int port)
{

//    _ok=false;

    pthread_mutexattr_t la;
    pthread_mutexattr_settype(&la,PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&_lock,&la);
    pthread_mutexattr_destroy(&la);

    logger_init(&_log);

    logger_user_init(&_log,"log/vdm.log");

    g_log=&_log;

    VDM_LOG_MSG("\n\tVDM (%s)",_VDM_VERSION);


    memset(&_ev_addr,0,sizeof _ev_addr);

    _ev_addr.sin_family=AF_INET;
    inet_pton(AF_INET,ip.c_str(),&_ev_addr.sin_addr);
    _ev_addr.sin_port=htons(port);


    _ev_base=event_base_new();
    _CHK_BEGIN_error(!_ev_base,"event_base_new() failed.")
        return;
    _CHK_END

    _ev_listener=evconnlistener_new_bind(_ev_base,accept_conn,this,LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_EXEC,-1,(struct sockaddr*)&_ev_addr,sizeof _ev_addr);
    _CHK_BEGIN_error(!_ev_listener,"evconnlistener_new_bind() failed.")
        return;
    _CHK_END

    evconnlistener_set_error_cb(_ev_listener,accept_error);

    cleanup_videostub();
    cerr<<"New VDManage"<<endl;

}

VDManager::~VDManager()
{

    cerr<<"Delete VDManager"<<endl;

    cleanup_videostub();

    lock();
    list<DecodeResource*>::iterator it=_resources.begin();
    for(;it!=_resources.end();++it){
        DecodeResource*dr=*it;
//        _resources.erase(it);
        delete dr;
    }
    unlock();
//    _resources.clear();
//

//    pthread_cancel(_thr_sigproc);
    logger_fini(&_log);

    pthread_mutex_destroy(&_lock);

    event_base_free(_ev_base);
}




DecodeResource*VDManager::obtain_decode(string url,int width,int height)
{
    VDM_LOG_DEBUG("Obtain_decode(%s,%d,%d)",url.c_str(),width,height);
    lock();

    DecodeResource*res=NULL;
    list<DecodeResource*>::iterator it=_resources.begin();

    for(;it!=_resources.end();++it){
        int w,h;
        (*it)->get_geometry(w,h);

        if((*it)->get_url()==url && w==width && h==height){
            //find
            break;
        }
    }
#ifdef _ENABLE_REDUCE_DECODER_
    if(it!=_resources.end()){
        res=*it;
        VDM_LOG_DEBUG("Already Allocated DecodeResource..");
    }else{//not exists..
#endif
        res=new DecodeResource(url,width,height);
        _resources.push_back(res);
        VDM_LOG_DEBUG("Allocate New DecodeResource..");

#ifdef _ENABLE_REDUCE_DECODER_
    }
#endif
    unlock();

    return res;
}

DecodeResource*VDManager::release_decode(string url,int width,int height)
{

    VDM_LOG_DEBUG("Release_decode(%s,%d,%d)",url.c_str(),width,height);
    lock();

    list<DecodeResource*>::iterator it=_resources.begin();

    DecodeResource*dr;
    for(;it!=_resources.end();++it){
        dr=*it;
        int _width,_height;
        dr->get_geometry(_width,_height);

        if(dr->get_url()==url &&
                _width==width &&
                _height==height ){
            //find
            break;
        }
    }

    if(it!=_resources.end()){
        dr=*it;
        _resources.erase(it);
        unlock();
        return dr;
    }else{
        unlock();
        return NULL;
    }

}




DecodeResource* VDManager::get_decode_by_resid(int id)
{
    lock();

    VDM_LOG_DEBUG("Get_decode_by_resid(%d)",id);
    DecodeResource*res=NULL;

    list<DecodeResource*>::const_iterator it=_resources.begin();

    for(;it!=_resources.end();it++){
        
        list<VideoResource*>*vncs=(*it)->get_videos();
        list<VideoResource*>::const_iterator vit=vncs->begin();

        for(;vit!=vncs->end();vit++){
            if((*vit)->get_resid()==id){
                res=*it;
                break;
            }
        }
    }

    unlock();
    return res;
}




void VDManager:: print_manager(string desc)
{
    
    lock();
    list<DecodeResource*>::const_iterator it=_resources.begin();

    char buff[1024];
    char rbuff[512];
    snprintf(buff,sizeof buff,"Decode Manager (size:%d){ :: %s\n",_resources.size(),desc.c_str());
    int i=1;
    for(;it!=_resources.end();it++){
        (*it)->print_videos(rbuff,sizeof rbuff);
        snprintf_append(buff,sizeof buff," %d:%s\n",i,rbuff);
    }

    snprintf_append(buff,sizeof buff,"}\n");

    VDM_LOG_DEBUG("\n\033[33m%s\033[0m\n",buff);


    unlock();

}



list<DecodeResource*>VDManager::get_decode_by_index(int index)
{

    VDM_LOG_DEBUG("Get_decode_by_index(%d)",index);
    lock();

    list<DecodeResource*>ress;

    list<DecodeResource*>::const_iterator it=_resources.begin();

    for(;it!=_resources.end();it++){
        
        list<VideoResource*>*vncs=(*it)->get_videos();
        list<VideoResource*>::iterator vit=vncs->begin();

        for(;vit!=vncs->end();vit++){
//            cerr<<"----["<<(*vit)->get_index()<<"]"<<endl;
            if((*vit)->get_index()==index){
                ress.push_back(*it);
            }
        }
    }

    unlock();
//    cerr<<"sizeof decode by_index:"<<ress.size()<<endl;
    return ress;
}


DecodeResource*VDManager::get_decode_by_decoder_pid(int pid)
{
    
//    VDM_LOG_DEBUG("Get_decode_by_decode_pid(%d)",pid);
    lock();

    DecodeResource*res=NULL;
    list<DecodeResource*>::const_iterator it=_resources.begin();

    for(;it!=_resources.end();++it){
        if(pid==(*it)->get_decpid())
            break;
    }

    if(it!=_resources.end()){
        res=*it;
    }
    unlock();

    return res;
}


DecodeResource*VDManager::release_decode(DecodeResource*res)
{
    VDM_LOG_DEBUG("Release_decode()");
    lock();

    list<DecodeResource*>::iterator it=_resources.begin();

    for(;it!=_resources.end();++it){
        if(*res==**it){
            //find
            break;
        }
    }

    if(it!=_resources.end()){
        _resources.erase(it);
    }else{
        res=NULL;
    }

    unlock();
    return res;
}
 

static void*_sigprocess(void*arg)
{

    VDManager*vdm=reinterpret_cast<VDManager*>(arg);

    pthread_detach(pthread_self());
    pthread_setname_np(pthread_self(),"SigProc");

    sigset_t set,oset;
    sigemptyset(&set);
    sigaddset(&set,SIGTERM);
    sigaddset(&set,SIGUSR1);
    sigaddset(&set,SIGCHLD);


    int running=1;
    while(running){

        int signo;
        int status;
        int ret=sigwait(&set, &signo);
        _CHK_BEGIN_error(ret<0,"sigwait()")
        _CHK_END

//        VDM_LOG_DEBUG("sigwait return %d.",ret);


        switch(signo){

            case SIGCHLD:
                {
                    VDM_LOG_DEBUG("caught SIGCHLD..");
                    int pid=waitpid(-1,&status,0);

                    break;
                }

            case SIGTERM:
            case SIGUSR1:
                {
                    VDM_LOG_DEBUG("caught SIGTERM..");
//                    int ret=event_base_loopbreak(vdm->get_event_base());
//                    cerr<<"event_base_loopbreak return "<<ret<<endl;
                    char b;
//                    write(vdm->get_break_fd(),&b,1);
                    vdm->cleanup_videostub();
                    running=0;

                    break;
                }

            default:
                {

                }
        }

    }

    return NULL;
}

void do_sig_chld(evutil_socket_t fd, short what, void *ptr)
{

    VDM_LOG_DEBUG("Dispose SIGCHLD Event");

    int status;
    int pid=waitpid(-1,&status,0);
    VDManager*vdm=(VDManager*)ptr;

}



void do_sig_term(evutil_socket_t fd, short what, void *ptr)
{

    event_base*base=(event_base*)ptr;
    VDM_LOG_DEBUG("To Exit Event Dispatch");
    event_base_loopbreak(base);

}


int VDManager::start()
{
/*
    pipe(_fd_break);
    _ev_break=event_new(_ev_base,_fd_break[0],EV_READ,break_cb,_ev_base);
    
    event_add(_ev_break,NULL);
*/

    _ev_term=evsignal_new(_ev_base,SIGTERM,do_sig_term,_ev_base);
    evsignal_add(_ev_term,NULL);
#if 1
    _ev_chld=evsignal_new(_ev_base,SIGCHLD,do_sig_chld,this);
    evsignal_add(_ev_chld,NULL);
#endif

    sigset_t set,oset;
    sigfillset(&set);
    sigdelset(&set,SIGTERM);
    sigdelset(&set,SIGCHLD);
    pthread_sigmask(SIG_BLOCK,&set,&oset);

//    cleanup_videostub();
#if 0
    pthread_create(&_thr_sigproc,NULL,_sigprocess,this);
#endif

    event_base_dispatch(_ev_base);
    cerr<<"dispatch exited"<<endl;
    pthread_sigmask(SIG_SETMASK,&oset,NULL);
//    cleanup_videostub();

//    event_free(_ev_break);
    
}



void VDManager::cleanup_videostub()
{
    cerr<<"clean up Video Stub..."<<endl;
    VideoStub::cleanup();
}


