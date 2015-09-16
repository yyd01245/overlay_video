#ifndef _H_VDMANAGER_
#define _H_VDMANAGER_

#include<errno.h>
#include<event2/listener.h>
#include<event2/bufferevent.h>
#include<event2/buffer.h>
#include<arpa/inet.h>




#include<string>
#include<iostream>

#include"Resource.h"
#include"logger.h"


using namespace std;

class VDManager{

    public:
        VDManager(string ip,int port);
        ~VDManager();

        DecodeResource*release_decode(DecodeResource*res);
        DecodeResource*obtain_decode(string url,int width,int height);
        DecodeResource*release_decode(string url,int width,int height);

        void cleanup_videostub();

        DecodeResource*get_decode_by_decoder_pid(int pid);
//        VideoResource*get_video_by_audio_pid(int pid);

        list<DecodeResource*>get_decode_by_index(int index);
        DecodeResource*get_decode_by_resid(int id);
//        DecodeResource*get_decode_by_index(int index);

        struct event_base*get_event_base(){
            return _ev_base;
        }

        logger_t*get_logger(){
            return &_log;
        }

        void print_manager(string desc);
        int start();


        void lock(){
//            pthread_mutex_lock(&_lock);
        }

        void unlock(){
//            pthread_mutex_unlock(&_lock);
        }
/*
        int get_break_fd(){
            return _fd_break[1];
        }
*/


    private:
//        bool _ok;//
        
        pthread_mutex_t _lock;
        logger_t _log;

        list<DecodeResource*> _resources;

        pthread_t _thr_sigproc;
//        VncStub _vncstubs;
//        VncShmStub*_vncstubs;

//        int _fd_break[2];
        struct event* _ev_break;


        struct event* _ev_chld;
        struct event* _ev_term;

        struct event_base* _ev_base;
        struct evconnlistener* _ev_listener;
        struct sockaddr_in _ev_addr;
        string _ev_ip;
        int _ev_port;

};






#endif
